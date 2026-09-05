#include "ONEPhysicsRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/World.h"
#include "ONESnapshotAnimInstance.h"

CSV_DECLARE_CATEGORY_EXTERN(ONEPhysicality);

namespace
{
    struct FTransitionPose
    {
        TMap<FName,FTransform> Bones,Bodies;
    };
    FTransitionPose CaptureTransition(USkeletalMeshComponent* Mesh)
    {
        FTransitionPose Result;
        if (const USkeletalMesh* Asset=Mesh->GetSkeletalMeshAsset())
            for (const FMeshBoneInfo& Bone:Asset->GetRefSkeleton().GetRefBoneInfo())
                Result.Bones.Add(Bone.Name,Mesh->GetSocketTransform(Bone.Name));
        if (const UPhysicsAsset* Asset=Mesh->GetPhysicsAsset())
            for (const USkeletalBodySetup* Setup:Asset->SkeletalBodySetups)
                if (Setup) if (FBodyInstance* Body=Mesh->GetBodyInstance(Setup->BoneName))
                    if (Body->IsValidBodyInstance()) Result.Bodies.Add(Setup->BoneName,Body->GetUnrealWorldTransform());
        return Result;
    }
    void MeasureTransition(USkeletalMeshComponent* Mesh,const FTransitionPose& Before,float& Position,float& Angle)
    {
        Position=Angle=0.f;
        auto Compare=[&](const FTransform& A,const FTransform& B)
        {
            if (!A.IsValid() || !B.IsValid()) { Position=Angle=BIG_NUMBER; return; }
            Position=FMath::Max(Position,float(FVector::Distance(A.GetLocation(),B.GetLocation())));
            Angle=FMath::Max(Angle,float(FMath::RadiansToDegrees(A.GetRotation().AngularDistance(B.GetRotation()))));
        };
        for (const auto& Pair:Before.Bones) Compare(Pair.Value,Mesh->GetSocketTransform(Pair.Key));
        for (const auto& Pair:Before.Bodies)
        {
            FBodyInstance* Body=Mesh->GetBodyInstance(Pair.Key);
            if (!Body || !Body->IsValidBodyInstance()) { Position=Angle=BIG_NUMBER; continue; }
            Compare(Pair.Value,Body->GetUnrealWorldTransform());
        }
        if (Before.Bodies.IsEmpty()) Position=Angle=BIG_NUMBER;
    }
    bool FreezePhysicalPose(USkeletalMeshComponent* Mesh,ONEPhysicsRuntime::FRestState& State)
    {
        const FTransitionPose Before=CaptureTransition(Mesh);
        FPoseSnapshot Pose; Mesh->SnapshotPose(Pose);
        if (!Pose.bIsValid || Before.Bodies.IsEmpty()) return false;
        // Capture before changing animation class or simulation. All local bones,
        // including retained cut roots, come from the actual evaluated result.
        Mesh->SetAllBodiesSimulatePhysics(false);
        Mesh->SetAllBodiesPhysicsBlendWeight(0.f);
        if (!Cast<UONESnapshotAnimInstance>(Mesh->GetAnimInstance()))
            Mesh->SetAnimInstanceClass(UONESnapshotAnimInstance::StaticClass());
        if (auto* Anim=Cast<UONESnapshotAnimInstance>(Mesh->GetAnimInstance())) Anim->CapturedPose=MoveTemp(Pose);
        Mesh->TickAnimation(0.f,false); Mesh->RefreshBoneTransforms();
        Mesh->UpdateKinematicBonesToAnim(Mesh->GetComponentSpaceTransforms(),ETeleportType::TeleportPhysics,true,EAllowKinematicDeferral::DisallowDeferral);
        MeasureTransition(Mesh,Before,State.FreezePositionErrorCm,State.FreezeAngleErrorDegrees);
        State.Frozen=true; State.FrozenBodyCount=Before.Bodies.Num(); ++State.FreezeEvents;
        return true;
    }
    void ResumePhysicalPose(USkeletalMeshComponent* Mesh,ONEPhysicsRuntime::FRestState& State)
    {
        const FTransitionPose Before=CaptureTransition(Mesh);
        Mesh->TickAnimation(0.f,false); Mesh->RefreshBoneTransforms();
        Mesh->UpdateKinematicBonesToAnim(Mesh->GetComponentSpaceTransforms(),ETeleportType::TeleportPhysics,true,EAllowKinematicDeferral::DisallowDeferral);
        // Enable only retained instances. SetAllBodiesSimulatePhysics(true) would
        // also address terminated limbs; no missing physics actor is recreated.
        for (const auto& Pair:Before.Bodies)
            if (FBodyInstance* Body=Mesh->GetBodyInstance(Pair.Key))
                Body->SetInstanceSimulatePhysics(true,false,true);
        Mesh->SetAllBodiesPhysicsBlendWeight(1.f);
        Mesh->SetAllPhysicsLinearVelocity(FVector::ZeroVector,false);
        Mesh->SetAllPhysicsAngularVelocityInRadians(FVector::ZeroVector,false);
        MeasureTransition(Mesh,Before,State.ResumePositionErrorCm,State.ResumeAngleErrorDegrees);
        State.Frozen=false; State.FrozenBodyCount=0; ++State.ResumeEvents;
        UE_LOG(LogTemp,Display,TEXT("ONE_REST_RESUME owner=%s event=%d continuity=%.6fcm/%.6fdeg retained_bodies=%d"),
            *GetNameSafe(Mesh->GetOwner()),State.ResumeEvents,State.ResumePositionErrorCm,State.ResumeAngleErrorDegrees,Before.Bodies.Num());
    }
    bool InChain(USkeletalMeshComponent* Mesh,FName Bone,FName Root)
    {
        if (Root.IsNone()) return true;
        const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
        const int32 Index=Ref.FindBoneIndex(Bone),Parent=Ref.FindBoneIndex(Root);
        return Index!=INDEX_NONE && Parent!=INDEX_NONE && (Index==Parent || Ref.BoneIsChildOf(Index,Parent));
    }
    bool HasStaticSupport(USkeletalMeshComponent* Mesh,const TArray<FBodyInstance*>& Candidates)
    {
        // Only after a full pose window qualifies. Maximum two short queries per
        // decision, no corpse-to-corpse N-squared probing or broad overlap scan.
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ONERestSupport),false,Mesh->GetOwner());
        const FCollisionObjectQueryParams Objects(ECC_WorldStatic);
        for (int32 I=0;I<FMath::Min(2,Candidates.Num());++I)
        {
            FBodyInstance* Body=Candidates[I];
            const FBox Bounds=Body->GetBodyBounds();
            FVector Bottom;
            const FVector Below(Bounds.GetCenter().X,Bounds.GetCenter().Y,Bounds.Min.Z-10.f);
            if (Body->GetDistanceToBody(Below,Bottom)<0 || Bottom.ContainsNaN()) continue;
            FHitResult Hit;
            if (Mesh->GetWorld()->LineTraceSingleByObjectType(Hit,Bottom+FVector(0,0,3),Bottom-FVector(0,0,3),Objects,Params)
                && !Hit.bStartPenetrating && Hit.ImpactNormal.Z>.45f && FVector::Distance(Bottom,Hit.ImpactPoint)<=3.f)
                return true;
        }
        return false;
    }
}
void ONEPhysicsRuntime::ResetRest(USkeletalMeshComponent* Mesh,FRestState& State,bool Wake)
{
    if (Wake && Mesh && State.Frozen) ResumePhysicalPose(Mesh,State);
    State.DisturbedAt=Mesh && Mesh->GetWorld()?Mesh->GetWorld()->GetTimeSeconds():-1;
    State.LastSample=-1; State.WindowStart=-1; State.NextSupportAttempt=-1; State.StableSeconds=0; State.WindowPose.Reset();
    State.WasSleeping=false;
    if (Wake && Mesh)
    {
        ++State.ExplicitWakeEvents;
        Mesh->WakeAllRigidBodies();
        CSV_EVENT(ONEPhysicality,TEXT("REST_EXPLICIT_WAKE owner=%s"),*GetNameSafe(Mesh->GetOwner()));
    }
}
void ONEPhysicsRuntime::UpdateRest(USkeletalMeshComponent* Mesh,FRestState& State)
{
    if (!Mesh || !Mesh->GetWorld() || !Mesh->GetPhysicsAsset()) return;
    if (State.Frozen) return;
    CSV_SCOPED_TIMING_STAT(ONEPhysicality,RagdollRest);
    const double Now=Mesh->GetWorld()->GetTimeSeconds();
    const double Gap=State.LastSample<0?-1:Now-State.LastSample;
    if (Gap>=0 && Gap<.09) return;
    State.LastSample=Now;
    if (!Mesh->IsAnyRigidBodyAwake())
    { State.WasSleeping=true; State.WindowStart=-1; State.WindowPose.Reset(); State.StableSeconds=0; return; }
    TMap<FName,FTransform> Current;
    TArray<FBodyInstance*> SupportCandidates;
    int32 Awake=0;
    bool Valid=true;
    State.MaxLinear=State.MaxAngular=State.PoseDriftCm=State.PoseDriftDegrees=0;
    for (const USkeletalBodySetup* Setup:Mesh->GetPhysicsAsset()->SkeletalBodySetups)
    {
        if (!Setup) continue;
        FBodyInstance* Body=Mesh->GetBodyInstance(Setup->BoneName);
        if (!Body || !Body->IsValidBodyInstance() || !Body->IsInstanceSimulatingPhysics()) continue;
        Awake+=Body->IsInstanceAwake()?1:0;
        const FTransform Pose=Body->GetUnrealWorldTransform();
        const float Speed=float(Body->GetUnrealWorldVelocity().Size());
        const float Spin=float(Body->GetUnrealWorldAngularVelocityInRadians().Size());
        if (!Pose.IsValid() || !FMath::IsFinite(Speed) || !FMath::IsFinite(Spin)) { Valid=false; continue; }
        State.MaxLinear=FMath::Max(State.MaxLinear,Speed); State.MaxAngular=FMath::Max(State.MaxAngular,Spin);
        Current.Add(Setup->BoneName,Pose);
        if (const FTransform* Start=State.WindowPose.Find(Setup->BoneName))
        {
            State.PoseDriftCm=FMath::Max(State.PoseDriftCm,float(FVector::Distance(Pose.GetLocation(),Start->GetLocation())));
            State.PoseDriftDegrees=FMath::Max(State.PoseDriftDegrees,float(FMath::RadiansToDegrees(Pose.GetRotation().AngularDistance(Start->GetRotation()))));
        }
        // Compound pelvis includes an intentionally disabled stump shape. Its
        // closest-shape query would not prove support of the active geometry.
        if (Setup->AggGeom.GetElementCount()==1) SupportCandidates.Add(Body);
    }
    if (Awake==0 && Current.Num()>0 && Valid)
    {
        State.WasSleeping=true; State.WindowStart=-1; State.WindowPose.Reset(); State.StableSeconds=0; return;
    }
    if (State.WasSleeping)
    {
        ++State.ContactWakeEvents; State.DisturbedAt=Now; State.WasSleeping=false;
        CSV_EVENT(ONEPhysicality,TEXT("REST_CONTACT_WAKE owner=%s"),*GetNameSafe(Mesh->GetOwner()));
    }
    const bool Quiet=Valid && Current.Num()>0 && State.MaxLinear<=5.f && State.MaxAngular<=.35f;
    const bool Continuous=Gap>0 && Gap<=.25 && Current.Num()==State.WindowPose.Num();
    if (!Quiet || !Continuous || State.PoseDriftCm>1.f || State.PoseDriftDegrees>2.f || Now-State.DisturbedAt<2.)
    {
        State.WindowPose=MoveTemp(Current); State.WindowStart=Quiet && Now-State.DisturbedAt>=2.?Now:-1;
        State.StableSeconds=0; return;
    }
    if (State.WindowStart<0) { State.WindowStart=Now; State.WindowPose=MoveTemp(Current); return; }
    State.StableSeconds=float(Now-State.WindowStart);
    if (State.StableSeconds<1.25f) return;
    if (Now<State.NextSupportAttempt) return;
    State.NextSupportAttempt=Now+.5;
    SupportCandidates.Sort([](const FBodyInstance& A,const FBodyInstance& B){return A.GetBodyBounds().Min.Z<B.GetBodyBounds().Min.Z;});
    if (!HasStaticSupport(Mesh,SupportCandidates)) return;
    const float EstablishedSeconds=State.StableSeconds;
    if (!FreezePhysicalPose(Mesh,State)) return;
    State.WasSleeping=false;
    State.WindowPose.Reset(); State.WindowStart=-1; State.StableSeconds=0;
    CSV_EVENT(ONEPhysicality,TEXT("REST_SUPPORTED_FREEZE owner=%s duration=%.3f"),*GetNameSafe(Mesh->GetOwner()),EstablishedSeconds);
    UE_LOG(LogTemp,Display,TEXT("ONE_REST_SUPPORTED_FREEZE owner=%s event=%d stable=%.3fs linear=%.4fcm/s angular=%.4frad/s pose=%.4fcm/%.4fdeg continuity=%.6fcm/%.6fdeg retained_bodies=%d"),
        *GetNameSafe(Mesh->GetOwner()),State.FreezeEvents,EstablishedSeconds,State.MaxLinear,State.MaxAngular,State.PoseDriftCm,State.PoseDriftDegrees,
        State.FreezePositionErrorCm,State.FreezeAngleErrorDegrees,State.FrozenBodyCount);
}
ONEPhysicsRuntime::FStartResult ONEPhysicsRuntime::Start(USkeletalMeshComponent* Mesh,const FVector& InheritedVelocity,const TArray<FName>& MissingRoots)
{
    CSV_SCOPED_TIMING_STAT(ONEPhysicality,RagdollInitialize);
    FStartResult Result;
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Mesh->GetPhysicsAsset()) return Result;
    TMap<FName,FTransform> Before;
    for (const USkeletalBodySetup* Setup:Mesh->GetPhysicsAsset()->SkeletalBodySetups)
        if (Setup) Before.Add(Setup->BoneName,Mesh->GetSocketTransform(Setup->BoneName,RTS_World));
    // Terminated PA bodies retain their body index. UE5.7's default physics
    // blend otherwise reads their invalid world transform and snaps the stump
    // to identity. Compose only these non-simulated chains from snapshot locals;
    // existing simulated bodies already follow this same composition branch.
    Mesh->bLocalSpaceKinematics=true;
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetCollisionObjectType(ECC_PhysicsBody);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
    Mesh->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Block);
    // Character capsules cannot be trapped by a corpse or loose piece. Corpse
    // hit regions are separate bone-anchored queries; this mesh is physics-only.
    Mesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    Mesh->UpdateKinematicBonesToAnim(Mesh->GetComponentSpaceTransforms(),ETeleportType::TeleportPhysics,true,EAllowKinematicDeferral::DisallowDeferral);
    Mesh->SetSimulatePhysics(true);
    Mesh->SetAllBodiesSimulatePhysics(true);
    Mesh->SetAllBodiesPhysicsBlendWeight(1.f);
    // Simulation/filter changes may rewrite per-shape flags. Apply the mask
    // after those changes and before terminating any missing body chain.
    Result.StumpFitErrorCm=ConfigureLeftStump(Mesh,MissingRoots.Contains(TEXT("thigh_r")));
    for (FName Root:MissingRoots) Mesh->TermBodiesBelow(Root);
    Mesh->SetAllPhysicsLinearVelocity(InheritedVelocity.GetClampedToMaxSize(420.f),false);
    Result.PositionErrorCm=0; Result.AngleErrorDegrees=0;
    for (const auto& Pair:Before)
    {
        auto* Body=Mesh->GetBodyInstance(Pair.Key);
        if (!Body || !Body->IsValidBodyInstance() || !Body->IsInstanceSimulatingPhysics()) continue;
        ++Result.SimulatedBodies;
        const FTransform After=Body->GetUnrealWorldTransform();
        Result.PositionErrorCm=FMath::Max(Result.PositionErrorCm,float(FVector::Dist(After.GetLocation(),Pair.Value.GetLocation())));
        Result.AngleErrorDegrees=FMath::Max(Result.AngleErrorDegrees,float(FMath::RadiansToDegrees(After.GetRotation().AngularDistance(Pair.Value.GetRotation()))));
    }
    if (!Result.SimulatedBodies) Result.PositionErrorCm=Result.AngleErrorDegrees=BIG_NUMBER;
    Mesh->WakeAllRigidBodies();
    return Result;
}
float ONEPhysicsRuntime::ConfigureLeftStump(USkeletalMeshComponent* Mesh,bool Missing)
{
    CSV_SCOPED_TIMING_STAT(ONEPhysicality,StumpCollisionTransfer);
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Mesh->GetPhysicsAsset()) return BIG_NUMBER;
    FBodyInstance* Pelvis=Mesh->GetBodyInstance(TEXT("pelvis"));
    if (!Pelvis || !Pelvis->IsValidBodyInstance()) return BIG_NUMBER;
    const int32 SetupIndex=Mesh->GetPhysicsAsset()->FindBodyIndex(TEXT("pelvis"));
    if (!Mesh->GetPhysicsAsset()->SkeletalBodySetups.IsValidIndex(SetupIndex)) return BIG_NUMBER;
    const USkeletalBodySetup* Setup=Mesh->GetPhysicsAsset()->SkeletalBodySetups[SetupIndex];
    if (!Setup || Setup->AggGeom.SphylElems.Num()!=1) return BIG_NUMBER;
    const auto& Ref=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton();
    auto Bind=[&](FName Name)
    {
        FTransform Value=FTransform::Identity;
        for (int32 I=Ref.FindBoneIndex(Name);I!=INDEX_NONE;I=Ref.GetParentIndex(I)) Value*=Ref.GetRefBonePose()[I];
        return Value;
    };
    const FTransform RefThighLocal=Bind(TEXT("thigh_r")).GetRelativeTransform(Bind(TEXT("pelvis")));
    const FTransform PelvisWorld=Mesh->GetSocketTransform(TEXT("pelvis"));
    const FTransform CurrentThighLocal=Mesh->GetSocketTransform(TEXT("thigh_r")).GetRelativeTransform(PelvisWorld);
    const FTransform Delta=RefThighLocal.Inverse()*CurrentThighLocal;
    const FVector RefCenter=Setup->AggGeom.SphylElems[0].Center;
    const FVector ExpectedWorld=PelvisWorld.TransformPosition(Delta.TransformPosition(RefCenter));
    float Error=BIG_NUMBER;
    FPhysicsCommand::ExecuteWrite(Pelvis->GetPhysicsActor(),[&](const FPhysicsActorHandle& Actor)
    {
        TArray<FPhysicsShapeHandle> Shapes; Pelvis->GetAllShapes_AssumesLocked(Shapes);
        for (FPhysicsShapeHandle& Shape:Shapes)
        {
            if (FPhysicsInterface::GetShapeType(Shape)!=ECollisionShapeType::Capsule) continue;
            if (!Missing)
            {
                FPhysicsInterface::SetIsSimulationShape(Shape,false);
                FPhysicsInterface::SetIsQueryShape(Shape,false); Error=0.f; break;
            }
            // Chaos bakes primitive centers into geometry. Preserve the existing
            // outer transform and apply a bone-space delta, not a second center.
            const FTransform Outer=FPhysicsInterface::GetLocalTransform(Shape);
            const FVector InnerCenter=Outer.InverseTransformPosition(RefCenter);
            FPhysicsInterface::SetLocalTransform(Shape,Outer*Delta);
            // SetLocalTransform replaces this actor's union geometry. Reacquire
            // shape handles before touching filters or reading the actual result.
            TArray<FPhysicsShapeHandle> Updated; Pelvis->GetAllShapes_AssumesLocked(Updated);
            for (auto& NewShape:Updated)
                if (FPhysicsInterface::GetShapeType(NewShape)==ECollisionShapeType::Capsule)
                {
                    FPhysicsInterface::SetIsSimulationShape(NewShape,true);
                    FPhysicsInterface::SetIsQueryShape(NewShape,false);
                    const FVector ActualLocal=FPhysicsInterface::GetLocalTransform(NewShape).TransformPosition(InnerCenter);
                    const FTransform ActualPelvis=FPhysicsInterface::GetGlobalPose_AssumesLocked(Actor);
                    Error=float(FVector::Dist(ActualPelvis.TransformPosition(ActualLocal),ExpectedWorld)); break;
                }
            break;
        }
    });
    return Error;
}
int32 ONEPhysicsRuntime::Count(USkeletalMeshComponent* Mesh,bool AwakeOnly,FName Root)
{
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Mesh->GetPhysicsAsset()) return 0;
    int32 Number=0;
    for (const USkeletalBodySetup* Setup:Mesh->GetPhysicsAsset()->SkeletalBodySetups)
    {
        if (!Setup || !InChain(Mesh,Setup->BoneName,Root)) continue;
        auto* Body=Mesh->GetBodyInstance(Setup->BoneName);
        if (Body && Body->IsValidBodyInstance() && Body->IsInstanceSimulatingPhysics() && (!AwakeOnly || Body->IsInstanceAwake())) ++Number;
    }
    return Number;
}
int32 ONEPhysicsRuntime::ExistingChainBodies(USkeletalMeshComponent* Mesh,FName Root)
{
    if (!Mesh || !Mesh->GetSkeletalMeshAsset() || !Mesh->GetPhysicsAsset()) return 0;
    int32 Number=0;
    for (const USkeletalBodySetup* Setup:Mesh->GetPhysicsAsset()->SkeletalBodySetups)
        if (Setup && InChain(Mesh,Setup->BoneName,Root))
            if (auto* Body=Mesh->GetBodyInstance(Setup->BoneName)) if (Body->IsValidBodyInstance()) ++Number;
    return Number;
}
