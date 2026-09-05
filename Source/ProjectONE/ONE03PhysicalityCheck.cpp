#include "ONE03PhysicalityCheck.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEBloodSubsystem.h"
#include "ONEGameMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "NavMesh/RecastNavMesh.h"
#include "EngineUtils.h"
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
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(ONEPhysicality,true);

AONE03PhysicalityCheck::AONE03PhysicalityCheck()
{ PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void AONE03PhysicalityCheck::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(),TEXT("ONE03PhysicalityStartPhase="),FirstPhase);
    FirstPhase=FMath::Clamp(FirstPhase,0,10);
    bProfile=FParse::Param(FCommandLine::Get(),TEXT("ONE03PhysicalityProfile"));
    bCapture=!bProfile && FParse::Param(FCommandLine::Get(),TEXT("ONE03PhysicalityCapture"));
    // Extra pose reads/formatting are diagnostic work, never part of the cost profile.
    bRestTelemetry=!bProfile && FParse::Param(FCommandLine::Get(),TEXT("ONE03RestTelemetry"));
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03")/(bProfile?TEXT("PhysicalityProfile"):bCapture?TEXT("PhysicalityCapture"):TEXT("Physicality"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate03 Stage D actual physics, wounds and crowd integration\nControlled registered sandbox fixtures; region packets are explicit test inputs, not claimed player aiming. Mixed crowd phase also uses the production player weapon. Camera/audio capture is opt-in and excluded from performance runs.\n\n");
    FrameReport=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation\n");
    Telemetry=TEXT("seconds,phase,live,corpses,parts,bodies,awake,wounds,drops,decals,pools,remaining_blood,deposited_blood,largest_pool_cm,projection_traces,max_body_speed_cm_s,min_live_pair_cm,max_capsule_overlap_cm,player_health\n");
    const UPhysicsSettings* Physics=UPhysicsSettings::Get();
    const FString PhysicsDescription=FString::Printf(TEXT("Physics settings: substepping=%d, max_substep_seconds=%.9f, max_substeps=%d, max_physics_delta_seconds=%.9f. Optional corpse motion telemetry=%d.\n"),
        Physics->bSubstepping,Physics->MaxSubstepDeltaTime,Physics->MaxSubsteps,Physics->MaxPhysicsDeltaTime,bRestTelemetry);
    Report+=PhysicsDescription;
    UE_LOG(LogTemp,Display,TEXT("ONE03_PHYSICS_SETTINGS %s"),*PhysicsDescription);
    if (bRestTelemetry)
    {
        CorpseMotionTelemetry=TEXT("seconds,phase,corpse,body_count,awake_count,max_linear_cm_s,max_angular_rad_s,worst_linear_bone,worst_angular_bone,sample_delta_seconds,max_pose_step_cm,max_pose_step_degrees,continuous_low_motion_seconds,finite,manual_sleep_events,explicit_wake_calls,contact_wake_events,guard_stable_seconds,frozen,freeze_events,resume_events,frozen_body_count,freeze_error_cm,freeze_error_degrees,resume_error_cm,resume_error_degrees\n");
        Report+=TEXT("corpse_motion.csv is an opt-in post-physics observation at the existing approximately 10Hz sample cadence. Low motion means all active bodies <=5cm/s and <=0.12rad/s, plus <=2cm/2degrees since the previous sample; a missing/changed body set or gap over0.25s resets the measured duration. First-sample pose steps are -1 (unavailable). This is not a support/contact test or a command to sleep/freeze.\n");
        Report+=TEXT("Production rest guard is separate: minimum2s since disturbance, all bodies <=5cm/s and <=0.35rad/s, each bone within1cm/2degrees of a fixed measured window pose for1.25s, then confirmed static support within3cm. Qualified poses become kinematic with collision retained; ordinary contact does not resume them. Fresh accepted corpse damage/sever resumes the current pose before applying its impulse. Natural sleeping bodies stay simulated. Freeze/resume counts and transform errors are separate; BIG_NUMBER means that transition has not occurred. See ONE_REST_SUPPORTED_FREEZE and ONE_REST_RESUME log entries.\n");
    }
    if (bCapture)
    {
        UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE03PhysicalityCheck::Screenshot);
        if (auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
            if (auto Master=Mixer->GetMasterSubmix().Pin()) Mixer->AudioRenderThreadCommand([Master](){Master->SetAutoDisable(false);});
    }
}
void AONE03PhysicalityCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; Failures+=Pass?0:1;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE03_PHYSICALITY %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE03PhysicalityCheck::Capture()
{
    if (!bCapture || !bRecording || bFinished) return;
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1.0/30.0)
    { LastCapture=Now; PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames); FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false); }
}
void AONE03PhysicalityCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty() || !Player) return;
    const double CapturedAt=FPlatformTime::Seconds()-AudioStart;
    const auto* W=Player->GetWeaponComponent();
    const FString Row=FString::Printf(TEXT("%s,%.6f,%.6f,%d,%d,%d,%d,%d\n"),*PendingFrame,CapturedAt,Elapsed,Phase,W->GetEquippedIndex(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()));
    if (FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),Width,Height),85)) { FrameReport+=Row; ++Frames; }
    PendingFrame.Empty();
}
void AONE03PhysicalityCheck::Finish()
{
    if (bFinished) return;
    bFinished=true; FinishedAt=FPlatformTime::Seconds(); Player->ReleaseHeldInputs();
    if (bRecording)
    { UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("gameplay_master"),FPaths::ConvertRelativePathToFull(Folder)); bRecording=false; }
    if (bProfile && bCsvStarted)
    {
#if CSV_PROFILER
        CSV_EVENT(ONEPhysicality,TEXT("ONE03_PHYSICALITY_PROFILE_END phases=%d"),Phase);
        CsvCompletion=FCsvProfiler::Get()->EndCapture();
        if (!CsvCompletion.IsValid())
        { Check(false,TEXT("Profiler did not accept the end request for this scenario")); bCsvFinished=true; }
#endif
    }
    if (!bProfile || !bCsvStarted || bCsvFinished) WriteResults();
}
void AONE03PhysicalityCheck::WriteResults()
{
    if (bResultsWritten) return;
    bResultsWritten=true;
    Report+=FString::Printf(TEXT("\nChecks: %d\nFailures: %d\nCaptured frames: %d\n"),Checks,Failures,Frames);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    FFileHelper::SaveStringToFile(Telemetry,*(Folder/TEXT("observations.csv")));
    FFileHelper::SaveStringToFile(FrameReport,*(Folder/TEXT("frames.csv")));
    if (bRestTelemetry) FFileHelper::SaveStringToFile(CorpseMotionTelemetry,*(Folder/TEXT("corpse_motion.csv")));
    UE_LOG(LogTemp,Display,TEXT("ONE03_PHYSICALITY_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
bool AONE03PhysicalityCheck::StartProfile()
{
#if CSV_PROFILER
    auto* Profiler=FCsvProfiler::Get();
    if (!bCsvStartRequested)
    {
        if (Profiler->IsCapturing() || Profiler->IsWritingFile() || Profiler->IsEndCapturePending())
        { Check(false,TEXT("Profile scenario requires ownership of an idle CSV profiler; omit startup frame-limited captures")); Finish(); return false; }
        bool CategoriesReady=true;
        for (const TCHAR* Category:{TEXT("Chaos"),TEXT("PhysicsVerbose"),TEXT("PhysicsCounters"),TEXT("ONEPhysicality")})
        {
            const bool Enabled=Profiler->EnableCategoryByString(Category); CategoriesReady&=Enabled;
            Check(Enabled,FString::Printf(TEXT("Required CSV category available: %s"),Category));
        }
        if (!CategoriesReady) { Finish(); return false; }
        // Unique run directory preserves previous captures; leaving filename
        // empty lets UE choose the correct .csv/.csv.gz extension for its mode.
        CsvFolder=FPaths::ConvertRelativePathToFull(Folder/TEXT("CSV")/(FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
        IFileManager::Get().MakeDirectory(*CsvFolder,true);
        CSV_METADATA(TEXT("one_scenario"),TEXT("candidate03_physicality_all_11_phases"));
        CSV_METADATA(TEXT("one_media_capture"),TEXT("none"));
        Profiler->BeginCapture(-1,CsvFolder);
        bCsvStartRequested=true; CsvStartRequestedAt=FPlatformTime::Seconds();
        UE_LOG(LogTemp,Display,TEXT("ONE03_PROFILE_START_REQUEST folder=%s"),*CsvFolder);
        return false;
    }
    if (Profiler->IsCapturing())
    {
        bCsvStarted=true;
        CSV_EVENT(ONEPhysicality,TEXT("ONE03_PHYSICALITY_PROFILE_BEGIN after_warmup_seconds=%.3f"),Elapsed);
        return true;
    }
    if (FPlatformTime::Seconds()-CsvStartRequestedAt>10.)
    { Check(false,TEXT("CSV profiler did not start within ten seconds; no phases were run")); Finish(); }
    return false;
#else
    Check(false,TEXT("This executable was built without CSV_PROFILER support")); Finish(); return false;
#endif
}
bool AONE03PhysicalityCheck::FinishProfileWrite()
{
#if CSV_PROFILER
    auto* Profiler=FCsvProfiler::Get();
    if (bCsvStarted && !bCsvFinished)
    {
        if (!CsvCompletion.IsValid() || !CsvCompletion.IsReady()) return false;
        // EndCapture's future is fulfilled after FinalizeCsvFile and the writer
        // finish, not merely when the stop command has been enqueued.
        const FString File=CsvCompletion.Get();
        const bool Complete=!File.IsEmpty() && IFileManager::Get().FileSize(*File)>0;
        Check(Complete,TEXT("Engine CSV writer completed and produced a nonempty profiling file"));
        Check(Phase>=11,TEXT("CSV scenario reached the end of all eleven physicality phases"));
        Report+=TEXT("\nCompleted engine CSV: ")+File+TEXT("\nNo JPEG requests or audio recording were enabled in profile mode.\n");
        UE_LOG(LogTemp,Display,TEXT("ONE03_PROFILE_WRITTEN file=%s"),*File);
        bCsvFinished=true; WriteResults();
    }
    // Keep ticking while the writer or render-thread stop is outstanding. Never
    // trade a complete capture for a fixed exit delay that truncates its footer.
    if (Profiler->IsCapturing() || Profiler->IsWritingFile() || Profiler->IsEndCapturePending()) return false;
#endif
    return true;
}
void AONE03PhysicalityCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    if (!bFinished && Player) { Check(false,TEXT("Physicality scenario ended before completion")); Finish(); }
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this); Super::EndPlay(Reason);
}
void AONE03PhysicalityCheck::Damage(AONEZombie* Z,EONEHitRegion Region,float Amount,float Trauma,const FVector& Direction)
{
    if (!IsValid(Z)) return;
    static const FName Bones[]={TEXT("spine_02"),TEXT("head"),TEXT("upperarm_r"),TEXT("upperarm_l"),TEXT("thigh_r"),TEXT("thigh_l")};
    FONEWeaponDamagePacket Packet; Packet.ShotId=++FixtureShot;
    const FName Bone=Bones[int32(Region)];
    Packet.Get(Region).AddPellet(Amount,Trauma,Z->GetMesh()->GetSocketLocation(Bone),Direction,-Direction,Bone); Packet.Finalize();
    const bool WasDead=Z->IsDead(); Z->ReceiveWeaponDamage(Packet);
    if (!WasDead && Z->IsDead())
    {
        Check(Z->IsRagdollActive() && Z->GetActivePhysicsBodyCount()>0,TEXT("Death activates real skeletal physics bodies"));
        Check(Z->GetRagdollTransitionErrorCm()<.5f && Z->GetRagdollTransitionAngleDegrees()<.5f,
            FString::Printf(TEXT("Physics starts at evaluated animated bones: %.4fcm / %.4fdeg"),Z->GetRagdollTransitionErrorCm(),Z->GetRagdollTransitionAngleDegrees()));
    }
}
AONEZombie* AONE03PhysicalityCheck::Spawn(const FVector& Location)
{
    AONEZombie* Z=GetWorld()->GetAuthGameMode<AONEGameMode>()->SpawnSandboxEnemyAt(Location);
    if (Z) { Subjects.Add(Z); StartPositions.Add(Z->GetActorLocation()); SubjectProgress.Add(0); ++Spawned; }
    return Z;
}
void AONE03PhysicalityCheck::ClearFixture()
{
    Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately();
    // Each fixture retires registered live enemies through the same single-death
    // transaction, then invokes the production cleanup for bodies and blood.
    TArray<AONEZombie*> Retire; for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (IsValid(*It)) Retire.Add(*It);
    for (AONEZombie* Z:Retire) if (!Z->IsDead()) Damage(Z,EONEHitRegion::Body,1000,0,FVector(1,0,0));
    GetWorld()->GetAuthGameMode<AONEGameMode>()->ClearSandboxPresentation(); Subjects.Reset(); StartPositions.Reset(); SubjectProgress.Reset();
}
void AONE03PhysicalityCheck::PreparePhase()
{
    ClearFixture(); ++Phase;
    if (Phase>=11) { Finish(); return; }
    const TCHAR* Labels[]={TEXT("MINOR WOUND / MOVING TRAIL"),TEXT("SEVERE TORSO / COLLAPSE / GROWING POOL"),TEXT("ANATOMICAL LEFT ARM / REMAINING ARM"),TEXT("ANATOMICAL RIGHT ARM / REMAINING ARM"),TEXT("HEAD AND LEFT LEG / PHYSICAL PARTS"),TEXT("SIX MOVING DEATHS / CORPSE CONTACT"),TEXT("CORNER CROWD / PURSUIT"),TEXT("RACK SIDE PASSAGE / CROWD CONTACT"),TEXT("BENCH END / CROWD CONTACT"),TEXT("SUSTAINED MIXED CROWD / LIVE WEAPON"),TEXT("CLEANUP / NO REAPPEARING BLOOD")};
    Segment=Labels[Phase]; PhaseStart=Elapsed; ActionIndex=0; NextAction=.8f; NextSample=0;
    if (bProfile) CSV_EVENT(ONEPhysicality,TEXT("ONE03_PHASE_BEGIN index=%d label=%s"),Phase,*Segment);
    Spawned=0; AttackFrames=0; MaxBodies=MaxAwake=MaxWounds=MaxDrops=MaxDecals=MaxCorpses=MaxParts=0;
    MaximumVelocity=MaximumOverlap=MaxProgress=MaxPoolRadius=StartPoolRadius=MaxVolume=CoexistenceSeconds=0;
    bInvalidPhysicsSample=false; bFrozenWakeTested=false;
    ClosestPair=BIG_NUMBER; MinPlayerHealth=100; bPoolBaseline=false; FinalPelvisRotations.Reset();
    Player->Health->Restore(); Player->SetActorLocation(FVector(-220,360,98));
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(1000,0,0));
    Player->GetWeaponComponent()->RefillAllAmmo(); Player->GetWeaponComponent()->SelectWeapon(Phase%2);
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>(); GM->SetSandboxDimLighting(false);
    auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>(); CleanupGeneration=Blood->GetGeneration(); InitialVolume=Blood->GetDepositedBloodVolume();
    // Keep the initial minor-wound subject inside the ordinary camera frame,
    // including its early moving leakage, without changing pursuit behavior.
    if (Phase<=3) Spawn(FVector(Phase==0?360:160,360,98));
    else if (Phase==4)
    { Spawn(FVector(180,290,98)); Spawn(FVector(210,440,98)); }
    else if (Phase==5)
    {
        Player->SetActorLocation(FVector(-40,360,98));
        for (int32 I=0;I<6;++I) Spawn(FVector(240+(I/3)*100,270+(I%3)*95,98));
    }
    else if (Phase>=6 && Phase<=8)
    {
        const FVector Target=Phase==6 ? FVector(-1140,920,98) : Phase==7 ? FVector(-1130,-895,98) : FVector(-435,10,98);
        Player->SetActorLocation(Target);
        if (Phase==6 && FParse::Param(FCommandLine::Get(),TEXT("ONE03SpawnDiagnostics")))
        {
            // Opt-in topology dump distinguishes disconnected navigation from
            // crowd avoidance. It is excluded from normal play and profiling.
            auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
            auto* Recast=Nav?Cast<ARecastNavMesh>(Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)):nullptr;
            if (Recast)
            {
                FString Rows=TEXT("[\n"); bool First=true;
                TArray<FNavTileRef> Tiles; Recast->GetAllNavMeshTiles(Tiles);
                for (int32 Tile=0;Tile<Tiles.Num();++Tile)
                {
                    TArray<FNavPoly> Polys; Recast->GetPolysInTile(Tiles[Tile],Polys);
                    for (const FNavPoly& Poly:Polys)
                    {
                        TArray<FVector> Verts; TArray<NavNodeRef> Neighbors;
                        Recast->GetPolyVerts(Poly.Ref,Verts); Recast->GetPolyNeighbors(Poly.Ref,Neighbors);
                        if (!First) Rows+=TEXT(",\n"); First=false;
                        Rows+=FString::Printf(TEXT("{\"id\":\"%llu\",\"tile\":%d,\"vertices\":["),uint64(Poly.Ref),Tile);
                        for (int32 I=0;I<Verts.Num();++I) Rows+=FString::Printf(TEXT("%s[%.3f,%.3f,%.3f]"),I?TEXT(","):TEXT(""),Verts[I].X,Verts[I].Y,Verts[I].Z);
                        Rows+=TEXT("],\"neighbors\":[");
                        for (int32 I=0;I<Neighbors.Num();++I) Rows+=FString::Printf(TEXT("%s\"%llu\""),I?TEXT(","):TEXT(""),uint64(Neighbors[I]));
                        Rows+=TEXT("]}");
                    }
                }
                Rows+=TEXT("\n]\n"); FFileHelper::SaveStringToFile(Rows,*(Folder/TEXT("navigation_topology.json")));
            }
        }
        for (int32 I=0;I<18;++I)
        {
            const FVector Point=Phase==6 ? FVector(-900+(I/6)*95,300+(I%6)*90,98) :
                Phase==7 ? FVector(-900+(I/6)*95,-500+(I%6)*78,98) : FVector(-850+(I%6)*90,250+(I/6)*95,98);
            Spawn(Point);
        }
        Check(Spawned==18,Segment+FString::Printf(TEXT(" | registered safe spawns %d/18"),Spawned));
    }
    else if (Phase==9)
    {
        Player->SetActorLocation(FVector(0,440,98));
        for (int32 I=0;I<6;++I) Spawn(FVector(200+(I/3)*100,340+(I%3)*90,98));
    }
    UE_LOG(LogTemp,Display,TEXT("ONE03_PHYSICALITY_PHASE %d %s"),Phase,*Segment);
    if (Phase<6) Check(Spawned>0,Segment+TEXT(" | actual registered fixture exists"));
}
void AONE03PhysicalityCheck::Observe(float Dt)
{
    auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>();
    TArray<AONEZombie*> Living;
    int32 Bodies=0,Awake=0,CurrentAttackers=0;
    float Speed=0,Pair=BIG_NUMBER,Overlap=0;
    auto ObserveMesh=[&](USkeletalMeshComponent* Mesh)
    {
        for (FBodyInstance* BI:Mesh->Bodies)
            if (BI && BI->IsValidBodyInstance() && BI->IsInstanceSimulatingPhysics())
            {
                ++Bodies; if (BI->IsInstanceAwake()) ++Awake;
                const FVector Velocity=BI->GetUnrealWorldVelocity();
                if (Velocity.ContainsNaN() || !BI->GetUnrealWorldTransform().IsValid()) bInvalidPhysicsSample=true;
                else Speed=FMath::Max(Speed,float(Velocity.Size()));
            }
    };
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
    {
        AONEZombie* Z=*It; if (!IsValid(Z)) continue;
        if (Z->IsDead()) ObserveMesh(Z->GetMesh());
        else { Living.Add(Z); if (Z->GetCombatState()==EONEZombieState::Attack) { ++AttackFrames; ++CurrentAttackers; } }
    }
    for (TActorIterator<AONEGorePiece> It(GetWorld());It;++It) if (IsValid(*It) && It->GetPieceMesh()) ObserveMesh(It->GetPieceMesh());
    for (int32 I=0;I<Living.Num();++I)
        for (int32 J=I+1;J<Living.Num();++J)
        {
            const float Dist=FVector::Dist2D(Living[I]->GetActorLocation(),Living[J]->GetActorLocation());
            const float Radii=Living[I]->GetCapsuleComponent()->GetScaledCapsuleRadius()+Living[J]->GetCapsuleComponent()->GetScaledCapsuleRadius();
            Pair=FMath::Min(Pair,Dist); Overlap=FMath::Max(Overlap,Radii-Dist);
        }
    for (int32 I=0;I<Subjects.Num() && I<StartPositions.Num();++I)
        if (auto* Z=Subjects[I].Get())
        {
            const float Progress=FVector::Dist2D(StartPositions[I],Z->GetActorLocation());
            MaxProgress=FMath::Max(MaxProgress,Progress); SubjectProgress[I]=FMath::Max(SubjectProgress[I],Progress);
        }
    if (Living.Num()>=6 && Blood->GetCorpseCount()>=3 && Blood->GetWoundCount()>0 && CurrentAttackers>0) CoexistenceSeconds+=Dt;
    MaxBodies=FMath::Max(MaxBodies,Bodies); MaxAwake=FMath::Max(MaxAwake,Awake);
    MaximumVelocity=FMath::Max(MaximumVelocity,Speed); ClosestPair=FMath::Min(ClosestPair,Pair); MaximumOverlap=FMath::Max(MaximumOverlap,Overlap);
    MaxWounds=FMath::Max(MaxWounds,Blood->GetWoundCount()); MaxDrops=FMath::Max(MaxDrops,Blood->GetDropletCount()); MaxDecals=FMath::Max(MaxDecals,Blood->GetDecalCount());
    MaxCorpses=FMath::Max(MaxCorpses,Blood->GetCorpseCount()); MaxParts=FMath::Max(MaxParts,Blood->GetPieceCount());
    MaxPoolRadius=FMath::Max(MaxPoolRadius,float(Blood->GetLargestPoolRadius())); MaxVolume=FMath::Max(MaxVolume,float(Blood->GetRemainingBloodVolume()));
    const float ObservedHealth=Player->GetHealth();
    MinPlayerHealth=FMath::Min(MinPlayerHealth,ObservedHealth); Player->Health->Restore();
    if (Elapsed>=NextSample)
    {
        NextSample=Elapsed+.1f;
        if (bRestTelemetry) ObserveCorpseMotion();
        Telemetry+=FString::Printf(TEXT("%.6f,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.4f,%.4f,%.4f,%d,%.4f,%.4f,%.4f,%.4f\n"),
            Elapsed,Phase,Living.Num(),Blood->GetCorpseCount(),Blood->GetPieceCount(),Bodies,Awake,Blood->GetWoundCount(),Blood->GetDropletCount(),Blood->GetDecalCount(),Blood->GetPoolCount(),
            Blood->GetRemainingBloodVolume(),Blood->GetDepositedBloodVolume(),Blood->GetLargestPoolRadius(),Blood->GetProjectionTracesLastStep(),Speed,Pair==BIG_NUMBER?-1.f:Pair,Overlap,ObservedHealth);
        if (Blood->GetProjectionTracesLastStep()>Blood->GetMaximumProjectionTraces()) Check(false,TEXT("Wound projection work exceeded its per-step bound"));
    }
    CSV_CUSTOM_STAT(ONEPhysicality,Live,Living.Num(),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,RigidBodies,Bodies,ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,Awake,Awake,ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,Wounds,Blood->GetWoundCount(),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,Phase,Phase,ECsvCustomStatOp::Set);
}
void AONE03PhysicalityCheck::ObserveCorpseMotion()
{
    TSet<TWeakObjectPtr<AONEZombie>> Present;
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
    {
        AONEZombie* Corpse=*It;
        if (!Corpse->IsDead()) continue;
        USkeletalMeshComponent* Mesh=Corpse->GetMesh();
        if (!Mesh || !Mesh->GetPhysicsAsset()) continue;
        const TWeakObjectPtr<AONEZombie> Key(Corpse);
        Present.Add(Key);
        FCorpseMotionSample& Previous=CorpseMotionSamples.FindOrAdd(Key);
        TMap<FName,FTransform> Current;
        int32 Bodies=0,Awake=0;
        float Linear=0.f,Angular=0.f,PositionStep=0.f,RotationStep=0.f;
        FName LinearBone=NAME_None,AngularBone=NAME_None;
        bool Finite=true,SameBones=Previous.At>=0.f;
        for (const USkeletalBodySetup* Setup:Mesh->GetPhysicsAsset()->SkeletalBodySetups)
        {
            if (!Setup) continue;
            FBodyInstance* Body=Mesh->GetBodyInstance(Setup->BoneName);
            if (!Body || !Body->IsValidBodyInstance() || !Body->IsInstanceSimulatingPhysics()) continue;
            ++Bodies; Awake+=Body->IsInstanceAwake()?1:0;
            const FTransform Pose=Body->GetUnrealWorldTransform();
            const FVector Velocity=Body->GetUnrealWorldVelocity();
            const FVector AngularVelocity=Body->GetUnrealWorldAngularVelocityInRadians();
            if (!Pose.IsValid() || Velocity.ContainsNaN() || AngularVelocity.ContainsNaN())
            { Finite=false; continue; }
            const float Speed=float(Velocity.Size()),Spin=float(AngularVelocity.Size());
            if (!FMath::IsFinite(Speed) || !FMath::IsFinite(Spin)) { Finite=false; continue; }
            if (Speed>Linear) { Linear=Speed; LinearBone=Setup->BoneName; }
            if (Spin>Angular) { Angular=Spin; AngularBone=Setup->BoneName; }
            Current.Add(Setup->BoneName,Pose);
            if (const FTransform* Old=Previous.BoneTransforms.Find(Setup->BoneName))
            {
                PositionStep=FMath::Max(PositionStep,float(FVector::Distance(Pose.GetLocation(),Old->GetLocation())));
                RotationStep=FMath::Max(RotationStep,float(FMath::RadiansToDegrees(Pose.GetRotation().AngularDistance(Old->GetRotation()))));
            }
            else SameBones=false;
        }
        SameBones&=Current.Num()==Previous.BoneTransforms.Num();
        const float SampleDelta=Previous.At>=0.f?Elapsed-Previous.At:-1.f;
        const bool Continuous=SameBones && SampleDelta>0.f && SampleDelta<=.25f;
        const bool Low=Finite && Bodies>0 && Continuous && Linear<=5.f && Angular<=.12f && PositionStep<=2.f && RotationStep<=2.f;
        Previous.LowMotionSeconds=Low?Previous.LowMotionSeconds+SampleDelta:0.f;
        const auto& Rest=Corpse->GetRestState();
        CorpseMotionTelemetry+=FString::Printf(TEXT("%.6f,%d,%s,%d,%d,%.6f,%.6f,%s,%s,%.6f,%.6f,%.6f,%.6f,%d,%d,%d,%d,%.6f,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f\n"),
            Elapsed,Phase,*Corpse->GetName(),Bodies,Awake,Linear,Angular,*LinearBone.ToString(),*AngularBone.ToString(),SampleDelta,
            SameBones?PositionStep:-1.f,SameBones?RotationStep:-1.f,Previous.LowMotionSeconds,Finite?1:0,
            Rest.ManualSleepEvents,Rest.ExplicitWakeEvents,Rest.ContactWakeEvents,Rest.StableSeconds,
            Rest.Frozen?1:0,Rest.FreezeEvents,Rest.ResumeEvents,Rest.FrozenBodyCount,
            Rest.FreezePositionErrorCm,Rest.FreezeAngleErrorDegrees,Rest.ResumePositionErrorCm,Rest.ResumeAngleErrorDegrees);
        Previous.At=Elapsed; Previous.BoneTransforms=MoveTemp(Current);
    }
    // Weak samples cannot retain expired actors; removal also bounds this state
    // to the currently registered corpse population during the mixed encounter.
    for (auto It=CorpseMotionSamples.CreateIterator();It;++It)
        if (!Present.Contains(It.Key())) It.RemoveCurrent();
}
void AONE03PhysicalityCheck::FinishPhase()
{
    if (bProfile) CSV_EVENT(ONEPhysicality,TEXT("ONE03_PHASE_END index=%d"),Phase);
    auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>();
    const FString Prefix=Segment+TEXT(" | ");
    int32 Frozen=0;
    for (const auto& Weak:Subjects) if (auto* Z=Weak.Get())
    {
        const auto& Rest=Z->GetRestState();
        if (Rest.FreezeEvents>0)
            Check(Rest.FreezePositionErrorCm<.5f && Rest.FreezeAngleErrorDegrees<.5f,
                Prefix+FString::Printf(TEXT("supported freeze preserves evaluated bones and retained body transforms: %.6fcm / %.6fdeg"),Rest.FreezePositionErrorCm,Rest.FreezeAngleErrorDegrees));
        if (Rest.Frozen)
        {
            ++Frozen;
            Check(Rest.FrozenBodyCount>0 && Z->GetActivePhysicsBodyCount()==0 && Z->GetMesh()->GetCollisionEnabled()==ECollisionEnabled::PhysicsOnly,
                Prefix+TEXT("established frozen pose retains physics-only collision"));
        }
    }
    Check(MaxWounds<=96 && MaxDrops<=48 && MaxDecals<=90 && MaxCorpses<=14 && MaxParts<=18,Prefix+TEXT("all presentation owner counts remain bounded"));
    if (Phase>=1 && Phase<=5 || Phase==9)
    {
        Check(MaxBodies>0 && MaxAwake>0,Prefix+TEXT("actual bodies simulated during the sequence"));
        Check(!bInvalidPhysicsSample && FMath::IsFinite(MaximumVelocity) && MaximumVelocity<1800.f,Prefix+FString::Printf(TEXT("finite transforms and bounded physical speed %.2f cm/s"),MaximumVelocity));
    }
    if (Phase==0)
    {
        MinorDeposited=Blood->GetDepositedBloodVolume()-InitialVolume;
        Check(MaxProgress>100.f && MinorDeposited>0 && MaxDecals>=2,Prefix+TEXT("moving wound deposited a finite trail on surfaces"));
        Check(Blood->GetWoundCount()==0 && Blood->GetRemainingBloodVolume()<=.001f,Prefix+TEXT("minor leakage expires without infinite replenishment"));
    }
    if (Phase==1)
    {
        Check(Blood->GetDepositedBloodVolume()-InitialVolume>MinorDeposited,Prefix+TEXT("severe corpse wound deposits more volume than minor wound"));
        Check(MaxPoolRadius>StartPoolRadius+1.f && Blood->GetPoolCount()>0,Prefix+FString::Printf(TEXT("pool grows after its initial stamp %.2f -> %.2fcm"),StartPoolRadius,MaxPoolRadius));
        Check(Blood->GetWoundCount()==0,Prefix+TEXT("severe wound also stops emitting"));
        Check(Blood->GetPoolRenderRadiusErrorCm()<.001f,Prefix+TEXT("actual decal projection extent follows the pool growth state"));
        if (FParse::Param(FCommandLine::Get(),TEXT("ONE03PoolDiagnostics")))
            UE_LOG(LogTemp,Display,TEXT("ONE03_POOL_DIAGNOSTICS\n%s"),*Blood->DescribePools());
    }
    if (Phase==2 || Phase==3)
    {
        if (auto* Z=Subjects.Num()?Subjects[0].Get():nullptr)
        {
            const EONEHitRegion Missing=Phase==2?EONEHitRegion::ArmLeft:EONEHitRegion::ArmRight;
            Check(Z->IsDead() && Z->GetRegionPhysicsBodyCount(Missing)==0,Prefix+TEXT("severed arm collision remains absent in ragdoll"));
            Check(Phase==2?!Z->HasLeftArm()&&Z->HasRightArm():Z->HasLeftArm()&&!Z->HasRightArm(),Prefix+TEXT("correct anatomical arm remains absent"));
        }
        Check(MaxParts>=1,Prefix+TEXT("a separate physical part was present"));
    }
    if (Phase==4)
    {
        Check(MaxParts>=2,Prefix+TEXT("head and selected leg created independent parts"));
        for (int32 I=0;I<Subjects.Num();++I) if (auto* Z=Subjects[I].Get())
            Check(Z->IsDead() && Z->GetRegionPhysicsBodyCount(I==0?EONEHitRegion::Head:EONEHitRegion::LegLeft)==0,Prefix+TEXT("fatal region loss has no invisible retained collider"));
    }
    if (Phase==5)
    {
        int32 Low=0,Distinct=0,Awake=0;
        for (const auto& Weak:Subjects) if (auto* Z=Weak.Get())
        {
            if (!Z->IsDead()) continue;
            const FTransform Pelvis=Z->GetMesh()->GetSocketTransform(TEXT("pelvis"));
            if (Pelvis.GetLocation().Z<55 && Pelvis.GetLocation().Z>-5) ++Low;
            bool Unique=true;
            for (const FQuat& Other:FinalPelvisRotations) if (FMath::RadiansToDegrees(Pelvis.GetRotation().AngularDistance(Other))<=5.f) { Unique=false; break; }
            if (Unique) { ++Distinct; FinalPelvisRotations.Add(Pelvis.GetRotation()); }
            Awake+=Z->GetAwakePhysicsBodyCount();
        }
        Check(Low==6,Prefix+TEXT("all six bodies reached the floor contact area"));
        Check(Distinct>=3,Prefix+FString::Printf(TEXT("different settled pelvis orientations %d/6"),Distinct));
        Check(Awake<MaxBodies,Prefix+FString::Printf(TEXT("contact-established bodies sleep or preserve supported poses; awake %d / peak %d; frozen corpses %d"),Awake,MaxBodies,Frozen));
        Report+=Prefix+(bFrozenWakeTested?TEXT("fresh hit resumed a naturally qualifying frozen corpse during this run\n"):TEXT("no qualifying frozen corpse was available for the optional fresh-hit probe after8s; no forced freeze was used\n"));
    }
    if (Phase>=6 && Phase<=8)
    {
        const int32 Progressed=SubjectProgress.FilterByPredicate([](float P){return P>100.f;}).Num();
        Check(Progressed==Spawned && Spawned==18 && AttackFrames>0 && MinPlayerHealth<100,
            Prefix+FString::Printf(TEXT("individual progress over 100cm %d/%d and real attack contact"),Progressed,Spawned));
        Check(ClosestPair<70.f && MaximumOverlap<6.f,Prefix+FString::Printf(TEXT("close physical crowd: nearest %.2fcm / maximum overlap %.2fcm"),ClosestPair,MaximumOverlap));
    }
    if (Phase==9)
    {
        Check(MaxCorpses>=6 && CoexistenceSeconds>=.3f,Prefix+FString::Printf(TEXT("simultaneous living attacks, corpses and wounds for %.3fs"),CoexistenceSeconds));
        Check(Player->GetWeaponComponent()->GetTotalShotsFired()>0,Prefix+TEXT("normal equipped weapon discharged in the mixed encounter"));
    }
    if (Phase==10)
        Check(Blood->GetGeneration()==CleanupGeneration && Blood->GetWoundCount()==0 && Blood->GetDropletCount()==0 && Blood->GetPieceCount()==0 && Blood->GetCorpseCount()==0 && Blood->GetDecalCount()==0,
            Prefix+TEXT("production cleanup leaves no owned state or later blood respawn"));
}
void AONE03PhysicalityCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished)
    {
        if (bProfile && !FinishProfileWrite()) return;
        if (FPlatformTime::Seconds()-FinishedAt>(bProfile?3.:2.)) FPlatformMisc::RequestExit(false);
        return;
    }
    Elapsed+=Dt;
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player || Elapsed<5) return;
    if (bProfile && Phase<0 && !bCsvStarted && !StartProfile()) return;
    if (bCapture && !bRecording)
    { UAudioMixerBlueprintLibrary::StartRecordingOutput(this,210); AudioStart=FPlatformTime::Seconds(); bRecording=true; }
    if (Phase<0) { Phase=FirstPhase-1; PreparePhase(); }
    if (bFinished) return;
    const float T=Elapsed-PhaseStart;
    auto Subject=[&](int32 I)->AONEZombie* { return Subjects.IsValidIndex(I)?Subjects[I].Get():nullptr; };
    auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>();
    if (T>=NextAction)
    {
        if (Phase==0 && ActionIndex==0) { Damage(Subject(0),EONEHitRegion::Body,8,8,FVector(-1,0,0)); ++ActionIndex; }
        else if (Phase==1 && ActionIndex<2)
        { Damage(Subject(0),EONEHitRegion::Body,ActionIndex?130:45,ActionIndex?60:45,FVector(-1,.25,.15)); ++ActionIndex; NextAction=1.8f; }
        else if ((Phase==2 || Phase==3) && ActionIndex<2)
        {
            if (ActionIndex==0) Damage(Subject(0),Phase==2?EONEHitRegion::ArmLeft:EONEHitRegion::ArmRight,22,90,FVector(-1,Phase==2?-1:1,.2));
            else Damage(Subject(0),EONEHitRegion::Body,130,40,FVector(-1,0,.1));
            ++ActionIndex; NextAction=3.5f;
        }
        else if (Phase==4 && ActionIndex<2)
        { Damage(Subject(ActionIndex),ActionIndex==0?EONEHitRegion::Head:EONEHitRegion::LegLeft,30,110,FVector(-1,ActionIndex?.7:-.7,.1)); ++ActionIndex; NextAction=1.6f; }
        else if (Phase==5 && ActionIndex<6)
        { Damage(Subject(ActionIndex),EONEHitRegion::Body,150,55,FVector(ActionIndex%2?-1:.3f,ActionIndex%3-1,.1).GetSafeNormal()); ++ActionIndex; NextAction=1.1f+ActionIndex*.18f; }
        else if (Phase==9)
        {
            if (ActionIndex<6) { Damage(Subject(ActionIndex),EONEHitRegion::Body,150,55,FVector(-1,ActionIndex%2?.4:-.4,.1)); ++ActionIndex; NextAction=T+.15f; }
            else
            {
                GetWorld()->GetAuthGameMode<AONEGameMode>()->SpawnSandboxEnemies(6);
                ++ActionIndex; NextAction=T+1.f;
                if (ActionIndex%4==0) Player->GetWeaponComponent()->RefillAllAmmo();
            }
        }
    }
    if (Phase==5 && T>8.f && !bFrozenWakeTested)
    {
        for (const auto& Weak:Subjects) if (auto* Z=Weak.Get())
        {
            if (!Z->GetRestState().Frozen) continue;
            const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
            const int32 Points=GM->GetPoints(),Kills=GM->GetKills(),Before=Z->GetCorpseTransactionCount();
            const int32 Bodies=Z->GetRestState().FrozenBodyCount,Resumes=Z->GetRestState().ResumeEvents;
            Damage(Z,EONEHitRegion::Body,8,0,FVector(-1,.1,0));
            const auto& Rest=Z->GetRestState();
            Check(!Rest.Frozen && Rest.ResumeEvents==Resumes+1 && Z->GetActivePhysicsBodyCount()==Bodies && Z->GetAwakePhysicsBodyCount()>0,
                TEXT("Fresh body hit resumes retained frozen bodies before applying corpse impulse"));
            Check(Rest.ResumePositionErrorCm<.5f && Rest.ResumeAngleErrorDegrees<.5f,
                FString::Printf(TEXT("Frozen-to-simulated transition preserves bones and collision: %.6fcm / %.6fdeg"),Rest.ResumePositionErrorCm,Rest.ResumeAngleErrorDegrees));
            Check(GM->GetPoints()==Points && GM->GetKills()==Kills && Z->GetHealth()==0.f && Z->GetCorpseTransactionCount()==Before+1,
                TEXT("Fresh frozen-corpse hit is one corpse transaction with no health or kill award"));
            bFrozenWakeTested=true; break;
        }
    }
    if (Phase==1 && T>2.1f && !bPoolBaseline) { StartPoolRadius=Blood->GetLargestPoolRadius(); bPoolBaseline=true; }
    if (Phase==9 && ActionIndex>=6)
    {
        const float Angle=T*.6f;
        Player->AddMovementInput(FVector(FMath::Cos(Angle),FMath::Sin(Angle),0),.45f);
        AONEZombie* Nearest=nullptr; float Distance=BIG_NUMBER;
        for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (!It->IsDead())
        {
            const float D=FVector::DistSquared(It->GetActorLocation(),Player->GetActorLocation());
            if (D<Distance) { Distance=D; Nearest=*It; }
        }
        if (Nearest) Player->SetAimOverride(true,Nearest->GetMesh()->GetSocketLocation(TEXT("spine_02")));
        auto* W=Player->GetWeaponComponent();
        if (W->GetEquippedIndex()==0) W->SetTrigger(Nearest && FMath::Fmod(T,1.4f)<.7f);
        else W->SetTrigger(Nearest && FMath::Fmod(T,.95f)<.10f);
    }
    Observe(Dt); Capture();
    static const float Duration[]={7,12,10,10,9,13,11,11,11,20,3};
    if (T>Duration[Phase]) { FinishPhase(); PreparePhase(); }
    if (Elapsed>200 && !bFinished) { Check(false,TEXT("Scenario exceeded bounded runtime")); Finish(); }
}
