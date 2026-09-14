#include "ONEZombie.h"
#include "ONEInfectedAnimInstance.h"
#include "ONEInfectedVariant.h"
#include "ONEZombieAudioComponent.h"
#include "ONESnapshotAnimInstance.h"
#include "ONEPlayer.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(ONEPhysicality);

namespace ONERecoveryCollision
{
    bool SupportsStanding(const UPrimitiveComponent* Component)
    {
        // Collection triggers and damage-only regions cannot support the
        // transport capsule, even when their object type is WorldDynamic.
        return Component && Component->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Block;
    }
    bool OccupiesRecoverySpace(const UPrimitiveComponent* Component)
    {
        if (!Component) return false;
        if (SupportsStanding(Component)) return true;
        // Fallen meshes deliberately use PhysicsOnly. Their current-pose
        // regional queries still represent occupied anatomy for clearance.
        if (Cast<AONEZombie>(Component->GetOwner()) &&
            Component->GetCollisionResponseToChannel(ECC_Visibility)==ECR_Block) return true;
        return Component->IsPhysicsCollisionEnabled() &&
            Component->GetCollisionResponseToChannel(ECC_PhysicsBody)==ECR_Block;
    }
}

FVector AONEZombie::GetPhysicalRewardLocation() const
{
    // A full-physics body can move away from its disabled transport capsule.
    // Reward eligibility and probability are unchanged; only its origin follows
    // the real body at death instead of the abandoned standing position.
    if (bRagdollActive && GetMesh()->GetSkeletalMeshAsset())
    {
        const FVector Pelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
        if (!Pelvis.ContainsNaN()) return Pelvis;
    }
    return GetActorLocation();
}

FONEInfectedAnimationState AONEZombie::GetInfectedAnimationState() const
{
    FONEInfectedAnimationState Result;
    Result.StateAgeSeconds=GetStateElapsed();
    Result.ActionSerial=uint64(AttackSerial);
    Result.GaitPhaseOffset=GaitPhaseOffset;
    Result.ContactDirectionWorld=ContactDirection; Result.ContactStrength=ContactStrength;
    switch (State)
    {
        case EONEZombieState::Attack:
            Result.Motion=EONEInfectedMotionState::Attack;
            Result.ActionClip=GetAttackClipKey(); Result.ActionDurationSeconds=GetCurrentAttackDuration();
            Result.RecoveryLocomotionAlpha=AttackRecoveryAlpha;
            Result.EntryForwardSpeed=AttackEntrySpeed; Result.CommittedTravelCm=CommittedAttackTravel;
            break;
        case EONEZombieState::Hit:
            Result.Motion=EONEInfectedMotionState::HeavyHit;
            Result.ActionClip=TEXT("HeavyHit"); Result.ActionDurationSeconds=.52f; break;
        case EONEZombieState::Stumble:
            Result.Motion=EONEInfectedMotionState::Stumble;
            Result.ActionClip=TEXT("Stumble"); Result.ActionDurationSeconds=.58f; break;
        case EONEZombieState::Fallen: Result.Motion=EONEInfectedMotionState::Fallen; break;
        case EONEZombieState::GetUp:
            Result.Motion=EONEInfectedMotionState::GetUp;
            Result.ActionClip=bGetUpSupine?TEXT("GetUpSupine"):TEXT("GetUpProne");
            Result.ActionDurationSeconds=GetUpDuration-.35f; Result.ActionLeadInSeconds=.35f;
            Result.RecoverySnapshotAlpha=1.f-FMath::SmoothStep(0.f,.35f,Result.StateAgeSeconds);
            break;
        case EONEZombieState::Dead: Result.Motion=EONEInfectedMotionState::Dead; break;
        default: break;
    }
    return Result;
}

TArray<FName> AONEZombie::MissingPhysicsRoots() const
{
    TArray<FName> Missing;
    if (!HasHead()) Missing.Add(TEXT("head"));
    if (!HasLeftArm()) Missing.Add(TEXT("upperarm_r"));
    if (!HasRightArm()) Missing.Add(TEXT("upperarm_l"));
    if (!HasLeftLeg()) Missing.Add(TEXT("thigh_r"));
    return Missing;
}

void AONEZombie::CaptureCurrentPose()
{
    if (auto* Anim=Cast<UONEInfectedAnimInstance>(GetMesh()->GetAnimInstance()))
        GetMesh()->SnapshotPose(Anim->CapturedDeathPose);
    else if (auto* SnapshotAnim=Cast<UONESnapshotAnimInstance>(GetMesh()->GetAnimInstance()))
        GetMesh()->SnapshotPose(SnapshotAnim->CapturedPose);
}

void AONEZombie::StopLivingPhysicalResponse()
{
    bLivingUpperPhysics=false;
    if (!PhysicalAnimation) return;
    // Zero motor strength alone leaves target actors and teleport delegates
    // reading the old pose. Release them before full physics or mesh teardown.
    PhysicalAnimation->SetComponentTickEnabled(false);
    PhysicalAnimation->SetStrengthMultiplyer(0.f);
    PhysicalAnimation->SetSkeletalMeshComponent(nullptr);
}

void AONEZombie::StartLivingPhysicalResponse()
{
    if (IsDead() || IsLivingFallen() || IsGettingUp() || !GetMesh()->GetPhysicsAsset()) return;
    auto* PhysicalMesh=GetMesh();
    // The capsule transports the kinematic pelvis/legs. Only the upper-body
    // subtree yields to contact; it cannot drive the capsule or inflict damage.
    PhysicalMesh->bLocalSpaceKinematics=true;
    PhysicalMesh->SetCollisionObjectType(ECC_PhysicsBody);
    PhysicalMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    PhysicalMesh->SetCollisionResponseToChannel(ECC_WorldStatic,ECR_Block);
    PhysicalMesh->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
    PhysicalMesh->SetCollisionResponseToChannel(ECC_PhysicsBody,ECR_Block);
    PhysicalMesh->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
    PhysicalMesh->SetAllBodiesSimulatePhysics(false);
    PhysicalMesh->SetAllBodiesPhysicsBlendWeight(0.f);
    PhysicalMesh->UpdateKinematicBonesToAnim(PhysicalMesh->GetComponentSpaceTransforms(),ETeleportType::TeleportPhysics,true,EAllowKinematicDeferral::DisallowDeferral);
    PhysicalAnimation->SetSkeletalMeshComponent(PhysicalMesh);
    PhysicalAnimation->SetComponentTickEnabled(true);
    FPhysicalAnimationData Drive;
    Drive.bIsLocalSimulation=true;
    Drive.OrientationStrength=900.f; Drive.AngularVelocityStrength=100.f;
    Drive.MaxAngularForce=6500.f; Drive.MaxLinearForce=3500.f;
    PhysicalAnimation->ApplyPhysicalAnimationSettingsBelow(TEXT("spine_01"),Drive,true);
    PhysicalAnimation->SetStrengthMultiplyer(1.f);
    PhysicalMesh->SetAllBodiesBelowSimulatePhysics(TEXT("spine_01"),true,true);
    PhysicalMesh->SetAllBodiesBelowPhysicsBlendWeight(TEXT("spine_01"),.65f,false,true);
    ONEPhysicsRuntime::ConfigureLeftStump(PhysicalMesh,!HasLeftLeg());
    for (FName Root:MissingPhysicsRoots()) PhysicalMesh->TermBodiesBelow(Root);
    PhysicalMesh->SetAllBodiesNotifyRigidBodyCollision(true);
    bLivingUpperPhysics=true;
}

void AONEZombie::OnFootContact(bool bLeft)
{
    if (ZombieAudio && !IsDead() && !IsLivingFallen() && !IsGettingUp()) ZombieAudio->NotifyFootContact(bLeft);
}

void AONEZombie::OnPhysicalContact(UPrimitiveComponent* HitComponent,AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit)
{
    if (!OtherActor || OtherActor==this || !OtherComponent || NormalImpulse.ContainsNaN()) return;
    const FVector Normal=Hit.ImpactNormal.GetSafeNormal(SMALL_NUMBER,FVector::UpVector);
    const float Mass=FMath::Max(1.f,GetMesh()->GetBoneMass(Hit.MyBoneName));
    const float ImpactSpeed=FMath::Clamp(float(NormalImpulse.Size())/Mass,0.f,500.f);
    if (bRecoveryEffortActive && Normal.Z<.65f && ImpactSpeed>45.f &&
        FVector::DotProduct(Normal,RecoveryEffortDirection)<-.2f)
        StopRecoveryEffort(TEXT("opposing_contact"));
    const float Now=GetWorld()->GetTimeSeconds();
    const float* Last=ContactCooldowns.Find(OtherActor);
    if (Last && Now-*Last<.18f) return;
    ContactCooldowns.Add(OtherActor,Now);
    if (ContactCooldowns.Num()>24)
        for (auto It=ContactCooldowns.CreateIterator();It;++It)
            if (!It.Key().IsValid() || Now-It.Value()>2.f) It.RemoveCurrent();
    if (ZombieAudio && ImpactSpeed>=25.f) ZombieAudio->NotifyBodyContact(Hit.ImpactPoint,ImpactSpeed);
    if (IsDead() || IsLivingFallen() || IsGettingUp() || Normal.Z>.65f) return;
    FVector OtherVelocity=OtherActor->GetVelocity();
    if (const auto* Other=Cast<AONEZombie>(OtherActor)) OtherVelocity=Other->PreviousMovementVelocity;
    const float Closing=FMath::Max(0.f,float(FVector::DotProduct(PreviousMovementVelocity-OtherVelocity,-Normal)));
    // Constraint correction impulses alone cannot topple an idle actor.
    if (Closing>=25.f) RegisterContact(Normal,Closing,Hit.ImpactPoint,true);
}

void AONEZombie::OnCapsuleContact(UPrimitiveComponent* HitComponent,AActor* OtherActor,
    UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& Hit)
{
    if (IsDead() || IsLivingFallen() || IsGettingUp() || !OtherActor || OtherActor==this ||
        !OtherComponent || Hit.ImpactNormal.Z>.65f) return;
    const float Now=GetWorld()->GetTimeSeconds();
    const float* Last=ContactCooldowns.Find(OtherActor);
    if (Last && Now-*Last<.18f) return;
    FVector OtherVelocity=OtherActor->GetVelocity();
    if (const auto* Other=Cast<AONEZombie>(OtherActor)) OtherVelocity=Other->PreviousMovementVelocity;
    const FVector Normal=Hit.ImpactNormal.GetSafeNormal2D();
    const float Closing=FMath::Max(0.f,float(FVector::DotProduct(PreviousMovementVelocity-OtherVelocity,-Normal)));
    if (Closing<25.f) return;
    ContactCooldowns.Add(OtherActor,Now);
    RegisterContact(Normal,Closing,GetMesh()->GetSocketLocation(TEXT("spine_02")),false);
}

void AONEZombie::RegisterContact(const FVector& Direction,float ClosingSpeed,const FVector& Position,bool Physical)
{
    if (!bLivingUpperPhysics || State==EONEZombieState::Hit) return;
    const float Now=GetWorld()->GetTimeSeconds();
    ++PhysicalContactCount; LastContact=Now;
    ContactDirection=Direction.GetSafeNormal2D();
    ContactStrength=FMath::Max(ContactStrength,FMath::Clamp(ClosingSpeed/240.f,.1f,.85f));
    ContactPressure=FMath::Min(2.5f,ContactPressure+FMath::Clamp((ClosingSpeed-45.f)/145.f,0.f,1.f));
    // Capsule contacts transfer one bounded shoulder nudge into the simulated
    // upper body. Real skeletal contacts already received a solver impulse.
    if (!Physical)
        GetMesh()->AddImpulseAtLocation(ContactDirection*FMath::Clamp(ClosingSpeed*1.5f,35.f,220.f),Position,TEXT("spine_02"));
    if (Now>=NextFallAllowed && ClosingSpeed>=135.f && ContactPressure>=1.7f)
    {
        if (TryLivingFall(ContactDirection*280.f+FVector(0,0,20),TEXT("sustained_contact"))) return;
    }
    if (State==EONEZombieState::Pursue && ClosingSpeed>=95.f && Now-LastReaction>=HitReactCooldown)
    {
        LastReaction=Now;
        if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
        ChangeState(EONEZombieState::Stumble);
        NextAttack=FMath::Max(NextAttack,Now+.58f);
    }
}

bool AONEZombie::TryLivingFall(const FVector& Impulse,FName Cause)
{
    const bool RecoveryInterrupt=IsGettingUp() && Cause==TEXT("heavy_getup_hit");
    if (IsDead() || IsLivingFallen() || (IsGettingUp() && !RecoveryInterrupt) || !GetWorld() ||
        !HasHead() || !HasLeftLeg() || !GetMesh()->GetSkeletalMeshAsset() || !GetMesh()->GetPhysicsAsset() ||
        !FMath::IsFinite(Impulse.SizeSquared()) || (!RecoveryInterrupt && GetWorld()->GetTimeSeconds()<NextFallAllowed)) return false;
    int32 SimulatedLiving=0;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It) SimulatedLiving+=It->IsLivingFallen()?1:0;
    // At most four living full-body simulations. Other contact remains upper
    // body response/stumble; a crowd cannot start an unbounded knockdown chain.
    if (SimulatedLiving>=4) return false;
    const FVector Inherited=GetVelocity().GetClampedToMaxSize(PursuitSpeed);
    CaptureCurrentPose();
    StopLivingPhysicalResponse();
    StopPursuit(); ChangeState(EONEZombieState::Fallen); bContactDelivered=true;
    GetCharacterMovement()->DisableMovement(); GetCharacterMovement()->SetAvoidanceEnabled(false);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const auto Result=ONEPhysicsRuntime::Start(GetMesh(),Inherited,MissingPhysicsRoots());
    bRagdollActive=Result.SimulatedBodies>0;
    RagdollPositionError=Result.PositionErrorCm; RagdollAngleError=Result.AngleErrorDegrees;
    if (!bRagdollActive)
    {
        ChangeState(EONEZombieState::Pursue);
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        GetCharacterMovement()->SetMovementMode(MOVE_Walking); StartLivingPhysicalResponse();
        return false;
    }
    StumpFitError=Result.StumpFitErrorCm;
    RestState.Frozen=false; RestState.FrozenBodyCount=0;
    ONEPhysicsRuntime::ResetRest(GetMesh(),RestState,false);
    GetMesh()->AddImpulseAtLocation(Impulse.GetClampedToMaxSize(330.f),GetMesh()->GetSocketLocation(TEXT("spine_01")),TEXT("spine_01"));
    FallStartPelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
    ++LivingFallCount; RecoveryQuietSince=-1; RecoveryAttemptsInBurst=0;
    bRecoveryEffortActive=false; bRecoveryEffortBudgetReported=false; RecoveryEffortCount=0;
    RecoveryEffortTravelCm=0; NextRecoveryEffort=0; RecoveryEffortBlocker.Reset();
    NextRecoveryAttempt=GetWorld()->GetTimeSeconds()+1.1f;
    NextFallAllowed=GetWorld()->GetTimeSeconds()+8.f;
    if (ZombieAudio) ZombieAudio->NotifyFall();
    FTimerManagerTimerParameters Timer; Timer.bLoop=true; Timer.bMaxOncePerFrame=true;
    GetWorld()->GetTimerManager().SetTimer(RestTimer,this,&AONEZombie::ObserveRest,.1f,Timer);
    UE_LOG(LogTemp,Display,TEXT("ONE07_LIVING_FALL id=%u cause=%s health=%.3f count=%d pose_cm=%.5f"),
        GetUniqueID(),*Cause.ToString(),GetHealth(),LivingFallCount,RagdollPositionError);
    return true;
}

bool AONEZombie::FindRecoveryFloor(const FVector& Position,FHitResult& Floor) const
{
    if (Position.ContainsNaN()) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InfectedRecovery),false,this);
    FCollisionObjectQueryParams Solids; Solids.AddObjectTypesToQuery(ECC_WorldStatic);
    Solids.AddObjectTypesToQuery(ECC_WorldDynamic); Solids.AddObjectTypesToQuery(ECC_PhysicsBody);
    // Object traces may stop at their first blocking result. Ignore only each
    // rejected component, so a trigger cannot hide solid geometry behind it.
    for (int32 Attempt=0;Attempt<32;++Attempt)
    {
        if (!GetWorld()->LineTraceSingleByObjectType(Floor,Position+FVector(0,0,30),Position-FVector(0,0,140),Solids,Params)) break;
        if (ONERecoveryCollision::SupportsStanding(Floor.GetComponent()))
            return Floor.ImpactNormal.Z>=.85f && !Floor.bStartPenetrating;
        if (!Floor.GetComponent()) break;
        Params.AddIgnoredComponent(Floor.GetComponent());
    }
    return false;
}

bool AONEZombie::FindRecoverySpace(FVector& CapsuleLocation,FRotator& Facing,UPrimitiveComponent** AnatomyBlocker) const
{
    if (AnatomyBlocker) *AnatomyBlocker=nullptr;
    const auto* PhysicalMesh=GetMesh();
    const FVector Pelvis=PhysicalMesh->GetSocketLocation(TEXT("pelvis"));
    FHitResult Floor;
    if (!FindRecoveryFloor(Pelvis,Floor)) return false;
    // The only solution is directly above the actual body's supported pelvis.
    // No offset search toward the player, navigation projection or distant warp.
    CapsuleLocation=FVector(Pelvis.X,Pelvis.Y,Floor.ImpactPoint.Z+GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f);
    if (FMath::Abs(Pelvis.Z-Floor.ImpactPoint.Z)>105.f) return false;
    const FVector Head=PhysicalMesh->GetSocketLocation(TEXT("head"));
    const FVector Feet=(PhysicalMesh->GetSocketLocation(TEXT("foot_r"))+PhysicalMesh->GetSocketLocation(TEXT("foot_l")))*.5f;
    const FVector Axis=(Head-Feet).GetSafeNormal2D(SMALL_NUMBER,GetActorForwardVector());
    Facing=FRotator(0,Axis.Rotation().Yaw,0);
    // Extra shoulder/get-up clearance, including pawn capsules and movable
    // solid cover. Physics-only own body is ignored by actor identity.
    FCollisionQueryParams Params(SCENE_QUERY_STAT(InfectedRecovery),false,this);
    FCollisionObjectQueryParams Clearance; Clearance.AddObjectTypesToQuery(ECC_WorldStatic);
    Clearance.AddObjectTypesToQuery(ECC_WorldDynamic); Clearance.AddObjectTypesToQuery(ECC_PhysicsBody);
    Clearance.AddObjectTypesToQuery(ECC_Pawn);
    const FVector Center(CapsuleLocation.X,CapsuleLocation.Y,Floor.ImpactPoint.Z+96.f);
    bool HardBlocker=false;
    UPrimitiveComponent* Nearest=nullptr; double NearestDistance=DBL_MAX;
    auto Occupied=[&](const FVector& At,const FQuat& Rotation,const FCollisionShape& Shape)
    {
        TArray<FOverlapResult> Overlaps;
        GetWorld()->OverlapMultiByObjectType(Overlaps,At,Rotation,Clearance,Shape,Params);
        bool Blocked=false;
        for (const FOverlapResult& Overlap:Overlaps)
            if (ONERecoveryCollision::OccupiesRecoverySpace(Overlap.GetComponent()))
            {
                if (!Blocked && RecoveryAttemptsInBurst==5 && FParse::Param(FCommandLine::Get(),TEXT("ONE07RecoveryDiagnostics")))
                    UE_LOG(LogTemp,Display,TEXT("ONE07_RECOVERY_BLOCKED id=%u pelvis=%s volume=%s blocker=%s component=%s location=%s"),
                        GetUniqueID(),*Pelvis.ToCompactString(),*At.ToCompactString(),*GetNameSafe(Overlap.GetActor()),
                        *GetNameSafe(Overlap.GetComponent()),*Overlap.GetComponent()->GetComponentLocation().ToCompactString());
                Blocked=true;
                const auto* Other=Cast<AONEZombie>(Overlap.GetActor());
                if (!Other || (!Other->IsDead() && !Other->IsLivingFallen())) HardBlocker=true;
                else
                {
                    const double Distance=FVector::DistSquared2D(Pelvis,Overlap.GetComponent()->GetComponentLocation());
                    if (Distance<NearestDistance) { Nearest=Overlap.GetComponent(); NearestDistance=Distance; }
                }
            }
        return Blocked;
    };
    const bool UprightOccupied=Occupied(Center,FQuat::Identity,FCollisionShape::MakeCapsule(43.f,93.f));
    // The initial body, palms and feet occupy more floor than the upright
    // capsule. Wait if another body or cover occupies that rollout footprint.
    const FQuat AlongBody=FQuat::FindBetweenNormals(FVector::UpVector,Axis);
    const bool RolloutOccupied=Occupied(FVector(Pelvis.X,Pelvis.Y,Floor.ImpactPoint.Z+29.f),AlongBody,FCollisionShape::MakeCapsule(26.f,96.f));
    // An anatomical obstacle in one volume must not hide solid cover/player
    // occupancy in the other. Both original clearance shapes still must pass.
    if (AnatomyBlocker) *AnatomyBlocker=HardBlocker?nullptr:Nearest;
    return !UprightOccupied && !RolloutOccupied;
}

bool AONEZombie::HasRecoveryEffortFloor(const FVector& Direction) const
{
    const FVector Pelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
    FHitResult Support;
    if (!FindRecoveryFloor(Pelvis,Support) || FMath::Abs(Pelvis.Z-Support.ImpactPoint.Z)>105.f) return false;
    const FVector Feet=(GetMesh()->GetSocketLocation(TEXT("foot_r"))+GetMesh()->GetSocketLocation(TEXT("foot_l")))*.5f;
    const FVector Axis=(GetMesh()->GetSocketLocation(TEXT("head"))-Feet).GetSafeNormal2D(SMALL_NUMBER,GetActorForwardVector());
    const FVector Side=FVector::CrossProduct(FVector::UpVector,Axis);
    // Check the actual rollout footprint and its short destination on the same
    // floor. No navigation projection, cover top, ledge crossing or Z force.
    const FVector Goal=Pelvis+Direction*30.f;
    const FVector Probes[]={Pelvis+Direction*15.f,Goal,Goal+Axis*70.f,Goal-Axis*70.f,Goal+Side*26.f,Goal-Side*26.f};
    for (const FVector& Probe:Probes)
    {
        FHitResult Floor;
        if (!FindRecoveryFloor(Probe,Floor) || FMath::Abs(Floor.ImpactPoint.Z-Support.ImpactPoint.Z)>6.f) return false;
    }
    return true;
}

bool AONEZombie::BeginRecoveryEffort(UPrimitiveComponent* AnatomyBlocker)
{
    const auto* Other=AnatomyBlocker?Cast<AONEZombie>(AnatomyBlocker->GetOwner()):nullptr;
    if (!Other || (!Other->IsDead() && !Other->IsLivingFallen()) || !IsLivingFallen()) return false;
    const float Now=GetWorld()->GetTimeSeconds();
    if (Now<NextRecoveryEffort) return false;
    const FVector Pelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
    if (RecoveryEffortCount>=6 || RecoveryEffortTravelCm>=90.f ||
        (RecoveryEffortCount>0 && FVector::Dist2D(Pelvis,RecoveryEffortAnchor)>=90.f))
    {
        if (!bRecoveryEffortBudgetReported)
            UE_LOG(LogTemp,Display,TEXT("ONE07_RECOVERY_EFFORT_LIMIT id=%u attempts=%d travel_cm=%.3f still_fallen=1"),GetUniqueID(),RecoveryEffortCount,RecoveryEffortTravelCm);
        bRecoveryEffortBudgetReported=true; return false;
    }
    FVector Closest=AnatomyBlocker->GetComponentLocation(),Surface;
    if (AnatomyBlocker->GetClosestPointOnCollision(Pelvis,Surface)>0.f && !Surface.ContainsNaN()) Closest=Surface;
    const FVector Direction=(Pelvis-Closest).GetSafeNormal2D();
    if (Direction.IsNearlyZero() || !HasRecoveryEffortFloor(Direction)) return false;
    // Supported rest may have made retained bodies kinematic. Resume their
    // captured physical pose once; never resurrect a severed chain.
    const bool WasFrozen=RestState.Frozen;
    ONEPhysicsRuntime::ResetRest(GetMesh(),RestState,true);
    if (GetActivePhysicsBodyCount()==0) return false;
    if (RecoveryEffortCount==0) { RecoveryEffortAnchor=Pelvis; RecoveryEffortLastPelvis=Pelvis; }
    ++RecoveryEffortCount; bRecoveryEffortActive=true;
    RecoveryEffortDirection=Direction; RecoveryEffortStartPelvis=Pelvis;
    RecoveryEffortStartTravelCm=RecoveryEffortTravelCm; RecoveryEffortMaxSpeed=0;
    RecoveryEffortStarted=Now; NextRecoveryFloorCheck=Now+.1f; RecoveryEffortDamageSerial=DamageTransactions;
    RecoveryEffortBlocker=AnatomyBlocker; RecoveryQuietSince=-1;
    UE_LOG(LogTemp,Display,TEXT("ONE07_RECOVERY_EFFORT_BEGIN id=%u attempt=%d blocker=%s component=%s pelvis=%s direction=%s resumed_frozen=%d retained=%d health=%.3f"),
        GetUniqueID(),RecoveryEffortCount,*GetNameSafe(Other),*GetNameSafe(AnatomyBlocker),*Pelvis.ToCompactString(),*Direction.ToCompactString(),WasFrozen,GetActivePhysicsBodyCount(),GetHealth());
    return true;
}

void AONEZombie::StopRecoveryEffort(const TCHAR* Reason)
{
    if (!bRecoveryEffortActive) return;
    bRecoveryEffortActive=false; NextRecoveryEffort=GetWorld()->GetTimeSeconds()+2.f;
    NextRecoveryAttempt=NextRecoveryEffort; RecoveryQuietSince=-1;
    const FVector Pelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
    UE_LOG(LogTemp,Display,TEXT("ONE07_RECOVERY_EFFORT_END id=%u attempt=%d reason=%s pelvis=%s progress_cm=%.3f travel_cm=%.3f max_pelvis_speed=%.3f"),
        GetUniqueID(),RecoveryEffortCount,Reason,*Pelvis.ToCompactString(),FVector::Dist2D(Pelvis,RecoveryEffortStartPelvis),RecoveryEffortTravelCm,RecoveryEffortMaxSpeed);
    RecoveryEffortBlocker.Reset();
}

void AONEZombie::TickRecoveryEffort(float Dt)
{
    const auto* Other=RecoveryEffortBlocker.IsValid()?Cast<AONEZombie>(RecoveryEffortBlocker->GetOwner()):nullptr;
    if (!IsLivingFallen() || DamageTransactions!=RecoveryEffortDamageSerial || !Other || (!Other->IsDead() && !Other->IsLivingFallen()))
    { StopRecoveryEffort(TEXT("state_or_damage")); return; }
    const float Now=GetWorld()->GetTimeSeconds();
    if (Now-RecoveryEffortStarted>=.55f || RecoveryEffortTravelCm-RecoveryEffortStartTravelCm>=28.f ||
        RecoveryEffortTravelCm>=90.f || FVector::Dist2D(GetMesh()->GetSocketLocation(TEXT("pelvis")),RecoveryEffortAnchor)>=90.f || Dt>.1f)
    { StopRecoveryEffort(TEXT("bounded_interval")); return; }
    if (Now>=NextRecoveryFloorCheck)
    {
        NextRecoveryFloorCheck=Now+.1f;
        if (!HasRecoveryEffortFloor(RecoveryEffortDirection)) { StopRecoveryEffort(TEXT("lost_floor")); return; }
    }
    FBodyInstance* Pelvis=GetMesh()->GetBodyInstance(TEXT("pelvis"));
    FBodyInstance* Spine=GetMesh()->GetBodyInstance(TEXT("spine_01"));
    if (!Pelvis || !Spine || !Pelvis->IsInstanceSimulatingPhysics() || !Spine->IsInstanceSimulatingPhysics())
    { StopRecoveryEffort(TEXT("missing_simulation")); return; }
    const float Speed=float(Pelvis->GetUnrealWorldVelocity().Size2D());
    RecoveryEffortMaxSpeed=FMath::Max(RecoveryEffortMaxSpeed,Speed);
    if (Speed>70.f || FMath::Abs(Pelvis->GetUnrealWorldVelocity().Z)>45.f)
    { StopRecoveryEffort(TEXT("outside_impulse")); return; }
    float Mass=0;
    for (const USkeletalBodySetup* Setup:GetMesh()->GetPhysicsAsset()->SkeletalBodySetups)
        if (Setup) if (const auto* Body=GetMesh()->GetBodyInstance(Setup->BoneName))
            if (Body->IsValidBodyInstance() && Body->IsInstanceSimulatingPhysics()) Mass+=Body->GetBodyMass();
    // The inherited .65 friction must carry the whole retained body. Distribute
    // bounded horizontal effort at the hips/chest rather than tiny repeated
    // impulses or overwriting solver velocities. Contact remains authoritative.
    for (FBodyInstance* Body:{Pelvis,Spine})
    {
        const FVector Velocity=Body->GetUnrealWorldVelocity();
        if (Velocity.Size2D()>=55.f) continue;
        const float Along=float(FVector::DotProduct(Velocity,RecoveryEffortDirection));
        const float Acceleration=FMath::Clamp((50.f-Along)*30.f,0.f,1100.f);
        const float Share=Body==Pelvis?.55f:.45f;
        const float LateralSquared=FMath::Max(0.f,float(Velocity.SizeSquared2D())-Along*Along);
        const float MaximumAlong=FMath::Sqrt(FMath::Max(0.f,55.f*55.f-LateralSquared));
        const float Force=FMath::Min(Mass*Share*Acceleration,Body->GetBodyMass()*FMath::Max(0.f,MaximumAlong-Along)/FMath::Max(Dt,1.f/120.f));
        Body->AddForce(RecoveryEffortDirection*Force,true,false);
    }
}

bool AONEZombie::TryBeginGetUp()
{
    if (!IsLivingFallen() || IsDead()) return false;
    FVector SafeLocation; FRotator Facing;
    UPrimitiveComponent* AnatomyBlocker=nullptr;
    if (!FindRecoverySpace(SafeLocation,Facing,&AnatomyBlocker))
    {
        ++RecoveryBlockedCount; ++RecoveryAttemptsInBurst;
        NextRecoveryAttempt=GetWorld()->GetTimeSeconds()+(RecoveryAttemptsInBurst>=6?2.f:.4f);
        if (RecoveryAttemptsInBurst>=6) { RecoveryAttemptsInBurst=0; BeginRecoveryEffort(AnatomyBlocker); }
        return false;
    }
    auto* PhysicalMesh=GetMesh();
    FPoseSnapshot Snapshot; PhysicalMesh->SnapshotPose(Snapshot);
    if (!Snapshot.bIsValid || Snapshot.LocalTransforms.IsEmpty())
    { ++RecoveryBlockedCount; NextRecoveryAttempt=GetWorld()->GetTimeSeconds()+1.f; return false; }
    const FTransform BeforeFrame=PhysicalMesh->GetComponentTransform();
    TMap<FName,FVector> Before;
    for (FName Bone:{FName(TEXT("pelvis")),FName(TEXT("head")),FName(TEXT("hand_r")),FName(TEXT("hand_l")),FName(TEXT("foot_r")),FName(TEXT("foot_l"))})
        Before.Add(Bone,PhysicalMesh->GetSocketLocation(Bone));
    // Record the actual chest orientation before disabling physics.
    FTransform SpineBind=FTransform::Identity;
    const auto& Ref=PhysicalMesh->GetSkeletalMeshAsset()->GetRefSkeleton();
    for (int32 Bone=Ref.FindBoneIndex(TEXT("spine_02"));Bone!=INDEX_NONE;Bone=Ref.GetParentIndex(Bone)) SpineBind*=Ref.GetRefBonePose()[Bone];
    bGetUpSupine=PhysicalMesh->GetSocketTransform(TEXT("spine_02")).TransformVectorNoScale(SpineBind.InverseTransformVectorNoScale(FVector::ForwardVector)).Z>0.f;
    const FName RecoveryKey=bGetUpSupine?TEXT("GetUpSupine"):TEXT("GetUpProne");
    const auto* RecoveryClip=Variant?Variant->Clips.Find(RecoveryKey):nullptr;
    const UAnimSequence* RecoverySequence=RecoveryClip?RecoveryClip->LoadSynchronous():nullptr;
    GetUpDuration=.35f+(RecoverySequence?RecoverySequence->GetPlayLength():2.05f);
    StopLivingPhysicalResponse();
    PhysicalMesh->SetAllBodiesSimulatePhysics(false); PhysicalMesh->SetSimulatePhysics(false); PhysicalMesh->SetAllBodiesPhysicsBlendWeight(0.f);
    RestState.Frozen=false; RestState.FrozenBodyCount=0;
    PhysicalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetActorLocationAndRotation(SafeLocation,Facing,false,nullptr,ETeleportType::TeleportPhysics);
    PhysicalMesh->SetRelativeLocationAndRotation(FVector(0,0,-88),FRotator::ZeroRotator);
    const FTransform AfterFrame=PhysicalMesh->GetComponentTransform();
    // Local root rebase keeps every captured bone in its exact world pose even
    // though the empty standing capsule moves to the checked recovery frame.
    Snapshot.LocalTransforms[0]=Snapshot.LocalTransforms[0]*BeforeFrame*AfterFrame.Inverse();
    bRagdollActive=false;
    ChangeState(EONEZombieState::GetUp); bContactDelivered=true;
    PhysicalMesh->SetAnimInstanceClass(UONEInfectedAnimInstance::StaticClass());
    if (auto* Anim=Cast<UONEInfectedAnimInstance>(PhysicalMesh->GetAnimInstance()))
    {
        Anim->CapturedRecoveryPose=Snapshot;
        if (Variant) Anim->ConfigureClips(Variant->Clips);
        // UE keeps an existing instance when its class is unchanged. Recovery
        // must not accumulate another foot/effort subscriber on that instance.
        Anim->OnFootContact.RemoveAll(this);
        Anim->OnAttackEffort.RemoveAll(ZombieAudio.Get());
        Anim->OnFootContact.AddUObject(this,&AONEZombie::OnFootContact);
        Anim->OnAttackEffort.AddUObject(ZombieAudio.Get(),&UONEZombieAudioComponent::NotifyAttack);
    }
    PhysicalMesh->TickAnimation(0.f,false); PhysicalMesh->RefreshBoneTransforms();
    RecoveryRebaseError=0.f;
    for (const auto& Pair:Before) RecoveryRebaseError=FMath::Max(RecoveryRebaseError,float(FVector::Dist(Pair.Value,PhysicalMesh->GetSocketLocation(Pair.Key))));
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GetWorld()->GetTimerManager().ClearTimer(RestTimer);
    UE_LOG(LogTemp,Display,TEXT("ONE07_GETUP_BEGIN id=%u health=%.3f rebase_cm=%.5f blocked=%d clip=%s duration=%.3f"),
        GetUniqueID(),GetHealth(),RecoveryRebaseError,RecoveryBlockedCount,*RecoveryKey.ToString(),GetUpDuration);
    return true;
}

void AONEZombie::CompleteGetUp()
{
    if (State!=EONEZombieState::GetUp || IsDead()) return;
    ++RecoveryCount; ContactPressure=0; NextPath=0;
    ChangeState(EONEZombieState::Pursue);
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->MaxWalkSpeed=PursuitSpeed;
    NextAttack=GetWorld()->GetTimeSeconds()+.3f;
    StartLivingPhysicalResponse();
    UE_LOG(LogTemp,Display,TEXT("ONE07_GETUP_COMPLETE id=%u health=%.3f count=%d"),GetUniqueID(),GetHealth(),RecoveryCount);
}

void AONEZombie::TickLivingPhysicality(float Dt)
{
    if (bRecoveryEffortActive && !IsLivingFallen()) StopRecoveryEffort(TEXT("state_changed"));
    ContactStrength=FMath::Max(0.f,ContactStrength-Dt*1.8f);
    ContactPressure=FMath::Max(0.f,ContactPressure-Dt*.55f);
    if (IsGettingUp())
    {
        if (GetStateElapsed()>=GetUpDuration) CompleteGetUp();
        return;
    }
    if (IsLivingFallen())
    {
        if (RecoveryEffortCount>0)
        {
            const FVector Pelvis=GetMesh()->GetSocketLocation(TEXT("pelvis"));
            RecoveryEffortTravelCm+=float(FVector::Dist2D(Pelvis,RecoveryEffortLastPelvis));
            RecoveryEffortLastPelvis=Pelvis;
        }
        if (bRecoveryEffortActive) { TickRecoveryEffort(Dt); return; }
        const float Now=GetWorld()->GetTimeSeconds();
        if (Now<NextRecoveryAttempt) return;
        float MaxSpeed=0,MaxSpin=0;
        for (FName Bone:{FName(TEXT("pelvis")),FName(TEXT("spine_01")),FName(TEXT("head"))})
            if (const FBodyInstance* Body=GetMesh()->GetBodyInstance(Bone))
            { MaxSpeed=FMath::Max(MaxSpeed,float(Body->GetUnrealWorldVelocity().Size())); MaxSpin=FMath::Max(MaxSpin,float(Body->GetUnrealWorldAngularVelocityInRadians().Size())); }
        if (MaxSpeed>35.f || MaxSpin>1.5f) { RecoveryQuietSince=-1; NextRecoveryAttempt=Now+.2f; return; }
        if (RecoveryQuietSince<0) RecoveryQuietSince=Now;
        if (Now-RecoveryQuietSince>=.35f) TryBeginGetUp();
        return;
    }
    PreviousMovementVelocity=GetVelocity();
    if (bLivingUpperPhysics)
    {
        const float Weight=.65f+ContactStrength*.25f;
        GetMesh()->SetAllBodiesBelowPhysicsBlendWeight(TEXT("spine_01"),Weight,false,true);
    }
}
