#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE06CombatTypes.h"
#include "ONEPowerUpPickup.generated.h"

class USphereComponent;
class UONEPowerUpComponent;
class UONE06PickupVisualComponent;
class AONEPlayer;

UCLASS()
class PROJECTONE_API AONEPowerUpPickup : public AActor
{
    GENERATED_BODY()
public:
    AONEPowerUpPickup();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    void Initialize(UONEPowerUpComponent* Authority,EONEPowerUpType Type,const FGuid& Run,float Lifetime,float Radius);
    bool TryCollect(AONEPlayer* Player);
    bool CanReach(AONEPlayer* Player) const;
    void CommitCollected();
    EONEPowerUpType GetType() const { return Type; }
    const FGuid& GetRunId() const { return RunId; }
    float GetRemainingLifetime() const { return RemainingLifetime; }
    bool IsAvailable() const { return bInitialized && !bCollected && RemainingLifetime>0.f; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> Collection;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONE06PickupVisualComponent> Visual;
private:
    TWeakObjectPtr<UONEPowerUpComponent> Manager;
    FGuid RunId;
    EONEPowerUpType Type=EONEPowerUpType::Count;
    float RemainingLifetime=0.f,TotalLifetime=30.f,CollectedTail=0.f;
    bool bInitialized=false,bCollected=false;
};
