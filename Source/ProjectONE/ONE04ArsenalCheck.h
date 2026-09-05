#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ONEWeaponTypes.h"
#include "ONE04ArsenalCheck.generated.h"
class AONEPlayer;
class AONEGameMode;
class AONEZombie;
class AONEWeaponMagazine;
class FJsonValue;
UCLASS()
class PROJECTONE_API AONE04ArsenalCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE04ArsenalCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NewStage);
    void Finish();
    void Key(const FKey& Key,EInputEvent Event);
    void Pulse(const FKey& Key);
    void PrepareVariant(int32 Index);
    void CompleteVariant();
    AActor* StaticBox(const FVector& Center,const FVector& Extent);
    AONEZombie* TargetAt(float Distance);
    void ClearTargets();
    void PrepareRayCase(int32 Index);
    void CheckRayCase();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEGameMode> GM;
    UPROPERTY() TArray<TObjectPtr<AONEZombie>> Targets;
    UPROPERTY() TObjectPtr<AActor> Floor;
    UPROPERTY() TObjectPtr<AActor> Cover;
    TWeakObjectPtr<AONEWeaponMagazine> Dropped,FirstBudget;
    TArray<TPair<FKey,float>> Releases;
    TArray<TSharedPtr<FJsonValue>> Records;
    FString Csv;
    FVector RestPosition=FVector::ZeroVector;
    int32 Stage=0,Variant=0,Checks=0,Failures=0,Shots=0,Ejections=0,Drops=0,Ammo=0,Reserve=0,Inserts=0,BudgetCount=0,RayCase=0;
    float StageStart=0,DropAt=0,DropTime=.28f,LastTrace=-1;
    double StartReal=0,StageReal=0,FinishedReal=0;
    bool bFinished=false,bSawShotBeforeEject=false,bSawPumpBeforeEject=false,bEarlyCase=false,bLateCase=false;
};
