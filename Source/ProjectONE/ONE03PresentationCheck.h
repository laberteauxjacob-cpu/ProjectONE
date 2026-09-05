#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE03PresentationCheck.generated.h"
class AONEPlayer;
/** Opt-in Stage C gameplay-camera comparison; never spawned in ordinary play. */
UCLASS()
class PROJECTONE_API AONE03PresentationCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE03PresentationCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    FString GetSegmentLabel() const { return Segment; }
private:
    void Check(bool Pass,const FString& Label);
    void PrepareTrial();
    void Observe();
    void Finish();
    void Capture();
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    int32 Trial=-1,Stage=0,Failures=0,Frames=0,StartShots=0,FlashSamples=0;
    int32 ShotPoseSamples=0,SoundRepeatErrors=0,StalePoseErrors=0,ShapeSamples=0;
    int32 LastSoundByWeapon[2]={-1,-1};
    int32 ReloadAmmoBefore=0,ReloadCommitsBefore=0;
    float Elapsed=0,StageStart=0,LastPulse=-100,MaxAttachmentError=0,MaxShotPoseError=0;
    float MaxShapeAttachmentError=0;
    double AudioStart=0,LastCapture=0,FinishedAt=0;
    bool bCapture=false,bRecording=false,bFinished=false,bReloadRequested=false;
    uint64 ObservedShotFrame=0;
    FString Folder,Report,FrameReport,PoseReport,PendingFrame,Segment;
};
