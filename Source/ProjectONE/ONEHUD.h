#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "ONEUITypes.h"
#include "ONEWeaponTypes.h"
#include "ONEHUD.generated.h"
class UTexture2D;
class AONEPlayer;
class AONEGameMode;
class AONEProgressionMachine;

/** Original atlas typography and Canvas composition. Buttons use the guarded
 * controller path; the box reel observes the physical machine's transaction. */
UCLASS()
class PROJECTONE_API AONEHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void DrawHUD() override;
    bool HandlePointerPressed(const FVector2D& Position);
    bool HandlePointerReleased(const FVector2D& Position);
    void ToggleTools();
    void CloseTools();
    bool IsPointerUIActive() const;
    bool IsToolsOpen() const { return bTools; }
    bool HasCompleteArtwork() const;
    bool GetActionBounds(EONEUIAction Action,FBox2D& Rect) const;
    const TMap<FName,FBox2D>& GetLayoutBounds() const { return LayoutBounds; }
    int32 GetLayoutOverlapCount() const;
    EONEWeaponFamily GetVisibleReelFamily() const { return ReelFamily; }
    int32 GetObservedBoxCycle() const { return ReelCycle; }
    bool IsBoxReelVisible() const { return ReelMachine.IsValid(); }
    float GetDeathPresentationProgress() const { return DeathPresentationProgress; }
private:
    struct FButton { EONEUIAction Action; FBox2D Rect; bool Enabled; };
    UPROPERTY() TObjectPtr<UTexture2D> Glyphs;
    UPROPERTY() TObjectPtr<UTexture2D> Numerals;
    UPROPERTY() TObjectPtr<UTexture2D> PanelMask;
    UPROPERTY() TArray<TObjectPtr<UTexture2D>> WeaponIcons;
    TArray<FButton> Buttons;
    TMap<FName,FBox2D> LayoutBounds;
    EONEUIAction PressedAction=EONEUIAction::None;
    int32 PressedContext=0;
    bool bTools=false;
    float K=1.f;
    TWeakObjectPtr<AONEProgressionMachine> ReelMachine;
    EONEWeaponFamily ReelFamily=EONEWeaponFamily::Invalid, PreviousReelFamily=EONEWeaponFamily::Invalid;
    int32 ReelCycle=INDEX_NONE;
    uint64 ReelReceipt=0;
    float ReelChangedAt=0.f;
    double DeathPresentedAt=-1.;
    float DeathPresentationProgress=1.f;
    int32 UIContext() const;
    float Advance(TCHAR Character,float Height) const;
    float TextWidth(const FString& Value,float Height) const;
    void Text(const FString& Value,float X,float Y,float Height,FLinearColor Color);
    void CenterText(const FString& Value,float X,float Y,float Height,FLinearColor Color);
    float Number(int32 Value,float X,float Y,float Height,FLinearColor Color);
    void Rounded(float X,float Y,float Width,float Height,FLinearColor Color,float Radius=12.f);
    void RecordPanel(FName Name,float X,float Y,float Width,float Height);
    void Keycap(const FString& Key,float X,float Y,float Width=30.f);
    void Button(EONEUIAction Action,const FString& Label,const FString& Key,float X,float Y,float Width,float Height,bool Primary=false,bool Enabled=true);
    void Icon(EONEWeaponFamily Family,bool Upgraded,float X,float Y,float Width,float Height,float Alpha=1.f);
    TArray<FString> Wrap(const FString& Value,float Width,float Height) const;
    void DrawTools(const AONEGameMode* GM,float W,float H);
    void DrawMenu(const AONEGameMode* GM,float W,float H);
    bool DrawBoxReel(const AONEPlayer* Player,float W,float Top);
};
