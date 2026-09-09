#include "ONE06CaptureComponent.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AudioDeviceManager.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSubmix.h"
#include "Async/Async.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace ONE06CaptureDetails
{
    TWeakObjectPtr<UONE06CaptureComponent> ActiveCapture;
    // The engine master-WAV exporter is global. A failed/interrupted export
    // requires a fresh process, rather than risking replacement of its writer.
    bool bInterruptedAudioExport=false;
    constexpr double CaptureLimitSeconds=180.0;
    constexpr int32 CaptureLimitFrames=6000;
    constexpr double CaptureInterval=1.0/30.0;
    constexpr double AudioTailSeconds=0.4;
    FString CsvCell(FString Value)
    {
        Value.ReplaceInline(TEXT("\r"),TEXT(" "));
        Value.ReplaceInline(TEXT("\n"),TEXT(" "));
        Value.ReplaceInline(TEXT("\""),TEXT("\"\""));
        return TEXT("\"")+Value+TEXT("\"");
    }
    uint16 LE16(const uint8* P) { return uint16(P[0])|(uint16(P[1])<<8); }
    uint32 LE32(const uint8* P) { return uint32(P[0])|(uint32(P[1])<<8)|(uint32(P[2])<<16)|(uint32(P[3])<<24); }
}

UONE06CaptureComponent::UONE06CaptureComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.bStartWithTickEnabled=false;
    PrimaryComponentTick.bTickEvenWhenPaused=true;
}

bool UONE06CaptureComponent::IsAnyCaptureActive()
{
    return ONE06CaptureDetails::ActiveCapture.IsValid()||ONE06CaptureDetails::bInterruptedAudioExport;
}

bool UONE06CaptureComponent::IsProfiling()
{
#if CSV_PROFILER
    if (FCsvProfiler* Profiler=FCsvProfiler::Get())
        if (Profiler->IsCapturing()||Profiler->IsWritingFile()||Profiler->IsEndCapturePending()) return true;
#endif
    const TCHAR* Command=FCommandLine::Get();
    FString Token;
    while (FParse::Token(Command,Token,false))
        if (Token.StartsWith(TEXT("-"))&&
            (Token.Contains(TEXT("profile"),ESearchCase::IgnoreCase)||
             Token.Contains(TEXT("csvcapture"),ESearchCase::IgnoreCase))) return true;
    return false;
}

bool UONE06CaptureComponent::BeginCapture(const FString& InFolder)
{
    if (State!=ECaptureState::Idle) return false;
    if (!GetWorld()||!GetWorld()->GetGameViewport()) { Fail(TEXT("A live game viewport is required"));return false; }
    if (IsProfiling()) { Fail(TEXT("Capture and profiling cannot overlap"));return false; }
    if (IsAnyCaptureActive()) { Fail(TEXT("Another capture or interrupted audio export owns this process"));return false; }
    if (FScreenshotRequest::IsScreenshotRequested()) { Fail(TEXT("Another viewport screenshot is pending"));return false; }
    FString Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    FPaths::NormalizeDirectoryName(Saved);
    Folder=FPaths::IsRelative(InFolder)?Saved/InFolder:FPaths::ConvertRelativePathToFull(InFolder);
    FPaths::NormalizeDirectoryName(Folder);
    if (InFolder.IsEmpty()||!FPaths::CollapseRelativeDirectories(Folder)||
        !FPaths::IsUnderDirectory(Folder,Saved)||Folder.Equals(Saved,ESearchCase::IgnoreCase))
    { Folder.Reset();Fail(TEXT("Capture output must be a dedicated directory inside Project Saved"));return false; }
    TArray<FString> ExistingFrames;
    IFileManager::Get().FindFiles(ExistingFrames,*(Folder/TEXT("frame_*.jpg")),true,false);
    if (ExistingFrames.Num()||IFileManager::Get().FileExists(*(Folder/TEXT("frames.csv")))||
        IFileManager::Get().FileExists(*(Folder/TEXT("capture.json")))||
        IFileManager::Get().FileExists(*(Folder/TEXT("gameplay_master.wav"))))
    { Folder.Reset();Fail(TEXT("Capture artifacts already exist; use a fresh output directory"));return false; }
    if (!IFileManager::Get().MakeDirectory(*Folder,true)) { Folder.Reset();Fail(TEXT("Could not create capture output directory"));return false; }
    auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this);
    if (!Mixer) { Fail(TEXT("An active audio mixer is required"));return false; }
    auto Master=Mixer->GetMasterSubmix().Pin();
    if (!Master) { Fail(TEXT("The master audio submix is unavailable"));return false; }
    Mixer->AudioRenderThreadCommand([Master](){ Master->SetAutoDisable(false); });
    ONE06CaptureDetails::ActiveCapture=this;bOwnsLease=true;
    FramesCsv=TEXT("file,audio_seconds,world_seconds,phase,label,engine_frame,width,height\n");
    SetPhaseLabel(TEXT("unlabelled"));
    UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&UONE06CaptureComponent::Screenshot);
    bBoundScreenshot=true;
    UAudioMixerBlueprintLibrary::StartRecordingOutput(this,float(ONE06CaptureDetails::CaptureLimitSeconds));
    AudioStart=FPlatformTime::Seconds();LastRequest=AudioStart-ONE06CaptureDetails::CaptureInterval;
    bRecordingAudio=true;State=ECaptureState::Capturing;
    SetComponentTickEnabled(true);
    return true;
}

void UONE06CaptureComponent::SetPhaseLabel(const FString& Label)
{
    if (State!=ECaptureState::Idle&&State!=ECaptureState::Capturing) return;
    FString Clean=Label.Left(128);
    Clean.ReplaceInline(TEXT("\r"),TEXT(" "));Clean.ReplaceInline(TEXT("\n"),TEXT(" "));
    if (Clean.IsEmpty()) Clean=TEXT("unlabelled");
    if (PhaseLabels.Num()==0||Clean!=PhaseLabel)
    { PhaseLabel=Clean;Phase=PhaseLabels.Add(Clean); }
}

void UONE06CaptureComponent::CaptureTick(const FString& Label)
{
    SetPhaseLabel(Label);Pump();
}

void UONE06CaptureComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime,TickType,ThisTickFunction);Pump();
}

bool UONE06CaptureComponent::FinishCaptureAndWait()
{
    if (State==ECaptureState::Capturing)
    { State=ECaptureState::Finishing;FinishRequested=FPlatformTime::Seconds(); }
    Pump();return State==ECaptureState::Complete;
}

void UONE06CaptureComponent::Pump()
{
    if (State==ECaptureState::Idle||State==ECaptureState::Complete||State==ECaptureState::Failed) return;
    const double Now=FPlatformTime::Seconds();
    if (IsProfiling()) { Fail(TEXT("Profiling started while capture was active"));return; }
    if (bRecordingAudio&&Now-AudioStart>=ONE06CaptureDetails::CaptureLimitSeconds)
    { Fail(TEXT("Capture exceeded the hard 180 second recording limit"));return; }
    if (ImageWrite.IsValid()&&ImageWrite.IsReady())
    {
        const bool Saved=ImageWrite.Get();ImageWrite=TFuture<bool>();
        if (!Saved) { Fail(TEXT("A full viewport JPEG could not be written"));return; }
        const FFrameRecord& R=WritingFrame;
        FramesCsv+=FString::Printf(TEXT("%s,%.9f,%.9f,%d,%s,%llu,%d,%d\n"),
            *ONE06CaptureDetails::CsvCell(R.File),R.AudioSeconds,R.WorldSeconds,R.Phase,*ONE06CaptureDetails::CsvCell(R.Label),
            static_cast<unsigned long long>(R.EngineFrame),R.Width,R.Height);
        ++FramesWritten;
        if (FirstFrameSeconds<0) FirstFrameSeconds=R.AudioSeconds;
        LastFrameSeconds=R.AudioSeconds;
    }
    if (!PendingFile.IsEmpty()&&Now-RequestStarted>5.0)
    { Fail(TEXT("A requested viewport screenshot did not complete within five seconds"));return; }
    if (ImageWrite.IsValid()&&Now-LastScreenshot>10.0)
    { Fail(TEXT("The bounded JPEG writer did not finish within ten seconds"));return; }
    if (State==ECaptureState::Capturing)
    {
        if (FramesReceived>=ONE06CaptureDetails::CaptureLimitFrames) { Fail(TEXT("Capture reached the hard 6000 frame limit"));return; }
        if (PendingFile.IsEmpty()&&!ImageWrite.IsValid()&&!FScreenshotRequest::IsScreenshotRequested()&&Now-LastRequest>=ONE06CaptureDetails::CaptureInterval)
        {
            PendingFile=FString::Printf(TEXT("frame_%05d.jpg"),FramesReceived);
            PendingLabel=PhaseLabel;PendingPhase=Phase;
            LastRequest=RequestStarted=Now;
            FScreenshotRequest::RequestScreenshot(Folder/PendingFile,true,false);
        }
    }
    else if (State==ECaptureState::Finishing&&PendingFile.IsEmpty()&&!ImageWrite.IsValid()&&
        Now-FMath::Max(FinishRequested,LastScreenshot)>=ONE06CaptureDetails::AudioTailSeconds)
    {
        if (!FramesWritten) { Fail(TEXT("Capture finished without any viewport frames"));return; }
        StopAudio();State=ECaptureState::WritingAudio;
    }
    else if (State==ECaptureState::WritingAudio)
    {
        const int64 Bytes=IFileManager::Get().FileSize(*(Folder/TEXT("gameplay_master.wav")));
        if (Bytes!=LastWaveBytes) { LastWaveBytes=Bytes;WaveSizeChangedAt=Now; }
        if (Bytes>44&&Now-WaveSizeChangedAt>=0.5&&ReadWaveFacts())
        {
            if (LastFrameSeconds>=WaveDuration)
            { Fail(TEXT("The finalized master WAV does not cover the final screenshot timestamp"));return; }
            if (!WriteRecords(true)) { Fail(TEXT("Capture CSV or metadata could not be finalized"));return; }
            State=ECaptureState::Complete;
            if (bOwnsLease) { ONE06CaptureDetails::ActiveCapture.Reset();bOwnsLease=false; }
            SetComponentTickEnabled(false);
            UE_LOG(LogTemp,Display,TEXT("ONE06_CAPTURE_COMPLETE frames=%d audio_seconds=%.6f"),FramesWritten,WaveDuration);
        }
        else if (Now-AudioStopped>15.0)
            Fail(TEXT("The master WAV did not finalize as stable, nonempty PCM within fifteen seconds"));
    }
}

void UONE06CaptureComponent::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFile.IsEmpty()||(State!=ECaptureState::Capturing&&State!=ECaptureState::Finishing)) return;
    const double Now=FPlatformTime::Seconds();
    if (Width<=0||Height<=0||int64(Width)*Height!=Colors.Num()||ImageWrite.IsValid())
    { Fail(TEXT("The viewport callback returned invalid pixels or overlapped a writer"));return; }
    if (FramesReceived>=ONE06CaptureDetails::CaptureLimitFrames||Now-AudioStart>=ONE06CaptureDetails::CaptureLimitSeconds)
    { Fail(TEXT("A screenshot exceeded a hard capture limit"));return; }
    if (!FramesReceived) { FirstWidth=Width;FirstHeight=Height; }
    if (Width!=FirstWidth||Height!=FirstHeight)
    { Fail(TEXT("The viewport changed size during capture"));return; }
    WritingFrame.File=PendingFile;WritingFrame.Label=PendingLabel;
    WritingFrame.AudioSeconds=Now-AudioStart;WritingFrame.WorldSeconds=GetWorld()->GetTimeSeconds();
    WritingFrame.EngineFrame=GFrameCounter;WritingFrame.Phase=PendingPhase;
    WritingFrame.Width=Width;WritingFrame.Height=Height;
    const FString Path=Folder/PendingFile;
    TArray<FColor> Pixels=Colors;
    PendingFile.Reset();LastScreenshot=Now;++FramesReceived;
    ImageWrite=Async(EAsyncExecution::ThreadPool,[Path,Width,Height,Pixels=MoveTemp(Pixels)]()
    {
        return FImageUtils::SaveImageByExtension(*Path,FImageView(Pixels.GetData(),Width,Height),85);
    });
}

void UONE06CaptureComponent::StopAudio()
{
    if (bBoundScreenshot) { UGameViewportClient::OnScreenshotCaptured().RemoveAll(this);bBoundScreenshot=false; }
    if (bRecordingAudio)
    {
        AudioStopped=FPlatformTime::Seconds();
        UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,
            TEXT("gameplay_master"),Folder);
        bRecordingAudio=false;WaveSizeChangedAt=FPlatformTime::Seconds();
    }
}

bool UONE06CaptureComponent::ReadWaveFacts()
{
    TUniquePtr<FArchive> File(IFileManager::Get().CreateFileReader(*(Folder/TEXT("gameplay_master.wav"))));
    if (!File||File->TotalSize()!=LastWaveBytes) return false;
    uint8 Header[12];File->Serialize(Header,12);
    if (FMemory::Memcmp(Header,"RIFF",4)||FMemory::Memcmp(Header+8,"WAVE",4)||
        int64(ONE06CaptureDetails::LE32(Header+4))+8!=LastWaveBytes) return false;
    bool bFormat=false,bData=false;
    int32 Channels=0,Rate=0,Bits=0,BlockAlign=0,ByteRate=0;
    int64 DataBytes=0,FormatOffset=-1,DataOffset=-1;
    uint32 FormatBytes=0;
    while (!File->IsError()&&File->Tell()+8<=LastWaveBytes)
    {
        const int64 ChunkOffset=File->Tell();
        uint8 Chunk[8];File->Serialize(Chunk,8);
        const uint32 Count=ONE06CaptureDetails::LE32(Chunk+4);
        const int64 End=File->Tell()+int64(Count);
        if (End>LastWaveBytes) return false;
        if (!FMemory::Memcmp(Chunk,"fmt ",4))
        {
            if (bFormat||Count<16) return false;
            uint8 Format[16];File->Serialize(Format,16);
            if (ONE06CaptureDetails::LE16(Format)!=1) return false;
            Channels=ONE06CaptureDetails::LE16(Format+2);Rate=int32(ONE06CaptureDetails::LE32(Format+4));
            ByteRate=int32(ONE06CaptureDetails::LE32(Format+8));FormatOffset=ChunkOffset;FormatBytes=Count;
            BlockAlign=ONE06CaptureDetails::LE16(Format+12);Bits=ONE06CaptureDetails::LE16(Format+14);bFormat=true;
        }
        else if (!FMemory::Memcmp(Chunk,"data",4))
        { if (bData) return false;DataBytes=Count;DataOffset=ChunkOffset;bData=true; }
        const int64 Next=End+(Count&1u);
        if (Next>LastWaveBytes) return false;
        File->Seek(Next);
    }
    if (File->IsError()||File->Tell()!=LastWaveBytes||!bFormat||!bData||Channels<1||Channels>16||Rate<8000||Rate>384000||
        Bits!=16||DataBytes<=0) return false;
    const int32 FrameBytes=Channels*2;
    // UE5.7 Audio.cpp SerializeWaveFile writes BlockAlign=2 even for stereo,
    // while ByteRate and interleaved PCM are correct. Recognize only that exact
    // 44-byte exporter layout; never use the erroneous field for sample counts.
    const bool UnrealBlockAlignQuirk=Channels>1&&BlockAlign==2&&FormatOffset==12&&FormatBytes==16&&
        DataOffset==36&&LastWaveBytes==44+DataBytes;
    if (ByteRate!=Rate*FrameBytes||DataBytes%FrameBytes||
        (BlockAlign!=FrameBytes&&!UnrealBlockAlignQuirk)) return false;
    WaveChannels=Channels;WaveSampleRate=Rate;WaveBits=Bits;WaveDataBytes=DataBytes;
    WaveHeaderBlockAlign=BlockAlign;bWaveUnrealBlockAlignQuirk=UnrealBlockAlignQuirk;
    WaveDuration=double(DataBytes)/double(Rate*FrameBytes);
    return WaveDuration>0;
}

bool UONE06CaptureComponent::WriteRecords(bool Complete)
{
    if (Folder.IsEmpty()) return false;
    TSharedRef<FJsonObject> Record=MakeShared<FJsonObject>();
    Record->SetStringField(TEXT("schema"),TEXT("one06.engine_capture.v1"));
    Record->SetStringField(TEXT("status"),Complete?TEXT("PASS"):TEXT("FAILED"));
    Record->SetStringField(TEXT("finished_utc"),FDateTime::UtcNow().ToIso8601());
    Record->SetStringField(TEXT("failure_reason"),FailureReason);
    Record->SetStringField(TEXT("capture_kind"),TEXT("Opt-in engine viewport and master mix; scripted fixture context is supplied by the owning check"));
    Record->SetBoolField(TEXT("native_input_evidence"),false);
    Record->SetBoolField(TEXT("performance_evidence"),false);
    Record->SetBoolField(TEXT("perceptual_audio_review"),false);
    Record->SetStringField(TEXT("timebase"),TEXT("audio_seconds is screenshot callback platform time minus the CPU timestamp immediately after StartRecordingOutput; audio render scheduling offset is not measured"));
    Record->SetStringField(TEXT("frame_policy"),TEXT("Actual full viewport including game HUD; JPEG quality 85; at most 30 requests per real second; one asynchronous writer; no fabricated frames, cropping, overlays or timestamp retiming"));
    Record->SetStringField(TEXT("source_binding"),TEXT("Owning run must bind this capture to its exact packaged source and runtime hashes"));
    Record->SetStringField(TEXT("frames_csv"),TEXT("frames.csv"));
    Record->SetStringField(TEXT("audio_file"),TEXT("gameplay_master.wav"));
    Record->SetNumberField(TEXT("frames_received"),FramesReceived);
    Record->SetNumberField(TEXT("frames_written"),FramesWritten);
    Record->SetBoolField(TEXT("screenshot_pending"),!PendingFile.IsEmpty());
    Record->SetBoolField(TEXT("image_writer_pending"),ImageWrite.IsValid());
    Record->SetNumberField(TEXT("width"),FirstWidth);Record->SetNumberField(TEXT("height"),FirstHeight);
    Record->SetNumberField(TEXT("first_frame_audio_seconds"),FirstFrameSeconds);
    Record->SetNumberField(TEXT("last_frame_audio_seconds"),LastFrameSeconds);
    Record->SetNumberField(TEXT("audio_duration_seconds"),WaveDuration);
    Record->SetNumberField(TEXT("audio_bytes"),LastWaveBytes);
    Record->SetNumberField(TEXT("audio_pcm_bytes"),WaveDataBytes);
    Record->SetNumberField(TEXT("audio_channels"),WaveChannels);
    Record->SetNumberField(TEXT("audio_sample_rate"),WaveSampleRate);
    Record->SetNumberField(TEXT("audio_bits_per_sample"),WaveBits);
    Record->SetNumberField(TEXT("audio_header_block_align"),WaveHeaderBlockAlign);
    Record->SetNumberField(TEXT("audio_computed_frame_bytes"),WaveChannels*2);
    Record->SetBoolField(TEXT("audio_unreal_block_align_quirk"),bWaveUnrealBlockAlignQuirk);
    Record->SetBoolField(TEXT("audio_finalized_stable_nonempty"),Complete);
    Record->SetNumberField(TEXT("minimum_real_audio_tail_seconds"),ONE06CaptureDetails::AudioTailSeconds);
    Record->SetNumberField(TEXT("actual_real_audio_tail_seconds"),AudioStopped>0?AudioStopped-FMath::Max(FinishRequested,LastScreenshot):0);
    Record->SetNumberField(TEXT("stop_audio_clock_seconds"),AudioStopped>0?AudioStopped-AudioStart:0);
    Record->SetNumberField(TEXT("maximum_recording_seconds"),ONE06CaptureDetails::CaptureLimitSeconds);
    Record->SetNumberField(TEXT("maximum_frames"),ONE06CaptureDetails::CaptureLimitFrames);
    TArray<TSharedPtr<FJsonValue>> Labels;
    for (int32 Index=0;Index<PhaseLabels.Num();++Index)
    {
        TSharedRef<FJsonObject> Item=MakeShared<FJsonObject>();
        Item->SetNumberField(TEXT("phase"),Index);Item->SetStringField(TEXT("label"),PhaseLabels[Index]);
        Labels.Add(MakeShared<FJsonValueObject>(Item));
    }
    Record->SetArrayField(TEXT("phases"),Labels);
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer=TJsonWriterFactory<>::Create(&Json);
    if (!FJsonSerializer::Serialize(Record,Writer)) return false;
    const bool CsvSaved=FFileHelper::SaveStringToFile(FramesCsv,*(Folder/TEXT("frames.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const bool JsonSaved=FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("capture.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    return CsvSaved&&JsonSaved;
}

void UONE06CaptureComponent::Fail(const FString& Reason)
{
    if (State==ECaptureState::Failed||State==ECaptureState::Complete) return;
    FailureReason=Reason;
    const bool HadAudio=bRecordingAudio||State==ECaptureState::WritingAudio;
    if (bOwnsLease&&!PendingFile.IsEmpty()) FScreenshotRequest::Reset();
    StopAudio();State=ECaptureState::Failed;
    if (HadAudio) ONE06CaptureDetails::bInterruptedAudioExport=true;
    if (bOwnsLease) { ONE06CaptureDetails::ActiveCapture.Reset();bOwnsLease=false; }
    WriteRecords(false);SetComponentTickEnabled(false);
    UE_LOG(LogTemp,Error,TEXT("ONE06_CAPTURE_FAILURE %s"),*FailureReason);
}

void UONE06CaptureComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    if (State!=ECaptureState::Idle&&State!=ECaptureState::Complete&&State!=ECaptureState::Failed)
        Fail(TEXT("The capture owner ended play before capture and audio finalization completed"));
    Super::EndPlay(Reason);
}
