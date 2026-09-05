#include "ONE04PresentationCheck.h"
#include "ONE04MachinePresentation.h"
#include "ONEProgressionMachine.h"
#include "ONEInteractionComponent.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEWeaponComponent.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
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
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(ONEProgression,true);

AONE04PresentationCheck::AONE04PresentationCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE04PresentationCheck::BeginPlay()
{
    Super::BeginPlay();
    bManual=FParse::Value(FCommandLine::Get(),TEXT("ONE04ManualCapture="),ManualDuration);
    bProfile=FParse::Param(FCommandLine::Get(),TEXT("ONE04Profile"));
    bCapture=bManual || FParse::Param(FCommandLine::Get(),TEXT("ONE04PresentationCapture"));
    ManualDuration=FMath::Clamp(ManualDuration,15.f,300.f);
    FParse::Value(FCommandLine::Get(),TEXT("ONE04ProfileEnemies="),EnemyCount);
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate04")/(bManual?TEXT("Manual"):bProfile?TEXT("Profile"):TEXT("PresentationCapture"));
    Folder/=FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate04 machine/inventory presentation\n");
    Report+=bManual?TEXT("Passive native-input recorder: no scripted keys, cursor, movement, health, ammo or transactions.\n"):
        TEXT("Scripted production PlayerController key dispatch and projected mouse cursor over real frames; not native human input. No teleports, camera overrides, direct inventory installs or direct machine commits. T/X/C are the disclosed ordinary sandbox grant/forced-roll controls.\n");
    if (bProfile) Report+=TEXT("Profile: no screenshots/audio capture. Production-input acquisition setup and initial actor/asset construction precede CSV and are not measured by this scenario. CSV includes deposit, both-machine overlap, combat and ready-state tail without removal of spikes. Registered sandbox enemies are replenished toward the requested live count; actual counts are recorded. Player health is restored during this stress fixture only. This is not an ordinary survival claim or an identical workload to older stationary living-only profiles.\n");
    FramesCsv=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation\n");
    InputCsv=TEXT("world_seconds,frame,phase,key,event,handled\n");
    ObservationCsv=TEXT("world_seconds,phase,profile_seconds,live,box_state,upgrade_state,box_previews,upgrade_previews,box_loops,upgrade_loops,weapon,family,upgraded,ammo,reserve,operation,shots,magazine_drops,live_magazines,cases,points,health,x,y,z,interaction_progress,dim,cursor_x,cursor_y,aim_x,aim_y,aim_z,body_yaw\n");
    ChaptersCsv=TEXT("phase,world_seconds,label\n");
    Check(!(bProfile && bCapture),TEXT("Profile and media capture modes are mutually exclusive"));
    Check(!bProfile || EnemyCount==6 || EnemyCount==12 || EnemyCount==18,TEXT("Profile count is exactly 6, 12 or 18"));
    if (Failures) { Finish(false); return; }
    if (bCapture)
    {
        UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE04PresentationCheck::Screenshot);
        if (auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
            if (auto Master=Mixer->GetMasterSubmix().Pin()) Mixer->AudioRenderThreadCommand([Master](){Master->SetAutoDisable(false);});
    }
}
void AONE04PresentationCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; Failures+=Pass?0:1;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE04_PRESENTATION %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE04PresentationCheck::Plan()
{
    auto Add=[this](EStep Kind,const TCHAR* Label,float Seconds=1.f)->FStep&
    { auto& S=Steps.AddDefaulted_GetRef(); S.Kind=Kind; S.Label=Label; S.Seconds=Seconds; return S; };
    auto Tap=[&](const FKey& K,const TCHAR* Label){Add(EStep::Tap,Label,.16f).Key=K;};
    auto Walk=[&](int32 M,const TCHAR* Label){Add(EStep::Walk,Label,15.f).Machine=M;};
    auto Hold=[&](int32 M,const TCHAR* Label){Add(EStep::Hold,Label,4.f).Machine=M;};
    auto State=[&](int32 M,EONEMachineState Value,const TCHAR* Label,float Timeout)
    { auto& S=Add(EStep::WaitState,Label,Timeout); S.Machine=M; S.State=int32(Value); };
    auto Select=[&](EONEWeaponFamily F,const TCHAR* Label){Add(EStep::Select,Label,3.f).Family=F;};
    auto Gun=[&](EONEWeaponFamily F,bool U,const TCHAR* Label)
    {
        auto& V=Add(EStep::Verify,Label,.2f); V.Family=F; V.bUpgraded=U;
        Add(EStep::Fire,Label,F==EONEWeaponFamily::Shotgun?4.f:2.4f);
        Add(EStep::Reload,TEXT("ACTUAL RELOAD / OLD MAGAZINE DROP OR INDIVIDUAL SHELLS"),F==EONEWeaponFamily::Shotgun?8.f:3.5f);
    };
    auto Buy=[&](EONEWeaponFamily F)
    {
        Tap(F==EONEWeaponFamily::Carbine?EKeys::X:EKeys::C,TEXT("DISCLOSED SANDBOX NEXT-ROLL FAMILY"));
        Walk(0,TEXT("WASD THROUGH CENTER AISLE TO MYSTERY BOX"));
        Hold(0,TEXT("HOLD F / PAY 950 / OPEN AND CYCLE"));
        State(0,EONEMachineState::Ready,TEXT("ACTUAL BOX PREVIEW CYCLE / WAIT FOR REWARD"),7.f);
        Add(EStep::Wait,TEXT("READY REWARD WAITS FOR DELIBERATE COLLECTION"),1.3f);
        Hold(0,TEXT("F RELEASED THEN HELD AGAIN / COLLECT BOX REWARD"));
        Add(EStep::Wait,TEXT("ACTUAL COLLECTION AND EQUIP"),1.f);
        Select(F,TEXT("SELECT THE ACQUIRED INSTANCE THROUGH SLOT INPUT"));
    };
    auto UpgradeGun=[&](EONEWeaponFamily F)
    {
        Walk(1,TEXT("WASD THROUGH CENTER AISLE TO PACK-A-PUNCH"));
        Hold(1,TEXT("HOLD F / PHYSICAL HANDOFF / PAY 5000"));
        State(1,EONEMachineState::Active,TEXT("HANDOFF ACCEPTANCE / RESERVED INSTANCE"),2.f);
        Add(EStep::Wait,TEXT("REMAINING HANDOFF / RETURN MOVEMENT CONTROL"),.28f);
        Add(EStep::Retreat,TEXT("WASD RETREAT 180 CM FROM INTAKE WHILE PROCESSING"),3.f).Machine=1;
        Tap(EKeys::F2,TEXT("DISCLOSED SANDBOX ENEMY FOR OTHER-WEAPON COMBAT"));
        Add(EStep::Fire,TEXT("FIGHT WITH AVAILABLE WEAPON DURING REAL PROCESSING"),4.5f);
        State(1,EONEMachineState::Ready,TEXT("NINE-SECOND PROCESS / PHYSICAL OUTPUT"),11.f);
        Add(EStep::Wait,TEXT("READY UPGRADE WAITS / NO AUTOMATIC PICKUP"),1.3f);
        Walk(1,TEXT("RETURN TO OUTPUT CONTACT POSITION"));
        Hold(1,TEXT("FRESH HOLD F / RETRIEVE THE SAME UPGRADED INSTANCE"));
        Add(EStep::Wait,TEXT("HAND RETRIEVAL / NORMAL EQUIP VISIBLE SWAP"),1.f);
        Select(F,TEXT("SELECT RETURNED UPGRADED SLOT"));
    };
    Tap(EKeys::T,TEXT("DISCLOSED SANDBOX +10000 POINTS"));
    Add(EStep::Wait,TEXT("RELEASED T / SEPARATE INPUT FRAME"),.2f);
    Tap(EKeys::T,TEXT("DISCLOSED SANDBOX +10000 POINTS"));
    if (!bProfile) Gun(EONEWeaponFamily::Pistol,false,TEXT("M1911 / BASE SEMIAUTOMATIC FIRE AND RELOAD"));
    Buy(EONEWeaponFamily::Carbine);
    if (bProfile)
    {
        Walk(1,TEXT("PROFILE SETUP / WALK TO UPGRADE CONTACT"));
        Add(EStep::StartProfile,TEXT("CSV START / REGISTERED COMBAT FIXTURE"),12.f);
        Hold(1,TEXT("PROFILE / PHYSICAL DEPOSIT AND INTAKE"));
        State(1,EONEMachineState::Active,TEXT("PROFILE / NINE-SECOND PROCESS START"),2.f);
        Tap(EKeys::C,TEXT("PROFILE / DISCLOSED NEXT SHOTGUN REWARD"));
        Walk(0,TEXT("PROFILE / WALK TO SECOND MACHINE WHILE PROCESSING"));
        Hold(0,TEXT("PROFILE / START SECOND MACHINE DURING UPGRADE"));
        Add(EStep::Combat,TEXT("PROFILE / COMBAT WITH BOTH MACHINES PRESENT"),25.f);
        return;
    }
    Gun(EONEWeaponFamily::Carbine,false,TEXT("M4A1 / BASE AUTOMATIC FIRE AND MAGAZINE RELOAD"));
    Tap(EKeys::F7,TEXT("F7 / DIM ARENA / COMPLETE PACK-A-PUNCH LIGHTING CYCLE"));
    UpgradeGun(EONEWeaponFamily::Carbine);
    Tap(EKeys::F7,TEXT("F7 / BRIGHT ARENA / READABLE WEAPON MECHANICS"));
    Gun(EONEWeaponFamily::Carbine,true,TEXT("OVERCURRENT / ACTUAL UPGRADED ASSEMBLY AND FIRE"));
    Select(EONEWeaponFamily::Pistol,TEXT("SELECT ORIGINAL PISTOL INSTANCE"));
    Add(EStep::Reload,TEXT("RESTORE PISTOL MAGAZINE THROUGH ACTUAL RELOAD"),5.f);
    UpgradeGun(EONEWeaponFamily::Pistol);
    Gun(EONEWeaponFamily::Pistol,true,TEXT("LAST WORD / ACTUAL UPGRADED PISTOL AND MAGAZINE DROP"));
    Select(EONEWeaponFamily::Carbine,TEXT("SELECT AVAILABLE SLOT FOR NEXT BOX ACQUISITION"));
    Tap(EKeys::F7,TEXT("F7 / DIM ARENA / COMPLETE MYSTERY BOX LIGHTING CYCLE"));
    Buy(EONEWeaponFamily::Shotgun);
    Tap(EKeys::F7,TEXT("F7 / BRIGHT ARENA / READABLE SHOTGUN MECHANICS"));
    Gun(EONEWeaponFamily::Shotgun,false,TEXT("REMINGTON 870 / BASE PUMP AND SHELL RELOAD"));
    UpgradeGun(EONEWeaponFamily::Shotgun);
    Gun(EONEWeaponFamily::Shotgun,true,TEXT("GRAVEBREAKER / ACTUAL UPGRADED PUMP AND SHELL RELOAD"));
    Add(EStep::Wait,TEXT("FINAL INVENTORY / BOTH MACHINES RETURNED TO IDLE"),3.f);
}
void AONE04PresentationCheck::Key(const FKey& K,bool Down)
{
    if (!Controller || bManual || Held.Contains(K)==Down) return;
    const bool Handled=Controller->InputKey(FInputKeyEventArgs::CreateSimulated(K,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
    if (Down) Held.Add(K); else Held.Remove(K);
    InputCsv+=FString::Printf(TEXT("%.6f,%llu,%d,%s,%s,%d\n"),Elapsed,static_cast<unsigned long long>(GFrameCounter),Phase,*K.ToString(),Down?TEXT("pressed"):TEXT("released"),Handled);
}
void AONE04PresentationCheck::ReleaseKeys()
{ const TArray<FKey> Keys=Held.Array(); for (const FKey& K:Keys) Key(K,false); }
void AONE04PresentationCheck::AimAt(const FVector& Point)
{
    if (!Controller || bManual) return;
    FVector2D Screen;
    if (Controller->ProjectWorldLocationToScreen(Point,Screen))
    {
        int32 W=0,H=0; Controller->GetViewportSize(W,H);
        Controller->SetMouseLocation(FMath::Clamp(FMath::RoundToInt(Screen.X),1,FMath::Max(1,W-2)),FMath::Clamp(FMath::RoundToInt(Screen.Y),1,FMath::Max(1,H-2)));
    }
}
AONEProgressionMachine* AONE04PresentationCheck::Machine(int32 Index) const
{ return Index==0?Box.Get():Upgrade.Get(); }
FVector AONE04PresentationCheck::ContactPoint(const AONEProgressionMachine* M) const
{ return M->GetPresentation()->GetComponentTransform().TransformPosition(FVector(M->IsBox()?125.f:170.f,0,96)); }
bool AONE04PresentationCheck::WalkTo(AONEProgressionMachine* M)
{
    const FVector Target=WalkLeg==0?FVector(0,-530,96):ContactPoint(M);
    const FVector Delta=Target-Player->GetActorLocation();
    AimAt(M->GetInteractionPoint());
    Key(EKeys::LeftShift,bProfile && bCsvStarted);
    if (Delta.Size2D()<13.f)
    {
        for (const FKey& K:{EKeys::W,EKeys::A,EKeys::S,EKeys::D}) Key(K,false);
        if (WalkLeg==0) { WalkLeg=1; return false; }
        return true;
    }
    Key(EKeys::D,Delta.X>8.f); Key(EKeys::A,Delta.X< -8.f);
    Key(EKeys::S,Delta.Y>8.f); Key(EKeys::W,Delta.Y< -8.f);
    return false;
}
void AONE04PresentationCheck::EnterStep()
{
    if (!Steps.IsValidIndex(Phase)) { Finish(true); return; }
    const FStep& S=Steps[Phase]; Segment=S.Label; StepStart=Elapsed; StepFirstFrame=GFrameCounter;
    ShotsAtStep=Player->GetWeaponComponent()->GetTotalShotsFired();
    DropsAtStep=Player->GetWeaponComponent()->GetMagazineDropCount();
    HoldCountAtStep=Player->GetInteractionComponent()->GetCompletedHolds();
    WalkLeg=0; FirePulses=0; NextFire=0;
    RetreatStart=Player->GetActorLocation();
    ChaptersCsv+=FString::Printf(TEXT("%d,%.6f,%s\n"),Phase,Elapsed,*Segment);
    UE_LOG(LogTemp,Display,TEXT("ONE04_PHASE phase=%d seconds=%.6f label=%s"),Phase,Elapsed,*Segment);
    if (S.Kind==EStep::Tap) Key(S.Key,true);
}
void AONE04PresentationCheck::Advance()
{ ReleaseKeys(); ++Phase; EnterStep(); }
void AONE04PresentationCheck::FireInput(float Time)
{
    auto* W=Player->GetWeaponComponent();
    FVector Target=Player->GetActorLocation()+FVector(-100,380,35);
    float Closest=BIG_NUMBER;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
        if (!It->IsDead())
        {
            const float Distance=FVector::DistSquared(It->GetActorLocation(),Player->GetActorLocation());
            if (Distance<Closest) { Closest=Distance; Target=It->GetActorLocation()+FVector(0,0,15); }
        }
    AimAt(Target);
    if (!W->HasUsableWeapon()) { Key(EKeys::LeftMouseButton,false); return; }
    if (W->GetDefinition().bAutomatic) Key(EKeys::LeftMouseButton,true);
    else if (Held.Contains(EKeys::LeftMouseButton)) Key(EKeys::LeftMouseButton,false);
    else if (Time>=NextFire && W->CanFire())
    { Key(EKeys::LeftMouseButton,true); ++FirePulses; NextFire=Time+FMath::Max(.18f,W->GetDefinition().FireInterval+.04f); }
}
void AONE04PresentationCheck::RunStep(float Dt)
{
    const int32 PreviousPhase=Phase;
    const FStep S=Steps[Phase]; const float T=Elapsed-StepStart;
    auto* W=Player->GetWeaponComponent();
    // No step may press and release a key within the same game frame.
    if (GFrameCounter<=StepFirstFrame) return;
    switch (S.Kind)
    {
    case EStep::Wait: if (T>=S.Seconds) Advance(); break;
    case EStep::Tap: if (T>=S.Seconds) Advance(); break;
    case EStep::Walk:
        if (WalkTo(Machine(S.Machine)))
        {
            Check(Machine(S.Machine)->CanReach(Player),TEXT("Actual WASD route reached machine interaction range"));
            if (Failures) Finish(false); else Advance();
        }
        break;
    case EStep::Retreat:
    {
        const AONEProgressionMachine* M=Machine(S.Machine);
        const FVector Target=ContactPoint(M)+M->GetPresentation()->GetForwardVector()*180.f;
        const FVector Delta=Target-Player->GetActorLocation();
        AimAt(M->GetInteractionPoint());
        Key(EKeys::D,Delta.X>8.f); Key(EKeys::A,Delta.X< -8.f);
        Key(EKeys::S,Delta.Y>8.f); Key(EKeys::W,Delta.Y< -8.f);
        if (Delta.Size2D()<13.f)
        {
            Check(FVector::Dist2D(Player->GetActorLocation(),RetreatStart)>=150.f && M->GetState()==EONEMachineState::Active,
                TEXT("Actual WASD moved away from the intake while the reserved weapon continued processing"));
            if (Failures) Finish(false); else Advance();
        }
        else if (T>=S.Seconds)
        { Check(false,TEXT("Production-input retreat did not reach its target within three seconds")); Finish(false); }
        break;
    }
    case EStep::Hold:
        AimAt(Machine(S.Machine)->GetInteractionPoint());
        if (T>.20f) Key(EKeys::F,true);
        if (Player->GetInteractionComponent()->GetCompletedHolds()>HoldCountAtStep)
        { Check(true,TEXT("Production F hold committed through interaction component")); Advance(); }
        break;
    case EStep::WaitState:
        AimAt(Machine(S.Machine)->GetInteractionPoint());
        if (int32(Machine(S.Machine)->GetState())==S.State)
        { Check(true,TEXT("Observed expected actual machine state")); Advance(); }
        break;
    case EStep::Select:
    {
        const auto* Selected=W->GetSlotState(W->GetEquippedIndex());
        if (Selected && Selected->Status==EONEWeaponSlotStatus::Available && Selected->Family==S.Family && !W->IsBusy())
        { Advance(); break; }
        for (int32 I=0;I<2;++I)
            if (const auto* Slot=W->GetSlotState(I); Slot && Slot->Family==S.Family && Slot->Status==EONEWeaponSlotStatus::Available)
                Key(I==0?EKeys::One:EKeys::Two,true);
        break;
    }
    case EStep::Verify:
    {
        const auto* D=W->GetDefinitionForWeapon(W->GetEquippedIndex());
        const bool Match=D && D->Family==S.Family && D->bUpgraded==S.bUpgraded;
        Check(Match,TEXT("Comparison uses the requested actually acquired family and variant"));
        if (Match) { ++CompletedConfigurations; Advance(); } else Finish(false);
        break;
    }
    case EStep::Fire:
        FireInput(T);
        if (T>=S.Seconds)
        {
            Check(W->GetTotalShotsFired()-ShotsAtStep>=2,TEXT("At least two real discharges committed during the labeled fire segment"));
            if (Failures) Finish(false); else Advance();
        }
        break;
    case EStep::Reload:
        // A pump/recoil already underway must finish before a new R edge.
        // Keeping a rejected R press held would not produce a later reload.
        if (FirePulses==0 && !W->IsBusy() && W->HasUsableWeapon() && W->GetAmmo()<W->GetDefinition().Capacity)
        { Key(EKeys::R,true); FirePulses=1; NextFire=T+.16f; }
        else if (FirePulses>0 && T>NextFire) Key(EKeys::R,false);
        if (T>=S.Seconds)
        {
            Check(!W->IsReloading(),TEXT("Real reload completed before the next machine action"));
            if (W->HasUsableWeapon() && !W->GetDefinition().bShellReload && W->GetAmmo()<W->GetDefinition().Capacity)
                Check(false,TEXT("Magazine reload restored the actual loaded ammunition"));
            if (Failures) Finish(false); else Advance();
        }
        break;
    case EStep::StartProfile: if (StartProfile()) Advance(); break;
    case EStep::Combat:
        FireInput(T);
        // Stay in the open center route without changing actor location directly.
        // Small alternating strafe segments exercise movement while preserving the
        // existing arena and machine placement. Sprint would cancel real reloads.
        Key(EKeys::D,T<2.f || (T>=6.f && T<8.f));
        Key(EKeys::A,T>=3.f && T<5.f);
        if (T>=S.Seconds) Advance();
        break;
    }
    if (!bFinished && Phase==PreviousPhase && T>S.Seconds+2.f)
    { Check(false,TEXT("Current production-input step exceeded its bounded timeout: ")+S.Label); Finish(false); }
}
void AONE04PresentationCheck::ReplenishEnemies()
{
    int32 Missing=FMath::Max(0,EnemyCount-Live);
    // Explicit registered, floor/path-validated grid in the unchanged open arena.
    // SpawnSandboxEnemies is a six-enemy convenience command and is deliberately
    // not used for 12/18-count fixtures. Only successful returns count as spawns.
    for (int32 I=0;I<18 && Missing>0;++I)
    {
        const FVector Point(-325+(I%6)*130,220+(I/6)*145,98);
        if (Mode->SpawnSandboxEnemyAt(Point)) { --Missing; ++Spawned; }
    }
}
void AONE04PresentationCheck::Observe(float Dt)
{
    Live=0;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (!It->IsDead()) ++Live;
    if (bProfile && bCsvStarted && !bFinished)
    {
        ProfileSeconds+=Dt;
        ProfileMaximumLive=FMath::Max(ProfileMaximumLive,Live);
        ++ProfileSamples; if (Live==EnemyCount) ++ExactCountSamples;
        if (Box->GetState()==EONEMachineState::Active && Upgrade->GetState()==EONEMachineState::Active) BothActiveSeconds+=Dt;
        // Deliberate stress-fixture protection, never used by either recording mode.
        Player->GetHealthComponent()->Restore();
        if (Elapsed>=NextReplenish)
        {
            NextReplenish=Elapsed+.5f;
            if (Live<EnemyCount) ReplenishEnemies();
        }
        CSV_CUSTOM_STAT(ONEProgression,Live,Live,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONEProgression,RequestedLive,EnemyCount,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONEProgression,BoxState,int32(Box->GetState()),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONEProgression,UpgradeState,int32(Upgrade->GetState()),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONEProgression,Phase,Phase,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONEProgression,ProfileSeconds,ProfileSeconds,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONEProgression,LiveMagazines,Player->GetWeaponComponent()->GetLiveMagazineCount(),ECsvCustomStatOp::Set);
    }
    if (Elapsed<NextObservation) return;
    NextObservation=Elapsed+.1f;
    const auto* W=Player->GetWeaponComponent(); const auto* D=W->GetDefinitionForWeapon(W->GetEquippedIndex());
    const FVector P=Player->GetActorLocation(),Aim=Player->GetAimPoint();
    float MouseX=-1,MouseY=-1; Controller->GetMousePosition(MouseX,MouseY);
    const auto* B=Box?Box->GetPresentation():nullptr; const auto* U=Upgrade?Upgrade->GetPresentation():nullptr;
    ObservationCsv+=FString::Printf(TEXT("%.6f,%d,%.6f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%.2f,%.2f,%.4f,%.4f,%.4f,%.4f\n"),
        Elapsed,Phase,ProfileSeconds,Live,Box?int32(Box->GetState()):-1,Upgrade?int32(Upgrade->GetState()):-1,
        B?B->GetVisiblePreviewPartCount():0,U?U->GetVisiblePreviewPartCount():0,B?B->GetActiveLoopCount():0,U?U->GetActiveLoopCount():0,
        W->GetEquippedIndex(),D?int32(D->Family):-1,D?D->bUpgraded:false,W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()),
        W->GetTotalShotsFired(),W->GetMagazineDropCount(),W->GetLiveMagazineCount(),W->GetLiveCaseCount(),Mode->GetPoints(),Player->GetHealth(),P.X,P.Y,P.Z,
        Player->GetInteractionComponent()->GetProgress(),Mode->IsSandboxDimLighting(),MouseX,MouseY,Aim.X,Aim.Y,Aim.Z,Player->GetBodyFacingYaw());
}
void AONE04PresentationCheck::Capture()
{
    if (!bCapture || !bRecording || bFinished || bFinishRequested) return;
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1./30.)
    { LastCapture=Now; PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames); FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false); }
}
void AONE04PresentationCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty() || !Player) return;
    const double At=FPlatformTime::Seconds()-AudioStart;
    const auto* W=Player->GetWeaponComponent();
    const FString Row=FString::Printf(TEXT("%s,%.6f,%.6f,%d,%d,%d,%d,%d\n"),*PendingFrame,At,Elapsed,Phase,W->GetEquippedIndex(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()));
    if (FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),Width,Height),85))
    { FramesCsv+=Row; ++Frames; }
    else Check(false,TEXT("Requested native screenshot could not be written"));
    PendingFrame.Empty();
    if (bFinishRequested) Finish(bRequestedComplete);
}
bool AONE04PresentationCheck::StartProfile()
{
#if CSV_PROFILER
    auto* P=FCsvProfiler::Get();
    if (!bCsvRequested)
    {
        if (P->IsCapturing() || P->IsWritingFile() || P->IsEndCapturePending())
        { Check(false,TEXT("Scenario requires exclusive ownership of an idle CSV profiler")); Finish(false); return false; }
        for (const TCHAR* C:{TEXT("Chaos"),TEXT("PhysicsVerbose"),TEXT("PhysicsCounters"),TEXT("ONEPhysicality"),TEXT("ONEProgression")})
            Check(P->EnableCategoryByString(C),FString(TEXT("Required CSV category enabled: "))+C);
        if (Failures) { Finish(false); return false; }
        CsvFolder=FPaths::ConvertRelativePathToFull(Folder/TEXT("CSV")); IFileManager::Get().MakeDirectory(*CsvFolder,true);
        CSV_METADATA(TEXT("one_scenario"),TEXT("candidate04_two_machines_production_input_combat"));
        CSV_METADATA(TEXT("one_media_capture"),TEXT("none"));
        CSV_METADATA(TEXT("one_requested_enemies"),*FString::FromInt(EnemyCount));
        P->BeginCapture(-1,CsvFolder); bCsvRequested=true; CsvRequestAt=FPlatformTime::Seconds(); return false;
    }
    if (P->IsCapturing())
    {
        bCsvStarted=true; ReplenishEnemies(); NextReplenish=Elapsed+.5f;
        CSV_EVENT(ONEProgression,TEXT("ONE04_PROFILE_BEGIN requested=%d"),EnemyCount);
        UE_LOG(LogTemp,Display,TEXT("ONE04_PROFILE_STARTED enemies=%d world_seconds=%.6f"),EnemyCount,Elapsed);
        return true;
    }
    if (FPlatformTime::Seconds()-CsvRequestAt>10.) { Check(false,TEXT("CSV did not start within ten seconds")); Finish(false); }
    return false;
#else
    Check(false,TEXT("This executable lacks CSV_PROFILER support")); Finish(false); return false;
#endif
}
bool AONE04PresentationCheck::FinishProfileWrite()
{
#if CSV_PROFILER
    auto* P=FCsvProfiler::Get();
    if (bCsvStarted && !bCsvFinished)
    {
        if (!CsvCompletion.IsValid() || !CsvCompletion.IsReady()) return false;
        const FString File=CsvCompletion.Get();
        Check(!File.IsEmpty() && IFileManager::Get().FileSize(*File)>0,TEXT("CSV writer finalized a nonempty complete output"));
        Report+=TEXT("\nEngine CSV output: ")+File+TEXT("\n");
        UE_LOG(LogTemp,Display,TEXT("ONE04_PROFILE_WRITTEN file=%s enemies=%d"),*File,EnemyCount);
        bCsvFinished=true; WriteResults();
    }
    if (P->IsCapturing() || P->IsWritingFile() || P->IsEndCapturePending()) return false;
#endif
    return true;
}
void AONE04PresentationCheck::Finish(bool Complete)
{
    if (bFinished) return;
    ReleaseKeys();
    if (bRecording && !PendingFrame.IsEmpty())
    { bFinishRequested=true; bRequestedComplete=Complete; return; }
    bFinished=true; bComplete=Complete; FinishedAt=FPlatformTime::Seconds(); ReleaseKeys();
    if (bRecording)
    {
        UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("gameplay_master"),FPaths::ConvertRelativePathToFull(Folder));
        bRecording=false;
    }
    if (bProfile && bCsvStarted)
    {
        Check(BothActiveSeconds>=.5f,TEXT("Both machines actually processed concurrently for at least half a second"));
        Check(ProfileSeconds>=25.f && ProfileSamples>0,TEXT("Profile retained at least 25 seconds of actual scenario frames"));
        Check(ProfileMaximumLive==EnemyCount && ExactCountSamples>0,TEXT("Requested live count was actually achieved and recorded"));
#if CSV_PROFILER
        CSV_EVENT(ONEProgression,TEXT("ONE04_PROFILE_END samples=%d overlap=%.6f"),ProfileSamples,BothActiveSeconds);
        CsvCompletion=FCsvProfiler::Get()->EndCapture();
        if (!CsvCompletion.IsValid()) { Check(false,TEXT("CSV end request was rejected")); bCsvFinished=true; }
#endif
    }
    if (Complete && !bManual && !bProfile) Check(CompletedConfigurations==6,TEXT("All six actual catalog configurations were acquired and shown"));
    if (Complete && !bManual && !bProfile) Check(Player->GetWeaponComponent()->GetMagazineDropCount()>=4,TEXT("At least four actual pistol/rifle old-magazine releases occurred in the recorded sequence"));
    if (!bProfile || !bCsvStarted || bCsvFinished) WriteResults();
}
void AONE04PresentationCheck::WriteResults()
{
    if (bResultsWritten) return; bResultsWritten=true;
    Report+=FString::Printf(TEXT("\nComplete: %d\nChecks: %d\nFailures: %d\nFrames: %d\nProfile requested live: %d\nProfile actual maximum live: %d\nProfile actual frames: %d\nFrames at exact requested count: %d\nBoth active seconds: %.6f\nSuccessful registered spawns: %d\n"),bComplete,Checks,Failures,Frames,EnemyCount,ProfileMaximumLive,ProfileSamples,ExactCountSamples,BothActiveSeconds,Spawned);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    FFileHelper::SaveStringToFile(FramesCsv,*(Folder/TEXT("frames.csv")));
    FFileHelper::SaveStringToFile(InputCsv,*(Folder/TEXT("input_events.csv")));
    FFileHelper::SaveStringToFile(ObservationCsv,*(Folder/TEXT("observations.csv")));
    FFileHelper::SaveStringToFile(ChaptersCsv,*(Folder/TEXT("chapters.csv")));
    UE_LOG(LogTemp,Display,TEXT("ONE04_PRESENTATION_COMPLETE complete=%d failures=%d checks=%d frames=%d profile=%d"),bComplete,Failures,Checks,Frames,bProfile);
}
void AONE04PresentationCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished)
    {
        if (bProfile && !FinishProfileWrite()) return;
        if (!bManual && FPlatformTime::Seconds()-FinishedAt>2.) FPlatformMisc::RequestExit(false);
        return;
    }
    Elapsed+=Dt;
    if (bFinishRequested)
    { if (PendingFrame.IsEmpty()) Finish(bRequestedComplete); return; }
    if (!Player)
    {
        Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
        Controller=Player?Cast<AONEPlayerController>(Player->GetController()):nullptr;
        Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
        for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It)
            if (It->IsBox()) Box=*It; else Upgrade=*It;
    }
    if (!Player || !Controller || !Mode)
    { if (Elapsed>10.f) { Check(false,TEXT("Expected production player/controller/game mode did not initialize")); Finish(false); } return; }
    if (Elapsed<2.f) return;
    if (bCapture && !bRecording)
    { UAudioMixerBlueprintLibrary::StartRecordingOutput(this,bManual?ManualDuration+3:360.f); AudioStart=FPlatformTime::Seconds(); bRecording=true; }
    if (bManual)
    {
        Segment=TEXT("NATIVE KEYBOARD / MOUSE OBSERVATION ONLY"); Observe(Dt); Capture();
        if (FPlatformTime::Seconds()-AudioStart>=ManualDuration) Finish(true);
        return;
    }
    if (Phase<0)
    {
        Check(Mode->IsSandbox(),TEXT("Scripted demonstration uses explicitly labeled sandbox mode"));
        Check(Box && Upgrade && Box->GetPresentation()->IsConfigured() && Upgrade->GetPresentation()->IsConfigured(),TEXT("Both saved-map machines have complete original presentation assets"));
        const auto* W=Player->GetWeaponComponent(); const auto* S0=W->GetSlotState(0); const auto* S1=W->GetSlotState(1);
        Check(S0 && S1 && S0->Family==EONEWeaponFamily::Pistol && !S0->bUpgraded && S1->Status==EONEWeaponSlotStatus::Empty,TEXT("Scenario begins with ordinary M1911 plus empty second slot"));
        if (Failures) { Finish(false); return; }
        Plan(); Phase=0; EnterStep();
    }
    if (Player->IsDead()) { Check(false,TEXT("Player died before scripted sequence completed")); Finish(false); return; }
    Observe(Dt);
    { CSV_SCOPED_TIMING_STAT(ONEProgression,PresentationDriver); RunStep(Dt); }
    Capture();
    if (Elapsed>355.f && !bFinished) { Check(false,TEXT("Scenario exceeded its total bounded runtime")); Finish(false); }
}
void AONE04PresentationCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    // An interrupted level cannot promise delivery of its pending screenshot.
    // Preserve completed rows and mark the run incomplete instead of blocking.
    PendingFrame.Empty();
    if (!bFinished) { Check(false,TEXT("Level transition or process exit interrupted the recording/scenario")); Finish(false); }
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this);
    Super::EndPlay(Reason);
}
