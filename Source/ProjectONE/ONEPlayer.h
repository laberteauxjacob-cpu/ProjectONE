#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ONEPlayer.generated.h"
class UONEHealthComponent;
class UONEWeaponComponent;
class UONEInteractionComponent;
class UAnimSequence;
enum class EONEWeaponFamily : uint8;
class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
struct FONEWeaponDefinition;

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
    UONEInteractionComponent* GetInteractionComponent() const { return Interaction; }
    FVector GetAimPoint() const { return AimPoint; }
    FVector GetMuzzleLocation() const;
    void FlashMuzzle();
    void ClearMuzzleFlash();
    bool IsMuzzleFlashVisible() const;
    float GetMuzzleFlashIntensity() const;
    FTransform GetMuzzleFlashTransform() const;
    void ClearWeaponEffects();
    void ApplyWeaponPresentation(const FONEWeaponDefinition& Definition);
    void ClearEquippedPresentation();
    FTransform GetWeaponWorldTransform() const;
    FTransform GetMagazineReleaseTransform() const;
    void BeginMachineAction(EONEWeaponFamily Family,bool bRetrieving,const FVector& FocusPoint);
    void EndMachineAction();
    UAnimSequence* GetMachineActionAnimation(float& Time) const;
    bool IsInMachineAction() const { return bMachineAction; }
    void SuppressCarriedPresentation(bool bSuppress);
    void ReleaseHeldInputs();
    void SetSprintHeld(bool Held);
    bool IsSprintRequested() const { return bSprint && !IsDead(); }
    void ClearReloadPresentation();
    float GetBodyFacingYaw() const { return BodyFacingYaw; }
    bool IsTurningInPlace() const { return bTurningInPlace; }
    float GetTurnAnimationTime() const { return TurnTime; }
    int32 GetTurnDirection() const { return TurnDirection; }
    float GetPivotFootWeight(int32 Foot) const;
    const FTransform& GetPivotFootWorld(int32 Foot) const { return PivotFeet[Foot]; }
    const FVector& GetPivotKneeWorld(int32 Foot) const { return PivotKnees[Foot]; }
    void SetAimOverride(bool Enabled,const FVector& Position) { bAimOverride=Enabled; OverrideAimPoint=Position; }
    UPROPERTY(EditAnywhere, Category="Movement") float WalkSpeed = 225.f;
    UPROPERTY(EditAnywhere, Category="Movement") float RunSpeed = 370.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredWalkSpeed = 225.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredRunSpeed = 370.f;
    UPROPERTY(EditAnywhere, Category="Animation") float TurnTriggerAngle = 55.f;
    UPROPERTY(EditAnywhere, Category="Animation") float MaximumAimOffset = 70.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredTurnDuration = .60f;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEHealthComponent> Health;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEWeaponComponent> Weapon;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEInteractionComponent> Interaction;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Gun;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ForeEnd;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Slide;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> LoadingShell;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> SeatedMagazine;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HeldMagazine;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USpringArmComponent> CameraArm;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> MuzzleLight;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UProceduralMeshComponent> MuzzleFlashMesh;
    float LastDamageTime = -100.f;
private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartFire(); void StopFire(); void Reload();
    void StartSprint(); void StopSprint();
    void SelectCarbine(); void SelectShotgun(); void CycleWeapon();
    void StartInteract(); void StopInteract();
    void UpdateBodyFacing(float DeltaSeconds,float AimYaw);
    void CapturePivotFeet();
    void BuildMuzzleFlash();
    void UpdateMuzzleFlash(float DeltaSeconds);
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> MuzzleFlashMaterial;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> MachineActionClip;
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UAnimSequence>> MachineClips;
    FQuat HandReferenceInverse=FQuat::Identity;
    bool bMachineAction=false,bSuppressCarried=false;
    float MachineActionTime=0;
    FVector MachineFocus=FVector::ZeroVector;
    bool bSprint = false;
    bool bFacingInitialized = false;
    bool bTurningInPlace = false;
    float BodyFacingYaw = 0.f;
    float TurnStartYaw = 0.f;
    float TurnTime = 0.f;
    float PreviousAimYaw = 0.f;
    float AimAngularSpeed = 0.f;
    float PivotElapsed = 1.f;
    float PivotReleaseAt[2] = {0.f,0.f};
    FTransform PivotFeet[2];
    FVector PivotKnees[2];
    int32 TurnDirection = 1;
    bool bAimOverride = false;
    FVector OverrideAimPoint=FVector::ZeroVector;
    FVector AimPoint = FVector::ZeroVector;
    float MuzzleTime = 0.f;
    float MuzzleDuration = .045f;
    float MuzzlePeakIntensity = 0.f;
};
