#include "ONE04ProgressionCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEProgressionMachine.h"
#include "ONE04MachinePresentation.h"
#include "ONEInteractionComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "CoreGlobals.h"

namespace ONE04ProgressionContinuation
{
    bool Pending=false;
    int32 Checks=0,Failures=0;
    double Elapsed=0;
    FONEWeaponReservation OldToken;
    uint64 OldReceipt=0;
    TArray<TSharedPtr<FJsonValue>> Records;
    FString Csv;
}
AONE04ProgressionCheck::AONE04ProgressionCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    PrimaryActorTick.bTickEvenWhenPaused=true;
}
void AONE04ProgressionCheck::BeginPlay()
{
    Super::BeginPlay(); StartReal=StageReal=FPlatformTime::Seconds(); StageStart=GetWorld()->GetTimeSeconds();
    Csv=TEXT("elapsed_seconds,world_seconds,frame,stage,points,run_id,equipped,slot0_status,slot0_instance,slot0_ammo,slot0_reserve,slot1_status,slot1_instance,slot1_ammo,slot1_reserve,box_state,upgrade_state\n");
    if (ONE04ProgressionContinuation::Pending)
    {
        Stage=99; Checks=ONE04ProgressionContinuation::Checks; Failures=ONE04ProgressionContinuation::Failures;
        Records=MoveTemp(ONE04ProgressionContinuation::Records); Csv=MoveTemp(ONE04ProgressionContinuation::Csv);
        ElapsedOffset=ONE04ProgressionContinuation::Elapsed; Token=ONE04ProgressionContinuation::OldToken; Receipt=ONE04ProgressionContinuation::OldReceipt;
        ONE04ProgressionContinuation::Pending=false;
    }
}
void AONE04ProgressionCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    auto R=MakeShared<FJsonObject>(); R->SetNumberField(TEXT("stage"),Stage);
    R->SetNumberField(TEXT("elapsed_seconds"),ElapsedOffset+FPlatformTime::Seconds()-StartReal);
    R->SetBoolField(TEXT("pass"),Pass); R->SetStringField(TEXT("label"),Label);
    Records.Add(MakeShared<FJsonValueObject>(R));
    UE_LOG(LogTemp,Display,TEXT("ONE04_PROGRESSION %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE04ProgressionCheck::Next(int32 NewStage)
{
    Stage=NewStage; StageStart=GetWorld()->GetTimeSeconds(); StageReal=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("ONE04_PROGRESSION_STAGE %d"),Stage);
}
void AONE04ProgressionCheck::Key(const FKey& InKey,EInputEvent Event)
{
    if (auto* PC=Player ? Cast<AONEPlayerController>(Player->GetController()) : nullptr)
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(InKey,Event,Event==IE_Released ? 0.f : 1.f));
}
bool AONE04ProgressionCheck::Approach(AONEProgressionMachine* Machine)
{
    if (!Machine) { Check(false,TEXT("Machine fixture missing")); Finish(); return false; }
    FVector Point=Machine->GetInteractionPoint()+Machine->GetActorForwardVector()*45.f; Point.Z=98.f;
    Player->GetCharacterMovement()->StopMovementImmediately(); Player->SetActorLocation(Point);
    Player->SetAimOverride(true,Machine->GetInteractionPoint());
    const bool Reach=Machine->CanReach(Player);
    Check(Reach,FString::Printf(TEXT("Actual arena %s has a clear forward interaction fixture"),Machine->IsBox()?TEXT("box"):TEXT("upgrader")));
    if (!Reach) Finish();
    return Reach;
}
void AONE04ProgressionCheck::SetPoints(int32 Target)
{
    // Explicit sandbox fixture: use the same grant and receipt-spend APIs as
    // gameplay, never write the score field or alter machine prices.
    for (int32 N=0;N<3 && GM->GetPoints()<Target;++N) GM->GrantSandboxPoints();
    if (GM->GetPoints()>Target) Check(GM->TrySpendPoints(GM->GetPoints()-Target,GM->NewMachineReceipt()),TEXT("Boundary fixture spends surplus through centralized receipt API"));
    Check(GM->GetPoints()==Target,FString::Printf(TEXT("Explicit sandbox point boundary fixture: %d"),Target));
}
void AONE04ProgressionCheck::Trace()
{
    if (!Player || !GM) return;
    auto* W=Player->GetWeaponComponent(); const auto* A=W->GetSlotState(0); const auto* B=W->GetSlotState(1);
    if (!A || !B) return;
    Csv+=FString::Printf(TEXT("%.6f,%.6f,%llu,%d,%d,%llu,%d,%d,%llu,%d,%d,%d,%llu,%d,%d,%d,%d\n"),
        ElapsedOffset+FPlatformTime::Seconds()-StartReal,GetWorld()->GetTimeSeconds(),GFrameCounter,Stage,GM->GetPoints(),W->GetRunId(),W->GetEquippedIndex(),
        int32(A->Status),A->InstanceId,A->Ammo,A->Reserve,int32(B->Status),B->InstanceId,B->Ammo,B->Reserve,
        IsValid(Box)?int32(Box->GetState()):-1,IsValid(Upgrade)?int32(Upgrade->GetState()):-1);
}
void AONE04ProgressionCheck::Finish()
{
    if (bFinished) return;
    Key(EKeys::F,IE_Released); Key(EKeys::W,IE_Released); Key(EKeys::LeftMouseButton,IE_Released);
    if (Player) { Player->ReleaseHeldInputs(); Player->SetAimOverride(false,FVector::ZeroVector); }
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    auto Root=MakeShared<FJsonObject>(); Root->SetStringField(TEXT("candidate"),TEXT("06"));
    Root->SetStringField(TEXT("scope"),TEXT("Candidate06 production PlayerController action-specific tap/hold dispatch; actual machines, ownership, reload exception, automatic return/deadline and centralized receipts. Approach teleports and sandbox point grants/spends are declared fixtures. The legacy ONE04 mode/output name remains for runner compatibility; this is not the preserved Candidate04 result, OS input, art/audio approval or performance proof."));
    Root->SetNumberField(TEXT("checks"),Checks); Root->SetNumberField(TEXT("failures"),Failures); Root->SetArrayField(TEXT("assertions"),Records);
    Root->SetBoolField(TEXT("real_level_restart"),Stage==99);
    FString Json; FJsonSerializer::Serialize(Root,TJsonWriterFactory<>::Create(&Json));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate04/Progression"); IFileManager::Get().MakeDirectory(*Folder,true);
    if (!FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("checks.json"))) || !FFileHelper::SaveStringToFile(Csv,*(Folder/TEXT("timeline.csv")))) ++Failures;
    UE_LOG(LogTemp,Display,TEXT("ONE04_PROGRESSION_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
void AONE04ProgressionCheck::Tick(float Dt)
{
    Super::Tick(Dt); const double Now=FPlatformTime::Seconds();
    if (bFinished) { if (Now-FinishedReal>.4) FPlatformMisc::RequestExit(false); return; }
    if (Now-StartReal>175 || Now-StageReal>18) { Check(false,FString::Printf(TEXT("Bounded progression timeout stage %d"),Stage)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!GM) GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !GM || !Player->GetController()) return;
    auto* W=Player->GetWeaponComponent();
    if (Stage!=90 && Stage!=98 && Stage!=99) Player->Health->Restore();
    const float T=GetWorld()->GetTimeSeconds()-StageStart;
    const double RealT=Now-StageReal;
    if (GetWorld()->GetTimeSeconds()-LastTrace>=.1f) { LastTrace=GetWorld()->GetTimeSeconds(); Trace(); }
    switch (Stage)
    {
    case 0: if (T>.7f)
    {
        for (TActorIterator<AONEMysteryBox> It(GetWorld());It;++It) { Box=*It; break; }
        for (TActorIterator<AONEUpgradeMachine> It(GetWorld());It;++It) { Upgrade=*It; break; }
        Check(GM->IsSandbox(),TEXT("Opt-in progression fixture runs in explicitly enabled sandbox"));
        Check(W->GetWeaponCount()==2 && W->GetCatalogCount()==6 && W->GetEquippedIndex()==0 && W->GetDefinition().Id==TEXT("P1911") && W->GetAmmo()==7 && W->GetReserveAmmo()==56 && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Empty,TEXT("Genuine fresh startup is M1911 seven/fifty-six with empty second slot"));
        Check(IsValid(Upgrade),TEXT("Actual arena contains upgrade machine"));
        if (!Approach(Box) || !IsValid(Upgrade)) { Finish(); break; }
        UpgradeTransform=Upgrade->GetActorTransform(); SetPoints(949); Key(EKeys::F,IE_Pressed); Next(1);
    } break;
    case 1: if (T>.6f)
    {
        Check(Box->GetAcceptedCount()==0 && GM->GetPoints()==949 && Box->GetState()==EONEMachineState::Idle,TEXT("Held production F cannot buy box at949"));
        Key(EKeys::F,IE_Released); SetPoints(950); Next(80);
    } break;
    case 2: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(3); } break;
    case 3: if (T>.15f) { Key(EKeys::F,IE_Released); Next(4); } break;
    case 4: if (T>.5f)
    {
        Check(Box->GetAcceptedCount()==0 && GM->GetPoints()==950,TEXT("Released incomplete hold leaves inventory and points unchanged"));
        Player->SetActorLocation(Player->GetActorLocation()+Box->GetActorForwardVector()*500.f); Key(EKeys::F,IE_Pressed); Next(5);
    } break;
    case 5: if (T>.6f)
    {
        Check(Box->GetAcceptedCount()==0 && GM->GetPoints()==950,TEXT("Out-of-range production hold cannot spend"));
        Key(EKeys::F,IE_Released); if (!Approach(Box)) break;
        GM->SetForcedBoxReward(EONEWeaponFamily::Carbine); Next(6);
    } break;
    case 6: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(7); } break;
    case 7: if (Box->GetState()==EONEMachineState::Ready && Box->GetStateElapsed()>1.f)
    {
        Check(Box->GetAcceptedCount()==1 && Box->GetDeliveredCount()==0 && GM->GetPoints()==0 && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Empty,TEXT("950 spent once; uninterrupted F cannot buy again or collect after five-second reveal"));
        Check(Box->GetRewardFamily()==EONEWeaponFamily::Carbine,TEXT("Chosen forced test result remains the revealed family"));
        Key(EKeys::F,IE_Released); Next(8);
    } break;
    case 8: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(9); } break;
    case 9: if (T>.95f)
    {
        Check(Box->GetDeliveredCount()==1 && W->GetEquippedIndex()==1 && W->GetSlotState(0)->Family==EONEWeaponFamily::Pistol && W->GetSlotState(1)->Family==EONEWeaponFamily::Carbine && GM->GetPoints()==0,TEXT("Fresh hold collects actual M4A1 into empty slot1 without replacing M1911 or charging again"));
        Key(EKeys::F,IE_Released); Next(10);
    } break;
    case 10: if (Box->GetState()==EONEMachineState::Idle && T>.1f)
    { SetPoints(950); GM->SetForcedBoxReward(EONEWeaponFamily::Shotgun); Key(EKeys::F,IE_Pressed); Next(11); } break;
    case 11: if (Box->GetState()==EONEMachineState::Ready)
    { Key(EKeys::F,IE_Released); Next(12); } break;
    case 12: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(13); } break;
    case 13: if (T>.12f) { Key(EKeys::One,IE_Pressed); Next(14); } break;
    case 14: if (T>.06f) { Key(EKeys::One,IE_Released); Next(15); } break;
    case 15: if (T>.65f)
    {
        Check(Box->GetDeliveredCount()==1 && W->GetEquippedIndex()==0 && Box->GetState()==EONEMachineState::Ready,TEXT("Switching slots during F hold cancels the stale replacement plan"));
        const auto Offer=Box->BuildOffer(Player);
        Check(Offer.Acquisition.Kind==EONEWeaponAcquisitionKind::Replace && Offer.Acquisition.Slot==0 && Offer.Acquisition.ExpectedInstanceId==W->GetSlotState(0)->InstanceId,TEXT("Refreshed prompt fingerprints the actually selected replacement instance"));
        Key(EKeys::F,IE_Released); Next(16);
    } break;
    case 16: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(17); } break;
    case 17: if (T>.95f)
    {
        Check(Box->GetDeliveredCount()==2 && W->GetSlotState(0)->Family==EONEWeaponFamily::Shotgun && W->GetSlotState(1)->Family==EONEWeaponFamily::Carbine,TEXT("Revalidated collection replaces exactly slot0 with Remington870 and preserves slot1"));
        Key(EKeys::F,IE_Released); W->ResetStarterLoadout();
        Check(W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Empty,TEXT("Declared single-weapon fixture resets starter via production inventory API"));
        if (!Approach(Upgrade)) break; SetPoints(4999); Next(18);
    } break;
    case 18: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(19); } break;
    case 19: if (T>.6f)
    {
        Check(Upgrade->GetState()==EONEMachineState::Idle && Upgrade->GetAcceptedCount()==0 && GM->GetPoints()==4999,TEXT("4999 cannot start or pay for upgrade"));
        Key(EKeys::F,IE_Released); SetPoints(5000); Instance=W->GetSlotState(0)->InstanceId; Next(20);
    } break;
    case 20: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(21); } break;
    case 21: if (T>.03f)
    {
        Check(Upgrade->GetState()==EONEMachineState::Active && Upgrade->GetAcceptedCount()==1 && GM->GetPoints()==0 &&
            W->GetSlotState(0)->Status==EONEWeaponSlotStatus::MachineReserved,
            TEXT("One short F press atomically charges and reserves before the hold threshold"));
        Player->SetActorLocation(Player->GetActorLocation()+Upgrade->GetActorForwardVector()*500.f); Key(EKeys::F,IE_Released); Next(22);
    } break;
    case 22: if (T>.6f)
    {
        Check(Upgrade->GetState()==EONEMachineState::Active && Upgrade->GetAcceptedCount()==1 && GM->GetPoints()==0 &&
            !W->HasUsableWeapon() && W->GetSlotState(0)->InstanceId==Instance,
            TEXT("Leaving after accepted tap does not undo payment or lose the exact reserved instance"));
        if (!Approach(Upgrade)) break; Next(23);
    } break;
    case 23: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        AcceptedAt=GetWorld()->GetTimeSeconds()-Upgrade->GetStateElapsed(); Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt();
        Check(GM->GetPoints()==0 && Upgrade->GetAcceptedCount()==1 && Token.Slot==0 && Token.InstanceId==Instance && !W->HasUsableWeapon() && W->GetSlotState(0)->Status==EONEWeaponSlotStatus::MachineReserved,TEXT("5000 acceptance reserves exact original slot/instance and deliberately leaves only-gun player unarmed"));
        Check(!GM->TrySpendPoints(5000,Receipt) && GM->GetPoints()==0,TEXT("Accepted receipt cannot charge twice or make points negative"));
        const auto Before=*W->GetSlotState(0); W->RefillAllAmmo(); W->GrantRoundAmmo();
        Check(W->GetSlotState(0)->Ammo==Before.Ammo && W->GetSlotState(0)->Reserve==Before.Reserve && !W->SelectWeapon(0),TEXT("Developer refill, round reward and slot selection cannot bypass reservation"));
        Check(!W->IsFamilyRollEligible(EONEWeaponFamily::Pistol),TEXT("Reserved family is excluded from a new normal box roll"));
        Next(24);
    } break;
    case 24: if (T>.35f)
    {
        Check(!W->IsHandoffLocked(),TEXT("Brief handoff releases long before nine-second processing ends"));
        MoveOrigin=Player->GetActorLocation(); Key(EKeys::F,IE_Released); Key(EKeys::W,IE_Pressed); Next(25);
    } break;
    case 25: if (T>.3f)
    {
        Key(EKeys::W,IE_Released);
        Check(!W->HasUsableWeapon() && FVector::Dist2D(Player->GetActorLocation(),MoveOrigin)>15.f,TEXT("Unarmed player can move through production W binding during processing"));
        if (!Approach(Box)) break; SetPoints(950); GM->SetForcedBoxReward(EONEWeaponFamily::Carbine); Next(26);
    } break;
    case 26: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(27); } break;
    case 27: if (Box->GetState()==EONEMachineState::Ready)
    {
        Check(Upgrade->GetState()==EONEMachineState::Active && W->GetSlotState(0)->Status==EONEWeaponSlotStatus::MachineReserved,TEXT("Independent box finishes while original weapon is still processing after walking away"));
        Key(EKeys::F,IE_Released); Next(28);
    } break;
    case 28: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(29); } break;
    case 29: if (T>.95f)
    {
        Check(W->GetEquippedIndex()==1 && W->GetDefinition().Family==EONEWeaponFamily::Carbine && W->GetSlotState(0)->InstanceId==Instance && W->GetSlotState(0)->Status!=EONEWeaponSlotStatus::Available,TEXT("Box fills genuine slot1 while slot0 remains owned and unavailable"));
        Key(EKeys::F,IE_Released); Next(30);
    } break;
    case 30: if (Upgrade->GetState()==EONEMachineState::Ready)
    {
        const float Processing=GetWorld()->GetTimeSeconds()-AcceptedAt-Upgrade->GetStateElapsed();
        Check(FMath::Abs(Processing-9.f)<.15f,FString::Printf(TEXT("Actual processing clock matches nine seconds: %.4f"),Processing));
        Next(31);
    } break;
    case 31: if (T>2.f)
    {
        Check(Upgrade->GetState()==EONEMachineState::Ready && Upgrade->GetDeliveredCount()==0 &&
            Upgrade->GetReadySecondsRemaining()>12.7f && Upgrade->GetReadySecondsRemaining()<13.1f &&
            W->GetSlotState(0)->Status==EONEWeaponSlotStatus::ReadyToCollect && W->GetEquippedIndex()==1,
            TEXT("Ready deadline starts at completion; after two seconds away about thirteen remain"));
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(0,1500,20));
        ShotCount=W->GetTotalShotsFired(); Key(EKeys::LeftMouseButton,IE_Pressed); Next(70);
    } break;
    case 32: if (T>1.2f)
    {
        Check(Upgrade->GetDeliveredCount()==1 && W->GetEquippedIndex()==0 && W->GetSlotState(0)->InstanceId==Instance && W->GetDefinition().Id==TEXT("P1911_UP") && W->GetAmmo()==14 && W->GetReserveAmmo()==168 && GM->GetPoints()==0,TEXT("Automatic ownership return preserves Last Word slot/instance and refills once; later deliberate selection works"));
        Key(EKeys::F,IE_Released); ShotCount=W->GetTotalShotsFired();
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(0,1500,20)); Key(EKeys::LeftMouseButton,IE_Pressed); Next(33);
    } break;
    case 33: if (T>.08f) { Key(EKeys::LeftMouseButton,IE_Released); Next(34); } break;
    case 34: if (T>.45f)
    {
        Check(W->GetTotalShotsFired()==ShotCount+1 && W->GetAmmo()==13,TEXT("Collected Last Word fires a real semiautomatic discharge"));
        W->AddReserveAmmo(-10); if (!Approach(Box)) break; SetPoints(950); GM->SetForcedBoxReward(EONEWeaponFamily::Pistol); Next(35);
    } break;
    case 35: if (Box->GetState()==EONEMachineState::Idle && T>.1f) { Key(EKeys::F,IE_Pressed); Next(36); } break;
    case 36: if (T>.6f)
    {
        const auto Offer=Box->BuildOffer(Player);
        Check(Box->GetState()==EONEMachineState::Idle && GM->GetPoints()==950 && !Offer.bEnabled &&
            Offer.Detail.Contains(TEXT("Forced test reward")) && !W->IsFamilyRollEligible(EONEWeaponFamily::Pistol),
            TEXT("Forced base pistol is rejected while Last Word owns that family; no payment or refill"));
        GM->SetForcedBoxReward(EONEWeaponFamily::Invalid); Box->RollWeights=FVector(1,0,0);
        Key(EKeys::F,IE_Released); Next(37);
    } break;
    case 37: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(38); } break;
    case 38: if (T>.95f)
    {
        Check(Box->GetState()==EONEMachineState::Idle && GM->GetPoints()==950 && !Box->BuildOffer(Player).bEnabled &&
            Box->BuildOffer(Player).Detail.Contains(TEXT("All available weapon types")),
            TEXT("Constrained pistol-only catalog has no eligible reward and charges nothing with two runtime slots"));
        Check(W->GetSlotState(0)->InstanceId==Instance && W->GetSlotState(0)->bUpgraded && W->GetAmmoForWeapon(0)==13 &&
            W->GetReserveAmmoForWeapon(0)==158 && W->GetSlotState(1)->Family==EONEWeaponFamily::Carbine,
            TEXT("Rejected owned-family rolls neither refill nor downgrade or duplicate ownership"));
        Box->RollWeights=FVector(1,1,1);
        Key(EKeys::F,IE_Released); GM->SetForcedBoxReward(EONEWeaponFamily::Shotgun);
        Key(EKeys::Two,IE_Pressed); Key(EKeys::Two,IE_Released); Next(380);
    } break;
    case 380: if (T>.65f) { Key(EKeys::F,IE_Pressed); Next(381); } break;
    case 381: if (Box->GetState()==EONEMachineState::Ready)
    {
        Key(EKeys::F,IE_Released); Receipt=Box->GetPaymentReceipt();
        Check(Box->GetRewardFamily()==EONEWeaponFamily::Shotgun && GM->GetPoints()==0,
            TEXT("Race fixture pays once for a fixed initially unowned shotgun result"));
        const auto Plan=W->BuildAcquisitionPlan(EONEWeaponFamily::Shotgun);
        Check(Plan.Slot==1 && W->ApplyAcquisitionPlan(Plan),
            TEXT("Declared developer race acquires the paid family through production inventory API before Box collection"));
        Next(382);
    } break;
    case 382: if (T>.2f)
    {
        Check(Box->GetInvalidBoxDeliveryCount()==1 && GM->GetPoints()==950 &&
            W->GetSlotState(1)->Family==EONEWeaponFamily::Shotgun && !GM->RefundPointsOnce(Receipt),
            TEXT("Owned-family race refunds failed delivery once, without reroll or duplicate acquisition")); Next(383);
    } break;
    case 383: if (T>.65f)
    {
        const auto Plan=W->BuildAcquisitionPlan(EONEWeaponFamily::Carbine);
        Check(Plan.Slot==1 && W->ApplyAcquisitionPlan(Plan),TEXT("Declared race-fixture cleanup restores base rifle in same available slot")); Next(384);
    } break;
    case 384: if (T>.65f) { Key(EKeys::One,IE_Pressed); Key(EKeys::One,IE_Released); Next(385); } break;
    case 385: if (T>.65f) { if (!Approach(Upgrade)) break; SetPoints(5000); Next(39); } break;
    case 39: if (Upgrade->GetState()==EONEMachineState::Idle && T>.1f)
    {
        Check(!Upgrade->BuildOffer(Player).bEnabled && Upgrade->BuildOffer(Player).Detail.Contains(TEXT("Already upgraded")),TEXT("Already-upgraded effective weapon cannot be charged for another tier"));
        Key(EKeys::Two,IE_Pressed); Next(40);
    } break;
    case 40: if (T>.06f) { Key(EKeys::Two,IE_Released); Next(41); } break;
    case 41: if (T>.55f)
    {
        Instance=W->GetSlotState(1)->InstanceId;
        Check(W->GetEquippedIndex()==1 && !W->GetDefinition().bUpgraded,TEXT("Independent M4A1 instance remains base after pistol upgrade"));
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(0,1500,20));
        Key(EKeys::LeftMouseButton,IE_Pressed); Next(410);
    } break;
    case 410: if (T>.04f) { Key(EKeys::LeftMouseButton,IE_Released); Next(411); } break;
    case 411: if (T>.2f) { Key(EKeys::R,IE_Pressed); Key(EKeys::R,IE_Released); Next(412); } break;
    case 412: if (T>.65f)
    {
        const auto* S=W->GetSlotState(1);
        Check(W->IsMagazineReloadCommitted() && !S->bMagazinePresent && S->Ammo<24,
            TEXT("Deposit fixture is a real partial-ammo reload after magazine removal but before insertion"));
        Check(Upgrade->BuildOffer(Player).bEnabled && Upgrade->BuildOffer(Player).Input==EONEInteractionInput::Tap,
            TEXT("Tap deposit remains available during a committed magazine reload"));
        Key(EKeys::F,IE_Pressed); Key(EKeys::F,IE_Released); Next(42);
    } break;
    case 42: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt();
        Check(GM->GetPoints()==0 && Token.InstanceId==Instance && Token.Slot==1 &&
            !Token.Before.bMagazinePresent && Token.Before.Ammo<24,
            TEXT("Reload-transfer exception snapshots the exact partial-ammo M4A1 without granting insertion ammunition"));
        Key(EKeys::F,IE_Released); Upgrade->Destroy(); Upgrade=nullptr; Next(43);
    } break;
    case 43: if (T>.3f)
    {
        Check(GM->GetPoints()==5000 && W->GetSlotState(1)->InstanceId==Instance && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Available && !W->GetSlotState(1)->bUpgraded,TEXT("Destroyed processing machine restores original weapon and refunds exactly once"));
        Check(W->GetAmmoForWeapon(1)==Token.Before.Ammo && W->GetReserveAmmoForWeapon(1)==Token.Before.Reserve,
            TEXT("Technical rollback preserves earned partial reload ammunition without a free refill"));
        Check(!GM->RefundPointsOnce(Receipt) && GM->GetPoints()==5000 && !W->CollectUpgrade(Token),TEXT("Technical recovery receipt/token cannot replay refund or delivery"));
        FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Upgrade=GetWorld()->SpawnActor<AONEUpgradeMachine>(AONEUpgradeMachine::StaticClass(),UpgradeTransform,Spawn);
        Check(IsValid(Upgrade),TEXT("Declared replacement machine fixture uses original transform after destruction test"));
        W->ResetStarterLoadout(); if (!Approach(Upgrade)) break;
        W->AddReserveAmmo(-56); Count=0; Player->SetAimOverride(true,Player->GetActorLocation()+FVector(0,1500,20)); Next(430);
    } break;
    case 430: if (T>.28f) { Key(EKeys::LeftMouseButton,IE_Pressed); Next(431); } break;
    case 431: if (T>.03f)
    {
        Key(EKeys::LeftMouseButton,IE_Released); ++Count;
        if (Count<7) Next(430); else Next(432);
    } break;
    case 432: if (T>.3f)
    {
        Check(W->GetAmmo()==0 && W->GetReserveAmmo()==0 && Upgrade->BuildOffer(Player).bEnabled,
            TEXT("A genuinely exhausted pistol remains eligible for tap deposit with both ammo pools empty")); Next(44);
    } break;
    case 44: if (T>.2f) { Key(EKeys::F,IE_Pressed); Next(45); } break;
    case 45: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt(); Key(EKeys::F,IE_Released);
        Check(Token.IsValid() && GM->GetPoints()==0,TEXT("Expiry fixture has a real charged exact-instance reservation"));
        Player->SetActorLocation(Player->GetActorLocation()+Upgrade->GetActorForwardVector()*500.f); Next(46);
    } break;
    case 46: if (Upgrade->GetState()==EONEMachineState::Ready)
    {
        Check(Upgrade->GetReadySecondsRemaining()>14.8f && !Upgrade->IsExpiryWarning(),TEXT("Fifteen-second deadline begins only when the paid upgrade becomes ready"));
        AcceptedAt=Upgrade->GetReadySecondsRemaining(); Key(EKeys::Escape,IE_Pressed); Next(460);
    } break;
    case 460: if (RealT>.12) { Key(EKeys::Escape,IE_Released); Next(461); } break;
    case 461: if (RealT>.35)
    {
        Check(UGameplayStatics::IsGamePaused(this) && FMath::IsNearlyEqual(Upgrade->GetReadySecondsRemaining(),AcceptedAt,.03f),
            TEXT("Production pause freezes the ready/loss countdown")); Key(EKeys::Escape,IE_Pressed); Next(462);
    } break;
    case 462: if (RealT>.12) { Key(EKeys::Escape,IE_Released); Next(47); } break;
    case 47: if (T>14.6f)
    {
        Check(Upgrade->GetState()==EONEMachineState::Ready && Upgrade->IsExpiryWarning() &&
            W->GetSlotState(Token.Slot)->Status==EONEWeaponSlotStatus::ReadyToCollect,
            TEXT("Weapon remains reserved with warning before its actual fifteen-second deadline")); Next(48);
    } break;
    case 48: if (T>.7f)
    {
        Check(Upgrade->GetExpiredCount()==1 && Upgrade->GetDeliveredCount()==0 && GM->GetPoints()==0 &&
            W->GetSlotState(Token.Slot)->Status==EONEWeaponSlotStatus::Empty && !W->HasUsableWeapon() &&
            W->IsFamilyRollEligible(EONEWeaponFamily::Pistol),TEXT("Expired upgrade permanently removes exact instance and re-enables its family without refund"));
        Check(!GM->RefundPointsOnce(Receipt) && !W->ExpireUpgrade(Token) && !W->CollectUpgrade(Token) &&
            Upgrade->WasLastLossFor(Player),TEXT("Intentional expiry closes receipt/token exactly once and emits owner/run-bound loss"));
        W->ResetStarterLoadout(); SetPoints(5000); Next(49);
    } break;
    case 49: if (Upgrade->GetState()==EONEMachineState::Idle)
    { Check(!Upgrade->WasLastLossFor(Player),TEXT("Old loss notification cannot cross inventory run reset")); if (!Approach(Upgrade)) break; Key(EKeys::F,IE_Pressed); Next(50); } break;
    case 50: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt(); Key(EKeys::F,IE_Released);
        AcceptedAt=GetWorld()->GetTimeSeconds()-Upgrade->GetStateElapsed(); Next(501);
    } break;
    case 501: if (Upgrade->GetDeliveredCount()==1)
    {
        Check(GetWorld()->GetTimeSeconds()-AcceptedAt>=8.98f && GetWorld()->GetTimeSeconds()-AcceptedAt<9.15f &&
            W->GetSlotState(Token.Slot)->InstanceId==Token.InstanceId && W->GetSlotState(Token.Slot)->bUpgraded &&
            W->GetSlotState(Token.Slot)->Status==EONEWeaponSlotStatus::Available && Upgrade->GetReadySecondsRemaining()==0,
            TEXT("Owner standing in range receives exact-slot upgrade at nine seconds without F or exit/re-entry"));
        Check(!GM->RefundPointsOnce(Receipt),TEXT("Successful automatic return closes its nonrefundable receipt")); Next(502);
    } break;
    case 502: if (Upgrade->GetState()==EONEMachineState::Idle && T>.9f)
    {
        W->ResetStarterLoadout(); SetPoints(5000); if (!Approach(Upgrade)) break;
        Key(EKeys::F,IE_Pressed); Key(EKeys::F,IE_Released); Next(503);
    } break;
    case 503: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt();
        Check(Token.IsValid() && GM->GetPoints()==0,TEXT("Restart fixture has a fresh charged in-flight upgrade"));
        Player->ReceiveAttack(1000,Player->GetActorLocation()+FVector(100,0,0)); Next(90);
    } break;
    case 70: if (T>.18f)
    {
        Key(EKeys::LeftMouseButton,IE_Released); Key(EKeys::R,IE_Pressed); Key(EKeys::R,IE_Released);
        // InputKey dispatch is consumed by PlayerInput on the next frame.
        // Observe the real operation before entering automatic return reach.
        Next(700);
    } break;
    case 700:
        if (T>.05f && W->IsMagazineReloadCommitted())
        {
            Check(W->GetEquippedIndex()==1 && W->GetTotalShotsFired()>ShotCount && W->GetAmmo()<W->GetDefinition().Capacity &&
                Upgrade->GetState()==EONEMachineState::Ready && !Upgrade->CanReach(Player),
                TEXT("Other-rifle committed reload is active before approaching ready output"));
            OtherReloadStart=GetWorld()->GetTimeSeconds()-W->GetReloadElapsed();
            if (!Approach(Upgrade)) break; Next(71);
        }
        else if (T>.6f) { Check(false,TEXT("Production R did not begin the other-rifle reload before ready approach")); Finish(); }
        break;
    case 71: if (T>.05f)
    {
        Check(Upgrade->GetDeliveredCount()==1 && W->GetSlotState(0)->Status==EONEWeaponSlotStatus::Available &&
            W->GetSlotState(0)->InstanceId==Instance && W->GetSlotState(0)->bUpgraded && W->IsMagazineReloadCommitted() &&
            W->GetEquippedIndex()==1 && !W->IsHandoffLocked() &&
            FMath::Abs((GetWorld()->GetTimeSeconds()-W->GetReloadElapsed())-OtherReloadStart)<.01f,
            TEXT("No-F approach restores ownership immediately without interrupting the other committed reload")); Next(72);
    } break;
    case 72: if (T>.2f && !W->IsMagazineReloadCommitted()) { Key(EKeys::One,IE_Pressed); Key(EKeys::One,IE_Released); Next(32); } break;
    case 80: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(81); } break;
    case 81: if (T>.15f) { Key(EKeys::Escape,IE_Pressed); Next(82); } break;
    case 82: if (RealT>.12)
    {
        Key(EKeys::Escape,IE_Released); Key(EKeys::F,IE_Released);
        Check(UGameplayStatics::IsGamePaused(this) && Box->GetAcceptedCount()==0 && GM->GetPoints()==950,TEXT("Production Escape pauses and cancels an unfinished F hold before payment"));
        Next(83);
    } break;
    case 83: if (RealT>.25) { Key(EKeys::Escape,IE_Pressed); Next(84); } break;
    case 84: if (RealT>.12)
    {
        Key(EKeys::Escape,IE_Released);
        Check(!UGameplayStatics::IsGamePaused(this) && Box->GetAcceptedCount()==0 && GM->GetPoints()==950,TEXT("Resuming cannot complete a stale interaction or charge")); Next(2);
    } break;
    case 90: if (T>.3f)
    {
        Check(Player->IsDead() && GM->IsGameOver() && !W->MarkUpgradeReady(Token) && !W->CollectUpgrade(Token),TEXT("Death invalidates an accepted in-flight upgrade and prevents delayed ready/delivery"));
        ONE04ProgressionContinuation::Pending=true; ONE04ProgressionContinuation::Checks=Checks; ONE04ProgressionContinuation::Failures=Failures;
        ONE04ProgressionContinuation::Records=Records; ONE04ProgressionContinuation::Csv=Csv;
        ONE04ProgressionContinuation::Elapsed=ElapsedOffset+Now-StartReal; ONE04ProgressionContinuation::OldToken=Token; ONE04ProgressionContinuation::OldReceipt=Receipt;
        GM->RestartScene(); Next(98);
    } break;
    case 99: if (T>.8f)
    {
        Check(GM->IsSandbox() && !Player->IsDead() && GM->GetPoints()==0 && W->GetEquippedIndex()==0 && W->GetDefinition().Id==TEXT("P1911") && W->GetAmmo()==7 && W->GetReserveAmmo()==56 && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Empty,TEXT("Real level restart restores fresh M1911/empty inventory and zero earned/test balance"));
        Check(W->GetRunId()!=Token.RunId && W->GetSlotState(0)->InstanceId!=Token.InstanceId && !W->MarkUpgradeReady(Token) && !W->CollectUpgrade(Token) && !W->RollbackUpgrade(Token),TEXT("Prior-run instance token cannot ready, deliver or restore into restarted run"));
        Check(!GM->RefundPointsOnce(Receipt) && GM->GetPoints()==0,TEXT("Prior-run receipt cannot refund into fresh run"));
        int32 Ready=0,Machines=0; for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It) { ++Machines; if (It->GetState()!=EONEMachineState::Idle) ++Ready; }
        Check(Machines==2 && Ready==0,TEXT("New level contains both idle machines and no ghost processing or ready output")); Finish();
    } break;
    default: break;
    }
}
