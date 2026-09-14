#include "ONE07ProfileCheck.h"
#include "ONE05PresentationCheck.h"
#include "ONE06CaptureComponent.h"
#include "ONE05Audio.h"
#include "ONEAmbientAudioComponent.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
#include "ONEPowerUpComponent.h"
#include "ONEZombie.h"
#include "ONEZombieAudioComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "ProfilingDebugging/CsvProfiler.h"

CSV_DEFINE_CATEGORY(ONECandidate07Profile,true);

AONE07ProfileCheck::AONE07ProfileCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE07ProfileCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; Failures+=Pass?0:1;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE07_PROFILE %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE07ProfileCheck::BeginPlay()
{
    Super::BeginPlay(); StartedAt=GetWorld()->GetTimeSeconds();
    Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir())/TEXT("Candidate07/Profile")/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    IFileManager::Get().MakeDirectory(*Folder,true);
    FParse::Value(FCommandLine::Get(),TEXT("ONE05ProfileEnemies="),Requested);
    FParse::Value(FCommandLine::Get(),TEXT("ONE05ProfileWeapon="),Weapon);
    FParse::Value(FCommandLine::Get(),TEXT("ONE07ProfileFalls="),FallMode);
    Report=TEXT("Candidate07 recording-free physicality census paired with the existing ONE05/Candidate06 profile workload.\n")
        TEXT("Ordinary machine acquisition, overlapping machine activity, weapon input/reload, registered replenishment, per-frame player health restore and five pre-profile pickup placements are retained from that disclosed fixture. This is not human play or an ordinary survival encounter.\n")
        TEXT("The companion never changes health, inventory, targeting, population or machine states. In mixed mode it requests at most two living falls at each of .5, 12.5 and 24.5 measured seconds through TryLivingFall; these are explicit interventions, not natural contact results. None mode adds only observation.\n")
        TEXT("Every tick after the driver and player are found is retained through the last capturing tick, including initialized setup. csv_active labels the companion's observed engine CSV window; no measured census frame, spike, shortfall or state is filtered. Raw engine CSV remains authoritative for CPU/GPU/frame timings and must be retained in full, including any unmatched boundary row. Companion census overhead is included.\n")
        TEXT("Counts are current-world snapshots, not cumulative totals: living = standing + fallen + get-up; standing includes pursuit, attack, hit and stumble. Simulated bodies count infected leader-mesh rigid-body instances, including sleeping bodies; awake is separate. Detached debris and the rest of the world are outside that body census; engine physics counters remain available. Voice counts report component playback state, not audited mixer audibility. Actor contact/fall/recovery totals may decrease when actors retire.\n");
    FallEvents=TEXT("profile_seconds,episode,actor_id,accepted,health_before,health_after,fall_count\n");
    Check(Requested==1 || Requested==2 || Requested==6 || Requested==12 || Requested==18,TEXT("Requested population is one of 1, 2, 6, 12, 18"));
    Check(Weapon==TEXT("M4A1") || Weapon==TEXT("Overcurrent") || Weapon==TEXT("870"),TEXT("Requested carried weapon is M4A1, Overcurrent or 870"));
    Check(FallMode==TEXT("mixed") || FallMode==TEXT("none"),TEXT("Explicit fall workload is mixed or none"));
    bool CaptureRequested=false;
    for (const TCHAR* Flag:{TEXT("ONE07Capture"),TEXT("ONE06Capture"),TEXT("ONE05PresentationCapture"),TEXT("ONE05MotionCapture"),
        TEXT("ONE04PresentationCapture"),TEXT("ONE03MovementCapture"),TEXT("ONE03PhysicalityCapture"),TEXT("ONE03PresentationCapture")})
        CaptureRequested|=FParse::Param(FCommandLine::Get(),Flag);
    CaptureRequested|=FString(FCommandLine::Get()).Contains(TEXT("ManualCapture="));
    Check(!CaptureRequested && !UONE06CaptureComponent::IsAnyCaptureActive(),TEXT("No viewport/audio recording is requested or active"));
#if CSV_PROFILER
    Check(FCsvProfiler::Get()->EnableCategoryByString(TEXT("ONECandidate07Profile")),TEXT("Physicality census CSV category is enabled"));
    CSV_METADATA(TEXT("one_c07_companion"),TEXT("all_frames_physicality_census"));
    CSV_METADATA(TEXT("one_c07_explicit_falls"),*FallMode);
#else
    Check(false,TEXT("CSV profiler support is required"));
#endif
    bValid=Failures==0; Frames.Reserve(24000);
    if (!bValid) { WriteResults(false); FPlatformMisc::RequestExit(false); }
}
void AONE07ProfileCheck::RequestFalls(float ProfileSeconds)
{
    if (FallMode!=TEXT("mixed") || Episode>=3 || ProfileSeconds<.5f+12.f*Episode) return;
    ++Episode;
    TArray<AONEZombie*> Eligible;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
        if (!It->IsDead() && !It->IsLivingFallen() && !It->IsGettingUp()) Eligible.Add(*It);
    Eligible.Sort([](const AONEZombie& A,const AONEZombie& B){return A.GetUniqueID()<B.GetUniqueID();});
    int32 Attempted=0;
    for (AONEZombie* Z:Eligible)
    {
        if (Attempted>=FMath::Min(2,Requested)) break;
        ++Attempted; ++FallAttempts;
        const float Before=Z->GetHealth();
        const bool Accepted=Z->TryLivingFall(FVector(220,30,15),TEXT("explicit_profile_fixture"));
        AcceptedFalls+=Accepted?1:0;
        FallEvents+=FString::Printf(TEXT("%.6f,%d,%u,%d,%.3f,%.3f,%d\n"),ProfileSeconds,Episode,Z->GetUniqueID(),Accepted,Before,Z->GetHealth(),Z->GetLivingFallCount());
        CSV_EVENT(ONECandidate07Profile,TEXT("EXPLICIT_FALL episode=%d id=%u accepted=%d"),Episode,Z->GetUniqueID(),Accepted);
    }
    if (!Attempted) FallEvents+=FString::Printf(TEXT("%.6f,%d,0,0,0,0,0\n"),ProfileSeconds,Episode);
}
void AONE07ProfileCheck::Tick(float Dt)
{
    Super::Tick(Dt); if (!bValid || bWritten) return;
    CSV_SCOPED_TIMING_STAT(ONECandidate07Profile,CensusDriver);
    if (!Driver)
        for (TActorIterator<AONE05PresentationCheck> It(GetWorld());It;++It)
        { Driver=*It; AddTickPrerequisiteActor(Driver); break; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Driver || !Player)
    {
        if (GetWorld()->GetTimeSeconds()-StartedAt>10.)
        { Check(false,TEXT("Existing profile driver and ordinary player became available")); WriteResults(false); FPlatformMisc::RequestExit(false); }
        return;
    }
    bool Capturing=false;
#if CSV_PROFILER
    Capturing=FCsvProfiler::Get()->IsCapturing();
#endif
    if (Capturing && !bSeenCsv)
    { bSeenCsv=true; CsvStartedAt=GetWorld()->GetTimeSeconds(); LastShots=Player->GetWeaponComponent()->GetTotalShotsFired(); }
    if (!Capturing && bSeenCsv) { WriteResults(true); return; }
    FFrame Row; Row.Frame=GFrameCounter; Row.WorldSeconds=GetWorld()->GetTimeSeconds(); Row.DeltaSeconds=Dt;
    Row.CsvActive=Capturing?1:0; Row.ProfileSeconds=Capturing?float(Row.WorldSeconds-CsvStartedAt):-1.f;
    const FString Label=Driver->GetSegmentLabel(); Row.Phase=PhaseLabels.IndexOfByKey(Label);
    if (Row.Phase==INDEX_NONE) Row.Phase=PhaseLabels.Add(Label);
    if (Capturing) RequestFalls(Row.ProfileSeconds);
    auto& C=Row.Counts;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
    {
        const bool Dead=It->IsDead();
        if (Dead) ++C.Dead;
        else { ++C.Live; if (It->IsLivingFallen()) ++C.Fallen; else if (It->IsGettingUp()) ++C.GetUp; else ++C.Standing; }
        const int32 Bodies=It->GetActivePhysicsBodyCount(); C.SimulatedBodies+=Bodies; C.AwakeBodies+=It->GetAwakePhysicsBodyCount();
        if (Dead) C.DeadBodies+=Bodies; else C.LivingBodies+=Bodies;
        C.ContactEvents+=It->GetPhysicalContactCount(); C.Falls+=It->GetLivingFallCount(); C.Recoveries+=It->GetRecoveryCount(); C.Blocked+=It->GetRecoveryBlockedCount();
        if (It->ZombieAudio) C.ZombieVoices+=It->ZombieAudio->GetActiveVoiceCount();
    }
    auto* Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (Mode && Mode->GetAmbientAudio()) C.AmbientVoices=Mode->GetAmbientAudio()->GetActiveVoiceCount();
    if (const auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>()) C.ContactVoices=Audio->GetActiveContactVoiceCount();
    if (Mode && Mode->GetPowerUps())
    {
        const auto* Power=Mode->GetPowerUps(); C.WorldDrops=Power->GetActiveDropCount();
        C.InstaSeconds=Power->GetRemainingSeconds(EONEPowerUpType::InstaKill); C.DoubleSeconds=Power->GetRemainingSeconds(EONEPowerUpType::DoublePoints);
    }
    const auto* W=Player->GetWeaponComponent(); C.Shots=W->GetTotalShotsFired(); C.Ammo=W->GetAmmo(); C.Reserve=W->GetReserveAmmo(); C.Operation=int32(W->GetOperation());
    const auto* Definition=W->GetDefinitionForWeapon(W->GetEquippedIndex());
    if (Definition) { C.Family=int32(Definition->Family); C.Upgraded=Definition->bUpgraded?1:0; }
    if (auto* Controller=Cast<APlayerController>(Player->GetController())) Controller->GetViewportSize(Row.Width,Row.Height);
    const auto Read=[](const TCHAR* Name){const auto* V=IConsoleManager::Get().FindConsoleVariable(Name); return V?V->GetFloat():-1.f;};
    Row.Cap=Read(TEXT("t.MaxFPS")); Row.VSync=int32(Read(TEXT("r.VSync"))); Row.ScreenPercentage=Read(TEXT("r.ScreenPercentage"));
    if (Capturing)
    {
        ++CsvRows; ExactRows+=C.Live==Requested?1:0; FallenRows+=C.Fallen>0?1:0; GetUpRows+=C.GetUp>0?1:0; DeadRows+=C.Dead>0?1:0;
        SettingsMismatchRows+=(Row.Width!=1600 || Row.Height!=900 || !FMath::IsNearlyEqual(Row.Cap,120.f) || Row.VSync!=0 || !FMath::IsNearlyEqual(Row.ScreenPercentage,100.f))?1:0;
        if (Definition && Definition->Family==(Weapon==TEXT("870")?EONEWeaponFamily::Shotgun:EONEWeaponFamily::Carbine) && Definition->bUpgraded==(Weapon==TEXT("Overcurrent")))
            RequestedWeaponShots+=FMath::Max(0,C.Shots-LastShots);
        LastShots=C.Shots;
        CSV_CUSTOM_STAT(ONECandidate07Profile,Frame,int32(Row.Frame),ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,RequestedLive,Requested,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,Live,C.Live,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,Standing,C.Standing,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,Fallen,C.Fallen,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,GetUp,C.GetUp,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,Dead,C.Dead,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,SimulatedBodies,C.SimulatedBodies,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,AwakeBodies,C.AwakeBodies,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,ZombieVoices,C.ZombieVoices,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,AmbientVoices,C.AmbientVoices,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,ContactVoices,C.ContactVoices,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,CurrentActorContacts,C.ContactEvents,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,CurrentActorFalls,C.Falls,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,CurrentActorRecoveries,C.Recoveries,ECsvCustomStatOp::Set);
        CSV_CUSTOM_STAT(ONECandidate07Profile,CurrentActorBlocked,C.Blocked,ECsvCustomStatOp::Set);
    }
    Frames.Add(Row);
    if (UONE06CaptureComponent::IsAnyCaptureActive())
    { Check(false,TEXT("No media recorder became active during profile")); WriteResults(false); FPlatformMisc::RequestExit(false); }
    if (Row.WorldSeconds-StartedAt>220.)
    { Check(false,TEXT("Profile companion finished within 220 gameplay seconds")); WriteResults(false); FPlatformMisc::RequestExit(false); }
}
void AONE07ProfileCheck::WriteResults(bool Complete)
{
    if (bWritten) return; bWritten=true;
    Check(Complete && bSeenCsv && CsvRows>0,TEXT("Observed a complete nonempty engine CSV capture window"));
    Check(SettingsMismatchRows==0 && CsvRows>0,TEXT("Every measured census frame used actual 1600x900, cap120, VSync0 and screen percentage100"));
    Check(ExactRows>0,TEXT("Requested live population was actually observed"));
    Check(RequestedWeaponShots>0,TEXT("Requested carried weapon actually discharged during the measured window"));
    if (FallMode==TEXT("mixed")) Check(Episode==3 && AcceptedFalls>0 && FallenRows>0,TEXT("Three disclosed fall episodes were attempted and actual fallen frames observed"));
    FString Csv=TEXT("frame,world_seconds,delta_seconds,csv_active,profile_seconds,phase,width,height,cap,vsync,screen_percentage,requested_live,live,standing,fallen,get_up,dead,simulated_bodies,awake_bodies,living_bodies,dead_bodies,zombie_voices,ambient_voices,contact_voices,current_actor_contact_events,current_actor_falls,current_actor_recoveries,current_actor_blocked,shots,family,upgraded,world_drops,insta_seconds,double_seconds,ammo,reserve,operation\n");
    for (const auto& R:Frames)
    {
        const auto& C=R.Counts;
        Csv+=FString::Printf(TEXT("%llu,%.9f,%.9f,%d,%.9f,%d,%d,%d,%.3f,%d,%.3f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%d,%d,%d\n"),
            static_cast<unsigned long long>(R.Frame),R.WorldSeconds,R.DeltaSeconds,R.CsvActive,R.ProfileSeconds,R.Phase,R.Width,R.Height,R.Cap,R.VSync,R.ScreenPercentage,
            Requested,C.Live,C.Standing,C.Fallen,C.GetUp,C.Dead,C.SimulatedBodies,C.AwakeBodies,C.LivingBodies,C.DeadBodies,C.ZombieVoices,C.AmbientVoices,C.ContactVoices,
            C.ContactEvents,C.Falls,C.Recoveries,C.Blocked,C.Shots,C.Family,C.Upgraded,C.WorldDrops,C.InstaSeconds,C.DoubleSeconds,C.Ammo,C.Reserve,C.Operation);
    }
    const bool Saved=FFileHelper::SaveStringToFile(Csv,*(Folder/TEXT("timeline.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM) &&
        FFileHelper::SaveStringToFile(FallEvents,*(Folder/TEXT("fall_events.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    Check(Saved,TEXT("Every retained frame and explicit fall request was written"));
    TSharedRef<FJsonObject> Json=MakeShared<FJsonObject>();
    Json->SetStringField(TEXT("schema"),TEXT("one07.profile_companion.v1")); Json->SetBoolField(TEXT("complete"),Complete);
    Json->SetNumberField(TEXT("checks"),Checks); Json->SetNumberField(TEXT("failures"),Failures);
    Json->SetNumberField(TEXT("requested_live"),Requested); Json->SetStringField(TEXT("weapon"),Weapon); Json->SetStringField(TEXT("fall_mode"),FallMode);
    Json->SetNumberField(TEXT("retained_rows"),Frames.Num()); Json->SetNumberField(TEXT("csv_active_rows"),CsvRows); Json->SetNumberField(TEXT("exact_population_rows"),ExactRows);
    Json->SetNumberField(TEXT("fallen_rows"),FallenRows); Json->SetNumberField(TEXT("get_up_rows"),GetUpRows); Json->SetNumberField(TEXT("dead_rows"),DeadRows);
    Json->SetNumberField(TEXT("requested_weapon_discharges"),RequestedWeaponShots); Json->SetNumberField(TEXT("fall_attempts"),FallAttempts); Json->SetNumberField(TEXT("accepted_falls"),AcceptedFalls);
    TArray<TSharedPtr<FJsonValue>> Labels; for (const FString& Label:PhaseLabels) Labels.Add(MakeShared<FJsonValueString>(Label)); Json->SetArrayField(TEXT("phase_labels"),Labels);
    Json->SetStringField(TEXT("scope"),TEXT("Companion census completion only; require the paired driver report and complete raw engine CSV. No timing exclusions, media recording, native input, visual review or audio audition."));
    FString Text; FJsonSerializer::Serialize(Json,TJsonWriterFactory<>::Create(&Text));
    const bool JsonSaved=FFileHelper::SaveStringToFile(Text,*(Folder/TEXT("observations.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    if (!JsonSaved) Check(false,TEXT("Companion JSON report saved"));
    if (!FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
        Check(false,TEXT("Companion assertion report saved"));
    UE_LOG(LogTemp,Display,TEXT("ONE07_PROFILE_COMPLETE complete=%d failures=%d checks=%d rows=%d csv_rows=%d exact_rows=%d accepted_falls=%d"),
        Complete,Failures,Checks,Frames.Num(),CsvRows,ExactRows,AcceptedFalls);
}
void AONE07ProfileCheck::EndPlay(const EEndPlayReason::Type Reason)
{ if (!bWritten) WriteResults(false); Super::EndPlay(Reason); }
