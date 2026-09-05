#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponTypes.h"
#include "ONE05WeaponCheck.generated.h"
class AONEPlayer;
class UONEWeaponComponent;
class FJsonValue;

/** Opt-in real-component regression; never installed in ordinary play. */
UCLASS()
class PROJECTONE_API AONE05WeaponCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE05WeaponCheck();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NewStage);
    void Prepare(int32 NewVariant);
    void Tap();
    void Finish();
    void Observe();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<UONEWeaponComponent> Weapon;
    int32 Stage=0,Variant=0,Checks=0,Failures=0,ExpectedRate=0;
    int32 Shots=0,Ammo=0,Reserve=0,Dry=0,Drops=0,Transfers=0,Ejections=0,Rejected=0,LastObservedShots=0;
    uint64 SavedRun=0,SavedInstance=0,PressFrame=0;
    float StageStart=0,PausedOperation=0;
    double StartedReal=0,StageReal=0,FinishedReal=0;
    bool bFinished=false,bCadence=false,bLateTap=false;
    FDelegateHandle LateTickHandle;
    TArray<double> ShotTimes;
    TArray<TSharedPtr<FJsonValue>> Records;
    FONEWeaponAcquisitionPlan StalePlan;
    FString Csv;
};
