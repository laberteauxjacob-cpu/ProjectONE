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
