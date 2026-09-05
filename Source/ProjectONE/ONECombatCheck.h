#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONECombatCheck.generated.h"
class AONEPlayer;
class AONEZombie;
/** Opt-in integration driver. Calls the same carried weapons and registered enemies as play. */
UCLASS()
class PROJECTONE_API AONECombatCheck : public AActor
{
    GENERATED_BODY()
public:
    AONECombatCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 Step);
    void Finish();
    void Compare(float Dt);
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEZombie> Target;
    int32 Stage=0,Failures=0,Ammo=0,Reserve=0,Count=0,Shots=0,Ejections=0,Frames=0,LastPhase=-1;
    float Elapsed=0,StageStart=0,PausedOperation=0,FootTravel=0;
    double PauseStart=0,AudioStart=0,LastCapture=0,FinishedAt=0;
    bool bComparison=false,bRecording=false,bFinished=false;
    FVector FirstFoot=FVector::ZeroVector;
    TArray<TWeakObjectPtr<AONEZombie>> SpawnCheckEnemies;
    TArray<FVector> SpawnCheckOrigins;
    FString Report,FrameReport,PendingFrame,Folder;
};
