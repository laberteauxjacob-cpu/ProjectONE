#include "ONE03WeaponCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

namespace
{
    bool bRestartPending=false;
    FString RestartReport;
    int32 RestartFailures=0,RestartChecks=0;
    struct FReloadCase
    {
        int32 Slot;
        EONEWeaponOperation Operation;
        float Time;
        bool bTransferred;
        const TCHAR* Label;
    };
    const FReloadCase ReloadCases[]={
        {0,EONEWeaponOperation::MagazineReload,.25f,false,TEXT("rifle before magazine removal")},
        {0,EONEWeaponOperation::MagazineReload,.75f,false,TEXT("rifle with magazine removed")},
        {0,EONEWeaponOperation::MagazineReload,1.10f,false,TEXT("rifle before ammunition transfer")},
        {0,EONEWeaponOperation::MagazineReload,1.28f,true,TEXT("rifle after ammunition transfer")},
        {0,EONEWeaponOperation::MagazineReload,1.90f,true,TEXT("rifle after bolt event")},
        {1,EONEWeaponOperation::ShellStart,.18f,false,TEXT("shotgun opening")},
        {1,EONEWeaponOperation::ShellInsert,.40f,false,TEXT("shotgun before shell insertion")},
        {1,EONEWeaponOperation::ShellInsert,.68f,true,TEXT("shotgun after shell insertion")}
    };
}

AONE03WeaponCheck::AONE03WeaponCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
}
void AONE03WeaponCheck::BeginPlay()
{
    Super::BeginPlay(); StartReal=StageReal=FPlatformTime::Seconds(); StageStart=GetWorld()->GetTimeSeconds();
    Report=TEXT("Candidate03 Stage B: actual player/weapon integration\nReal capacities, shot dispatch, operation clocks and ammo transfers. No mocked weapon or ammo setter.\nInitial binding fixture injects frame-spaced UE input events through PlayerController::InputKey; this proves production binding dispatch, not native OS keyboard delivery.\n");
    if (bRestartPending)
    {
        Stage=99; Report=RestartReport; Failures=RestartFailures; Checks=RestartChecks; bRestartPending=false;
    }
}
void AONE03WeaponCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE03_WEAPON %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE03WeaponCheck::Next(int32 NextStage)
{
    Stage=NextStage; StageStart=GetWorld()->GetTimeSeconds(); StageReal=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("ONE03_WEAPON_STAGE %d case=%d"),Stage,CaseIndex);
}
void AONE03WeaponCheck::Prepare(int32 Slot)
{
    Player->ReleaseHeldInputs();
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(FVector(-250,300,98));
    Player->GetWeaponComponent()->RefillAllAmmo();
    Player->GetWeaponComponent()->SelectWeapon(Slot);
}
void AONE03WeaponCheck::Finish()
{
    if (bFinished) return;
    if (Player) Player->ReleaseHeldInputs();
    UGameplayStatics::SetGamePaused(this,false);
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    Report+=FString::Printf(TEXT("\nChecks: %d\nFailures: %d\n"),Checks,Failures);
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03/Weapons");
    IFileManager::Get().MakeDirectory(*Folder,true);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    UE_LOG(LogTemp,Display,TEXT("ONE03_WEAPON_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
void AONE03WeaponCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    const double Now=FPlatformTime::Seconds();
    if (bFinished) { if (Now-FinishedReal>.3) FPlatformMisc::RequestExit(false); return; }
    if (Now-StartReal>140 || Now-StageReal>20)
    { Check(false,FString::Printf(TEXT("Weapon scenario timeout at stage %d case %d"),Stage,CaseIndex)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !GM) return;
    GM->MaximumActive=0;
    auto* W=Player->GetWeaponComponent();
    auto* PC=Cast<AONEPlayerController>(Player->GetController());
    auto RouteKey=[PC](const FKey& Key,EInputEvent Event)
    {
        if (PC) PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key,Event,Event==IE_Released ? 0.f : 1.f));
    };
    const float T=GetWorld()->GetTimeSeconds()-StageStart;
    if (Stage<90) Player->Health->Restore();
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(900,0,30));
    switch (Stage)
    {
    case 0: if (T>.6f)
    {
        Check(GM->IsSandbox(),TEXT("Weapon check uses the real sandbox without normal waves"));
        Prepare(0); Next(100);
    } break;
    case 100: if (PC && !W->IsBusy() && T>.06f)
    {
        Shots=W->GetTotalShotsFired(); Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo();
        RouteKey(EKeys::LeftMouseButton,IE_Pressed); Next(101);
    } break;
    case 101: if (T>.06f)
    {
        RouteKey(EKeys::LeftMouseButton,IE_Released); Next(102);
    } break;
    case 102: if (T>.25f)
    {
        Check(W->GetTotalShotsFired()==Shots+1 && W->GetAmmo()==Ammo-1,TEXT("Controller-routed mouse press/release dispatches one real fire action"));
        Ammo=W->GetAmmo(); RouteKey(EKeys::R,IE_Pressed); Next(103);
    } break;
    case 103: if (T>.06f)
    {
        RouteKey(EKeys::R,IE_Released); Next(104);
    } break;
    case 104: if (T>.45f)
    {
        Check(W->IsReloading() && W->GetAmmo()==Ammo,TEXT("Controller-routed R press starts the production magazine reload"));
        Interrupts=W->GetSprintReloadInterruptCount(); RouteKey(EKeys::LeftShift,IE_Pressed); Next(105);
    } break;
    case 105: if (T>.08f)
    {
        Check(PC && PC->IsInputKeyDown(EKeys::LeftShift) && Player->IsSprintRequested() && !W->IsReloading() && W->GetSprintReloadInterruptCount()==Interrupts+1,TEXT("Controller-routed LeftShift dispatches Run and immediately interrupts reload"));
        Check(W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve,TEXT("Production Shift binding preserves pre-transfer ammunition"));
        InterruptOrigin=Player->GetActorLocation(); RouteKey(EKeys::W,IE_Pressed); Next(106);
    } break;
    case 106: if (T>.45f)
    {
        Check(PC && PC->IsInputKeyDown(EKeys::W) && PC->IsInputKeyDown(EKeys::LeftShift) && Player->IsSprintRequested(),TEXT("Frame-spaced controller events maintain simultaneous W and LeftShift input"));
        const FVector Travel=Player->GetActorLocation()-InterruptOrigin;
        Check(Player->GetVelocity().Size2D()>Player->RunSpeed-8.f && Travel.Y<-75.f && FMath::Abs(Travel.X)<10.f,TEXT("Production W axis and Run action produce real sprint displacement in the mapped direction"));
        RouteKey(EKeys::W,IE_Released); RouteKey(EKeys::LeftShift,IE_Released); Next(107);
    } break;
    case 107: if (T>.25f)
    {
        Check(PC && !PC->IsInputKeyDown(EKeys::W) && !PC->IsInputKeyDown(EKeys::LeftShift) && !Player->IsSprintRequested() && Player->GetVelocity().Size2D()<10.f,TEXT("Production release events clear movement and sprint without lingering held input"));
        Check(!W->IsReloading() && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve,TEXT("Controller-routed interrupted reload cannot resume a stale transfer after key release"));
        Prepare(0); Next(1);
    } break;
    case 1: if (!W->IsBusy() && W->GetEquippedIndex()==0)
    {
        Check(W->GetAmmo()==24 && W->GetAmmoForWeapon(1)==6,TEXT("Both normal loaded capacities are used"));
        Shots=W->GetTotalShotsFired(); Reserve=W->GetReserveAmmo(); Automatic=W->GetAutomaticReloadCount();
        W->SetTrigger(true); Next(2);
    } break;
    case 2:
        if (W->GetAmmo()==0) W->SetTrigger(false);
        if (W->GetAutomaticReloadCount()>Automatic)
        {
            W->SetTrigger(false);
            Check(W->GetTotalShotsFired()-Shots==24 && W->GetAmmo()==0,TEXT("Rifle last shot empties its actual magazine and starts auto reload without another trigger pull"));
            Check(W->IsReloading() && W->GetReserveAmmo()==Reserve && W->GetAmmoForWeapon(1)==6,TEXT("Automatic rifle reload begins before its transfer and leaves holstered shotgun untouched"));
            Next(3);
        } break;
    case 3: if (!W->IsBusy() && W->GetAmmo()==24)
    {
        Check(W->GetReserveAmmo()==Reserve-24 && W->GetAutomaticReloadCount()==Automatic+1 && W->GetTotalShotsFired()==Shots+24,TEXT("Automatic rifle reload conserves ammo, runs once and does not fire after released input"));
        Prepare(1); Next(4);
    } break;
    case 4: if (!W->IsBusy() && W->GetEquippedIndex()==1)
    {
        Shots=W->GetTotalShotsFired(); Reserve=W->GetReserveAmmo(); Automatic=W->GetAutomaticReloadCount(); Ejections=W->GetEjectionCount();
        Transfers=W->GetShellInsertCount();
        Next(5);
    } break;
    case 5:
        if (W->GetAmmo()>0 && W->CanFire()) { W->SetTrigger(false); W->SetTrigger(true); }
        if (W->GetAmmo()==0) W->SetTrigger(false);
        if (W->GetAutomaticReloadCount()>Automatic)
        {
            W->SetTrigger(false);
            Check(W->GetTotalShotsFired()-Shots==6 && W->GetAmmo()==0,TEXT("Shotgun automatic reload follows six real trigger presses and the final empty magazine"));
            Check(!W->NeedsPump(1) && W->GetEjectionCount()==Ejections+6 && W->IsReloading(),TEXT("Final shotgun shot finishes required pump and exactly one ejection per shot before automatic shell loading"));
            Next(6);
        } break;
    case 6: if (!W->IsBusy() && W->GetAmmo()==6)
    {
        Check(W->GetReserveAmmo()==Reserve-6 && W->GetShellInsertCount()==Transfers+6 && W->GetAutomaticReloadCount()==Automatic+1 && W->GetTotalShotsFired()==Shots+6,TEXT("Automatic shell reload fills capacity from six earned insertions without ghost semi-auto fire"));
        CaseIndex=0; Prepare(ReloadCases[CaseIndex].Slot); Next(10);
    } break;
    case 10: if (!W->IsBusy() && W->GetEquippedIndex()==ReloadCases[CaseIndex].Slot)
    {
        Shots=W->GetTotalShotsFired(); W->SetTrigger(true); Next(11);
    } break;
    case 11: if (W->GetTotalShotsFired()>Shots)
    {
        W->SetTrigger(false); Check(W->GetTotalShotsFired()==Shots+1,TEXT("Interruption fixture spends one real round")); Next(12);
    } break;
    case 12: if (!W->IsBusy() && !W->NeedsPump(W->GetEquippedIndex()))
    {
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); Interrupts=W->GetSprintReloadInterruptCount();
        Automatic=W->GetAutomaticReloadCount(); Ejections=W->GetEjectionCount();
        Transfers=ReloadCases[CaseIndex].Slot==0 ? W->GetMagazineCommitCount() : W->GetShellInsertCount();
        W->BeginReload(); Next(13);
    } break;
    case 13:
    {
        const auto& C=ReloadCases[CaseIndex];
        if (W->GetOperation()==C.Operation && W->GetOperationElapsed()>=C.Time)
        {
            const float At=W->GetOperationElapsed(); InterruptOrigin=Player->GetActorLocation();
            Player->SetSprintHeld(true);
            const int32 Earned=C.bTransferred ? 1 : 0;
            const int32 ActualTransfers=C.Slot==0 ? W->GetMagazineCommitCount() : W->GetShellInsertCount();
            Check(!W->IsReloading() && W->GetSprintReloadInterruptCount()==Interrupts+1,FString::Printf(TEXT("Shift immediately cancels %s at %.3f seconds"),C.Label,At));
            Check(W->GetAmmo()==Ammo+Earned && W->GetReserveAmmo()==Reserve-Earned && ActualTransfers==Transfers+Earned,FString::Printf(TEXT("%s preserves exactly completed transfers"),C.Label));
            Check(!Player->LoadingShell->IsVisible() && !Player->HeldMagazine->IsVisible(),TEXT("Sprint interruption immediately removes obsolete held reload props"));
            Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); Next(14);
        }
    } break;
    case 14:
        W->BeginReload(); // Repeated R while Shift is held must never win precedence.
        if (T<.3f) Player->AddMovementInput(FVector(1,0,0));
        if (T>.9f)
        {
            Check(Player->IsSprintRequested() && FMath::IsNearlyEqual(Player->GetCharacterMovement()->MaxWalkSpeed,Player->RunSpeed,.1f) && FVector::Dist2D(Player->GetActorLocation(),InterruptOrigin)>30,TEXT("Reload interruption permits actual sprint displacement immediately"));
            Check(!W->IsReloading() && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && W->GetAutomaticReloadCount()==Automatic && W->GetEjectionCount()==Ejections,TEXT("Held Shift defeats repeated R without delayed transfers, auto reload or extra ejection"));
            ++CaseIndex;
            if (CaseIndex<static_cast<int32>(UE_ARRAY_COUNT(ReloadCases))) { Prepare(ReloadCases[CaseIndex].Slot); Next(10); }
            else { Prepare(0); Next(30); }
        } break;
    case 30: if (!W->IsBusy() && W->GetEquippedIndex()==0)
    {
        Automatic=W->GetAutomaticReloadCount(); Reserve=W->GetReserveAmmo(); Shots=W->GetTotalShotsFired();
        Player->SetSprintHeld(true); W->SetTrigger(true); Next(31);
    } break;
    case 31:
        if (W->GetAmmo()==0) W->ClearHeldInput();
        if (W->GetAmmo()==0 && !W->IsBusy()) Next(32);
        break;
    case 32:
        W->BeginReload();
        if (T>.8f)
        {
            Check(Player->IsSprintRequested() && W->GetAmmo()==0 && W->GetReserveAmmo()==Reserve && !W->IsReloading() && !W->CanAutoReload() && W->GetAutomaticReloadCount()==Automatic,TEXT("Empty equipped rifle defers automatic and manual reload while stationary Shift remains held"));
            Player->SetSprintHeld(false); Check(W->CanAutoReload(),TEXT("Releasing Shift makes the empty equipped rifle immediately eligible")); Next(33);
        } break;
    case 33: if (W->IsReloading() && W->GetOperationElapsed()>.5f)
    {
        Check(W->GetAutomaticReloadCount()==Automatic+1 && W->GetTotalShotsFired()==Shots+24,TEXT("Sprint release starts one automatic reload without stale held firing"));
        Player->SetSprintHeld(true); W->SelectWeapon(1); Player->SetSprintHeld(false); Next(34);
    } break;
    case 34: if (T>1.7f)
    {
        Check(W->GetEquippedIndex()==1 && W->GetAmmo()==6 && W->GetAmmoForWeapon(0)==0 && W->GetReserveAmmoForWeapon(0)==Reserve && W->GetAutomaticReloadCount()==Automatic+1,TEXT("Switching leaves canceled empty rifle holstered without hidden auto reload or stale commit"));
        W->SelectWeapon(0); Next(35);
    } break;
    case 35: if (W->IsReloading() && W->GetOperationElapsed()>.55f)
    {
        Check(W->GetAutomaticReloadCount()==Automatic+2,TEXT("Re-equipping the empty rifle waits for equip then automatically reloads once"));
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); Shots=W->GetTotalShotsFired(); PausedOperation=W->GetOperationElapsed();
        Player->ReleaseHeldInputs(); UGameplayStatics::SetGamePaused(this,true); Next(36);
        W->BeginReload(); W->SetTrigger(true); Player->SetSprintHeld(true);
        Check(!W->SelectWeapon(1) && !W->CanFire() && !W->CanAutoReload(),TEXT("Direct weapon start requests explicitly reject paused state"));
    } break;
    case 36: if (Now-StageReal>.7)
    {
        Check(FMath::IsNearlyEqual(W->GetOperationElapsed(),PausedOperation,.02f) && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && W->IsReloading(),TEXT("Pause preserves reload clock, ammo and operation despite direct input attempts"));
        UGameplayStatics::SetGamePaused(this,false); Next(37);
    } break;
    case 37: if (!W->IsBusy() && W->GetAmmo()==24)
    {
        Check(W->GetReserveAmmo()==Reserve-24 && W->GetTotalShotsFired()==Shots,TEXT("Resume completes the legitimate reload with no buffered pause shot"));
        Prepare(1); Next(50);
    } break;
    case 50: if (!W->IsBusy() && W->GetEquippedIndex()==1)
    {
        Ejections=W->GetEjectionCount(); Shots=W->GetTotalShotsFired(); W->SetTrigger(true); Next(51);
    } break;
    case 51:
        if (W->GetTotalShotsFired()>Shots) W->SetTrigger(false);
        if (W->GetOperation()==EONEWeaponOperation::Pump && W->GetOperationElapsed()>.24f)
        {
            Check(W->GetTotalShotsFired()==Shots+1 && W->NeedsPump(1) && !W->CanFire() && W->GetEjectionCount()==Ejections+1,TEXT("Pump is still required after ejection and blocks a second shot before lock"));
            Player->SetSprintHeld(true); W->BeginReload();
            Check(W->NeedsPump(1) && W->GetOperation()==EONEWeaponOperation::Pump,TEXT("Sprint and R cannot cancel or skip an active pump"));
            W->SelectWeapon(0); Next(52);
        } break;
    case 52: if (T>.65f)
    {
        Check(W->NeedsPump(1) && W->GetEjectionCount()==Ejections+1,TEXT("Holstered post-ejection shotgun retains its unfinished pump"));
        W->SelectWeapon(1); Player->SetSprintHeld(false); Next(53);
    } break;
    case 53: if (T>1.2f)
    {
        Check(!W->NeedsPump(1) && W->CanFire() && W->GetAmmo()==5 && W->GetEjectionCount()==Ejections+1 && W->GetTotalShotsFired()==Shots+1,TEXT("Resumed pump locks with no duplicate ejection, ammo loss or ghost shot"));
        Prepare(1); W->AddReserveAmmo(-99999); Automatic=W->GetAutomaticReloadCount(); Shots=W->GetTotalShotsFired(); Ejections=W->GetEjectionCount(); Next(54);
    } break;
    case 54:
        if (W->GetAmmo()>0 && W->CanFire()) { W->SetTrigger(false); W->SetTrigger(true); }
        if (W->GetAmmo()==0) W->SetTrigger(false);
        if (W->GetAmmo()==0 && !W->IsBusy())
        {
            Check(W->GetTotalShotsFired()==Shots+6 && W->GetEjectionCount()==Ejections+6,TEXT("Zero-reserve shotgun still fires and ejects exactly six loaded shells"));
            W->SetTrigger(true); Next(55);
        } break;
    case 55: if (T>.12f)
    {
        W->SetTrigger(false);
        Check(W->GetTimeSinceEmpty()<.2f && W->GetTotalShotsFired()==Shots+6 && W->GetEjectionCount()==Ejections+6,TEXT("Dry trigger produces empty feedback without a shot or casing"));
        Next(56);
    } break;
    case 56: if (T>.9f)
    {
        Check(W->GetAmmo()==0 && W->GetReserveAmmo()==0 && !W->IsReloading() && !W->CanAutoReload() && W->GetAutomaticReloadCount()==Automatic,TEXT("No reserve means no repeating automatic reload loop"));
        W->AddReserveAmmo(6); Next(57);
    } break;
    case 57: if (W->IsReloading())
    {
        Check(W->GetAutomaticReloadCount()==Automatic+1,TEXT("New reserve makes the empty equipped shotgun automatically reload"));
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); Shots=W->GetTotalShotsFired(); Automatic=W->GetAutomaticReloadCount();
        Player->ReceiveAttack(1000,Player->GetActorLocation()+FVector(100,0,0));
        W->BeginReload(); W->SetTrigger(true); Player->SetSprintHeld(true);
        Check(!W->SelectWeapon(0) && !W->CanAutoReload(),TEXT("Dead player rejects direct equip, fire, sprint and reload starts")); Next(90);
    } break;
    case 90: if (T>1.3f)
    {
        Check(Player->IsDead() && GM->IsGameOver() && !W->IsReloading() && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && W->GetTotalShotsFired()==Shots && W->GetAutomaticReloadCount()==Automatic,TEXT("Death cancels auto shell loading and all later transfers or firing"));
        bRestartPending=true; RestartReport=Report; RestartFailures=Failures; RestartChecks=Checks;
        GM->RestartScene(); Next(98);
    } break;
    case 99: if (T>.8f)
    {
        Check(!Player->IsDead() && GM->IsSandbox() && GM->GetPoints()==0,TEXT("Real level restart restores live sandbox and score"));
        Check(W->GetEquippedIndex()==0 && W->GetAmmoForWeapon(0)==24 && W->GetReserveAmmoForWeapon(0)==192 && W->GetAmmoForWeapon(1)==6 && W->GetReserveAmmoForWeapon(1)==36 && !W->NeedsPump(1),TEXT("Restart restores both carried ammo stores and clears pump obligation"));
        Check(W->GetTotalShotsFired()==0 && W->GetAutomaticReloadCount()==0 && !W->IsBusy() && !Player->IsSprintRequested(),TEXT("Restart has no stale operation, automatic reload, trigger or sprint input")); Finish();
    } break;
    default: break;
    }
}
