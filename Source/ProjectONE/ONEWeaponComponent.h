#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEWeaponTypes.h"
#include "ONEWeaponComponent.generated.h"
class UAudioComponent;
class AONEWeaponCase;
class AONEWeaponMagazine;
UCLASS(ClassGroup=(ONE), meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONEWeaponComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEWeaponComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* ThisTick) override;
    void SetTrigger(bool Held);
    void ClearHeldInput() { bTrigger=false; bPendingShot=false; }
    void BeginReload();
    void CancelReload();
    void InterruptReloadForSprint();
    bool CanAutoReload() const;
    void CancelAllOperations();
    bool SelectWeapon(int32 Index);
    void CycleWeapon();
    bool HasUsableWeapon() const;
    const FONECarriedWeaponState* GetSlotState(int32 Slot) const;
    uint64 GetInventoryRevision() const { return InventoryRevision; }
    uint64 GetRunId() const { return RunId; }
    const FONEWeaponDefinition* GetCatalogDefinition(EONEWeaponFamily Family,bool bUpgraded=false) const;
    int32 GetCatalogCount() const { return WeaponDefinitions.Num(); }
    void SetHandoffLocked(bool bLocked);
    bool IsHandoffLocked() const { return bHandoffLocked; }
    bool ReserveEquippedForUpgrade(FONEWeaponReservation& Out);
    bool MarkUpgradeReady(const FONEWeaponReservation& Token);
    bool CollectUpgrade(const FONEWeaponReservation& Token);
    bool RollbackUpgrade(const FONEWeaponReservation& Token);
    void InvalidateMachineTransactions();
    void ResetStarterLoadout();
    void GiveTestLoadout();
    bool IsFamilyRollEligible(EONEWeaponFamily Family) const;
    FONEWeaponAcquisitionPlan BuildAcquisitionPlan(EONEWeaponFamily Family) const;
    bool ApplyAcquisitionPlan(const FONEWeaponAcquisitionPlan& Plan);
    void RefillAllAmmo();
    void ClearEjectedCases();
    void RefreshEquippedPresentation();
    int32 GetAmmo() const { return GetAmmoForWeapon(EquippedIndex); }
    int32 GetReserveAmmo() const { return GetReserveAmmoForWeapon(EquippedIndex); }
    int32 GetAmmoForWeapon(int32 Index) const;
    int32 GetReserveAmmoForWeapon(int32 Index) const;
    int32 GetEquippedIndex() const { return EquippedIndex; }
    int32 GetPendingWeaponIndex() const { return PendingIndex; }
    int32 GetWeaponCount() const { return 2; }
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
    void GrantRoundAmmo();
    int32 GetTotalShotsFired() const { return ShotsFired; }
    int32 GetShellInsertCount() const { return ShellsInserted; }
    int32 GetMagazineCommitCount() const { return MagazinesCommitted; }
    int32 GetEjectionCount() const { return CasesEjected; }
    int32 GetAutomaticReloadCount() const { return AutomaticReloads; }
    int32 GetSprintReloadInterruptCount() const { return SprintReloadInterrupts; }
    uint64 GetLastShotId() const { return LastShotId; }
    int32 GetLastShotVictimCount() const { return LastShotVictimCount; }
    FVector GetLastShotMuzzle() const { return LastShotMuzzle; }
    uint32 GetLastShotPoseFrame() const { return LastShotPoseFrame; }
    uint32 GetLastShotPoseRevision() const { return LastShotPoseRevision; }
    uint64 GetLastShotFrame() const { return LastShotFrame; }
    int32 GetLastShotSoundIndex() const { return LastShotSoundIndex; }
    int32 GetLastShotSoundIndexForWeapon(int32 Index) const;
    int32 GetEjectionCountForWeapon(int32 Index) const;
    int32 GetLiveCaseCount() const;
    AONEWeaponCase* GetLastEjectedCase() const;
    int32 GetMaximumCases() const { return MaximumCases; }
    float GetCaseLifetime() const { return CaseLifetime; }
    const FONEWeaponDefinition& GetDefinition() const;
    const FONEWeaponDefinition* GetDefinitionForWeapon(int32 Index) const;
    UAnimSequence* GetReadyAnimation() const;
    UAnimSequence* GetActionAnimation(float& Time) const;
    float GetPumpFraction() const;
    float GetSlideFraction() const;
    bool DidReloadStartEmpty() const { return bReloadStartedEmpty; }
    bool ShouldShowLoadingShell() const;
    bool ShouldShowSeatedMagazine() const;
    bool ShouldShowHeldMagazine() const;
    int32 GetMagazineDropCount() const { return MagazinesDropped; }
    int32 GetLiveMagazineCount() const;
    AONEWeaponMagazine* GetLastDroppedMagazine() const;
    UPROPERTY(EditAnywhere,EditFixedSize,Category="Weapons") TArray<FONEWeaponDefinition> WeaponDefinitions;
    UPROPERTY(EditAnywhere,Category="Cases",meta=(ClampMin="1",ClampMax="64")) int32 MaximumCases=32;
    UPROPERTY(EditAnywhere,Category="Cases",meta=(ClampMin="1",ClampMax="15")) float CaseLifetime=6.f;
    UPROPERTY(EditAnywhere,Category="Magazines",meta=(ClampMin="1",ClampMax="32")) int32 MaximumMagazines=12;
    UPROPERTY(EditAnywhere,Category="Magazines",meta=(ClampMin="1",ClampMax="20")) float MagazineLifetime=8.f;
    // Candidate01 read-compatible mirrors. Edit the corresponding definition instead.
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") int32 MagazineSize=24;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float FireInterval=.16f;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float ReloadDuration=2.1f;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float Damage=32.f;
    UPROPERTY(VisibleAnywhere,Category="Current Weapon") float Range=2800.f;
private:
    void Fire();
    void EjectCase(int32 Index);
    void DropMagazine(int32 Index);
    bool IsSlotAvailable(int32 Slot) const;
    bool MatchesReservation(const FONEWeaponReservation& Token) const;
    void RefillSlot(int32 Slot,bool bMaximumReserve);
    void InstallWeapon(int32 Slot,EONEWeaponFamily Family,bool bUpgraded=false);
    void ChooseAvailableAfterRemoval();
    USoundBase* ChooseShotSound(int32 Index);
    void StartOperation(EONEWeaponOperation Next,int32 DefinitionIndex=-1);
    void FinishOperation();
    void AdvanceOperationEvents();
    void ProcessWeaponEvent(const FONEWeaponTimedEvent& Event);
    void StopOperationAudio();
    void PlayMechanical(USoundBase* Sound);
    const FONEWeaponOperationDefinition* FindOperation(int32 Index,EONEWeaponOperation Op) const;
    float FindEventTime(EONEWeaponEvent Event,float Fallback) const;
    UPROPERTY(Transient) TArray<FONECarriedWeaponState> Carried;
    UPROPERTY(Transient) TArray<TObjectPtr<UObject>> LoadedAssets;
    TArray<TWeakObjectPtr<UAudioComponent>> OperationAudio,ShotAudio;
    TArray<TWeakObjectPtr<AONEWeaponCase>> Cases;
    TArray<TWeakObjectPtr<AONEWeaponMagazine>> Magazines;
    FONEWeaponReservation ActiveReservation;
    uint64 RunId=0,InventoryRevision=0;
    bool bHandoffLocked=false,bReloadStartedEmpty=false;
    EONEWeaponOperation Operation=EONEWeaponOperation::Ready;
    int32 EquippedIndex=0,PendingIndex=-1,OperationIndex=0,NextEvent=0,OperationSerial=0;
    int32 ShotsFired=0,ShellsInserted=0,MagazinesCommitted=0,CasesEjected=0;
    int32 AutomaticReloads=0,SprintReloadInterrupts=0;
    int32 MagazinesDropped=0;
    uint64 LastShotId=0;
    uint64 LastShotFrame=0;
    uint32 LastShotPoseFrame=0,LastShotPoseRevision=0;
    int32 LastShotSoundIndex=INDEX_NONE;
    int32 LastShotVictimCount=0;
    FVector LastShotMuzzle=FVector::ZeroVector;
    bool bTrigger=false,bPendingShot=false,bLastHitKill=false;
    float OperationStart=0,LastShot=-100,LastEmpty=-100,LastHit=-100;
};
