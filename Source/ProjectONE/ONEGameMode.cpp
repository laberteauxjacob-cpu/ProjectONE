#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEPlayerController.h"
#include "ONEHUD.h"
#include "ONEAmbientAudioComponent.h"
#include "ONEZombieAudioComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEProgressionMachine.h"
#include "ONEInteractionComponent.h"
#include "ONEBloodSubsystem.h"
#include "DrawDebugHelpers.h"
#include "ONEValidation.h"
#include "ONEPresentationCheck.h"
#include "ONECombatCheck.h"
#include "ONE03MovementCheck.h"
#include "ONE03WeaponCheck.h"
#include "ONE03PresentationCheck.h"
#include "ONE03CaseCheck.h"
#include "ONE03DamageCheck.h"
#include "ONE03PhysicalityCheck.h"
#include "ONE04ProgressionCheck.h"
#include "ONE04ArsenalCheck.h"
#include "ONE04PresentationCheck.h"
#include "ONE05AimCheck.h"
#include "ONE05WeaponCheck.h"
#include "ONE05PresentationCheck.h"
#include "ONE05MotionCheck.h"
#include "ONE05UICheck.h"
#include "Misc/CommandLine.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Components/CapsuleComponent.h"
#include "Engine/TargetPoint.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Light.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "TimerManager.h"

AONEGameMode::AONEGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AONEPlayer::StaticClass();
    PlayerControllerClass = AONEPlayerController::StaticClass();
    HUDClass = AONEHUD::StaticClass();
    ZombieClass = AONEZombie::StaticClass();
    ForcedBoxReward=EONEWeaponFamily::Invalid;
    AmbientAudio=CreateDefaultSubobject<UONEAmbientAudioComponent>(TEXT("FacilityAudio"));
}
void AONEGameMode::BeginPlay()
{
    Super::BeginPlay();
    const bool bMovementCheck=FParse::Param(FCommandLine::Get(),TEXT("ONE03MovementCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONE03MovementCapture")) || FString(FCommandLine::Get()).Contains(TEXT("ONE03ManualCapture="));
    const bool bWeaponCheck=FParse::Param(FCommandLine::Get(),TEXT("ONE03WeaponCheck"));
    const bool bPresentationCheck=FParse::Param(FCommandLine::Get(),TEXT("ONE03PresentationCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONE03PresentationCapture"));
    const bool bCaseCheck=FParse::Param(FCommandLine::Get(),TEXT("ONE03CaseCheck"));
    const bool bDamageCheck=FParse::Param(FCommandLine::Get(),TEXT("ONE03DamageCheck"));
    const bool bPhysicalityCheck=FParse::Param(FCommandLine::Get(),TEXT("ONE03PhysicalityCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONE03PhysicalityCapture")) || FParse::Param(FCommandLine::Get(),TEXT("ONE03PhysicalityProfile"));
    const bool bCandidate04=FString(FCommandLine::Get()).Contains(TEXT("ONE04"));
    const bool bCandidate05=FString(FCommandLine::Get()).Contains(TEXT("ONE05"));
    bSandbox=UGameplayStatics::HasOption(OptionsString,TEXT("ONESandbox")) || FParse::Param(FCommandLine::Get(),TEXT("ONECombatCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONECompare")) || bMovementCheck || bWeaponCheck || bPresentationCheck || bCaseCheck || bDamageCheck || bPhysicalityCheck || bCandidate04 || bCandidate05;
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
    const bool bLegacyLoadout=bMovementCheck || bWeaponCheck || bPresentationCheck || bCaseCheck || bDamageCheck || bPhysicalityCheck ||
        FParse::Param(FCommandLine::Get(),TEXT("ONEValidate")) || FString(FCommandLine::Get()).Contains(TEXT("ONEBenchmark=")) ||
        FParse::Param(FCommandLine::Get(),TEXT("ONEPresentation")) || FParse::Param(FCommandLine::Get(),TEXT("ONECombatCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONECompare"));
    if (bLegacyLoadout)
    {
        // World BeginPlay order may run the mode before the pawn's component
        // initializes its ordinary starter. Install the explicit legacy fixture
        // after those initializers, before the probes' warm-up expires.
        GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this]()
        {
            if (auto* P=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0)))
            {
                auto* W=P->GetWeaponComponent(); W->GiveTestLoadout();
                UE_LOG(LogTemp,Display,TEXT("ONE_LEGACY_LOADOUT family=%d selected=%d ammo=%d/%d begun=%d"),
                    int32(W->GetDefinition().Family),W->GetEquippedIndex(),W->GetAmmoForWeapon(0),W->GetAmmoForWeapon(1),P->HasActorBegunPlay());
            }
        }));
    }
    if (FParse::Param(FCommandLine::Get(),TEXT("ONEValidate")) || FString(FCommandLine::Get()).Contains(TEXT("ONEBenchmark=")))
        GetWorld()->SpawnActor<AONEValidation>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONEPresentation"))) GetWorld()->SpawnActor<AONEPresentationCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONECombatCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONECompare"))) GetWorld()->SpawnActor<AONECombatCheck>();
    if (bMovementCheck) GetWorld()->SpawnActor<AONE03MovementCheck>();
    if (bWeaponCheck) GetWorld()->SpawnActor<AONE03WeaponCheck>();
    if (bPresentationCheck) GetWorld()->SpawnActor<AONE03PresentationCheck>();
    if (bCaseCheck) GetWorld()->SpawnActor<AONE03CaseCheck>();
    if (bDamageCheck) GetWorld()->SpawnActor<AONE03DamageCheck>();
    if (bPhysicalityCheck) GetWorld()->SpawnActor<AONE03PhysicalityCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE04ProgressionCheck"))) GetWorld()->SpawnActor<AONE04ProgressionCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE04ArsenalCheck"))) GetWorld()->SpawnActor<AONE04ArsenalCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE05AimCheck"))) GetWorld()->SpawnActor<AONE05AimCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE05WeaponCheck"))) GetWorld()->SpawnActor<AONE05WeaponCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE05UICheck"))) GetWorld()->SpawnActor<AONE05UICheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE05MotionCheck")) || FParse::Param(FCommandLine::Get(),TEXT("ONE05MotionCapture"))) GetWorld()->SpawnActor<AONE05MotionCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE05PresentationCapture")) || FParse::Param(FCommandLine::Get(),TEXT("ONE05Profile")) ||
        FString(FCommandLine::Get()).Contains(TEXT("ONE05ManualCapture="))) GetWorld()->SpawnActor<AONE05PresentationCheck>();
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE04PresentationCapture")) || FParse::Param(FCommandLine::Get(),TEXT("ONE04Profile")) ||
        FString(FCommandLine::Get()).Contains(TEXT("ONE04ManualCapture="))) GetWorld()->SpawnActor<AONE04PresentationCheck>();
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
    SurvivalSeconds+=DeltaSeconds;
    if (bSandbox)
    {
        for (auto It=Alive.CreateIterator();It;++It) if (!It->IsValid()) It.RemoveCurrent();
        const FVector Origin(-500,300,6);
        DrawDebugLine(GetWorld(),Origin,Origin+FVector(1000,0,0),FColor(64,180,173),false,-1,0,2);
        for (int32 Distance : {0,200,500,1000})
        {
            const FVector Tick=Origin+FVector(Distance,0,0);
            DrawDebugLine(GetWorld(),Tick-FVector(0,35,0),Tick+FVector(0,35,0),FColor(230,170,60),false,-1,0,2);
            // HUD projects the distance labels and excludes essential panels.
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
    if (AmbientAudio) AmbientAudio->Shutdown();
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It)
        if (auto* Audio=It->FindComponentByClass<UONEZombieAudioComponent>()) Audio->Shutdown();
    if (auto* PC=Cast<AONEPlayerController>(UGameplayStatics::GetPlayerController(this,0))) PC->GuardGameplayInput();
    for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It) It->InvalidateRun();
    if (auto* P=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) P->GetWeaponComponent()->InvalidateMachineTransactions();
    UE_LOG(LogTemp, Display, TEXT("ONE_GAME_OVER round=%d kills=%d points=%d"), Round, Kills, Points);
}
void AONEGameMode::RestartScene()
{
    if (AmbientAudio) AmbientAudio->Shutdown();
    if (auto* PC=Cast<AONEPlayerController>(UGameplayStatics::GetPlayerController(this,0))) PC->GuardGameplayInput();
    for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It) It->InvalidateRun();
    if (auto* P=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0)))
    { P->GetInteractionComponent()->Cancel(true); P->GetWeaponComponent()->InvalidateMachineTransactions(); }
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
    const bool Diagnose=FParse::Param(FCommandLine::Get(),TEXT("ONE03SpawnDiagnostics"));
    if (!bSandbox || bGameOver || Alive.Num()>=MaximumActive) return nullptr;
    const AONEPlayer* Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    UNavigationSystemV1* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!Player || !Nav || !ZombieClass) return nullptr;
    FNavLocation PlayerFloor;
    if (!Nav->ProjectPointToNavigation(Player->GetNavAgentLocation(),PlayerFloor,FVector(80,80,45)))
    { if (Diagnose) UE_LOG(LogTemp,Display,TEXT("ONE03_SPAWN no_player_floor player=%s navagent=%s"),*Player->GetActorLocation().ToString(),*Player->GetNavAgentLocation().ToString()); return nullptr; }
    if (Diagnose) UE_LOG(LogTemp,Display,TEXT("ONE03_SPAWN request=%s player=%s navagent=%s floor=%s"),*Location.ToString(),*Player->GetActorLocation().ToString(),*Player->GetNavAgentLocation().ToString(),*PlayerFloor.Location.ToString());
    int32 NavMiss=0,PathMiss=0,Blocked=0,SpawnMiss=0;
    const UCapsuleComponent* Capsule=ZombieClass.GetDefaultObject()->GetCapsuleComponent();
    const float HalfHeight=Capsule->GetScaledCapsuleHalfHeight();
    const FCollisionShape Shape=FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(),HalfHeight);
    const FVector DesiredFloor(Location.X,Location.Y,PlayerFloor.Location.Z);
    // A pawn-center query can prefer a bench's isolated top polygon. Search near
    // the player's floor height, and require a complete route before placement.
    const FVector Offsets[]={FVector::ZeroVector,
        FVector(90,0,0),FVector(-90,0,0),FVector(0,90,0),FVector(0,-90,0),
        FVector(90,90,0),FVector(-90,90,0),FVector(90,-90,0),FVector(-90,-90,0),
        FVector(180,0,0),FVector(-180,0,0),FVector(0,180,0),FVector(0,-180,0),
        FVector(180,180,0),FVector(-180,180,0),FVector(180,-180,0),FVector(-180,-180,0)};
    for (const FVector& Offset:Offsets)
    {
        FNavLocation Projected;
        if (!Nav->ProjectPointToNavigation(DesiredFloor+Offset,Projected,FVector(50,50,35))) { ++NavMiss; continue; }
        UNavigationPath* Path=UNavigationSystemV1::FindPathToLocationSynchronously(this,Projected.Location,PlayerFloor.Location);
        if (!Path || !Path->IsValid() || Path->IsPartial()) { ++PathMiss; continue; }
        const FVector Point=Projected.Location+FVector(0,0,HalfHeight+10.f);
        if (GetWorld()->OverlapBlockingTestByChannel(Point,FQuat::Identity,ECC_Pawn,Shape)) { ++Blocked; continue; }
        FActorSpawnParameters Params;
        // Do not let collision adjustment move a verified floor point onto a prop.
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding;
        if (AONEZombie* Zombie=GetWorld()->SpawnActor<AONEZombie>(ZombieClass,Point,FRotator(0,180,0),Params))
        { Alive.Add(Zombie); if (Diagnose) UE_LOG(LogTemp,Display,TEXT("ONE03_SPAWN accepted=%s"),*Point.ToString()); return Zombie; }
        ++SpawnMiss;
    }
    if (Diagnose) UE_LOG(LogTemp,Display,TEXT("ONE03_SPAWN rejected nav=%d path=%d overlap=%d spawn=%d"),NavMiss,PathMiss,Blocked,SpawnMiss);
    return nullptr;
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
bool AONEGameMode::TrySpendPoints(int32 Cost,uint64 Receipt)
{
    if (bGameOver || Cost<=0 || Receipt==0 || Receipt>NextMachineReceipt || MachineReceipts.Contains(Receipt) || Points<Cost) return false;
    MachineReceipts.Add(Receipt,Cost); Points-=Cost; return true;
}
bool AONEGameMode::RefundPointsOnce(uint64 Receipt)
{
    int32* Paid=MachineReceipts.Find(Receipt);
    if (bGameOver || !Paid || *Paid<=0) return false;
    Points=int32(FMath::Min<int64>(MAX_int32,int64(Points)+*Paid)); *Paid=0; return true;
}
void AONEGameMode::CancelUnacceptedMachineActions(AONEPlayer* P)
{
    for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It) It->CancelUnacceptedAction(P);
}
void AONEGameMode::GrantSandboxPoints()
{
    if (!bSandbox || bGameOver) return;
    const int32 Added=FMath::Min(10000,MAX_int32-Points);
    Points+=Added; SandboxGrantedPoints=int32(FMath::Min<int64>(MAX_int32,int64(SandboxGrantedPoints)+Added));
    UE_LOG(LogTemp,Display,TEXT("ONE04_DEV_POINTS added=%d total=%d"),Added,Points);
}
void AONEGameMode::SetForcedBoxReward(EONEWeaponFamily Family)
{
    if (!bSandbox || bGameOver) return;
    ForcedBoxReward=Family;
    UE_LOG(LogTemp,Display,TEXT("ONE04_DEV_NEXT_BOX family=%d"),int32(Family));
}
EONEWeaponFamily AONEGameMode::ConsumeForcedBoxReward()
{
    const EONEWeaponFamily F=bSandbox ? ForcedBoxReward : EONEWeaponFamily::Invalid;
    ForcedBoxReward=EONEWeaponFamily::Invalid; return F;
}
FString AONEGameMode::GetForcedBoxRewardLabel() const
{
    switch (ForcedBoxReward)
    {
        case EONEWeaponFamily::Pistol: return TEXT("M1911");
        case EONEWeaponFamily::Carbine: return TEXT("M4A1");
        case EONEWeaponFamily::Shotgun: return TEXT("Remington 870");
        default: return TEXT("RANDOM");
    }
}
void AONEGameMode::SetSandboxDimLighting(bool Dim)
{
    if (!bSandbox) return;
    if (SandboxLightIntensities.IsEmpty())
        for (TActorIterator<ALight> It(GetWorld());It;++It)
            if (auto* Light=It->GetLightComponent()) SandboxLightIntensities.Add(Light,Light->Intensity);
    bDimLighting=Dim;
    // Only authored room-light actors participate. The weapon's attached light
    // keeps its own short envelope, and the map's fixed exposure is unchanged.
    for (const auto& Entry:SandboxLightIntensities)
        if (auto* Light=Entry.Key.Get())
        {
            // Retain enough room illumination to judge hands, cases and nearby
            // surfaces under the existing fixed exposure, not a black-screen test.
            Light->SetIntensity(Entry.Value*(Dim?.18f:1.f));
        }
}
