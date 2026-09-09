#include "ONEPowerUpComponent.h"
#include "ONEPowerUpPickup.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Engine/World.h"

void FONEPowerUpModifiers::Advance(float Seconds)
{
    if (!FMath::IsFinite(Seconds) || Seconds<=0.f) return;
    InstaKillSeconds=FMath::Max(0.,InstaKillSeconds-Seconds);
    DoublePointsSeconds=FMath::Max(0.,DoublePointsSeconds-Seconds);
}
void FONEPowerUpModifiers::Refresh(EONEPowerUpType Type,float Duration)
{
    if (!FMath::IsFinite(Duration) || Duration<=0.f) return;
    if (Type==EONEPowerUpType::InstaKill) InstaKillSeconds=Duration;
    if (Type==EONEPowerUpType::DoublePoints) DoublePointsSeconds=Duration;
}
float FONEPowerUpModifiers::Remaining(EONEPowerUpType Type) const
{ return float(Type==EONEPowerUpType::InstaKill ? InstaKillSeconds : Type==EONEPowerUpType::DoublePoints ? DoublePointsSeconds : 0.); }
bool ONEPowerUpRules::PassesChance(float Sample,float Chance)
{ return FMath::IsFinite(Sample) && Sample>=0.f && Sample<1.f && FMath::IsFinite(Chance) && Sample<FMath::Clamp(Chance,0.f,1.f); }
EONEPowerUpType ONEPowerUpRules::WeightedType(float Sample,const FVector& Weights)
{
    if (!FMath::IsFinite(Sample) || Sample<0.f || Sample>=1.f || Weights.ContainsNaN()) return EONEPowerUpType::Count;
    const double A=FMath::Max(0.,Weights.X),B=FMath::Max(0.,Weights.Y),C=FMath::Max(0.,Weights.Z),Sum=A+B+C;
    if (!FMath::IsFinite(Sum) || Sum<=0.) return EONEPowerUpType::Count;
    const double Pick=Sample*Sum;
    return Pick<A ? EONEPowerUpType::InstaKill : Pick<A+B ? EONEPowerUpType::DoublePoints : EONEPowerUpType::MaxAmmo;
}
UONEPowerUpComponent::UONEPowerUpComponent() { PrimaryComponentTick.bCanEverTick=true; }
void UONEPowerUpComponent::ResetForRun(const FGuid& NewRun)
{
    InvalidateRun(); RunId=NewRun; Stats={}; DropRandom.Initialize(DropSeed);
    PickupNotificationSerial=0; LastCollectedType=EONEPowerUpType::Count;
    LastDropResult=EONEPowerUpDropResult::NotEligible;
}
void UONEPowerUpComponent::InvalidateRun()
{
    RunId.Invalidate(); Modifiers.Reset(); PickupNotificationSerial=0; LastCollectedType=EONEPowerUpType::Count;
    const auto Old=Pickups; Pickups.Empty();
    for (const auto& Pickup:Old) if (Pickup.IsValid()) Pickup->Destroy();
}
void UONEPowerUpComponent::EndPlay(const EEndPlayReason::Type Reason)
{ InvalidateRun(); Super::EndPlay(Reason); }
void UONEPowerUpComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime,TickType,TickFunction);
    if (!RunId.IsValid() || (GetWorld() && UGameplayStatics::IsGamePaused(this))) return;
    Modifiers.Advance(DeltaTime);
    Pickups.RemoveAll([](const auto& Item){ return !Item.IsValid(); });
}
int32 UONEPowerUpComponent::GetActiveDropCount() const
{
    int32 Count=0;
    for (const auto& Pickup:Pickups) if (Pickup.IsValid() && Pickup->IsAvailable()) ++Count;
    return Count;
}
EONEPowerUpDropResult UONEPowerUpComponent::ConsiderRegisteredDeath(const FVector& Location)
{
    if (!RunId.IsValid()) return LastDropResult=EONEPowerUpDropResult::NotEligible;
    ++Stats.EligibleDeaths;
    if (!ONEPowerUpRules::PassesChance(DropRandom.GetFraction(),TotalDropChance))
    { ++Stats.ChanceMisses; return LastDropResult=EONEPowerUpDropResult::ChanceMiss; }
    ++Stats.PassedRolls;
    const EONEPowerUpType Type=ONEPowerUpRules::WeightedType(DropRandom.GetFraction(),TypeWeights);
    if (Type==EONEPowerUpType::Count) { ++Stats.DisabledWeights; return LastDropResult=EONEPowerUpDropResult::DisabledWeights; }
    return LastDropResult=SpawnDrop(Type,Location,false);
}
EONEPowerUpDropResult UONEPowerUpComponent::ForceDrop(EONEPowerUpType Type,const FVector& Location)
{
    const auto* GM=Cast<AONEGameMode>(GetOwner());
    if (!GM || !GM->IsSandbox() || GM->IsGameOver() || !RunId.IsValid() || Type>=EONEPowerUpType::Count)
        return LastDropResult=EONEPowerUpDropResult::NotEligible;
    ++Stats.ForcedAttempts;
    return LastDropResult=SpawnDrop(Type,Location,true);
}
bool UONEPowerUpComponent::FindReachableGround(const FVector& Near,FVector& Result) const
{
    if (!GetWorld() || Near.ContainsNaN()) return false;
    const auto* Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Player || Player->IsDead() || !Nav) return false;
    FNavLocation PlayerFloor;
    if (!Nav->ProjectPointToNavigation(Player->GetNavAgentLocation(),PlayerFloor,FVector(80,80,80))) return false;
    const FVector Offsets[]={FVector::ZeroVector,FVector(90,0,0),FVector(-90,0,0),FVector(0,90,0),FVector(0,-90,0),
        FVector(90,90,0),FVector(-90,90,0),FVector(90,-90,0),FVector(-90,-90,0),
        FVector(180,0,0),FVector(-180,0,0),FVector(0,180,0),FVector(0,-180,0)};
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ONEPowerUpGround),false,Player);
    FCollisionObjectQueryParams Objects; Objects.AddObjectTypesToQuery(ECC_WorldStatic); Objects.AddObjectTypesToQuery(ECC_WorldDynamic);
    for (const FVector& Offset:Offsets)
    {
        FHitResult Ground;
        if (!GetWorld()->LineTraceSingleByObjectType(Ground,Near+Offset+FVector(0,0,150),Near+Offset-FVector(0,0,450),Objects,Query)
            || Ground.ImpactNormal.Z<.65f) continue;
        FNavLocation Floor;
        if (!Nav->ProjectPointToNavigation(Ground.ImpactPoint,Floor,FVector(55,55,35))) continue;
        UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(GetWorld(),PlayerFloor.Location,Floor.Location);
        if (!Path || !Path->IsValid() || Path->IsPartial()) continue;
        const FVector Center=Floor.Location+FVector(0,0,45);
        // No wall/prop may cover the pickup. Pawn corpse collision is considered
        // too; another bounded offset is preferable to burying the emblem.
        FCollisionObjectQueryParams Occupants=Objects; Occupants.AddObjectTypesToQuery(ECC_Pawn); Occupants.AddObjectTypesToQuery(ECC_PhysicsBody);
        if (GetWorld()->OverlapAnyTestByObjectType(Center,FQuat::Identity,Occupants,FCollisionShape::MakeSphere(44.f),Query)) continue;
        FHitResult Reach;
        if (GetWorld()->LineTraceSingleByObjectType(Reach,Floor.Location+FVector(0,0,8),Center,Objects,Query)) continue;
        Result=Center; return true;
    }
    return false;
}
EONEPowerUpDropResult UONEPowerUpComponent::SpawnDrop(EONEPowerUpType Type,const FVector& Location,bool Forced)
{
    // The cap applies to live pickup actors including their brief collection cue
    // tail, so rapid forced collection cannot build an unbounded sound/actor bank.
    Pickups.RemoveAll([](const auto& Item){ return !Item.IsValid(); });
    if (Pickups.Num()>=FMath::Clamp(MaximumWorldDrops,1,32))
    { if (Forced) ++Stats.ForcedFailures; else ++Stats.CapSuppressed; return EONEPowerUpDropResult::CapSuppressed; }
    FVector Position;
    if (!FindReachableGround(Location,Position)) { if (Forced) ++Stats.ForcedFailures; else ++Stats.NoReachableGround; return EONEPowerUpDropResult::NoReachableGround; }
    const FTransform Transform(FRotator::ZeroRotator,Position);
    auto* Pickup=GetWorld()->SpawnActorDeferred<AONEPowerUpPickup>(AONEPowerUpPickup::StaticClass(),Transform,GetOwner(),nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Pickup) { if (Forced) ++Stats.ForcedFailures; else ++Stats.SpawnFailed; return EONEPowerUpDropResult::SpawnFailed; }
    Pickup->Initialize(this,Type,RunId,WorldLifetime,CollectionRadius);
    Pickups.Add(Pickup);
    UGameplayStatics::FinishSpawningActor(Pickup,Transform);
    if (Forced) ++Stats.ForcedSpawned; else ++Stats.Spawned;
    UE_LOG(LogTemp,Display,TEXT("ONE06_POWERUP_SPAWN type=%d forced=%d active=%d"),int32(Type),Forced,GetActiveDropCount());
    return EONEPowerUpDropResult::Spawned;
}
bool UONEPowerUpComponent::Collect(AONEPowerUpPickup* Pickup,AONEPlayer* Player)
{
    const auto* GM=Cast<AONEGameMode>(GetOwner());
    if (!RunId.IsValid() || !GM || GM->IsGameOver() || !IsValid(Pickup) || Pickup->GetRunId()!=RunId ||
        !Pickups.Contains(Pickup) || !IsValid(Player) || Player!=UGameplayStatics::GetPlayerPawn(this,0) || Player->IsDead() ||
        !Pickup->CanReach(Player)) return false;
    const EONEPowerUpType Type=Pickup->GetType();
    if (Type>=EONEPowerUpType::Count) return false;
    // Consume first so callbacks cannot collect the same actor twice.
    Pickup->CommitCollected();
    if (Type==EONEPowerUpType::MaxAmmo) Player->GetWeaponComponent()->ApplyMaxAmmoPowerUp();
    else Modifiers.Refresh(Type,FMath::Clamp(TimedEffectDuration,1.f,300.f));
    ++Stats.Collected; ++PickupNotificationSerial; LastCollectedType=Type;
    UE_LOG(LogTemp,Display,TEXT("ONE06_POWERUP_COLLECT type=%d duration=%.3f serial=%llu"),int32(Type),GetRemainingSeconds(Type),PickupNotificationSerial);
    return true;
}
void UONEPowerUpComponent::RetirePickup(AONEPowerUpPickup* Pickup,bool Expired)
{
    if (Pickups.Remove(Pickup)>0 && Expired) ++Stats.Expired;
}
