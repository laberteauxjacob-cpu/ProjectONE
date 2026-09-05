#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ONEPlayerController.generated.h"
UCLASS()
class PROJECTONE_API AONEPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AONEPlayerController();
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;
    virtual void FlushPressedKeys() override;
private:
    void TogglePause();
    void Restart();
    void QuitFromPause();
    void ToggleSandbox();
    void SpawnOne();
    void SpawnSix();
    void Refill();
    void ResetSandbox();
    void ClearGore();
};
