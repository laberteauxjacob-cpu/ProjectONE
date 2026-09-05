#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ONEUITypes.h"
#include "ONEPlayerController.generated.h"
UCLASS()
class PROJECTONE_API AONEPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AONEPlayerController();
    virtual void BeginPlay() override;
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void SetupInputComponent() override;
    virtual void FlushPressedKeys() override;
    virtual bool InputKey(const FInputKeyEventArgs& Params) override;
    virtual void PlayerTick(float DeltaTime) override;
    void ExecuteUIAction(EONEUIAction Action);
    void GuardGameplayInput();
private:
    bool bTraceInput=false;
    bool bRawLeftHeld=false,bSuppressPointerUntilRelease=false;
    void ToggleTools();
    void RefreshPointerStyle();
    void TogglePause();
    void Restart();
    void QuitFromPause();
    void ToggleSandbox();
    void SpawnOne();
    void SpawnSix();
    void Refill();
    void ResetSandbox();
    void ClearGore();
    void ToggleLighting();
    void GrantPoints();
    void ForcePistol();
    void ForceCarbine();
    void ForceShotgun();
    void RandomBox();
};
