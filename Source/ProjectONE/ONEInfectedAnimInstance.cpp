#include "ONEInfectedAnimInstance.h"
#include "ONEZombie.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_PoseSnapshot.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "BoneControllers/AnimNode_ModifyBone.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

namespace ONEInfectedAnimationDetails
{
    const TCHAR* RequiredClips[]={TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("TurnLeft"),TEXT("TurnRight"),
        TEXT("SwipeLeft"),TEXT("SwipeRight"),TEXT("RakeLeft"),TEXT("RakeRight"),TEXT("TwoHand"),
        TEXT("HeavyHit"),TEXT("Stumble"),TEXT("GetUpProne"),TEXT("GetUpSupine")};

    struct FSequenceEvaluator : FAnimNode_SequenceEvaluator_Standalone
    {
        bool bLoop=false;
        virtual bool IsLooping() const override { return bLoop; }
        virtual bool SetShouldLoop(bool Value) override { bLoop=Value; return true; }
    };

    struct FStepLeg : FAnimNode_TwoBoneIK
    {
        FVector Correction=FVector::ZeroVector;
        virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,TArray<FBoneTransform>& Transforms) override
        {
            const auto& Bones=Output.Pose.GetPose().GetBoneContainer();
            const FVector Foot=Output.Pose.GetComponentSpaceTransform(IKBone.GetCompactPoseIndex(Bones)).GetLocation();
            const FVector Knee=Output.Pose.GetComponentSpaceTransform(CachedLowerLimbIndex).GetLocation();
            const FVector Hip=Output.Pose.GetComponentSpaceTransform(CachedUpperLimbIndex).GetLocation();
            const FVector Axis=(Foot-Hip).GetSafeNormal();
            FVector Bend=Knee-(Hip+Axis*FVector::DotProduct(Knee-Hip,Axis));
            if (Bend.IsNearlyZero()) Bend=FVector::ForwardVector;
            EffectorLocation=Foot+Correction;
            JointTargetLocation=Knee+Bend.GetSafeNormal()*30.f;
            FAnimNode_TwoBoneIK::EvaluateSkeletalControl_AnyThread(Output,Transforms);
        }
    };

    struct FProxy : FAnimInstanceProxy
    {
        FSequenceEvaluator Idle,Walk,Run,Turn,Action;
        FAnimNode_TwoWayBlend Gait,Locomotion,Turning,ActionBlend,RecoveryBlend,Final;
        FAnimNode_PoseSnapshot PhysicsPose,RecoveryPose;
        FAnimNode_ConvertLocalToComponentSpace Component;
        FAnimNode_ConvertComponentToLocalSpace Local;
        FAnimNode_ModifyBone Spine,Neck,ShoulderLeft,ShoulderRight;
        FStepLeg Feet[2];
        FONEInfectedPhaseCursor FootCursor;
        FONEInfectedPhaseCursor AttackCursor;
        TArray<FONEInfectedPhaseMarker> FootMarkers;
        double GaitTurns=0,IdleTime=0;
        float SmoothedSpeed=0,PreviousYaw=0,TurnTime=0;
        bool bInitialized=false;
        uint64 SeenGeneration=0;
        uint64 SeenActionSerial=MAX_uint64;
        EONEInfectedMotionState PreviousMotion=EONEInfectedMotionState::Locomotion;
        FProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance)
        {
            Gait.A.SetLinkNode(&Walk); Gait.B.SetLinkNode(&Run);
            Locomotion.A.SetLinkNode(&Idle); Locomotion.B.SetLinkNode(&Gait);
            Turning.A.SetLinkNode(&Locomotion); Turning.B.SetLinkNode(&Turn);
            ActionBlend.A.SetLinkNode(&Turning); ActionBlend.B.SetLinkNode(&Action);
            Component.LocalPose.SetLinkNode(&ActionBlend);
            Spine.ComponentPose.SetLinkNode(&Component);
            Neck.ComponentPose.SetLinkNode(&Spine);
            ShoulderLeft.ComponentPose.SetLinkNode(&Neck);
            ShoulderRight.ComponentPose.SetLinkNode(&ShoulderLeft);
            Spine.BoneToModify.BoneName=TEXT("spine_01"); Neck.BoneToModify.BoneName=TEXT("neck");
            ShoulderLeft.BoneToModify.BoneName=TEXT("upperarm_r"); ShoulderRight.BoneToModify.BoneName=TEXT("upperarm_l");
            for (auto* Node:{&Spine,&Neck,&ShoulderLeft,&ShoulderRight})
            { Node->RotationMode=BMM_Additive; Node->RotationSpace=BCS_ComponentSpace; Node->SetAlpha(0.f); }
            Feet[0].ComponentPose.SetLinkNode(&ShoulderRight); Feet[1].ComponentPose.SetLinkNode(&Feet[0]);
            for (int32 I=0;I<2;++I)
            {
                Feet[I].IKBone.BoneName=I==0?TEXT("foot_r"):TEXT("foot_l");
                Feet[I].bAllowStretching=false;
                Feet[I].EffectorLocationSpace=BCS_ComponentSpace;
                Feet[I].JointTargetLocationSpace=BCS_ComponentSpace;
                Feet[I].SetAlpha(0.f);
            }
            Local.ComponentPose.SetLinkNode(&Feet[1]);
            RecoveryPose.Mode=ESnapshotSourceMode::SnapshotPin;
            PhysicsPose.Mode=ESnapshotSourceMode::SnapshotPin;
            RecoveryBlend.A.SetLinkNode(&Local); RecoveryBlend.B.SetLinkNode(&RecoveryPose);
            Final.A.SetLinkNode(&RecoveryBlend); Final.B.SetLinkNode(&PhysicsPose);
            // In the authored source, _l plants at phase zero, _r at .5.
            FONEInfectedPhaseMarker R; R.TimeSeconds=0.f; R.Event=EONEInfectedMotionEvent::RightFootPlant;
            FONEInfectedPhaseMarker L; L.TimeSeconds=.5f; L.Event=EONEInfectedMotionEvent::LeftFootPlant;
            FootMarkers={R,L};
        }
        virtual FAnimNode_Base* GetCustomRootNode() override { return &Final; }
        static void Sample(FAnimNode_SequenceEvaluator_Standalone& Node,UAnimSequence* Clip,float Time,bool Loop)
        {
            if (!Clip) return;
            Node.SetSequence(Clip); Node.SetTeleportToExplicitTime(true); Node.SetShouldLoop(Loop);
            const float Length=FMath::Max(.001f,Clip->GetPlayLength());
            Node.SetExplicitTime(Loop?FMath::Fmod(FMath::Max(Time,0.f),Length):FMath::Clamp(Time,0.f,Length-.0001f));
        }
        virtual void PreUpdate(UAnimInstance* Instance,float Dt) override
        {
            FAnimInstanceProxy::PreUpdate(Instance,Dt);
            auto* Anim=Cast<UONEInfectedAnimInstance>(Instance);
            auto* Z=Cast<AONEZombie>(Instance->TryGetPawnOwner());
            if (!Anim || !Z || !FMath::IsFinite(Dt) || Dt<0) return;
            const FONEInfectedAnimationState State=Z->GetInfectedAnimationState();
            const float Yaw=Z->GetActorRotation().Yaw;
            if (!bInitialized)
            { GaitTurns=FMath::Frac(FMath::Max(0.f,State.GaitPhaseOffset)); PreviousYaw=Yaw; bInitialized=true; }
            if (State.Motion!=PreviousMotion || SeenGeneration!=Anim->MotionEventGeneration)
            {
                FootCursor.Reset(); AttackCursor.Reset(); Anim->ResetMotionEvents(); SeenGeneration=Anim->MotionEventGeneration;
                PreviousMotion=State.Motion;
            }
            const bool Physical=State.Motion==EONEInfectedMotionState::Falling || State.Motion==EONEInfectedMotionState::Fallen || State.Motion==EONEInfectedMotionState::Dead;
            PhysicsPose.Snapshot=Anim->CapturedDeathPose; PhysicsPose.PreUpdate(Instance);
            RecoveryPose.Snapshot=Anim->CapturedRecoveryPose; RecoveryPose.PreUpdate(Instance);
            Final.Alpha=Physical?1.f:0.f;
            RecoveryBlend.Alpha=State.Motion==EONEInfectedMotionState::GetUp?FMath::Clamp(State.RecoverySnapshotAlpha,0.f,1.f):0.f;
            if (Physical) { PreviousYaw=Yaw; return; }
            const float Speed=Z->GetVelocity().Size2D();
            const auto* Movement=Z->GetCharacterMovement();
            const bool Grounded=Movement && Movement->IsMovingOnGround();
            SmoothedSpeed=FMath::FInterpTo(SmoothedSpeed,Speed,Dt,12.f);
            const float RunAlpha=FMath::Clamp((SmoothedSpeed-Z->AuthoredWalkSpeed)/FMath::Max(1.f,Z->AuthoredRunSpeed-Z->AuthoredWalkSpeed),0.f,1.f);
            auto* WalkClip=Anim->FindClip(TEXT("Walk")); auto* RunClip=Anim->FindClip(TEXT("Run"));
            const float Stride=FMath::Lerp(Z->AuthoredWalkSpeed*(WalkClip?WalkClip->GetPlayLength():1.04f),
                Z->AuthoredRunSpeed*(RunClip?RunClip->GetPlayLength():.66f),RunAlpha);
            const bool GaitAllowed=Grounded && (State.Motion==EONEInfectedMotionState::Locomotion ||
                (State.Motion==EONEInfectedMotionState::Attack && State.RecoveryLocomotionAlpha>.01f));
            if (GaitAllowed && Speed>3.f) GaitTurns+=Dt*Speed/FMath::Max(1.f,Stride);
            IdleTime+=Dt;
            const float Phase=float(FMath::Fmod(GaitTurns,1.));
            Sample(Idle,Anim->FindClip(TEXT("Idle")),float(IdleTime),true);
            Sample(Walk,WalkClip,Phase*(WalkClip?WalkClip->GetPlayLength():1.f),true);
            Sample(Run,RunClip,Phase*(RunClip?RunClip->GetPlayLength():1.f),true);
            Gait.Alpha=RunAlpha; Locomotion.Alpha=FMath::Clamp(SmoothedSpeed/45.f,0.f,1.f);
            const float YawRate=Dt>SMALL_NUMBER?FMath::FindDeltaAngleDegrees(PreviousYaw,Yaw)/Dt:0.f; PreviousYaw=Yaw;
            const bool TurnAllowed=State.Motion==EONEInfectedMotionState::Locomotion && Grounded && Speed<45.f && FMath::Abs(YawRate)>12.f;
            TurnTime=TurnAllowed?TurnTime+Dt*FMath::Clamp(FMath::Abs(YawRate)/90.f,.4f,2.f):0.f;
            Sample(Turn,Anim->FindClip(YawRate<0?TEXT("TurnLeft"):TEXT("TurnRight")),TurnTime,true);
            Turning.Alpha=FMath::FInterpTo(Turning.Alpha,TurnAllowed?FMath::Clamp(FMath::Abs(YawRate)/100.f,0.f,1.f):0.f,Dt,10.f);
            FName Key=State.ActionClip;
            if (Key.IsNone())
            {
                if (State.Motion==EONEInfectedMotionState::Attack) Key=FName(*Z->GetAttackClipKey().ToString().Replace(TEXT("C05_"),TEXT("")));
                else if (State.Motion==EONEInfectedMotionState::HeavyHit) Key=TEXT("HeavyHit");
                else if (State.Motion==EONEInfectedMotionState::Stumble) Key=TEXT("Stumble");
                else if (State.Motion==EONEInfectedMotionState::GetUp) Key=TEXT("GetUpProne");
                else Key=TEXT("Idle");
            }
            auto* ActionClip=Anim->FindClip(Key);
            const float ClipTime=State.ActionDurationSeconds>0 && ActionClip ?
                FMath::Max(0.f,State.StateAgeSeconds-State.ActionLeadInSeconds)*ActionClip->GetPlayLength()/State.ActionDurationSeconds : State.StateAgeSeconds;
            Sample(Action,ActionClip,ClipTime,false);
            if (State.Motion==EONEInfectedMotionState::Attack)
                for (const auto& D:Z->AttackDefinitions) if (D.AnimationKey==Key)
                {
                    if (SeenActionSerial!=State.ActionSerial || !AttackCursor.bPrimed)
                    {
                        AttackCursor.Reset(); SeenActionSerial=State.ActionSerial;
                        ONEInfectedAttacks::AdvanceEvents(AttackCursor,D.Events,0,D.Duration,false,true);
                    }
                    for (const auto& Event:ONEInfectedAttacks::AdvanceEvents(AttackCursor,D.Events,State.StateAgeSeconds,D.Duration,false,true))
                    {
                        if (Event.Event==EONEInfectedMotionEvent::AttackEffort) Anim->OnAttackEffort.Broadcast(int32(D.Family));
                        else if (Event.Event==EONEInfectedMotionEvent::LeftFootPlant || Event.Event==EONEInfectedMotionEvent::RightFootPlant)
                            Anim->QueueFootContact(Event.Event==EONEInfectedMotionEvent::LeftFootPlant);
                    }
                    break;
                }
            const float TargetAction=State.Motion==EONEInfectedMotionState::Locomotion?0.f:
                State.Motion==EONEInfectedMotionState::Attack?1.f-FMath::Clamp(State.RecoveryLocomotionAlpha,0.f,1.f):1.f;
            ActionBlend.Alpha=FMath::FInterpTo(ActionBlend.Alpha,TargetAction,Dt,18.f);
            const FTransform MeshWorld=Z->GetMesh()->GetComponentTransform();
            const float Age=Z->GetMinorReactionAge();
            const float Minor=Age>=0 && Age<.22f?FMath::Sin(PI*Age/.22f)*FMath::Exp(-5.f*Age)*Z->GetMinorReactionStrength():0.f;
            const FVector MinorDirection=MeshWorld.InverseTransformVectorNoScale(Z->GetMinorReactionDirection()).GetSafeNormal2D();
            const FVector ContactDirection=MeshWorld.InverseTransformVectorNoScale(State.ContactDirectionWorld).GetSafeNormal2D();
            const float Contact=FMath::Clamp(State.ContactStrength,0.f,1.f);
            const float Active=State.Motion==EONEInfectedMotionState::GetUp?0.f:1.f;
            const float MinorScale=State.Motion==EONEInfectedMotionState::Attack?.65f:1.f;
            Spine.Rotation=FRotator(-MinorDirection.X*5.f*Minor*MinorScale-ContactDirection.X*8.f*Contact,0,
                MinorDirection.Y*5.f*Minor*MinorScale+ContactDirection.Y*8.f*Contact);
            Neck.Rotation=FRotator(MinorDirection.X*3.f*Minor+ContactDirection.X*3.f*Contact,MinorDirection.Y*2.f*Minor,0);
            ShoulderLeft.Rotation=FRotator(ContactDirection.X*6.f*Contact,0,-ContactDirection.Y*7.f*Contact);
            ShoulderRight.Rotation=FRotator(ContactDirection.X*6.f*Contact,0,-ContactDirection.Y*7.f*Contact);
            Spine.SetAlpha(Active); Neck.SetAlpha(Active);
            ShoulderLeft.SetAlpha(Active*(Z->HasLeftArm()?1.f:0.f)); ShoulderRight.SetAlpha(Active*(Z->HasRightArm()?1.f:0.f));
            float Correction=0.f;
            if (State.Motion==EONEInfectedMotionState::Attack)
                for (const auto& D:Z->AttackDefinitions) if (D.AnimationKey==Key)
                {
                    const float Reference=ONEInfectedAttacks::StepPosition(D,ClipTime,195.f);
                    Correction=Reference-FMath::Clamp(State.CommittedTravelCm,0.f,D.StepDistanceCm);
                    break;
                }
            const FVector Offset=MeshWorld.InverseTransformVectorNoScale(Z->GetActorForwardVector())*Correction;
            for (auto& Foot:Feet)
            {
                Foot.Correction=Offset;
                Foot.SetAlpha(State.Motion==EONEInfectedMotionState::Attack?TargetAction:0.f);
            }
            for (const auto& Event:ONEInfectedAttacks::AdvanceEvents(FootCursor,FootMarkers,GaitTurns,1.f,true,
                GaitAllowed && Speed>8.f && TargetAction<.5f))
                Anim->QueueFootContact(Event.Event==EONEInfectedMotionEvent::LeftFootPlant);
        }
    };
}

UONEInfectedAnimInstance::UONEInfectedAnimInstance() { bUseMultiThreadedAnimationUpdate=false; }
void UONEInfectedAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation(); Clips.Reset(); ResetMotionEvents();
    for (const TCHAR* Key:ONEInfectedAnimationDetails::RequiredClips)
    {
        const FString Asset=TEXT("A_Infected_C07_")+FString(Key);
        if (auto* Clip=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/ONE/Animations/Candidate07/")+Asset+TEXT(".")+Asset))) Clips.Add(Key,Clip);
    }
    for (const auto& Pair:ClipOverrides) if (auto* Clip=Pair.Value.LoadSynchronous()) Clips.Add(Pair.Key,Clip);
}
void UONEInfectedAnimInstance::ConfigureClips(const TMap<FName,TSoftObjectPtr<UAnimSequence>>& Overrides)
{
    ClipOverrides=Overrides;
    for (const auto& Pair:Overrides) if (auto* Clip=Pair.Value.LoadSynchronous()) Clips.Add(Pair.Key,Clip);
    ResetMotionEvents();
}
UAnimSequence* UONEInfectedAnimInstance::FindClip(FName Key) const
{ const auto* Found=Clips.Find(Key); return Found?Found->Get():nullptr; }
bool UONEInfectedAnimInstance::HasRequiredClips() const
{ for (const TCHAR* Key:ONEInfectedAnimationDetails::RequiredClips) if (!FindClip(Key)) return false; return true; }
void UONEInfectedAnimInstance::ResetMotionEvents()
{ PendingFeet=0; LastFootTime[0]=LastFootTime[1]=-100.; LastMovingTime=-100.; ++MotionEventGeneration; }
void UONEInfectedAnimInstance::QueueFootContact(bool Left) { PendingFeet|=Left?1:2; }
void UONEInfectedAnimInstance::NativePostEvaluateAnimation()
{
    Super::NativePostEvaluateAnimation();
    const uint8 Feet=PendingFeet; PendingFeet=0;
    auto* Z=Cast<AONEZombie>(TryGetPawnOwner());
    if (!Z || Z->IsDead() || !GetWorld() || !Z->GetCharacterMovement()->IsMovingOnGround()) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if (Z->GetVelocity().Size2D()>8.f) LastMovingTime=Now;
    if (!Feet || Now-LastMovingTime>.22) return;
    const auto State=Z->GetInfectedAnimationState();
    if (State.Motion!=EONEInfectedMotionState::Locomotion && State.Motion!=EONEInfectedMotionState::Attack) return;
    for (int32 I=0;I<2;++I)
    {
        if (!(Feet&(1<<I)) || Now-LastFootTime[I]<.15) continue;
        const FName Bone=I==0?TEXT("toe_r"):TEXT("toe_l");
        const FVector Point=GetSkelMeshComponent()->GetSocketLocation(Bone);
        FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(InfectedFootContact),false,Z); Params.bReturnPhysicalMaterial=true;
        const bool Contact=GetWorld()->SweepSingleByChannel(Hit,Point+FVector(0,0,4),Point-FVector(0,0,9),FQuat::Identity,
            ECC_Visibility,FCollisionShape::MakeSphere(2.f),Params);
        if (Contact && Hit.bBlockingHit && Hit.ImpactNormal.Z>.5f && !Hit.bStartPenetrating)
        { LastFootTime[I]=Now; OnFootContact.Broadcast(I==0); }
    }
}
FAnimInstanceProxy* UONEInfectedAnimInstance::CreateAnimInstanceProxy() { return new ONEInfectedAnimationDetails::FProxy(this); }
void UONEInfectedAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete static_cast<ONEInfectedAnimationDetails::FProxy*>(Proxy); }
