#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONE06CombatTypes.h"
#include "ONEPowerUpComponent.generated.h"

class AONEPlayer;
class AONEPowerUpPickup;

struct PROJECTONE_API FONEPowerUpModifiers
{
    void Advance(float GameplaySeconds);
    void Refresh(EONEPowerUpType Type,float Duration);
    float Remaining(EONEPowerUpType Type) const;
    void Reset() { InstaKillSeconds=DoublePointsSeconds=0.; }
private:
    double InstaKillSeconds=0.,DoublePointsSeconds=0.;
};

namespace ONEPowerUpRules
{
    PROJECTONE_API bool PassesChance(float Sample,float Chance);
    PROJECTONE_API EONEPowerUpType WeightedType(float Sample,const FVector& Weights);
}

struct FONEPowerUpDropStats
{
    uint64 EligibleDeaths=0, ChanceMisses=0, PassedRolls=0, DisabledWeights=0;
    uint64 CapSuppressed=0, NoReachableGround=0, SpawnFailed=0, Spawned=0;
    uint64 ForcedAttempts=0, ForcedSpawned=0, ForcedFailures=0, Collected=0, Expired=0;
};

UCLASS(ClassGroup=(ONE),meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONEPowerUpComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEPowerUpComponent();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void ResetForRun(const FGuid& NewRun);
    void InvalidateRun();
    const FGuid& GetRunId() const { return RunId; }
    float GetRemainingSeconds(EONEPowerUpType Type) const { return Modifiers.Remaining(Type); }
    bool IsActive(EONEPowerUpType Type) const { return GetRemainingSeconds(Type)>0.f; }
    EONEPowerUpDropResult ConsiderRegisteredDeath(const FVector& Location);
    EONEPowerUpDropResult ForceDrop(EONEPowerUpType Type,const FVector& Location);
    bool Collect(AONEPowerUpPickup* Pickup,AONEPlayer* Player);
    void RetirePickup(AONEPowerUpPickup* Pickup,bool Expired);
    int32 GetActiveDropCount() const;
    uint64 GetPickupNotificationSerial() const { return PickupNotificationSerial; }
    EONEPowerUpType GetLastCollectedType() const { return LastCollectedType; }
    const FONEPowerUpDropStats& GetDropStats() const { return Stats; }
    EONEPowerUpDropResult GetLastDropResult() const { return LastDropResult; }

    UPROPERTY(EditAnywhere,Category="Power-ups",meta=(ClampMin="0",ClampMax="1")) float TotalDropChance=.01f;
    UPROPERTY(EditAnywhere,Category="Power-ups") FVector TypeWeights=FVector(1,1,1);
    UPROPERTY(EditAnywhere,Category="Power-ups",meta=(ClampMin="1",ClampMax="32")) int32 MaximumWorldDrops=8;
    UPROPERTY(EditAnywhere,Category="Power-ups",meta=(ClampMin="1")) float WorldLifetime=30.f;
    UPROPERTY(EditAnywhere,Category="Power-ups",meta=(ClampMin="1")) float TimedEffectDuration=30.f;
    UPROPERTY(EditAnywhere,Category="Power-ups",meta=(ClampMin="20",ClampMax="150")) float CollectionRadius=80.f;
    UPROPERTY(EditAnywhere,Category="Power-ups") int32 DropSeed=6202606;
private:
    EONEPowerUpDropResult SpawnDrop(EONEPowerUpType Type,const FVector& Location,bool Forced);
    bool FindReachableGround(const FVector& Near,FVector& Result) const;
    FGuid RunId;
    FRandomStream DropRandom;
    FONEPowerUpModifiers Modifiers;
    FONEPowerUpDropStats Stats;
    TArray<TWeakObjectPtr<AONEPowerUpPickup>> Pickups;
    uint64 PickupNotificationSerial=0;
    EONEPowerUpType LastCollectedType=EONEPowerUpType::Count;
    EONEPowerUpDropResult LastDropResult=EONEPowerUpDropResult::NotEligible;
};
