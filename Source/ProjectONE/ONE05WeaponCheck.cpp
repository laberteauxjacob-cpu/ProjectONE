#include "ONE05WeaponCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "Engine/World.h"
#include "CoreGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"

namespace ONE05WeaponFixture
{
    bool bRestartPending=false;
    int32 RestartChecks=0,RestartFailures=0,RestartRate=0;
    uint64 PreviousRun=0,PreviousInstance=0;
    TArray<TSharedPtr<FJsonValue>> RestartRecords;
    FString RestartCsv;
    EONEWeaponFamily Family(int32 Index)
    { return Index<2?EONEWeaponFamily::Pistol:Index<4?EONEWeaponFamily::Carbine:EONEWeaponFamily::Shotgun; }
}
AONE05WeaponCheck::AONE05WeaponCheck()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
}
void AONE05WeaponCheck::BeginPlay()
{
    Super::BeginPlay(); StartedReal=StageReal=FPlatformTime::Seconds(); StageStart=GetWorld()->GetTimeSeconds();
    FParse::Value(FCommandLine::Get(),TEXT("ONE05Rate="),ExpectedRate);
    Csv=TEXT("world_seconds,frame,stage,variant,shots,ammo,reserve,operation,burst,accepted,rejected,dry,last_shot_time\n");
    LateTickHandle=FWorldDelegates::OnWorldPostActorTick.AddWeakLambda(this,[this](UWorld* World,ELevelTick,float)
    {
        if (World==GetWorld() && bLateTap && Weapon)
        { bLateTap=false; Tap(); }
    });
    if (ONE05WeaponFixture::bRestartPending)
    {
        Stage=900; Checks=ONE05WeaponFixture::RestartChecks; Failures=ONE05WeaponFixture::RestartFailures;
        ExpectedRate=ONE05WeaponFixture::RestartRate; Records=MoveTemp(ONE05WeaponFixture::RestartRecords); Csv=MoveTemp(ONE05WeaponFixture::RestartCsv);
        ONE05WeaponFixture::bRestartPending=false;
    }
}
void AONE05WeaponCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    FWorldDelegates::OnWorldPostActorTick.Remove(LateTickHandle);
    Super::EndPlay(Reason);
}
void AONE05WeaponCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    auto R=MakeShared<FJsonObject>(); R->SetBoolField(TEXT("pass"),Pass); R->SetStringField(TEXT("label"),Label);
    R->SetNumberField(TEXT("stage"),Stage); R->SetNumberField(TEXT("variant"),Variant); R->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds());
    Records.Add(MakeShared<FJsonValueObject>(R));
    UE_LOG(LogTemp,Display,TEXT("ONE05_WEAPON %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE05WeaponCheck::Next(int32 NewStage)
{ Stage=NewStage; StageStart=GetWorld()->GetTimeSeconds(); StageReal=FPlatformTime::Seconds(); }
void AONE05WeaponCheck::Tap()
{ Weapon->SetTrigger(false); Weapon->SetTrigger(true); Weapon->SetTrigger(false); PressFrame=GFrameCounter; }
void AONE05WeaponCheck::Prepare(int32 NewVariant)
{
    Variant=NewVariant; Player->ReleaseHeldInputs(); Weapon->SetTrigger(false);
    Weapon->ResetStarterLoadout(); Weapon->SetTrigger(false);
    const EONEWeaponFamily Other=Variant<2?EONEWeaponFamily::Carbine:ONE05WeaponFixture::Family(Variant);
    Check(Weapon->ApplyAcquisitionPlan(Weapon->BuildAcquisitionPlan(Other)),TEXT("Fixture installs a second owned family through production acquisition API"));
    Shots=Weapon->GetTotalShotsFired(); Rejected=Weapon->GetRejectedTriggerPressCount(); Weapon->SetTrigger(true);
    Check(Weapon->GetRejectedTriggerPressCount()==Rejected+1 && !Weapon->HasAcceptedFramePress(),TEXT("Fire requested during equip is rejected immediately rather than queued"));
    Next(1);
}
void AONE05WeaponCheck::Observe()
{
    const int32 Current=Weapon->GetTotalShotsFired();
    if (Current!=LastObservedShots)
    {
        if (bCadence)
        {
            Check(Current-LastObservedShots==1,TEXT("Established burst produces at most one actual discharge per evaluated frame"));
            ShotTimes.Add(double(GetWorld()->GetTimeSeconds())-Weapon->GetTimeSinceShot());
        }
        LastObservedShots=Current;
    }
    Csv+=FString::Printf(TEXT("%.6f,%llu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.6f\n"),
        GetWorld()->GetTimeSeconds(),GFrameCounter,Stage,Variant,Current,Weapon->GetAmmo(),Weapon->GetReserveAmmo(),int32(Weapon->GetOperation()),
        Weapon->IsAutomaticBurstActive(),Weapon->GetAcceptedTriggerPressCount(),Weapon->GetRejectedTriggerPressCount(),Weapon->GetDryFireCount(),
        GetWorld()->GetTimeSeconds()-Weapon->GetTimeSinceShot());
}
void AONE05WeaponCheck::Finish()
{
    if (bFinished) return;
    if (Player) Player->ReleaseHeldInputs(); UGameplayStatics::SetGamePaused(this,false);
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("candidate"),TEXT("05"));
    R->SetStringField(TEXT("fixture"),TEXT("Six actual catalog variants installed using owned-instance APIs; machine waits bypassed only for loadout setup. Real component SetTrigger/BeginReload requests, evaluated pose shots, ammunition, mechanical events, frame cadence, controller FlushPressedKeys, actual death and OpenLevel restart. Requests normally run before weapon tick; a labeled late-tap case runs in OnWorldPostActorTick. One deliberate 320ms game-thread sleep per automatic weapon probes hitch recovery. No ammo setter, modified fire interval or artificial damage target. This is functional engine input-boundary evidence, not native OS held-input, audible quality, contact/aim scene, or performance proof. ONE05Rate declares runner-requested cap; actual shot times are retained in CSV."));
    R->SetNumberField(TEXT("requested_fps"),ExpectedRate); R->SetNumberField(TEXT("checks"),Checks); R->SetNumberField(TEXT("failures"),Failures); R->SetArrayField(TEXT("assertions"),Records);
    FString Json; FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));
    const FString Folder=FPaths::ProjectSavedDir()/FString::Printf(TEXT("Candidate05/Weapon%d"),ExpectedRate); IFileManager::Get().MakeDirectory(*Folder,true);
    const bool JsonSaved=FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("checks.json")));
    const bool CsvSaved=FFileHelper::SaveStringToFile(Csv,*(Folder/TEXT("timeline.csv")));
    if (!JsonSaved || !CsvSaved) ++Failures;
    UE_LOG(LogTemp,Display,TEXT("ONE05_WEAPON_COMPLETE failures=%d checks=%d rate=%d"),Failures,Checks,ExpectedRate);
}
void AONE05WeaponCheck::Tick(float Dt)
{
    Super::Tick(Dt); const double Real=FPlatformTime::Seconds();
    if (bFinished) { if (Real-FinishedReal>.5) FPlatformMisc::RequestExit(false); return; }
    if (Real-StartedReal>200 || Real-StageReal>30) { Check(false,FString::Printf(TEXT("Bounded fixture timeout stage%d variant%d"),Stage,Variant)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !GM || !Player->GetController()) return;
    if (!Weapon) { Weapon=Player->GetWeaponComponent(); Weapon->AddTickPrerequisiteActor(this); LastObservedShots=Weapon->GetTotalShotsFired(); }
    Observe(); const float T=GetWorld()->GetTimeSeconds()-StageStart;
    if (Stage<800) Player->Health->Restore();
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(2000,0,42));
    switch (Stage)
    {
    case 0: if (T>.7f)
    {
        Check(GM->IsSandbox(),TEXT("Functional fixture uses explicit sandbox with normal waves disabled"));
        Check(Weapon->GetDefinition().Family==EONEWeaponFamily::Pistol && Weapon->GetAmmo()==7 && Weapon->GetReserveAmmo()==56 && !Weapon->GetDefinitionForWeapon(1),TEXT("Actual startup remains M1911 seven/fifty-six with empty second slot"));
        Player->SetActorLocation(FVector(-250,300,98)); Player->GetCharacterMovement()->StopMovementImmediately(); Prepare(0);
    } break;
    case 1: if (!Weapon->IsBusy()) { if (Variant<2) Weapon->SelectWeapon(0); Next(2); } break;
    case 2: if (!Weapon->IsBusy())
    {
        if (Variant%2) { FONEWeaponReservation Token; Check(Weapon->ReserveEquippedForUpgrade(Token) && Weapon->MarkUpgradeReady(Token) && Weapon->CollectUpgrade(Token),TEXT("Upgraded fixture retains real same-instance reserve/collect path")); }
        Next(3);
    } break;
    case 3: if (!Weapon->IsBusy() && Weapon->CanFire())
    {
        Check(Weapon->GetDefinition().Family==ONE05WeaponFixture::Family(Variant) && Weapon->GetDefinition().bUpgraded==(Variant%2!=0),TEXT("Requested effective family and upgrade actually equipped"));
        Check(Weapon->GetTotalShotsFired()==Shots && !Weapon->IsAutomaticBurstActive(),TEXT("Held invalid equip press stays discarded after the weapon becomes ready"));
        Shots=Weapon->GetTotalShotsFired(); Dry=Weapon->GetDryFireCount(); Ammo=Weapon->GetAmmo(); Tap(); Next(4);
    } break;
    case 4: if (Weapon->GetTotalShotsFired()>Shots)
    {
        Check(Weapon->GetTotalShotsFired()==Shots+1 && Weapon->GetAmmo()==Ammo-1 && Weapon->GetLastShotFrame()==PressFrame,TEXT("Eligible same-frame press/release produces exactly one post-pose discharge"));
        Rejected=Weapon->GetRejectedTriggerPressCount(); Tap();
        Check(Weapon->GetRejectedTriggerPressCount()==Rejected+1 && !Weapon->HasAcceptedFramePress(),TEXT("Immediate cooldown/pump press is discarded at request time")); Next(5);
    } break;
    case 5: if (T>1.05f && !Weapon->IsBusy())
    {
        Check(Weapon->GetTotalShotsFired()==Shots+1 && Weapon->GetDryFireCount()==Dry,TEXT("Rejected busy press cannot become a later shot or dry click"));
        bLateTap=true; Next(50);
    } break;
    case 50: if (T>1.05f && !Weapon->IsBusy())
    {
        Check(Weapon->GetTotalShotsFired()==Shots+2 && Weapon->GetAmmo()==Ammo-2 && Weapon->GetLastShotFrame()==PressFrame+1,TEXT("Eligible post-actor-tick tap gets exactly the next pose dispatch and is not lost"));
        // Subsequent transfer checks intentionally reload exactly one missing
        // cartridge; the late tap has its own actual-ammo assertion above.
        Weapon->SetTrigger(false); Weapon->RefillAllAmmo(); Tap(); Next(51);
    } break;
    case 51: if (T>1.05f && !Weapon->IsBusy())
    {
        Shots=Weapon->GetTotalShotsFired()-1;
        Weapon->SetHandoffLocked(true); Rejected=Weapon->GetRejectedTriggerPressCount(); Weapon->SetTrigger(true);
        Check(Weapon->GetRejectedTriggerPressCount()==Rejected+1 && !Weapon->HasAcceptedFramePress(),TEXT("Handoff rejects a new held fire request")); Next(6);
    } break;
    case 6: if (T>.15f) { Weapon->SetHandoffLocked(false); Next(7); } break;
    case 7: if (T>.25f)
    {
        Check(Weapon->GetTotalShotsFired()==Shots+1 && !Weapon->IsAutomaticBurstActive(),TEXT("Leaving handoff while invalid fire remains held cannot start a burst"));
        Weapon->SetTrigger(false); Ammo=Weapon->GetAmmo(); Reserve=Weapon->GetReserveAmmo(); Drops=Weapon->GetMagazineDropCount(); Transfers=Weapon->GetMagazineCommitCount();
        StalePlan=Weapon->BuildAcquisitionPlan(Weapon->GetDefinition().Family); Player->SetSprintHeld(true); Weapon->BeginReload(); Next(8);
    } break;
    case 8: if (Weapon->IsReloading() && Weapon->GetOperationElapsed()>.1f)
    {
        Check(Player->IsSprintRequested() && Weapon->IsReloading(),TEXT("Held sprint permits manual reload instead of canceling it"));
        if (Weapon->GetDefinition().bShellReload) { Next(12); break; }
        const uint64 Revision=Weapon->GetInventoryRevision(); const float Clock=Weapon->GetOperationElapsed(); FONEWeaponReservation Token;
        Weapon->CancelReload(); Weapon->BeginReload(); Tap();
        const bool Switched=Weapon->SelectWeapon(1-Weapon->GetEquippedIndex());
        const bool Reserved=Weapon->ReserveEquippedForUpgrade(Token); const bool Acquired=Weapon->ApplyAcquisitionPlan(StalePlan);
        Weapon->RefillAllAmmo(); Weapon->SetHandoffLocked(true);
        Check(Weapon->IsMagazineReloadCommitted() && !Weapon->CanChangeInventory() && !Switched && !Reserved && !Acquired && !Weapon->IsHandoffLocked() && Weapon->GetPendingWeaponIndex()==INDEX_NONE,TEXT("Committed magazine reload rejects cancel/fire/switch/reserve/acquire/refill/handoff backdoors"));
        Check(Weapon->GetInventoryRevision()==Revision && Weapon->GetOperationElapsed()>=Clock && Weapon->GetAmmo()==Ammo && Weapon->GetReserveAmmo()==Reserve,TEXT("Rejected actions preserve exact inventory revision, ammunition and reload clock")); Next(9);
    } break;
    case 9: if (!Weapon->IsBusy() && T>.3f)
    {
        Check(Weapon->GetAmmo()==Ammo+1 && Weapon->GetReserveAmmo()==Reserve-1 && Weapon->GetMagazineCommitCount()==Transfers+1 && Weapon->GetMagazineDropCount()==Drops+1,TEXT("Committed reload finishes one real transfer and one dropped magazine"));
        Check(Weapon->GetTotalShotsFired()==Shots+1 && Weapon->ShouldShowSeatedMagazine() && !Weapon->ShouldShowHeldMagazine(),TEXT("Completion restores presentation without resurrecting rejected fire")); Player->SetSprintHeld(false); Next(20);
    } break;
    case 12: if (Weapon->GetOperation()==EONEWeaponOperation::ShellInsert && Weapon->GetOperationElapsed()>.2f)
    {
        Tap(); Check(Weapon->GetOperation()==EONEWeaponOperation::ShellEnd && !Weapon->HasAcceptedFramePress(),TEXT("Loaded shotgun fire stops shell loading and returns hands without banking a click")); Next(13);
    } break;
    case 13: if (T>.45f && !Weapon->IsBusy())
    {
        Check(Weapon->GetAmmo()==Ammo && Weapon->GetReserveAmmo()==Reserve && Weapon->GetTotalShotsFired()==Shots+1,TEXT("Shell return grants no unearned shell and produces no delayed shot")); Tap(); Next(14);
    } break;
    case 14: if (T>1.f && !Weapon->IsBusy())
    { Check(Weapon->GetTotalShotsFired()==Shots+2 && Weapon->GetAmmo()==Ammo-1,TEXT("Fresh eligible press after shell return fires exactly once")); Player->SetSprintHeld(false); Next(20); } break;
    case 20:
        Weapon->SetTrigger(false); Weapon->RefillAllAmmo(); Shots=Weapon->GetTotalShotsFired();
        if (Weapon->GetDefinition().bAutomatic) { ShotTimes.Reset(); bCadence=true; Weapon->SetTrigger(true); Next(21); }
        else Next(30);
        break;
    case 21: if (T>=1.8f)
    {
        Weapon->SetTrigger(false); bCadence=false;
        const double I=Weapon->GetDefinition().FireInterval; const double Frame=ExpectedRate>0?1./ExpectedRate:FMath::Max(double(Dt),1./120.);
        const double Span=ShotTimes.Num()>1?ShotTimes.Last()-ShotTimes[0]:0;
        Check(ShotTimes.Num()>=17 && FMath::Abs(Span-(ShotTimes.Num()-1)*I)<=Frame+.006,FString::Printf(TEXT("Actual established automatic burst preserves cadence interval%.9f across%d shots span%.6f"),I,ShotTimes.Num(),Span));
        Shots=Weapon->GetTotalShotsFired(); Next(22);
    } break;
    case 22: if (T>.25f)
    { Check(Weapon->GetTotalShotsFired()==Shots && !Weapon->IsAutomaticBurstActive(),TEXT("Releasing automatic fire immediately ends the established burst")); Weapon->SetTrigger(true); Next(220); } break;
    case 220: if (Weapon->GetTotalShotsFired()>Shots)
    { Shots=Weapon->GetTotalShotsFired(); FPlatformProcess::Sleep(.32f); Next(221); } break;
    case 221: if (Weapon->GetTotalShotsFired()>Shots)
    {
        Check(T>.25f && Weapon->GetTotalShotsFired()==Shots+1,TEXT("Deliberate 320ms hitch produces one current discharge, not accumulated shot debt"));
        Shots=Weapon->GetTotalShotsFired(); Next(222);
    } break;
    case 222: if (T>.05f)
    {
        Check(Weapon->GetTotalShotsFired()==Shots,TEXT("Hitch recovery does not pay missed shots on immediately following frames"));
        if (auto* PC=Cast<AONEPlayerController>(Player->GetController())) PC->FlushPressedKeys();
        Shots=Weapon->GetTotalShotsFired(); Weapon->SetTrigger(true); Next(24);
    } break;
    case 24: if (T>.25f)
    {
        Check(Weapon->GetTotalShotsFired()==Shots && !Weapon->IsAutomaticBurstActive() && !Weapon->HasAcceptedFramePress(),TEXT("Focus/key flush disarms established burst and requires a real release before a new press"));
        Weapon->SetTrigger(false); Player->ReleaseHeldInputs(); Weapon->RefillAllAmmo(); Weapon->SetTrigger(false); Tap(); Next(25);
    } break;
    case 25: if (T>.15f)
    { Weapon->BeginReload(); Next(26); } break;
    case 26: if (Weapon->IsMagazineReloadCommitted() && Weapon->GetOperationElapsed()>.2f)
    {
        PausedOperation=Weapon->GetOperationElapsed(); Shots=Weapon->GetTotalShotsFired(); Ammo=Weapon->GetAmmo();
        Player->ReleaseHeldInputs(); UGameplayStatics::SetGamePaused(this,true); Weapon->SetTrigger(false); Weapon->SetTrigger(true); Weapon->BeginReload(); Next(27);
    } break;
    case 27: if (Real-StageReal>.3)
    {
        Check(Weapon->GetOperationElapsed()==PausedOperation && Weapon->GetAmmo()==Ammo && Weapon->GetTotalShotsFired()==Shots && !Weapon->HasAcceptedFramePress(),TEXT("Paused direct requests preserve committed clock/ammo and cannot arm firing"));
        UGameplayStatics::SetGamePaused(this,false); Next(28);
    } break;
    case 28: if (!Weapon->IsBusy() && T>.3f)
    { Check(Weapon->GetTotalShotsFired()==Shots && Weapon->GetAmmo()==Weapon->GetDefinition().Capacity,TEXT("Unpause completes reload without accepting the held paused press")); Weapon->SetTrigger(false); Next(30); } break;
    case 30:
        Weapon->SetTrigger(false); Weapon->RefillAllAmmo(); Weapon->AddReserveAmmo(-99999);
        Shots=Weapon->GetTotalShotsFired(); Ammo=Weapon->GetAmmo(); Dry=Weapon->GetDryFireCount(); Ejections=Weapon->GetEjectionCount();
        if (Weapon->GetDefinition().bAutomatic) Weapon->SetTrigger(true);
        Next(31); break;
    case 31:
        if (!Weapon->GetDefinition().bAutomatic && Weapon->GetAmmo()>0 && Weapon->CanFire()) Tap();
        if (Weapon->GetAmmo()==0 && !Weapon->IsBusy())
        {
            Check(Weapon->GetTotalShotsFired()==Shots+Ammo && Weapon->GetEjectionCount()==Ejections+Ammo && Weapon->GetDryFireCount()==Dry,TEXT("Real full-capacity drain ejects exactly one case per shot without an automatic dry click"));
            Weapon->SetTrigger(false); Weapon->SetTrigger(true); Check(Weapon->GetDryFireCount()==Dry+1,TEXT("Fresh ready empty/no-reserve attempt produces exactly one dry click")); Next(32);
        }
        break;
    case 32: if (T>.5f)
    {
        Check(Weapon->GetDryFireCount()==Dry+1 && !Weapon->IsReloading(),TEXT("Held empty trigger neither repeats dry clicks nor invents reserve/reload"));
        Tap(); Check(Weapon->GetDryFireCount()==Dry+2,TEXT("A later deliberately released/repressed empty trigger clicks again"));
        Tap(); Check(Weapon->GetDryFireCount()==Dry+2 && Weapon->GetLastInputResult()==EONEWeaponInputResult::DryFireRateLimited,TEXT("Rapid fresh empty presses respect the dry-feedback rate limit"));
        Player->SetSprintHeld(true); Weapon->AddReserveAmmo(1); Shots=Weapon->GetTotalShotsFired(); Next(33);
    } break;
    case 33: if (Weapon->IsReloading())
    {
        Weapon->SetTrigger(true); Check(Weapon->IsReloading() && !Weapon->HasAcceptedFramePress() && Weapon->GetDryFireCount()==Dry+2,TEXT("Empty automatic reload during sprint rejects fire without stopping first transfer or dry clicking")); Next(34);
    } break;
    case 34: if (!Weapon->IsBusy() && T>.5f)
    {
        Check(Weapon->GetAmmo()==1 && Weapon->GetReserveAmmo()==0 && Weapon->GetTotalShotsFired()==Shots && !Weapon->IsAutomaticBurstActive(),TEXT("One available reserve transfers automatically while sprinting; invalid held request stays discarded"));
        Player->SetSprintHeld(false); Weapon->SetTrigger(false);
        if (Variant<5) Prepare(Variant+1); else Next(800);
    } break;
    case 800:
        SavedRun=Weapon->GetRunId(); SavedInstance=Weapon->GetSlotState(Weapon->GetEquippedIndex())->InstanceId;
        Shots=Weapon->GetTotalShotsFired(); Ammo=Weapon->GetAmmo(); Player->ReceiveAttack(1000,Player->GetActorLocation()+FVector(100,0,0));
        Weapon->SetTrigger(false); Weapon->SetTrigger(true); Weapon->BeginReload(); Next(801); break;
    case 801: if (T>.5f)
    {
        Check(Player->IsDead() && Weapon->GetAmmo()==Ammo && Weapon->GetTotalShotsFired()==Shots && !Weapon->IsReloading() && !Weapon->HasAcceptedFramePress(),TEXT("Death cleanup rejects firing/reload and cannot leave a delayed operation"));
        ONE05WeaponFixture::bRestartPending=true; ONE05WeaponFixture::RestartChecks=Checks; ONE05WeaponFixture::RestartFailures=Failures;
        ONE05WeaponFixture::RestartRate=ExpectedRate; ONE05WeaponFixture::PreviousRun=SavedRun; ONE05WeaponFixture::PreviousInstance=SavedInstance;
        ONE05WeaponFixture::RestartRecords=Records; ONE05WeaponFixture::RestartCsv=Csv; GM->RestartScene(); Next(802);
    } break;
    case 900: if (T>.7f)
    {
        Check(!Player->IsDead() && GM->IsSandbox() && Weapon->GetRunId()!=ONE05WeaponFixture::PreviousRun && Weapon->GetSlotState(0)->InstanceId!=ONE05WeaponFixture::PreviousInstance,TEXT("Actual OpenLevel restart creates a fresh live run and owned instance"));
        Check(Weapon->GetDefinition().Family==EONEWeaponFamily::Pistol && Weapon->GetAmmo()==7 && Weapon->GetReserveAmmo()==56 && !Weapon->GetDefinitionForWeapon(1) && Weapon->GetTotalShotsFired()==0 && !Weapon->IsBusy(),TEXT("Restart leaves starter loadout and no old held shot or reload callback")); Finish();
    } break;
    default: break;
    }
}
