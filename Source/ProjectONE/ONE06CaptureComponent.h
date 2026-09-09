#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Async/Future.h"
#include "ONE06CaptureComponent.generated.h"

/** Opt-in engine capture, never a native-input or performance measurement.
 * One full viewport request and one JPEG writer are allowed at a time. */
UCLASS(ClassGroup=(ONE))
class PROJECTONE_API UONE06CaptureComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONE06CaptureComponent();
    bool BeginCapture(const FString& InFolder);
    void SetPhaseLabel(const FString& Label);
    void CaptureTick(const FString& Label);
    // Nonblocking: call again (or let TickComponent poll) until true or HasFailed().
    bool FinishCaptureAndWait();
    bool HasFailed() const { return State==ECaptureState::Failed; }
    bool IsRecording() const { return bRecordingAudio; }
    bool IsComplete() const { return State==ECaptureState::Complete; }
    FString GetFailureReason() const { return FailureReason; }
    FString GetOutputFolder() const { return Folder; }
    int32 GetFrameCount() const { return FramesWritten; }
    static bool IsAnyCaptureActive();
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    enum class ECaptureState : uint8 { Idle,Capturing,Finishing,WritingAudio,Complete,Failed };
    struct FFrameRecord
    {
        FString File,Label;
        double AudioSeconds=0,WorldSeconds=0;
        uint64 EngineFrame=0;
        int32 Phase=0,Width=0,Height=0;
    };
    void Pump();
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    void StopAudio();
    void Fail(const FString& Reason);
    bool WriteRecords(bool Complete);
    bool ReadWaveFacts();
    static bool IsProfiling();
    ECaptureState State=ECaptureState::Idle;
    TFuture<bool> ImageWrite;
    FFrameRecord WritingFrame;
    TArray<FString> PhaseLabels;
    FString Folder,PhaseLabel,PendingFile,PendingLabel,FramesCsv,FailureReason;
    double AudioStart=0,LastRequest=0,RequestStarted=0,LastScreenshot=0;
    double FinishRequested=0,AudioStopped=0,WaveSizeChangedAt=0;
    double FirstFrameSeconds=-1,LastFrameSeconds=-1,WaveDuration=0;
    int64 LastWaveBytes=-1,WaveDataBytes=0;
    int32 FramesReceived=0,FramesWritten=0,Phase=0,PendingPhase=0;
    int32 FirstWidth=0,FirstHeight=0,WaveChannels=0,WaveSampleRate=0,WaveBits=0,WaveHeaderBlockAlign=0;
    bool bWaveUnrealBlockAlignQuirk=false;
    bool bRecordingAudio=false,bBoundScreenshot=false,bOwnsLease=false;
};
