#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ONE06CombatTypes.h"
#include "ONE06PickupVisualComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class UAudioComponent;
class USoundBase;

/** Original primitive-built emblems, one small nonshadow light and one collection
 * cue. The pickup actor alone owns overlap, lifetime, run and effect transactions.
 * Local XY is the emblem face; geometry stays within 44 cm of its center. */
UCLASS(ClassGroup=(ONE), meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONE06PickupVisualComponent : public USceneComponent
{
    GENERATED_BODY()
public:
    UONE06PickupVisualComponent();
    void Configure(EONEPowerUpType Type);
    void SetRemainingLifetime(float Remaining,float Total);
    void PlayCollectionCue();
    void Shutdown();
    bool IsConfigured() const;
    int32 GetVisiblePartCount() const;
    float GetGlowIntensity() const;
    float GetVisualOpacity() const { return VisualOpacity; }
    int32 GetCollectionCueCount() const { return CollectionCueCount; }
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    UStaticMeshComponent* Part(const TCHAR* Name,const TCHAR* Shape,const FVector& Position,
        const FVector& Scale,UMaterialInstanceDynamic* Material,float Yaw=0.f);
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TObjectPtr<USceneComponent> Emblem;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BodyMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> InkMaterial;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> GlowMaterial;
    UPROPERTY() TObjectPtr<UPointLightComponent> Glow;
    UPROPERTY() TObjectPtr<UAudioComponent> CueVoice;
    UPROPERTY() TObjectPtr<USoundBase> CollectionSound;
    EONEPowerUpType Kind=EONEPowerUpType::Count;
    FLinearColor Color=FLinearColor::White;
    float VisualOpacity=1.f;
    int32 CollectionCueCount=0;
    bool bConfigured=false,bCollected=false;
};
