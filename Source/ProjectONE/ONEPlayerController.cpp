#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerInput.h"
#include "ONEWeaponComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "HAL/PlatformTime.h"
#include "CoreGlobals.h"
AONEPlayerController::AONEPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}
void AONEPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bTraceInput=FParse::Param(FCommandLine::Get(),TEXT("ONE03InputTrace"));
    CurrentMouseCursor=EMouseCursor::Crosshairs;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}
bool AONEPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    const bool bHandled=Super::InputKey(Params);
    if (bTraceInput && Params.Event!=IE_Axis)
    {
        const auto* P=Cast<AONEPlayer>(GetPawn());
        const auto* W=P ? P->GetWeaponComponent() : nullptr;
        // These modifier flags are PlayerInput's processed state, not an OS
        // modifier snapshot. Delivered modifier key edges are logged separately.
        UE_LOG(LogTemp,Display,TEXT("ONE03_INPUT frame=%llu real=%.6f key=%s event=%d amount=%.3f processed_shift=%d processed_ctrl=%d processed_alt=%d processed_cmd=%d handled=%d paused=%d sprint=%d operation=%d ammo=%d reserve=%d"),
            static_cast<unsigned long long>(GFrameCounter),FPlatformTime::Seconds(),*Params.Key.ToString(),int32(Params.Event),Params.AmountDepressed,
            PlayerInput && PlayerInput->IsShiftPressed(),PlayerInput && PlayerInput->IsCtrlPressed(),PlayerInput && PlayerInput->IsAltPressed(),PlayerInput && PlayerInput->IsCmdPressed(),
            bHandled,IsPaused(),P && P->IsSprintRequested(),W ? int32(W->GetOperation()) : -1,W ? W->GetAmmo() : -1,W ? W->GetReserveAmmo() : -1);
    }
    return bHandled;
}
void AONEPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction("Pause", IE_Pressed, this, &AONEPlayerController::TogglePause).bExecuteWhenPaused = true;
    InputComponent->BindAction("Restart", IE_Pressed, this, &AONEPlayerController::Restart).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::Q, IE_Pressed, this, &AONEPlayerController::QuitFromPause).bExecuteWhenPaused = true;
    InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AONEPlayerController::ToggleSandbox).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::F2, IE_Pressed, this, &AONEPlayerController::SpawnOne);
    InputComponent->BindKey(EKeys::F3, IE_Pressed, this, &AONEPlayerController::SpawnSix);
    InputComponent->BindKey(EKeys::F4, IE_Pressed, this, &AONEPlayerController::Refill);
    InputComponent->BindKey(EKeys::F5, IE_Pressed, this, &AONEPlayerController::ResetSandbox).bExecuteWhenPaused=true;
    InputComponent->BindKey(EKeys::F6, IE_Pressed, this, &AONEPlayerController::ClearGore);
    InputComponent->BindKey(EKeys::F7, IE_Pressed, this, &AONEPlayerController::ToggleLighting);
}
void AONEPlayerController::FlushPressedKeys()
{
    if (bTraceInput) UE_LOG(LogTemp,Display,TEXT("ONE03_INPUT_FLUSH frame=%llu real=%.6f"),static_cast<unsigned long long>(GFrameCounter),FPlatformTime::Seconds());
    Super::FlushPressedKeys();
    if (AONEPlayer* ControlledPawn=Cast<AONEPlayer>(GetPawn())) ControlledPawn->ReleaseHeldInputs();
}
void AONEPlayerController::TogglePause()
{
    if (const AONEGameMode* GM = GetWorld()->GetAuthGameMode<AONEGameMode>(); GM && !GM->IsGameOver())
    {
        if (AONEPlayer* P=Cast<AONEPlayer>(GetPawn())) P->ReleaseHeldInputs();
        SetPause(!IsPaused());
    }
}
void AONEPlayerController::Restart()
{
    if (AONEGameMode* GM = GetWorld()->GetAuthGameMode<AONEGameMode>(); GM && (GM->IsGameOver() || IsPaused())) GM->RestartScene();
}
void AONEPlayerController::QuitFromPause()
{
    if (const AONEGameMode* GM = GetWorld()->GetAuthGameMode<AONEGameMode>(); GM && (GM->IsGameOver() || IsPaused()))
        UKismetSystemLibrary::QuitGame(this, this, EQuitPreference::Quit, false);
}
void AONEPlayerController::ToggleSandbox() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->ToggleSandbox(); }
void AONEPlayerController::SpawnOne() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->SpawnSandboxEnemies(1); }
void AONEPlayerController::SpawnSix() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->SpawnSandboxEnemies(6); }
void AONEPlayerController::Refill() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->RefillSandboxAmmo(); }
void AONEPlayerController::ResetSandbox() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();GM && GM->IsSandbox()) GM->RestartScene(); }
void AONEPlayerController::ClearGore() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->ClearSandboxPresentation(); }
void AONEPlayerController::ToggleLighting() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->ToggleSandboxLighting(); }
