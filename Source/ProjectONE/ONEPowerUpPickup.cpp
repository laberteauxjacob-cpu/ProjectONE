#include "ONEPowerUpPickup.h"
#include "ONEPowerUpComponent.h"
#include "ONE06PickupVisualComponent.h"
#include "ONEPlayer.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AONEPowerUpPickup::AONEPowerUpPickup()
{
    PrimaryActorTick.bCanEverTick=true;
    Collection=CreateDefaultSubobject<USphereComponent>(TEXT("CollectionReach"));
    SetRootComponent(Collection);
    Collection->SetSphereRadius(80.f);
    Collection->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Collection->SetCollisionObjectType(ECC_WorldDynamic);
    Collection->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collection->SetCollisionResponseToChannel(ECC_Pawn,ECR_Overlap);
    Collection->SetGenerateOverlapEvents(true);
    Collection->SetCanEverAffectNavigation(false);
    Visual=CreateDefaultSubobject<UONE06PickupVisualComponent>(TEXT("PowerUpVisual"));
    Visual->SetupAttachment(Collection);
}
void AONEPowerUpPickup::Initialize(UONEPowerUpComponent* Authority,EONEPowerUpType NewType,const FGuid& Run,float Lifetime,float Radius)
{
    Manager=Authority; Type=NewType; RunId=Run;
    TotalLifetime=RemainingLifetime=FMath::Clamp(Lifetime,1.f,300.f);
    Collection->SetSphereRadius(FMath::Clamp(Radius,20.f,150.f));
    bInitialized=true;
    Visual->Configure(Type); Visual->SetRemainingLifetime(RemainingLifetime,TotalLifetime);
}
bool AONEPowerUpPickup::CanReach(AONEPlayer* Player) const
{
    if (!IsAvailable() || !IsValid(Player) || Player->IsDead() || !GetWorld() || UGameplayStatics::IsGamePaused(this)) return false;
    // Require a real overlap with the player's collision body, then independently
    // reject intervening world geometry. Initial standing overlaps are polled too.
    if (!Collection->IsOverlappingComponent(Player->GetCapsuleComponent())) return false;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ONEPowerUpReach),false,this); Query.AddIgnoredActor(Player);
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    FHitResult Hit;
    return !GetWorld()->LineTraceSingleByObjectType(Hit,GetActorLocation(),Player->GetActorLocation(),Objects,Query);
}
bool AONEPowerUpPickup::TryCollect(AONEPlayer* Player)
{ return CanReach(Player) && Manager.IsValid() && Manager->Collect(this,Player); }
void AONEPowerUpPickup::CommitCollected()
{
    if (!IsAvailable()) return;
    bCollected=true; CollectedTail=1.05f;
    Collection->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->PlayCollectionCue();
}
void AONEPowerUpPickup::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized || !FMath::IsFinite(DeltaSeconds) || DeltaSeconds<=0.f || UGameplayStatics::IsGamePaused(this)) return;
    if (!Manager.IsValid() || Manager->GetRunId()!=RunId) { Destroy(); return; }
    if (bCollected)
    { CollectedTail-=DeltaSeconds; if (CollectedTail<=0.f) Destroy(); return; }
    // Expiry wins at the exact deadline; collection never races destruction.
    RemainingLifetime=FMath::Max(0.f,RemainingLifetime-DeltaSeconds);
    Visual->SetRemainingLifetime(RemainingLifetime,TotalLifetime);
    if (RemainingLifetime<=0.f)
    { Manager->RetirePickup(this,true); Destroy(); return; }
    TArray<AActor*> Overlaps; Collection->GetOverlappingActors(Overlaps,AONEPlayer::StaticClass());
    for (AActor* Actor:Overlaps) if (TryCollect(Cast<AONEPlayer>(Actor))) break;
}
void AONEPowerUpPickup::EndPlay(const EEndPlayReason::Type Reason)
{ if (Visual) Visual->Shutdown(); if (Manager.IsValid()) Manager->RetirePickup(this,false); Manager.Reset(); Super::EndPlay(Reason); }
