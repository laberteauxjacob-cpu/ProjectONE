#include "ONEValidation.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEGameMode.h"
#include "ONEBloodSubsystem.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "UnrealClient.h"
#include "HighResScreenshot.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/IConsoleManager.h"
#include "RHI.h"

namespace { bool bONEValidationRestart=false; FString ONERestartEntries; int32 ONERestartFailures=0; }

AONEValidation::AONEValidation() { PrimaryActorTick.bCanEverTick=true; }
void AONEValidation::BeginPlay()
{
    Super::BeginPlay();
    FParse::Value(FCommandLine::Get(),TEXT("ONEBenchmark="),BenchmarkCount);
    IFileManager::Get().MakeDirectory(*(FPaths::ProjectSavedDir()/TEXT("Validation")),true);
    UE_LOG(LogTemp,Display,TEXT("ONE_VALIDATION_BEGIN benchmark=%d GPU=%s"),BenchmarkCount,*GRHIAdapterName);
    if (bONEValidationRestart) { Stage=16; Entries=ONERestartEntries; Failed=ONERestartFailures; bONEValidationRestart=false; }
}
void AONEValidation::Check(bool Pass,const FString& Label)
{
    if (!Pass) ++Failed;
    Entries += FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE_CHECK %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONEValidation::Capture(const FString& Label)
{
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Validation");
    FScreenshotRequest::RequestScreenshot(Folder/(Label+TEXT(".png")),true,false);
}
AONEZombie* AONEValidation::SpawnTest(const FVector& Location)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    return GetWorld()->SpawnActor<AONEZombie>(Location,FRotator(0,180,0),Params);
}
void AONEValidation::Hit(AONEZombie* Zombie,FName Bone,float Damage)
{
    if (!IsValid(Zombie)) return;
    FHitResult Result;
    Result.BoneName=Bone;
    Result.ImpactPoint=Zombie->GetMesh()->GetSocketLocation(Bone);
    Zombie->ReceiveBullet(Result,FVector(1,0,.15),Damage);
}
void AONEValidation::SaveAndExit()
{
    FString Report=FString::Printf(TEXT("Project ONE actual in-engine validation\nGPU: %s\nResolution: %s\nFailures: %d\n\n%s"),*GRHIAdapterName,GEngine&&GEngine->GameViewport?*GEngine->GameViewport->Viewport->GetSizeXY().ToString():TEXT("unavailable"),Failed,*Entries);
    for (const TCHAR* Setting : {TEXT("sg.ViewDistanceQuality"),TEXT("sg.AntiAliasingQuality"),TEXT("sg.ShadowQuality"),TEXT("sg.PostProcessQuality"),TEXT("sg.TextureQuality"),TEXT("sg.EffectsQuality"),TEXT("r.ScreenPercentage"),TEXT("r.AntiAliasingMethod"),TEXT("r.DynamicGlobalIlluminationMethod"),TEXT("r.ReflectionMethod"),TEXT("r.VSync"),TEXT("t.MaxFPS")})
        if (IConsoleVariable* Value=IConsoleManager::Get().FindConsoleVariable(Setting)) Report+=FString::Printf(TEXT("%s=%s\n"),Setting,*Value->GetString());
    if (FrameTimes.Num())
    {
        FrameTimes.Sort();
        double Sum=0; for (float F:FrameTimes) Sum+=F;
        Report+=FString::Printf(TEXT("\nFrame samples: %d; mean %.3f ms; median %.3f ms; p95 %.3f ms; requested benchmark enemies %d\n"),FrameTimes.Num(),Sum/FrameTimes.Num()*1000,FrameTimes[FrameTimes.Num()/2]*1000,FrameTimes[FMath::Clamp(FMath::FloorToInt(FrameTimes.Num()*.95f),0,FrameTimes.Num()-1)]*1000,BenchmarkCount);
    }
    const FString Name=BenchmarkCount?FString::Printf(TEXT("benchmark_%d.txt"),BenchmarkCount):TEXT("runtime.txt");
    FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("Validation")/Name));
    UE_LOG(LogTemp,Display,TEXT("ONE_VALIDATION_COMPLETE failures=%d"),Failed);
    FPlatformMisc::RequestExit(false);
}
void AONEValidation::Tick(float Dt)
{
    Super::Tick(Dt); Elapsed+=Dt;
    AONEPlayer* P=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    AONEGameMode* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!P || !GM) { if (Elapsed>30) { Check(false,TEXT("Missing player or game mode")); SaveAndExit(); } return; }
    if (FParse::Param(FCommandLine::Get(),TEXT("ONERecord")) && Stage>=6 && Stage<=7 && Elapsed-LastRecordFrame>.125f)
    {
        LastRecordFrame=Elapsed;
        Capture(FString::Printf(TEXT("record_%07d"),FMath::RoundToInt(Elapsed*1000)));
    }
    UONEWeaponComponent* W=P->GetWeaponComponent();
    if (BenchmarkCount>0)
    {
        GM->MaximumActive=0;
        P->Health->Restore();
        if (Stage==0 && Elapsed>4)
        {
            for (int32 I=0;I<BenchmarkCount;++I)
            {
                const float Angle=2*PI*I/BenchmarkCount;
                AONEZombie* Z=SpawnTest(FVector(FMath::Cos(Angle)*750,FMath::Sin(Angle)*650,98));
                if (Z) Z->AttackDamage=0;
            }
            Stage=1;
        }
        if (Elapsed>10 && Elapsed<30) FrameTimes.Add(Dt);
        if (Stage==1 && Elapsed>15) { Capture(FString::Printf(TEXT("benchmark_%d"),BenchmarkCount)); Stage=2; }
        if (Elapsed>31)
        {
            int32 Count=0; for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (!It->IsDead()) ++Count;
            Check(Count==BenchmarkCount,FString::Printf(TEXT("Sustained runtime actual alive=%d requested=%d"),Count,BenchmarkCount)); SaveAndExit();
        }
        return;
    }
    if (Stage<10 || Stage==13) P->Health->Restore();
    if (Stage==0 && Elapsed>3)
    {
        Check(P->GetMesh()->GetSkeletalMeshAsset()!=nullptr,TEXT("Player skeletal mesh loaded"));
        Check(P->GetMesh()->GetNumBones()>20,TEXT("Player reusable skeleton contains >20 bones"));
        Check(P->GetMesh()->GetAnimInstance()!=nullptr,TEXT("Player native skeletal animation graph active"));
        Check(P->Gun->GetStaticMesh()!=nullptr,TEXT("New carbine loaded"));
        Check(P->GetMesh()->Bounds.BoxExtent.Z>60 && P->GetMesh()->Bounds.BoxExtent.Z<130,TEXT("Player imported scale plausible 120-260cm height"));
        Capture(TEXT("01_idle"));
        ArmTest=SpawnTest(P->GetActorLocation()+FVector(220,0,0));
        if (ArmTest) { ArmTest->SetActorTickEnabled(false); InitialArmLocation=ArmTest->GetActorLocation(); InitialDistance=FVector::Dist2D(ArmTest->GetActorLocation(),P->GetActorLocation()); }
        Check(ArmTest && ArmTest->HeadMesh->GetSkeletalMeshAsset() && ArmTest->ArmMesh->GetSkeletalMeshAsset(),TEXT("Infected modular meshes loaded"));
        Stage=1; StageTime=Elapsed;
    }
    else if (Stage==1 && Elapsed-StageTime>1)
    {
        Hit(ArmTest,TEXT("upperarm_l"),32); Hit(ArmTest,TEXT("upperarm_l"),32);
        Check(ArmTest && !ArmTest->HasRightArm() && ArmTest->HasLeftArm() && !ArmTest->IsDead(),TEXT("Two source_l arm hits sever anatomical right arm while infected survives"));
        if (ArmTest)
        {
            UONEBloodSubsystem* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>();
            const int32 PiecesBefore=Blood?Blood->GetPieceCount():-1;
            bool bPoseMatched=false;
            for (TActorIterator<AONEGorePiece> It(GetWorld());It;++It)
                if (It->GetPieceMesh() && It->GetPieceMesh()->GetSkeletalMeshAsset() && It->GetActivePhysicsBodyCount()>0 && It->GetTransitionErrorCm()<1.f) bPoseMatched=true;
            Check(bPoseMatched,TEXT("Detached arm begins at current evaluated shoulder transform"));
            const float Before=ArmTest->GetHealth(); Hit(ArmTest,TEXT("upperarm_l"),500);
            Check(ArmTest->GetHealth()==Before && !ArmTest->IsDead(),TEXT("Repeated removed-arm damage ignored"));
            Check(Blood && Blood->GetPieceCount()==PiecesBefore,TEXT("Repeated severing does not create duplicate pieces"));
            Check(!ArmTest->ArmMesh->IsVisible() && ArmTest->ArmRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("Removed arm rendering and hit detection disabled"));
            ArmTest->SetActorTickEnabled(true);
        }
        Stage=2; StageTime=Elapsed;
    }
    else if (Stage==2 && Elapsed-StageTime>1)
    {
        Capture(TEXT("02_surviving_arm_loss"));
        HeadTest=SpawnTest(P->GetActorLocation()+FVector(-230,0,0));
        Stage=3; StageTime=Elapsed;
    }
    else if (Stage==3 && Elapsed-StageTime>.7f)
    {
        Hit(HeadTest,TEXT("head"),32);
        Check(HeadTest && HeadTest->IsDead() && !HeadTest->HasHead(),TEXT("Head removal causes immediate death"));
        if (HeadTest) { const float Before=HeadTest->GetHealth(); Hit(HeadTest,TEXT("head"),32); Check(HeadTest->GetHealth()==Before,TEXT("Repeated head damage does not mutate dead health")); }
        Stage=4; StageTime=Elapsed;
    }
    else if (Stage==4 && Elapsed-StageTime>.8f)
    {
        Capture(TEXT("03_head_removal"));
        IntactTest=SpawnTest(P->GetActorLocation()+FVector(150,-230,0));
        Stage=5; StageTime=Elapsed;
    }
    else if (Stage==5 && Elapsed-StageTime>.6f)
    {
        Hit(IntactTest,TEXT("spine_02"),1000);
        Check(IntactTest && IntactTest->IsDead() && IntactTest->HasHead() && IntactTest->HasLeftArm() && IntactTest->HasRightArm() && IntactTest->HasLeftLeg(),TEXT("Lethal torso hit preserves an intact corpse"));
        W->SetTrigger(true); AmmoBefore=W->GetAmmo(); ReserveBefore=W->GetReserveAmmo();
        Stage=6; StageTime=Elapsed;
    }
    else if (Stage==6)
    {
        if (Elapsed-StageTime<3) P->AddMovementInput(FVector(1,0,0),.5f);
        if (W->IsReloading() && W->GetAmmo()==0)
        {
            Check(W->GetAmmo()==0,TEXT("Held trigger exhausts magazine at cadence"));
            W->SetTrigger(false);
            Check(W->GetAutomaticReloadCount()>0 && W->GetReserveAmmo()==ReserveBefore,TEXT("Final shot automatically starts reload before transferring reserve ammunition"));
            Check(W->IsReloading(),TEXT("Reload starts from empty magazine without a second input"));
            Capture(TEXT("04_reload_and_crowd")); Stage=7; StageTime=Elapsed;
        }
        else if (Elapsed-StageTime>8) { Check(false,TEXT("Final-shot automatic reload started within cadence deadline")); SaveAndExit(); }
    }
    else if (Stage==7 && Elapsed-StageTime>2.5f)
    {
        Check(W->GetAmmo()==W->MagazineSize && W->GetReserveAmmo()==ReserveBefore-W->MagazineSize,TEXT("Reload conserves total ammunition"));
        Check(!W->IsReloading(),TEXT("Reload completion exits reload state"));
        Check(ArmTest && FVector::Dist2D(ArmTest->GetActorLocation(),InitialArmLocation)>40 && FVector::Dist2D(ArmTest->GetActorLocation(),P->GetActorLocation())<InitialDistance+50,TEXT("One-arm infected moves and continues pursuing"));
        Capture(TEXT("05_combat")); Stage=8; StageTime=Elapsed;
    }
    else if (Stage==8)
    {
        // Clear through the real damage API, including game-mode registered enemies.
        for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (!It->IsDead()) Hit(*It,TEXT("spine_02"),1000);
        if (GM->IsIntermission() && GM->GetRound()>=1)
        {
            PointsBefore=GM->GetPoints();
            for (TActorIterator<AONEZombie> It(GetWorld());It;++It) GM->NotifyZombieKilled(*It,100);
            Check(GM->GetPoints()==PointsBefore,TEXT("Duplicate kill notifications do not award points"));
            Check(GM->GetRemaining()==0,TEXT("Round completion reaches zero remaining"));
            Capture(TEXT("06_round_complete")); Stage=9; StageTime=Elapsed;
        }
    }
    else if (Stage==9 && GM->GetRound()>=2)
    {
        Check(true,TEXT("Round two starts after intermission"));
        GM->MaximumActive=0;
        for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (!It->IsDead()) Hit(*It,TEXT("spine_02"),1000);
        AttackTest=SpawnTest(P->GetActorLocation()+FVector(95,0,0));
        Stage=10; StageTime=Elapsed;
    }
    else if (Stage==10 && AttackTest && AttackTest->GetCombatState()==EONEZombieState::Attack)
    {
        Hit(AttackTest,TEXT("spine_02"),1);
        Check(AttackTest->GetCombatState()==EONEZombieState::Hit,TEXT("Hit interrupts a pending attack"));
        HealthBefore=P->GetHealth(); Stage=11; StageTime=Elapsed;
    }
    else if (Stage==11 && Elapsed-StageTime>.52f)
    {
        Check(P->GetHealth()==HealthBefore,TEXT("Interrupted attack did not deliver delayed damage"));
        Hit(AttackTest,TEXT("head"),32); Stage=12; StageTime=Elapsed;
    }
    else if (Stage==12 && Elapsed-StageTime>1.1f)
    {
        Check(P->GetHealth()==HealthBefore,TEXT("Dead attacker did not deliver delayed damage"));
        if (UONEBloodSubsystem* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>())
        {
            for (int32 I=0;I<120;++I) Blood->Pool(FVector(-800+(I%10)*12,500+(I/10)*12,40),15);
            Check(Blood->GetDecalCount()>0 && Blood->GetDecalCount()<=90,TEXT("Persistent ground blood remains bounded at 90 decals"));
            Check(Blood->GetPieceCount()<=18 && Blood->GetCorpseCount()<=14,TEXT("Detached pieces and corpses remain within explicit caps"));
        }
        Stage=13; StageTime=Elapsed;
    }
    else if (Stage==13)
    {
        if (Elapsed-StageTime>29) W->SetTrigger(true);
        if (Elapsed-StageTime>30)
        {
            int32 DeadCount=0;
            for (TActorIterator<AONEZombie> It(GetWorld());It;++It) if (It->IsDead()) ++DeadCount;
            Check(DeadCount==0,TEXT("Expired corpses cleaned after 30 seconds"));
            if (UONEBloodSubsystem* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Check(Blood->GetPieceCount()==0,TEXT("Expired detached pieces cleaned after 30 seconds"));
            W->SetTrigger(false); W->BeginReload();
            Check(W->IsReloading(),TEXT("Reload in progress immediately before lethal attack"));
            Stage=14; StageTime=Elapsed;
            P->ReceiveAttack(1000,P->GetActorLocation()+FVector(100,0,0));
        }
    }
    else if (Stage==14 && Elapsed-StageTime>.5f)
    {
        Check(P->IsDead() && GM->IsGameOver(),TEXT("Player death enters game-over state"));
        Check(!W->IsReloading(),TEXT("Player death cancels reload"));
        Capture(TEXT("07_game_over")); Stage=15; StageTime=Elapsed;
    }
    else if (Stage==15 && Elapsed-StageTime>1)
    {
        ONERestartEntries=Entries; ONERestartFailures=Failed; bONEValidationRestart=true;
        GM->RestartScene();
    }
    else if (Stage==16 && Elapsed>2)
    {
        Check(!P->IsDead() && !GM->IsGameOver() && GM->GetPoints()==0 && GM->GetRound()==0,TEXT("Restart loads a living player and resets round/score state"));
        Capture(TEXT("08_restart")); Stage=17; StageTime=Elapsed;
    }
    else if (Stage==17 && Elapsed-StageTime>1) SaveAndExit();
    if (Elapsed>110) { Check(false,FString::Printf(TEXT("Runtime validation timed out at stage %d"),Stage)); SaveAndExit(); }
}
