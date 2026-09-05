#include "ONEWeaponComponent.h"
#include "ONEWeaponCatalog.h"
#include "ONEPlayer.h"
#include "Kismet/GameplayStatics.h"

namespace ONEInventoryIds
{
    uint64 NextRun=0,NextInstance=0,NextReservation=0;
}

const FONEWeaponDefinition* UONEWeaponComponent::GetCatalogDefinition(EONEWeaponFamily Family,bool bUpgraded) const
{ return WeaponDefinitions.FindByPredicate([=](const auto& D){ return D.Family==Family && D.bUpgraded==bUpgraded; }); }
const FONECarriedWeaponState* UONEWeaponComponent::GetSlotState(int32 Slot) const
{ return Carried.IsValidIndex(Slot) ? &Carried[Slot] : nullptr; }
bool UONEWeaponComponent::IsSlotAvailable(int32 Slot) const
{ const auto* S=GetSlotState(Slot); return S && S->Status==EONEWeaponSlotStatus::Available && S->InstanceId!=0 && GetDefinitionForWeapon(Slot); }
bool UONEWeaponComponent::HasUsableWeapon() const { return IsSlotAvailable(EquippedIndex); }
const FONEWeaponDefinition* UONEWeaponComponent::GetDefinitionForWeapon(int32 Slot) const
{
    const auto* S=GetSlotState(Slot);
    return S && S->Status!=EONEWeaponSlotStatus::Empty ? GetCatalogDefinition(S->Family,S->bUpgraded) : nullptr;
}
const FONEWeaponDefinition& UONEWeaponComponent::GetDefinition() const
{ return HasUsableWeapon() ? *GetDefinitionForWeapon(EquippedIndex) : ONEWeaponCatalog::Unarmed(); }

void UONEWeaponComponent::RefillSlot(int32 Slot,bool bMaximumReserve)
{
    if (!IsSlotAvailable(Slot)) return;
    auto& S=Carried[Slot]; const auto& D=*GetDefinitionForWeapon(Slot);
    S.Ammo=D.Capacity; S.Reserve=FMath::Clamp(bMaximumReserve ? D.ReserveLimit : D.InitialReserve,0,D.ReserveLimit);
    S.bNeedsPump=false; S.bCaseEjected=false; S.PendingCaseShotId=0; S.bMagazinePresent=true;
}
void UONEWeaponComponent::InstallWeapon(int32 Slot,EONEWeaponFamily Family,bool bUpgraded)
{
    if (!Carried.IsValidIndex(Slot) || !GetCatalogDefinition(Family,bUpgraded)) return;
    auto& S=Carried[Slot]; S=FONECarriedWeaponState();
    S.InstanceId=++ONEInventoryIds::NextInstance; S.Family=Family; S.bUpgraded=bUpgraded; S.Status=EONEWeaponSlotStatus::Available;
    RefillSlot(Slot,false);
}
void UONEWeaponComponent::ResetStarterLoadout()
{
    CancelAllOperations(); ClearEjectedCases(); ActiveReservation=FONEWeaponReservation(); bHandoffLocked=false;
    RunId=++ONEInventoryIds::NextRun; Carried.Reset(); Carried.SetNum(2);
    InstallWeapon(0,EONEWeaponFamily::Pistol); EquippedIndex=0; OperationIndex=0;
    LastShot=-100; LastEmpty=-100; ++InventoryRevision; RefreshEquippedPresentation();
}
void UONEWeaponComponent::GiveTestLoadout()
{
    ResetStarterLoadout(); InstallWeapon(0,EONEWeaponFamily::Carbine); InstallWeapon(1,EONEWeaponFamily::Shotgun);
    EquippedIndex=0; ++InventoryRevision; RefreshEquippedPresentation();
}
void UONEWeaponComponent::SetHandoffLocked(bool bLocked)
{
    if (bHandoffLocked==bLocked) return;
    if (bLocked) { if (IsReloading()) CancelReload(); CancelAllOperations(); }
    bHandoffLocked=bLocked; ++InventoryRevision;
}
void UONEWeaponComponent::ChooseAvailableAfterRemoval()
{
    EquippedIndex=INDEX_NONE;
    for (int32 I=0;I<2;++I) if (IsSlotAvailable(I)) { EquippedIndex=I; break; }
    OperationIndex=EquippedIndex; RefreshEquippedPresentation();
    // The handoff owner releases the lock before gameplay resumes; Tick then
    // resumes any legitimate pump obligation on this remaining weapon.
}
bool UONEWeaponComponent::ReserveEquippedForUpgrade(FONEWeaponReservation& Out)
{
    Out=FONEWeaponReservation();
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || ActiveReservation.IsValid() || !HasUsableWeapon()) return false;
    const auto* D=GetDefinitionForWeapon(EquippedIndex); if (!D || D->bUpgraded || !GetCatalogDefinition(D->Family,true)) return false;
    if (IsReloading()) CancelReload();
    CancelAllOperations();
    ActiveReservation.RunId=RunId; ActiveReservation.ReservationId=++ONEInventoryIds::NextReservation;
    ActiveReservation.Slot=EquippedIndex; ActiveReservation.InstanceId=Carried[EquippedIndex].InstanceId;
    ActiveReservation.Before=Carried[EquippedIndex]; Out=ActiveReservation;
    Carried[EquippedIndex].Status=EONEWeaponSlotStatus::MachineReserved;
    ++InventoryRevision; ChooseAvailableAfterRemoval(); return true;
}
bool UONEWeaponComponent::MatchesReservation(const FONEWeaponReservation& Token) const
{
    const auto* S=GetSlotState(Token.Slot);
    return Token.IsValid() && ActiveReservation.IsValid() && Token.RunId==RunId && Token.RunId==ActiveReservation.RunId &&
        Token.ReservationId==ActiveReservation.ReservationId && Token.InstanceId==ActiveReservation.InstanceId && Token.Slot==ActiveReservation.Slot &&
        S && S->InstanceId==Token.InstanceId && (S->Status==EONEWeaponSlotStatus::MachineReserved || S->Status==EONEWeaponSlotStatus::ReadyToCollect);
}
bool UONEWeaponComponent::MarkUpgradeReady(const FONEWeaponReservation& Token)
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || !MatchesReservation(Token) || Carried[Token.Slot].Status!=EONEWeaponSlotStatus::MachineReserved) return false;
    Carried[Token.Slot].Status=EONEWeaponSlotStatus::ReadyToCollect; ++InventoryRevision; return true;
}
bool UONEWeaponComponent::CollectUpgrade(const FONEWeaponReservation& Token)
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || !MatchesReservation(Token) || Carried[Token.Slot].Status!=EONEWeaponSlotStatus::ReadyToCollect) return false;
    if (!GetCatalogDefinition(Carried[Token.Slot].Family,true)) return false;
    if (IsReloading()) CancelReload(); CancelAllOperations();
    const int32 Slot=Token.Slot;
    auto& S=Carried[Slot]; S.bUpgraded=true; S.Status=EONEWeaponSlotStatus::Available;
    RefillSlot(Slot,true); ActiveReservation=FONEWeaponReservation(); ++InventoryRevision;
    // Always pass through the normal equip operation, including return to the
    // previously unarmed slot. Its midpoint installs the visible assembly.
    PendingIndex=Slot; StartOperation(EONEWeaponOperation::Equip,Slot); return true;
}
bool UONEWeaponComponent::RollbackUpgrade(const FONEWeaponReservation& Token)
{
    if (!MatchesReservation(Token)) return false;
    const int32 Slot=Token.Slot;
    Carried[Slot]=ActiveReservation.Before; ActiveReservation=FONEWeaponReservation(); ++InventoryRevision;
    if (!HasUsableWeapon()) { EquippedIndex=Slot; OperationIndex=EquippedIndex; RefreshEquippedPresentation(); }
    return true;
}
void UONEWeaponComponent::InvalidateMachineTransactions()
{
    CancelAllOperations();
    if (ActiveReservation.IsValid()) RollbackUpgrade(ActiveReservation);
    ActiveReservation=FONEWeaponReservation(); bHandoffLocked=false; RunId=++ONEInventoryIds::NextRun; ++InventoryRevision;
}
void UONEWeaponComponent::CycleWeapon()
{
    const int32 From=PendingIndex>=0 ? PendingIndex : EquippedIndex;
    for (int32 Step=1;Step<=2;++Step)
    { const int32 Slot=(FMath::Max(-1,From)+Step)%2; if (IsSlotAvailable(Slot) && Slot!=From) { SelectWeapon(Slot); return; } }
}
bool UONEWeaponComponent::IsFamilyRollEligible(EONEWeaponFamily Family) const
{
    if (!GetCatalogDefinition(Family,false)) return false;
    for (const auto& S:Carried) if (S.Family==Family && (S.Status==EONEWeaponSlotStatus::MachineReserved || S.Status==EONEWeaponSlotStatus::ReadyToCollect)) return false;
    return true;
}
FONEWeaponAcquisitionPlan UONEWeaponComponent::BuildAcquisitionPlan(EONEWeaponFamily Family) const
{
    FONEWeaponAcquisitionPlan Plan; Plan.Family=Family; Plan.RunId=RunId; Plan.Revision=InventoryRevision;
    if (bHandoffLocked || !IsFamilyRollEligible(Family)) return Plan;
    for (int32 I=0;I<Carried.Num();++I) if (IsSlotAvailable(I) && Carried[I].Family==Family)
    {
        Plan.Slot=I; Plan.ExpectedInstanceId=Carried[I].InstanceId; const auto& D=*GetDefinitionForWeapon(I);
        Plan.Kind=Carried[I].Ammo>=D.Capacity && Carried[I].Reserve>=D.ReserveLimit ? EONEWeaponAcquisitionKind::AlreadyFull : EONEWeaponAcquisitionKind::Refill;
        return Plan;
    }
    for (int32 I=0;I<Carried.Num();++I) if (Carried[I].Status==EONEWeaponSlotStatus::Empty)
    { Plan.Slot=I; Plan.Kind=EONEWeaponAcquisitionKind::FillEmpty; return Plan; }
    if (HasUsableWeapon()) { Plan.Slot=EquippedIndex; Plan.ExpectedInstanceId=Carried[EquippedIndex].InstanceId; Plan.Kind=EONEWeaponAcquisitionKind::Replace; }
    return Plan;
}
bool UONEWeaponComponent::ApplyAcquisitionPlan(const FONEWeaponAcquisitionPlan& Plan)
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || !Plan.IsValid() || BuildAcquisitionPlan(Plan.Family)!=Plan) return false;
    if (IsReloading()) CancelReload(); CancelAllOperations();
    if (Plan.Kind==EONEWeaponAcquisitionKind::Refill || Plan.Kind==EONEWeaponAcquisitionKind::AlreadyFull) RefillSlot(Plan.Slot,true);
    else InstallWeapon(Plan.Slot,Plan.Family);
    ++InventoryRevision;
    // An acquired/refilled reward is readied visibly, never silently replaces
    // the index before the authored equip midpoint.
    if (EquippedIndex==Plan.Slot) EquippedIndex=INDEX_NONE;
    PendingIndex=Plan.Slot; StartOperation(EONEWeaponOperation::Equip,Plan.Slot); return true;
}
