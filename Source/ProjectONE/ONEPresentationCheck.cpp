#include "ONEPresentationCheck.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEGameMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"

namespace
{
    struct FPhase { const TCHAR* Name; float End; FVector Direction; bool Run; bool Fire; bool Reload; };
    const FPhase Phases[]={
        {TEXT("01_idle"),2.f,FVector::ZeroVector,false,false,false},
        {TEXT("02_forward_walk"),4.f,FVector(1,0,0),false,false,false},
        {TEXT("03_run"),5.5f,FVector(1,0,0),true,false,false},
        {TEXT("04_backward"),7.5f,FVector(-1,0,0),false,false,false},
        {TEXT("05_strafe_negative_y"),9.5f,FVector(0,-1,0),false,false,false},
        {TEXT("06_strafe_positive_y"),11.5f,FVector(0,1,0),false,false,false},
        {TEXT("07_diagonal"),13.5f,FVector(-1,1,0),false,false,false},
        {TEXT("08_moving_fire"),15.5f,FVector(-1,-1,0),false,true,false},
        {TEXT("09_moving_reload"),18.f,FVector(0,1,0),false,false,true},
        {TEXT("10_ready"),20.f,FVector::ZeroVector,false,false,false}
    };
}
AONEPresentationCheck::AONEPresentationCheck() { PrimaryActorTick.bCanEverTick=true; }
void AONEPresentationCheck::BeginPlay()
{
    Super::BeginPlay();
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Presentation")),true);
    Report=TEXT("Project ONE scripted in-engine presentation check\nActual character movement, authored skeletal clips, weapon logic and attack timing.\nCamera is the ordinary gameplay camera. Test-only aim holds +X for locomotion, then tracks the nearest living crowd enemy.\n\n");
    FrameReport=TEXT("timestamp_ms,phase,player_x,player_y,speed_cm_s,left_foot_z,right_foot_z\n");
    if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->MaximumActive=0;
    UE_LOG(LogTemp,Display,TEXT("ONE_PRESENTATION_BEGIN"));
}
void AONEPresentationCheck::Check(bool Pass,const FString& Label)
{
    if (!Pass) ++Failures;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE_PRESENTATION %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONEPresentationCheck::Capture(const FString& Name)
{
    FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("Presentation")/(Name+TEXT(".png")),true,false);
}
void AONEPresentationCheck::EndPhase()
{
    if (CurrentPhase<0 || CurrentPhase>=UE_ARRAY_COUNT(Phases) || !Player) return;
    const FPhase& Phase=Phases[CurrentPhase];
    const float Average=SpeedSamples ? SpeedSum/SpeedSamples : 0.f;
    Report+=FString::Printf(TEXT("MEASURE | %s mean speed %.2fcm/s; maximum component-space foot displacement %.2fcm; samples %d\n"),Phase.Name,Average,FootTravel,SpeedSamples);
    if (!Phase.Direction.IsNearlyZero())
    {
        const float Expected=Phase.Run ? Player->RunSpeed : Player->WalkSpeed;
        Check(Average>Expected*.55f && Average<Expected*1.3f,FString(Phase.Name)+TEXT(" observed movement speed plausible"));
        Check(FootTravel>4.f,FString(Phase.Name)+TEXT(" authored foot pose changes while moving"));
        if (Phase.Fire)
        {
            Check(Player->GetWeaponComponent()->GetAmmo()<PhaseAmmoStart,TEXT("Moving-fire phase consumes real ammunition"));
            Check(FootTravel>4.f,TEXT("Lower-body animation continues during upper-body firing"));
        }
        if (Phase.Reload) Check(!Player->GetWeaponComponent()->IsReloading() && Player->GetWeaponComponent()->GetAmmo()==Player->GetWeaponComponent()->MagazineSize,TEXT("Reload completes while lower-body locomotion continues"));
    }
    else Check(Average<8.f,FString(Phase.Name)+TEXT(" stationary speed settles"));
}
AONEZombie* AONEPresentationCheck::SpawnEnemy(const FVector& Position)
{
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    return GetWorld()->SpawnActor<AONEZombie>(Position,FRotator(0,180,0),Params);
}
void AONEPresentationCheck::Finish()
{
    if (bComplete) return; bComplete=true;
    Player->ReleaseHeldInputs(); Player->SetAimOverride(false,FVector::ZeroVector);
    const float Span=LastRecord-FirstRecord;
    Report+=FString::Printf(TEXT("\nFailures: %d\nRecorded frames: %d; span %.3fs; actual capture rate %.2ffps (target at most 8fps). PNG readback is synchronous and affects timing; this is not a performance benchmark. Actual timestamps are in frames.csv and filenames.\n"),Failures,RecordCount,Span,Span>0?(RecordCount-1)/Span:0.f);
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Presentation");
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("presentation.txt")));
    FFileHelper::SaveStringToFile(FrameReport,*(Folder/TEXT("frames.csv")));
    UE_LOG(LogTemp,Display,TEXT("ONE_PRESENTATION_COMPLETE failures=%d frames=%d"),Failures,RecordCount);
    FPlatformMisc::RequestExit(false);
}
void AONEPresentationCheck::Tick(float Dt)
{
    Super::Tick(Dt); if (bComplete) return; Elapsed+=Dt;
    if (!Player)
    {
        Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
        if (Player)
        {
            // The test route begins in the measured clear center corridor, independent
            // of future ordinary PlayerStart edits. All phase motion then uses input.
            Player->SetActorLocation(FVector(-300,100,95));
            Player->GetCharacterMovement()->StopMovementImmediately();
        }
    }
    if (!Player) { if (Elapsed>8) { Check(false,TEXT("Player unavailable")); FPlatformMisc::RequestExit(false); } return; }
    if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->MaximumActive=0;
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(800,0,42));
    if (Elapsed<24 || (Elapsed>29 && AttackStage>=6)) Player->Health->Restore();
    int32 NextPhase=0;
    while (NextPhase<UE_ARRAY_COUNT(Phases) && Elapsed>=Phases[NextPhase].End) ++NextPhase;
    bool RequestedCapture=false;
    if (NextPhase!=CurrentPhase)
    {
        EndPhase(); CurrentPhase=NextPhase; PhaseStart=Elapsed; bPhaseCaptured=false; FootTravel=0; SpeedSum=0; SpeedSamples=0;
        FirstLeftFoot=Player->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation();
        FirstRightFoot=Player->GetMesh()->GetSocketTransform(TEXT("foot_r"),RTS_Component).GetLocation();
        Player->ReleaseHeldInputs();
        if (CurrentPhase<UE_ARRAY_COUNT(Phases))
        {
            const FPhase& Phase=Phases[CurrentPhase];
            Player->SetSprintHeld(Phase.Run);
            Player->GetWeaponComponent()->SetTrigger(Phase.Fire);
            PhaseAmmoStart=Player->GetWeaponComponent()->GetAmmo();
            if (Phase.Reload) Player->GetWeaponComponent()->BeginReload();
        }
    }
    if (CurrentPhase<UE_ARRAY_COUNT(Phases))
    {
        const FPhase& Phase=Phases[CurrentPhase];
        if (!Phase.Direction.IsNearlyZero()) Player->AddMovementInput(Phase.Direction.GetSafeNormal(),1.f);
        if (Elapsed-PhaseStart>.35f)
        {
            SpeedSum+=Player->GetVelocity().Size2D(); ++SpeedSamples;
            const FVector L=Player->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation();
            const FVector R=Player->GetMesh()->GetSocketTransform(TEXT("foot_r"),RTS_Component).GetLocation();
            FootTravel=FMath::Max(FootTravel,static_cast<float>(FMath::Max(FVector::Dist(L,FirstLeftFoot),FVector::Dist(R,FirstRightFoot))));
        }
        if (!bPhaseCaptured && Elapsed-PhaseStart>.70f) { Capture(Phase.Name); RequestedCapture=true; bPhaseCaptured=true; }
        if (GEngine) GEngine->AddOnScreenDebugMessage(7351,.2f,FColor(75,190,170),FString(TEXT("SCRIPTED PRESENTATION / "))+Phase.Name);
    }
    // Timestamped genuine framebuffer captures include directed crowd hits, without interpolated frames.
    if (Elapsed>=11.5f && Elapsed<24.f && Elapsed-LastRecord>=.125f && !RequestedCapture)
    {
        if (RecordCount==0) FirstRecord=Elapsed;
        LastRecord=Elapsed; const int32 Millis=FMath::RoundToInt(Elapsed*1000);
        Capture(FString::Printf(TEXT("record_%07d"),Millis)); ++RecordCount;
        const FVector Pos=Player->GetActorLocation();
        const TCHAR* Name=CurrentPhase<UE_ARRAY_COUNT(Phases) ? Phases[CurrentPhase].Name : TEXT("crowd_combat");
        FrameReport+=FString::Printf(TEXT("%d,%s,%.2f,%.2f,%.2f,%.2f,%.2f\n"),Millis,Name,Pos.X,Pos.Y,Player->GetVelocity().Size2D(),Player->GetMesh()->GetSocketLocation(TEXT("foot_l")).Z,Player->GetMesh()->GetSocketLocation(TEXT("foot_r")).Z);
    }
    if (Elapsed>=20 && !bCrowdSpawned)
    {
        bCrowdSpawned=true;
        Crowd.Add(SpawnEnemy(FVector(-500,400,98))); Crowd.Add(SpawnEnemy(FVector(500,350,98))); Crowd.Add(SpawnEnemy(FVector(0,-300,98)));
        Player->GetWeaponComponent()->SetTrigger(true);
    }
    if (Elapsed>=20 && Elapsed<24)
    {
        AONEZombie* Nearest=nullptr; float Best=MAX_flt;
        for (auto& Z:Crowd)
            if (IsValid(Z) && !Z->IsDead())
            {
                const float Distance=FVector::DistSquared2D(Player->GetActorLocation(),Z->GetActorLocation());
                if (Distance<Best) { Best=Distance; Nearest=Z; }
            }
        if (Nearest) Player->SetAimOverride(true,Nearest->GetActorLocation()+FVector(0,0,40));
        Player->AddMovementInput(FVector(-1,0,0),1.f);
        if (Elapsed>22.5f && AttackStage==0) { Capture(TEXT("11_three_enemy_navigation")); AttackStage=1; }
    }
    if (Elapsed>=23.8f && !bCrowdDamageChecked)
    {
        bCrowdDamageChecked=true;
        int32 Damaged=0,Killed=0;
        for (auto& Z:Crowd)
            if (IsValid(Z)) { if (Z->GetHealth()<Z->Health->MaxHealth) ++Damaged; if (Z->IsDead()) ++Killed; }
        Check(Damaged>0,FString::Printf(TEXT("Real aimed weapon fire damages crowd enemies (%d damaged, %d killed)"),Damaged,Killed));
    }
    if (Elapsed>=24 && AttackStage<=1)
    {
        for (auto& Z:Crowd) if (IsValid(Z)) Z->Destroy(); Crowd.Empty();
        Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately();
        Player->SetActorLocation(FVector(0,400,95)); Player->Health->Restore(); Player->LastDamageTime=-100;
        HealthBefore=Player->GetHealth(); Attacker=SpawnEnemy(Player->GetActorLocation()+FVector(95,0,0));
        Check(Attacker!=nullptr,TEXT("Uninterrupted attacker spawned")); AttackStage=2;
    }
    if (AttackStage==2 && Attacker && Attacker->GetCombatState()==EONEZombieState::Attack && Attacker->GetStateElapsed()>.49f && Attacker->GetStateElapsed()<.65f)
    { Capture(TEXT("12_uninterrupted_attack_contact")); AttackStage=3; }
    if (Elapsed>24.15f && Elapsed<26.3f && IsValid(Attacker))
    {
        const FName Names[]={TEXT("head"),TEXT("lowerarm_l"),TEXT("hand_l")};
        USkeletalMeshComponent* Leader=Attacker->GetMesh();
        if (!bModularSampled)
        {
            bModularSampled=true;
            for (FName Bone:Names)
            {
                Check(Leader->RequiredBones.Contains(Leader->GetBoneIndex(Bone)),FString::Printf(TEXT("Leader evaluates required modular bone %s"),*Bone.ToString()));
                ModularStart.Add(Leader->GetSocketTransform(Bone,RTS_Component).GetLocation()); ModularTravel.Add(0.f);
            }
        }
        for (int32 I=0;I<3;++I)
        {
            const FVector Pos=Leader->GetSocketTransform(Names[I],RTS_Component).GetLocation();
            ModularTravel[I]=FMath::Max(ModularTravel[I],static_cast<float>(FVector::Dist(Pos,ModularStart[I])));
        }
    }
    if (Elapsed>=26.3f && AttackStage<=3)
    {
        Check(Player->GetHealth()<HealthBefore,TEXT("Uninterrupted authored attack delivers contact damage"));
        const TCHAR* Names[]={TEXT("head"),TEXT("lowerarm_l"),TEXT("hand_l")};
        for (int32 I=0;I<3;++I) Check(ModularTravel.IsValidIndex(I) && ModularTravel[I]>.3f,FString::Printf(TEXT("Modular %s moves during authored intact attack (%.2fcm)"),Names[I],ModularTravel.IsValidIndex(I)?ModularTravel[I]:0.f));
        if (IsValid(Attacker)) Attacker->Destroy();
        Player->Health->Restore(); Player->LastDamageTime=-100; HealthBefore=Player->GetHealth();
        Attacker=SpawnEnemy(Player->GetActorLocation()+FVector(95,0,0));
        if (Attacker)
        {
            FHitResult Hit; Hit.BoneName=TEXT("upperarm_l"); Hit.ImpactPoint=Attacker->GetMesh()->GetSocketLocation(Hit.BoneName);
            Attacker->ReceiveBullet(Hit,FVector(1,0,0),32); Attacker->ReceiveBullet(Hit,FVector(1,0,0),32);
        }
        Check(Attacker && !Attacker->HasRightArm() && Attacker->HasLeftArm() && !Attacker->IsDead(),TEXT("Attack candidate survives loss of source_l / anatomical right arm"));
        AttackStage=4;
    }
    if (AttackStage==4 && Attacker && Attacker->GetCombatState()==EONEZombieState::Attack && Attacker->GetStateElapsed()>.49f && Attacker->GetStateElapsed()<.65f)
    { Capture(TEXT("13_one_arm_attack_contact")); AttackStage=5; }
    if (Elapsed>=29 && AttackStage<=5)
    {
        Report+=FString::Printf(TEXT("MEASURE | surviving-arm attack health before %.2f, after %.2f\n"),HealthBefore,Player->GetHealth());
        Check(Attacker && !Attacker->HasRightArm() && Attacker->HasLeftArm() && Player->GetHealth()<HealthBefore,TEXT("Surviving one-arm zombie delivers remaining-arm attack damage"));
        Check(RecordCount>2 && FirstRecord<12.f && LastRecord>23.7f,TEXT("Timestamped gameplay sequence spans movement and crowd combat"));
        AttackStage=6;
    }
    if (Elapsed>=29 && Elapsed<33)
    {
        Player->AddMovementInput(FVector(-1,0,0),1.f);
        Player->GetWeaponComponent()->SetTrigger(true);
    }
    if (Elapsed>=34) Finish();
}
