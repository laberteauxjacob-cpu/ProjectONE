#include "ONE06CombatAwards.h"

void FONECombatAwardLedger::Reset(const FGuid& NewRun)
{ RunId=NewRun; NextId=ActiveId=0; Receipts.Empty(); }
void FONECombatAwardLedger::Invalidate()
{ Reset(FGuid()); }
FONECombatDischargeContext FONECombatAwardLedger::Begin(int32 Multiplier,bool InstaKill)
{
    if (!RunId.IsValid() || NextId==MAX_uint64) return {};
    FReceipt Receipt; Receipt.Context.RunId=RunId; Receipt.Context.DischargeId=++NextId;
    Receipt.Context.PointMultiplier=Multiplier==2 ? 2 : 1; Receipt.Context.bInstaKill=InstaKill;
    ActiveId=NextId;
    if (NextId>MaximumDischarges) Receipts.Remove(NextId-MaximumDischarges);
    Receipts.Add(NextId,Receipt);
    return Receipt.Context;
}
FONECombatAwardLedger::FReceipt* FONECombatAwardLedger::Find(const FONECombatDischargeContext& Context)
{
    if (!Context.IsValid() || Context.RunId!=RunId || Context.DischargeId!=ActiveId) return nullptr;
    FReceipt* Receipt=Receipts.Find(Context.DischargeId);
    return Receipt && Receipt->Context.PointMultiplier==Context.PointMultiplier &&
        Receipt->Context.bInstaKill==Context.bInstaKill ? Receipt : nullptr;
}
void FONECombatAwardLedger::End(const FONECombatDischargeContext& Context)
{ if (Find(Context)) ActiveId=0; }
void FONECombatAwardLedger::RegisteredDeath(const FObjectKey& Victim)
{
    if (FReceipt* Receipt=Receipts.Find(ActiveId))
        if (Receipt->RegisteredDeaths.Num()<MaximumVictimsPerDischarge) Receipt->RegisteredDeaths.Add(Victim);
}
int32 FONECombatAwardLedger::Resolve(const FONECombatDischargeContext& Context,const FObjectKey& Victim,
    EONEWeaponHitOutcome Outcome,bool HeadshotQualified,bool StillRegisteredAlive)
{
    FReceipt* Receipt=Find(Context);
    if (!Receipt || Receipt->AwardedVictims.Contains(Victim) || Receipt->AwardedVictims.Num()>=MaximumVictimsPerDischarge) return 0;
    const bool NewKill=Outcome==EONEWeaponHitOutcome::NewKill;
    if (NewKill ? !Receipt->RegisteredDeaths.Contains(Victim) :
        (Outcome!=EONEWeaponHitOutcome::LiveHit || !StillRegisteredAlive)) return 0;
    Receipt->AwardedVictims.Add(Victim);
    return (10+(NewKill ? (HeadshotQualified ? 120 : 100) : 0))*Receipt->Context.PointMultiplier;
}
