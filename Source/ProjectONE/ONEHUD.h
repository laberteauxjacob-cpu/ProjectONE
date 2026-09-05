#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ONEHUD.generated.h"
UCLASS()
class PROJECTONE_API AONEHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void Label(const FString& Text, float X, float Y, float Scale = 1.f, FLinearColor Color = FLinearColor(.85f,.90f,.90f));
    void Panel(float X, float Y, float W, float H);
};
