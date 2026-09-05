#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ONEWeaponTypes.h"
#include "ONE04ProgressionCheck.generated.h"
class AONEPlayer;
class AONEGameMode;
class AONEMysteryBox;
class AONEUpgradeMachine;
class AONEProgressionMachine;
class FJsonValue;
/** Opt-in real machine/input integration. Point setup and actor positioning are
 * explicitly declared fixtures; hold timing runs through production bindings. */
UCLASS()
class PROJECTONE_API AONE04ProgressionCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE04ProgressionCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NewStage);
    void Finish();
    void Key(const FKey& Key,EInputEvent Event);
    bool Approach(AONEProgressionMachine* Machine);
    void SetPoints(int32 Points);
    void Trace();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEGameMode> GM;
    UPROPERTY() TObjectPtr<AONEMysteryBox> Box;
    UPROPERTY() TObjectPtr<AONEUpgradeMachine> Upgrade;
    FONEWeaponReservation Token;
    FTransform UpgradeTransform=FTransform::Identity;
    FVector MoveOrigin=FVector::ZeroVector;
    uint64 Instance=0,Receipt=0;
    int32 Stage=0,Checks=0,Failures=0,Count=0,ShotCount=0;
    float StageStart=0,AcceptedAt=0,LastTrace=-1;
    double StartReal=0,StageReal=0,FinishedReal=0,ElapsedOffset=0;
    bool bFinished=false;
    TArray<TSharedPtr<FJsonValue>> Records;
    FString Csv;
};
