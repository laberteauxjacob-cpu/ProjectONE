#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE03MovementCheck.generated.h"
class AONEPlayer;

/** Opt-in movement integration and timestamped gameplay-camera recording. */
UCLASS()
class PROJECTONE_API AONE03MovementCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE03MovementCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FString GetSegmentLabel() const { return Segment; }
private:
    void Check(bool Pass,const FString& Label);
    void PrepareTrial();
    void Finish();
    void Capture();
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    void RecordPose();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    int32 Trial=-1,Stage=0,Failures=0,Samples=0,Frames=0,TurnSamples=0;
    float Elapsed=0,StageStart=0,SpeedSum=0,MinSpeed=BIG_NUMBER,MaxSpeed=0,TurnFootTravel=0;
    double AudioStart=0,LastCapture=0,FinishedAt=0;
    bool bCapture=false,bManual=false,bRecording=false,bFinished=false;
    float ManualDuration=90;
    float LeftMinZ=BIG_NUMBER,LeftMaxZ=-BIG_NUMBER,RightMinZ=BIG_NUMBER,RightMaxZ=-BIG_NUMBER;
    int32 MovingShots=0;
    FVector InputDirection=FVector::ZeroVector,PreviousFoot=FVector::ZeroVector;
    FString Folder,Report,PoseReport,FrameReport,PendingFrame,Segment;
};
