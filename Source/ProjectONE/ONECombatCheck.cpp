#include "ONECombatCheck.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "ONEPlayerController.h"
#include "ONEBloodSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

namespace { bool RestartCheck=false; FString RestartReport; int32 RestartFailures=0; }
AONECombatCheck::AONECombatCheck() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true; }
void AONECombatCheck::BeginPlay()
{
    Super::BeginPlay();
    bComparison=FParse::Param(FCommandLine::Get(),TEXT("ONECompare"));
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate02")/(bComparison?TEXT("Comparison"):TEXT("Combat"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate02 actual runtime integration checks\nNo weapon or damage mocks. Normal gameplay camera.\n\n");
    FrameReport=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation\n");
    if (RestartCheck) { Stage=99; Report=RestartReport; Failures=RestartFailures; RestartCheck=false; }
    if (bComparison) UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONECombatCheck::Screenshot);
}
void AONECombatCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this);
    Super::EndPlay(Reason);
}
void AONECombatCheck::Check(bool Pass,const FString& Label)
{
    if (!Pass) ++Failures;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE_COMBAT %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONECombatCheck::Next(int32 Step) { Stage=Step; StageStart=Elapsed; }
void AONECombatCheck::Finish()
{
    if (bFinished) return;
    bFinished=true; FinishedAt=FPlatformTime::Seconds();
    Player->ReleaseHeldInputs();
    if (bRecording)
    {
        UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("gameplay_master"),FPaths::ConvertRelativePathToFull(Folder));
        bRecording=false;
    }
    Report+=FString::Printf(TEXT("\nFailures: %d\nActual captured frames: %d\n"),Failures,Frames);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    FFileHelper::SaveStringToFile(FrameReport,*(Folder/TEXT("frames.csv")));
    UE_LOG(LogTemp,Display,TEXT("ONE_COMBAT_COMPLETE failures=%d"),Failures);
}
void AONECombatCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty() || !Player) return;
    if (FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),Width,Height),85))
    {
        const auto* W=Player->GetWeaponComponent();
        FrameReport+=FString::Printf(TEXT("%s,%.6f,%.6f,%d,%d,%d,%d,%d\n"),*PendingFrame,FPlatformTime::Seconds()-AudioStart,Elapsed,LastPhase,W->GetEquippedIndex(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()));
        ++Frames;
    }
    PendingFrame.Empty();
}
void AONECombatCheck::Compare(float Dt)
{
    auto* W=Player->GetWeaponComponent();
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    Player->Health->Restore();
    if (Elapsed<2) return;
    if (!bRecording)
    {
        Player->SetActorLocation(FVector(-250,200,98));
        Player->GetCharacterMovement()->StopMovementImmediately();
        W->RefillAllAmmo();
        UAudioMixerBlueprintLibrary::StartRecordingOutput(this,28.f);
        AudioStart=FPlatformTime::Seconds(); bRecording=true;
    }
    const float T=Elapsed-2;
    const int32 Phase=T<2?0:T<5?1:T<7.5?2:T<9?3:T<13?4:T<17?5:T<21?6:7;
    if (Phase!=LastPhase)
    {
        if (LastPhase==1 || LastPhase==4) Check(FootTravel>4.f,FString::Printf(TEXT("Weapon %d lower-body poses change during actual moving fire"),LastPhase==1?0:1));
        W->SetTrigger(false); Player->SetSprintHeld(false);
        FootTravel=0; FirstFoot=Player->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation();
        LastPhase=Phase;
        if (Phase==1 || Phase==4)
        {
            Target=GM->SpawnSandboxEnemyAt(Player->GetActorLocation()+FVector(450,0,0));
            if (Target) Target->AttackDamage=0;
        }
        if (Phase==1) W->SetTrigger(true);
        if (Phase==2) W->BeginReload();
        if (Phase==3) W->SelectWeapon(1);
        if (Phase==5) W->BeginReload();
        if (Phase==6)
        {
            W->CancelReload();
            Player->SetActorLocation(FVector(-250,200,98));
            Target=GM->SpawnSandboxEnemyAt(Player->GetActorLocation()+FVector(270,0,0));
            if (Target) { Target->AttackDamage=0; }
        }
        if (Phase==7) { Check(Frames>120,TEXT("Actual gameplay frames recorded with master output audio")); Finish(); return; }
    }
    Player->SetAimOverride(true,IsValid(Target)&&!Target->IsDead()?Target->GetMesh()->GetSocketLocation(TEXT("spine_02")):Player->GetActorLocation()+FVector(900,0,30));
    if (Phase==1 || Phase==4)
    {
        Player->AddMovementInput(FVector(0,Phase==1?1.f:-1.f,0),.65f);
        FootTravel=FMath::Max(FootTravel,FVector::Dist(FirstFoot,Player->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation()));
    }
    if ((Phase==4 || Phase==6) && W->CanFire()) { W->SetTrigger(false); W->SetTrigger(true); }
    if (Phase==5 && !W->IsBusy() && W->GetAmmo()<W->GetDefinition().Capacity) W->BeginReload();
    if (Phase==6 && W->GetAmmo()==0 && !W->IsBusy()) W->BeginReload();
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1.0/24.0)
    {
        LastCapture=Now; PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames);
        FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false);
    }
}
void AONECombatCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished) { if (FPlatformTime::Seconds()-FinishedAt>2) FPlatformMisc::RequestExit(false); return; }
    Elapsed+=Dt;
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player) return;
    if (bComparison) { Compare(Dt); return; }
    auto* W=Player->GetWeaponComponent();
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    float T=Elapsed-StageStart;
    if (Stage<30) Player->Health->Restore();
    Player->SetAimOverride(true,IsValid(Target) && Stage>=24 && Stage<=25 ? Target->GetMesh()->GetSocketLocation(TEXT("spine_02")) : Player->GetActorLocation()+FVector(1000,0,30));
    switch(Stage)
    {
    case 0: if (Elapsed>2) {
        Check(GM->IsSandbox() && W->GetWeaponCount()==2,TEXT("Sandbox uses two real carried weapons with automatic waves disabled"));
        Check(W->GetAmmoForWeapon(0)==24 && W->GetAmmoForWeapon(1)==6,TEXT("Both weapons begin with distinct loaded capacities"));
        W->GrantRoundAmmo();
        Check(W->GetReserveAmmoForWeapon(0)==240 && W->GetReserveAmmoForWeapon(1)==44,TEXT("Round reward resupplies each carried weapon by its own configured amount"));
        W->RefillAllAmmo();
        W->SetTrigger(true); Next(1);
    } break;
    case 1: if (T>.55f) {
        W->SetTrigger(false); Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo();
        Check(Ammo<24 && Ammo>0,TEXT("Carbine held trigger fires repeatedly"));
        W->BeginReload(); Next(2);
    } break;
    case 2: if (T>.6f) { W->SelectWeapon(1); Next(3); } break;
    case 3: if (T>2.3f) {
        Check(W->GetEquippedIndex()==1 && Player->Gun->GetStaticMesh() && Player->Gun->GetStaticMesh()->GetName().Contains(TEXT("PumpShotgun")),TEXT("Equip replaces the visible weapon mesh"));
        Check(W->GetAmmoForWeapon(0)==Ammo && W->GetReserveAmmoForWeapon(0)==Reserve,TEXT("Unequipped carbine reload cannot commit after pre-insertion cancellation"));
        Ejections=W->GetEjectionCount(); W->SetTrigger(true); Next(4);
    } break;
    case 4: if (T>.1f) {
        W->SetTrigger(false);
        Check(W->GetAmmo()==5 && W->NeedsPump(1),TEXT("Shotgun discharge consumes exactly one shell and creates pump obligation"));
        W->SelectWeapon(0); Next(5);
    } break;
    case 5: if (T>.8f) {
        Check(W->GetAmmoForWeapon(1)==5 && W->NeedsPump(1),TEXT("Holstering preserves shotgun ammunition and pending pump"));
        W->SelectWeapon(1); Next(6);
    } break;
    case 6: if (T>1.4f) {
        Check(!W->NeedsPump(1) && W->CanFire(),TEXT("Re-equipping finishes interrupted pump before accepting another shot"));
        Check(W->GetEjectionCount()==Ejections+1,TEXT("Interrupted pump ejects its spent shell exactly once"));
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); W->BeginReload(); Next(7);
    } break;
    case 7: if (T>.8f) { W->SelectWeapon(0); Next(8); } break;
    case 8: if (T>1.5f) {
        Check(W->GetAmmoForWeapon(1)==Ammo && W->GetReserveAmmoForWeapon(1)==Reserve,TEXT("Shell reload canceled before insertion grants no shell"));
        W->SelectWeapon(1); Next(9);
    } break;
    case 9: if (T>.6f) { W->BeginReload(); Next(10); } break;
    case 10: if (T>1.04f) { W->SelectWeapon(0); Next(11); } break;
    case 11: if (T>1.2f) {
        Check(W->GetAmmoForWeapon(1)==Ammo+1 && W->GetReserveAmmoForWeapon(1)==Reserve-1,TEXT("Switch after insertion preserves exactly the earned shell"));
        W->SetTrigger(true); Next(12);
    } break;
    case 12: if (T>.4f) { W->CycleWeapon(); Shots=W->GetTotalShotsFired(); Next(13); } break;
    case 13: if (T>1) {
        Check(W->GetTotalShotsFired()==Shots,TEXT("Switch clears held firing without a ghost shotgun discharge"));
        Check(Player->MuzzleLight->Intensity==0,TEXT("Switch leaves no stale muzzle flash"));
        W->RefillAllAmmo(); Count=0; Next(14);
    } break;
    case 14:
        if (W->CanFire() && W->GetAmmo()>0) { W->SetTrigger(false); W->SetTrigger(true); ++Count; }
        if (W->GetAmmo()==0 && !W->IsBusy()) {
            W->SetTrigger(false); W->SetTrigger(true);
            Check(Count==6 && W->GetAmmoForWeapon(0)==24,TEXT("Six independent shotgun cycles empty only its own magazine"));
            Next(140);
        } break;
    case 140: if (T>.05f) {
            Check(W->GetTimeSinceEmpty()<.15f,TEXT("Empty shotgun triggers distinct empty feedback"));
            W->SetTrigger(false);
            Reserve=W->GetReserveAmmo(); Shots=W->GetTotalShotsFired(); W->BeginReload(); Next(15);
        } break;
    case 15: if (T>1.03f) {
        Check(W->GetAmmo()==1 && W->GetReserveAmmo()==Reserve-1,TEXT("Reload from empty grants one shell at insertion event"));
        W->SetTrigger(true); W->SetTrigger(false);
        Check(W->GetAmmo()==1 && W->GetOperation()==EONEWeaponOperation::ShellEnd,TEXT("Short reload-interrupt tap first enters closing pose without an early shot"));
        Next(16);
    } break;
    case 16: if (T>1.3f) {
        W->SetTrigger(false);
        Check(W->GetTotalShotsFired()==Shots+1 && W->GetAmmo()==0 && W->GetReserveAmmo()==Reserve-1,TEXT("Fire interrupts shell reload through closing pose and spends only its earned shell"));
        W->SelectWeapon(0); Next(17);
    } break;
    case 17: if (T>.6f) { W->SetTrigger(true); Next(18); } break;
    case 18: if (T>.3f) { W->SetTrigger(false); Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); W->BeginReload(); Next(19); } break;
    case 19: if (T>.65f) {
        PausedOperation=W->GetOperationElapsed(); PauseStart=FPlatformTime::Seconds(); UGameplayStatics::SetGamePaused(this,true); Next(20);
    } break;
    case 20: if (FPlatformTime::Seconds()-PauseStart>.8) {
        Check(FMath::IsNearlyEqual(W->GetOperationElapsed(),PausedOperation,.02f) && W->GetAmmo()==Ammo,TEXT("Pause freezes reload clock and ammunition"));
        UGameplayStatics::SetGamePaused(this,false); Next(21);
    } break;
    case 21: if (T>1.7f) {
        Check(W->GetAmmo()==24 && W->GetReserveAmmo()==Reserve-(24-Ammo),TEXT("Unpaused magazine insertion conserves total ammunition"));
        W->SelectWeapon(1); W->RefillAllAmmo(); Next(22);
    } break;
    case 22: if (T>.6f) {
        W->SelectWeapon(1); Player->SetActorLocation(FVector(-350,-160,98)); Next(23);
    } break;
    case 23: if (T>.7f) {
        Target=GM->SpawnSandboxEnemyAt(Player->GetActorLocation()+FVector(220,0,0));
        Check(Target!=nullptr,TEXT("Sandbox spawns enemy into authoritative encounter registry"));
        if (Target) { Target->SetActorTickEnabled(false); Target->GetCharacterMovement()->StopMovementImmediately(); }
        Next(24);
    } break;
    case 24: if (T>.5f) {
        if (Target) Player->SetAimOverride(true,Target->GetMesh()->GetSocketLocation(TEXT("spine_02")));
        Count=GM->GetPoints(); Shots=Target?Target->GetDamageTransactionCount():0;
        W->SetTrigger(true); Next(25);
    } break;
    case 25: if (T>.2f) {
        W->SetTrigger(false);
        Check(Target && Target->GetDamageTransactionCount()==Shots+1,TEXT("Actual eight-pellet discharge creates one damage transaction per victim"));
        if (Target) {
            const int32 Transactions=Target->GetDamageTransactionCount();
            const int32 Sever=Target->GetSeverCount(); const float Hp=Target->GetHealth(); const int32 Points=GM->GetPoints();
            FONEWeaponDamagePacket Duplicate; Duplicate.ShotId=W->GetLastShotId(); Duplicate.BodyDamage=120; Duplicate.Pellets=8;
            Target->ReceiveWeaponDamage(Duplicate);
            Check(Target->GetDamageTransactionCount()==Transactions && Target->GetHealth()==Hp && Target->GetSeverCount()==Sever && GM->GetPoints()==Points,TEXT("Repeated discharge ID cannot repeat damage, severing, effects or points"));
            Check(Sever<=1 && GM->GetPoints()-Count<=100,TEXT("Pellet burst cannot create multiple severed parts or death awards"));
            Check(GM->GetPoints()-Count==(Target->IsDead()?100:0),TEXT("Registered pellet victim awards exactly one hundred points if killed"));
        }
        GM->ClearSandboxPresentation();
        if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Check(Blood->GetDecalCount()==0 && Blood->GetPieceCount()==0 && Blood->GetCorpseCount()==0,TEXT("Sandbox clear removes blood, parts and bodies through bounded runtime subsystem"));
        W->RefillAllAmmo(); W->SetTrigger(true); Next(260);
    } break;
    case 260: if (T>.1f) {
        W->SetTrigger(false); Shots=W->GetTotalShotsFired();
        W->SetTrigger(true); W->SetTrigger(false);
        if (auto* Controller=Cast<AONEPlayerController>(Player->GetController())) Controller->FlushPressedKeys();
        Next(261);
    } break;
    case 261: if (T>1.1f) {
        Check(W->GetTotalShotsFired()==Shots,TEXT("Viewport key flush clears a semi-auto tap queued behind the pump"));
        Next(26);
    } break;
    case 26: if (T>1.1f) { W->SetTrigger(false); W->BeginReload(); Next(30); } break;
    case 30: if (T>.65f) {
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); Player->ReceiveAttack(1000,Player->GetActorLocation()+FVector(100,0,0)); Next(31);
    } break;
    case 31: if (T>1.5f) {
        Check(Player->IsDead() && GM->IsGameOver() && !W->IsReloading(),TEXT("Death cancels shotgun reload and enters game over"));
        Check(W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && Player->MuzzleLight->Intensity==0,TEXT("Death prevents stale shell commit and clears firing effects"));
        RestartCheck=true; RestartReport=Report; RestartFailures=Failures; GM->RestartScene(); Next(98);
    } break;
    case 99: if (Elapsed>2) {
        Check(!Player->IsDead() && GM->IsSandbox() && GM->GetPoints()==0,TEXT("Restart restores live sandbox and resets score"));
        Check(W->GetAmmoForWeapon(0)==24 && W->GetReserveAmmoForWeapon(0)==192 && W->GetAmmoForWeapon(1)==6 && W->GetReserveAmmoForWeapon(1)==36 && !W->NeedsPump(1),TEXT("Restart restores both weapons, reserves, operation and pump states"));
        Finish();
    } break;
    }
    if (Elapsed>100) { Check(false,FString::Printf(TEXT("Timed out at stage %d"),Stage)); Finish(); }
}
