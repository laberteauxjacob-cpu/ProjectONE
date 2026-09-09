#include "ONE05PresentationCheck.h"
#include "ONE04MachinePresentation.h"
#include "ONEProgressionMachine.h"
#include "ONEInteractionComponent.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEWeaponComponent.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEHUD.h"
#include "ONEUITypes.h"
#include "ONEAmbientAudioComponent.h"
#include "ONEZombieAudioComponent.h"
#include "ONEPowerUpComponent.h"
#include "ONE06CaptureComponent.h"
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
CSV_DEFINE_CATEGORY(ONECandidate05Presentation,true);

AONE05PresentationCheck::AONE05PresentationCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE05PresentationCheck::BeginPlay()
{
    Super::BeginPlay();
    LastDriverTime=FPlatformTime::Seconds();
    bManual=FParse::Value(FCommandLine::Get(),TEXT("ONE05ManualCapture="),ManualDuration);
    bProfile=FParse::Param(FCommandLine::Get(),TEXT("ONE05Profile"));
    FParse::Value(FCommandLine::Get(),TEXT("ONE05ProfileWeapon="),ProfileWeapon);
    bProfileUpgraded=ProfileWeapon==TEXT("Overcurrent")||ProfileWeapon==TEXT("Gravebreaker");
    ProfileFamily=(ProfileWeapon==TEXT("870")||ProfileWeapon==TEXT("Gravebreaker"))?EONEWeaponFamily::Shotgun:EONEWeaponFamily::Carbine;
    bCapture=bManual || FParse::Param(FCommandLine::Get(),TEXT("ONE05PresentationCapture"));
    ManualDuration=FMath::Clamp(ManualDuration,15.f,300.f);
    FParse::Value(FCommandLine::Get(),TEXT("ONE05ProfileEnemies="),EnemyCount);
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate05")/(bManual?TEXT("Manual"):bProfile?TEXT("Profile"):TEXT("PresentationCapture"));
    Folder/=FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate06 machine/inventory presentation via legacy ONE05 driver; current tap-deposit, automatic-return and ready-expiry rules. Historical output mode names do not identify an old candidate build.\n");
    Report+=bManual?TEXT("Passive native-input recorder: no scripted keys, cursor, movement, health, ammo or transactions.\n"):
        TEXT("Scripted production PlayerController key dispatch and projected mouse cursor over real frames; not native human input. No teleports, camera overrides, direct inventory installs or direct machine commits. T/X/C are the disclosed ordinary sandbox grant/forced-roll controls.\n");
    if (bProfile) Report+=TEXT("Profile: recording disabled. Real production-input M4A1/Overcurrent or 870/Gravebreaker acquisition precedes CSV. The measured window includes pistol deposit, both-machine overlap, the requested carried weapon's production fire/reload input and the ready tail. Registered sandbox enemies replenish toward the requested count; health restoration and five forced pickup placements are explicit stress-fixture exceptions. Two pickups activate Insta-Kill and Double Points through actual player overlap before CSV, and three additional world drops begin present. Timers and drop lifetimes then expire normally; actual per-frame counters establish their occupancy. Setup actors/assets are warm. All measured spikes remain.\n");
    else if (!bManual) Report+=TEXT("Final game-over presentation uses one declared fatal damage fixture after ordinary acquisition/fire; it is not a survival or natural-death claim. Near-aim cursor uses projected real world points and actual evaluated muzzle traces.\n");
    FramesCsv=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation\n");
    InputCsv=TEXT("world_seconds,frame,phase,key,event,handled\n");
    ObservationCsv=TEXT("world_seconds,phase,profile_seconds,live,box_state,upgrade_state,box_previews,upgrade_previews,box_loops,upgrade_loops,weapon,family,upgraded,ammo,reserve,operation,shots,magazine_drops,live_magazines,cases,points,health,x,y,z,interaction_progress,dim,cursor_x,cursor_y,aim_x,aim_y,aim_z,body_yaw\n");
    if (bProfile)
    {
        ObservationCsv.RemoveAt(ObservationCsv.Len()-1);
        ObservationCsv+=TEXT(",world_drops,insta_kill_seconds,double_points_seconds,last_shot_live_hits,last_shot_corpse_hits,last_shot_contacts,shell_inserts,magazine_commits\n");
    }
    ChaptersCsv=TEXT("phase,world_seconds,label\n");
    Check(!(bProfile && bCapture),TEXT("Profile and media capture modes are mutually exclusive"));
    Check(!bProfile || EnemyCount==6 || EnemyCount==12 || EnemyCount==18,TEXT("Profile count is exactly 6, 12 or 18"));
    Check(!bProfile || ProfileWeapon==TEXT("M4A1") || ProfileWeapon==TEXT("Overcurrent") ||
        ProfileWeapon==TEXT("870") || ProfileWeapon==TEXT("Gravebreaker"),TEXT("Profile explicitly selects M4A1, Overcurrent, 870 or Gravebreaker"));
    if (bProfile) Check(!UONE06CaptureComponent::IsAnyCaptureActive(),TEXT("No Candidate06 viewport/audio capture owns the profile process"));
    if (Failures) { Finish(false); return; }
    if (bCapture)
    {
        UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE05PresentationCheck::Screenshot);
        if (auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
            if (auto Master=Mixer->GetMasterSubmix().Pin()) Mixer->AudioRenderThreadCommand([Master](){Master->SetAutoDisable(false);});
    }
}
void AONE05PresentationCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; Failures+=Pass?0:1;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE05_PRESENTATION %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE05PresentationCheck::Plan()
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
        Add(EStep::NearAim,TEXT("NEAR CURSOR 8 / 35 / 95 CM / FULL CIRCLE / REAL MUZZLE COLLISION"),6.f);
        Add(EStep::Reload,F==EONEWeaponFamily::Shotgun?TEXT("ACTUAL RELOAD / EARNED INDIVIDUAL SHELLS"):TEXT("COMMITTED MAGAZINE / SHIFT AND INVALID FIRE / RELEASE WITHOUT A QUEUED SHOT"),F==EONEWeaponFamily::Shotgun?8.f:3.5f);
        if (F==EONEWeaponFamily::Carbine)
        {
            Add(EStep::Fire,Label,2.4f);
            Add(EStep::Reload,TEXT("COMMITTED RIFLE RELOAD / SHIFT REMAINS HELD"),3.5f);
        }
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
        Tap(EKeys::F,TEXT("TAP F / ACCEPTED TRANSFER / PHYSICAL HANDOFF / PAY 5000"));
        State(1,EONEMachineState::Active,TEXT("HANDOFF ACCEPTANCE / RESERVED INSTANCE"),2.f);
        Add(EStep::Wait,TEXT("REMAINING HANDOFF / RETURN MOVEMENT CONTROL"),.28f);
        Add(EStep::Retreat,TEXT("WASD RETREAT 180 CM FROM INTAKE WHILE PROCESSING"),3.f).Machine=1;
        Tap(EKeys::F2,TEXT("DISCLOSED SANDBOX ENEMY FOR OTHER-WEAPON COMBAT"));
        Add(EStep::Fire,TEXT("FIGHT WITH AVAILABLE WEAPON DURING REAL PROCESSING"),4.5f);
        State(1,EONEMachineState::Ready,TEXT("NINE-SECOND PROCESS / PHYSICAL OUTPUT"),11.f);
        Add(EStep::Wait,TEXT("READY UPGRADE / FIFTEEN-SECOND RETURN DEADLINE WHILE AWAY"),1.3f);
        Walk(1,TEXT("RETURN TO OUTPUT CONTACT POSITION"));
        auto& Returned=Add(EStep::WaitOwned,TEXT("AUTOMATIC SAME-INSTANCE RETURN / NO F REQUIRED"),2.f);
        Returned.Machine=1; Returned.Family=F;
        Add(EStep::Wait,TEXT("HAND RETRIEVAL / NORMAL EQUIP VISIBLE SWAP"),1.f);
        Select(F,TEXT("SELECT RETURNED UPGRADED SLOT"));
    };
    if (!bProfile) Add(EStep::Wait,TEXT("QUIET FACILITY / ORIGINAL SPATIAL AMBIENCE / ORDINARY HUD"),8.f);
    Tap(EKeys::T,TEXT("DISCLOSED SANDBOX +10000 POINTS"));
    Add(EStep::Wait,TEXT("RELEASED T / SEPARATE INPUT FRAME"),.2f);
    Tap(EKeys::T,TEXT("DISCLOSED SANDBOX +10000 POINTS"));
    if (!bProfile) Gun(EONEWeaponFamily::Pistol,false,TEXT("M1911 / BASE SEMIAUTOMATIC FIRE AND RELOAD"));
    Buy(bProfile?ProfileFamily:EONEWeaponFamily::Carbine);
    if (bProfile)
    {
        if (bProfileUpgraded) UpgradeGun(ProfileFamily);
        auto& VerifiedWeapon=Add(EStep::Verify,TEXT("PROFILE SETUP / ACTUAL REQUESTED WEAPON ACQUIRED"),.2f);
        VerifiedWeapon.Family=ProfileFamily; VerifiedWeapon.bUpgraded=bProfileUpgraded;
        Select(EONEWeaponFamily::Pistol,TEXT("PROFILE SETUP / ORIGINAL PISTOL FOR MEASURED DEPOSIT"));
        Walk(1,TEXT("PROFILE SETUP / WALK TO UPGRADE CONTACT"));
        Add(EStep::StartProfile,TEXT("CSV START / REGISTERED COMBAT FIXTURE"),12.f);
        Tap(EKeys::F,TEXT("PROFILE / TAP F DEPOSIT AND PHYSICAL INTAKE"));
        State(1,EONEMachineState::Active,TEXT("PROFILE / NINE-SECOND PROCESS START"),2.f);
        Tap(ProfileFamily==EONEWeaponFamily::Carbine?EKeys::C:EKeys::X,TEXT("PROFILE / DISCLOSED ELIGIBLE OTHER-FAMILY BOX REWARD"));
        Walk(0,TEXT("PROFILE / WALK TO SECOND MACHINE WHILE PROCESSING"));
        Hold(0,TEXT("PROFILE / START SECOND MACHINE DURING UPGRADE"));
        Add(EStep::Combat,TEXT("PROFILE / REQUESTED CARRIED WEAPON FIRE AND RELOAD WITH BOTH MACHINES PRESENT"),25.f);
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
    Tap(EKeys::H,TEXT("H / EXPAND GROUPED CONTROL TRAY"));
    Add(EStep::Wait,TEXT("COMPACT TOOLS / ORIGINAL ICONS AND CONTROL GROUPS"),2.5f);
    Tap(EKeys::H,TEXT("H / CLOSE TOOLS"));
    Tap(EKeys::Escape,TEXT("ESCAPE / PAUSE DESIGNED OVER THE REAL SCENE"));
    Add(EStep::Wait,TEXT("PAUSE / MOUSE POINTER / RESUME RESTART QUIT"),2.5f);
    Add(EStep::ClickResume,TEXT("ACTUAL DRAWN RESUME BUTTON / MOUSE PRESS AND RELEASE"),2.f);
    Add(EStep::Wait,TEXT("RESUMED / NO MENU CLICK FIRES A SHOT"),1.f);
    Add(EStep::Death,TEXT("DECLARED FATAL DAMAGE FIXTURE / GAME OVER / REAL RUN STATS"),4.f);
}
void AONE05PresentationCheck::Key(const FKey& K,bool Down)
{
    if (!Controller || bManual || Held.Contains(K)==Down) return;
    const bool Handled=Controller->InputKey(FInputKeyEventArgs::CreateSimulated(K,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
    if (Down) Held.Add(K); else Held.Remove(K);
    InputCsv+=FString::Printf(TEXT("%.6f,%llu,%d,%s,%s,%d\n"),Elapsed,static_cast<unsigned long long>(GFrameCounter),Phase,*K.ToString(),Down?TEXT("pressed"):TEXT("released"),Handled);
}
void AONE05PresentationCheck::ReleaseKeys()
{ const TArray<FKey> Keys=Held.Array(); for (const FKey& K:Keys) Key(K,false); }
void AONE05PresentationCheck::AimAt(const FVector& Point)
{
    if (!Controller || bManual) return;
    FVector2D Screen;
    if (Controller->ProjectWorldLocationToScreen(Point,Screen))
    {
        int32 W=0,H=0; Controller->GetViewportSize(W,H);
        Controller->SetMouseLocation(FMath::Clamp(FMath::RoundToInt(Screen.X),1,FMath::Max(1,W-2)),FMath::Clamp(FMath::RoundToInt(Screen.Y),1,FMath::Max(1,H-2)));
    }
}
AONEProgressionMachine* AONE05PresentationCheck::Machine(int32 Index) const
{ return Index==0?Box.Get():Upgrade.Get(); }
FVector AONE05PresentationCheck::ContactPoint(const AONEProgressionMachine* M) const
{ return M->GetPresentation()->GetComponentTransform().TransformPosition(FVector(M->IsBox()?125.f:170.f,0,96)); }
bool AONE05PresentationCheck::WalkTo(AONEProgressionMachine* M)
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
void AONE05PresentationCheck::EnterStep()
{
    if (!Steps.IsValidIndex(Phase)) { Finish(true); return; }
    const FStep& S=Steps[Phase]; Segment=S.Label; StepStart=Elapsed; StepFirstFrame=GFrameCounter;
    ShotsAtStep=Player->GetWeaponComponent()->GetTotalShotsFired();
    NearObservedShots=ShotsAtStep; NearRequestedBand=0;
    for (int32& Count:NearBandShots) Count=0;
    DropsAtStep=Player->GetWeaponComponent()->GetMagazineDropCount();
    HoldCountAtStep=Player->GetInteractionComponent()->GetCompletedHolds();
    TapCountAtStep=Player->GetInteractionComponent()->GetCompletedTaps();
    WalkLeg=0; FirePulses=0; NextFire=0;
    RetreatStart=Player->GetActorLocation();
    ChaptersCsv+=FString::Printf(TEXT("%d,%.6f,%s\n"),Phase,Elapsed,*Segment);
    UE_LOG(LogTemp,Display,TEXT("ONE05_PHASE phase=%d seconds=%.6f label=%s"),Phase,Elapsed,*Segment);
    if (S.Kind==EStep::Tap) Key(S.Key,true);
    if (S.Kind==EStep::Combat)
    {
        const auto* D=Player->GetWeaponComponent()->GetDefinitionForWeapon(Player->GetWeaponComponent()->GetEquippedIndex());
        Check(D && D->Family==ProfileFamily && D->bUpgraded==bProfileUpgraded,TEXT("Measured combat starts with the requested actually carried weapon"));
        if (Failures) { Finish(false); return; }
        CSV_EVENT(ONECandidate05Presentation,TEXT("ONE05_UPGRADED_COMBAT_BEGIN phase=%d shots=%d"),Phase,ShotsAtStep);
    }
}
void AONE05PresentationCheck::Advance()
{ ReleaseKeys(); ++Phase; EnterStep(); }
void AONE05PresentationCheck::FireInput(float Time)
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
    if (W->GetDefinition().bAutomatic)
    {
        // Reload/equip interruption requires a new release and press. The driver
        // follows that public rule rather than pretending a held input resumes.
        if (Held.Contains(EKeys::LeftMouseButton))
        { if (!W->IsAutomaticBurstActive() && !W->HasAcceptedFramePress()) Key(EKeys::LeftMouseButton,false); }
        else if (W->CanFire()) Key(EKeys::LeftMouseButton,true);
    }
    else if (Held.Contains(EKeys::LeftMouseButton)) Key(EKeys::LeftMouseButton,false);
    else if (Time>=NextFire && W->CanFire())
    { Key(EKeys::LeftMouseButton,true); ++FirePulses; NextFire=Time+FMath::Max(.18f,W->GetDefinition().FireInterval+.04f); }
}
void AONE05PresentationCheck::RunStep(float Dt)
{
    const int32 PreviousPhase=Phase;
    const FStep S=Steps[Phase]; const float T=Elapsed-StepStart;
    auto* W=Player->GetWeaponComponent();
    // No step may press and release a key within the same game frame.
    if (GFrameCounter<=StepFirstFrame) return;
    switch (S.Kind)
    {
    case EStep::Wait: if (T>=S.Seconds) Advance(); break;
    case EStep::Tap: if (T>=S.Seconds)
        {
            if (S.Key==EKeys::F)
            {
                CapturedUpgrade=Upgrade->GetReservation();
                Check(Player->GetInteractionComponent()->GetCompletedTaps()==TapCountAtStep+1 && CapturedUpgrade.IsValid() &&
                    Upgrade->GetState()==EONEMachineState::Active,TEXT("One production F tap paid and reserved the exact instance without a hold"));
            }
            if (Failures) Finish(false); else Advance();
        } break;
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
    case EStep::WaitOwned:
    {
        const auto* Slot=W->GetSlotState(CapturedUpgrade.Slot);
        if (CapturedUpgrade.IsValid() && W->GetRunId()==CapturedUpgrade.RunId && Slot &&
            Slot->InstanceId==CapturedUpgrade.InstanceId && Slot->Family==S.Family && Slot->bUpgraded &&
            Slot->Status==EONEWeaponSlotStatus::Available)
        { Check(true,TEXT("Approach restored the reserved upgraded instance to its original slot without F")); Advance(); }
        break;
    }
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
        Check(Player->IsHeldAuraVisible()==S.bUpgraded,TEXT("Held aura follows the actual collected variant"));
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
    case EStep::NearAim:
    {
        const int32 NowShots=W->GetTotalShotsFired();
        if (NowShots>NearObservedShots) { NearBandShots[NearRequestedBand]+=NowShots-NearObservedShots; NearObservedShots=NowShots; }
        const FVector Intent=FRotator(0,T*180.f,0).Vector();
        const int32 Band=FMath::Min(2,FMath::FloorToInt(T/2.f));
        const FVector Cursor=Player->GetAimOrigin()+Intent*(Band==0?8.f:Band==1?35.f:95.f);
        AimAt(Cursor);
        if (Held.Contains(EKeys::LeftMouseButton)) Key(EKeys::LeftMouseButton,false);
        else if (T>=NextFire && W->CanFire())
        { NearRequestedBand=Band; Key(EKeys::LeftMouseButton,true); NextFire=T+.90f; }
        if (T>=S.Seconds)
        {
            for (int32 I=0;I<3;++I) Check(NearBandShots[I]>0,FString::Printf(TEXT("Recorded near-cursor section%d committed%d actual discharges"),I,NearBandShots[I]));
            if (Failures) Finish(false); else Advance();
        }
        break;
    }
    case EStep::Reload:
        Key(EKeys::LeftShift,true);
        // A pump/recoil already underway must finish before a new R edge.
        // Keeping a rejected R press held would not produce a later reload.
        if (FirePulses==0 && W->IsMagazineReloadCommitted())
        { Key(EKeys::LeftMouseButton,true); FirePulses=2; NextFire=T+.08f; }
        else if (FirePulses==0 && !W->IsBusy() && W->HasUsableWeapon() && W->GetAmmo()<W->GetDefinition().Capacity)
        { Key(EKeys::R,true); FirePulses=1; NextFire=T+.16f; }
        else if (FirePulses==1 && T>NextFire)
        {
            Key(EKeys::R,false);
            if (W->IsMagazineReloadCommitted())
            { Key(EKeys::LeftMouseButton,true); FirePulses=2; NextFire=T+.08f; }
            else FirePulses=3;
        }
        else if (FirePulses==2 && T>NextFire)
        { Key(EKeys::LeftMouseButton,false); FirePulses=3; }
        if (T>=S.Seconds)
        {
            Check(!W->IsReloading(),TEXT("Real reload completed before the next machine action"));
            Check(W->GetTotalShotsFired()==ShotsAtStep,TEXT("Reload input segment and released invalid fire produce no delayed shot"));
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
        // existing arena and machine placement. Reload remains committed.
        Key(EKeys::D,T<2.f || (T>=6.f && T<8.f));
        Key(EKeys::A,T>=3.f && T<5.f);
        if (T>=S.Seconds)
        {
            const auto* D=W->GetDefinitionForWeapon(W->GetEquippedIndex());
            const int32 Committed=W->GetTotalShotsFired()-ShotsAtStep;
            Check(D && D->Family==ProfileFamily && D->bUpgraded==bProfileUpgraded,TEXT("Measured combat ends with the requested carried weapon"));
            const int32 MinimumDischarges=ProfileFamily==EONEWeaponFamily::Shotgun?10:60;
            Check(Committed>=MinimumDischarges,FString::Printf(TEXT("Measured requested-weapon combat committed at least %d actual discharges across fire and reloads"),MinimumDischarges));
            CSV_EVENT(ONECandidate05Presentation,TEXT("ONE05_UPGRADED_COMBAT_END phase=%d shots_committed=%d"),Phase,Committed);
            if (Failures) Finish(false); else Advance();
        }
        break;
    case EStep::ClickResume:
    {
        auto* HUD=Cast<AONEHUD>(Controller->GetHUD()); FBox2D Bounds;
        if (FirePulses==0 && HUD && HUD->GetActionBounds(EONEUIAction::Resume,Bounds))
        { const FVector2D P=Bounds.GetCenter(); Controller->SetMouseLocation(FMath::RoundToInt(P.X),FMath::RoundToInt(P.Y)); Key(EKeys::LeftMouseButton,true); FirePulses=1; NextFire=T+.15f; }
        else if (FirePulses==1 && T>=NextFire) { Key(EKeys::LeftMouseButton,false); FirePulses=2; }
        else if (FirePulses==2 && T>=NextFire+.2f)
        { Check(!Controller->IsPaused() && W->GetTotalShotsFired()==ShotsAtStep,TEXT("Drawn Resume mouse click resumes without firing")); Advance(); }
        break;
    }
    case EStep::Death:
        if (FirePulses==0) { Player->ReceiveAttack(10000.f,Player->GetActorLocation()+FVector(100,0,0)); FirePulses=1; }
        if (T>=S.Seconds) { Check(Mode->IsGameOver() && Player->IsDead(),TEXT("Real death lifecycle drives game-over UI")); Advance(); }
        break;
    }
    if (!bFinished && Phase==PreviousPhase && T>S.Seconds+2.f)
    { Check(false,TEXT("Current production-input step exceeded its bounded timeout: ")+S.Label); Finish(false); }
}
void AONE05PresentationCheck::ReplenishEnemies()
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
void AONE05PresentationCheck::Observe(float Dt)
{
    Live=0;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (!It->IsDead()) ++Live;
    if (bProfile && bCsvStarted && !bFinished)
    {
        ProfileSeconds+=Dt;
        ProfileMaximumLive=FMath::Max(ProfileMaximumLive,Live);
        ++ProfileSamples; if (Live==EnemyCount) ++ExactCountSamples;
        auto* PowerUps=Mode->GetPowerUps();
        const int32 WorldDrops=PowerUps?PowerUps->GetActiveDropCount():0;
        const float InstaSeconds=PowerUps?PowerUps->GetRemainingSeconds(EONEPowerUpType::InstaKill):0.f;
        const float DoubleSeconds=PowerUps?PowerUps->GetRemainingSeconds(EONEPowerUpType::DoublePoints):0.f;
        if (InstaSeconds>0.f&&DoubleSeconds>0.f) ++ProfilePowerUpSamples;
        if (WorldDrops>=3) ++ProfileThreeDropSamples;
        if (Box->GetState()==EONEMachineState::Active && Upgrade->GetState()==EONEMachineState::Active) BothActiveSeconds+=Dt;
        // Deliberate stress-fixture protection, never used by either recording mode.
        Player->GetHealthComponent()->Restore();
        if (Elapsed>=NextReplenish)
        {
            NextReplenish=Elapsed+.5f;
            if (Live<EnemyCount) ReplenishEnemies();
        }
        CSV_CUSTOM_STAT(ONECandidate05Presentation,Live,Live,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,RequestedLive,EnemyCount,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,WorldDrops,WorldDrops,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,InstaKillSeconds,InstaSeconds,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,DoublePointsSeconds,DoubleSeconds,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,PickupCollections,PowerUps?int32(PowerUps->GetDropStats().Collected):0,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,PickupExpiries,PowerUps?int32(PowerUps->GetDropStats().Expired):0,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,BoxState,int32(Box->GetState()),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,UpgradeState,int32(Upgrade->GetState()),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,Phase,Phase,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,ProfileSeconds,ProfileSeconds,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LiveMagazines,Player->GetWeaponComponent()->GetLiveMagazineCount(),ECsvCustomStatOp::Set);
        const auto* Effective=Player->GetWeaponComponent()->GetDefinitionForWeapon(Player->GetWeaponComponent()->GetEquippedIndex());
        const auto* ProfileWeaponComponent=Player->GetWeaponComponent();
        int32 LastContacts=0;
        for (const auto& Path:ProfileWeaponComponent->GetLastProjectilePaths()) LastContacts+=Path.Contacts.Num();
        CSV_CUSTOM_STAT(ONECandidate05Presentation,EffectiveFamily,Effective?int32(Effective->Family):-1,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LastShotContacts,LastContacts,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LastShotLiveHits,ProfileWeaponComponent->GetLastShotLiveHitCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LastShotCorpseHits,ProfileWeaponComponent->GetLastShotCorpseHitCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LastShotNewKills,ProfileWeaponComponent->GetLastShotNewKillCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LastShotProjectiles,ProfileWeaponComponent->GetLastProjectilePaths().Num(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,LastShotPelletContacts,ProfileWeaponComponent->GetLastShotContactPelletCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,ShellInserts,ProfileWeaponComponent->GetShellInsertCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,MagazineCommits,ProfileWeaponComponent->GetMagazineCommitCount(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,EffectiveUpgraded,Effective && Effective->bUpgraded?1:0,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,Shots,Player->GetWeaponComponent()->GetTotalShotsFired(),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,HeldAura,Player->IsHeldAuraVisible()?1:0,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate05Presentation,AmbientVoices,Mode->GetAmbientAudio()?Mode->GetAmbientAudio()->GetActiveVoiceCount():0,ECsvCustomStatOp::Set);
        int32 ZombieVoices=0;
        for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
            if (auto* Audio=It->FindComponentByClass<UONEZombieAudioComponent>()) ZombieVoices+=Audio->GetActiveVoiceCount();
        CSV_CUSTOM_STAT(ONECandidate05Presentation,ZombieVoices,ZombieVoices,ECsvCustomStatOp::Set);
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
    if (bProfile)
    {
        const auto* PowerUps=Mode->GetPowerUps();
        int32 Contacts=0;for (const auto& Path:W->GetLastProjectilePaths()) Contacts+=Path.Contacts.Num();
        ObservationCsv.RemoveAt(ObservationCsv.Len()-1);
        ObservationCsv+=FString::Printf(TEXT(",%d,%.6f,%.6f,%d,%d,%d,%d,%d\n"),
            PowerUps?PowerUps->GetActiveDropCount():0,PowerUps?PowerUps->GetRemainingSeconds(EONEPowerUpType::InstaKill):0.f,
            PowerUps?PowerUps->GetRemainingSeconds(EONEPowerUpType::DoublePoints):0.f,W->GetLastShotLiveHitCount(),
            W->GetLastShotCorpseHitCount(),Contacts,W->GetShellInsertCount(),W->GetMagazineCommitCount());
    }
}
void AONE05PresentationCheck::Capture()
{
    if (!bCapture || !bRecording || bFinished || bFinishRequested) return;
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1./30.)
    { LastCapture=Now; PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames); FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false); }
}
void AONE05PresentationCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
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
bool AONE05PresentationCheck::PrepareProfilePickups()
{
    auto* PowerUps=Mode->GetPowerUps();
    if (!PowerUps) { Check(false,TEXT("Profile pickup authority is present"));Finish(false);return false; }
    if (ProfilePickupSetupStage==0)
    {
        ProfilePickupSetupAt=FPlatformTime::Seconds();
        Check(PowerUps->ForceDrop(EONEPowerUpType::InstaKill,Player->GetActorLocation())==EONEPowerUpDropResult::Spawned,
            TEXT("Profile setup placed an actual Insta-Kill pickup at player overlap"));
        if (Failures) { Finish(false);return false; }
        ProfilePickupSetupStage=1;return false;
    }
    if (FPlatformTime::Seconds()-ProfilePickupSetupAt>5.0&&ProfilePickupSetupStage<4)
    { Check(false,TEXT("Profile timed pickups collected through real overlap within five seconds"));Finish(false);return false; }
    if (ProfilePickupSetupStage==1)
    {
        if (!PowerUps->IsActive(EONEPowerUpType::InstaKill)) return false;
        Check(PowerUps->ForceDrop(EONEPowerUpType::DoublePoints,Player->GetActorLocation())==EONEPowerUpDropResult::Spawned,
            TEXT("Profile setup placed an actual Double Points pickup at player overlap"));
        if (Failures) { Finish(false);return false; }
        ProfilePickupSetupStage=2;return false;
    }
    if (ProfilePickupSetupStage==2)
    {
        if (!PowerUps->IsActive(EONEPowerUpType::DoublePoints)) return false;
        const EONEPowerUpType Types[]={EONEPowerUpType::InstaKill,EONEPowerUpType::DoublePoints,EONEPowerUpType::MaxAmmo};
        for (int32 I=0;I<3;++I)
            Check(PowerUps->ForceDrop(Types[I],FVector(-220.f+220.f*I,120.f,98.f))==EONEPowerUpDropResult::Spawned,
                FString::Printf(TEXT("Profile setup placed visible world pickup type %d through the real authority"),int32(Types[I])));
        if (Failures) { Finish(false);return false; }
        ProfilePickupSetupStage=3;return false;
    }
    if (ProfilePickupSetupStage==3)
    {
        Check(PowerUps->IsActive(EONEPowerUpType::InstaKill)&&PowerUps->IsActive(EONEPowerUpType::DoublePoints)&&
            PowerUps->GetDropStats().Collected>=2&&PowerUps->GetActiveDropCount()>=3,
            TEXT("Profile begins with both overlap-collected timed effects and at least three actual world pickups"));
        if (Failures) { Finish(false);return false; }
        ProfilePickupSetupStage=4;
    }
    return true;
}
bool AONE05PresentationCheck::StartProfile()
{
#if CSV_PROFILER
    auto* P=FCsvProfiler::Get();
    if (!bCsvRequested)
    {
        if (UONE06CaptureComponent::IsAnyCaptureActive() || P->IsCapturing() || P->IsWritingFile() || P->IsEndCapturePending())
        { Check(false,TEXT("Scenario requires exclusive ownership of an idle CSV profiler")); Finish(false); return false; }
        if (!PrepareProfilePickups()) return false;
        for (const TCHAR* C:{TEXT("Chaos"),TEXT("PhysicsVerbose"),TEXT("PhysicsCounters"),TEXT("ONEPhysicality"),TEXT("ONECandidate05Presentation")})
            Check(P->EnableCategoryByString(C),FString(TEXT("Required CSV category enabled: "))+C);
        if (Failures) { Finish(false); return false; }
        CsvFolder=FPaths::ConvertRelativePathToFull(Folder/TEXT("CSV")); IFileManager::Get().MakeDirectory(*CsvFolder,true);
        CSV_METADATA(TEXT("one_scenario"),TEXT("candidate06_legacy05_two_machines_production_input_combat"));
        CSV_METADATA(TEXT("one_carried_weapon"),*ProfileWeapon);
        CSV_METADATA(TEXT("one_carried_family"),ProfileFamily==EONEWeaponFamily::Shotgun?TEXT("Shotgun"):TEXT("Carbine"));
        CSV_METADATA(TEXT("one_pickup_fixture"),TEXT("Two actual overlap collections plus three forced world drops before CSV; normal lifetime decay"));
        CSV_METADATA(TEXT("one_media_capture"),TEXT("none"));
        CSV_METADATA(TEXT("one_requested_enemies"),*FString::FromInt(EnemyCount));
        P->BeginCapture(-1,CsvFolder); bCsvRequested=true; CsvRequestAt=FPlatformTime::Seconds(); return false;
    }
    if (P->IsCapturing())
    {
        bCsvStarted=true; ReplenishEnemies(); NextReplenish=Elapsed+.5f;
        CSV_EVENT(ONECandidate05Presentation,TEXT("ONE05_PROFILE_BEGIN requested=%d"),EnemyCount);
        UE_LOG(LogTemp,Display,TEXT("ONE05_PROFILE_STARTED enemies=%d world_seconds=%.6f"),EnemyCount,Elapsed);
        return true;
    }
    if (FPlatformTime::Seconds()-CsvRequestAt>10.) { Check(false,TEXT("CSV did not start within ten seconds")); Finish(false); }
    return false;
#else
    Check(false,TEXT("This executable lacks CSV_PROFILER support")); Finish(false); return false;
#endif
}
bool AONE05PresentationCheck::FinishProfileWrite()
{
#if CSV_PROFILER
    auto* P=FCsvProfiler::Get();
    if (bCsvStarted && !bCsvFinished)
    {
        if (!CsvCompletion.IsValid() || !CsvCompletion.IsReady()) return false;
        const FString File=CsvCompletion.Get();
        Check(!File.IsEmpty() && IFileManager::Get().FileSize(*File)>0,TEXT("CSV writer finalized a nonempty complete output"));
        Report+=TEXT("\nEngine CSV output: ")+File+TEXT("\n");
        UE_LOG(LogTemp,Display,TEXT("ONE05_PROFILE_WRITTEN file=%s enemies=%d"),*File,EnemyCount);
        bCsvFinished=true; WriteResults();
    }
    if (P->IsCapturing() || P->IsWritingFile() || P->IsEndCapturePending()) return false;
#endif
    return true;
}
void AONE05PresentationCheck::Finish(bool Complete)
{
    if (bFinished) return;
    ReleaseKeys();
    if (bRecording && !PendingFrame.IsEmpty())
    { bFinishRequested=true; bRequestedComplete=Complete; return; }
    if (bRecording)
    {
        // Stop requesting frames, drain the final actual callback, then retain
        // a real mixer tail. No generated silence, time stretch or endpoint crop.
        if (!bAudioTailWaiting)
        { bFinishRequested=true; bRequestedComplete=Complete; bAudioTailWaiting=true; AudioTailUntil=FPlatformTime::Seconds()+.4; return; }
        if (FPlatformTime::Seconds()<AudioTailUntil) return;
    }
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
        Check(ProfilePowerUpSamples>0&&ProfileThreeDropSamples>0,TEXT("Measured frames include both timed effects and at least three world drops"));
#if CSV_PROFILER
        CSV_EVENT(ONECandidate05Presentation,TEXT("ONE05_PROFILE_END samples=%d overlap=%.6f"),ProfileSamples,BothActiveSeconds);
        CsvCompletion=FCsvProfiler::Get()->EndCapture();
        if (!CsvCompletion.IsValid()) { Check(false,TEXT("CSV end request was rejected")); bCsvFinished=true; }
#endif
    }
    if (Complete && !bManual && !bProfile) Check(CompletedConfigurations==6,TEXT("All six actual catalog configurations were acquired and shown"));
    if (Complete && !bManual && !bProfile) Check(Player->GetWeaponComponent()->GetMagazineDropCount()>=4,TEXT("At least four actual pistol/rifle old-magazine releases occurred in the recorded sequence"));
    if (!bProfile || !bCsvStarted || bCsvFinished) WriteResults();
}
void AONE05PresentationCheck::WriteResults()
{
    if (bResultsWritten) return; bResultsWritten=true;
    const bool DataWritten=FFileHelper::SaveStringToFile(FramesCsv,*(Folder/TEXT("frames.csv"))) &&
        FFileHelper::SaveStringToFile(InputCsv,*(Folder/TEXT("input_events.csv"))) &&
        FFileHelper::SaveStringToFile(ObservationCsv,*(Folder/TEXT("observations.csv"))) &&
        FFileHelper::SaveStringToFile(ChaptersCsv,*(Folder/TEXT("chapters.csv")));
    Check(DataWritten,TEXT("Scenario timestamp and input records were written"));
    Report+=FString::Printf(TEXT("\nComplete: %d\nChecks: %d\nFailures: %d\nFrames: %d\nProfile requested live: %d\nProfile actual maximum live: %d\nProfile actual frames: %d\nFrames at exact requested count: %d\nBoth active seconds: %.6f\nSuccessful registered spawns: %d\n"),bComplete,Checks,Failures,Frames,EnemyCount,ProfileMaximumLive,ProfileSamples,ExactCountSamples,BothActiveSeconds,Spawned);
    if (bProfile) Report+=FString::Printf(TEXT("Requested carried weapon: %s\nFrames with both timed power-ups active: %d\nFrames with at least three world drops: %d\nLast-shot contact fields are snapshots repeated until the next shot, not cumulative event totals.\n"),*ProfileWeapon,ProfilePowerUpSamples,ProfileThreeDropSamples);
    if (!FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt"))))
        Check(false,TEXT("Scenario report could not be written"));
    UE_LOG(LogTemp,Display,TEXT("ONE05_PRESENTATION_COMPLETE complete=%d failures=%d checks=%d frames=%d profile=%d"),bComplete,Failures,Checks,Frames,bProfile);
}
void AONE05PresentationCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished)
    {
        if (bProfile && !FinishProfileWrite()) return;
        if (!bManual && FPlatformTime::Seconds()-FinishedAt>2.) FPlatformMisc::RequestExit(false);
        return;
    }
    const double Now=FPlatformTime::Seconds();
    Elapsed+=Controller && Controller->IsPaused() ? float(FMath::Clamp(Now-LastDriverTime,0.,.25)) : Dt;
    LastDriverTime=Now;
    if (bRecording && !PendingFrame.IsEmpty() && Now-LastCapture>5.)
    {
        PendingFrame.Empty();
        Check(false,TEXT("Requested screenshot callback did not complete within five seconds"));
        Finish(false); return;
    }
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
    if (Player->IsDead() && (!Steps.IsValidIndex(Phase) || Steps[Phase].Kind!=EStep::Death)) { Check(false,TEXT("Player died before scripted sequence completed")); Finish(false); return; }
    Observe(Dt);
    { CSV_SCOPED_TIMING_STAT(ONECandidate05Presentation,PresentationDriver); RunStep(Dt); }
    Capture();
    if (Elapsed>355.f && !bFinished) { Check(false,TEXT("Scenario exceeded its total bounded runtime")); Finish(false); }
}
void AONE05PresentationCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    // An interrupted level cannot promise delivery of its pending screenshot.
    // Preserve completed rows and mark the run incomplete instead of blocking.
    PendingFrame.Empty();
    if (!bFinished)
    {
        // No future actor tick exists after EndPlay. Export completed media and
        // rows immediately and label interruption rather than waiting for a tail.
        bAudioTailWaiting=true; AudioTailUntil=0;
        Check(false,TEXT("Level transition or process exit interrupted the recording/scenario")); Finish(false);
        if (!bResultsWritten) WriteResults();
    }
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this);
    Super::EndPlay(Reason);
}
