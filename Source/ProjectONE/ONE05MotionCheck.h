#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ONE05MotionCheck.generated.h"
class AONEPlayer;
class AONEPlayerController;
class AONEGameMode;
class AONEZombie;

/** Opt-in actual input/pose capture. Fixture resets and scripted input are disclosed. */
UCLASS()
class PROJECTONE_API AONE05MotionCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE05MotionCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FString GetSegmentLabel() const { return Segment; }
private:
    void Check(bool Pass,const FString& Label);
    void Key(const FKey& K,bool Down);
    void ReleaseKeys();
    void AimAt(const FVector& WorldPoint);
    void EnterPhase();
    void CompletePhase();
    void RunPhase(float Dt);
    void Observe();
    void SpawnFixture(const FVector& Point,bool Hold);
    void Capture();
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    void Finish(bool Complete);
    void Finalize();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEPlayerController> Controller;
    UPROPERTY() TObjectPtr<AONEGameMode> Mode;
    UPROPERTY() TObjectPtr<AONEZombie> Enemy;
    TSet<FKey> Held;
    int32 Phase=-1,Checks=0,Failures=0,Frames=0,Samples=0,ObservedShots=0;
    int32 LiveOutcomes=0,KillOutcomes=0,CorpseOutcomes=0,MinorSamples=0,PlayerReactionSamples=0;
    int32 CursorFallbacks=0,AttackAudioStart=0;
    float Elapsed=0,PhaseStart=0,SpeedSum=0,SpeedMin=BIG_NUMBER,SpeedMax=0;
    float AimMaxDegrees=0,DirectionMaxDegrees=0;
    float PhaseHealth=0,NextFire=0,ReleaseFireAt=0,DeathAt=-1;
    float FootMin[2]={BIG_NUMBER,BIG_NUMBER},FootMax[2]={-BIG_NUMBER,-BIG_NUMBER};
    FVector Origin=FVector(0,360,98),MoveDirection=FVector::ZeroVector;
    double AudioStart=0,LastCapture=0,LastScreenshotCompletedAt=0,FinishRequestedAt=0,FinishedAt=0;
    bool bCapture=false,bRecording=false,bAttackStarted=false,bAttackSettled=false;
    bool bMovementPressed=false,bCorpseRetreatComplete=false,bFinishing=false,bFinished=false,bComplete=false;
    FString Folder,Segment,Report,FramesCsv,PosesCsv,InputCsv,ChaptersCsv,PendingFrame;
};
