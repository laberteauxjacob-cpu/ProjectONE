#include "ONEAnimInstance.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"
#include "AnimNodes/AnimNode_RotateRootBone.h"
#include "Animation/AnimNodeSpaceConversions.h"
#include "BoneControllers/AnimNode_TwoBoneIK.h"
#include "BoneControllers/AnimNode_ModifyBone.h"

struct FONEAnimProxy : FAnimInstanceProxy
{
    FAnimNode_SequenceEvaluator_Standalone Idle,WalkA,WalkB,RunA,RunB,Turn,Ready,Action,Death;
    FAnimNode_TwoWayBlend WalkDirection,RunDirection,Running,Locomotion,Turning,FullAction,Final;
    FAnimNode_RotateRootBone Facing;
    FAnimNode_ConvertLocalToComponentSpace PivotComponent;
    FAnimNode_ConvertComponentToLocalSpace PivotLocal;
    FAnimNode_ModifyBone PivotPelvis,PivotFootRotation[2];
    FAnimNode_TwoBoneIK PivotLeg[2];
    FAnimNode_LayeredBoneBlend ReadyUpperBody,UpperBody;
    float Phase=0,IdleClock=0,SmoothedSpeed=0,DirectionYaw=0;
    FONEAnimProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance)
    {
        WalkDirection.A.SetLinkNode(&WalkA); WalkDirection.B.SetLinkNode(&WalkB);
        RunDirection.A.SetLinkNode(&RunA); RunDirection.B.SetLinkNode(&RunB);
        Running.A.SetLinkNode(&WalkDirection); Running.B.SetLinkNode(&RunDirection);
        Locomotion.A.SetLinkNode(&Idle); Locomotion.B.SetLinkNode(&Running);
        Turning.A.SetLinkNode(&Locomotion); Turning.B.SetLinkNode(&Turn);
        Facing.BasePose.SetLinkNode(&Turning);
        PivotComponent.LocalPose.SetLinkNode(&Facing);
        PivotPelvis.ComponentPose.SetLinkNode(&PivotComponent);
        PivotPelvis.BoneToModify.BoneName=TEXT("pelvis");
        PivotPelvis.TranslationMode=BMM_Additive;
        PivotPelvis.TranslationSpace=BCS_ComponentSpace;
        PivotPelvis.Translation=FVector(0,0,-5.f);
        for (int32 I=0;I<2;++I)
        {
            const FName Foot=I==0 ? TEXT("foot_l") : TEXT("foot_r");
            PivotLeg[I].IKBone.BoneName=Foot;
            PivotLeg[I].bAllowStretching=false;
            PivotLeg[I].EffectorLocationSpace=BCS_ComponentSpace;
            PivotLeg[I].JointTargetLocationSpace=BCS_ComponentSpace;
            PivotFootRotation[I].BoneToModify.BoneName=Foot;
            PivotFootRotation[I].RotationMode=BMM_Replace;
            PivotFootRotation[I].RotationSpace=BCS_ComponentSpace;
            PivotFootRotation[I].ComponentPose.SetLinkNode(&PivotLeg[I]);
        }
        PivotLeg[0].ComponentPose.SetLinkNode(&PivotPelvis);
        PivotLeg[1].ComponentPose.SetLinkNode(&PivotFootRotation[0]);
        PivotLocal.ComponentPose.SetLinkNode(&PivotFootRotation[1]);
        ReadyUpperBody.BasePose.SetLinkNode(&PivotLocal);
        ReadyUpperBody.AddPose();
        ReadyUpperBody.BlendPoses[0].SetLinkNode(&Ready);
        FBranchFilter ReadyBranch; ReadyBranch.BoneName=TEXT("spine_01"); ReadyBranch.BlendDepth=2;
        ReadyUpperBody.LayerSetup[0].BranchFilters.Add(ReadyBranch);
        ReadyUpperBody.bMeshSpaceRotationBlend=true;
        UpperBody.BasePose.SetLinkNode(&ReadyUpperBody);
        UpperBody.AddPose();
        UpperBody.BlendPoses[0].SetLinkNode(&Action);
        FBranchFilter Branch; Branch.BoneName=TEXT("spine_01"); Branch.BlendDepth=2;
        UpperBody.LayerSetup[0].BranchFilters.Add(Branch);
        UpperBody.bMeshSpaceRotationBlend=true;
        FullAction.A.SetLinkNode(&UpperBody); FullAction.B.SetLinkNode(&Action);
        Final.A.SetLinkNode(&FullAction); Final.B.SetLinkNode(&Death);
    }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Final; }
    static void Sample(FAnimNode_SequenceEvaluator_Standalone& Node,UAnimSequence* Clip,float Time,bool Loop=true)
    {
        if (!Clip) return;
        Node.SetSequence(Clip);
        Node.SetTeleportToExplicitTime(true);
        const float Length=FMath::Max(.001f,Clip->GetPlayLength());
        Node.SetExplicitTime(Loop ? FMath::Fmod(Time,Length) : FMath::Clamp(Time,0.f,FMath::Max(0.f,Length-.0001f)));
    }
    virtual void PreUpdate(UAnimInstance* Instance,float Dt) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Dt);
        UONEAnimInstance* Anim=Cast<UONEAnimInstance>(Instance);
        APawn* Pawn=Instance->TryGetPawnOwner();
        if (!Anim || !Pawn) return;
        AONEZombie* Z=Cast<AONEZombie>(Pawn);
        AONEPlayer* P=Cast<AONEPlayer>(Pawn);
        const float FacingYaw=P ? P->GetBodyFacingYaw() : Pawn->GetActorRotation().Yaw;
        const FVector Local=FRotator(0,FacingYaw,0).UnrotateVector(Pawn->GetVelocity());
        const float Speed=Local.Size2D();
        SmoothedSpeed=FMath::FInterpTo(SmoothedSpeed,Speed,Dt,12.f);
        if (Speed>3.f)
        {
            const float Desired=FMath::RadiansToDegrees(FMath::Atan2(Local.Y,Local.X));
            DirectionYaw=FMath::FixedTurn(DirectionYaw,Desired,Dt*900.f);
        }
        const float WalkSpeed=Z ? Z->AuthoredWalkSpeed : (P ? P->AuthoredWalkSpeed : 225.f);
        const float RunSpeed=Z ? Z->AuthoredRunSpeed : (P ? P->AuthoredRunSpeed : 370.f);
        const float RunWeight=FMath::Clamp((SmoothedSpeed-WalkSpeed)/FMath::Max(1.f,RunSpeed-WalkSpeed),0.f,1.f);
        UAnimSequence* WalkClip=Anim->FindClip(TEXT("Walk"));
        UAnimSequence* RunClip=Anim->FindClip(TEXT("Run"));
        UAnimSequence* WalkSecond=WalkClip;
        UAnimSequence* RunSecond=RunClip;
        float DirectionBlend=0.f,StrideProjection=1.f;
        if (P)
        {
            static const TCHAR* Directions[]={TEXT("F"),TEXT("FR"),TEXT("R"),TEXT("BR"),TEXT("B"),TEXT("BL"),TEXT("L"),TEXT("FL")};
            const float Sector=FRotator::ClampAxis(DirectionYaw)/45.f;
            const int32 A=FMath::FloorToInt(Sector)%8,B=(A+1)%8;
            DirectionBlend=Sector-FMath::FloorToFloat(Sector);
            auto DirectionClip=[&](const TCHAR* Gait,int32 Index,UAnimSequence* Fallback)
            {
                UAnimSequence* Clip=Anim->FindClip(FName(*FString::Printf(TEXT("C03_%s_%s"),Gait,Directions[Index])));
                return Clip ? Clip : Fallback;
            };
            WalkClip=DirectionClip(TEXT("Walk"),A,WalkClip);
            WalkSecond=DirectionClip(TEXT("Walk"),B,WalkClip);
            RunClip=DirectionClip(TEXT("Run"),A,RunClip);
            RunSecond=DirectionClip(TEXT("Run"),B,RunClip);
            // Compensate only projection loss between adjacent 45-degree source
            // strides. Exact cardinal AND diagonal clips always use 1.
            const float Angle=PI/4.f;
            StrideProjection=(1.f-DirectionBlend)*FMath::Cos(DirectionBlend*Angle)
                +DirectionBlend*FMath::Cos((1.f-DirectionBlend)*Angle);
        }
        const float WalkStride=WalkSpeed*(WalkClip ? WalkClip->GetPlayLength() : .72f);
        const float RunStride=RunSpeed*(RunClip ? RunClip->GetPlayLength() : .62f);
        const float Stride=FMath::Lerp(WalkStride,RunStride,RunWeight)*StrideProjection;
        Phase=FMath::Fmod(Phase+Dt*Speed/FMath::Max(1.f,Stride),1.f);
        IdleClock+=Dt;
        Sample(Idle,Anim->FindClip(TEXT("Idle")),IdleClock);
        auto LoopSample=[&](FAnimNode_SequenceEvaluator_Standalone& Node,UAnimSequence* Clip)
        {
            Sample(Node,Clip,Phase*(Clip ? Clip->GetPlayLength() : 1.f));
        };
        LoopSample(WalkA,WalkClip); LoopSample(WalkB,WalkSecond);
        LoopSample(RunA,RunClip); LoopSample(RunB,RunSecond);
        WalkDirection.Alpha=DirectionBlend;
        RunDirection.Alpha=DirectionBlend;
        Running.Alpha=RunWeight;
        Locomotion.Alpha=FMath::Clamp(SmoothedSpeed/60.f,0.f,1.f);
        const bool bTurn=P && P->IsTurningInPlace();
        UAnimSequence* TurnClip=P ? Anim->FindClip(P->GetTurnDirection()>0 ? TEXT("C03_Turn_R") : TEXT("C03_Turn_L")) : nullptr;
        Sample(Turn,TurnClip ? TurnClip : Anim->FindClip(TEXT("Idle")),P ? P->GetTurnAnimationTime() : 0.f,false);
        Turning.Alpha=FMath::FInterpTo(Turning.Alpha,bTurn ? 1.f : 0.f,Dt,25.f);
        Facing.Yaw=P ? FMath::FindDeltaAngleDegrees(Pawn->GetActorRotation().Yaw,FacingYaw) : 0.f;
        // RotateRootBone postmultiplies the root basis. Convert component yaw
        // through the actual imported root basis instead of assuming its axes.
        if (const USkeletalMesh* Mesh=Instance->GetSkelMeshComponent()->GetSkeletalMeshAsset())
            if (Mesh->GetRefSkeleton().GetNum()>0)
                Facing.MeshToComponent=Mesh->GetRefSkeleton().GetRefBonePose()[0].GetRotation().Rotator();
        const FTransform MeshWorld=Instance->GetSkelMeshComponent()->GetComponentTransform();
        float PivotWeight=0.f;
        for (int32 I=0;I<2;++I)
        {
            const float Weight=P ? P->GetPivotFootWeight(I) : 0.f;
            PivotLeg[I].SetAlpha(Weight);
            PivotFootRotation[I].SetAlpha(Weight);
            PivotWeight=FMath::Max(PivotWeight,Weight);
            if (P && Weight>0.f)
            {
                const FTransform Foot=P->GetPivotFootWorld(I).GetRelativeTransform(MeshWorld);
                PivotLeg[I].EffectorLocation=Foot.GetLocation();
                PivotLeg[I].JointTargetLocation=MeshWorld.InverseTransformPosition(P->GetPivotKneeWorld(I));
                PivotFootRotation[I].Rotation=Foot.Rotator();
            }
        }
        PivotPelvis.SetAlpha(PivotWeight);
        float ActionWeight=0,DeathWeight=0;
        Sample(Ready,Anim->FindClip(TEXT("Idle")),IdleClock);
        ReadyUpperBody.BlendWeights[0]=P ? 1.f : 0.f;
        if (P && P->GetWeaponComponent())
        {
            auto* W=P->GetWeaponComponent();
            Sample(Ready,W->GetReadyAnimation(),IdleClock);
            float ActionTime=0;
            if (UAnimSequence* ActionClip=W->GetActionAnimation(ActionTime)) { Sample(Action,ActionClip,ActionTime,false); ActionWeight=1; }
            else if (!Action.GetSequence()) Sample(Action,Anim->FindClip(TEXT("Idle")),0);
        }
        if (Z)
        {
            switch (Z->GetCombatState())
            {
                case EONEZombieState::Attack:
                    Sample(Action,Anim->FindClip(Z->HasLeftArm() ? TEXT("Attack") : TEXT("AttackOneArm")),Z->GetStateElapsed(),false);
                    ActionWeight=1; break;
                case EONEZombieState::Hit:
                    Sample(Action,Anim->FindClip(Z->IsHeavyReaction() ? TEXT("HeavyHit") : TEXT("Hit")),Z->GetStateElapsed(),false);
                    ActionWeight=1; break;
                case EONEZombieState::Dead:
                    Sample(Death,Anim->FindClip(TEXT("Death")),Z->GetStateElapsed(),false);
                    DeathWeight=1; break;
                default: break;
            }
            if (!Action.GetSequence()) Sample(Action,Anim->FindClip(TEXT("Attack")),0,false);
            if (!Death.GetSequence()) Sample(Death,Anim->FindClip(TEXT("Death")),0,false);
        }
        else if (!Death.GetSequence()) Sample(Death,Anim->FindClip(TEXT("Idle")),0);
        UpperBody.BlendWeights[0]=FMath::FInterpTo(UpperBody.BlendWeights[0],Z ? 0.f : ActionWeight,Dt,24.f);
        FullAction.Alpha=FMath::FInterpTo(FullAction.Alpha,Z ? ActionWeight : 0.f,Dt,24.f);
        Final.Alpha=FMath::FInterpTo(Final.Alpha,DeathWeight,Dt,20.f);
    }
};
UONEAnimInstance::UONEAnimInstance() { bUseMultiThreadedAnimationUpdate=false; }
void UONEAnimInstance::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();
    Clips.Reset();
    const bool bInfected=Cast<AONEZombie>(TryGetPawnOwner())!=nullptr;
    const FString Prefix=bInfected ? TEXT("A_Infected_") : TEXT("A_Response_");
    TArray<FString> Names={TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("Fire"),TEXT("Reload"),TEXT("Attack"),TEXT("AttackOneArm"),TEXT("Hit"),TEXT("HeavyHit"),TEXT("Death")};
    if (!bInfected)
    {
        for (const TCHAR* Gait:{TEXT("Walk"),TEXT("Run")})
            for (const TCHAR* Direction:{TEXT("F"),TEXT("FR"),TEXT("R"),TEXT("BR"),TEXT("B"),TEXT("BL"),TEXT("L"),TEXT("FL")})
                Names.Add(FString::Printf(TEXT("C03_%s_%s"),Gait,Direction));
        Names.Add(TEXT("C03_Turn_L")); Names.Add(TEXT("C03_Turn_R"));
    }
    for (const FString& Name:Names)
    {
        if (bInfected && (Name==TEXT("Fire")||Name==TEXT("Reload"))) continue;
        if (!bInfected && (Name.StartsWith(TEXT("Attack"))||Name==TEXT("Hit")||Name==TEXT("HeavyHit")||Name==TEXT("Death"))) continue;
        const FString Asset=Prefix+Name;
        if (auto* Clip=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/ONE/Animations/")+Asset+TEXT(".")+Asset))) Clips.Add(FName(*Name),Clip);
    }
}
UAnimSequence* UONEAnimInstance::FindClip(FName Key) const { const auto* Clip=Clips.Find(Key); return Clip ? Clip->Get() : nullptr; }
FAnimInstanceProxy* UONEAnimInstance::CreateAnimInstanceProxy() { return new FONEAnimProxy(this); }
void UONEAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete static_cast<FONEAnimProxy*>(Proxy); }
