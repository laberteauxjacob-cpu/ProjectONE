#pragma once
#include "CoreMinimal.h"
#include "ONE06CombatTypes.generated.h"

UENUM(BlueprintType)
enum class EONEPowerUpType : uint8 { InstaKill, DoublePoints, MaxAmmo, Count UMETA(Hidden) };

// One authority-issued snapshot per synchronous discharge, including every pellet
// and penetrated victim. The award ledger verifies the snapshot on submission.
struct FONECombatDischargeContext
{
    FGuid RunId;
    uint64 DischargeId=0;
    int32 PointMultiplier=1;
    bool bInstaKill=false;
    bool IsValid() const { return RunId.IsValid() && DischargeId!=0; }
};

enum class EONEPowerUpDropResult : uint8
{
    NotEligible, ChanceMiss, DisabledWeights, CapSuppressed, NoReachableGround, SpawnFailed, Spawned
};
