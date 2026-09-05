#include "ONEAnimInstance.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponComponent.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"
#include "AnimNodes/AnimNode_SequenceEvaluator.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "AnimNodes/AnimNode_LayeredBoneBlend.h"

struct FONEAnimProxy : FAnimInstanceProxy
{
    FAnimNode_SequenceEvaluator_Standalone Idle,Walk,Run,Back,Left,Right,Action,Death;
    FAnimNode_TwoWayBlend ForwardBack,LeftRight,Directions,Running,Locomotion,FullAction,Final;
    FAnimNode_LayeredBoneBlend UpperBody;
    float Phase=0,IdleClock=0,SmoothedSpeed=0,SmoothedX=1,SmoothedY=0;
    FONEAnimProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance)
    {
        ForwardBack.A.SetLinkNode(&Walk); ForwardBack.B.SetLinkNode(&Back);
        LeftRight.A.SetLinkNode(&Left); LeftRight.B.SetLinkNode(&Right);
        Directions.A.SetLinkNode(&ForwardBack); Directions.B.SetLinkNode(&LeftRight);
        Running.A.SetLinkNode(&Directions); Running.B.SetLinkNode(&Run);
        Locomotion.A.SetLinkNode(&Idle); Locomotion.B.SetLinkNode(&Running);
        UpperBody.BasePose.SetLinkNode(&Locomotion);
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
        const FVector Local=Pawn->GetActorRotation().UnrotateVector(Pawn->GetVelocity());
        const float Speed=Local.Size2D();
        SmoothedSpeed=FMath::FInterpTo(SmoothedSpeed,Speed,Dt,12.f);
        if (Speed>3)
        {
            SmoothedX=FMath::FInterpTo(SmoothedX,Local.X/Speed,Dt,12.f);
            SmoothedY=FMath::FInterpTo(SmoothedY,Local.Y/Speed,Dt,12.f);
        }
        const float WalkSpeed=Z ? Z->AuthoredWalkSpeed : (P ? P->AuthoredWalkSpeed : 180.f);
        const float RunSpeed=Z ? Z->AuthoredRunSpeed : (P ? P->AuthoredRunSpeed : 370.f);
        float RunWeight=FMath::Clamp((SmoothedSpeed-WalkSpeed)/FMath::Max(1.f,RunSpeed-WalkSpeed),0.f,1.f);
        // Side/backward sprint uses the authored directional stride at a matched playback rate.
        if (!Z) RunWeight*=FMath::Clamp((SmoothedX-.45f)/.55f,0.f,1.f);
        UAnimSequence* WalkClip=Anim->FindClip(TEXT("Walk"));
        UAnimSequence* RunClip=Anim->FindClip(TEXT("Run"));
        const float WalkStride=WalkSpeed*(WalkClip ? WalkClip->GetPlayLength() : .8f);
        const float RunStride=RunSpeed*(RunClip ? RunClip->GetPlayLength() : .6f);
        const float Stride=FMath::Lerp(WalkStride,RunStride,RunWeight);
        // A blend of orthogonal steps shortens diagonal stride; compensate its phase rate.
        const float DirectionScale=Z ? 1.f : FMath::Lerp(FMath::Max(1.f,FMath::Abs(SmoothedX)+FMath::Abs(SmoothedY)),1.f,RunWeight);
        Phase=FMath::Fmod(Phase+Dt*Speed*DirectionScale/FMath::Max(1.f,Stride),1.f);
        IdleClock+=Dt;
        Sample(Idle,Anim->FindClip(TEXT("Idle")),IdleClock);
        auto LoopSample=[&](FAnimNode_SequenceEvaluator_Standalone& Node,FName Name)
        {
            UAnimSequence* Clip=Anim->FindClip(Name); if (!Clip) Clip=WalkClip;
            Sample(Node,Clip,Phase*(Clip ? Clip->GetPlayLength() : 1.f));
        };
        LoopSample(Walk,TEXT("Walk")); LoopSample(Run,TEXT("Run"));
        LoopSample(Back,TEXT("Back"));
        // UE import reflects the authored Y axis: source StrafeL moves toward UE +Y.
        LoopSample(Left,TEXT("StrafeR")); LoopSample(Right,TEXT("StrafeL"));
        ForwardBack.Alpha=Z ? 0.f : FMath::Clamp(-SmoothedX*3.f,0.f,1.f);
        LeftRight.Alpha=FMath::Clamp(SmoothedY*3.f+.5f,0.f,1.f);
        Directions.Alpha=Z ? 0.f : FMath::Abs(SmoothedY)/FMath::Max(.01f,FMath::Abs(SmoothedX)+FMath::Abs(SmoothedY));
        Running.Alpha=RunWeight;
        Locomotion.Alpha=FMath::Clamp(SmoothedSpeed/40.f,0.f,1.f);
        float ActionWeight=0,DeathWeight=0;
        if (P && P->GetWeaponComponent())
        {
            auto* W=P->GetWeaponComponent();
            if (W->IsReloading()) { Sample(Action,Anim->FindClip(TEXT("Reload")),W->GetReloadElapsed(),false); ActionWeight=1; }
            else { Sample(Action,Anim->FindClip(TEXT("Fire")),W->GetTimeSinceShot(),false); ActionWeight=W->GetTimeSinceShot()<.2f ? 1.f : 0.f; }
        }
        if (Z)
        {
            switch (Z->GetCombatState())
            {
                case EONEZombieState::Attack:
                    Sample(Action,Anim->FindClip(Z->HasLeftArm() ? TEXT("Attack") : TEXT("AttackOneArm")),Z->GetStateElapsed(),false);
                    ActionWeight=1; break;
                case EONEZombieState::Hit:
                    Sample(Action,Anim->FindClip(TEXT("Hit")),Z->GetStateElapsed(),false);
                    ActionWeight=1; break;
                case EONEZombieState::Dead:
                    Sample(Death,Anim->FindClip(TEXT("Death")),Z->GetStateElapsed(),false);
                    DeathWeight=1; break;
                default: break;
            }
            // Provide valid sequences even while actions have zero weight.
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
    const FString Prefix=Cast<AONEZombie>(TryGetPawnOwner()) ? TEXT("A_Infected_") : TEXT("A_Response_");
    const TArray<FString> Names={TEXT("Idle"),TEXT("Walk"),TEXT("Run"),TEXT("Back"),TEXT("StrafeL"),TEXT("StrafeR"),TEXT("Fire"),TEXT("Reload"),TEXT("Attack"),TEXT("AttackOneArm"),TEXT("Hit"),TEXT("Death")};
    const bool bInfected=Cast<AONEZombie>(TryGetPawnOwner())!=nullptr;
    for (const FString& Name:Names)
    {
        if (bInfected && (Name==TEXT("Back")||Name.StartsWith(TEXT("Strafe"))||Name==TEXT("Fire")||Name==TEXT("Reload"))) continue;
        if (!bInfected && (Name.StartsWith(TEXT("Attack"))||Name==TEXT("Hit")||Name==TEXT("Death"))) continue;
        const FString Asset=Prefix+Name;
        if (auto* Clip=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/ONE/Animations/")+Asset+TEXT(".")+Asset))) Clips.Add(FName(*Name),Clip);
    }
}
UAnimSequence* UONEAnimInstance::FindClip(FName Key) const { const auto* Clip=Clips.Find(Key); return Clip ? Clip->Get() : nullptr; }
FAnimInstanceProxy* UONEAnimInstance::CreateAnimInstanceProxy() { return new FONEAnimProxy(this); }
void UONEAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete static_cast<FONEAnimProxy*>(Proxy); }
