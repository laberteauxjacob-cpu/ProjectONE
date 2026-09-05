#include "ONE03MovementCheck.h"
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

namespace
{
    const FVector Movement03Directions[]={FVector(1,0,0),FVector(1,1,0),FVector(0,1,0),FVector(-1,1,0),FVector(-1,0,0),FVector(-1,-1,0),FVector(0,-1,0),FVector(1,-1,0)};
    const TCHAR* DirectionNames[]={TEXT("FORWARD"),TEXT("FORWARD RIGHT"),TEXT("RIGHT"),TEXT("BACK RIGHT"),TEXT("BACK"),TEXT("BACK LEFT"),TEXT("LEFT"),TEXT("FORWARD LEFT")};
}
AONE03MovementCheck::AONE03MovementCheck()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true;
    // Observe completed movement AND skeletal evaluation. Input requests queued
    // here naturally enter the next frame's normal CharacterMovement update.
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE03MovementCheck::BeginPlay()
{
    Super::BeginPlay();
    bManual=FParse::Value(FCommandLine::Get(),TEXT("ONE03ManualCapture="),ManualDuration);
    bCapture=bManual || FParse::Param(FCommandLine::Get(),TEXT("ONE03MovementCapture"));
    ManualDuration=FMath::Clamp(ManualDuration,15.f,180.f);
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03")/(bManual?TEXT("Manual"):bCapture?TEXT("MovementCapture"):TEXT("Movement"));
    if (!bManual && FParse::Param(FCommandLine::Get(),TEXT("ONE03TurnCheck"))) Folder+=TEXT("_Turns");
    if (bManual) Folder/=FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=bManual?TEXT("Candidate03 ordinary-input gameplay capture\nThis recorder observes the actual player and audio; it does not drive input, aim, health or ammunition.\n\n"):
        TEXT("Candidate03 actual movement integration\nEach directional axis feeds CharacterMovement input; diagonal inputs are not pre-normalized by this probe.\n\n");
    PoseReport=TEXT("seconds,trial,stage,speed,actor_x,actor_y,actor_z,actor_yaw,left_x,left_y,left_z,right_x,right_y,right_z,muzzle_x,muzzle_y,muzzle_z,weapon,ammo,operation,body_yaw,sprint_requested,sprint_reload_interrupts,automatic_reloads,total_shots\n");
    FrameReport=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation\n");
    if (bCapture)
    {
        UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE03MovementCheck::Screenshot);
        if (auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
            if (auto Master=Mixer->GetMasterSubmix().Pin()) Mixer->AudioRenderThreadCommand([Master](){Master->SetAutoDisable(false);});
    }
}
void AONE03MovementCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    if (!bFinished && Player)
    { Report+=TEXT("\nRecording ended by level transition or exit; duration incomplete.\n"); Finish(); }
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this); Super::EndPlay(Reason);
}
void AONE03MovementCheck::Check(bool Pass,const FString& Label)
{
    Failures+=Pass?0:1; Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE03_MOVEMENT %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE03MovementCheck::PrepareTrial()
{
    ++Trial; Stage=0; StageStart=Elapsed; Samples=0; SpeedSum=0; MinSpeed=BIG_NUMBER; MaxSpeed=0;
    auto* W=Player->GetWeaponComponent();
    Player->ReleaseHeldInputs(); W->RefillAllAmmo();
    Player->GetCharacterMovement()->StopMovementImmediately();
    if (Trial>=48)
    {
        if (Trial>=54) { Finish(); return; }
        Player->SetActorLocation(FVector(0,360,98));
        W->SelectWeapon(Trial%2);
        PreviousFoot=Player->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation();
        TurnFootTravel=0; TurnSamples=0;
        LeftMinZ=RightMinZ=BIG_NUMBER; LeftMaxZ=RightMaxZ=-BIG_NUMBER;
        Segment=FString::Printf(TEXT("%s / %s"),Trial%2?TEXT("SHOTGUN"):TEXT("CARBINE"),Trial<50?TEXT("STATIONARY CLOCKWISE / COUNTERCLOCKWISE"):Trial<52?TEXT("CONTINUOUS DIRECTION REVERSALS"):TEXT("MOVING FIRE / RAPID AIM"));
        Stage=19; return;
    }
    const int32 Weapon=Trial/24,Mode=(Trial%24)/8,Direction=Trial%8;
    InputDirection=Movement03Directions[Direction];
    Player->SetActorLocation(FVector(0,360,98)-InputDirection.GetSafeNormal()*190.f);
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(10000,0,40));
    W->SelectWeapon(Weapon);
    Segment=FString::Printf(TEXT("%s / %s / %s"),Weapon?TEXT("SHOTGUN"):TEXT("CARBINE"),Mode==0?TEXT("WALK"):Mode==1?TEXT("SPRINT"):TEXT("RELOAD WALK"),DirectionNames[Direction]);
}
void AONE03MovementCheck::RecordPose()
{
    const auto* W=Player->GetWeaponComponent();
    const FVector P=Player->GetActorLocation(),L=Player->GetMesh()->GetSocketLocation(TEXT("foot_l")),R=Player->GetMesh()->GetSocketLocation(TEXT("foot_r")),M=Player->GetMuzzleLocation();
    PoseReport+=FString::Printf(TEXT("%.6f,%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d,%d,%.4f,%d,%d,%d,%d\n"),Elapsed,Trial,Stage,Player->GetVelocity().Size2D(),P.X,P.Y,P.Z,Player->GetActorRotation().Yaw,L.X,L.Y,L.Z,R.X,R.Y,R.Z,M.X,M.Y,M.Z,W->GetEquippedIndex(),W->GetAmmo(),int32(W->GetOperation()),Player->GetBodyFacingYaw(),Player->IsSprintRequested()?1:0,W->GetSprintReloadInterruptCount(),W->GetAutomaticReloadCount(),W->GetTotalShotsFired());
}
void AONE03MovementCheck::Capture()
{
    if (!bCapture || !bRecording || bFinished) return;
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1.0/30.0)
    { LastCapture=Now; PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames); FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false); }
}
void AONE03MovementCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty() || !Player) return;
    const double CapturedAt=FPlatformTime::Seconds()-AudioStart;
    auto* W=Player->GetWeaponComponent();
    const FString Row=FString::Printf(TEXT("%s,%.6f,%.6f,%d,%d,%d,%d,%d\n"),*PendingFrame,CapturedAt,Elapsed,Trial,W->GetEquippedIndex(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()));
    if (FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),Width,Height),85))
    {
        FrameReport+=Row; ++Frames;
    }
    PendingFrame.Empty();
}
void AONE03MovementCheck::Finish()
{
    if (bFinished) return; bFinished=true; FinishedAt=FPlatformTime::Seconds();
    if (!bManual) Player->ReleaseHeldInputs();
    if (bRecording)
    { UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("gameplay_master"),FPaths::ConvertRelativePathToFull(Folder)); bRecording=false; }
    Report+=FString::Printf(TEXT("\nFailures: %d\nCaptured frames: %d\n"),Failures,Frames);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    FFileHelper::SaveStringToFile(PoseReport,*(Folder/TEXT("poses.csv")));
    FFileHelper::SaveStringToFile(FrameReport,*(Folder/TEXT("frames.csv")));
    UE_LOG(LogTemp,Display,TEXT("ONE03_MOVEMENT_COMPLETE failures=%d"),Failures);
}
void AONE03MovementCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished) { if (!bManual && FPlatformTime::Seconds()-FinishedAt>2) FPlatformMisc::RequestExit(false); return; }
    Elapsed+=Dt;
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player || Elapsed<2) return;
    if (bCapture && !bRecording)
    { UAudioMixerBlueprintLibrary::StartRecordingOutput(this,bManual?ManualDuration+2:180.f); AudioStart=FPlatformTime::Seconds(); bRecording=true; }
    if (bManual)
    {
        const auto* ManualWeapon=Player->GetWeaponComponent();
        Segment=FString::Printf(TEXT("MANUAL INPUT / %s / %s"),*ManualWeapon->GetWeaponName().ToString(),Player->IsSprintRequested()?TEXT("SPRINT"):ManualWeapon->IsReloading()?TEXT("RELOAD"):TEXT("KEYBOARD AND MOUSE"));
        RecordPose(); Capture();
        if (FPlatformTime::Seconds()-AudioStart>=ManualDuration) Finish();
        return;
    }
    auto* W=Player->GetWeaponComponent(); Player->Health->Restore();
    if (Trial<0)
    {
        const auto* Z=GetDefault<AONEZombie>();
        Check(Player->WalkSpeed>FMath::Max(Z->PursuitSpeed,Z->ShambleSpeed),TEXT("Player walk tuning exceeds fastest implemented infected speed"));
        Check(Player->RunSpeed>Player->WalkSpeed,TEXT("Sprint remains faster than walking"));
        if (FParse::Param(FCommandLine::Get(),TEXT("ONE03TurnCheck"))) Trial=47;
        PrepareTrial();
    }
    float T=Elapsed-StageStart;
    if (Stage==0 && T>.55f && !W->IsBusy())
    {
        if ((Trial%24)/8==2) { W->SetTrigger(true); Stage=10; StageStart=Elapsed; }
        else { Player->SetSprintHeld((Trial%24)/8==1); Stage=1; StageStart=Elapsed; }
    }
    else if (Stage==10)
    {
        if (T>.1f) W->SetTrigger(false);
        if (T>1.f && !W->IsBusy()) { W->BeginReload(); Stage=1; StageStart=Elapsed; }
    }
    else if (Stage==1)
    {
        Player->AddMovementInput(FVector::ForwardVector,InputDirection.X);
        Player->AddMovementInput(FVector::RightVector,InputDirection.Y);
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(10000,0,40));
        if (T>.3f)
        { const float Speed=Player->GetVelocity().Size2D(); ++Samples; SpeedSum+=Speed; MinSpeed=FMath::Min(MinSpeed,Speed); MaxSpeed=FMath::Max(MaxSpeed,Speed); }
        if (T>.85f)
        {
            const float Expected=(Trial%24)/8==1?Player->RunSpeed:Player->WalkSpeed;
            const float Mean=SpeedSum/FMath::Max(1,Samples);
            Check(Samples>10 && FMath::Abs(Mean-Expected)<3.f && MaxSpeed<=Expected+3.f && MinSpeed>=Expected-8.f,
                FString::Printf(TEXT("%s actual mean/min/max %.2f/%.2f/%.2f target %.2f cm/s"),*Segment,Mean,MinSpeed,MaxSpeed,Expected));
            PrepareTrial();
        }
    }
    else if (Stage==19 && T>.6f && !W->IsBusy())
    { Stage=Trial<50?20:Trial<52?21:22; StageStart=Elapsed; MovingShots=W->GetTotalShotsFired(); }
    else if (Stage==20)
    {
        // Continuous full circles, both signs, then reversals. Aim is an ordinary
        // world point; measured feet are evaluated output rather than a pose mock.
        const float Angle=T<4.f?T*PI*.5f:T<8.f?(8.f-T)*PI*.5f:T<10.f?(T-8.f)*PI:T<14.f?(FMath::FloorToInt(T-10.f)%2?PI:0.f):(T>=14.7f && T<14.82f?PI:0.f);
        if (T>=10.f) Segment=FString::Printf(TEXT("%s / ABRUPT 180-DEGREE AIM REVERSALS"),Trial%2?TEXT("SHOTGUN"):TEXT("CARBINE"));
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(FMath::Cos(Angle)*800,FMath::Sin(Angle)*800,42));
        const FVector Foot=Player->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation();
        if (T>.3f) { TurnFootTravel+=FVector::Distance(PreviousFoot,Foot); ++TurnSamples; }
        PreviousFoot=Foot;
        if (T>.3f)
        {
            const float L=Player->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z,R=Player->GetMesh()->GetSocketLocation(TEXT("foot_r")).Z;
            LeftMinZ=FMath::Min(LeftMinZ,L); LeftMaxZ=FMath::Max(LeftMaxZ,L);
            RightMinZ=FMath::Min(RightMinZ,R); RightMaxZ=FMath::Max(RightMaxZ,R);
        }
        if (T>15.6f)
        {
            Check(Player->GetVelocity().Size2D()<1.f,TEXT("Stationary circles do not translate the player capsule"));
            Check(TurnSamples>100 && TurnFootTravel>100.f,TEXT("Repeated aim circles produce evaluated lower-body foot motion"));
            Check(LeftMaxZ-LeftMinZ>5.f && RightMaxZ-RightMinZ>5.f,FString::Printf(TEXT("Both feet lift in world space during turn steps: %.2f / %.2f cm"),LeftMaxZ-LeftMinZ,RightMaxZ-RightMinZ));
            PrepareTrial();
        }
    }
    else if (Stage==21 || Stage==22)
    {
        const int32 Leg=FMath::FloorToInt(T/1.1f);
        const FVector Direction=Movement03Directions[(Leg%2?4:0)+(Leg/2)%4];
        Player->AddMovementInput(Direction.GetSafeNormal());
        Player->SetSprintHeld(Leg>=2);
        const float Angle=Stage==22?T*1.7f:0.f;
        Player->SetAimOverride(true,Player->GetActorLocation()+FVector(FMath::Cos(Angle)*800,FMath::Sin(Angle)*800,40));
        if (Stage==22)
        {
            if (W->GetDefinition().bAutomatic) W->SetTrigger(true);
            else if (W->CanFire()) { W->SetTrigger(false); W->SetTrigger(true); }
        }
        if (T>6.6f)
        {
            if (Stage==22) Check(W->GetTotalShotsFired()-MovingShots>=4,TEXT("Actual shots dispatch during continuous translation, sprint and rapid aim changes"));
            PrepareTrial();
        }
    }
    if (bFinished) return;
    RecordPose(); Capture();
    if (Elapsed>180.f) { Check(false,TEXT("Movement scenario completed within its timeout")); Finish(); }
}
