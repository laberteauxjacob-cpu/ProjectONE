#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEHealthComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_FourParams(FONEHealthChanged,float,float,float,bool);

UCLASS(ClassGroup=(ONE), meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONEHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEHealthComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health") float MaxHealth = 100.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health") float Health = 100.f;
    UPROPERTY(EditAnywhere, Category="Health|Player Recovery", meta=(ClampMin="0")) float RegenerationDelay=15.f;
    UPROPERTY(EditAnywhere, Category="Health|Player Recovery", meta=(ClampMin="0",ClampMax="1")) float RegenerationFractionPerSecond=.10f;
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    bool ApplyDamage(float Amount);
    bool IsDead() const { return Health <= 0.f; }
    void Restore();
    void SetMaximumHealth(float NewMaximum,bool PreserveFraction=false);
    void EnablePlayerRegeneration(bool Enabled);
    void InvalidateRecovery();
    float GetMissingHealthFraction() const;
    float GetRegenerationDelayRemaining() const;
    bool IsRegenerating() const { return bRegenerating; }
    FONEHealthChanged OnHealthChanged; // old health, new health, maximum, regeneration
private:
    bool bPlayerRecoveryEligible=false, bAcceptedDamage=false, bRegenerating=false;
    double DamageFreeSeconds=0.;
};
