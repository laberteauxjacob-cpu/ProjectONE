#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEHUD.h"
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
    DefaultMouseCursor = EMouseCursor::Default;
}
void AONEPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bTraceInput=FParse::Param(FCommandLine::Get(),TEXT("ONE03InputTrace"));
    RefreshPointerStyle();
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    SetInputMode(Mode);
}
void AONEPlayerController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    // The camera is 14.5m above/behind the pawn. Measure audible machine
    // proximity at the player while retaining the camera's spatial orientation.
    if (InPawn) SetAudioListenerAttenuationOverride(InPawn->GetRootComponent(),FVector(0,0,35));
}
void AONEPlayerController::OnUnPossess()
{
    ClearAudioListenerAttenuationOverride(); Super::OnUnPossess();
}
bool AONEPlayerController::InputKey(const FInputKeyEventArgs& Params)
{
    AONEHUD* HUD=Cast<AONEHUD>(GetHUD());
    if (Params.Key==EKeys::LeftMouseButton)
    {
        if (Params.Event==IE_Pressed) bRawLeftHeld=true;
        else if (Params.Event==IE_Released) bRawLeftHeld=false;
        if (HUD && HUD->IsPointerUIActive())
        {
            float X=0,Y=0; GetMousePosition(X,Y);
            if (Params.Event==IE_Pressed)
            { bSuppressPointerUntilRelease=true; HUD->HandlePointerPressed(FVector2D(X,Y)); }
            else if (Params.Event==IE_Released)
            { bSuppressPointerUntilRelease=false; HUD->HandlePointerReleased(FVector2D(X,Y)); }
            // Menu/tray clicks never enter PlayerInput's firing action mapping.
            return true;
        }
        if (bSuppressPointerUntilRelease)
        {
            if (Params.Event==IE_Released) bSuppressPointerUntilRelease=false;
            return true;
        }
        if (Params.Event==IE_Repeat) return true;
    }
    if (HUD && HUD->IsPointerUIActive())
    {
        const bool GameplayKey=Params.Key==EKeys::W || Params.Key==EKeys::A || Params.Key==EKeys::S || Params.Key==EKeys::D ||
            Params.Key==EKeys::LeftShift || Params.Key==EKeys::R || Params.Key==EKeys::F || Params.Key==EKeys::One ||
            Params.Key==EKeys::Two || Params.Key==EKeys::Tab || Params.Key==EKeys::MouseScrollUp || Params.Key==EKeys::MouseScrollDown;
        if (GameplayKey) return true;
    }
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
    InputComponent->BindKey(EKeys::T, IE_Pressed, this, &AONEPlayerController::GrantPoints);
    InputComponent->BindKey(EKeys::Z, IE_Pressed, this, &AONEPlayerController::ForcePistol);
    InputComponent->BindKey(EKeys::X, IE_Pressed, this, &AONEPlayerController::ForceCarbine);
    InputComponent->BindKey(EKeys::C, IE_Pressed, this, &AONEPlayerController::ForceShotgun);
    InputComponent->BindKey(EKeys::V, IE_Pressed, this, &AONEPlayerController::RandomBox);
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AONEPlayerController::ToggleTools).bExecuteWhenPaused=true;
}
void AONEPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    RefreshPointerStyle();
}
void AONEPlayerController::RefreshPointerStyle()
{
    const AONEHUD* HUD=Cast<AONEHUD>(GetHUD());
    const AONEGameMode* GM=GetWorld() ? GetWorld()->GetAuthGameMode<AONEGameMode>() : nullptr;
    const bool Pointer=(HUD && HUD->IsPointerUIActive()) || IsPaused() || (GM && GM->IsGameOver());
    // Canvas draws the gameplay reticle; the system pointer is reserved for UI.
    bShowMouseCursor=Pointer;
    CurrentMouseCursor=EMouseCursor::Default;
}
void AONEPlayerController::GuardGameplayInput()
{
    bSuppressPointerUntilRelease=bRawLeftHeld;
    FlushPressedKeys();
}
void AONEPlayerController::ToggleTools()
{
    if (auto* HUD=Cast<AONEHUD>(GetHUD()))
    { GuardGameplayInput(); HUD->ToggleTools(); RefreshPointerStyle(); }
}
void AONEPlayerController::ExecuteUIAction(EONEUIAction Action)
{
    GuardGameplayInput();
    switch (Action)
    {
        case EONEUIAction::Resume:
            if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();GM && !GM->IsGameOver())
            { if (auto* HUD=Cast<AONEHUD>(GetHUD())) HUD->CloseTools(); SetPause(false); }
            break;
        case EONEUIAction::Restart: Restart(); break;
        case EONEUIAction::Quit: QuitFromPause(); break;
        case EONEUIAction::ToggleSandbox: ToggleSandbox(); break;
        case EONEUIAction::SpawnOne: SpawnOne(); break;
        case EONEUIAction::SpawnSix: SpawnSix(); break;
        case EONEUIAction::Refill: Refill(); break;
        case EONEUIAction::GrantPoints: GrantPoints(); break;
        case EONEUIAction::ResetSandbox: ResetSandbox(); break;
        case EONEUIAction::ClearGore: ClearGore(); break;
        case EONEUIAction::ToggleLighting: ToggleLighting(); break;
        case EONEUIAction::ForcePistol: ForcePistol(); break;
        case EONEUIAction::ForceCarbine: ForceCarbine(); break;
        case EONEUIAction::ForceShotgun: ForceShotgun(); break;
        case EONEUIAction::RandomBox: RandomBox(); break;
        case EONEUIAction::CloseTools: if (auto* HUD=Cast<AONEHUD>(GetHUD())) HUD->CloseTools(); break;
        default: break;
    }
    RefreshPointerStyle();
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
        GuardGameplayInput();
        if (auto* HUD=Cast<AONEHUD>(GetHUD())) HUD->CloseTools();
        SetPause(!IsPaused());
        RefreshPointerStyle();
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
void AONEPlayerController::GrantPoints() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->GrantSandboxPoints(); }
void AONEPlayerController::ForcePistol() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->SetForcedBoxReward(EONEWeaponFamily::Pistol); }
void AONEPlayerController::ForceCarbine() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->SetForcedBoxReward(EONEWeaponFamily::Carbine); }
void AONEPlayerController::ForceShotgun() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->SetForcedBoxReward(EONEWeaponFamily::Shotgun); }
void AONEPlayerController::RandomBox() { if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->SetForcedBoxReward(EONEWeaponFamily::Invalid); }
