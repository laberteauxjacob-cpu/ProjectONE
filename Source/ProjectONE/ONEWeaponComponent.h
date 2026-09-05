#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEWeaponComponent.generated.h"
class USoundBase;

UCLASS(ClassGroup=(ONE), meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONEWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEWeaponComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* ThisTick) override;
    void SetTrigger(bool Held) { bTrigger = Held; }
    void BeginReload();
    void CancelReload() { bReloading = false; }
    int32 GetAmmo() const { return Ammo; }
    int32 GetReserveAmmo() const { return ReserveAmmo; }
    bool IsReloading() const { return bReloading; }
    float GetReloadProgress() const;
    float GetReloadElapsed() const;
    float GetTimeSinceShot() const;
    float GetTimeSinceEmpty() const;
    bool WasLastHitKill() const { return bLastHitKill; }
    float GetTimeSinceHit() const;
    void AddReserveAmmo(int32 Count) { ReserveAmmo = FMath::Min(270,ReserveAmmo+Count); }
    UPROPERTY(EditAnywhere, Category="Weapon") int32 MagazineSize = 24;
    UPROPERTY(EditAnywhere, Category="Weapon") float FireInterval = .16f;
    UPROPERTY(EditAnywhere, Category="Weapon") float ReloadDuration = 2.1f;
    UPROPERTY(EditAnywhere, Category="Weapon") float Damage = 32.f;
    UPROPERTY(EditAnywhere, Category="Weapon") float Range = 2800.f;
private:
    void Fire();
    UPROPERTY() TObjectPtr<USoundBase> ShotSound;
    UPROPERTY() TObjectPtr<USoundBase> ReloadSound;
    UPROPERTY() TObjectPtr<USoundBase> EmptySound;
    UPROPERTY() TObjectPtr<USoundBase> ImpactSound;
    int32 Ammo=24, ReserveAmmo=192;
    bool bTrigger=false, bReloading=false, bLastHitKill=false;
    float ReloadStart=-100, LastShot=-100, LastEmpty=-100, LastHit=-100;
};
