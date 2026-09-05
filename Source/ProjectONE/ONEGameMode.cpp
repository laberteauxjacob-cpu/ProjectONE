#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEPlayerController.h"
#include "ONEHUD.h"
#include "ONEWeaponComponent.h"
#include "ONEValidation.h"
#include "ONEPresentationCheck.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "Engine/TargetPoint.h"
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
    for (TActorIterator<ATargetPoint> It(GetWorld()); It; ++It)
        if (It->ActorHasTag("ONE_Spawn")) SpawnLocations.Add(It->GetActorLocation());
    if (SpawnLocations.IsEmpty()) SpawnLocations = { FVector(-950,-700,100), FVector(950,-700,100), FVector(-950,650,100), FVector(950,650,100) };
    if (FParse::Param(FCommandLine::Get(),TEXT("ONEValidate")) || FString(FCommandLine::Get()).Contains(TEXT("ONEBenchmark=")))
        GetWorld()->SpawnActor<AONEValidation>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONEPresentation"))) GetWorld()->SpawnActor<AONEPresentationCheck>();
}
void AONEGameMode::StartRound()
{
    if (Round > 0)
        if (AONEPlayer* Player = Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) Player->GetWeaponComponent()->AddReserveAmmo(48);
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
    UGameplayStatics::OpenLevel(this, FName(*UGameplayStatics::GetCurrentLevelName(this, true)));
}
