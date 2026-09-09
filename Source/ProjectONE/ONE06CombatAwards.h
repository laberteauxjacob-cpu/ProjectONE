#pragma once
#include "CoreMinimal.h"
#include "UObject/ObjectKey.h"
#include "ONE06CombatTypes.h"
#include "ONEWeaponTypes.h"

// Bounded receipt ledger; evicted or old-run receipts remain invalid rather than
// becoming awardable again. It owns combat arithmetic, never prices or refunds.
class PROJECTONE_API FONECombatAwardLedger
{
public:
    void Reset(const FGuid& NewRun);
    void Invalidate();
    FONECombatDischargeContext Begin(int32 Multiplier, bool InstaKill);
    void End(const FONECombatDischargeContext& Context);
    void RegisteredDeath(const FObjectKey& Victim);
    int32 Resolve(const FONECombatDischargeContext& Context, const FObjectKey& Victim,
                  EONEWeaponHitOutcome Outcome, bool HeadshotQualified, bool StillRegisteredAlive);
    const FGuid& GetRunId() const { return RunId; }
    int32 GetRetainedDischarges() const { return Receipts.Num(); }
    static constexpr int32 MaximumDischarges=128;
    static constexpr int32 MaximumVictimsPerDischarge=256;
private:
    struct FReceipt
    {
        FONECombatDischargeContext Context;
        TSet<FObjectKey> AwardedVictims;
        TSet<FObjectKey> RegisteredDeaths;
    };
    FReceipt* Find(const FONECombatDischargeContext& Context);
    FGuid RunId;
    uint64 NextId=0, ActiveId=0;
    TMap<uint64,FReceipt> Receipts;
};
