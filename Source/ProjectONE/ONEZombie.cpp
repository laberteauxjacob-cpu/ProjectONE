#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEZombieAudioComponent.h"
#include "ONE05AttackMotion.h"
#include "ONEPlayer.h"
#include "ONEInfectedAnimInstance.h"
#include "ONEInfectedVariant.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "ONEPhysicalAnimationComponent.h"
#include "ONESnapshotAnimInstance.h"
#include "ONEBloodSubsystem.h"
#include "ONEPhysicsRuntime.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DECLARE_CATEGORY_EXTERN(ONEPhysicality);
#include "ONEGameMode.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Rendering/SkeletalMeshRenderData.h"

namespace
{
    void EnsureModularPoseBones(USkeletalMesh* Asset)
    {
        if (!Asset || !Asset->GetResourceForRendering()) return;
        const FName Bones[]={TEXT("head"),TEXT("hand_l"),TEXT("hand_r"),TEXT("foot_l"),TEXT("foot_r"),TEXT("toe_l"),TEXT("toe_r")};
        bool Missing=false;
        const FSkeletalMeshRenderData* Data=Asset->GetResourceForRendering();
        for (int32 LOD=0;LOD<Data->LODRenderData.Num();++LOD)
            for (FName Bone:Bones)
            {
                const int32 Index=Asset->GetRefSkeleton().FindBoneIndex(Bone);
                const bool Present=Index!=INDEX_NONE && Data->LODRenderData[LOD].RequiredBones.Contains(Index);
                if (!Present)
                {
                    Missing=true;
                    UE_LOG(LogTemp,Display,TEXT("ONE_MODULAR_LOD missing bone=%s lod=%d; preserving authored evaluation through forced socket"),*Bone.ToString(),LOD);
                }
            }
        if (!Missing) return;
        // Followers cannot add missing bones through the base engine implementation.
        // Force evaluation only if the actual imported render LOD omitted a modular chain.
        for (FName Bone:Bones)
        {
            const FName Name(*FString::Printf(TEXT("ONE_Pose_%s"),*Bone.ToString()));
            if (Asset->FindSocket(Name)) continue;
            USkeletalMeshSocket* Socket=NewObject<USkeletalMeshSocket>(Asset,NAME_None,RF_Transient);
            Socket->SocketName=Name; Socket->BoneName=Bone; Socket->bForceAlwaysAnimated=true;
            Asset->GetMeshOnlySocketList().Add(Socket);
        }
        Asset->RebuildSocketMap();
    }
}

AONEZombie::AONEZombie()
{
    PrimaryActorTick.bCanEverTick=true;
    Tags.Add(TEXT("Infected"));
    Health=CreateDefaultSubobject<UONEHealthComponent>(TEXT("Health"));
    ZombieAudio=CreateDefaultSubobject<UONEZombieAudioComponent>(TEXT("ZombieAudio"));
    PhysicalAnimation=CreateDefaultSubobject<UONEPhysicalAnimationComponent>(TEXT("LivingPhysicalAnimation"));
    AttackDefinitions=ONEInfectedAttacks::Defaults();
    Health->MaxHealth=112;
    GetCapsuleComponent()->InitCapsuleSize(27,88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    GetMesh()->SetRelativeLocation(FVector(0,0,-88));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    GetMesh()->SetAnimInstanceClass(UONEInfectedAnimInstance::StaticClass());
    GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    auto MakePart=[this](const TCHAR* Name)
    {
        auto* Part=CreateDefaultSubobject<USkeletalMeshComponent>(Name);
        Part->SetupAttachment(GetMesh());
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetCanEverAffectNavigation(false);
        Part->SetGenerateOverlapEvents(false);
        return Part;
    };
    HeadMesh=MakePart(TEXT("Head"));
    ArmLeftMesh=MakePart(TEXT("ArmLeft")); ArmRightMesh=MakePart(TEXT("ArmRight"));
    LegLeftMesh=MakePart(TEXT("LegLeft")); ArmMesh=ArmRightMesh;
    auto MakeRegion=[this](const TCHAR* Name,float Radius)
    {
        auto* Region=CreateDefaultSubobject<UCapsuleComponent>(Name);
        Region->SetupAttachment(GetMesh()); Region->InitCapsuleSize(Radius,Radius+10.f);
        return Region;
    };
    // C07's modeled face includes a real nose/jaw silhouette. These dimensions
    // contain the measured skin, with a minimum 0.087 cm source margin.
    HeadRegion=MakeRegion(TEXT("HeadRegion"),9.f); HeadRegion->SetCapsuleHalfHeight(12.3f);
    ArmLeftRegion=MakeRegion(TEXT("ArmLeftRegion"),5.8f);
    UpperArmLeftRegion=MakeRegion(TEXT("UpperArmLeftRegion"),7.f);
    ArmRightRegion=MakeRegion(TEXT("ArmRightRegion"),5.8f);
    UpperArmRightRegion=MakeRegion(TEXT("UpperArmRightRegion"),7.f);
    ArmRegion=ArmRightRegion; UpperArmRegion=UpperArmRightRegion;
    auto MakeLegRegion=[this](const TCHAR* Name,float Radius)
    {
        auto* Region=CreateDefaultSubobject<UCapsuleComponent>(Name);
        Region->SetupAttachment(GetMesh()); Region->InitCapsuleSize(Radius,Radius+20.f);
        return Region;
    };
    LegLeftRegion=MakeLegRegion(TEXT("LegLeftRegion"),6.5f);
    UpperLegLeftRegion=MakeLegRegion(TEXT("UpperLegLeftRegion"),8.f);
    LegRightRegion=MakeLegRegion(TEXT("LegRightRegion"),6.5f);
    UpperLegRightRegion=MakeLegRegion(TEXT("UpperLegRightRegion"),8.f);
    BodyRegion=CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyRegion"));
    BodyRegion->SetupAttachment(GetMesh());
    BodyRegion->InitCapsuleSize(14.f,32.f);
    for (UPrimitiveComponent* C:TArray<UPrimitiveComponent*>{HeadRegion,ArmLeftRegion,UpperArmLeftRegion,ArmRightRegion,UpperArmRightRegion,
        LegLeftRegion,UpperLegLeftRegion,LegRightRegion,UpperLegRightRegion,BodyRegion})
    {
        C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        C->SetCollisionObjectType(ECC_WorldDynamic);
        C->SetCollisionResponseToAllChannels(ECR_Ignore);
        C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        C->SetCanEverAffectNavigation(false); C->SetGenerateOverlapEvents(false);
    }
    AIControllerClass=AAIController::StaticClass();
    AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,300,0);
    GetCharacterMovement()->MaxWalkSpeed=ShambleSpeed;
    GetCharacterMovement()->MaxAcceleration=650;
    GetCharacterMovement()->BrakingDecelerationWalking=1200;
    GetCharacterMovement()->bEnablePhysicsInteraction=false;
    GetCharacterMovement()->bUseRVOAvoidance=true;
    GetCharacterMovement()->AvoidanceConsiderationRadius=250;
}
void AONEZombie::BeginPlay()
{
    Super::BeginPlay();
    PresentationRandom.Initialize(int32(GetUniqueID())*3253+707);
    AttackRandom.Initialize(int32(GetUniqueID())*7919+7007);
    GaitPhaseOffset=PresentationRandom.FRand();
    ApplyVariant();
    if (Variant && ZombieAudio) ZombieAudio->ConfigureVoiceVariation(Variant->VoiceVariation);
    if (const USkeletalMesh* Asset=GetMesh()->GetSkeletalMeshAsset())
    {
        // Register components without named sockets in the constructor: the
        // runtime-loaded skeleton does not exist until this point in BeginPlay.
        const TArray<USceneComponent*> Regions={HeadRegion,ArmLeftRegion,UpperArmLeftRegion,ArmRightRegion,UpperArmRightRegion,
            LegLeftRegion,UpperLegLeftRegion,LegRightRegion,UpperLegRightRegion,BodyRegion};
        const FName Bones[]={TEXT("head"),TEXT("lowerarm_r"),TEXT("upperarm_r"),TEXT("lowerarm_l"),TEXT("upperarm_l"),
            TEXT("calf_r"),TEXT("thigh_r"),TEXT("calf_l"),TEXT("thigh_l"),TEXT("spine_01")};
        for (int32 I=0;I<Regions.Num();++I) Regions[I]->AttachToComponent(GetMesh(),FAttachmentTransformRules::KeepRelativeTransform,Bones[I]);
        const FReferenceSkeleton& Skeleton=Asset->GetRefSkeleton();
        auto Bind=[&Skeleton](FName Name)
        {
            FTransform Result=FTransform::Identity;
            for (int32 Bone=Skeleton.FindBoneIndex(Name);Bone!=INDEX_NONE;Bone=Skeleton.GetParentIndex(Bone)) Result*=Skeleton.GetRefBonePose()[Bone];
            return Result;
        };
        HeadRegion->SetRelativeLocation(Bind(TEXT("head")).InverseTransformPosition(FVector(-.2,0,168.7)));
        HeadRegion->SetRelativeRotation(Bind(TEXT("head")).GetRotation().Inverse());
        const FTransform TorsoBind=Bind(TEXT("spine_01"));
        BodyRegion->SetRelativeLocation(TorsoBind.InverseTransformPosition(FVector(0,0,122)));
        BodyRegion->SetRelativeRotation(TorsoBind.GetRotation().Inverse());
        // Narrow capsules cover the entire bone segments and shared knee. Their
        // reference widths leave separation between anatomical sides (8+8<18cm).
        const TArray<UCapsuleComponent*> LegQueries={UpperLegLeftRegion,LegLeftRegion,UpperLegRightRegion,LegRightRegion};
        const FName DistalBones[]={TEXT("calf_r"),TEXT("foot_r"),TEXT("calf_l"),TEXT("foot_l")};
        FVector EndsA[4],EndsB[4];
        for (int32 I=0;I<LegQueries.Num();++I)
        {
            auto* Query=LegQueries[I]; const FTransform ParentBind=Bind(Query->GetAttachSocketName());
            EndsA[I]=ParentBind.GetLocation(); EndsB[I]=Bind(DistalBones[I]).GetLocation();
            const FVector End=ParentBind.InverseTransformPosition(EndsB[I]);
            Query->SetCapsuleHalfHeight(float(End.Size())*.5f+Query->GetUnscaledCapsuleRadius());
            Query->SetRelativeLocation(End*.5);
            Query->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector,End.GetSafeNormal()));
        }
        ReferenceLegQuerySeparation=BIG_NUMBER;
        for (int32 L=0;L<2;++L) for (int32 R=2;R<4;++R)
        {
            FVector A,B; FMath::SegmentDistToSegmentSafe(EndsA[L],EndsB[L],EndsA[R],EndsB[R],A,B);
            ReferenceLegQuerySeparation=FMath::Min(ReferenceLegQuerySeparation,float(FVector::Dist(A,B))-LegQueries[L]->GetUnscaledCapsuleRadius()-LegQueries[R]->GetUnscaledCapsuleRadius());
        }
        const TArray<UCapsuleComponent*> ArmQueries={UpperArmLeftRegion,ArmLeftRegion,UpperArmRightRegion,ArmRightRegion};
        const FName ArmEnds[]={TEXT("lowerarm_r"),TEXT("hand_r"),TEXT("lowerarm_l"),TEXT("hand_l")};
        for (int32 I=0;I<ArmQueries.Num();++I)
        {
            auto* Region=ArmQueries[I]; const FTransform Parent=Bind(Region->GetAttachSocketName());
            const FVector End=Parent.InverseTransformPosition(Bind(ArmEnds[I]).GetLocation());
            Region->SetCapsuleHalfHeight(float(End.Size())*.5f+Region->GetUnscaledCapsuleRadius());
            Region->SetRelativeLocation(End*.5f);
            Region->SetRelativeRotation(FQuat::FindBetweenNormals(FVector::UpVector,End.GetSafeNormal()));
        }
    }
    GetMesh()->SetAnimInstanceClass(UONEInfectedAnimInstance::StaticClass());
    if (auto* Anim=Cast<UONEInfectedAnimInstance>(GetMesh()->GetAnimInstance()))
    {
        if (Variant) Anim->ConfigureClips(Variant->Clips);
        Anim->OnFootContact.AddUObject(this,&AONEZombie::OnFootContact);
        Anim->OnAttackEffort.AddUObject(ZombieAudio.Get(),&UONEZombieAudioComponent::NotifyAttack);
    }
    GetMesh()->AddTickPrerequisiteActor(this);
    GetMesh()->OnComponentHit.AddDynamic(this,&AONEZombie::OnPhysicalContact);
    GetCapsuleComponent()->OnComponentHit.AddDynamic(this,&AONEZombie::OnCapsuleContact);
    GetMesh()->TickAnimation(0.f,false); GetMesh()->RefreshBoneTransforms();
    StartLivingPhysicalResponse();
    Target=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    StateStart=GetWorld()->GetTimeSeconds();
    NextPath=StateStart+PresentationRandom.FRandRange(0,.35f);
}
TArray<FString> AONEZombie::GetProductionVariantPaths()
{
    // Presentation selection is independent of combat/spread/reward streams.
    // Missing definitions never silently load a historical mannequin.
    static const TCHAR* Names[]={TEXT("Maintenance"),TEXT("Laboratory"),TEXT("FacilityStaff")};
    TArray<FString> Paths;
    for (const TCHAR* Name:Names)
        Paths.Add(FString::Printf(TEXT("/Game/ONE/Characters/Candidate07/DA_Infected_%s.DA_Infected_%s"),Name,Name));
    return Paths;
}
bool AONEZombie::ApplyVariant()
{
    if (!VariantOverride.IsNull()) Variant=VariantOverride.LoadSynchronous();
    else
    {
        const TArray<FString> Paths=GetProductionVariantPaths();
        const FString& Path=Paths[PresentationRandom.RandRange(0,Paths.Num()-1)];
        Variant=LoadObject<UONEInfectedVariant>(nullptr,*Path);
    }
    if (!Variant)
    { UE_LOG(LogTemp,Error,TEXT("ONE07_VARIANT_MISSING production infected has no C07 definition")); return false; }
    USkeletalMesh* Core=Variant->Core.LoadSynchronous();
    if (!Core) { UE_LOG(LogTemp,Error,TEXT("ONE07_VARIANT_MISSING core id=%s"),*Variant->VariantId.ToString()); return false; }
    EnsureModularPoseBones(Core); GetMesh()->SetSkeletalMesh(Core);
    GetMesh()->SetPhysicsAsset(Variant->BodyPhysics.LoadSynchronous());
    const TArray<USkeletalMeshComponent*> Parts={HeadMesh,ArmLeftMesh,ArmRightMesh,LegLeftMesh};
    const TSoftObjectPtr<USkeletalMesh> Meshes[]={Variant->Head,Variant->ArmLeft,Variant->ArmRight,Variant->LegLeft};
    const TSoftObjectPtr<UPhysicsAsset> Physics[]={Variant->HeadPhysics,Variant->ArmLeftPhysics,Variant->ArmRightPhysics,Variant->LegLeftPhysics};
    bool Complete=GetMesh()->GetPhysicsAsset()!=nullptr;
    for (int32 I=0;I<Parts.Num();++I)
    {
        Parts[I]->SetSkeletalMesh(Meshes[I].LoadSynchronous());
        Parts[I]->SetPhysicsAsset(Physics[I].LoadSynchronous());
        Parts[I]->SetLeaderPoseComponent(GetMesh());
        Complete&=Parts[I]->GetSkeletalMeshAsset()!=nullptr && Parts[I]->GetPhysicsAsset()!=nullptr;
    }
    UE_LOG(LogTemp,Display,TEXT("ONE07_VARIANT id=%s complete=%d"),*Variant->VariantId.ToString(),Complete);
    return Complete;
}
FName AONEZombie::GetVariantId() const { return Variant ? Variant->VariantId : NAME_None; }
bool AONEZombie::IsDead() const { return State==EONEZombieState::Dead; }
float AONEZombie::GetHealth() const { return Health->Health; }
float AONEZombie::GetStateElapsed() const { return GetWorld()->GetTimeSeconds()-StateStart; }
void AONEZombie::ChangeState(EONEZombieState Next)
{
    State=Next; StateStart=GetWorld()->GetTimeSeconds(); bContactDelivered=false;
    if (auto* Anim=Cast<UONEInfectedAnimInstance>(GetMesh()->GetAnimInstance())) Anim->ResetMotionEvents();
    GetCharacterMovement()->bOrientRotationToMovement=Next==EONEZombieState::Pursue;
    if (Next!=EONEZombieState::Dead) GetCharacterMovement()->SetAvoidanceEnabled(Next==EONEZombieState::Pursue);
    if (ZombieAudio) ZombieAudio->SetPursuing(Next==EONEZombieState::Pursue && IsValid(Target) && !Target->IsDead());
}
float AONEZombie::GetMinorReactionAge() const { return GetWorld() ? GetWorld()->GetTimeSeconds()-MinorReactionStart : BIG_NUMBER; }
const FONEInfectedAttackDefinition* AONEZombie::CurrentAttackDefinition() const
{ return AttackDefinitions.IsValidIndex(AttackDefinitionIndex)?&AttackDefinitions[AttackDefinitionIndex]:nullptr; }
float AONEZombie::GetCurrentAttackContactTime() const
{ const auto* D=CurrentAttackDefinition(); return (D?D->ContactTime:.48f)*(AttackContactTime/.48f); }
float AONEZombie::GetCurrentAttackDuration() const
{ const auto* D=CurrentAttackDefinition(); return (D?D->Duration:1.f)*AttackDuration; }
bool AONEZombie::RequiredAttackArmsPresent() const { return ONE05AttackMotion::ArmsAvailable(RequiredAttackArms,HasLeftArm(),HasRightArm()); }
FName AONEZombie::GetAttackClipKey() const
{
    const auto* D=CurrentAttackDefinition(); return D?D->AnimationKey:NAME_None;
}
bool AONEZombie::TryStartAttack(AONEPlayer* Victim,int32 PreferredFamily)
{
    if (!IsValid(Victim) || Victim->IsDead() || IsDead() || State!=EONEZombieState::Pursue ||
        GetWorld()->GetTimeSeconds()<NextAttack || (!HasLeftArm() && !HasRightArm()) ||
        FVector::DistSquared2D(GetActorLocation(),Victim->GetActorLocation())>FMath::Square(AttackRange)) return false;
    Target=Victim;
    const FVector ToVictim=(Victim->GetActorLocation()-GetActorLocation()).GetSafeNormal2D(SMALL_NUMBER,GetActorForwardVector());
    FONEInfectedAttackContext Context;
    Context.AvailableLimbs=(HasLeftArm()?1:0)|(HasRightArm()?2:0);
    Context.DistanceCm=FVector::Dist2D(GetActorLocation(),Victim->GetActorLocation());
    Context.BearingDegrees=FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw,ToVictim.Rotation().Yaw);
    Context.ForwardSpeed=FMath::Max(0.f,float(FVector::DotProduct(GetVelocity(),ToVictim)));
    Context.bGrounded=GetCharacterMovement()->IsMovingOnGround();
    Context.PreviousPerformance=PreviousAttack; Context.PreferredFamily=PreferredFamily;
    AttackDefinitionIndex=ONEInfectedAttacks::Select(AttackDefinitions,Context,AttackRandom);
    if (AttackDefinitionIndex==INDEX_NONE) return false;
    const auto& Definition=AttackDefinitions[AttackDefinitionIndex];
    AttackFamily=int32(Definition.Family); RequiredAttackArms=uint8(Definition.RequiredLimbs);
    PreviousAttack=Definition.Id;
    ++AttackSerial;
    AttackHeading=ToVictim;
    AttackStartPosition=GetActorLocation();
    AttackEntrySpeed=FMath::Clamp(Context.ForwardSpeed,0.f,PursuitSpeed);
    AttackRecoveryAlpha=0; CommittedAttackTravel=0;
    // Release path following, then let CharacterMovement sweep the short authored step.
    // No target-distance warp and no rotation updates after this commitment.
    if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
    GetCharacterMovement()->ConsumeInputVector();
    GetCharacterMovement()->Velocity=AttackHeading*AttackEntrySpeed;
    SetActorRotation(AttackHeading.Rotation());
    ChangeState(EONEZombieState::Attack);
    return true;
}
void AONEZombie::TickAttack(float Dt)
{
    const float Age=GetStateElapsed();
    const auto* Definition=CurrentAttackDefinition();
    if (!Definition) { ChangeState(EONEZombieState::Pursue); NextPath=0; return; }
    const auto& D=*Definition;
    const float MotionAge=Age/FMath::Max(AttackDuration,.01f);
    const bool Arms=RequiredAttackArmsPresent();
    if (MotionAge<D.SteeringRelease) SetActorRotation(AttackHeading.Rotation());
    if (MotionAge<=D.RecoveryLocomotionStart)
        CommittedAttackTravel=FMath::Clamp(float(FVector::DotProduct(GetActorLocation()-AttackStartPosition,AttackHeading)),0.f,D.StepDistanceCm);
    AttackRecoveryAlpha=ONEInfectedAttacks::RecoveryAlpha(D,MotionAge);
    const float Planned=ONEInfectedAttacks::StepSpeed(D,MotionAge,AttackEntrySpeed);
    const float StepSpeed=Arms ? FMath::Min(Planned,FMath::Max(0.f,D.StepDistanceCm-CommittedAttackTravel)/FMath::Max(Dt,.001f)) : 0.f;
    const bool CanRecover=Arms && bContactDelivered && AttackRecoveryAlpha>0 && IsValid(Target) && !Target->IsDead();
    const float TargetDistance=CanRecover?FVector::Dist2D(GetActorLocation(),Target->GetActorLocation()):0.f;
    const float RecoverySpeed=TargetDistance>AttackRange*.75f ? PursuitSpeed*AttackRecoveryAlpha : 0.f;
    const float Speed=FMath::Max(StepSpeed,RecoverySpeed);
    const FVector Heading=CanRecover?(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal2D():AttackHeading;
    auto* Movement=GetCharacterMovement();
    Movement->ConsumeInputVector();
    Movement->MaxWalkSpeed=Speed;
    Movement->Velocity=FVector(Heading.X*Speed,Heading.Y*Speed,Movement->Velocity.Z);
    if (CanRecover && MotionAge>=D.SteeringRelease)
        SetActorRotation(FMath::RInterpConstantTo(GetActorRotation(),Heading.Rotation(),Dt,300.f));
    // Active input avoids applying walking brake deceleration to the authored
    // speed on the movement tick. The capsule still performs its normal sweep.
    if (Speed>0.f) Movement->AddInputVector(Heading);
    if (!bContactDelivered && (!Arms || Age>=GetCurrentAttackContactTime()))
    {
        bContactDelivered=true;
        if (Arms && IsValid(Target) && !Target->IsDead())
        {
            ++AttackContactAttempts;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(InfectedContact),false,this);
            Params.AddIgnoredActor(Target);
            FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic);
            Objects.AddObjectTypesToQuery(ECC_WorldDynamic); Objects.AddObjectTypesToQuery(ECC_PhysicsBody);
            TArray<FHitResult> Covers;
            GetWorld()->SweepMultiByObjectType(Covers,GetActorLocation()+FVector(0,0,20),
                Target->GetActorLocation()+FVector(0,0,20),FQuat::Identity,Objects,FCollisionShape::MakeSphere(7.f),Params);
            bool Blocked=false;
            for (const FHitResult& Cover:Covers)
                if (const auto* Component=Cover.GetComponent())
                    if (!Cast<AONEZombie>(Cover.GetActor()) &&
                        (Component->GetCollisionObjectType()==ECC_WorldStatic || Component->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Block))
                    { Blocked=true; break; }
            if (!Blocked && ONE05AttackMotion::ContactGeometry(GetActorLocation(),AttackHeading,Target->GetActorLocation(),AttackRange+8.f))
            { ++AttackDamageDispatches; Target->ReceiveAttack(AttackDamage,GetActorLocation()); }
        }
    }
    if (Age>=GetCurrentAttackDuration())
    { ChangeState(EONEZombieState::Pursue); Movement->MaxWalkSpeed=PursuitSpeed; NextAttack=GetWorld()->GetTimeSeconds()+.3f; NextPath=0; }
}
void AONEZombie::StopPursuit()
{
    if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
    GetCharacterMovement()->StopMovementImmediately();
}
void AONEZombie::Tick(float Dt)
{
    Super::Tick(Dt);
    if (IsDead()) return;
    TickLivingPhysicality(Dt);
    if (IsLivingFallen() || IsGettingUp()) return;
    if (!IsValid(Target)) Target=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Target || Target->IsDead()) { StopPursuit(); if (ZombieAudio) ZombieAudio->SetPursuing(false); return; }
    const float Now=GetWorld()->GetTimeSeconds();
    const float Distance=FVector::Dist2D(GetActorLocation(),Target->GetActorLocation());
    if (State==EONEZombieState::Stumble)
    {
        if (GetStateElapsed()>=.58f) { ChangeState(EONEZombieState::Pursue); NextPath=0; }
        else
        {
            auto* Movement=GetCharacterMovement();
            Movement->MaxWalkSpeed=ShambleSpeed*.6f;
            Movement->AddInputVector(GetActorForwardVector()*.5f);
            return;
        }
    }
    if (State==EONEZombieState::Hit)
    {
        if (GetStateElapsed()>=(bHeavyReaction ? .52f : .4f)) ChangeState(EONEZombieState::Pursue);
        else return;
    }
    if (State==EONEZombieState::Attack) { TickAttack(Dt); return; }
    if (Distance<=AttackRange && Now>=NextAttack && TryStartAttack(Target)) return;
    if (ZombieAudio) ZombieAudio->SetPursuing(true);
    GetCharacterMovement()->MaxWalkSpeed=Distance>550 ? ShambleSpeed : PursuitSpeed;
    if (Now>=NextPath)
    {
        NextPath=Now+.4f+PresentationRandom.FRandRange(0,.1f);
        if (auto* AI=Cast<AAIController>(GetController())) AI->MoveToActor(Target,AttackRange*.25f,true,true,true,nullptr,true);
    }
}
namespace
{
    FName RegionRoot(EONEHitRegion Region)
    {
        switch (Region)
        {
            case EONEHitRegion::Head: return TEXT("head");
            case EONEHitRegion::ArmLeft: return TEXT("upperarm_r");
            case EONEHitRegion::ArmRight: return TEXT("upperarm_l");
            case EONEHitRegion::LegLeft: return TEXT("thigh_r");
            case EONEHitRegion::LegRight: return TEXT("thigh_l");
            default: return TEXT("spine_01");
        }
    }
    bool BoneMatches(EONEHitRegion Region,FName Bone)
    {
        switch (Region)
        {
            case EONEHitRegion::Head: return Bone==TEXT("head");
            case EONEHitRegion::ArmLeft: return Bone==TEXT("upperarm_r") || Bone==TEXT("lowerarm_r") || Bone==TEXT("hand_r");
            case EONEHitRegion::ArmRight: return Bone==TEXT("upperarm_l") || Bone==TEXT("lowerarm_l") || Bone==TEXT("hand_l");
            case EONEHitRegion::LegLeft: return Bone==TEXT("thigh_r") || Bone==TEXT("calf_r") || Bone==TEXT("foot_r") || Bone==TEXT("toe_r");
            case EONEHitRegion::LegRight: return Bone==TEXT("thigh_l") || Bone==TEXT("calf_l") || Bone==TEXT("foot_l") || Bone==TEXT("toe_l");
            case EONEHitRegion::Body: return Bone==TEXT("pelvis") || Bone==TEXT("spine_01") || Bone==TEXT("spine_02") || Bone==TEXT("neck");
            default: return false;
        }
    }
}
bool AONEZombie::IsRegionPresent(EONEHitRegion Region) const
{
    switch (Region)
    {
        case EONEHitRegion::Head: return HasHead();
        case EONEHitRegion::ArmLeft: return HasLeftArm();
        case EONEHitRegion::ArmRight: return HasRightArm();
        case EONEHitRegion::LegLeft: return HasLeftLeg();
        case EONEHitRegion::Body: case EONEHitRegion::LegRight: return true;
        default: return false;
    }
}
FName AONEZombie::ResolveRegionBone(EONEHitRegion Region,FName Requested) const
{
    return BoneMatches(Region,Requested) ? Requested : RegionRoot(Region);
}
void AONEZombie::ReceiveBullet(const FHitResult& Hit,const FVector& Direction,float Damage)
{
    const EONEHitRegion Region=GetHitRegion(Hit);
    if (!FONEWeaponDamagePacket::IsValidRegion(Region)) return;
    FONEWeaponDamagePacket Packet;
    const float Trauma=Region==EONEHitRegion::Head ? Damage*2.f : Damage;
    Packet.Get(Region).AddPellet(Damage,Trauma,Hit.ImpactPoint,Direction,Hit.ImpactNormal,Hit.BoneName);
    Packet.Finalize(); ReceiveWeaponDamage(Packet);
}
EONEHitRegion AONEZombie::GetHitRegion(const FHitResult& Hit) const
{
    const auto* Component=Hit.GetComponent();
    EONEHitRegion Region=EONEHitRegion::Body;
    // An explicit query component is authoritative. A bone cannot relabel it.
    if (Component==HeadRegion) Region=EONEHitRegion::Head;
    else if (Component==ArmLeftRegion || Component==UpperArmLeftRegion) Region=EONEHitRegion::ArmLeft;
    else if (Component==ArmRightRegion || Component==UpperArmRightRegion) Region=EONEHitRegion::ArmRight;
    else if (Component==LegLeftRegion || Component==UpperLegLeftRegion) Region=EONEHitRegion::LegLeft;
    else if (Component==LegRightRegion || Component==UpperLegRightRegion) Region=EONEHitRegion::LegRight;
    else if (Component!=BodyRegion && !Hit.BoneName.IsNone())
        for (int32 I=1;I<FONEWeaponDamagePacket::RegionCount;++I)
            if (BoneMatches(EONEHitRegion(I),Hit.BoneName)) { Region=EONEHitRegion(I); break; }
    return IsRegionPresent(Region) ? Region : EONEHitRegion::Invalid;
}
bool AONEZombie::ReceiveWeaponDamage(const FONEWeaponDamagePacket& Packet)
{ return ReceiveWeaponDamageOutcome(Packet)!=EONEWeaponHitOutcome::Rejected; }
EONEWeaponHitOutcome AONEZombie::ReceiveWeaponDamageOutcome(const FONEWeaponDamagePacket& Packet)
{
    CSV_SCOPED_TIMING_STAT(ONEPhysicality,RegionalDamage);
    if (Packet.ShotId && RecentShotIds.Contains(Packet.ShotId)) return EONEWeaponHitOutcome::Rejected;
    FONEWeaponRegionDamage Accepted[FONEWeaponDamagePacket::RegionCount];
    bool SeverRegion[FONEWeaponDamagePacket::RegionCount]={};
    float Total=0,HealthDamage=0,HeadHealthDamage=0,Largest=0;
    int32 Strongest=0;
    // Read the complete pre-discharge presence mask before changing any region.
    for (int32 I=0;I<FONEWeaponDamagePacket::RegionCount;++I)
    {
        const auto Region=EONEHitRegion(I); const auto& In=Packet.Regions[I];
        if (!IsRegionPresent(Region) || !FMath::IsFinite(In.Damage) || !FMath::IsFinite(In.Trauma) || In.Damage<=0 ||
            In.Position.ContainsNaN() || In.Direction.ContainsNaN() || In.Normal.ContainsNaN()) continue;
        auto& Out=Accepted[I]; Out=In;
        Out.Damage=FMath::Min(In.Damage,10000.f); Out.Trauma=FMath::Clamp(In.Trauma,0.f,10000.f);
        Out.Direction=In.Direction.GetSafeNormal(SMALL_NUMBER,GetActorForwardVector());
        Out.Normal=In.Normal.GetSafeNormal(SMALL_NUMBER,-Out.Direction);
        Out.Bone=ResolveRegionBone(Region,In.Bone);
        Total+=Out.Damage;
        const float Multiplier=Region==EONEHitRegion::Head ? FMath::Clamp(HeadDamageMultiplier,1.f,3.f) :
            (Region==EONEHitRegion::ArmLeft || Region==EONEHitRegion::ArmRight) ? .4f : 1.f;
        HealthDamage+=Out.Damage*Multiplier;
        if (Region==EONEHitRegion::Head) HeadHealthDamage+=Out.Damage*Multiplier;
        if (Out.Damage>Largest) { Largest=Out.Damage; Strongest=I; }
    }
    if (Total<=0) return EONEWeaponHitOutcome::Rejected;
    if (Packet.ShotId)
    {
        if (RecentShotIds.Num()>=32) RecentShotIds.RemoveAt(0);
        RecentShotIds.Add(Packet.ShotId);
    }
    const bool WasDead=IsDead();
    const bool HeadDominant=HeadHealthDamage>HealthDamage*.5f;
    if (WasDead) ++CorpseTransactions; else ++DamageTransactions;
    for (int32 I=0;I<FONEWeaponDamagePacket::RegionCount;++I)
    {
        if (Accepted[I].Damage<=0) continue;
        RegionalTrauma[I]=FMath::Min(RegionalTrauma[I]+Accepted[I].Trauma,100000.f);
        const EONEHitRegion Region=EONEHitRegion(I);
        const float Threshold=Region==EONEHitRegion::Head ? HeadSeverThreshold :
            Region==EONEHitRegion::ArmLeft || Region==EONEHitRegion::ArmRight ? ArmSeverThreshold :
            Region==EONEHitRegion::LegLeft ? LegSeverThreshold : BIG_NUMBER;
        SeverRegion[I]=RegionalTrauma[I]>=Threshold;
        // Head trauma cannot bypass health. Removal is presentation of lethal
        // head damage (or a head-dominant Insta-Kill), and remains possible on a corpse.
        if (Region==EONEHitRegion::Head)
            SeverRegion[I]&=WasDead || HeadHealthDamage>=Health->Health || (Packet.bForceLethal && HeadDominant);
    }
    // One spray per victim transaction; each anatomical wound has its own anchor.
    const auto& Main=Accepted[Strongest];
    if (bRagdollActive) ONEPhysicsRuntime::ResetRest(GetMesh(),RestState,true);
    auto* Blood=GetWorld() ? GetWorld()->GetSubsystem<UONEBloodSubsystem>() : nullptr;
    if (Blood && GetMesh()->GetSkeletalMeshAsset())
        Blood->Impact(Main.Position,Main.Direction,SeverRegion[1] || SeverRegion[2] || SeverRegion[3] || SeverRegion[4]);
    for (int32 I=0;I<FONEWeaponDamagePacket::RegionCount;++I)
    {
        auto& RegionDamage=Accepted[I]; if (RegionDamage.Damage<=0) continue;
        const auto Region=EONEHitRegion(I);
        if (SeverRegion[I]) Sever(Region,RegionDamage.Direction);
        else if (Blood && GetMesh()->GetSkeletalMeshAsset())
        {
            const bool HeavyBleed=Region==EONEHitRegion::Body && RegionDamage.Damage>=40.f;
            const float Volume=HeavyBleed ? FMath::Clamp(RegionDamage.Damage*.5f,20.f,32.f) : FMath::Clamp(RegionDamage.Damage*.16f,1.2f,16.f);
            Blood->AddWound(GetMesh(),Region,RegionDamage.Bone,RegionDamage.Position,RegionDamage.Normal,Volume,HeavyBleed);
        }
        if (IsRegionPresent(Region) && bRagdollActive)
            GetMesh()->AddImpulseAtLocation(RegionDamage.Direction*FMath::Clamp(RegionDamage.Damage*4.f,100.f,500.f),
                GetMesh()->GetSocketLocation(RegionDamage.Bone),RegionDamage.Bone);
    }
    if (WasDead) return EONEWeaponHitOutcome::CorpseHit;
    const bool FatalLoss=!HasHead() || !HasLeftLeg() || (!HasLeftArm() && !HasRightArm());
    Health->ApplyDamage(FatalLoss || Packet.bForceLethal ? Health->Health : HealthDamage);
    if (Health->IsDead())
    {
        // Classification uses this lethal discharge's effective health damage;
        // old head trauma and one incidental head pellet cannot claim the bonus.
        bLastKillHeadshot=HeadDominant;
        Die(Main.Direction,EONEHitRegion(Strongest),Main.Bone,Main.Position,FMath::Clamp(Total*4.f,150.f,550.f));
        return EONEWeaponHitOutcome::NewKill;
    }
    const float Now=GetWorld()->GetTimeSeconds();
    const bool Heavy=FMath::IsFinite(Packet.HeavyStaggerThreshold) && Total>=FMath::Max(0.f,Packet.HeavyStaggerThreshold);
    if (ZombieAudio) ZombieAudio->NotifyHit(Heavy);
    if (Now-MinorReactionStart>=MinorReactionInterval)
    {
        MinorReactionStart=Now; MinorReactionDirection=Main.Direction;
        MinorReactionStrength=FMath::Clamp(Total/32.f,.25f,1.f);
    }
    if (IsLivingFallen())
    { RecoveryQuietSince=-1; NextRecoveryAttempt=FMath::Max(NextRecoveryAttempt,Now+.35f); return EONEWeaponHitOutcome::LiveHit; }
    if (IsGettingUp())
    {
        if (Heavy && Now-LastReaction>=HitReactCooldown)
        { LastReaction=Now; TryLivingFall(Main.Direction*240.f,TEXT("heavy_getup_hit")); }
        return EONEWeaponHitOutcome::LiveHit;
    }
    if (Heavy && Now-LastReaction>=HitReactCooldown)
    {
        if (ContactStrength>.35f && TryLivingFall(Main.Direction*280.f,TEXT("heavy_hit_while_unbalanced")))
            return EONEWeaponHitOutcome::LiveHit;
        bHeavyReaction=true; LastReaction=Now; StopPursuit(); ChangeState(EONEZombieState::Hit);
        NextAttack=FMath::Max(NextAttack,Now+.52f);
    }
    return EONEWeaponHitOutcome::LiveHit;
}
void AONEZombie::GetCutWorld(EONEHitRegion Region,FVector& Point,FVector& Normal) const
{
    const FName Root=RegionRoot(Region);
    // Repository-authored cut manifest component coordinates; convert through
    // imported reference transforms instead of assuming Blender's local axes.
    FVector RefPoint(0,0,157.8),RefNormal=FVector::UpVector;
    if (Region==EONEHitRegion::ArmLeft || Region==EONEHitRegion::ArmRight)
    {
        const float Side=Region==EONEHitRegion::ArmLeft ? -1.f : 1.f;
        RefPoint=FVector(.6531973,Side*19.3063946,140.4276123);
        RefNormal=FVector(.13608277,Side*.27216554,-.95257938);
    }
    else if (Region==EONEHitRegion::LegLeft)
    { RefPoint=FVector(.456,-9,79.040001); RefNormal=FVector(.028559776,0,-.999592125); }
    Point=GetActorLocation(); Normal=GetActorUpVector();
    if (const auto* Asset=GetMesh()->GetSkeletalMeshAsset())
    {
        const auto& Ref=Asset->GetRefSkeleton(); FTransform Bind=FTransform::Identity;
        for (int32 I=Ref.FindBoneIndex(Root);I!=INDEX_NONE;I=Ref.GetParentIndex(I)) Bind*=Ref.GetRefBonePose()[I];
        const FTransform Current=GetMesh()->GetSocketTransform(Root);
        Point=Current.TransformPosition(Bind.InverseTransformPosition(RefPoint));
        Normal=Current.TransformVectorNoScale(Bind.InverseTransformVectorNoScale(RefNormal)).GetSafeNormal();
    }
}
void AONEZombie::Sever(EONEHitRegion Region,const FVector& Direction)
{
    if (!IsRegionPresent(Region)) return;
    if (bRagdollActive) ONEPhysicsRuntime::ResetRest(GetMesh(),RestState,true);
    if (bRagdollActive)
    {
        if (auto* Anim=Cast<UONEInfectedAnimInstance>(GetMesh()->GetAnimInstance()))
            GetMesh()->SnapshotPose(Anim->CapturedDeathPose);
        else if (auto* SnapshotAnim=Cast<UONESnapshotAnimInstance>(GetMesh()->GetAnimInstance()))
            GetMesh()->SnapshotPose(SnapshotAnim->CapturedPose);
    }
    USkeletalMeshComponent* Part=nullptr;
    switch (Region)
    {
        case EONEHitRegion::Head: Part=HeadMesh; bHeadSevered=true; HeadRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); break;
        case EONEHitRegion::ArmLeft:
            Part=ArmLeftMesh; bLeftArmSevered=true;
            ArmLeftRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); UpperArmLeftRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); break;
        case EONEHitRegion::ArmRight:
            Part=ArmRightMesh; bRightArmSevered=true;
            ArmRightRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); UpperArmRightRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); break;
        case EONEHitRegion::LegLeft:
            Part=LegLeftMesh; bLeftLegSevered=true;
            LegLeftRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); UpperLegLeftRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); break;
        default: return;
    }
    ++SeverCount;
    const FName Bone=RegionRoot(Region); FVector Cut,Normal; GetCutWorld(Region,Cut,Normal);
    if (Part && Part->GetSkeletalMeshAsset() && GetMesh()->GetSkeletalMeshAsset())
        if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>())
        {
            AONEGorePiece* Piece=Blood->Detach(Part,GetMesh(),Bone,Direction);
            Blood->AddWound(GetMesh(),Region,Bone,Cut,Normal,24.f,true);
            if (Piece) Blood->AddWound(Piece->GetPieceMesh(),Region,Bone,Cut,-Normal,12.f,true);
        }
    if (Part) Part->SetVisibility(false);
    if (bRagdollActive && Region==EONEHitRegion::LegLeft)
        StumpFitError=ONEPhysicsRuntime::ConfigureLeftStump(GetMesh(),true);
    // Preserve cut-root transforms/weights for the remaining capped stump.
    // Kinematic missing chains must also be removed, not merely made non-simulated.
    if (GetMesh()->GetPhysicsAsset()) GetMesh()->TermBodiesBelow(Bone);
}
void AONEZombie::Die(const FVector& Direction,EONEHitRegion ImpactRegion,FName ImpactBone,const FVector& ImpactPosition,float Impulse)
{
    if (IsDead()) return;
    LastDeathImpulse=Impulse;
    if (ZombieAudio) ZombieAudio->NotifyDeath();
    const FVector Inherited=GetVelocity();
    const bool WasFullPhysics=bRagdollActive;
    CaptureCurrentPose();
    StopLivingPhysicalResponse();
    StopPursuit(); ChangeState(EONEZombieState::Dead);
    GetCharacterMovement()->DisableMovement(); GetCharacterMovement()->SetAvoidanceEnabled(false);
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (GetMesh()->GetSkeletalMeshAsset() && GetMesh()->GetPhysicsAsset())
    {
        if (!WasFullPhysics)
        {
            const auto Result=ONEPhysicsRuntime::Start(GetMesh(),Inherited,MissingPhysicsRoots());
            RagdollPositionError=Result.PositionErrorCm; RagdollAngleError=Result.AngleErrorDegrees;
            StumpFitError=Result.StumpFitErrorCm; bRagdollActive=Result.SimulatedBodies>0;
        }
        // Lethal damage to a fallen body keeps its current transforms and
        // velocities; it does not reinitialize a standing physics pose.
        ONEPhysicsRuntime::ResetRest(GetMesh(),RestState,WasFullPhysics);
        // If the impacted body was severed, distribute the bounded impact to the
        // torso; do not address a terminated rigid body with a stale region bone.
        const FName Body=GetMesh()->IsSimulatingPhysics(ImpactBone) ? ImpactBone : FName(TEXT("spine_01"));
        if (bRagdollActive) GetMesh()->AddImpulseAtLocation(Direction*Impulse,GetMesh()->GetSocketLocation(Body),Body);
    }
    if (bRagdollActive)
    {
        // Timers execute after physics without moving the character/animation
        // tick dependencies to a different group. No catch-up loop after hitches.
        FTimerManagerTimerParameters Parameters; Parameters.bLoop=true; Parameters.bMaxOncePerFrame=true;
        GetWorld()->GetTimerManager().SetTimer(RestTimer,this,&AONEZombie::ObserveRest,.1f,Parameters);
    }
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>())
    {
        // Promote only an actual fatal torso impact. Head/limb loss already
        // owns cut-anchored sources and must not invent another chest injury.
        if (ImpactRegion==EONEHitRegion::Body && GetMesh()->GetSkeletalMeshAsset())
            Blood->AddWound(GetMesh(),EONEHitRegion::Body,ResolveRegionBone(EONEHitRegion::Body,ImpactBone),ImpactPosition,-Direction,8.f,true);
        Blood->RegisterCorpse(this);
    }
    SetLifeSpan(28.f);
    if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->NotifyZombieKilled(this,100);
}
int32 AONEZombie::GetActivePhysicsBodyCount() const { return ONEPhysicsRuntime::Count(GetMesh()); }
void AONEZombie::ObserveRest() { if (bRagdollActive) ONEPhysicsRuntime::UpdateRest(GetMesh(),RestState); }
int32 AONEZombie::GetAwakePhysicsBodyCount() const { return ONEPhysicsRuntime::Count(GetMesh(),true); }
int32 AONEZombie::GetRegionPhysicsBodyCount(EONEHitRegion Region) const
{
    return FONEWeaponDamagePacket::IsValidRegion(Region) ? ONEPhysicsRuntime::ExistingChainBodies(GetMesh(),RegionRoot(Region)) : 0;
}
void AONEZombie::EndPlay(const EEndPlayReason::Type Reason)
{
    StopLivingPhysicalResponse();
    if (ZombieAudio) ZombieAudio->Shutdown();
    if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(RestTimer);
    if (GetWorld()) if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->RemoveSourcesForActor(this);
    Super::EndPlay(Reason);
}

float AONEZombie::GetLegQueryCoverageErrorCm(EONEHitRegion Region) const
{
    if ((Region!=EONEHitRegion::LegLeft && Region!=EONEHitRegion::LegRight) || !IsRegionPresent(Region) || !GetMesh()->GetSkeletalMeshAsset()) return BIG_NUMBER;
    const bool Left=Region==EONEHitRegion::LegLeft;
    const UCapsuleComponent* Queries[]={Left?UpperLegLeftRegion.Get():UpperLegRightRegion.Get(),Left?LegLeftRegion.Get():LegRightRegion.Get()};
    const FName Bones[]={Left?FName(TEXT("thigh_r")):FName(TEXT("thigh_l")),Left?FName(TEXT("calf_r")):FName(TEXT("calf_l")),Left?FName(TEXT("foot_r")):FName(TEXT("foot_l"))};
    float Maximum=0.f;
    for (int32 Segment=0;Segment<2;++Segment)
    {
        const FVector Start=GetMesh()->GetSocketLocation(Bones[Segment]),End=GetMesh()->GetSocketLocation(Bones[Segment+1]);
        for (float T:{0.f,.25f,.5f,.75f,1.f})
        {
            const FVector Point=FMath::Lerp(Start,End,T); float Best=BIG_NUMBER;
            for (const UCapsuleComponent* Query:Queries)
            {
                const FVector Center=Query->GetComponentLocation(),Axis=Query->GetUpVector();
                const float Radius=Query->GetScaledCapsuleRadius(),Half=Query->GetScaledCapsuleHalfHeight()-Radius;
                Best=FMath::Min(Best,float(FMath::PointDistToSegment(Point,Center-Axis*Half,Center+Axis*Half))-Radius);
            }
            Maximum=FMath::Max(Maximum,FMath::Max(0.f,Best));
        }
    }
    return Maximum;
}
