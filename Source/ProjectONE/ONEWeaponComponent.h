#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEWeaponTypes.h"
#include "ONEWeaponComponent.generated.h"
class UAudioComponent;
class AONEWeaponCase;
UCLASS(ClassGroup=(ONE), meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONEWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEWeaponComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* ThisTick) override;
    void SetTrigger(bool Held);
    void BeginReload();
    void CancelReload();
    void CancelAllOperations();
    bool SelectWeapon(int32 Index);
    void CycleWeapon() { SelectWeapon(((PendingIndex>=0 ? PendingIndex : EquippedIndex)+1)%2); }
    void RefillAllAmmo();
    void ClearEjectedCases();
    void RefreshEquippedPresentation();
    int32 GetAmmo() const { return GetAmmoForWeapon(EquippedIndex); }
    int32 GetReserveAmmo() const { return GetReserveAmmoForWeapon(EquippedIndex); }
    int32 GetAmmoForWeapon(int32 Index) const;
    int32 GetReserveAmmoForWeapon(int32 Index) const;
    int32 GetEquippedIndex() const { return EquippedIndex; }
    int32 GetPendingWeaponIndex() const { return PendingIndex; }
    int32 GetWeaponCount() const { return WeaponDefinitions.Num(); }
    FText GetWeaponName() const;
    bool IsReloading() const;
    bool IsBusy() const { return Operation!=EONEWeaponOperation::Ready; }
    bool CanFire() const;
    bool NeedsPump(int32 Index) const;
    EONEWeaponOperation GetOperation() const { return Operation; }
    float GetOperationElapsed() const;
    float GetOperationDuration() const;
    float GetOperationProgress() const;
    float GetReloadProgress() const;
    float GetReloadElapsed() const;
    float GetTimeSinceShot() const;
    float GetTimeSinceEmpty() const;
    float GetTimeSinceHit() const;
    bool WasLastHitKill() const { return bLastHitKill; }
    void AddReserveAmmo(int32 Count);
    int32 GetTotalShotsFired() const { return ShotsFired; }
    int32 GetShellInsertCount() const { return ShellsInserted; }
    int32 GetMagazineCommitCount() const { return MagazinesCommitted; }
    int32 GetEjectionCount() const { return CasesEjected; }
    uint64 GetLastShotId() const { return LastShotId; }
    const FONEWeaponDefinition& GetDefinition() const;
    const FONEWeaponDefinition* GetDefinitionForWeapon(int32 Index) const;
    UAnimSequence* GetReadyAnimation() const;
    UAnimSequence* GetActionAnimation(float& Time) const;
    float GetPumpFraction() const;
    bool ShouldShowLoadingShell() const;
    bool ShouldShowSeatedMagazine() const;
    bool ShouldShowHeldMagazine() const;
    UPROPERTY(EditAnywhere,EditFixedSize,Category="Weapons") TArray<FONEWeaponDefinition> WeaponDefinitions;
    // Candidate01 read-compatible mirrors. Edit the corresponding definition instead.
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") int32 MagazineSize=24;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float FireInterval=.16f;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float ReloadDuration=2.1f;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float Damage=32.f;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float Range=2800.f;
private:
    void Fire();
    void StartOperation(EONEWeaponOperation Next,int32 DefinitionIndex=-1);
    void FinishOperation();
    void ProcessWeaponEvent(const FONEWeaponTimedEvent& Event);
    void StopOperationAudio();
    void PlayMechanical(USoundBase* Sound);
    const FONEWeaponOperationDefinition* FindOperation(int32 Index,EONEWeaponOperation Op) const;
    float FindEventTime(EONEWeaponEvent Event,float Fallback) const;
    UPROPERTY(Transient) TArray<FONECarriedWeaponState> Carried;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> LoadedAssets;
    TArray<TWeakObjectPtr<UAudioComponent>> OperationAudio,ShotAudio;
    TArray<TWeakObjectPtr<AONEWeaponCase>> Cases;
    EONEWeaponOperation Operation=EONEWeaponOperation::Ready;
    int32 EquippedIndex=0,PendingIndex=-1,OperationIndex=0,NextEvent=0,OperationSerial=0;
    int32 ShotsFired=0,ShellsInserted=0,MagazinesCommitted=0,CasesEjected=0;
    uint64 LastShotId=0;
    bool bTrigger=false,bPendingShot=false,bLastHitKill=false;
    float OperationStart=0,LastShot=-100,LastEmpty=-100,LastHit=-100;
};
