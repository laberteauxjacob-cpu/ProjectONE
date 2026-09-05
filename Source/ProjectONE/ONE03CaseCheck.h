#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE03CaseCheck.generated.h"
class AONEPlayer;
class AONEWeaponCase;

/** Opt-in actual-emission/case lifecycle check. No screenshot or audio capture. */
UCLASS()
class PROJECTONE_API AONE03CaseCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE03CaseCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NextStage);
    void Prepare(int32 Slot);
    void InspectCase(AONEWeaponCase* Case,int32 Slot,uint64 Shot);
    void ObserveLifecycle();
    void Finish();
    int32 ActualCaseCount() const;
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    TWeakObjectPtr<AONEWeaponCase> FocusCase,FirstBudgetCase;
    TSet<uint64> ExtendedCaseShots;
    int32 Stage=0,Failures=0,Checks=0,Shots=0,Ejections=0,Ammo=0,Reserve=0;
    int32 PreviousBounces=0,GravitySamples=0,HorizontalSamples=0,MaximumLive=0,TargetShots=0;
    uint64 PumpShot=0;
    float StageStart=0,BirthObserved=0,PreviousSampleTime=0,SettledAt=-1,GravitySum=0,MaxHorizontalVelocityError=0;
    float PumpEventTime=.18f,PauseLife=0;
    double StartReal=0,StageReal=0,FinishedReal=0;
    bool bFinished=false,bHaveSample=false,bSawBounce=false,bSawRebound=false,bSawSettled=false,bStableSettled=true;
    FVector PreviousVelocity=FVector::ZeroVector,PreviousLocation=FVector::ZeroVector,SettledLocation=FVector::ZeroVector;
    FVector PauseLocation=FVector::ZeroVector,PauseVelocity=FVector::ZeroVector;
    FQuat LaunchRotation=FQuat::Identity;
    FString Report,Timeline;
};
