#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ONEPlayer.generated.h"
class UONEHealthComponent;
class UONEWeaponComponent;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UPointLightComponent;

UCLASS()
class PROJECTONE_API AONEPlayer : public ACharacter
{
    GENERATED_BODY()
public:
    AONEPlayer();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    void ReceiveAttack(float Damage, const FVector& From);
    float GetHealth() const;
    float GetMaxHealth() const;
    bool IsDead() const;
    UONEHealthComponent* GetHealthComponent() const { return Health; }
    UONEWeaponComponent* GetWeaponComponent() const { return Weapon; }
    FVector GetAimPoint() const { return AimPoint; }
    FVector GetMuzzleLocation() const;
    void FlashMuzzle();
    void ReleaseHeldInputs();
    void SetSprintHeld(bool Held) { bSprint=Held; }
    void SetAimOverride(bool Enabled,const FVector& Position) { bAimOverride=Enabled; OverrideAimPoint=Position; }
    UPROPERTY(EditAnywhere, Category="Movement") float WalkSpeed = 180.f;
    UPROPERTY(EditAnywhere, Category="Movement") float RunSpeed = 370.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredWalkSpeed = 180.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredRunSpeed = 370.f;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEHealthComponent> Health;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEWeaponComponent> Weapon;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gun;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> MuzzleLight;
    float LastDamageTime = -100.f;
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartFire(); void StopFire(); void Reload();
    void StartSprint(); void StopSprint();
    bool bSprint = false;
    bool bAimOverride = false;
    FVector OverrideAimPoint=FVector::ZeroVector;
    FVector AimPoint = FVector::ZeroVector;
    float MuzzleTime = 0.f;
};
