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
    auto Root=MakeShared<FJsonObject>(); Root->SetStringField(TEXT("candidate"),TEXT("04"));
    Root->SetStringField(TEXT("scope"),TEXT("Production PlayerController InputKey dispatch with frame-spaced holds; actual machine actors, operation timers, inventory and centralized points. Forward approach teleports and explicit sandbox point grants/spends are declared fixtures. This is not OS input, motion/art approval, audio audition or performance proof."));
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
    if (Now-StartReal>135 || Now-StageReal>18) { Check(false,FString::Printf(TEXT("Bounded progression timeout stage %d"),Stage)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!GM) GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !GM || !Player->GetController()) return;
    auto* W=Player->GetWeaponComponent();
    if (Stage<90) Player->Health->Restore();
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
    case 21: if (Upgrade->GetState()==EONEMachineState::Handoff)
    {
        Player->SetActorLocation(Player->GetActorLocation()+Upgrade->GetActorForwardVector()*500.f); Key(EKeys::F,IE_Released); Next(22);
    } break;
    case 22: if (T>.6f)
    {
        Check(Upgrade->GetState()==EONEMachineState::Idle && Upgrade->GetAcceptedCount()==0 && GM->GetPoints()==5000 && W->HasUsableWeapon() && W->GetSlotState(0)->InstanceId==Instance,TEXT("Leaving during preaccept handoff restores usable original gun with no charge"));
        if (!Approach(Upgrade)) break; Key(EKeys::F,IE_Pressed); Next(23);
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
        Check(Upgrade->GetState()==EONEMachineState::Ready && Upgrade->GetDeliveredCount()==0 && W->GetSlotState(0)->Status==EONEWeaponSlotStatus::ReadyToCollect && W->GetEquippedIndex()==1,TEXT("Ready output waits without auto-delivery or expiry; other weapon stays available"));
        if (!Approach(Upgrade)) break; Key(EKeys::F,IE_Pressed); Next(70);
    } break;
    case 32: if (T>1.2f)
    {
        Check(Upgrade->GetDeliveredCount()==1 && W->GetEquippedIndex()==0 && W->GetSlotState(0)->InstanceId==Instance && W->GetDefinition().Id==TEXT("P1911_UP") && W->GetAmmo()==14 && W->GetReserveAmmo()==168 && GM->GetPoints()==0,TEXT("Fresh F retrieval returns Last Word into same slot/instance and refills its effective capacities exactly once"));
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
    case 36: if (Box->GetState()==EONEMachineState::Ready)
    {
        const auto Offer=Box->BuildOffer(Player);
        Check(Offer.Acquisition.Kind==EONEWeaponAcquisitionKind::Refill && Offer.Acquisition.Slot==0 && Offer.Detail.Contains(TEXT("REFILL")),TEXT("Duplicate base pistol reward is visibly an ammo refill for owned Last Word"));
        Key(EKeys::F,IE_Released); Next(37);
    } break;
    case 37: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(38); } break;
    case 38: if (T>.95f)
    {
        Check(W->GetSlotState(0)->InstanceId==Instance && W->GetSlotState(0)->bUpgraded && W->GetAmmoForWeapon(0)==14 && W->GetReserveAmmoForWeapon(0)==168 && W->GetSlotState(1)->Family==EONEWeaponFamily::Carbine,TEXT("Duplicate refill neither downgrades nor duplicates upgraded ownership"));
        Key(EKeys::F,IE_Released); if (!Approach(Upgrade)) break; SetPoints(5000); Next(39);
    } break;
    case 39: if (Upgrade->GetState()==EONEMachineState::Idle && T>.1f)
    {
        Check(!Upgrade->BuildOffer(Player).bEnabled && Upgrade->BuildOffer(Player).Detail.Contains(TEXT("Already upgraded")),TEXT("Already-upgraded effective weapon cannot be charged for another tier"));
        Key(EKeys::Two,IE_Pressed); Next(40);
    } break;
    case 40: if (T>.06f) { Key(EKeys::Two,IE_Released); Next(41); } break;
    case 41: if (T>.55f)
    { Instance=W->GetSlotState(1)->InstanceId; Check(W->GetEquippedIndex()==1 && !W->GetDefinition().bUpgraded,TEXT("Independent M4A1 instance remains base after pistol upgrade")); Key(EKeys::F,IE_Pressed); Next(42); } break;
    case 42: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt();
        Check(GM->GetPoints()==0 && Token.InstanceId==Instance && Token.Slot==1,TEXT("Second accepted purchase binds the exact M4A1 slot1 instance"));
        Key(EKeys::F,IE_Released); Upgrade->Destroy(); Upgrade=nullptr; Next(43);
    } break;
    case 43: if (T>.3f)
    {
        Check(GM->GetPoints()==5000 && W->GetSlotState(1)->InstanceId==Instance && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Available && !W->GetSlotState(1)->bUpgraded,TEXT("Destroyed processing machine restores original weapon and refunds exactly once"));
        Check(!GM->RefundPointsOnce(Receipt) && GM->GetPoints()==5000 && !W->CollectUpgrade(Token),TEXT("Technical recovery receipt/token cannot replay refund or delivery"));
        FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Upgrade=GetWorld()->SpawnActor<AONEUpgradeMachine>(AONEUpgradeMachine::StaticClass(),UpgradeTransform,Spawn);
        Check(IsValid(Upgrade),TEXT("Declared replacement machine fixture uses original transform after destruction test"));
        W->ResetStarterLoadout(); if (!Approach(Upgrade)) break; Next(44);
    } break;
    case 44: if (T>.2f) { Key(EKeys::F,IE_Pressed); Next(45); } break;
    case 45: if (Upgrade->GetState()==EONEMachineState::Active)
    {
        Token=Upgrade->GetReservation(); Receipt=Upgrade->GetPaymentReceipt(); Key(EKeys::F,IE_Released);
        Check(Token.IsValid() && GM->GetPoints()==0,TEXT("Restart fixture has a real charged in-flight upgrade"));
        Player->ReceiveAttack(1000,Player->GetActorLocation()+FVector(100,0,0)); Next(90);
    } break;
    case 70: if (Upgrade->GetState()==EONEMachineState::Collecting)
    {
        Check(Upgrade->GetDeliveredCount()==0 && Upgrade->GetStateElapsed()<.18f,TEXT("Retrieval cancellation fixture occurs before actual take event"));
        Key(EKeys::F,IE_Released); Player->SetActorLocation(Player->GetActorLocation()+Upgrade->GetActorForwardVector()*300.f); Next(71);
    } break;
    case 71: if (T>.35f)
    {
        auto* Visual=Upgrade->GetPresentation();
        Check(Upgrade->GetState()==EONEMachineState::Ready && Upgrade->GetDeliveredCount()==0 && W->GetSlotState(0)->InstanceId==Instance && W->GetSlotState(0)->Status==EONEWeaponSlotStatus::ReadyToCollect && W->GetEquippedIndex()==1 && !W->IsHandoffLocked(),TEXT("Leaving before take returns ready reservation without delivery or loss of other usable gun"));
        Check(Visual && Visual->HasCompletePreview() && Visual->GetVisiblePreviewPartCount()==3 && FVector::Dist(Visual->GetPreviewWorldTransform().GetLocation(),Visual->GetOutputWorldTransform().GetLocation())<.5f,TEXT("Canceled retrieval restores complete Last Word preview at output rather than following old hand"));
        if (!Approach(Upgrade)) break; Next(72);
    } break;
    case 72: if (T>.1f) { Key(EKeys::F,IE_Pressed); Next(32); } break;
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
