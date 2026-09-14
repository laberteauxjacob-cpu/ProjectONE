#include "ONE07PhysicalityCheck.h"
#include "ONEZombie.h"
#include "ONEInfectedVariant.h"
#include "ONEInfectedAnimInstance.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEZombieAudioComponent.h"
#include "ONEBloodSubsystem.h"
#include "ONE06CaptureComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"

namespace
{
    FONEWeaponDamagePacket ProbePacket(uint64 Id,EONEHitRegion Region,float Damage,float Trauma,const FVector& Position)
    {
        FONEWeaponDamagePacket P; P.ShotId=Id;
        P.Get(Region).AddPellet(Damage,Trauma,Position,FVector::ForwardVector,-FVector::ForwardVector,NAME_None);
        P.Finalize(); return P;
    }
}
AONE07PhysicalityCheck::AONE07PhysicalityCheck()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    Capture=CreateDefaultSubobject<UONE06CaptureComponent>(TEXT("BoundedEngineCapture"));
}
void AONE07PhysicalityCheck::BeginPlay()
{
    Super::BeginPlay();
    bEncounter=FParse::Param(FCommandLine::Get(),TEXT("ONE07Encounter"));
    bCapture=FParse::Param(FCommandLine::Get(),TEXT("ONE07Capture"));
    Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir())/TEXT("Candidate07")/(bEncounter?TEXT("Encounter"):TEXT("Physicality"))/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate07 actual runtime evidence. Not native human play, audio audition or visual acceptance.\n");
    Report+=bEncounter?TEXT("Ordinary encounter: scripted production controller movement, projected torso-height cursor and LMB/R only. No grants, teleports, health restores, actor spawning or forced damage. Actual survival may end early.\n"):
        TEXT("Controlled registered sandbox fixtures: declared phase-boundary retirement/cleanup/player placement and health restore; group phases also replenish player health for observation. The isolated fall stops approach velocity once and applies a backward bounded impulse; mixed falls retain approach velocity. Fall and regional sever packets are explicit diagnostic interventions. The final lineup explicitly cycles the production appearance pool, then pursues normally. These are not ordinary player aiming or naturally triggered knockdowns.\n");
    Telemetry=TEXT("seconds,phase,id,variant,state,health,x,y,z,pelvis_x,pelvis_y,pelvis_z,head_x,head_y,head_z,speed,physics,awake,contacts,falls,recoveries,blocked,pose_cm,rebase_cm,head_present,left_arm,right_arm,left_leg,foot_cues,body_cues,attack_cues,live,kills,player_health,health_restores\n");
    Inputs=TEXT("seconds,key,event,handled\n");
}
void AONE07PhysicalityCheck::Check(bool Pass,const FString& Text)
{
    ++Checks; Failures+=Pass?0:1;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Text);
    UE_LOG(LogTemp,Display,TEXT("ONE07_PHYSICALITY %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Text);
}
void AONE07PhysicalityCheck::Key(const FKey& K,bool Down)
{
    if (!Controller || Held.Contains(K)==Down) return;
    const bool Handled=Controller->InputKey(FInputKeyEventArgs::CreateSimulated(K,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
    if (Down) Held.Add(K); else Held.Remove(K);
    ++InputEdges; Inputs+=FString::Printf(TEXT("%.6f,%s,%s,%d\n"),Elapsed,*K.ToString(),Down?TEXT("pressed"):TEXT("released"),Handled);
}
void AONE07PhysicalityCheck::ReleaseKeys()
{ const TArray<FKey> Keys=Held.Array(); for (const FKey& K:Keys) Key(K,false); }
void AONE07PhysicalityCheck::EnterPhase()
{
    ReleaseKeys();
    if (ProbeObstacle) { ProbeObstacle->Destroy(); ProbeObstacle=nullptr; }
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
        if (!It->IsDead()) It->ReceiveWeaponDamage(ProbePacket(700000+It->GetUniqueID()+uint64(Phase+1)*100000,EONEHitRegion::Body,1000,0,It->GetActorLocation()));
    Mode->ClearSandboxPresentation(); Subjects.Reset();
    ++Phase; PhaseStart=Elapsed; Action=0;
    if (Phase>=7) { Finish(); return; }
    const TCHAR* Labels[]={TEXT("ONE INFECTED / ORDINARY CAMERA / APPROACH AND ATTACK"),
        TEXT("ONE BODY / EXPLICIT BOUNDED LIVING FALL / HEALTH RETAINED"),
        TEXT("TWO BODIES / SHARED APPROACH / REAL CONTACT"),
        TEXT("SIX BODIES / DECLARED SOLID OBSTACLE / REAL CONTACT"),
        TEXT("MIXED LIVING FALL AND GET-UP / PHYSICALITY PROBE"),
        TEXT("CURRENT-POSE LIMB LOSS AND DEATH / REGIONAL PACKET PROBE"),
        TEXT("EXPLICIT THREE-VARIANT LINEUP / SIX BODY PURSUIT")};
    Label=Labels[Phase];
    Player->SetActorLocation(Origin,false,nullptr,ETeleportType::TeleportPhysics);
    Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetHealthComponent()->Restore(); ++HealthRestores;
    if (Phase==3)
    {
        // A visible, stationary diagnostic obstacle creates two real approach
        // paths. It is confined to this explicit sandbox fixture.
        ProbeObstacle=GetWorld()->SpawnActor<AStaticMeshActor>(Origin+FVector(-285,0,-23),FRotator::ZeroRotator);
        if (ProbeObstacle)
        {
            auto* Shape=ProbeObstacle->GetStaticMeshComponent();
            // Runtime-created static-mobility components reject SetStaticMesh.
            // This stationary fixture is movable geometry with ordinary blocking.
            Shape->SetMobility(EComponentMobility::Movable);
            Shape->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
            Shape->SetWorldScale3D(FVector(.8f,1.8f,1.5f));
            Shape->SetCollisionProfileName(TEXT("BlockAll"));
            Shape->SetCanEverAffectNavigation(true);
        }
        Check(ProbeObstacle && ProbeObstacle->GetStaticMeshComponent()->GetStaticMesh(),TEXT("Declared visible obstacle provides blocking geometry"));
    }
    const int32 Count=Phase<2?1:Phase==2?2:Phase==5?4:6;
    for (int32 I=0;I<Count;++I)
    {
        const float Distance=Phase==1?300.f:Phase==5?390.f:550.f;
        const FVector Position=Phase==6?Origin+FVector(-Distance-float(I/3)*90.f,(I%3-1)*145.f,0):
            Origin+FVector(-Distance-float(I/2)*72.f,(I%2?.5f:-.5f)*(Count<=2?58.f:76.f),0);
        UONEInfectedVariant* Appearance=nullptr;
        if (Phase==6)
        {
            const TArray<FString> Paths=AONEZombie::GetProductionVariantPaths();
            if (I==0) Check(Paths.Num()==3,TEXT("Lineup uses all three production appearances"));
            if (!Paths.IsEmpty()) Appearance=LoadObject<UONEInfectedVariant>(nullptr,*Paths[I%Paths.Num()]);
            Check(Appearance!=nullptr,TEXT("Declared lineup appearance resolves from the production pool"));
        }
        if (auto* Z=Mode->SpawnSandboxEnemyAt(Position,Appearance)) Subjects.Add(Z);
    }
    Check(Subjects.Num()==Count,FString::Printf(TEXT("Phase%d registered %d intended actors"),Phase,Count));
    for (const auto& Z:Subjects)
    {
        auto* Anim=Cast<UONEInfectedAnimInstance>(Z->GetMesh()->GetAnimInstance());
        Check(!Z->GetVariantId().IsNone() && Anim && Anim->HasRequiredClips(),TEXT("New variant and infected-specific complete motion bank loaded"));
        Check(FMath::IsNearlyEqual(Z->GetHealth(),112.f),TEXT("Visual variant starts with shared112 health"));
    }
    StartKills=Mode->GetKills(); StartRemaining=Mode->GetRemaining();
    if (Capture && bCapture) Capture->SetPhaseLabel(Label);
}
void AONE07PhysicalityCheck::DriveEncounter(float Dt)
{
    if (Player->IsDead()) { Label=TEXT("ORDINARY ENCOUNTER / REAL PLAYER DEATH"); Finish(); return; }
    // Four measured-duration movement legs use real WASD/Shift input. Arena
    // collision remains authoritative; the driver has no transform override.
    const int32 Leg=int32(Elapsed/3.2f)%4;
    Key(EKeys::W,Leg==0); Key(EKeys::D,Leg==1); Key(EKeys::S,Leg==2); Key(EKeys::A,Leg==3);
    Key(EKeys::LeftShift,false);
    AONEZombie* Closest=nullptr; float Best=BIG_NUMBER;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
        if (!It->IsDead())
        {
            const float Distance=FVector::DistSquared2D(Player->GetActorLocation(),It->GetActorLocation());
            if (Distance<Best) { Best=Distance; Closest=*It; }
        }
    auto* W=Player->GetWeaponComponent();
    if (ReleaseFireAt>0 && Elapsed>=ReleaseFireAt) { Key(EKeys::LeftMouseButton,false); ReleaseFireAt=0; }
    Key(EKeys::RightMouseButton,false); Key(EKeys::LeftControl,false);
    if (Closest)
    {
        FVector Aim=Closest->GetActorLocation(); Aim.Z=Player->GetActorLocation().Z-90.f+125.f;
        FVector2D Screen;
        if (Player->ProjectLogicalWorld(Aim,Screen))
        {
            int32 Width,Height; Controller->GetViewportSize(Width,Height);
            Controller->SetMouseLocation(FMath::Clamp(FMath::RoundToInt(Screen.X),1,Width-2),FMath::Clamp(FMath::RoundToInt(Screen.Y),1,Height-2));
            if (Elapsed>=NextFire && W->CanFire())
            { Key(EKeys::LeftMouseButton,true); ReleaseFireAt=Elapsed+.045f; NextFire=Elapsed+.29f; }
        }
    }
    if (W->GetAmmo()==0 && W->GetReserveAmmo()>0 && !W->IsBusy()) { Key(EKeys::R,true); Key(EKeys::R,false); }
    if (Elapsed>=110.f) Finish();
}
void AONE07PhysicalityCheck::Observe(float Dt)
{
    if (Elapsed<NextSample) return; NextSample=Elapsed+.1f;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
    {
        const auto* Z=*It; const FVector P=Z->GetActorLocation(),Pelvis=Z->GetMesh()->GetSocketLocation(TEXT("pelvis")),Head=Z->GetMesh()->GetSocketLocation(TEXT("head"));
        const auto* Audio=Z->ZombieAudio.Get();
        Telemetry+=FString::Printf(TEXT("%.5f,%d,%u,%s,%d,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%d,%d,%d,%d,%d,%d,%.5f,%.5f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.3f,%d\n"),
            Elapsed,Phase,Z->GetUniqueID(),*Z->GetVariantId().ToString(),int32(Z->GetCombatState()),Z->GetHealth(),P.X,P.Y,P.Z,Pelvis.X,Pelvis.Y,Pelvis.Z,Head.X,Head.Y,Head.Z,Z->GetVelocity().Size2D(),
            Z->GetActivePhysicsBodyCount(),Z->GetAwakePhysicsBodyCount(),Z->GetPhysicalContactCount(),Z->GetLivingFallCount(),Z->GetRecoveryCount(),Z->GetRecoveryBlockedCount(),Z->GetRagdollTransitionErrorCm(),Z->GetRecoveryRebaseErrorCm(),
            Z->HasHead(),Z->HasLeftArm(),Z->HasRightArm(),Z->HasLeftLeg(),Audio?Audio->GetFootCueCount():0,Audio?Audio->GetBodyCueCount():0,Audio?Audio->GetAttackCueCount():0,
            Mode->GetRemaining(),Mode->GetKills(),Player->GetHealth(),HealthRestores);
    }
}
void AONE07PhysicalityCheck::Tick(float Dt)
{
    Super::Tick(Dt); Elapsed+=Dt;
    if (bFinishing)
    {
        if (bCapture && !Capture->HasFailed() && !Capture->FinishCaptureAndWait()) return;
        if (!bWritten) WriteResults();
        if (Elapsed-FinishAt>1.f) FPlatformMisc::RequestExit(false);
        return;
    }
    if (!Player)
    {
        if (Elapsed<.7f) return;
        Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
        Controller=Cast<AONEPlayerController>(UGameplayStatics::GetPlayerController(this,0));
        Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
        Check(Player && Controller && Mode,TEXT("Runtime player/controller/game mode ready"));
        if (!Player || !Controller || !Mode) { Finish(); return; }
        if (bCapture)
        {
            const bool Started=Capture->BeginCapture(Folder/TEXT("Media"));
            Check(Started,TEXT("Bounded actual engine viewport/master-output capture began"));
            if (!Started) { Finish(); return; }
        }
        if (bEncounter) { Phase=0; Label=TEXT("ORDINARY ENCOUNTER / PRODUCTION CONTROLLER INPUT"); }
        else EnterPhase();
    }
    if (bCapture) Capture->SetPhaseLabel(Label);
    if (bEncounter) { DriveEncounter(Dt); Observe(Dt); return; }
    const float Age=Elapsed-PhaseStart;
    if (Phase>=2 && Player->GetHealth()<45.f) { Player->GetHealthComponent()->Restore(); ++HealthRestores; }
    if ((Phase==1 || Phase==4) && Age>1.f && Action==0)
    {
        const int32 Count=Phase==1?1:FMath::Min(2,Subjects.Num());
        for (int32 I=0;I<Count;++I)
        {
            const float Before=Subjects[I]->GetHealth();
            if (Phase==1) Subjects[I]->GetCharacterMovement()->StopMovementImmediately();
            const FVector Impulse=Phase==1?FVector(-320,20,15):FVector(220,30,15);
            Check(Subjects[I]->TryLivingFall(Impulse,TEXT("explicit_recorded_probe")),TEXT("Declared probe entered living physics"));
            Check(Subjects[I]->GetHealth()==Before && !Subjects[I]->IsDead(),TEXT("Falling retained health and living identity"));
        }
        Check(Mode->GetKills()==StartKills && Mode->GetRemaining()==StartRemaining,TEXT("Fall did not count a kill or remove registered population"));
        ++Action;
    }
    if (Phase==4 && Age>7.5f && Action==1)
    {
        // A disclosed production-input escape clears the traffic that blocked
        // the fallen actors. No actor is moved or retired to create clearance.
        Key(EKeys::D,true); Action=2;
        Label=TEXT("MIXED FALLS / PRODUCTION D ESCAPE / LOCAL RECOVERY CLEARANCE");
    }
    if (Phase==4 && Age>9.f && Action==2) { Key(EKeys::D,false); Action=3; }
    if (Phase==5 && Age>2.f+Action*2.2f && Action<4 && Subjects.IsValidIndex(Action))
    {
        auto* Z=Subjects[Action].Get(); const EONEHitRegion Regions[]={EONEHitRegion::ArmLeft,EONEHitRegion::ArmRight,EONEHitRegion::Head,EONEHitRegion::LegLeft};
        Z->ReceiveWeaponDamage(ProbePacket(790000+Action,Regions[Action],Action==2?200.f:1.f,100.f,Z->GetMesh()->GetSocketLocation(TEXT("spine_02"))));
        if (!Z->IsDead()) Z->ReceiveWeaponDamage(ProbePacket(791000+Action,EONEHitRegion::Body,200.f,0,Z->GetMesh()->GetSocketLocation(TEXT("spine_01"))));
        Check(Z->IsDead(),TEXT("Declared sever/death probe transitioned once")); ++Action;
    }
    Observe(Dt);
    const float Durations[]={6.f,10.f,10.f,12.f,14.f,16.f,12.f};
    if (Phase>=0 && Age>=Durations[Phase])
    {
        if (Phase==1) Check(Subjects.Num()==1 && Subjects[0]->GetRecoveryCount()>0,TEXT("One living fall recovered in local clear space"));
        if (Phase==4)
        {
            int32 Recovered=0;
            for (const auto& Subject:Subjects) if (Subject && Subject->GetRecoveryCount()>0) ++Recovered;
            Check(Recovered>0,TEXT("A living fall in the mixed group completed local recovery"));
        }
        EnterPhase();
    }
}
void AONE07PhysicalityCheck::Finish()
{ if (bFinishing) return; bFinishing=true; FinishAt=Elapsed; ReleaseKeys(); if (!bCapture) WriteResults(); }
void AONE07PhysicalityCheck::WriteResults()
{
    if (bWritten) return;
    if (bCapture) Check(Capture->IsComplete() && !Capture->HasFailed(),TEXT("Actual media finalized with a real audio tail"));
    Report+=FString::Printf(TEXT("\nChecks=%d Failures=%d HealthRestores=%d InputEdges=%d Encounter=%d\n"),Checks,Failures,HealthRestores,InputEdges,bEncounter);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(Telemetry,*(Folder/TEXT("observations.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(Inputs,*(Folder/TEXT("input_events.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    bWritten=true;
    UE_LOG(LogTemp,Display,TEXT("ONE07_PHYSICALITY_COMPLETE checks=%d failures=%d encounter=%d phases=%d frames=%d"),Checks,Failures,bEncounter,Phase,Capture->GetFrameCount());
}
void AONE07PhysicalityCheck::EndPlay(const EEndPlayReason::Type Reason)
{ ReleaseKeys(); if (ProbeObstacle) ProbeObstacle->Destroy(); Super::EndPlay(Reason); }
