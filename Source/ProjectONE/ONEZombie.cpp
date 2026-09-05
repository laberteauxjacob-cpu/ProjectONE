#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEPlayer.h"
#include "ONEAnimInstance.h"
#include "ONEBloodSubsystem.h"
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
        const FName Bones[]={TEXT("head"),TEXT("lowerarm_l"),TEXT("hand_l")};
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
        for (FName Bone:TArray<FName>{TEXT("head"),TEXT("hand_l")})
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
    Health->MaxHealth=112;
    GetCapsuleComponent()->InitCapsuleSize(27,88);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
    GetMesh()->SetRelativeLocation(FVector(0,0,-88));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    GetMesh()->SetAnimInstanceClass(UONEAnimInstance::StaticClass());
    GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    HeadMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Head"));
    HeadMesh->SetupAttachment(GetMesh());
    HeadMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArmMesh=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("LeftArm"));
    ArmMesh->SetupAttachment(GetMesh());
    ArmMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadRegion=CreateDefaultSubobject<USphereComponent>(TEXT("HeadRegion"));
    HeadRegion->SetupAttachment(GetMesh(),TEXT("head"));
    HeadRegion->InitSphereRadius(18.f);
    HeadRegion->SetRelativeLocation(FVector(0,0,9));
    ArmRegion=CreateDefaultSubobject<USphereComponent>(TEXT("ArmRegion"));
    ArmRegion->SetupAttachment(GetMesh(),TEXT("lowerarm_l"));
    ArmRegion->InitSphereRadius(13.f);
    UpperArmRegion=CreateDefaultSubobject<USphereComponent>(TEXT("UpperArmRegion"));
    UpperArmRegion->SetupAttachment(GetMesh(),TEXT("upperarm_l"));
    UpperArmRegion->InitSphereRadius(12.f);
    BodyRegion=CreateDefaultSubobject<UCapsuleComponent>(TEXT("BodyRegion"));
    // The torso capsule follows character-space Z; imported bone Y/Z axes differ.
    BodyRegion->SetupAttachment(GetMesh());
    BodyRegion->InitCapsuleSize(24.f,51.f);
    BodyRegion->SetRelativeLocation(FVector(0,0,95));
    for (UPrimitiveComponent* C : TArray<UPrimitiveComponent*>{HeadRegion,ArmRegion,UpperArmRegion,BodyRegion})
    {
        C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
        C->SetCollisionObjectType(ECC_WorldDynamic);
        C->SetCollisionResponseToAllChannels(ECR_Ignore);
        C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        C->SetCanEverAffectNavigation(false);
        C->SetGenerateOverlapEvents(false);
    }
    AIControllerClass=AAIController::StaticClass();
    AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
    bUseControllerRotationYaw=false;
    GetCharacterMovement()->bOrientRotationToMovement=true;
    GetCharacterMovement()->RotationRate=FRotator(0,300,0);
    GetCharacterMovement()->MaxWalkSpeed=ShambleSpeed;
    GetCharacterMovement()->MaxAcceleration=650;
    GetCharacterMovement()->BrakingDecelerationWalking=1200;
    GetCharacterMovement()->bUseRVOAvoidance=true;
    GetCharacterMovement()->AvoidanceConsiderationRadius=250;
}
void AONEZombie::BeginPlay()
{
    Super::BeginPlay();
    if (auto* M=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Infected.SK_Infected")))
    {
        EnsureModularPoseBones(M);
        GetMesh()->SetSkeletalMesh(M);
    }
    if (auto* M=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Infected_Head.SK_Infected_Head"))) HeadMesh->SetSkeletalMesh(M);
    if (auto* M=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Infected_ArmL.SK_Infected_ArmL"))) ArmMesh->SetSkeletalMesh(M);
    if (const USkeletalMesh* Asset=GetMesh()->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& Skeleton=Asset->GetRefSkeleton();
        FTransform Bind=FTransform::Identity;
        for (int32 Bone=Skeleton.FindBoneIndex(TEXT("head"));Bone!=INDEX_NONE;Bone=Skeleton.GetParentIndex(Bone)) Bind*=Skeleton.GetRefBonePose()[Bone];
        HeadRegion->SetRelativeLocation(Bind.GetRotation().Inverse().RotateVector(FVector(0,0,9)));
    }
    GetMesh()->SetAnimInstanceClass(UONEAnimInstance::StaticClass());
    HeadMesh->SetLeaderPoseComponent(GetMesh());
    ArmMesh->SetLeaderPoseComponent(GetMesh());
    Target=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    StateStart=GetWorld()->GetTimeSeconds();
    NextPath=StateStart+FMath::FRandRange(0,.35f);
}
bool AONEZombie::IsDead() const { return State==EONEZombieState::Dead; }
float AONEZombie::GetHealth() const { return Health->Health; }
float AONEZombie::GetStateElapsed() const { return GetWorld()->GetTimeSeconds()-StateStart; }
void AONEZombie::ChangeState(EONEZombieState Next) { State=Next; StateStart=GetWorld()->GetTimeSeconds(); bContactDelivered=false; }
void AONEZombie::StopPursuit()
{
    if (auto* AI=Cast<AAIController>(GetController())) AI->StopMovement();
    GetCharacterMovement()->StopMovementImmediately();
}
void AONEZombie::Tick(float Dt)
{
    Super::Tick(Dt);
    if (IsDead()) return;
    if (!IsValid(Target)) Target=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Target || Target->IsDead()) { StopPursuit(); return; }
    const float Now=GetWorld()->GetTimeSeconds();
    const float Distance=FVector::Dist2D(GetActorLocation(),Target->GetActorLocation());
    if (State==EONEZombieState::Hit)
    {
        if (GetStateElapsed()>=(bHeavyReaction ? .52f : .4f)) ChangeState(EONEZombieState::Pursue);
        else return;
    }
    if (State==EONEZombieState::Attack)
    {
        // One shared state clock drives both the authored clip and its contact event.
        // Leaving Attack (including a hit or death) cancels pending contact immediately.
        if (!bContactDelivered && GetStateElapsed()>=AttackContactTime)
        {
            bContactDelivered=true;
            const FVector ToTarget=(Target->GetActorLocation()-GetActorLocation()).GetSafeNormal2D();
            FHitResult Cover;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(InfectedContact),false,this);
            Params.AddIgnoredActor(Target);
            const bool bCover=GetWorld()->LineTraceSingleByObjectType(Cover,GetActorLocation()+FVector(0,0,20),Target->GetActorLocation()+FVector(0,0,20),FCollisionObjectQueryParams(ECC_WorldStatic),Params);
            if (Distance<=AttackRange+8 && FVector::DotProduct(ToTarget,GetActorForwardVector())>.35f && !bCover) Target->ReceiveAttack(AttackDamage,GetActorLocation());
        }
        if (GetStateElapsed()>=AttackDuration) { ChangeState(EONEZombieState::Pursue); NextAttack=Now+.3f; }
        return;
    }
    if (Distance<=AttackRange && Now>=NextAttack)
    {
        StopPursuit();
        FVector Look=Target->GetActorLocation()-GetActorLocation(); Look.Z=0;
        SetActorRotation(Look.Rotation());
        ChangeState(EONEZombieState::Attack); return;
    }
    GetCharacterMovement()->MaxWalkSpeed=Distance>550 ? ShambleSpeed : PursuitSpeed;
    if (Now>=NextPath)
    {
        NextPath=Now+.4f+FMath::FRandRange(0,.1f);
        if (auto* AI=Cast<AAIController>(GetController())) AI->MoveToActor(Target,AttackRange*.25f,true,true,true,nullptr,true);
    }
}
void AONEZombie::ReceiveBullet(const FHitResult& Hit,const FVector& Direction,float Damage)
{
    // Preserve the Candidate01 single-carbine-hit entry point for gameplay probes.
    FONEWeaponDamagePacket Packet; Packet.Direction=Direction; Packet.Position=Hit.ImpactPoint; Packet.Pellets=1;
    switch (GetHitRegion(Hit))
    {
        case EONEHitRegion::Head: Packet.HeadDamage=Damage; Packet.HeadTrauma=FMath::Max(HeadSeverThreshold,Damage*2.f); break;
        case EONEHitRegion::Arm: Packet.ArmDamage=Damage; Packet.ArmTrauma=Damage; break;
        case EONEHitRegion::Body: Packet.BodyDamage=Damage; break;
        default: return;
    }
    ReceiveWeaponDamage(Packet);
}
EONEHitRegion AONEZombie::GetHitRegion(const FHitResult& Hit) const
{
    if (IsDead()) return EONEHitRegion::Invalid;
    const bool HeadHit=Hit.GetComponent()==HeadRegion || Hit.BoneName==TEXT("head");
    const bool ArmHit=Hit.GetComponent()==ArmRegion || Hit.GetComponent()==UpperArmRegion || Hit.BoneName==TEXT("upperarm_l") || Hit.BoneName==TEXT("lowerarm_l") || Hit.BoneName==TEXT("hand_l");
    if (HeadHit) return bHeadSevered ? EONEHitRegion::Invalid : EONEHitRegion::Head;
    if (ArmHit) return bArmSevered ? EONEHitRegion::Invalid : EONEHitRegion::Arm;
    return EONEHitRegion::Body;
}
bool AONEZombie::ReceiveWeaponDamage(const FONEWeaponDamagePacket& Packet)
{
    if (IsDead() || (Packet.ShotId!=0 && RecentShotIds.Contains(Packet.ShotId))) return false;
    const float Body=FMath::Max(0.f,Packet.BodyDamage);
    const float Head=bHeadSevered ? 0.f : FMath::Max(0.f,Packet.HeadDamage);
    const float Arm=bArmSevered ? 0.f : FMath::Max(0.f,Packet.ArmDamage);
    if (Body+Head+Arm<=0.f) return false;
    if (Packet.ShotId!=0)
    {
        if (RecentShotIds.Num()>=32) RecentShotIds.RemoveAt(0);
        RecentShotIds.Add(Packet.ShotId);
    }
    ++DamageTransactions;
    HeadTrauma+=bHeadSevered ? 0.f : FMath::Max(0.f,Packet.HeadTrauma);
    ArmDamage+=bArmSevered ? 0.f : FMath::Max(0.f,Packet.ArmTrauma);
    const bool HeadLoss=!bHeadSevered && Head>0 && HeadTrauma>=HeadSeverThreshold;
    const bool ArmLoss=!HeadLoss && !bArmSevered && Arm>0 && ArmDamage>=ArmSeverThreshold;
    // One discharge produces at most one detachment and one impact spray per victim.
    if (HeadLoss) { Sever(true,Packet.Direction); Health->ApplyDamage(Health->MaxHealth); }
    else
    {
        Health->ApplyDamage(Body+Head+Arm*.4f);
        if (ArmLoss) Sever(false,Packet.Direction);
    }
    if (!HeadLoss && !ArmLoss)
        if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->Impact(Packet.Position,Packet.Direction,false);
    if (Health->IsDead()) { Die(Packet.Direction); return true; }
    const float Now=GetWorld()->GetTimeSeconds();
    if (Now-LastReaction>=HitReactCooldown)
    {
        bHeavyReaction=Body+Head+Arm>=Packet.HeavyStaggerThreshold;
        LastReaction=Now; StopPursuit(); ChangeState(EONEZombieState::Hit);
        NextAttack=Now+(bHeavyReaction ? .52f : .4f);
    }
    return true;
}
void AONEZombie::Sever(bool bHead,const FVector& Direction)
{
    if ((bHead&&bHeadSevered)||(!bHead&&bArmSevered)) return;
    ++SeverCount;
    USkeletalMeshComponent* Part=bHead ? HeadMesh.Get() : ArmMesh.Get();
    const FName Bone=bHead ? FName(TEXT("head")) : FName(TEXT("upperarm_l"));
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>())
    {
        Blood->Detach(Part,GetMesh(),Bone,Direction);
        Blood->Impact(GetMesh()->GetSocketLocation(Bone),Direction,true);
    }
    Part->SetVisibility(false);
    if (bHead) { bHeadSevered=true; HeadRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
    else { bArmSevered=true; ArmRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); UpperArmRegion->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}
void AONEZombie::Die(const FVector& Direction)
{
    if (IsDead()) return;
    StopPursuit(); ChangeState(EONEZombieState::Dead);
    GetCharacterMovement()->DisableMovement();
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    for (UPrimitiveComponent* C : TArray<UPrimitiveComponent*>{HeadRegion,ArmRegion,UpperArmRegion,BodyRegion}) C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>())
    {
        Blood->Pool(GetActorLocation()+GetActorForwardVector()*30.f,75.f);
        Blood->RegisterCorpse(this);
    }
    SetLifeSpan(28.f);
    if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->NotifyZombieKilled(this,100);
}
