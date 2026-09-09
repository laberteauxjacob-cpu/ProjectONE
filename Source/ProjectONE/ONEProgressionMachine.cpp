#include "ONEProgressionMachine.h"
#include "ONE04MachinePresentation.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
#include "ONEGameMode.h"
#include "ONE06MachineRules.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DECLARE_CATEGORY_EXTERN(ONEProgression);

AONEProgressionMachine::AONEProgressionMachine()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostPhysics;
    Collision=CreateDefaultSubobject<UBoxComponent>(TEXT("MachineCollision"));
    SetRootComponent(Collision);
    Collision->SetMobility(EComponentMobility::Static);
    Collision->SetCollisionProfileName(TEXT("BlockAll"));
    Collision->SetCanEverAffectNavigation(true);
    Presentation=CreateDefaultSubobject<UONE04MachinePresentation>(TEXT("MachinePresentation"));
    Presentation->SetupAttachment(Collision);
    Tags.Add(TEXT("Metal"));
}
AONEMysteryBox::AONEMysteryBox()
{
    bIsBox=true; Collision->SetBoxExtent(FVector(65,102,52));
    // Actor origin is the collision center. The model's ground root sits below.
    Presentation->SetRelativeLocation(FVector(-3,0,-52));
}
AONEUpgradeMachine::AONEUpgradeMachine()
{
    bIsBox=false; Collision->SetBoxExtent(FVector(100,110,110));
    Presentation->SetRelativeLocation(FVector(-26,0,-110));
}
void AONEProgressionMachine::BeginPlay()
{
    Super::BeginPlay(); Presentation->Configure(bIsBox);
    if (!Presentation->IsConfigured()) { RecoverTechnicalFailure(); return; }
    UpdatePresentation();
}
float AONEProgressionMachine::RollWeight(EONEWeaponFamily Family) const
{
    const double V=Family==EONEWeaponFamily::Pistol ? RollWeights.X : Family==EONEWeaponFamily::Carbine ? RollWeights.Y :
        Family==EONEWeaponFamily::Shotgun ? RollWeights.Z : 0;
    return FMath::IsFinite(V) ? float(FMath::Clamp(V,0.,10000.)) : 0.f;
}
FVector AONEProgressionMachine::GetInteractionPoint() const
{
    return Presentation->GetComponentTransform().TransformPosition(bIsBox ? FVector(80,0,86) : FVector(128,0,106));
}
bool AONEProgressionMachine::CanReach(const AONEPlayer* P) const
{
    if (!P || P->IsDead() || State==EONEMachineState::Disabled) return false;
    const FVector Focus=GetInteractionPoint();
    const FVector Local=Presentation->GetComponentTransform().InverseTransformPosition(P->GetActorLocation());
    if (bIsBox)
    {
        if (Local.X<60.f || FMath::Abs(P->GetActorLocation().Z-Focus.Z)>145.f ||
            FVector::DistSquared2D(P->GetActorLocation(),Focus)>FMath::Square(180.f)) return false;
    }
    else if (!ONE06MachineRules::WithinUpgradeArea(Local,FMath::Clamp(UpgradeReachCm,100.f,250.f),
        FMath::Clamp(UpgradeMinimumLocalX,-40.f,60.f))) return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(MachineReach),false,P);
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,P->GetActorLocation()+FVector(0,0,25),Focus,ECC_Visibility,Params);
    return !Blocked || Hit.GetActor()==this;
}
bool AONEProgressionMachine::CanDeposit(AONEPlayer* P,FString& Reason) const
{
    const auto* W=P ? P->GetWeaponComponent() : nullptr;
    const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!CanContact(P)) { Reason=TEXT("Approach the front or side with a clear path"); return false; }
    if (!W || !W->HasUsableWeapon()) { Reason=TEXT("Equip an available base weapon"); return false; }
    if (W->IsHandoffLocked()) { Reason=TEXT("Finish the weapon handoff"); return false; }
    if (W->GetDefinition().bUpgraded) { Reason=TEXT("Already upgraded - one tier per weapon"); return false; }
    if (!GM || GM->IsGameOver() || GM->GetPoints()<UpgradePrice) { Reason=TEXT("Requires 5,000 points"); return false; }
    return true;
}
bool AONEProgressionMachine::CanContact(const AONEPlayer* P) const
{
    return CanReach(P);
}
FONEInteractionOffer AONEProgressionMachine::BuildOffer(AONEPlayer* P) const
{
    FONEInteractionOffer O; O.Machine=const_cast<AONEProgressionMachine*>(this); O.Epoch=Epoch;
    O.Title=bIsBox ? TEXT("MYSTERY BOX") : TEXT("PACK-A-PUNCH");
    if (!P || P->IsDead()) return O;
    const auto* W=P->GetWeaponComponent(); const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    O.RunId=W->GetRunId(); O.Revision=W->GetInventoryRevision(); O.Slot=W->GetEquippedIndex();
    if (const auto* S=W->GetSlotState(O.Slot)) O.InstanceId=S->InstanceId;
    if (State==EONEMachineState::Idle)
    {
        O.Price=bIsBox ? BoxPrice : UpgradePrice;
        O.Action=bIsBox ? EONEInteractionAction::BuyBox : EONEInteractionAction::DepositUpgrade;
        O.Input=bIsBox ? EONEInteractionInput::Hold : EONEInteractionInput::Tap;
        if (bIsBox)
        {
            O.Detail=TEXT("950 points - roll one weapon");
            O.bEnabled=GM && !GM->IsGameOver() && GM->GetPoints()>=BoxPrice && !W->IsHandoffLocked();
            if (GM && GM->GetPoints()<BoxPrice) O.Detail=TEXT("Requires 950 points");
            if (W->IsHandoffLocked()) O.Detail=TEXT("Finish the weapon handoff");
            bool bEligible=false;
            for (const auto F:{EONEWeaponFamily::Pistol,EONEWeaponFamily::Carbine,EONEWeaponFamily::Shotgun})
                bEligible|=W->IsFamilyRollEligible(F) && RollWeight(F)>0;
            if (!bEligible) { O.bEnabled=false; O.Detail=TEXT("All available weapon types are owned - no eligible reward"); }
            if (GM && GM->GetForcedBoxReward()!=EONEWeaponFamily::Invalid &&
                (!W->IsFamilyRollEligible(GM->GetForcedBoxReward()) || RollWeight(GM->GetForcedBoxReward())<=0))
            { O.bEnabled=false; O.Detail=TEXT("Forced test reward is owned or excluded - V resets to random"); }
        }
        else
        {
            O.bEnabled=CanDeposit(P,O.Detail);
            if (O.bEnabled)
            {
                bool OnlyWeapon=true;
                for (int32 Slot=0;Slot<W->GetWeaponCount();++Slot)
                    if (const auto* Other=W->GetSlotState(Slot);Slot!=O.Slot && Other && Other->Status==EONEWeaponSlotStatus::Available)
                        OnlyWeapon=false;
                O.Detail=FString::Printf(TEXT("5,000 - upgrade %s; reserve slot %d.%s"),*W->GetDefinition().DisplayName.ToString(),O.Slot+1,
                    OnlyWeapon?TEXT(" UNARMED until automatic return or another acquisition."):TEXT(""));
            }
        }
    }
    else if (State==EONEMachineState::Ready && IsCurrentOwner() && Customer.Get()==P)
    {
        if (bIsBox)
        {
            O.Action=EONEInteractionAction::CollectBox;
            O.Input=EONEInteractionInput::Hold;
            O.Acquisition=W->BuildAcquisitionPlan(RewardFamily);
            O.bEnabled=W->IsFamilyRollEligible(RewardFamily) && O.Acquisition.IsValid();
            const auto* Reward=W->GetCatalogDefinition(RewardFamily);
            const auto* Current=W->GetDefinitionForWeapon(O.Acquisition.Slot);
            const FString Name=Reward ? Reward->DisplayName.ToString() : TEXT("Weapon");
            switch (O.Acquisition.Kind)
            {
                case EONEWeaponAcquisitionKind::FillEmpty: O.Detail=FString::Printf(TEXT("Take %s into empty slot %d"),*Name,O.Acquisition.Slot+1); break;
                case EONEWeaponAcquisitionKind::Replace: O.Detail=FString::Printf(TEXT("Take %s - REPLACE %s in slot %d"),*Name,Current?*Current->DisplayName.ToString():TEXT("weapon"),O.Acquisition.Slot+1); break;
                default: O.Detail=TEXT("Owned-family delivery invalid - refunding this roll"); break;
            }
            if (W->IsMagazineReloadCommitted())
            { O.bEnabled=false; O.Detail=TEXT("Finish reloading, then hold F to collect"); }
        }
        else
        {
            O.Action=EONEInteractionAction::CollectUpgrade;
            O.Input=EONEInteractionInput::Automatic;
            O.bEnabled=false;
            O.ReadySecondsRemaining=GetReadySecondsRemaining(); O.bExpiryWarning=IsExpiryWarning();
            const auto* D=W->GetCatalogDefinition(RewardFamily,true);
            O.Detail=FString::Printf(TEXT("Approach to receive %s - no F - %.1fs before loss - slot %d"),
                D?*D->DisplayName.ToString():TEXT("upgrade"),O.ReadySecondsRemaining,Reservation.Slot+1);
        }
    }
    else if (State==EONEMachineState::Active)
        O.Detail=bIsBox ? TEXT("Selecting weapon...") : FString::Printf(TEXT("UPGRADING - %.1fs - slot %d reserved"),FMath::Max(0.f,ActiveDuration-StateElapsed),Reservation.Slot+1);
    else if (State==EONEMachineState::Handoff) O.Detail=TEXT("Handing over weapon...");
    else if (State==EONEMachineState::Collecting) O.Detail=TEXT("Retrieving weapon...");
    else if (State==EONEMachineState::Closing) O.Detail=TEXT("Resetting mechanism...");
    else O.Detail=TEXT("Unavailable");
    O.bEnabled=O.bEnabled && CanReach(P) && !UGameplayStatics::IsGamePaused(this);
    return O;
}
bool AONEProgressionMachine::CommitOffer(AONEPlayer* P,const FONEInteractionOffer& Offered)
{
    const FONEInteractionOffer Current=BuildOffer(P);
    if (!Current.bEnabled || !Current.SameContext(Offered) || !CanReach(P))
    {
        if (bIsBox && State==EONEMachineState::Ready && IsOwnedBy(P) &&
            !P->GetWeaponComponent()->IsFamilyRollEligible(RewardFamily)) RejectInvalidBoxDelivery();
        return false;
    }
    auto* W=P->GetWeaponComponent(); auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!GM || GM->IsGameOver()) return false;
    if (Current.Action==EONEInteractionAction::BuyBox)
    {
        TArray<EONEWeaponFamily> Eligible;
        for (const auto F:{EONEWeaponFamily::Pistol,EONEWeaponFamily::Carbine,EONEWeaponFamily::Shotgun})
            if (W->IsFamilyRollEligible(F) && RollWeight(F)>0) Eligible.Add(F);
        if (Eligible.IsEmpty()) return false;
        const EONEWeaponFamily Forced=GM->GetForcedBoxReward();
        if (Forced!=EONEWeaponFamily::Invalid && !Eligible.Contains(Forced)) return false;
        const uint64 Receipt=GM->NewMachineReceipt();
        if (!GM->TrySpendPoints(BoxPrice,Receipt)) return false;
        PaymentReceipt=Receipt; Customer=P; OwnerRunId=W->GetRunId(); bDelivered=false;
        RollPool=Eligible;
        GM->ConsumeForcedBoxReward();
        float Total=0; for (const auto F:Eligible) Total+=RollWeight(F);
        float Draw=FMath::FRand()*Total; RewardFamily=Eligible.Last();
        for (const auto F:Eligible) { Draw-=RollWeight(F); if (Draw<=0) { RewardFamily=F; break; } }
        if (Eligible.Contains(Forced)) RewardFamily=Forced;
        ActiveDuration=FMath::Clamp(RollDuration,3.f,8.f);
        ++AcceptedCount; CycleIndex=0; NextCycle=0;
        SetState(EONEMachineState::Active);
        UE_LOG(LogTemp,Display,TEXT("ONE04_BOX_ACCEPT receipt=%llu run=%llu result=%d points=%d"),PaymentReceipt,OwnerRunId,int32(RewardFamily),GM->GetPoints());
        return true;
    }
    if (Current.Action==EONEInteractionAction::CollectBox)
    {
        if (!W->IsFamilyRollEligible(RewardFamily)) { RejectInvalidBoxDelivery(); return false; }
        if (!W->ApplyAcquisitionPlan(Current.Acquisition)) return false;
        GM->CloseMachineReceipt(PaymentReceipt);
        bDelivered=true; ++DeliveredCount; Presentation->SetPreview(nullptr); SetState(EONEMachineState::Closing);
        UE_LOG(LogTemp,Display,TEXT("ONE04_BOX_COLLECT receipt=%llu kind=%d slot=%d"),PaymentReceipt,int32(Current.Acquisition.Kind),Current.Acquisition.Slot);
        return true;
    }
    if (Current.Action==EONEInteractionAction::DepositUpgrade)
        return AcceptUpgrade(P);
    return false;
}
bool AONEProgressionMachine::IsCurrentOwner() const
{
    const auto* P=Customer.Get();
    return !bInvalidated && P && !P->IsDead() && P->GetWeaponComponent()->GetRunId()==OwnerRunId;
}
bool AONEProgressionMachine::IsOwnedBy(const AONEPlayer* Player) const
{
    return IsCurrentOwner() && Customer.Get()==Player;
}
float AONEProgressionMachine::GetReadySecondsRemaining() const
{
    return !bIsBox && State==EONEMachineState::Ready ? FMath::Max(0.f,FMath::Clamp(ReadyLifetime,1.f,60.f)-StateElapsed) : 0.f;
}
bool AONEProgressionMachine::IsExpiryWarning() const
{
    return !bIsBox && State==EONEMachineState::Ready && GetReadySecondsRemaining()<=FMath::Clamp(ExpiryWarningSeconds,0.f,15.f);
}
float AONEProgressionMachine::GetTimeSinceWeaponLost() const
{
    return GetWorld() ? FMath::Max(0.f,GetWorld()->GetTimeSeconds()-LastLostTime) : BIG_NUMBER;
}
bool AONEProgressionMachine::WasLastLossFor(const AONEPlayer* P) const
{
    return !bInvalidated && P && !P->IsDead() && LastLostOwner.Get()==P && LastLostReceipt!=0 &&
        P->GetWeaponComponent()->GetRunId()==LastLostRunId;
}
void AONEProgressionMachine::RejectInvalidBoxDelivery()
{
    if (!bIsBox || bDelivered || !IsCurrentOwner() || (State!=EONEMachineState::Ready && State!=EONEMachineState::Active)) return;
    bool bRefunded=false;
    if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) bRefunded=GM->RefundPointsOnce(PaymentReceipt);
    ++InvalidBoxDeliveryCount;
    UE_LOG(LogTemp,Display,TEXT("ONE06_BOX_INVALID_DELIVERY receipt=%llu run=%llu family=%d refunded_now=%d"),PaymentReceipt,OwnerRunId,int32(RewardFamily),bRefunded);
    PaymentReceipt=0; bDelivered=true;
    Presentation->SetPreview(nullptr); Presentation->BeginLossRetraction(); SetState(EONEMachineState::Closing);
}
void AONEProgressionMachine::ResolveReadyUpgrade()
{
    if (bIsBox || State!=EONEMachineState::Ready || !IsCurrentOwner()) return;
    AONEPlayer* P=Customer.Get(); auto* W=P->GetWeaponComponent();
    const auto Resolution=ONE06MachineRules::ResolveReady(StateElapsed,FMath::Clamp(ReadyLifetime,1.f,60.f),CanContact(P));
    if (Resolution==ONE06MachineRules::EReadyResolution::Collect)
    {
        // Ownership commits before any cosmetic transition. A different gun's
        // reload never blocks this transaction or gets canceled for retrieval.
        if (!W->CollectUpgrade(Reservation)) { RecoverTechnicalFailure(); return; }
        if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->CloseMachineReceipt(PaymentReceipt);
        bDelivered=true; ++DeliveredCount; bCollectedVisual=false;
        if (W->GetEquippedIndex()==Reservation.Slot)
        {
            P->SuppressCarriedPresentation(true);
            Presentation->BeginRetrievalTo(P->GetWeaponWorldTransform());
            bCollectedVisual=true;
            SetState(EONEMachineState::Collecting);
        }
        else { Presentation->SetPreview(nullptr); SetState(EONEMachineState::Closing); }
        UE_LOG(LogTemp,Display,TEXT("ONE06_UPGRADE_AUTO_RETURN receipt=%llu run=%llu instance=%llu slot=%d"),PaymentReceipt,OwnerRunId,Reservation.InstanceId,Reservation.Slot);
    }
    else if (Resolution==ONE06MachineRules::EReadyResolution::Expire)
    {
        if (!W->ExpireUpgrade(Reservation)) { RecoverTechnicalFailure(); return; }
        if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->CloseMachineReceipt(PaymentReceipt);
        LastLostOwner=P; LastLostRunId=OwnerRunId; LastLostReceipt=PaymentReceipt;
        LastLostFamily=RewardFamily; LastLostTime=GetWorld()->GetTimeSeconds(); ++ExpiredCount;
        UE_LOG(LogTemp,Display,TEXT("ONE06_UPGRADE_EXPIRED receipt=%llu run=%llu instance=%llu slot=%d refund=0"),PaymentReceipt,OwnerRunId,Reservation.InstanceId,Reservation.Slot);
        // This is a resolved gameplay loss. Clear the recovery token/receipt so
        // destruction during retraction cannot invoke technical rollback.
        Reservation={}; PaymentReceipt=0; bDelivered=true;
        Presentation->BeginLossRetraction(); SetState(EONEMachineState::Closing);
    }
}
bool AONEProgressionMachine::AcceptUpgrade(AONEPlayer* P)
{
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>(); FString Reason;
    if (!P || !GM || !CanDeposit(P,Reason)) return false;
    auto* W=P->GetWeaponComponent();
    const EONEWeaponFamily Family=W->GetDefinition().Family;
    const FTransform Hand=P->GetWeaponWorldTransform();
    FONEWeaponReservation Pending;
    if (!W->ReserveEquippedForUpgrade(Pending)) return false;
    const uint64 Receipt=GM->NewMachineReceipt();
    if (!GM->TrySpendPoints(UpgradePrice,Receipt))
    { W->RollbackUpgrade(Pending); return false; }
    Reservation=Pending; Customer=P; OwnerRunId=W->GetRunId(); RewardFamily=Family;
    PaymentReceipt=Receipt; ++AcceptedCount; bDelivered=false; bOutputVariant=false;
    ActiveDuration=FMath::Clamp(ProcessingDuration,8.f,10.f);
    P->BeginMachineAction(RewardFamily,false,GetInteractionPoint()); bOwnsAction=true;
    P->SuppressCarriedPresentation(true);
    Presentation->SetPreview(W->GetCatalogDefinition(RewardFamily,false)); Presentation->BeginTransferFrom(Hand);
    if (!Presentation->HasCompletePreview()) { RecoverTechnicalFailure(); return false; }
    ActionReleaseAt=.72f; SetState(EONEMachineState::Active);
    UE_LOG(LogTemp,Display,TEXT("ONE04_UPGRADE_ACCEPT receipt=%llu run=%llu instance=%llu slot=%d points=%d"),PaymentReceipt,OwnerRunId,Reservation.InstanceId,Reservation.Slot,GM->GetPoints());
    return true;
}
void AONEProgressionMachine::SetState(EONEMachineState NewState)
{
    State=NewState; StateElapsed=0; ++Epoch; UpdatePresentation();
}
void AONEProgressionMachine::UpdatePresentation()
{
    EONE04MachineVisualState Visual=EONE04MachineVisualState::Idle;
    if (State==EONEMachineState::Active) Visual=EONE04MachineVisualState::Active;
    else if (State==EONEMachineState::Ready || State==EONEMachineState::Collecting) Visual=EONE04MachineVisualState::Ready;
    else if (State==EONEMachineState::Closing) Visual=EONE04MachineVisualState::Closing;
    else if (State==EONEMachineState::Disabled) Visual=EONE04MachineVisualState::Disabled;
    Presentation->SetExpiryWarning(IsExpiryWarning());
    Presentation->UpdateVisual(Visual,StateElapsed,State==EONEMachineState::Closing?.85f:ActiveDuration);
}
void AONEProgressionMachine::FinishAction()
{
    if (bOwnsAction)
        if (AONEPlayer* P=Customer.Get()) P->EndMachineAction();
    if (bCollectedVisual)
        if (AONEPlayer* P=Customer.Get()) P->SuppressCarriedPresentation(false);
    bCollectedVisual=false;
    bOwnsAction=false;
    ActionReleaseAt=0;
}
void AONEProgressionMachine::Tick(float Dt)
{
    CSV_SCOPED_TIMING_STAT(ONEProgression,MachineState);
    Super::Tick(Dt);
    if (State==EONEMachineState::Disabled || UGameplayStatics::IsGamePaused(this)) return;
    StateElapsed+=Dt;
    if (State!=EONEMachineState::Idle && State!=EONEMachineState::Closing && !IsCurrentOwner()) { InvalidateRun(); return; }
    if (bIsBox && (State==EONEMachineState::Active || State==EONEMachineState::Ready) &&
        !Customer->GetWeaponComponent()->IsFamilyRollEligible(RewardFamily))
    { RejectInvalidBoxDelivery(); return; }
    if (State==EONEMachineState::Active)
    {
        auto* W=Customer->GetWeaponComponent();
        if (!bIsBox && ActionReleaseAt>0 && StateElapsed>=ActionReleaseAt) FinishAction();
        if (!bIsBox && !bOutputVariant && StateElapsed>=ActiveDuration*(7.7f/9.f))
        { bOutputVariant=true; Presentation->SetPreview(W->GetCatalogDefinition(RewardFamily,true)); }
        if (bIsBox && StateElapsed>=NextCycle && StateElapsed<ActiveDuration)
        {
            if (RollPool.IsEmpty()) { RecoverTechnicalFailure(); return; }
            Presentation->SetPreview(W->GetCatalogDefinition(RollPool[CycleIndex++%RollPool.Num()])); Presentation->PlayCycleCue();
            if (!Presentation->HasCompletePreview()) { RecoverTechnicalFailure(); return; }
            NextCycle=StateElapsed+(ActiveDuration/5.f)*FMath::Lerp(.10f,.65f,FMath::Square(StateElapsed/ActiveDuration));
        }
        if (StateElapsed>=ActiveDuration)
        {
            if (!bIsBox && !W->MarkUpgradeReady(Reservation)) { RecoverTechnicalFailure(); return; }
            Presentation->SetPreview(W->GetCatalogDefinition(RewardFamily,!bIsBox));
            if (!Presentation->HasCompletePreview()) { RecoverTechnicalFailure(); return; }
            SetState(EONEMachineState::Ready);
            UE_LOG(LogTemp,Display,TEXT("ONE04_MACHINE_READY box=%d receipt=%llu instance=%llu"),bIsBox,PaymentReceipt,Reservation.InstanceId);
        }
    }
    else if (State==EONEMachineState::Collecting)
    {
        AONEPlayer* P=Customer.Get();
        if (StateElapsed>=.18f)
        {
            Presentation->SetPreview(nullptr); P->SuppressCarriedPresentation(false);
            bCollectedVisual=false;
            SetState(EONEMachineState::Closing);
        }
    }
    else if (State==EONEMachineState::Closing && StateElapsed>=.85f)
    {
        Presentation->SetPreview(nullptr); Customer.Reset(); Reservation={}; PaymentReceipt=0;
        RewardFamily=EONEWeaponFamily::Invalid; RollPool.Reset(); SetState(EONEMachineState::Idle);
    }
    // Also resolves the Active -> Ready transition immediately, so an owner
    // already in range needs neither F nor an exit/re-enter movement.
    ResolveReadyUpgrade();
    UpdatePresentation();
}
void AONEProgressionMachine::CancelUnacceptedAction(AONEPlayer* P)
{
    if (!P || Customer.Get()!=P) return;
    if (State==EONEMachineState::Handoff)
    { FinishAction(); Customer.Reset(); Reservation={}; PaymentReceipt=0; SetState(EONEMachineState::Idle); }
}
void AONEProgressionMachine::RecoverTechnicalFailure()
{
    if (IsCurrentOwner() && !bDelivered)
    {
        if (Reservation.IsValid()) Customer->GetWeaponComponent()->RollbackUpgrade(Reservation);
        if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->RefundPointsOnce(PaymentReceipt);
    }
    FinishAction(); Reservation={}; PaymentReceipt=0; Customer.Reset();
    Presentation->SetPreview(nullptr); SetState(EONEMachineState::Disabled); Presentation->Shutdown();
    UE_LOG(LogTemp,Warning,TEXT("ONE04_MACHINE_TECHNICAL_RECOVERY box=%d"),bIsBox);
}
void AONEProgressionMachine::InvalidateRun()
{
    bInvalidated=true; FinishAction(); Reservation={}; PaymentReceipt=0; Customer.Reset();
    LastLostOwner.Reset(); LastLostRunId=LastLostReceipt=0; LastLostFamily=EONEWeaponFamily::Invalid;
    Presentation->SetPreview(nullptr); SetState(EONEMachineState::Disabled); Presentation->Shutdown();
}
void AONEProgressionMachine::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Reason==EEndPlayReason::Destroyed && !bInvalidated) RecoverTechnicalFailure();
    else InvalidateRun();
    Super::EndPlay(Reason);
}
