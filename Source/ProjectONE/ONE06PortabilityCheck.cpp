#include "ONE06PortabilityCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEInteractionComponent.h"
#include "ONEProgressionMachine.h"
#include "ONE04MachinePresentation.h"
#include "ONEPowerUpComponent.h"
#include "ONEPowerUpPickup.h"
#include "ONE06CaptureComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "CoreGlobals.h"

AONE06PortabilityCheck::AONE06PortabilityCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    Capture=CreateDefaultSubobject<UONE06CaptureComponent>(TEXT("OptionalEngineCapture"));
}
void AONE06PortabilityCheck::BeginPlay()
{
    Super::BeginPlay(); StartedReal=FPlatformTime::Seconds(); PhaseAt=GetWorld()->GetTimeSeconds();
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate06/Portability")/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Timeline=TEXT("frame,world_seconds,phase,points,health,regen_delay,regenerating,player_x,player_y,player_z,box_state,upgrade_state,ready_remaining,equipped,slot0_status,slot0_instance,slot0_ammo,slot0_reserve,slot1_status,slot1_instance,slot1_ammo,slot1_reserve,taps,holds,shots,pickup_notifications\n");
    bCaptureRequested=FParse::Param(FCommandLine::Get(),TEXT("ONE06Capture"));
    if (bCaptureRequested)
    {
        // Packaged ProjectSavedDir may itself be relative to the executable.
        // The capture API interprets relative inputs as relative to Saved.
        bCaptureStarted=Capture->BeginCapture(FPaths::ConvertRelativePathToFull(Folder/TEXT("Media")));
        if (!bCaptureStarted)
        { bCaptureFailureRecorded=true; Check(false,TEXT("Optional engine capture could not start: ")+Capture->GetFailureReason()); }
        else Capture->SetPhaseLabel(TEXT("Second-map warmup and authored fixture discovery"));
    }
}
void AONE06PortabilityCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    auto Row=MakeShared<FJsonObject>(); Row->SetNumberField(TEXT("phase"),Phase);
    Row->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds());
    Row->SetBoolField(TEXT("pass"),Pass); Row->SetStringField(TEXT("label"),Label);
    Assertions.Add(MakeShared<FJsonValueObject>(Row));
    UE_LOG(LogTemp,Display,TEXT("ONE06_PORTABILITY %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE06PortabilityCheck::Enter(int32 Next)
{
    Phase=Next; PhaseAt=GetWorld()->GetTimeSeconds();
    static const TCHAR* Labels[]={
        TEXT("Second-map fixture discovery"),TEXT("Registered standing target and real pistol aim"),
        TEXT("Pistol discharge and impact scoring"),TEXT("Manager-created Max Ammo overlap"),
        TEXT("Held F purchases relocated Mystery Box"),TEXT("Normal five-second Box reel"),
        TEXT("Held F collects carbine into second slot"),TEXT("Original pistol selection and accepted injury"),
        TEXT("Tap F deposits original pistol"),TEXT("Nine-second pistol processing while remaining in reach"),
        TEXT("Automatic exact-slot Last Word return without F"),TEXT("Select second owned carbine"),
        TEXT("Tap F deposits carbine"),TEXT("Carbine handoff before leaving reach"),
        TEXT("Carbine processing while away; normal health recovery"),TEXT("Fifteen-second uncollected ready deadline"),
        TEXT("Permanent carbine loss and closed receipt; pistol retained")};
    if (bCaptureStarted && Next>=0 && Next<UE_ARRAY_COUNT(Labels)) Capture->SetPhaseLabel(Labels[Next]);
}
void AONE06PortabilityCheck::Key(const FKey& Input,bool Down)
{
    if (auto* PC=Player?Cast<AONEPlayerController>(Player->GetController()):nullptr)
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(Input,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
}
void AONE06PortabilityCheck::Position(const FVector& XY)
{
    // A disclosed fixture teleport, relative to authored actors and measured floor.
    Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(FVector(XY.X,XY.Y,FloorZ+Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f),false,nullptr,ETeleportType::TeleportPhysics);
    Player->GetCapsuleComponent()->UpdateOverlaps();
}
bool AONE06PortabilityCheck::Approach(AONEProgressionMachine* Machine)
{
    Position(Machine->GetInteractionPoint()+Machine->GetActorForwardVector()*45.f);
    Player->SetAimOverride(true,Machine->GetInteractionPoint());
    const bool Reach=Machine->CanReach(Player);
    Check(Reach,Machine->IsBox()?TEXT("Relocated Box has a clear reachable authored front"):TEXT("Relocated PaP has a clear reachable authored front"));
    return Reach;
}
bool AONE06PortabilityCheck::Observe(float DeltaSeconds)
{
    auto* W=Player->GetWeaponComponent(); auto* H=Player->GetHealthComponent();
    const auto* A=W->GetSlotState(0); const auto* B=W->GetSlotState(1);
    if (!A || !B || Player->IsDead() || Mode->IsGameOver() || Mode->GetPoints()<0)
    { Check(false,TEXT("Per-frame live-player, nonnegative-points and two-slot invariants")); return false; }
    const float Now=GetWorld()->GetTimeSeconds(); const FVector P=Player->GetActorLocation();
    Timeline+=FString::Printf(TEXT("%llu,%.6f,%d,%d,%.6f,%.6f,%d,%.4f,%.4f,%.4f,%d,%d,%.6f,%d,%d,%llu,%d,%d,%d,%llu,%d,%d,%d,%d,%d,%llu\n"),
        GFrameCounter,Now,Phase,Mode->GetPoints(),H->Health,H->GetRegenerationDelayRemaining(),H->IsRegenerating(),P.X,P.Y,P.Z,
        int32(Box->GetState()),int32(Upgrade->GetState()),Upgrade->GetReadySecondsRemaining(),W->GetEquippedIndex(),
        int32(A->Status),A->InstanceId,A->Ammo,A->Reserve,int32(B->Status),B->InstanceId,B->Ammo,B->Reserve,
        Player->GetInteractionComponent()->GetCompletedTaps(),Player->GetInteractionComponent()->GetCompletedHolds(),W->GetTotalShotsFired(),Mode->GetPowerUps()->GetPickupNotificationSerial());
    ++ObservedFrames;
    if (DamageAt>=0.)
    {
        const double Age=Now-DamageAt;
        const float Expected=FMath::Min(H->MaxHealth,InjuredHealth+H->MaxHealth*.1f*float(FMath::Max(0.,Age-15.)));
        // Actor ordering can contribute one frame at the transition; tolerate two.
        const float Tolerance=.25f+2.f*FMath::Max(0.f,DeltaSeconds)*H->MaxHealth*.1f;
        if (FMath::Abs(H->Health-Expected)>Tolerance)
        { Check(false,TEXT("Per-frame accepted-damage recovery follows real15-second delay and10%-maximum rate")); return false; }
        bSawPreDelay|=Age>=14. && Age<14.9 && FMath::IsNearlyEqual(H->Health,InjuredHealth,.05f);
        bSawRecovery|=Age>15. && H->IsRegenerating() && H->Health>InjuredHealth && Upgrade->GetState()==EONEMachineState::Active;
        bRecovered|=Age>=17. && FMath::IsNearlyEqual(H->Health,H->MaxHealth,.05f);
    }
    return true;
}
void AONE06PortabilityCheck::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); const double Real=FPlatformTime::Seconds(),Now=GetWorld()->GetTimeSeconds();
    if (bFinished) { if (Real-FinishedReal>.4) FPlatformMisc::RequestExit(false); return; }
    if (bFinishing) { Finish(bFinishComplete); return; }
    if (bCaptureRequested && Capture->HasFailed())
    { if (!bCaptureFailureRecorded) { bCaptureFailureRecorded=true; Check(false,TEXT("Optional engine capture failed: ")+Capture->GetFailureReason()); } Finish(false); return; }
    if (Real-StartedReal>120.) { Check(false,FString::Printf(TEXT("Bounded120-second portability timeout phase%d"),Phase)); Finish(false); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Mode) Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !Mode || !Player->GetController()) return;
    if (Failures>0) { Finish(false); return; }
    auto* W=Player->GetWeaponComponent(); auto* H=Player->GetHealthComponent();
    auto* Powers=Mode->GetPowerUps(); auto* Interaction=Player->GetInteractionComponent();
    const double T=Now-PhaseAt;
    if (Phase>0 && (!IsValid(Box) || !IsValid(Upgrade) || !Observe(DeltaSeconds))) { Finish(false); return; }
    switch (Phase)
    {
    case 0: if (T>1.)
    {
        int32 Boxes=0,Upgrades=0,Markers=0;
        for (TActorIterator<AONEMysteryBox> It(GetWorld());It;++It) { Box=*It; ++Boxes; }
        for (TActorIterator<AONEUpgradeMachine> It(GetWorld());It;++It) { Upgrade=*It; ++Upgrades; }
        for (TActorIterator<AActor> It(GetWorld());It;++It) if (It->ActorHasTag(TEXT("ONE06_PickupFixture"))) { PickupAnchor=It->GetActorLocation(); ++Markers; }
        const bool Authored=GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/Portability06")) && Boxes==1 && Upgrades==1 && Markers==1;
        Check(Authored,TEXT("Explicit Portability06 map supplies exactly one ordinary Box, PaP and tagged pickup anchor"));
        if (!Authored) { Finish(false); return; }
        Check(Mode->IsSandbox() && Mode->GetClass()==AONEGameMode::StaticClass(),TEXT("Opt-in fixture uses the ordinary GameMode in disclosed sandbox mode"));
        Check(Box->GetPresentation()->IsConfigured() && Upgrade->GetPresentation()->IsConfigured() && Box->RollDuration==5.f && Upgrade->ProcessingDuration==9.f && Upgrade->ReadyLifetime==15.f,
            TEXT("Both relocated machines have normal assets and unchanged5/9/15-second tuning"));
        Check(W->GetDefinition().Family==EONEWeaponFamily::Pistol && !W->GetDefinition().bUpgraded && W->GetAmmo()==7 && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Empty,
            TEXT("Second map starts with the normal M1911 and empty second slot"));
        Check(H->Health==H->MaxHealth && H->RegenerationDelay==15.f && H->RegenerationFractionPerSecond==.1f,TEXT("Normal player health and recovery defaults are present"));
        FHitResult Ground; FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE06PortabilityFloor),false,Player);
        const bool Grounded=GetWorld()->LineTraceSingleByChannel(Ground,PickupAnchor+FVector(0,0,400),PickupAnchor-FVector(0,0,400),ECC_Visibility,Params);
        Check(Grounded && Ground.ImpactNormal.Z>.7f,TEXT("Authored pickup anchor has actual supporting level collision"));
        if (!Grounded) { Finish(false); return; }
        FloorZ=Ground.ImpactPoint.Z; Position(PickupAnchor+FVector(0,-180,0));
        Target=Mode->SpawnSandboxEnemyAt(PickupAnchor+FVector(0,190,0));
        Check(Target.IsValid(),TEXT("Normal registered infected spawns on navigation reachable from this second-map player"));
        if (!Target.IsValid()) { Finish(false); return; }
        Target->AttackDamage=0.f; Target->SetActorTickEnabled(false); Target->GetCharacterMovement()->DisableMovement();
        if (auto* AI=Cast<AAIController>(Target->GetController())) AI->StopMovement();
        Target->GetMesh()->TickAnimation(0.f,false); Target->GetMesh()->RefreshBoneTransforms(); Target->GetMesh()->bPauseAnims=true;
        Player->SetAimOverride(true,FVector(Target->GetActorLocation().X,Target->GetActorLocation().Y,FloorZ+Player->TorsoAimHeight));
        PointsBefore=Mode->GetPoints(); ShotsBefore=W->GetTotalShotsFired(); Enter(1);
    } break;
    case 1: if (T>.4 && W->CanFire()) { Key(EKeys::LeftMouseButton,true); Enter(2); } break;
    case 2: if (T>.3)
    {
        Key(EKeys::LeftMouseButton,false);
        Check(W->GetTotalShotsFired()==ShotsBefore+1 && W->GetLastShotLiveHitCount()==1 && W->GetLastShotNewKillCount()==0 && Mode->GetPoints()==PointsBefore+10,
            TEXT("Actual pistol trace hits the standing registered target and awards exactly10 impact points on the second map"));
        if (Target.IsValid()) Target->Destroy(); Target.Reset();
        NotificationsBefore=Powers->GetPickupNotificationSerial();
        const auto Drop=Powers->ForceDrop(EONEPowerUpType::MaxAmmo,PickupAnchor);
        Check(Drop==EONEPowerUpDropResult::Spawned,TEXT("Production power-up placement finds reachable ground at the authored second-map marker"));
        for (TActorIterator<AONEPowerUpPickup> It(GetWorld());It;++It)
            if (It->IsAvailable() && It->GetType()==EONEPowerUpType::MaxAmmo && It->GetRunId()==Powers->GetRunId()) Pickup=*It;
        if (!Pickup.IsValid()) { Check(false,TEXT("Manager-created Max Ammo pickup exists in current run")); Finish(false); return; }
        Position(Pickup->GetActorLocation()); Pickup->Collection->UpdateOverlaps();
        Check(Pickup->Collection->IsOverlappingComponent(Player->GetCapsuleComponent()),TEXT("Declared fixture teleport produces real pickup-sphere/player-capsule overlap"));
        Enter(3);
    } break;
    case 3: if (T>.5)
    {
        Check(Powers->GetPickupNotificationSerial()==NotificationsBefore+1 && Powers->GetLastCollectedType()==EONEPowerUpType::MaxAmmo && W->GetAmmo()==W->GetDefinition().Capacity && W->GetReserveAmmo()==W->GetDefinition().ReserveLimit,
            TEXT("Ordinary overlap polling collects Max Ammo once and fills real available ammunition without direct collection call"));
        Mode->GrantSandboxPoints(); Mode->GrantSandboxPoints();
        Check(W->IsFamilyRollEligible(EONEWeaponFamily::Carbine),TEXT("Unowned carbine is eligible for this normal Box pool"));
        Mode->SetForcedBoxReward(EONEWeaponFamily::Carbine);
        if (!Approach(Box)) { Finish(false); return; }
        PointsBefore=Mode->GetPoints(); HoldsBefore=Interaction->GetCompletedHolds(); Key(EKeys::F,true); Enter(4);
    } break;
    case 4: if (T>.65)
    {
        Key(EKeys::F,false);
        Check(Box->GetAcceptedCount()==1 && Box->GetState()==EONEMachineState::Active && Interaction->GetCompletedHolds()==HoldsBefore+1 && Mode->GetPoints()==PointsBefore-950 && Box->GetRewardFamily()==EONEWeaponFamily::Carbine,
            TEXT("Held production F buys the eligible forced Box result once for normal950-point price"));
        Enter(5);
    } break;
    case 5: if (Box->GetState()==EONEMachineState::Ready)
    { Check(Box->GetRollPool().Contains(EONEWeaponFamily::Carbine) && !Box->GetRollPool().Contains(EONEWeaponFamily::Pistol),TEXT("Paid Box reel excludes the owned pistol family")); Key(EKeys::F,true); Enter(6); } break;
    case 6: if (T>.65)
    {
        Key(EKeys::F,false);
        Check(Box->GetDeliveredCount()==1 && Interaction->GetCompletedHolds()==HoldsBefore+2 && W->GetSlotState(1)->Family==EONEWeaponFamily::Carbine && W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Available,
            TEXT("Second held F collects real carbine into the empty owned slot"));
        Key(EKeys::One,true); Enter(7);
    } break;
    case 7: if (T>.6 && !W->IsBusy())
    {
        Key(EKeys::One,false);
        Check(W->GetEquippedIndex()==0 && W->GetDefinition().Family==EONEWeaponFamily::Pistol,TEXT("Production slot key selects original pistol for first upgrade"));
        if (!Approach(Upgrade)) { Finish(false); return; }
        Player->ReceiveAttack(19.f,Player->GetActorLocation()+FVector(100,0,0)); DamageAt=Now; InjuredHealth=H->Health;
        Check(FMath::IsNearlyEqual(InjuredHealth,H->MaxHealth-19.f,.01f),TEXT("Normal player attack receiver accepts19 damage without health-field editing"));
        PointsBefore=Mode->GetPoints(); TapsBefore=Interaction->GetCompletedTaps(); Key(EKeys::F,true); Enter(8);
    } break;
    case 8: if (T>.15)
    {
        Key(EKeys::F,false); Returned=Upgrade->GetReservation();
        Check(Returned.IsValid() && Returned.Slot==0 && Upgrade->GetState()==EONEMachineState::Active && Interaction->GetCompletedTaps()==TapsBefore+1 && Mode->GetPoints()==PointsBefore-5000,
            TEXT("One production F tap reserves exact pistol slot and pays normal5000 upgrade price"));
        FirstAcceptedAt=Now-Upgrade->GetStateElapsed(); Enter(9);
    } break;
    case 9: if (Upgrade->GetDeliveredCount()==1)
    {
        const auto* Slot=W->GetSlotState(Returned.Slot);
        Check(Now-FirstAcceptedAt>=9.-FMath::Max(.002f,DeltaSeconds) && Slot && Slot->InstanceId==Returned.InstanceId && Slot->Family==EONEWeaponFamily::Pistol && Slot->bUpgraded && Slot->Status==EONEWeaponSlotStatus::Available && Upgrade->CanReach(Player) && Interaction->GetCompletedTaps()==TapsBefore+1,
            TEXT("Standing within reach returns exact upgraded pistol automatically after9 seconds with no collection key"));
        Enter(10);
    } break;
    case 10: if (T>1. && Upgrade->GetState()==EONEMachineState::Idle && !W->IsBusy())
    { Key(EKeys::Two,true); Enter(11); } break;
    case 11: if (T>.5 && !W->IsBusy())
    {
        Key(EKeys::Two,false);
        Check(W->GetEquippedIndex()==1 && W->GetDefinition().Family==EONEWeaponFamily::Carbine && !W->GetDefinition().bUpgraded,TEXT("Second owned base family remains available for its own upgrade"));
        if (!Approach(Upgrade)) { Finish(false); return; }
        PointsBefore=Mode->GetPoints(); Key(EKeys::F,true); Enter(12);
    } break;
    case 12: if (T>.15)
    {
        Key(EKeys::F,false); Expired=Upgrade->GetReservation(); ExpiryReceipt=Upgrade->GetPaymentReceipt();
        Check(Expired.IsValid() && Expired.Slot==1 && Upgrade->GetState()==EONEMachineState::Active && Interaction->GetCompletedTaps()==TapsBefore+2 && Mode->GetPoints()==PointsBefore-5000,
            TEXT("Second tap reserves carbine exact slot and pays once independently of first return"));
        PointsAfterExpiryDeposit=Mode->GetPoints(); SecondAcceptedAt=Now-Upgrade->GetStateElapsed(); Enter(13);
    } break;
    case 13: if (T>.8)
    {
        Position(PickupAnchor); Player->SetAimOverride(true,Upgrade->GetInteractionPoint());
        Check(!Upgrade->CanReach(Player) && W->HasUsableWeapon(),TEXT("Authored pickup area lies outside PaP reach while returned pistol stays usable"));
        Enter(14);
    } break;
    case 14: if (Upgrade->GetState()==EONEMachineState::Ready)
    {
        ReadyAt=Now-Upgrade->GetStateElapsed();
        Check(Now-SecondAcceptedAt>=9.-FMath::Max(.002f,DeltaSeconds) && W->GetSlotState(Expired.Slot)->Status==EONEWeaponSlotStatus::ReadyToCollect && Upgrade->GetReadySecondsRemaining()>14.f,
            TEXT("Second actual9-second process opens the normal15-second ready deadline while player is away"));
        Enter(15);
    } break;
    case 15:
        if (!bSawReadyWarning && Upgrade->GetState()==EONEMachineState::Ready && Upgrade->GetReadySecondsRemaining()<.6f)
        {
            bSawReadyWarning=true;
            Check(Upgrade->IsExpiryWarning() && W->GetSlotState(Expired.Slot)->InstanceId==Expired.InstanceId && !W->IsFamilyRollEligible(EONEWeaponFamily::Carbine),
                TEXT("Uncollected exact instance remains reserved with warning immediately before deadline"));
        }
        if (Upgrade->GetExpiredCount()==1)
        {
            Check(Now-ReadyAt>=15.-FMath::Max(.002f,DeltaSeconds) && W->GetSlotState(Expired.Slot)->Status==EONEWeaponSlotStatus::Empty && W->IsFamilyRollEligible(EONEWeaponFamily::Carbine) && Mode->GetPoints()==PointsAfterExpiryDeposit,
                TEXT("Actual15-second away deadline permanently removes carbine and releases eligibility without refund"));
            Check(!Mode->RefundPointsOnce(ExpiryReceipt) && !W->CollectUpgrade(Expired) && !W->RollbackUpgrade(Expired),TEXT("Expired receipt and token cannot refund, return or technically roll back intentional loss"));
            Check(Upgrade->WasLastLossFor(Player) && Upgrade->GetLastLostFamily()==EONEWeaponFamily::Carbine && Upgrade->GetLastLostReceipt()==ExpiryReceipt,
                TEXT("Loss feedback identity belongs to this player, run, receipt and family"));
            Enter(16);
        }
        break;
    case 16: if (T>1.)
    {
        Check(bSawPreDelay && bSawRecovery && bRecovered,TEXT("Per-frame health observations prove15-second no-heal window and10%-maximum recovery during second machine process"));
        Check(bSawReadyWarning && Upgrade->GetExpiredCount()==1 && Upgrade->GetDeliveredCount()==1 && Upgrade->GetState()==EONEMachineState::Idle && Mode->GetPoints()==PointsAfterExpiryDeposit && W->HasUsableWeapon(),
            TEXT("Relocated PaP completes one automatic return and one permanent expiry without repeating either transaction"));
        Check(ObservedFrames>100,TEXT("Every active fixture frame recorded health, inventory, machine, input-count and point state"));
        Finish(true);
    } break;
    }
}
void AONE06PortabilityCheck::Finish(bool Complete)
{
    if (bFinished) return;
    if (!bFinishing)
    {
        bFinishing=true; bFinishComplete=Complete;
        Key(EKeys::F,false); Key(EKeys::LeftMouseButton,false); Key(EKeys::One,false); Key(EKeys::Two,false);
        if (Player) Player->ReleaseHeldInputs();
        if (Target.IsValid()) Target->Destroy();
        if (bCaptureStarted) Capture->SetPhaseLabel(TEXT("Final state and capture drain"));
    }
    if (bCaptureRequested)
    {
        if (Capture->HasFailed())
        {
            if (!bCaptureFailureRecorded) { bCaptureFailureRecorded=true; Check(false,TEXT("Optional engine capture failed: ")+Capture->GetFailureReason()); }
            bFinishComplete=false;
        }
        else if (bCaptureStarted)
        {
            // Component auto-ticks and polls its one writer, pending screenshot,
            // minimum0.4s real audio tail and stable finalized WAV. Never block.
            if (!Capture->FinishCaptureAndWait()) return;
            Check(Capture->IsComplete() && Capture->GetFrameCount()>0,TEXT("Optional full-viewport frames drained and master WAV finalized before owned process exit"));
        }
        else bFinishComplete=false;
    }
    WriteResults(bFinishComplete);
}
void AONE06PortabilityCheck::WriteResults(bool Complete)
{
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    if (Player) Player->SetAimOverride(false,FVector::ZeroVector);
    Check(FFileHelper::SaveStringToFile(Timeline,*(Folder/TEXT("timeline.csv"))),TEXT("Per-frame portability timeline saved"));
    auto Report=MakeShared<FJsonObject>(); Report->SetStringField(TEXT("candidate"),TEXT("06"));
    Report->SetBoolField(TEXT("complete"),Complete); Report->SetStringField(TEXT("map"),GetWorld()->GetOutermost()->GetName());
    Report->SetStringField(TEXT("method"),TEXT("Opt-in Portability06 fixture uses ordinary player/GameMode/machines/weapon traces/registered enemy/pickup placement and overlap/health clocks. Explicit setup uses authored-actor-relative teleports, a frozen non-attacking standing target, world-point aim, sandbox points, forced eligible Box reward and forced Max Ammo. InputKey is engine-simulated, not native input. No direct successful pickup collection, direct weapon damage packet, synthetic machine completion, performance or perceptual review claim."));
    Report->SetBoolField(TEXT("capture_requested"),bCaptureRequested);
    if (bCaptureRequested)
    {
        Report->SetBoolField(TEXT("capture_complete"),Capture->IsComplete());
        Report->SetNumberField(TEXT("captured_frames"),Capture->GetFrameCount());
        Report->SetStringField(TEXT("capture_metadata"),TEXT("Media/capture.json"));
        Report->SetStringField(TEXT("capture_failure_reason"),Capture->GetFailureReason());
    }
    else Report->SetStringField(TEXT("media"),TEXT("Disabled; observed_frames counts numeric timeline samples, not captured images."));
    Report->SetNumberField(TEXT("observed_frames"),ObservedFrames); Report->SetNumberField(TEXT("checks"),Checks); Report->SetNumberField(TEXT("failures"),Failures);
    Report->SetNumberField(TEXT("elapsed_real_seconds"),FinishedReal-StartedReal); Report->SetArrayField(TEXT("assertions"),Assertions);
    Report->SetStringField(TEXT("timing_precision"),TEXT("Machine transition observations allow one sampled frame for clock accumulation/order. Health is checked each active frame with two-frame plus0.25HP tolerance; exact ready-boundary ordering has separate rule tests."));
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    if (!FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("checks.json")))) { ++Failures; UE_LOG(LogTemp,Error,TEXT("ONE06_PORTABILITY FAIL | Report could not be saved")); }
    UE_LOG(LogTemp,Display,TEXT("ONE06_PORTABILITY_COMPLETE complete=%d failures=%d checks=%d observed_frames=%d"),Complete,Failures,Checks,ObservedFrames);
}
void AONE06PortabilityCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    if (!bFinished)
    {
        Check(false,TEXT("Portability fixture interrupted before completion")); bFinishComplete=false; Finish(false);
        // External world teardown cannot keep polling. Preserve an incomplete
        // report; the component records its own interrupted-capture failure.
        if (!bFinished) WriteResults(false);
    }
    Super::EndPlay(Reason);
}
