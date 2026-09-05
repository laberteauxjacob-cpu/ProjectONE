#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEPlayerController.h"
#include "ONEHUD.h"
#include "ONEWeaponComponent.h"
#include "ONEBloodSubsystem.h"
#include "DrawDebugHelpers.h"
#include "ONEValidation.h"
#include "ONEPresentationCheck.h"
#include "ONECombatCheck.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/TargetPoint.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"

AONEGameMode::AONEGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AONEPlayer::StaticClass();
    PlayerControllerClass = AONEPlayerController::StaticClass();
    HUDClass = AONEHUD::StaticClass();
    ZombieClass = AONEZombie::StaticClass();
}
void AONEGameMode::BeginPlay()
{
    Super::BeginPlay();
    bSandbox=UGameplayStatics::HasOption(OptionsString,TEXT("ONESandbox")) || FParse::Param(FCommandLine::Get(),TEXT("ONECombatCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONECompare"));
    if (bSandbox) { bIntermission=false; Countdown=0; }
    // Authored material categories drive concrete versus metal impact audio.
    for (TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
        if (const UStaticMesh* Mesh=It->GetStaticMeshComponent()->GetStaticMesh())
        {
            const FString Name=Mesh->GetName();
            if (Name.Contains(TEXT("Rack")) || Name.Contains(TEXT("Door")) || Name.Contains(TEXT("Vessel")) || Name.Contains(TEXT("Bench")) || Name.Contains(TEXT("Console")) || Name.Contains(TEXT("Hatch")) || Name.Contains(TEXT("Barrier")))
                It->Tags.AddUnique(TEXT("Metal"));
        }
    for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
        if (It->ActorHasTag("ONE_Spawn")) SpawnLocations.Add(It->GetActorLocation());
    if (SpawnLocations.IsEmpty()) SpawnLocations = { FVector(-950,-700,100), FVector(950,-700,100), FVector(-950,650,100), FVector(950,650,100) };
    if (FParse::Param(FCommandLine::Get(),TEXT("ONEValidate")) || FString(FCommandLine::Get()).Contains(TEXT("ONEBenchmark=")))
        GetWorld()->SpawnActor<AONEValidation>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONEPresentation"))) GetWorld()->SpawnActor<AONEPresentationCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONECombatCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONECompare"))) GetWorld()->SpawnActor<AONECombatCheck>();
}
void AONEGameMode::StartRound()
{
    if (Round > 0)
        if (AONEPlayer* Player = Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) Player->GetWeaponComponent()->GrantRoundAmmo();
    ++Round;
    ToSpawn = FMath::Min(4 + Round * 2, 40);
    bIntermission = false;
    SpawnClock = .1f;
    UE_LOG(LogTemp, Display, TEXT("ONE_ROUND_START round=%d count=%d"), Round, ToSpawn);
}
void AONEGameMode::SpawnEnemy()
{
    const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector PlayerPos = Player ? Player->GetActorLocation() : FVector::ZeroVector;
    FVector Point = SpawnLocations[FMath::RandHelper(SpawnLocations.Num())];
    for (int32 Attempt = 0; Attempt < 8 && FVector::DistSquared2D(Point, PlayerPos) < FMath::Square(650.f); ++Attempt)
        Point = SpawnLocations[FMath::RandHelper(SpawnLocations.Num())];
    if (UNavigationSystemV1* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation Projected;
        if (Nav->GetRandomReachablePointInRadius(Point, 120.f, Projected)) Point = Projected.Location + FVector(0,0,98);
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    if (AONEZombie* Zombie = GetWorld()->SpawnActor<AONEZombie>(ZombieClass, Point, FRotator::ZeroRotator, Params))
    {
        Alive.Add(Zombie);
        --ToSpawn;
    }
}
void AONEGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bGameOver) return;
    if (bSandbox)
    {
        for (auto It=Alive.CreateIterator();It;++It) if (!It->IsValid()) It.RemoveCurrent();
        const FVector Origin(-500,300,6);
        DrawDebugLine(GetWorld(),Origin,Origin+FVector(1000,0,0),FColor(64,180,173),false,-1,0,2);
        for (int32 Distance : {0,200,500,1000})
        {
            const FVector Tick=Origin+FVector(Distance,0,0);
            DrawDebugLine(GetWorld(),Tick-FVector(0,35,0),Tick+FVector(0,35,0),FColor(230,170,60),false,-1,0,2);
            DrawDebugString(GetWorld(),Tick+FVector(0,-50,10),FString::Printf(TEXT("%dm"),Distance/100),nullptr,FColor::White,0,false,.8f);
        }
        return;
    }
    if (bIntermission)
    {
        Countdown -= DeltaSeconds;
        if (Countdown <= 0) StartRound();
        return;
    }
    for (auto It = Alive.CreateIterator(); It; ++It) if (!It->IsValid()) It.RemoveCurrent();
    SpawnClock -= DeltaSeconds;
    if (ToSpawn > 0 && Alive.Num() < MaximumActive && SpawnClock <= 0)
    {
        SpawnEnemy();
        SpawnClock = SpawnInterval;
    }
    if (ToSpawn == 0 && Alive.IsEmpty())
    {
        bIntermission = true;
        Countdown = IntermissionSeconds;
        UE_LOG(LogTemp, Display, TEXT("ONE_ROUND_COMPLETE round=%d points=%d"), Round, Points);
    }
}
void AONEGameMode::NotifyZombieKilled(AONEZombie* Zombie, int32 Reward)
{
    // Removing from the authoritative live set makes repeated death notification idempotent.
    if (!Zombie || Alive.Remove(Zombie) == 0) return;
    Points += FMath::Max(0, Reward);
    ++Kills;
}
void AONEGameMode::PlayerDied()
{
    if (bGameOver) return;
    bGameOver = true;
    UE_LOG(LogTemp, Display, TEXT("ONE_GAME_OVER round=%d kills=%d points=%d"), Round, Kills, Points);
}
void AONEGameMode::RestartScene()
{
    UGameplayStatics::SetGamePaused(this, false);
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)),true,bSandbox?TEXT("ONESandbox=1"):TEXT(""));
}
void AONEGameMode::ToggleSandbox()
{
    bSandbox=!bSandbox;
    RestartScene();
}
AONEZombie* AONEGameMode::SpawnSandboxEnemyAt(const FVector& Location)
{
    if (!bSandbox || bGameOver || Alive.Num()>=MaximumActive) return nullptr;
    FVector Point=Location;
    if (UNavigationSystemV1* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
    {
        FNavLocation Projected;
        if (!Nav->ProjectPointToNavigation(Point,Projected,FVector(80,80,220))) return nullptr;
        Point=Projected.Location+FVector(0,0,98);
    }
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    AONEZombie* Zombie=GetWorld()->SpawnActor<AONEZombie>(ZombieClass,Point,FRotator(0,180,0),Params);
    if (Zombie) Alive.Add(Zombie);
    return Zombie;
}
void AONEGameMode::SpawnSandboxEnemies(int32 Count)
{
    if (const APawn* Player=UGameplayStatics::GetPlayerPawn(this,0))
        for (int32 I=0;I<FMath::Clamp(Count,0,6);++I)
            SpawnSandboxEnemyAt(Player->GetActorLocation()+FVector(400+(I/3)*110,(I%3-1)*110,0));
}
void AONEGameMode::RefillSandboxAmmo()
{
    if (!bSandbox) return;
    if (AONEPlayer* Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) Player->GetWeaponComponent()->RefillAllAmmo();
}
void AONEGameMode::ClearSandboxPresentation()
{
    if (!bSandbox) return;
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->ClearPresentation();
    if (auto* Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) Player->GetWeaponComponent()->ClearEjectedCases();
}
