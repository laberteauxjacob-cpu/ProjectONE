#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE03WeaponCheck.generated.h"
class AONEPlayer;

/** Opt-in Candidate03 regression using the real player and carried weapon operations. */
UCLASS()
class PROJECTONE_API AONE03WeaponCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE03WeaponCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NextStage);
    void Prepare(int32 Slot);
    void Finish();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    int32 Stage=0,CaseIndex=0,Failures=0,Checks=0;
    int32 Shots=0,Ammo=0,Reserve=0,Automatic=0,Ejections=0,Transfers=0,Interrupts=0;
    float StageStart=0,PausedOperation=0;
    double StartReal=0,StageReal=0,FinishedReal=0;
    bool bFinished=false;
    FVector InterruptOrigin=FVector::ZeroVector;
    FString Report;
};
