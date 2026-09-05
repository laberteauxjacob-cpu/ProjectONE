#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/InputComponent.h"
AONEPlayerController::AONEPlayerController()
{
    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}
void AONEPlayerController::BeginPlay()
{
    Super::BeginPlay();
    CurrentMouseCursor=EMouseCursor::Crosshairs;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
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
}
void AONEPlayerController::FlushPressedKeys()
{
    Super::FlushPressedKeys();
    if (AONEPlayer* Player=Cast<AONEPlayer>(GetPawn())) Player->ReleaseHeldInputs();
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
