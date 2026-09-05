#include "ONEProgressionMachine.h"
#include "ONE04MachinePresentation.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
#include "ONEGameMode.h"
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
    if (Local.X<(bIsBox?60.f:108.f) || FMath::Abs(P->GetActorLocation().Z-Focus.Z)>145.f ||
        FVector::DistSquared2D(P->GetActorLocation(),Focus)>FMath::Square(180.f)) return false;
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(MachineReach),false,P);
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(Hit,P->GetActorLocation()+FVector(0,0,25),Focus,ECC_Visibility,Params);
    return !Blocked || Hit.GetActor()==this;
}
bool AONEProgressionMachine::CanDeposit(AONEPlayer* P,FString& Reason) const
{
    const auto* W=P ? P->GetWeaponComponent() : nullptr;
    const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!CanContact(P)) { Reason=TEXT("Move to the center of the intake tray"); return false; }
    if (!W || !W->HasUsableWeapon()) { Reason=TEXT("Equip an available base weapon"); return false; }
    if (W->IsHandoffLocked()) { Reason=TEXT("Finish the weapon handoff"); return false; }
    if (W->GetDefinition().bUpgraded) { Reason=TEXT("Already upgraded - one tier per weapon"); return false; }
    const auto* S=W->GetSlotState(W->GetEquippedIndex());
    if (W->GetOperation()!=EONEWeaponOperation::Ready || S->bNeedsPump || !S->bMagazinePresent)
    { Reason=TEXT("Ready the weapon before depositing"); return false; }
    if (!GM || GM->IsGameOver() || GM->GetPoints()<UpgradePrice) { Reason=TEXT("Requires 5,000 points"); return false; }
    return true;
}
bool AONEProgressionMachine::CanContact(const AONEPlayer* P) const
{
    if (!CanReach(P)) return false;
    if (bIsBox) return true;
    const FVector Local=Presentation->GetComponentTransform().InverseTransformPosition(P->GetActorLocation());
    return Local.X<=190.f && FMath::Abs(Local.Y)<=55.f;
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
        if (bIsBox)
        {
            O.Detail=TEXT("950 points - roll one weapon");
            O.bEnabled=GM && !GM->IsGameOver() && GM->GetPoints()>=BoxPrice && !W->IsHandoffLocked();
            if (GM && GM->GetPoints()<BoxPrice) O.Detail=TEXT("Requires 950 points");
            if (W->IsHandoffLocked()) O.Detail=TEXT("Finish the weapon handoff");
            bool bEligible=false;
            for (const auto F:{EONEWeaponFamily::Pistol,EONEWeaponFamily::Carbine,EONEWeaponFamily::Shotgun})
                bEligible|=W->IsFamilyRollEligible(F) && RollWeight(F)>0;
            if (!bEligible) { O.bEnabled=false; O.Detail=TEXT("No eligible reward with the configured weights"); }
            if (GM && GM->GetForcedBoxReward()!=EONEWeaponFamily::Invalid && !W->IsFamilyRollEligible(GM->GetForcedBoxReward()))
            { O.bEnabled=false; O.Detail=TEXT("Forced test reward is reserved - V resets next roll to random"); }
        }
        else
        {
            O.bEnabled=CanDeposit(P,O.Detail);
            if (O.bEnabled)
            {
                const auto* Other=W->GetSlotState(1-O.Slot);
                const bool OnlyWeapon=!Other || Other->Status!=EONEWeaponSlotStatus::Available;
                O.Detail=FString::Printf(TEXT("5,000 - upgrade %s; reserve slot %d.%s"),*W->GetDefinition().DisplayName.ToString(),O.Slot+1,
                    OnlyWeapon?TEXT(" You will be UNARMED until you collect or acquire a weapon."):TEXT(""));
            }
        }
    }
    else if (State==EONEMachineState::Ready && IsCurrentOwner() && Customer.Get()==P)
    {
        if (bIsBox)
        {
            O.Action=EONEInteractionAction::CollectBox;
            O.Acquisition=W->BuildAcquisitionPlan(RewardFamily);
            O.bEnabled=O.Acquisition.IsValid();
            const auto* Reward=W->GetCatalogDefinition(RewardFamily);
            const auto* Current=W->GetDefinitionForWeapon(O.Acquisition.Slot);
            const FString Name=Reward ? Reward->DisplayName.ToString() : TEXT("Weapon");
            switch (O.Acquisition.Kind)
            {
                case EONEWeaponAcquisitionKind::FillEmpty: O.Detail=FString::Printf(TEXT("Take %s into empty slot %d"),*Name,O.Acquisition.Slot+1); break;
                case EONEWeaponAcquisitionKind::Replace: O.Detail=FString::Printf(TEXT("Take %s - REPLACE %s in slot %d"),*Name,Current?*Current->DisplayName.ToString():TEXT("weapon"),O.Acquisition.Slot+1); break;
                case EONEWeaponAcquisitionKind::Refill: O.Detail=FString::Printf(TEXT("%s AMMO REFILL - keep %s"),*Name,Current?*Current->DisplayName.ToString():TEXT("owned weapon")); break;
                case EONEWeaponAcquisitionKind::AlreadyFull: O.Detail=FString::Printf(TEXT("%s ammo FULL - collect consumes reward; no extra ammo"),Current?*Current->DisplayName.ToString():*Name); break;
                default: O.Detail=TEXT("Reward waiting - its family is reserved at Pack-a-Punch"); break;
            }
        }
        else
        {
            O.Action=EONEInteractionAction::CollectUpgrade;
            O.bEnabled=!W->IsHandoffLocked() && CanContact(P);
            const auto* D=W->GetCatalogDefinition(RewardFamily,true);
            O.Detail=FString::Printf(TEXT("Take %s - returns to slot %d, fully supplied"),D?*D->DisplayName.ToString():TEXT("upgraded weapon"),Reservation.Slot+1);
            if (!CanContact(P)) O.Detail=TEXT("Weapon ready - move to the center of the output tray");
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
    if (!Current.bEnabled || !Current.SameContext(Offered) || !CanReach(P)) return false;
    auto* W=P->GetWeaponComponent(); auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!GM || GM->IsGameOver()) return false;
    if (Current.Action==EONEInteractionAction::BuyBox)
    {
        TArray<EONEWeaponFamily> Eligible;
        for (const auto F:{EONEWeaponFamily::Pistol,EONEWeaponFamily::Carbine,EONEWeaponFamily::Shotgun})
            if (W->IsFamilyRollEligible(F) && RollWeight(F)>0) Eligible.Add(F);
        if (Eligible.IsEmpty()) return false;
        const uint64 Receipt=GM->NewMachineReceipt();
        if (!GM->TrySpendPoints(BoxPrice,Receipt)) return false;
        PaymentReceipt=Receipt; Customer=P; OwnerRunId=W->GetRunId(); bDelivered=false;
        RollPool=Eligible;
        const EONEWeaponFamily Forced=GM->ConsumeForcedBoxReward();
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
        if (!W->ApplyAcquisitionPlan(Current.Acquisition)) return false;
        bDelivered=true; ++DeliveredCount; Presentation->SetPreview(nullptr); SetState(EONEMachineState::Closing);
        UE_LOG(LogTemp,Display,TEXT("ONE04_BOX_COLLECT receipt=%llu kind=%d slot=%d"),PaymentReceipt,int32(Current.Acquisition.Kind),Current.Acquisition.Slot);
        return true;
    }
    if (Current.Action==EONEInteractionAction::DepositUpgrade)
    {
        Customer=P; OwnerRunId=W->GetRunId(); HandoffSlot=Current.Slot; HandoffInstance=Current.InstanceId;
        RewardFamily=W->GetDefinition().Family; bDelivered=false; PaymentReceipt=0;
        P->BeginMachineAction(RewardFamily,false,GetInteractionPoint()); bOwnsAction=true; HandoffRevision=W->GetInventoryRevision();
        SetState(EONEMachineState::Handoff); return true;
    }
    if (Current.Action==EONEInteractionAction::CollectUpgrade)
    {
        P->BeginMachineAction(RewardFamily,true,GetInteractionPoint()); bOwnsAction=true; P->SuppressCarriedPresentation(true);
        Presentation->BeginRetrievalTo(P->GetWeaponWorldTransform()); bCollectedVisual=false;
        SetState(EONEMachineState::Collecting); return true;
    }
    return false;
}
bool AONEProgressionMachine::IsCurrentOwner() const
{
    const auto* P=Customer.Get();
    return !bInvalidated && P && !P->IsDead() && P->GetWeaponComponent()->GetRunId()==OwnerRunId;
}
void AONEProgressionMachine::AcceptUpgrade()
{
    AONEPlayer* P=Customer.Get(); auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!IsCurrentOwner() || !P || !GM || !CanContact(P)) { CancelUnacceptedAction(P); return; }
    auto* W=P->GetWeaponComponent(); const auto* S=W->GetSlotState(HandoffSlot);
    if (W->GetInventoryRevision()!=HandoffRevision || W->GetEquippedIndex()!=HandoffSlot || !S ||
        S->InstanceId!=HandoffInstance || S->Status!=EONEWeaponSlotStatus::Available || S->bUpgraded || GM->GetPoints()<UpgradePrice)
    { CancelUnacceptedAction(P); return; }
    const FTransform Hand=P->GetWeaponWorldTransform();
    if (!W->ReserveEquippedForUpgrade(Reservation)) { CancelUnacceptedAction(P); return; }
    const uint64 Receipt=GM->NewMachineReceipt();
    if (!GM->TrySpendPoints(UpgradePrice,Receipt))
    { W->RollbackUpgrade(Reservation); Reservation={}; CancelUnacceptedAction(P); return; }
    PaymentReceipt=Receipt; ++AcceptedCount; bOutputVariant=false; ActiveDuration=FMath::Clamp(ProcessingDuration,8.f,10.f);
    P->SuppressCarriedPresentation(true);
    Presentation->SetPreview(W->GetCatalogDefinition(RewardFamily,false)); Presentation->BeginTransferFrom(Hand);
    if (!Presentation->HasCompletePreview()) { RecoverTechnicalFailure(); return; }
    ActionReleaseAt=.24f; SetState(EONEMachineState::Active);
    UE_LOG(LogTemp,Display,TEXT("ONE04_UPGRADE_ACCEPT receipt=%llu run=%llu instance=%llu slot=%d points=%d"),PaymentReceipt,OwnerRunId,Reservation.InstanceId,Reservation.Slot,GM->GetPoints());
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
    Presentation->UpdateVisual(Visual,StateElapsed,State==EONEMachineState::Closing?.85f:ActiveDuration);
}
void AONEProgressionMachine::FinishAction()
{
    if (bOwnsAction)
        if (AONEPlayer* P=Customer.Get()) P->EndMachineAction();
    bOwnsAction=false;
    ActionReleaseAt=0;
}
void AONEProgressionMachine::Tick(float Dt)
{
    CSV_SCOPED_TIMING_STAT(ONEProgression,MachineState);
    Super::Tick(Dt);
    if (State==EONEMachineState::Disabled) return;
    StateElapsed+=Dt;
    if (State!=EONEMachineState::Idle && State!=EONEMachineState::Closing && !IsCurrentOwner()) { InvalidateRun(); return; }
    if (State==EONEMachineState::Handoff)
    {
        if (!CanContact(Customer.Get()) || UGameplayStatics::IsGamePaused(this)) CancelUnacceptedAction(Customer.Get());
        else if (StateElapsed>=.48f) AcceptUpgrade();
    }
    else if (State==EONEMachineState::Active)
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
        AONEPlayer* P=Customer.Get(); auto* W=P->GetWeaponComponent();
        if (!bDelivered && !CanContact(P)) { CancelUnacceptedAction(P); return; }
        Presentation->BeginRetrievalTo(P->GetWeaponWorldTransform());
        if (!bDelivered && StateElapsed>=.18f)
        {
            if (!W->CollectUpgrade(Reservation)) { RecoverTechnicalFailure(); return; }
            bDelivered=true; ++DeliveredCount;
            UE_LOG(LogTemp,Display,TEXT("ONE04_UPGRADE_COLLECT receipt=%llu instance=%llu slot=%d"),PaymentReceipt,Reservation.InstanceId,Reservation.Slot);
        }
        if (bDelivered && !bCollectedVisual && W->GetEquippedIndex()==Reservation.Slot)
        {
            bCollectedVisual=true; Presentation->SetPreview(nullptr); P->SuppressCarriedPresentation(false);
        }
        if (StateElapsed>=.64f && bCollectedVisual) { FinishAction(); SetState(EONEMachineState::Closing); }
    }
    else if (State==EONEMachineState::Closing && StateElapsed>=.85f)
    {
        Customer.Reset(); Reservation={}; PaymentReceipt=0; RewardFamily=EONEWeaponFamily::Invalid; SetState(EONEMachineState::Idle);
    }
    UpdatePresentation();
}
void AONEProgressionMachine::CancelUnacceptedAction(AONEPlayer* P)
{
    if (!P || Customer.Get()!=P) return;
    if (State==EONEMachineState::Handoff)
    { FinishAction(); Customer.Reset(); Reservation={}; PaymentReceipt=0; SetState(EONEMachineState::Idle); }
    else if (State==EONEMachineState::Collecting && !bDelivered)
    {
        FinishAction(); Presentation->SetPreview(nullptr);
        Presentation->SetPreview(P->GetWeaponComponent()->GetCatalogDefinition(RewardFamily,true));
        SetState(EONEMachineState::Ready);
    }
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
    Presentation->SetPreview(nullptr); SetState(EONEMachineState::Disabled); Presentation->Shutdown();
}
void AONEProgressionMachine::EndPlay(const EEndPlayReason::Type Reason)
{
    if (Reason==EEndPlayReason::Destroyed && !bInvalidated) RecoverTechnicalFailure();
    else InvalidateRun();
    Super::EndPlay(Reason);
}
