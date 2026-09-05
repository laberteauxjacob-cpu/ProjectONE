#include "ONE05MotionCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEZombieAudioComponent.h"
#include "ONEAnimInstance.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
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

namespace
{
    const FVector Directions[]={FVector(1,0,0),FVector(1,1,0),FVector(0,1,0),FVector(-1,1,0),
        FVector(-1,0,0),FVector(-1,-1,0),FVector(0,-1,0),FVector(1,-1,0)};
    const TCHAR* DirectionNames[]={TEXT("FORWARD"),TEXT("FORWARD RIGHT"),TEXT("RIGHT"),TEXT("BACK RIGHT"),
        TEXT("BACK"),TEXT("BACK LEFT"),TEXT("LEFT"),TEXT("FORWARD LEFT")};
}
AONE05MotionCheck::AONE05MotionCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE05MotionCheck::BeginPlay()
{
    Super::BeginPlay();
    bCapture=FParse::Param(FCommandLine::Get(),TEXT("ONE05MotionCapture"));
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate05")/(bCapture?TEXT("MotionCapture"):TEXT("MotionCheck"));
    Folder/=FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8);
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report=TEXT("Candidate05 scripted production-input motion and explicit attack fixtures\n")
        TEXT("Normal gameplay camera and real PlayerController WASD/Shift/mouse events. This is not native human input or ordinary uninterrupted survival.\n")
        TEXT("Each stride/turn fixture resets player transform and stops previous movement at its boundary. Three isolated attack fixtures each reset player health once, spawn a production infected, select a family using the common TryStartAttack entry, and suspend that actor after recovery to prevent a second attack.\n")
        TEXT("The final body-hit fixture restores health/ammo once and spawns a pursuing infected; pistol shots then use actual production input, traces, damage outcomes, hit/death audio and ragdoll. After the kill, production WASD backs away before a low corpse trace so the fixture respects the normal 120cm forward-convergence/35degree pitch limit. This does not claim that point-blank low corpses are precisely aimable. No per-tick health restoration or camera override.\n")
        TEXT("Numeric checks do not establish naturalism, foot planting or audible timbre. Review chronological recorded frames and actual audio separately.\n");
    FramesCsv=TEXT("file,audio_seconds,world_seconds,phase,weapon,ammo,reserve,operation,frame\n");
    PosesCsv=TEXT("seconds,phase,frame,speed,actor_x,actor_y,actor_z,actor_yaw,body_yaw,pelvis_x,pelvis_y,pelvis_z,left_x,left_y,left_z,right_x,right_y,right_z,hand_l_x,hand_l_y,hand_l_z,hand_r_x,hand_r_y,hand_r_z,health,player_reaction_age,ammo,operation,shots,outcome,enemy_x,enemy_y,enemy_z,enemy_health,enemy_state,enemy_age,enemy_family,contact_attempts,damage_dispatches,minor_age,minor_strength,attack_audio,hit_audio,death_audio,enemy_pelvis_z,enemy_left_z,enemy_right_z,enemy_body_x,enemy_body_y,enemy_body_z,last_shot_muzzle_x,last_shot_muzzle_y,last_shot_muzzle_z,last_shot_dir_x,last_shot_dir_y,last_shot_dir_z,turn_time,turning,pivot_l_weight,pivot_r_weight\n");
    InputCsv=TEXT("seconds,phase,frame,key,event,handled\n");
    ChaptersCsv=TEXT("phase,seconds,label\n");
    if (bCapture)
    {
        UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE05MotionCheck::Screenshot);
        if (auto* Mixer=FAudioDeviceManager::GetAudioMixerDeviceFromWorldContext(this))
            if (auto Master=Mixer->GetMasterSubmix().Pin()) Mixer->AudioRenderThreadCommand([Master](){Master->SetAutoDisable(false);});
    }
}
void AONE05MotionCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; Failures+=Pass?0:1;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE05_MOTION %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE05MotionCheck::Key(const FKey& K,bool Down)
{
    if (!Controller || Held.Contains(K)==Down) return;
    const bool Handled=Controller->InputKey(FInputKeyEventArgs::CreateSimulated(K,Down?IE_Pressed:IE_Released,Down?1.f:0.f));
    if (Down) Held.Add(K); else Held.Remove(K);
    InputCsv+=FString::Printf(TEXT("%.6f,%d,%llu,%s,%s,%d\n"),Elapsed,Phase,static_cast<unsigned long long>(GFrameCounter),*K.ToString(),Down?TEXT("pressed"):TEXT("released"),Handled);
}
void AONE05MotionCheck::ReleaseKeys()
{ const TArray<FKey> Keys=Held.Array(); for (const FKey& K:Keys) Key(K,false); }
void AONE05MotionCheck::AimAt(const FVector& Point)
{
    FVector2D Screen; int32 Width=0,Height=0;
    Controller->GetViewportSize(Width,Height);
    if (Width>0 && Height>0 && Controller->ProjectWorldLocationToScreen(Point,Screen))
    {
        Player->SetAimOverride(false,FVector::ZeroVector);
        Controller->SetMouseLocation(FMath::Clamp(FMath::RoundToInt(Screen.X),1,Width-2),FMath::Clamp(FMath::RoundToInt(Screen.Y),1,Height-2));
    }
    else
    {
        // Headless numerical checks cannot dispatch a projected desktop cursor.
        // The report explicitly counts every fallback; rendered captures forbid it.
        ++CursorFallbacks; Player->SetAimOverride(true,Point);
    }
}
void AONE05MotionCheck::SpawnFixture(const FVector& Point,bool Hold)
{
    if (Enemy) Enemy->Destroy();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
    Enemy=GetWorld()->SpawnActor<AONEZombie>(Point,FRotator(0,180,0),Params);
    Check(IsValid(Enemy),TEXT("Explicit infected fixture spawned without collision adjustment"));
    if (!Enemy) { Finish(false); return; }
    if (Hold)
    {
        Enemy->SetActorTickEnabled(false);
        if (auto* AI=Cast<AAIController>(Enemy->GetController())) AI->StopMovement();
        Enemy->GetCharacterMovement()->DisableMovement();
        Enemy->ZombieAudio->SetPursuing(false);
    }
}
void AONE05MotionCheck::EnterPhase()
{
    ReleaseKeys(); Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately();
    PhaseStart=Elapsed; Samples=0; SpeedSum=0; SpeedMin=BIG_NUMBER; SpeedMax=0;
    AimMaxDegrees=DirectionMaxDegrees=0.f;
    bMovementPressed=false; bAttackStarted=false; bAttackSettled=false; PlayerReactionSamples=0;
    for (int32 I=0;I<2;++I) { FootMin[I]=BIG_NUMBER;FootMax[I]=-BIG_NUMBER; }
    if (Phase<16)
    {
        MoveDirection=Directions[Phase%8];
        const float Span=Phase<8?135.f:220.f;
        Player->SetActorLocation(Origin-MoveDirection.GetSafeNormal()*Span,false,nullptr,ETeleportType::TeleportPhysics);
        Segment=FString::Printf(TEXT("INPUT STRIDE FIXTURE / %s / %s"),Phase<8?TEXT("WALK 225"):TEXT("SPRINT 370"),DirectionNames[Phase%8]);
    }
    else if (Phase<18)
    {
        Player->SetActorLocation(Origin,false,nullptr,ETeleportType::TeleportPhysics);
        Segment=Phase==16?TEXT("CURSOR TURN FIXTURE / CONTINUOUS CLOCKWISE THEN COUNTERCLOCKWISE"):
            TEXT("CURSOR TURN FIXTURE / RAPID 180 DEGREE REVERSALS");
    }
    else if (Phase<21)
    {
        Player->SetActorLocation(Origin,false,nullptr,ETeleportType::TeleportPhysics);
        Player->GetHealthComponent()->Restore(); PhaseHealth=Player->GetHealth();
        SpawnFixture(Origin-FVector(80,0,0),true);
        Segment=FString::Printf(TEXT("EXPLICIT ATTACK FAMILY %d / ORDINARY DAMAGE AFTER SINGLE FIXTURE RESET"),Phase-18);
        AttackAudioStart=Enemy?Enemy->ZombieAudio->GetAttackCueCount():0;
        if (Phase==18 && Enemy)
        {
            auto* Anim=Cast<UONEAnimInstance>(Enemy->GetMesh()->GetAnimInstance());
            bool Loaded=Anim!=nullptr;
            for (const TCHAR* Name:{TEXT("C05_SwipeLeft"),TEXT("C05_SwipeRight"),TEXT("C05_RakeLeft"),TEXT("C05_RakeRight"),TEXT("C05_TwoHand")})
                Loaded=Loaded && Anim->FindClip(Name)!=nullptr;
            Check(Loaded,TEXT("All five revised attacks loaded on the production infected graph"));
        }
    }
    else if (Phase==21)
    {
        Player->SetActorLocation(Origin,false,nullptr,ETeleportType::TeleportPhysics);
        Player->GetHealthComponent()->Restore(); Player->GetWeaponComponent()->RefillAllAmmo();
        SpawnFixture(Origin+FVector(350,0,0),false);
        ObservedShots=Player->GetWeaponComponent()->GetTotalShotsFired();
        LiveOutcomes=KillOutcomes=CorpseOutcomes=MinorSamples=0;DeathAt=-1;NextFire=Elapsed+.7f;ReleaseFireAt=0;bCorpseRetreatComplete=false;
        Segment=TEXT("EXPLICIT TARGET SETUP / REAL PISTOL BODY HIT, MINOR REACTION, NEW KILL, CORPSE HIT");
    }
    else Segment=TEXT("FINAL REAL CORPSE / AUDIO TAIL / NO INPUT");
    ChaptersCsv+=FString::Printf(TEXT("%d,%.6f,%s\n"),Phase,Elapsed,*Segment);
    UE_LOG(LogTemp,Display,TEXT("ONE05_MOTION_PHASE phase=%d seconds=%.6f label=%s"),Phase,Elapsed,*Segment);
}
void AONE05MotionCheck::CompletePhase()
{
    if (Phase<16)
    {
        const float Expected=Phase<8?Player->WalkSpeed:Player->RunSpeed;
        const float Mean=Samples?SpeedSum/Samples:0;
        Check(Samples>=5 && FMath::Abs(Mean-Expected)<3.f,FString::Printf(TEXT("Phase%d production keys produce expected speed %.1f; mean %.3f min %.3f max %.3f samples%d"),Phase,Expected,Mean,SpeedMin,SpeedMax,Samples));
        Check(Player->IsSprintRequested()==(Phase>=8),TEXT("Processed Shift state matches requested movement mode"));
        Check(AimMaxDegrees<4.f && DirectionMaxDegrees<4.f,FString::Printf(TEXT("Named direction verified against actual body heading and velocity; maximum aim/direction errors %.3f/%.3f degrees"),AimMaxDegrees,DirectionMaxDegrees));
    }
    else if (Phase<18)
    {
        Check(Samples>10 && FootMax[0]-FootMin[0]>2.f && FootMax[1]-FootMin[1]>2.f,
            FString::Printf(TEXT("Turn phase%d has evaluated lift on both feet %.3f/%.3f cm; not a planting assertion"),Phase,FootMax[0]-FootMin[0],FootMax[1]-FootMin[1]));
    }
    else if (Phase<21)
    {
        Check(bAttackStarted && Enemy && Enemy->GetAttackFamily()==Phase-18,TEXT("Requested family used the production attack entry"));
        Check(Enemy && Enemy->GetAttackDamageDispatchCount()==1 && Enemy->GetAttackContactAttemptCount()==1,TEXT("Family dispatched exactly one consuming contact"));
        Check(FMath::IsNearlyEqual(Player->GetHealth(),PhaseHealth-19.f,.01f),TEXT("Ordinary contact removed19 health without an in-phase restore"));
        Check(PlayerReactionSamples>0,TEXT("Accepted player damage produced a brief directional reaction window"));
        Check(Enemy && Enemy->ZombieAudio->GetAttackCueCount()==AttackAudioStart+1,TEXT("Attack family issued one actual component audio cue"));
    }
    else if (Phase==21)
    {
        Check(LiveOutcomes>=1 && MinorSamples>=1,TEXT("Real nonfatal torso-aimed shot reported live feedback with minor pose reaction while no full stagger; exact hit region not asserted"));
        Check(KillOutcomes==1 && Enemy && Enemy->IsDead(),TEXT("Real trace transitioned one living target to one new kill"));
        Check(CorpseOutcomes>=1 && Enemy && Enemy->GetCorpseTransactionCount()>=1,TEXT("Later real corpse trace stayed cosmetic and did not repeat kill feedback"));
        Check(Enemy && Enemy->ZombieAudio->GetDeathCueCount()==1 && !Enemy->ZombieAudio->IsLivingAudioEnabled(),TEXT("Death issued one cue and stopped living audio scheduling"));
        Check(!Player->IsDead(),TEXT("Target fixture completed while player remained alive"));
    }
    ++Phase;
    if (Phase>22) { Finish(true); return; }
    EnterPhase();
}
void AONE05MotionCheck::RunPhase(float Dt)
{
    const float T=Elapsed-PhaseStart;
    if (Phase<16)
    {
        AimAt(Player->GetAimOrigin()+FVector(600,0,0));
        if (T>=.35f && !bMovementPressed)
        {
            bMovementPressed=true;
            Key(EKeys::LeftShift,Phase>=8);
            if (MoveDirection.X>0) Key(EKeys::D,true); else if (MoveDirection.X<0) Key(EKeys::A,true);
            if (MoveDirection.Y>0) Key(EKeys::S,true); else if (MoveDirection.Y<0) Key(EKeys::W,true);
        }
        if (T>=.65f && T<1.55f)
        {
            const float Speed=Player->GetVelocity().Size2D();SpeedSum+=Speed;SpeedMin=FMath::Min(SpeedMin,Speed);SpeedMax=FMath::Max(SpeedMax,Speed);++Samples;
            AimMaxDegrees=FMath::Max(AimMaxDegrees,FMath::Abs(FMath::FindDeltaAngleDegrees(0.f,Player->GetBodyFacingYaw())));
            const double Dot=FMath::Clamp(FVector::DotProduct(Player->GetVelocity().GetSafeNormal2D(),MoveDirection.GetSafeNormal2D()),-1.,1.);
            DirectionMaxDegrees=FMath::Max(DirectionMaxDegrees,float(FMath::RadiansToDegrees(FMath::Acos(Dot))));
        }
        if (T>=1.6f) CompletePhase();
    }
    else if (Phase<18)
    {
        const float Angle=Phase==16 ? (T<3.f?T*120.f:360.f-(T-3.f)*180.f) : (int32(T/.7f)%2 ? 180.f : 0.f);
        AimAt(Player->GetAimOrigin()+FRotator(0,Angle,0).Vector()*500.f);
        if (T>.5f)
        {
            const float L=Player->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z,R=Player->GetMesh()->GetSocketLocation(TEXT("foot_r")).Z;
            FootMin[0]=FMath::Min(FootMin[0],L);FootMax[0]=FMath::Max(FootMax[0],L);
            FootMin[1]=FMath::Min(FootMin[1],R);FootMax[1]=FMath::Max(FootMax[1],R);++Samples;
        }
        if (T>=6.f) CompletePhase();
    }
    else if (Phase<21)
    {
        if (!Enemy) return;
        AimAt(Enemy->GetActorLocation()+FVector(0,0,28));
        if (T>=.5f && !bAttackStarted)
        {
            Enemy->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Enemy->SetActorTickEnabled(true);
            bAttackStarted=Enemy->TryStartAttack(Player,Phase-18);
            if (!bAttackStarted) { Check(false,TEXT("Explicit family failed to enter windup")); Finish(false); return; }
        }
        if (Player->GetDamageReactionAge()>=0.f && Player->GetDamageReactionAge()<.28f) ++PlayerReactionSamples;
        if (bAttackStarted && !bAttackSettled && Enemy->GetCombatState()==EONEZombieState::Pursue)
        {
            bAttackSettled=true;Enemy->SetActorTickEnabled(false);Enemy->GetCharacterMovement()->DisableMovement();
            if (auto* AI=Cast<AAIController>(Enemy->GetController())) AI->StopMovement();
            Enemy->ZombieAudio->SetPursuing(false);
        }
        if (T>=2.5f) CompletePhase();
    }
    else if (Phase==21)
    {
        if (!Enemy) return;
        auto* W=Player->GetWeaponComponent();
        const FVector AimTarget=Enemy->BodyRegion->GetComponentLocation();
        AimAt(AimTarget);
        if (Enemy->IsDead() && DeathAt<0)
        {
            DeathAt=Elapsed;Key(EKeys::LeftMouseButton,false);ReleaseFireAt=0;
            Segment=TEXT("REAL CORPSE / WASD RETREAT TO A FEASIBLE LOW TARGET CONVERGENCE DISTANCE");
            UE_LOG(LogTemp,Display,TEXT("ONE05_MOTION_CORPSE_RETREAT seconds=%.6f initial_distance=%.3f"),Elapsed,FVector::Dist2D(Player->GetActorLocation(),AimTarget));
        }
        if (Enemy->IsDead() && !bCorpseRetreatComplete)
        {
            const FVector Away=(Player->GetActorLocation()-AimTarget).GetSafeNormal2D(SMALL_NUMBER,-FVector::ForwardVector);
            const float Distance=FVector::Dist2D(Player->GetActorLocation(),AimTarget);
            if (Distance<260.f)
            {
                Key(EKeys::D,Away.X>.35);Key(EKeys::A,Away.X<-.35);
                Key(EKeys::S,Away.Y>.35);Key(EKeys::W,Away.Y<-.35);
            }
            else
            {
                ReleaseKeys();bCorpseRetreatComplete=true;NextFire=Elapsed+.25f;
                Segment=TEXT("REAL CORPSE / RELEASE MOVEMENT THEN FIRE AT EVALUATED TORSO QUERY");
                UE_LOG(LogTemp,Display,TEXT("ONE05_MOTION_CORPSE_RETREAT_COMPLETE seconds=%.6f distance=%.3f"),Elapsed,Distance);
            }
        }
        if (ReleaseFireAt>0 && Elapsed>=ReleaseFireAt) { Key(EKeys::LeftMouseButton,false);ReleaseFireAt=0; }
        if (Elapsed>=NextFire && CorpseOutcomes==0 && !W->IsBusy() && (!Enemy->IsDead() || bCorpseRetreatComplete))
        { Key(EKeys::LeftMouseButton,true);ReleaseFireAt=Elapsed+.07f;NextFire=Elapsed+.40f; }
        if (W->GetTotalShotsFired()!=ObservedShots)
        {
            ObservedShots=W->GetTotalShotsFired();
            switch (W->GetLastShotOutcome())
            {
                case EONEWeaponHitOutcome::LiveHit:++LiveOutcomes;break;
                case EONEWeaponHitOutcome::NewKill:++KillOutcomes;break;
                case EONEWeaponHitOutcome::CorpseHit:++CorpseOutcomes;break;
                default:break;
            }
        }
        if (!Enemy->IsDead() && Enemy->GetMinorReactionAge()<.22f && Enemy->GetMinorReactionStrength()>0.f && Enemy->GetCombatState()!=EONEZombieState::Hit) ++MinorSamples;
        if (T>=9.f || (CorpseOutcomes>0 && DeathAt>0 && Elapsed-DeathAt>1.2f)) CompletePhase();
    }
    else if (T>=2.f) CompletePhase();
}
void AONE05MotionCheck::Observe()
{
    const auto* W=Player->GetWeaponComponent();auto* M=Player->GetMesh();
    const FVector A=Player->GetActorLocation(),P=M->GetSocketLocation(TEXT("pelvis")),L=M->GetSocketLocation(TEXT("foot_l")),R=M->GetSocketLocation(TEXT("foot_r"));
    const FVector HL=M->GetSocketLocation(TEXT("hand_l")),HR=M->GetSocketLocation(TEXT("hand_r")),E=Enemy?Enemy->GetActorLocation():FVector::ZeroVector;
    const auto* Audio=Enemy?Enemy->ZombieAudio.Get():nullptr;
    const FVector EB=Enemy?Enemy->BodyRegion->GetComponentLocation():FVector::ZeroVector;
    const FVector ShotMuzzle=W->GetLastShotMuzzle(),ShotDirection=W->GetLastShotDirection();
    PosesCsv+=FString::Printf(TEXT("%.6f,%d,%llu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.6f,%d,%d,%d,%d,%.4f,%.4f,%.4f,%.4f,%d,%.6f,%d,%d,%d,%.6f,%.4f,%d,%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.6f,%.6f,%.6f,%.6f,%d,%.6f,%.6f\n"),
        Elapsed,Phase,static_cast<unsigned long long>(GFrameCounter),Player->GetVelocity().Size2D(),A.X,A.Y,A.Z,Player->GetActorRotation().Yaw,Player->GetBodyFacingYaw(),
        P.X,P.Y,P.Z,L.X,L.Y,L.Z,R.X,R.Y,R.Z,HL.X,HL.Y,HL.Z,HR.X,HR.Y,HR.Z,Player->GetHealth(),Player->GetDamageReactionAge(),W->GetAmmo(),int32(W->GetOperation()),W->GetTotalShotsFired(),int32(W->GetLastShotOutcome()),
        E.X,E.Y,E.Z,Enemy?Enemy->GetHealth():0.f,Enemy?int32(Enemy->GetCombatState()):-1,Enemy?Enemy->GetStateElapsed():0.f,Enemy?Enemy->GetAttackFamily():-1,
        Enemy?Enemy->GetAttackContactAttemptCount():0,Enemy?Enemy->GetAttackDamageDispatchCount():0,Enemy?Enemy->GetMinorReactionAge():0.f,Enemy?Enemy->GetMinorReactionStrength():0.f,
        Audio?Audio->GetAttackCueCount():0,Audio?Audio->GetHitCueCount():0,Audio?Audio->GetDeathCueCount():0,
        Enemy?Enemy->GetMesh()->GetSocketLocation(TEXT("pelvis")).Z:0.,Enemy?Enemy->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z:0.,Enemy?Enemy->GetMesh()->GetSocketLocation(TEXT("foot_r")).Z:0.,
        EB.X,EB.Y,EB.Z,ShotMuzzle.X,ShotMuzzle.Y,ShotMuzzle.Z,ShotDirection.X,ShotDirection.Y,ShotDirection.Z,
        Player->GetTurnAnimationTime(),Player->IsTurningInPlace()?1:0,Player->GetPivotFootWeight(0),Player->GetPivotFootWeight(1));
}
void AONE05MotionCheck::Capture()
{
    if (!bCapture || !bRecording || bFinishing || bFinished) return;
    const double Now=FPlatformTime::Seconds();
    if (PendingFrame.IsEmpty() && Now-LastCapture>=1./30.)
    { LastCapture=Now;PendingFrame=FString::Printf(TEXT("frame_%05d.jpg"),Frames);FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false); }
}
void AONE05MotionCheck::Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty() || !Player) return;
    const double At=FPlatformTime::Seconds()-AudioStart;const auto* W=Player->GetWeaponComponent();
    const FString Row=FString::Printf(TEXT("%s,%.9f,%.6f,%d,%d,%d,%d,%d,%llu\n"),*PendingFrame,At,Elapsed,Phase,W->GetEquippedIndex(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation()),static_cast<unsigned long long>(GFrameCounter));
    if (FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),Width,Height),85)) {FramesCsv+=Row;++Frames;}
    else Check(false,TEXT("A requested real viewport frame failed to save"));
    PendingFrame.Empty();
    LastScreenshotCompletedAt=FPlatformTime::Seconds();
}
void AONE05MotionCheck::Finish(bool Complete)
{
    if (bFinished || bFinishing) return;
    bComplete=Complete;bFinishing=true;FinishRequestedAt=FPlatformTime::Seconds();ReleaseKeys();
    if (Player) Player->SetAimOverride(false,FVector::ZeroVector);
    if (!bCapture) Finalize();
}
void AONE05MotionCheck::Finalize()
{
    if (bFinished) return;bFinished=true;FinishedAt=FPlatformTime::Seconds();
    if (bRecording)
    { UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("gameplay_master"),FPaths::ConvertRelativePathToFull(Folder));bRecording=false; }
    Check(!bCapture || (Frames>50 && CursorFallbacks==0),TEXT("Rendered capture has actual viewport frames and no headless cursor fallback"));
    const bool FramesSaved=FFileHelper::SaveStringToFile(FramesCsv,*(Folder/TEXT("frames.csv")));
    const bool PosesSaved=FFileHelper::SaveStringToFile(PosesCsv,*(Folder/TEXT("poses.csv")));
    const bool InputsSaved=FFileHelper::SaveStringToFile(InputCsv,*(Folder/TEXT("input_events.csv")));
    const bool ChaptersSaved=FFileHelper::SaveStringToFile(ChaptersCsv,*(Folder/TEXT("chapters.csv")));
    Check(FramesSaved && PosesSaved && InputsSaved && ChaptersSaved,TEXT("Complete frame, evaluated-pose, input and chapter manifests saved successfully"));
    Report+=FString::Printf(TEXT("\nComplete: %d\nChecks: %d\nFailures: %d\nFrames: %d\nHeadless cursor fallback calls: %d\n"),bComplete,Checks,Failures,Frames,CursorFallbacks);
    if (!FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")))) Check(false,TEXT("Motion assertion report could not be saved"));
    UE_LOG(LogTemp,Display,TEXT("ONE05_MOTION_COMPLETE complete=%d failures=%d checks=%d frames=%d"),bComplete,Failures,Checks,Frames);
}
void AONE05MotionCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished) {if (FPlatformTime::Seconds()-FinishedAt>2.) FPlatformMisc::RequestExit(false);return;}
    if (bFinishing)
    {
        // Stop new screenshots, drain the final callback, then retain a genuine
        // mixer tail. No padded/stretched audio or manufactured image frame.
        const double Now=FPlatformTime::Seconds();
        if (!PendingFrame.IsEmpty() && Now-FinishRequestedAt>5.)
        {
            Check(false,TEXT("Final requested screenshot callback did not arrive within five seconds"));
            PendingFrame.Empty();bComplete=false;LastScreenshotCompletedAt=Now;
        }
        if (PendingFrame.IsEmpty() && Now-FMath::Max(FinishRequestedAt,LastScreenshotCompletedAt)>.40) Finalize();
        return;
    }
    Elapsed+=Dt;
    if (!Player)
    {
        Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
        Controller=Player?Cast<AONEPlayerController>(Player->GetController()):nullptr;
        Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
    }
    if (!Player || !Controller || !Mode)
    {if (Elapsed>10) {Check(false,TEXT("Production player/controller/game mode unavailable"));Finish(false);}return;}
    if (Elapsed<2.f) return;
    if (bCapture && !bRecording)
    {UAudioMixerBlueprintLibrary::StartRecordingOutput(this,110.f);AudioStart=FPlatformTime::Seconds();bRecording=true;}
    if (Phase<0)
    {
        Check(Mode->IsSandbox(),TEXT("Explicit motion scenario uses sandbox mode"));
        auto* Anim=Cast<UONEAnimInstance>(Player->GetMesh()->GetAnimInstance());
        bool Clips=Anim!=nullptr;
        for (const TCHAR* Gait:{TEXT("Walk"),TEXT("Run")}) for (const TCHAR* Direction:{TEXT("F"),TEXT("FR"),TEXT("R"),TEXT("BR"),TEXT("B"),TEXT("BL"),TEXT("L"),TEXT("FL")})
            Clips=Clips && Anim->FindClip(FName(*FString::Printf(TEXT("C05_%s_%s"),Gait,Direction)))!=nullptr;
        Clips=Clips && Anim->FindClip(TEXT("C05_Turn_L")) && Anim->FindClip(TEXT("C05_Turn_R"));
        Check(Clips,TEXT("All 18 revised directional and turn player clips loaded on the production graph"));
        Check(Player->WalkSpeed==225.f && Player->RunSpeed==370.f,TEXT("Accepted player speed tuning remains225/370"));
        if (Failures) {Finish(false);return;}
        Phase=0;EnterPhase();
    }
    Observe();RunPhase(Dt);Capture();
    if (Elapsed>100.f) {Check(false,TEXT("Motion scenario exceeded100 second bounded runtime"));Finish(false);}
}
void AONE05MotionCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    PendingFrame.Empty();
    if (!bFinished) {Check(false,TEXT("Motion run interrupted by level transition or exit"));bComplete=false;bFinishing=true;ReleaseKeys();Finalize();}
    UGameViewportClient::OnScreenshotCaptured().RemoveAll(this);Super::EndPlay(Reason);
}
