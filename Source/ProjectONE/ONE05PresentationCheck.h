#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ONEWeaponTypes.h"
#include "Async/Future.h"
#include "ONE05PresentationCheck.generated.h"
class AONEPlayer;
class AONEPlayerController;
class AONEGameMode;
class AONEProgressionMachine;

/** Opt-in production-input demonstration, passive manual recorder or media-free CSV profile.
 * Scripted input is dispatched across real frames; it is not native human input. */
UCLASS()
class PROJECTONE_API AONE05PresentationCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE05PresentationCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FString GetSegmentLabel() const { return Segment; }
private:
    enum class EStep : uint8 { Wait,Tap,Walk,Retreat,Hold,WaitState,Select,Fire,NearAim,Reload,Verify,StartProfile,Combat,ClickResume,Death };
    struct FStep
    {
        EStep Kind=EStep::Wait;
        FString Label;
        FKey Key;
        int32 Machine=0,State=0;
        float Seconds=1;
        EONEWeaponFamily Family=EONEWeaponFamily::Invalid;
        bool bUpgraded=false;
    };
    void Plan();
    void EnterStep();
    void Advance();
    void RunStep(float Dt);
    void Key(const FKey& K,bool Down);
    void ReleaseKeys();
    void AimAt(const FVector& Point);
    void FireInput(float Time);
    bool WalkTo(AONEProgressionMachine* Machine);
    FVector ContactPoint(const AONEProgressionMachine* Machine) const;
    AONEProgressionMachine* Machine(int32 Index) const;
    void Observe(float Dt);
    void ReplenishEnemies();
    void Check(bool Pass,const FString& Label);
    void Capture();
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    void Finish(bool Complete);
    void WriteResults();
    bool StartProfile();
    bool FinishProfileWrite();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEPlayerController> Controller;
    UPROPERTY() TObjectPtr<AONEGameMode> Mode;
    UPROPERTY() TObjectPtr<AONEProgressionMachine> Box;
    UPROPERTY() TObjectPtr<AONEProgressionMachine> Upgrade;
    TArray<FStep> Steps;
    TSet<FKey> Held;
    TSharedFuture<FString> CsvCompletion;
    int32 Phase=-1,Checks=0,Failures=0,Frames=0,EnemyCount=6,Live=0,Spawned=0;
    int32 ShotsAtStep=0,DropsAtStep=0,HoldCountAtStep=0,WalkLeg=0,FirePulses=0;
    int32 CompletedConfigurations=0,ProfileSamples=0,ExactCountSamples=0;
    int32 ProfileMaximumLive=0;
    uint64 StepFirstFrame=0;
    FVector RetreatStart=FVector::ZeroVector;
    float Elapsed=0,StepStart=0,NextObservation=0,NextReplenish=0,NextFire=0;
    float BothActiveSeconds=0,ProfileSeconds=0,ManualDuration=120;
    double AudioStart=0,LastCapture=0,FinishedAt=0,CsvRequestAt=0,LastDriverTime=0,AudioTailUntil=0;
    bool bCapture=false,bManual=false,bProfile=false,bRecording=false,bFinished=false;
    bool bComplete=false,bResultsWritten=false,bCsvRequested=false,bCsvStarted=false,bCsvFinished=false;
    bool bFinishRequested=false,bRequestedComplete=false;
    bool bProfileUpgraded=false;
    bool bAudioTailWaiting=false;
    int32 NearObservedShots=0,NearRequestedBand=0,NearBandShots[3]={0,0,0};
    FString Folder,Segment,Report,FramesCsv,InputCsv,ObservationCsv,ChaptersCsv,PendingFrame,CsvFolder;
};
