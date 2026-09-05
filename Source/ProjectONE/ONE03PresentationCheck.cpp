#include "ONE03PresentationCheck.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AudioMixerBlueprintLibrary.h"
#include "AudioDeviceManager.h"
#include "AudioMixerDevice.h"
#include "AudioMixerSubmix.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Components/PointLightComponent.h"
#include "CoreGlobals.h"

void AONE03PresentationCheck::Check(bool Pass,const FString& Label)
{
    Failures+=Pass?0:1; Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE03_PRESENTATION %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}

void AONE03PresentationCheck::Capture()
{
    if (!bCapture || !bRecording || bFinished) return;
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1.0/30.0)
    { LastCapture=Now; PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames); FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false); }
}

void AONE03PresentationCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty() || !Player) return;
    // Timestamp the captured pixels before synchronous JPEG encoding and I/O.
    const double CapturedAt=FPlatformTime::Seconds()-AudioStart;
    auto* W=Player->GetWeaponComponent();
    const FString Row=FString::Printf(TEXT("%s,%.6f,%.6f,%d,%d,%d,%d,%d\n"),*PendingFrame,CapturedAt,Elapsed,Trial,W->GetEquippedIndex(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()));
    if (FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),Width,Height),85))
    {
        FrameReport+=Row; ++Frames;
    }
    PendingFrame.Empty();
}

void AONE03PresentationCheck::Finish()
{
    if (bFinished) return; bFinished=true; FinishedAt=FPlatformTime::Seconds();
    Player->ReleaseHeldInputs();
    if (bRecording)
    { UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("gameplay_master"),FPaths::ConvertRelativePathToFull(Folder)); bRecording=false; }
    Report+=FString::Printf(TEXT("\nFailures: %d\nCaptured frames: %d\n"),Failures,Frames);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    FFileHelper::SaveStringToFile(PoseReport,*(Folder/TEXT("observations.csv")));
    FFileHelper::SaveStringToFile(FrameReport,*(Folder/TEXT("frames.csv")));
    UE_LOG(LogTemp,Display,TEXT("ONE03_PRESENTATION_COMPLETE failures=%d"),Failures);
}

AONE03PresentationCheck::AONE03PresentationCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE03PresentationCheck::BeginPlay()
{
    Super::BeginPlay();
    bCapture=FParse::Param(FCommandLine::Get(),TEXT("ONE03PresentationCapture"));
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03")/(bCapture?TEXT("PresentationCapture"):TEXT("Presentation"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate03 Stage C production weapon presentation\nScripted normal gameplay components; visual and audible approval are separate from these checks.\n\n");
    FrameReport=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation\n");
    PoseReport=TEXT("seconds,trial,speed,actor_yaw,muzzle_x,muzzle_y,muzzle_z,light_intensity,light_attachment_error_cm,shots,shot_sound_index,frame,shot_frame,shot_pose_frame,shape_visible,shape_attachment_error_cm,carbine_ejections,shotgun_ejections,live_cases\n");
    if (bCapture)
    {
        UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE03PresentationCheck::Screenshot);
        if (auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
            if (auto Master=Mixer->GetMasterSubmix().Pin()) Mixer->AudioRenderThreadCommand([Master](){Master->SetAutoDisable(false);});
    }
}
void AONE03PresentationCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    if (!bFinished && Player) { Check(false,TEXT("Scenario ended before all presentation segments completed")); Finish(); }
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this);
    Super::EndPlay(Reason);
}
void AONE03PresentationCheck::PrepareTrial()
{
    ++Trial;
    if (Trial>=16) { Finish(); return; }
    auto* W=Player->GetWeaponComponent();
    Player->ReleaseHeldInputs();
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(FVector(0,360,98));
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(10000,0,42));
    W->RefillAllAmmo(); W->SelectWeapon(Trial/8);
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    GM->SetSandboxDimLighting((Trial%8)>=4);
    const TCHAR* Modes[]={TEXT("STANDING FIRE"),TEXT("SIDEWAYS FIRE"),TEXT("DIAGONAL SPRINT / FIRE"),TEXT("MOVEMENT / RAPID AIM")};
    Segment=FString::Printf(TEXT("%s / %s / %s"),Trial/8?TEXT("12-GAUGE PUMP"):TEXT("5.56 CARBINE"),GM->IsSandboxDimLighting()?TEXT("DIM"):TEXT("BRIGHT"),Modes[Trial%4]);
    Stage=0; StageStart=Elapsed; FlashSamples=0; MaxAttachmentError=0; MaxShotPoseError=0; ShotPoseSamples=0;
    SoundRepeatErrors=0; StalePoseErrors=0; ShapeSamples=0; MaxShapeAttachmentError=0;
    LastPulse=-100; bReloadRequested=false;
}
void AONE03PresentationCheck::Observe()
{
    const auto* W=Player->GetWeaponComponent();
    const FVector M=Player->GetMuzzleLocation();
    const float Error=FVector::Distance(M,Player->MuzzleLight->GetComponentLocation());
    if (Player->MuzzleLight->Intensity>0) { ++FlashSamples; MaxAttachmentError=FMath::Max(MaxAttachmentError,Error); }
    const float ShapeError=FVector::Distance(M,Player->GetMuzzleFlashTransform().GetLocation());
    if (Player->IsMuzzleFlashVisible()) { ++ShapeSamples; MaxShapeAttachmentError=FMath::Max(MaxShapeAttachmentError,ShapeError); }
    if (W->GetLastShotFrame()!=ObservedShotFrame)
    {
        ObservedShotFrame=W->GetLastShotFrame();
        const int32 WeaponIndex=W->GetEquippedIndex(),Sound=W->GetLastShotSoundIndex();
        if (Sound<0 || Sound>=6 || LastSoundByWeapon[WeaponIndex]==Sound) ++SoundRepeatErrors;
        LastSoundByWeapon[WeaponIndex]=Sound;
        if (ObservedShotFrame==GFrameCounter)
        {
            ++ShotPoseSamples;
            MaxShotPoseError=FMath::Max(MaxShotPoseError,float(FVector::Distance(M,W->GetLastShotMuzzle())));
        }
        else ++StalePoseErrors;
        if (W->GetLastShotPoseFrame()!=uint32(ObservedShotFrame)) ++StalePoseErrors;
    }
    PoseReport+=FString::Printf(TEXT("%.6f,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.6f,%d,%d,%llu,%llu,%u,%d,%.6f,%d,%d,%d\n"),Elapsed,Trial,Player->GetVelocity().Size2D(),Player->GetActorRotation().Yaw,M.X,M.Y,M.Z,Player->MuzzleLight->Intensity,Error,W->GetTotalShotsFired(),W->GetLastShotSoundIndex(),static_cast<unsigned long long>(GFrameCounter),static_cast<unsigned long long>(W->GetLastShotFrame()),W->GetLastShotPoseFrame(),Player->IsMuzzleFlashVisible()?1:0,ShapeError,W->GetEjectionCountForWeapon(0),W->GetEjectionCountForWeapon(1),W->GetLiveCaseCount());
}
void AONE03PresentationCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished) { if (FPlatformTime::Seconds()-FinishedAt>2) FPlatformMisc::RequestExit(false); return; }
    Elapsed+=Dt;
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player || Elapsed<5) return;
    if (bCapture && !bRecording)
    { UAudioMixerBlueprintLibrary::StartRecordingOutput(this,150); AudioStart=FPlatformTime::Seconds(); bRecording=true; }
    Player->Health->Restore();
    auto* W=Player->GetWeaponComponent();
    if (Trial<0)
    {
        auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>(); GM->SetSandboxDimLighting(false);
        Check(GM->GetSandboxLightCount()>=10,TEXT("Bright/dim comparison uses the existing room lights"));
        PrepareTrial();
    }
    float T=Elapsed-StageStart;
    if (Stage==0 && T>.7f && !W->IsBusy())
    { StartShots=W->GetTotalShotsFired(); Stage=1; StageStart=Elapsed; }
    else if (Stage==1)
    {
        const int32 Mode=Trial%4;
        const float Sign=FMath::FloorToInt(T/1.f)%2 ? -1.f:1.f;
        if (Mode>0) Player->AddMovementInput((Mode==2?FVector(1,1,0):FVector(0,1,0)).GetSafeNormal(),Sign);
        Player->SetSprintHeld(Mode==2);
        const float Angle=Mode==3?T*3.2f:0;
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(FMath::Cos(Angle)*10000,FMath::Sin(Angle)*10000,42));
        if (Trial/8==0) W->SetTrigger(T>.2f && T<2.1f);
        else if (T>.2f && T<3.6f && T-LastPulse>.86f && W->CanFire())
        { W->SetTrigger(false); W->SetTrigger(true); LastPulse=T; }
        else if (T-LastPulse>.08f) W->SetTrigger(false);
        if (T>4.f) { W->SetTrigger(false); Player->SetSprintHeld(false); Stage=2; StageStart=Elapsed; }
    }
    else if (Stage==2 && Trial%4==0 && !bReloadRequested && T>.2f && !W->IsBusy())
    {
        Check(Player->MuzzleLight->Intensity==0 && !Player->IsMuzzleFlashVisible(),Segment+TEXT(" | flash naturally expires before reload cleanup"));
        ReloadAmmoBefore=W->GetAmmo();
        ReloadCommitsBefore=Trial/8?W->GetShellInsertCount():W->GetMagazineCommitCount();
        W->BeginReload(); bReloadRequested=W->IsReloading();
        Segment=FString::Printf(TEXT("%s / %s / RELOAD MECHANICAL COMPARISON"),Trial/8?TEXT("12-GAUGE PUMP"):TEXT("5.56 CARBINE"),(Trial%8)>=4?TEXT("DIM"):TEXT("BRIGHT"));
    }
    else if (Stage==2 && T>(Trial%4==0?5.2f:1.f))
    {
        const FString Prefix=Segment+TEXT(" | ");
        Check(W->GetTotalShotsFired()-StartShots>=3,Prefix+TEXT("at least three actual discharges"));
        Check(FlashSamples>0 && MaxAttachmentError<.05f,Prefix+FString::Printf(TEXT("existing flash light follows muzzle (max error %.4f cm)"),MaxAttachmentError));
        Check(ShapeSamples>0 && MaxShapeAttachmentError<.05f,Prefix+FString::Printf(TEXT("shaped flash follows muzzle (max error %.4f cm)"),MaxShapeAttachmentError));
        Check(ShotPoseSamples>=3 && StalePoseErrors==0 && MaxShotPoseError<.05f,Prefix+FString::Printf(TEXT("shots use this frame's evaluated muzzle (max error %.4f cm, stale samples %d)"),MaxShotPoseError,StalePoseErrors));
        Check(SoundRepeatErrors==0,Prefix+TEXT("six-variant bank selections do not immediately repeat"));
        Check(Player->MuzzleLight->Intensity==0,Prefix+TEXT("muzzle light expires after firing"));
        Check(!Player->IsMuzzleFlashVisible(),Prefix+TEXT("shaped flash expires after firing"));
        if (Trial%4==0)
        {
            const int32 Commits=Trial/8?W->GetShellInsertCount():W->GetMagazineCommitCount();
            Check(bReloadRequested && !W->IsBusy() && W->GetAmmo()==W->GetDefinition().Capacity && W->GetAmmo()>ReloadAmmoBefore && Commits>ReloadCommitsBefore,
                Prefix+TEXT("reload was accepted, transferred ammunition and completed"));
        }
        PrepareTrial();
    }
    if (bFinished) return;
    Observe(); Capture();
    if (Elapsed>150.f) { Check(false,TEXT("Presentation scenario completed within timeout")); Finish(); }
}
