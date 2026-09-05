#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponTypes.h"
#include "Async/Future.h"
#include "ONE03PhysicalityCheck.generated.h"
class AONEPlayer;
class AONEZombie;
/** Opt-in integration/camera scenario. No capture work in check/profile mode. */
UCLASS()
class PROJECTONE_API AONE03PhysicalityCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE03PhysicalityCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FString GetSegmentLabel() const { return Segment; }
private:
    void Check(bool Pass,const FString& Label);
    void PreparePhase();
    void FinishPhase();
    void ClearFixture();
    void Damage(AONEZombie* Z,EONEHitRegion Region,float Amount,float Trauma,const FVector& Direction);
    AONEZombie* Spawn(const FVector& Location);
    void Observe(float Dt);
    void ObserveCorpseMotion();
    void Finish();
    void WriteResults();
    bool StartProfile();
    bool FinishProfileWrite();
    void Capture();
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    TArray<TWeakObjectPtr<AONEZombie>> Subjects;
    TArray<FVector> StartPositions;
    TArray<float> SubjectProgress;
    TArray<FQuat> FinalPelvisRotations;
    int32 Phase=-1,FirstPhase=0,Failures=0,Checks=0,Frames=0,ActionIndex=0,AttackFrames=0;
    int32 MaxBodies=0,MaxAwake=0,MaxWounds=0,MaxDrops=0,MaxDecals=0,MaxCorpses=0,MaxParts=0,Spawned=0;
    float Elapsed=0,PhaseStart=0,NextAction=0,NextSample=0,MaximumVelocity=0,MaximumOverlap=0;
    float ClosestPair=BIG_NUMBER,MinPlayerHealth=100,StartPoolRadius=0,MaxPoolRadius=0;
    float MinorDeposited=0,InitialVolume=0,MaxVolume=0,MaxProgress=0,CoexistenceSeconds=0;
    uint64 FixtureShot=900000000;
    uint32 CleanupGeneration=0;
    double AudioStart=0,LastCapture=0,FinishedAt=0;
    double CsvStartRequestedAt=0;
    bool bCapture=false,bRecording=false,bFinished=false,bPoolBaseline=false,bInvalidPhysicsSample=false;
    bool bProfile=false,bCsvStartRequested=false,bCsvStarted=false,bCsvFinished=false,bResultsWritten=false;
    bool bRestTelemetry=false;
    bool bFrozenWakeTested=false;
    struct FCorpseMotionSample
    {
        float At=-1.f,LowMotionSeconds=0.f;
        TMap<FName,FTransform> BoneTransforms;
    };
    TMap<TWeakObjectPtr<AONEZombie>,FCorpseMotionSample> CorpseMotionSamples;
    TSharedFuture<FString> CsvCompletion;
    FString Folder,Report,FrameReport,Telemetry,PendingFrame,Segment;
    FString CsvFolder;
    FString CorpseMotionTelemetry;
};
