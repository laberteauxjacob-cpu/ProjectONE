#include "ONE06SurvivalCheck.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEGameMode.h"
#include "ONEHUD.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEPowerUpComponent.h"
#include "ONEPowerUpPickup.h"
#include "ONE06PickupVisualComponent.h"
#include "ONE06CaptureComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

AONE06SurvivalCheck::AONE06SurvivalCheck()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    Capture=CreateDefaultSubobject<UONE06CaptureComponent>(TEXT("SurvivalCapture"));
}
void AONE06SurvivalCheck::BeginPlay()
{
    Super::BeginPlay(); StartedReal=FPlatformTime::Seconds();
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate06/SurvivalCheck")/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    IFileManager::Get().MakeDirectory(*Folder,true);
    ChecksCsv=TEXT("index,world_seconds,pass,label\n");
    bCaptureRequested=FParse::Param(FCommandLine::Get(),TEXT("ONE06Capture"));
    if (bCaptureRequested)
    {
        const bool Started=Capture->BeginCapture(FPaths::ConvertRelativePathToFull(Folder/TEXT("Media")));
        Check(Started,TEXT("Requested engine viewport and master-audio capture started"));
        if (!Started) { Finish(false); return; }
        Capture->SetPhaseLabel(TEXT("Setup: production player and survival components"));
    }
}
void AONE06SurvivalCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    FString Escaped=Label; Escaped.ReplaceInline(TEXT("\""),TEXT("\"\""));
    ChecksCsv+=FString::Printf(TEXT("%d,%.6f,%d,\"%s\"\n"),Checks,GetWorld()->GetTimeSeconds(),Pass,*Escaped);
    UE_LOG(LogTemp,Display,TEXT("ONE06_SURVIVAL %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE06SurvivalCheck::Enter(int32 Next)
{
    Phase=Next; PhaseStarted=GetWorld()->GetTimeSeconds(); bPreDelayChecked=false;
    if (!bCaptureRequested) return;
    FString Label;
    switch (Next)
    {
        case 1: Label=TEXT("First accepted attack: 15-second recovery delay"); break;
        case 2: Label=TEXT("Repeated accepted attack: recovery delay resets"); break;
        case 3: Label=TEXT("200 HP maximum: 20 HP per second recovery"); break;
        case 5: Label=TEXT("Actual pistol discharge before Max Ammo collection"); break;
        case 6: Label=FString::Printf(TEXT("Pickup collection setup %d"),CollectIndex+1); break;
        case 7: Label=FString::Printf(TEXT("Standing overlap collection %d and effect"),CollectIndex+1); break;
        case 8: Label=TEXT("Real gameplay pause freezes survival and pickup clocks"); break;
        case 9: Label=TEXT("30-second world lifetime and timed-effect expiry"); break;
        case 10: Label=TEXT("Cap and award checks; final overlap collection"); break;
        case 11: Label=TEXT("Death invalidates recovery modifiers and pickup ownership"); break;
        case 12: Label=TEXT("Collected Insta-Kill and Double Points: ordinary torso cursor setup"); break;
        case 13: Label=TEXT("Actual pistol trace against a healthy 112 HP infected with both pickups active"); break;
        case 14: Label=TEXT("Collected effects produce one body kill and exactly 220 points"); break;
        default: Label=FString::Printf(TEXT("Survival fixture phase %d"),Next); break;
    }
    Capture->SetPhaseLabel(Label);
}
void AONE06SurvivalCheck::Observe()
{
    if (!Health || !Powers) return;
    auto Row=MakeShared<FJsonObject>();
    Row->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds()); Row->SetNumberField(TEXT("phase"),Phase);
    Row->SetNumberField(TEXT("health"),Health->Health); Row->SetNumberField(TEXT("maximum"),Health->MaxHealth);
    Row->SetNumberField(TEXT("regeneration_delay_remaining"),Health->GetRegenerationDelayRemaining());
    Row->SetBoolField(TEXT("regenerating"),Health->IsRegenerating()); Row->SetBoolField(TEXT("paused"),UGameplayStatics::IsGamePaused(this));
    Row->SetNumberField(TEXT("insta_seconds"),Powers->GetRemainingSeconds(EONEPowerUpType::InstaKill));
    Row->SetNumberField(TEXT("double_seconds"),Powers->GetRemainingSeconds(EONEPowerUpType::DoublePoints));
    Row->SetNumberField(TEXT("active_drops"),Powers->GetActiveDropCount());
    Row->SetNumberField(TEXT("points"),Mode->GetPoints()); Row->SetNumberField(TEXT("pickup_notifications"),double(Powers->GetPickupNotificationSerial()));
    Row->SetNumberField(TEXT("total_weapon_shots"),Player->GetWeaponComponent()->GetTotalShotsFired());
    if (TimedTarget.IsValid())
    {
        Row->SetNumberField(TEXT("timed_target_health"),TimedTarget->GetHealth());
        Row->SetBoolField(TEXT("timed_target_dead"),TimedTarget->IsDead());
        Row->SetNumberField(TEXT("last_shot_new_kills"),Player->GetWeaponComponent()->GetLastShotNewKillCount());
    }
    const auto& Stats=Powers->GetDropStats();
    Row->SetNumberField(TEXT("eligible_deaths"),double(Stats.EligibleDeaths)); Row->SetNumberField(TEXT("passed_drop_rolls"),double(Stats.PassedRolls));
    Row->SetNumberField(TEXT("chance_misses"),double(Stats.ChanceMisses)); Row->SetNumberField(TEXT("natural_cap_suppressed"),double(Stats.CapSuppressed));
    Row->SetNumberField(TEXT("forced_attempts"),double(Stats.ForcedAttempts)); Row->SetNumberField(TEXT("forced_failures"),double(Stats.ForcedFailures));
    Row->SetNumberField(TEXT("expired"),double(Stats.Expired)); Row->SetNumberField(TEXT("collected"),double(Stats.Collected));
    Row->SetNumberField(TEXT("tracked_world_lifetime"),Expiring.IsValid()?Expiring->GetRemainingLifetime():-1.f);
    if (const auto* PC=Cast<APlayerController>(Player->GetController()))
        if (const auto* Hud=Cast<AONEHUD>(PC->GetHUD()))
        { Row->SetNumberField(TEXT("hud_health_edge"),Hud->GetHealthEdgeSeverity()); Row->SetNumberField(TEXT("hud_damage_pulse"),Hud->GetDamagePulseStrength()); }
    Observations.Add(MakeShared<FJsonValueObject>(Row));
}
AONEPowerUpPickup* AONE06SurvivalCheck::Spawn(EONEPowerUpType Type,const FVector& Near)
{
    const auto Result=Powers->ForceDrop(Type,Near);
    Check(Result==EONEPowerUpDropResult::Spawned,FString::Printf(TEXT("Forced type%d finds reachable clear ground through production placement"),int32(Type)));
    if (Result!=EONEPowerUpDropResult::Spawned) return nullptr;
    AONEPowerUpPickup* Found=nullptr;
    for (TActorIterator<AONEPowerUpPickup> It(GetWorld());It;++It)
        if (It->GetType()==Type && It->IsAvailable() && It->GetRunId()==Powers->GetRunId()) Found=*It;
    Check(Found!=nullptr,TEXT("Spawned pickup is available in this exact run"));
    if (Found) Check(Found->Visual && Found->Visual->IsConfigured(),TEXT("Pickup has its real emblem glow and collection asset"));
    return Found;
}
void AONE06SurvivalCheck::PositionAt(AONEPowerUpPickup* Pickup,float HorizontalOffset)
{
    Player->GetCharacterMovement()->StopMovementImmediately();
    const float Half=Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Player->SetActorLocation(Pickup->GetActorLocation()+FVector(HorizontalOffset,0,Half-45.f+2.f),false,nullptr,ETeleportType::TeleportPhysics);
    Player->GetCapsuleComponent()->UpdateOverlaps(); Pickup->Collection->UpdateOverlaps();
}
void AONE06SurvivalCheck::StartCollection()
{
    const auto Type=CollectIndex==1 ? EONEPowerUpType::DoublePoints : CollectIndex==2 ? EONEPowerUpType::MaxAmmo : EONEPowerUpType::InstaKill;
    Player->SetActorLocation(OriginalPosition,false,nullptr,ETeleportType::TeleportPhysics);
    Current=Spawn(Type,OriginalPosition+FVector(260,0,0));
    if (!Current.IsValid()) { Finish(false); return; }
    NotificationBefore=Powers->GetPickupNotificationSerial();
    if (CollectIndex==0)
    {
        PositionAt(Current.Get(),70.f);
        AActor* Obstacle=GetWorld()->SpawnActor<AActor>(); Wall=Obstacle;
        auto* Box=NewObject<UBoxComponent>(Obstacle); Obstacle->SetRootComponent(Box);
        Box->SetBoxExtent(FVector(3,60,90)); Box->SetCollisionObjectType(ECC_WorldStatic);
        Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Box->SetCollisionResponseToAllChannels(ECR_Block);
        Box->RegisterComponent(); Obstacle->SetActorLocation(Current->GetActorLocation()+FVector(35,0,25));
        Check(Current->Collection->IsOverlappingComponent(Player->GetCapsuleComponent()),TEXT("Wall fixture starts from an actual player-capsule overlap"));
        Check(!Current->TryCollect(Player) && Powers->GetPickupNotificationSerial()==NotificationBefore,TEXT("Intervening real world-static wall blocks collection without notification"));
        Obstacle->Destroy(); Wall.Reset();
    }
    PositionAt(Current.Get());
    Enter(7); // no F and no direct successful TryCollect: ordinary actor polling collects
}
void AONE06SurvivalCheck::AwardFixture(bool Headshot)
{
    AONEZombie* Zombie=Mode->SpawnSandboxEnemyAt(OriginalPosition+FVector(520,Headshot?220.f:-220.f,0));
    Check(Zombie!=nullptr,TEXT("Scoring fixture uses a registered production infected on reachable ground"));
    if (!Zombie) { Finish(false); return; }
    Zombie->SetActorTickEnabled(false); Zombie->GetCharacterMovement()->StopMovementImmediately();
    const auto Context=Mode->BeginCombatDischarge(); const int32 Before=Mode->GetPoints(),Kills=Mode->GetKills();
    const uint64 Deaths=Powers->GetDropStats().EligibleDeaths,Suppressed=Powers->GetDropStats().CapSuppressed;
    FONEWeaponDamagePacket Packet; Packet.ShotId=uint64(6060000+Checks);
    Packet.Get(Headshot ? EONEHitRegion::Head : EONEHitRegion::Body).AddPellet(
        Zombie->GetHealthComponent()->Health,0.f,Zombie->GetActorLocation()+FVector(0,0,Headshot?65.f:20.f),FVector::ForwardVector,-FVector::ForwardVector,NAME_None);
    Packet.Finalize();
    const auto Outcome=Zombie->ReceiveWeaponDamageOutcome(Packet);
    Check(Outcome==EONEWeaponHitOutcome::NewKill,TEXT("One explicit aggregated damage packet returns a real new-kill outcome"));
    const int32 Expected=Headshot?130:110;
    Check(Mode->RecordCombatAward(Context,Zombie,Outcome,Headshot)==Expected && Mode->GetPoints()==Before+Expected,
        FString::Printf(TEXT("Actual registered lethal %s event awards exactly%d"),Headshot?TEXT("head"):TEXT("body"),Expected));
    Check(Mode->RecordCombatAward(Context,Zombie,Outcome,Headshot)==0,TEXT("Duplicate victim/discharge callback cannot duplicate impact or kill points"));
    Mode->NotifyZombieKilled(Zombie,100);
    Check(Mode->GetKills()==Kills+1 && Powers->GetDropStats().EligibleDeaths==Deaths+1,TEXT("Repeated death callback cannot duplicate kill registration or drop rolls"));
    Check(Powers->GetDropStats().CapSuppressed==Suppressed+1,TEXT("Passed100%-fixture roll at cap is recorded as cap suppression not chance miss"));
    ++Packet.ShotId;
    const auto Corpse=Zombie->ReceiveWeaponDamageOutcome(Packet);
    Check(Corpse==EONEWeaponHitOutcome::CorpseHit && Mode->RecordCombatAward(Context,Zombie,Corpse,false)==0,TEXT("Actual corpse transaction cannot award more points"));
    Mode->EndCombatDischarge(Context);
}
void AONE06SurvivalCheck::StartTimedCombat()
{
    Player->ReleaseHeldInputs();
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(OriginalPosition,false,nullptr,ETeleportType::TeleportPhysics);
    auto* Controller=Cast<APlayerController>(Player->GetController());
    Check(Controller!=nullptr,TEXT("Collected-effect combat fixture has an ordinary player controller"));
    if (!Controller) { Finish(false); return; }
    for (const FKey& Key:{EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::LeftControl})
        Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Key,IE_Released,0.f));
    // Explicit fixed standing target; health, regional damage, collision, death,
    // weapon tracing and scoring remain production behavior.
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    TimedTarget=GetWorld()->SpawnActor<AONEZombie>(OriginalPosition+FVector(250,0,0),FRotator(0,180,0),Params);
    Check(TimedTarget.IsValid(),TEXT("Declared standing target for collected-effect trace spawned"));
    if (!TimedTarget.IsValid()) { Finish(false); return; }
    Mode->RegisterZombie(TimedTarget.Get());
    TimedTarget->SetActorTickEnabled(false); TimedTarget->GetCharacterMovement()->DisableMovement();
    if (auto* AI=Cast<AAIController>(TimedTarget->GetController())) AI->StopMovement();
    TimedTarget->GetMesh()->TickAnimation(0.f,false); TimedTarget->GetMesh()->RefreshBoneTransforms(); TimedTarget->GetMesh()->bPauseAnims=true;
    const bool Healthy=TimedTarget->GetHealth()==112.f && TimedTarget->GetHealthComponent()->MaxHealth==112.f &&
        !TimedTarget->IsDead() && TimedTarget->GetSeverCount()==0;
    Check(Healthy,TEXT("Collected-effect target retains full ordinary 112 HP and intact regions"));
    if (!Healthy) { Finish(false); return; }
    Player->SetAimOverride(false,FVector::ZeroVector);
    FVector Point=TimedTarget->GetActorLocation();
    Point.Z=OriginalPosition.Z-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+Player->TorsoAimHeight;
    FVector2D Screen;
    const bool Projected=Player->ProjectLogicalWorld(Point,Screen);
    Check(Projected,TEXT("Collected-effect shot uses ordinary logical torso cursor projection without an aim override"));
    if (!Projected) { Finish(false); return; }
    Controller->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
    Enter(12);
}
void AONE06SurvivalCheck::StartPauseFixture()
{
    Player->SetActorLocation(OriginalPosition,false,nullptr,ETeleportType::TeleportPhysics);
    Expiring=Spawn(EONEPowerUpType::MaxAmmo,OriginalPosition+FVector(430,0,0));
    if (!Expiring.IsValid()) { Finish(false); return; }
    DropAt=GetWorld()->GetTimeSeconds(); ExpiredBefore=Powers->GetDropStats().Expired;
    Health->ApplyDamage(20.f); SnapshotHealth=Health->Health;
    PausedDelay=Health->GetRegenerationDelayRemaining(); PausedInsta=Powers->GetRemainingSeconds(EONEPowerUpType::InstaKill);
    PausedDouble=Powers->GetRemainingSeconds(EONEPowerUpType::DoublePoints); PausedLifetime=Expiring->GetRemainingLifetime();
    Check(UGameplayStatics::SetGamePaused(this,true),TEXT("Engine accepts real gameplay pause"));
    PausedReal=FPlatformTime::Seconds(); PausedWorld=GetWorld()->GetTimeSeconds(); Enter(8);
}
void AONE06SurvivalCheck::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const double Real=FPlatformTime::Seconds(),Now=GetWorld()->GetTimeSeconds();
    if (bFinished) { if (Real-FinishedReal>1.) FPlatformMisc::RequestExit(false); return; }
    if (bFinishing) { Finish(bComplete); return; }
    if (bCaptureRequested && Capture->HasFailed()) { Finish(false); return; }
    if (Real-StartedReal>140. || Now>95.) { Check(false,TEXT("Survival integration exceeded bounded runtime")); Finish(false); return; }
    if (!Player)
    {
        Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0)); Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
        Health=Player?Player->GetHealthComponent():nullptr; Powers=Mode?Mode->GetPowerUps():nullptr;
    }
    if (!Player || !Mode || !Health || !Powers)
    { if (Real-StartedReal>8.) { Check(false,TEXT("Production player health GameMode and power-up component exist")); Finish(false); } return; }
    if (Phase==0)
    {
        if (Now<2.) return;
        Check(Mode->IsSandbox(),TEXT("Dedicated opt-in fixture uses sandbox gameplay"));
        if (!bCaptureRequested) Check(!Capture->IsRecording() && Capture->GetFrameCount()==0,TEXT("Numeric mode has no media recording or frames"));
        Check(Health->RegenerationDelay==15.f && Health->RegenerationFractionPerSecond==.1f,TEXT("Actual player recovery defaults are15s and10% current max per second"));
        Check(Powers->TotalDropChance==.01f && Powers->TypeWeights.Equals(FVector(1,1,1)) && Powers->WorldLifetime==30.f && Powers->TimedEffectDuration==30.f,
            TEXT("Actual drop defaults are1% total equal weights30s world and effect durations"));
        OriginalPosition=Player->GetActorLocation(); Player->GetCharacterMovement()->StopMovementImmediately();
        Health->SetMaximumHealth(100.f); Health->Restore(); Player->ReceiveAttack(40.f,OriginalPosition-FVector(100,0,0));
        Check(Health->Health==60.f,TEXT("Production player accepts one health-reducing attack"));
        const float ProtectedDelay=Health->GetRegenerationDelayRemaining();
        Player->ReceiveAttack(19.f,OriginalPosition-FVector(100,0,0));
        Check(Health->Health==60.f && ProtectedDelay==15.f && Health->GetRegenerationDelayRemaining()==ProtectedDelay,
            TEXT("Immediate protected second attack preserves health and the original 15-second recovery deadline"));
        DamageAt=Now; Enter(1); return;
    }
    if (Observations.IsEmpty() || Now-Observations.Last()->AsObject()->GetNumberField(TEXT("world_seconds"))>=.1 || Phase==8) Observe();
    const double Age=Now-PhaseStarted;
    if (Phase==1 || Phase==2)
    {
        const float StartHealth=Phase==1?60.f:SnapshotHealth;
        if (!bPreDelayChecked && Now-DamageAt>=14.)
        {
            Check(Now-DamageAt<15. && FMath::IsNearlyEqual(Health->Health,StartHealth,.01f),TEXT("Observed actual player health unchanged before full15s delay"));
            const auto* PC=Cast<APlayerController>(Player->GetController()); const auto* Hud=PC?Cast<AONEHUD>(PC->GetHUD()):nullptr;
            Check(Hud && FMath::Abs(Hud->GetHealthEdgeSeverity()-Health->GetMissingHealthFraction())<.05f,
                TEXT("Rendered persistent injury edge follows current missing-health fraction"));
            if (Phase==2)
            { Check(!Health->ApplyDamage(0.f),TEXT("Zero damage is rejected by live health")); Check(!Health->ApplyDamage(-1.f),TEXT("Negative damage is rejected by live health")); }
            bPreDelayChecked=true;
        }
        if (Now-DamageAt<15.25) return;
        const float Expected=StartHealth+float(Now-DamageAt-15.)*10.f;
        Check(bPreDelayChecked && Health->Health>StartHealth && FMath::Abs(Health->Health-Expected)<FMath::Max(1.f,DeltaSeconds*20.f),
            TEXT("Real gameplay-time recovery starts at15s within one frame tolerance"));
        if (Phase==1)
        {
            const float Before=Health->Health; Player->ReceiveAttack(19.f,Player->GetActorLocation()-FVector(100,0,0));
            Check(FMath::IsNearlyEqual(Health->Health,Before-19.f,.001f) && !Health->IsRegenerating(),TEXT("New accepted attack interrupts recovery and restarts delay"));
            SnapshotHealth=Health->Health; DamageAt=Now; Enter(2);
        }
        else { Health->SetMaximumHealth(200.f); SnapshotHealth=Health->Health; Enter(3); }
    }
    else if (Phase==3 && Age>=.6)
    {
        Check(Health->Health>SnapshotHealth+9.f && FMath::Abs(Health->Health-SnapshotHealth-float(Age)*20.f)<FMath::Max(1.f,DeltaSeconds*40.f),
            TEXT("Actual player recovery uses200 maximum at20HP per gameplay second"));
        Health->SetMaximumHealth(100.f); Health->Restore(); CollectIndex=0; Enter(6);
    }
    else if (Phase==6)
    {
        if (CollectIndex==2)
        {
            ShotsBefore=Player->GetWeaponComponent()->GetTotalShotsFired();
            auto* Controller=Cast<APlayerController>(Player->GetController());
            Check(Controller!=nullptr,TEXT("Max Ammo firing fixture has an ordinary player controller"));
            if (!Controller) { Finish(false); return; }
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1.f));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0.f));
            Enter(5);
        }
        else StartCollection();
    }
    else if (Phase==5 && Age>=.2)
    {
        auto* Weapon=Player->GetWeaponComponent();
        Check(Weapon->GetTotalShotsFired()==ShotsBefore+1 && Weapon->GetAmmo()<Weapon->GetDefinition().Capacity,TEXT("Max Ammo fixture first discharges a real eligible pistol round"));
        CasesBefore=Weapon->GetEjectionCount(); MagazinesBefore=Weapon->GetMagazineDropCount(); StartCollection();
    }
    else if (Phase==7 && Age>=.15)
    {
        Check(Current.IsValid() && !Current->IsAvailable() && Powers->GetPickupNotificationSerial()==NotificationBefore+1,
            TEXT("Standing capsule overlap automatically collects once without F"));
        if (!Current.IsValid() || Current->IsAvailable()) { Finish(false); return; }
        Check(!Current->TryCollect(Player),TEXT("Repeated collection attempt is rejected"));
        const auto Type=Current->GetType();
        Check(Powers->GetLastCollectedType()==Type && Current->Visual->GetCollectionCueCount()==1,TEXT("Collection emits one matching notification and one cue"));
        const auto* PC=Cast<APlayerController>(Player->GetController()); const auto* Hud=PC?Cast<AONEHUD>(PC->GetHUD()):nullptr;
        Check(Hud && Hud->GetVisiblePowerUpTimers()==(CollectIndex==0?1:2),TEXT("Rendered compact HUD shows exactly the active timed modifiers"));
        if (Type==EONEPowerUpType::MaxAmmo)
        {
            auto* Weapon=Player->GetWeaponComponent(); bool Full=true;
            for (int32 Slot=0;Slot<Weapon->GetWeaponCount();++Slot)
                if (const auto* Def=Weapon->GetDefinitionForWeapon(Slot))
                    if (const auto* State=Weapon->GetSlotState(Slot); State && State->Status==EONEWeaponSlotStatus::Available)
                        Full=Full && Weapon->GetAmmoForWeapon(Slot)==Def->Capacity && Weapon->GetReserveAmmoForWeapon(Slot)==Def->ReserveLimit;
            Check(Full && Weapon->GetTotalShotsFired()==ShotsBefore+1 && Weapon->GetEjectionCount()==CasesBefore && Weapon->GetMagazineDropCount()==MagazinesBefore,
                TEXT("Actual Max Ammo refills available effective capacities without extra fire case or magazine events"));
        }
        else Check(Powers->GetRemainingSeconds(Type)>29.f && Powers->GetRemainingSeconds(Type)<=30.f,
            CollectIndex==3?TEXT("Timed recollection refreshes to30s without stacking"):TEXT("Collected timed effect starts its30s duration"));
        ++CollectIndex;
        if (CollectIndex<4) Enter(6);
        else
        {
            Check(Powers->IsActive(EONEPowerUpType::InstaKill) && Powers->IsActive(EONEPowerUpType::DoublePoints),TEXT("Different timed modifiers coexist on the ordinary GameMode"));
            StartTimedCombat();
        }
    }
    else if (Phase==12 && Age>=.4)
    {
        auto* Weapon=Player->GetWeaponComponent();
        if (!Weapon->CanFire())
        { if (Age>2.) { Check(false,TEXT("Collected-effect pistol becomes eligible within two seconds")); Finish(false); } return; }
        const bool Ready=TimedTarget.IsValid() && TimedTarget->GetHealth()==112.f &&
            Weapon->GetDefinition().Family==EONEWeaponFamily::Pistol && !Weapon->GetDefinition().bUpgraded &&
            Player->GetAimHeightLabel()==TEXT("Torso") && Powers->IsActive(EONEPowerUpType::InstaKill) && Powers->IsActive(EONEPowerUpType::DoublePoints);
        Check(Ready,TEXT("Full-health target and ordinary base pistol are ready while both collected effects remain active"));
        if (!Ready) { Finish(false); return; }
        ShotsBefore=Weapon->GetTotalShotsFired(); TimedPointsBefore=Mode->GetPoints(); TimedKillsBefore=Mode->GetKills();
        CastChecked<APlayerController>(Player->GetController())->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Pressed,1.f));
        Enter(13);
    }
    else if (Phase==13)
    {
        auto* Weapon=Player->GetWeaponComponent();
        if (Weapon->GetTotalShotsFired()==ShotsBefore)
        { if (Age>2.) { Check(false,TEXT("Collected-effect LMB press commits an actual shot within two seconds")); Finish(false); } return; }
        CastChecked<APlayerController>(Player->GetController())->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,IE_Released,0.f));
        const auto& Paths=Weapon->GetLastProjectilePaths();
        const bool Contact=Paths.Num()==1 && Paths[0].Contacts.Num()==1 &&
            Paths[0].Contacts[0].Victim.Get()==TimedTarget.Get() && !Paths[0].Contacts[0].bCorpse &&
            Paths[0].Contacts[0].Region==EONEHitRegion::Body && Paths[0].Contacts[0].Damage>0.f && Paths[0].Contacts[0].Damage<112.f;
        Check(Contact,TEXT("Actual pistol trace has one body-region target contact with raw damage below 112 HP"));
        Check(Weapon->GetTotalShotsFired()==ShotsBefore+1 && Weapon->GetLastShotNewKillCount()==1 &&
            TimedTarget.IsValid() && TimedTarget->IsDead() && !TimedTarget->WasLastKillHeadshot() && Mode->GetKills()==TimedKillsBefore+1,
            TEXT("Collected Insta-Kill turns one ordinary traced pistol impact into exactly one body kill"));
        Check(Mode->GetPoints()==TimedPointsBefore+220,TEXT("Collected Double Points awards exactly 220 for the traced Insta-Kill body kill"));
        Observe(); Enter(14);
    }
    else if (Phase==14 && Age>=.45)
    {
        Check(Player->GetWeaponComponent()->GetTotalShotsFired()==ShotsBefore+1 && Mode->GetPoints()==TimedPointsBefore+220,
            TEXT("Released traced pistol shot creates no extra discharge or duplicate doubled award"));
        if (TimedTarget.IsValid()) TimedTarget->Destroy(); TimedTarget.Reset();
        // The real death keeps its ordinary 1% roll. Remove any resulting world
        // drop explicitly before the following isolated single-expiry fixture.
        int32 IncidentalDrops=0;
        for (TActorIterator<AONEPowerUpPickup> It(GetWorld());It;++It)
            if (It->IsAvailable()) { It->Destroy(); ++IncidentalDrops; }
        UE_LOG(LogTemp,Display,TEXT("ONE06_SURVIVAL_FIXTURE_CLEANUP natural_drops_after_traced_kill=%d"),IncidentalDrops);
        StartPauseFixture();
    }
    else if (Phase==8 && Real-PausedReal>=.65)
    {
        Check(UGameplayStatics::IsGamePaused(this) && FMath::Abs(Now-PausedWorld)<.001 && Health->Health==SnapshotHealth &&
            Health->GetRegenerationDelayRemaining()==PausedDelay && Powers->GetRemainingSeconds(EONEPowerUpType::InstaKill)==PausedInsta &&
            Powers->GetRemainingSeconds(EONEPowerUpType::DoublePoints)==PausedDouble && Expiring.IsValid() && Expiring->GetRemainingLifetime()==PausedLifetime,
            TEXT("Actual engine pause freezes world recovery modifier and pickup clocks"));
        Check(UGameplayStatics::SetGamePaused(this,false),TEXT("Engine resumes from fixture pause")); Enter(9);
    }
    else if (Phase==9)
    {
        if (!bPreDelayChecked && Now-DropAt>=29.)
        { Check(Now-DropAt<30. && Expiring.IsValid() && Expiring->IsAvailable(),TEXT("Uncollected pickup remains available immediately before30s deadline")); bPreDelayChecked=true; }
        if (Now-DropAt<30.3) return;
        Check(bPreDelayChecked && !Expiring.IsValid() && Powers->GetDropStats().Expired==ExpiredBefore+1,TEXT("World pickup expires exactly once after its30s gameplay lifetime"));
        Check(!Powers->IsActive(EONEPowerUpType::InstaKill) && !Powers->IsActive(EONEPowerUpType::DoublePoints),TEXT("Both actual timed effects expire without permanent modifiers"));
        Powers->MaximumWorldDrops=2; Powers->TotalDropChance=1.f;
        Current=Spawn(EONEPowerUpType::InstaKill,OriginalPosition+FVector(230,120,0));
        DeadRejected=Spawn(EONEPowerUpType::MaxAmmo,OriginalPosition+FVector(430,-120,0));
        if (!Current.IsValid() || !DeadRejected.IsValid()) { Finish(false); return; }
        const uint64 ForcedFailures=Powers->GetDropStats().ForcedFailures;
        Check(Powers->ForceDrop(EONEPowerUpType::DoublePoints,OriginalPosition)==EONEPowerUpDropResult::CapSuppressed &&
            Powers->GetDropStats().ForcedFailures==ForcedFailures+1 && Powers->GetActiveDropCount()==2,
            TEXT("Forced cap suppression stays separate from normal probability counters"));
        AwardFixture(false); if (bFinished || bFinishing) return;
        AwardFixture(true); if (bFinished || bFinishing) return;
        Powers->TotalDropChance=.01f;
        NotificationBefore=Powers->GetPickupNotificationSerial(); PositionAt(Current.Get()); Enter(10);
    }
    else if (Phase==10 && Age>=.15)
    {
        Check(Powers->GetPickupNotificationSerial()==NotificationBefore+1 && Powers->IsActive(EONEPowerUpType::InstaKill),TEXT("Final lifecycle fixture has an actual active collected modifier"));
        BeforeDeathContext=Mode->BeginCombatDischarge(); PositionAt(DeadRejected.Get());
        Health->ApplyDamage(Health->Health);
        Check(Player->IsDead() && !DeadRejected->TryCollect(Player),TEXT("A dead player cannot collect an otherwise overlapping reachable pickup"));
        SnapshotHealth=Health->Health; Mode->PlayerDied();
        Check(!Powers->GetRunId().IsValid() && Powers->GetActiveDropCount()==0 && !Powers->IsActive(EONEPowerUpType::InstaKill) && !Mode->BeginCombatDischarge().IsValid(),
            TEXT("Shared death/reset invalidation clears run modifiers drops and new combat receipts"));
        Enter(11);
    }
    else if (Phase==11 && Age>=.3)
    {
        Check(Health->Health==0.f && !Health->IsRegenerating(),TEXT("Real player remains dead without ordinary recovery"));
        const auto* PC=Cast<APlayerController>(Player->GetController()); const auto* Hud=PC?Cast<AONEHUD>(PC->GetHUD()):nullptr;
        Check(Hud && Hud->GetHealthEdgeSeverity()==0.f,TEXT("Death presentation clears the persistent gameplay injury edge"));
        int32 Remaining=0; for (TActorIterator<AONEPowerUpPickup> It(GetWorld());It;++It) if (!It->IsActorBeingDestroyed()) ++Remaining;
        Check(Remaining==0,TEXT("Lifecycle cleanup destroys available pickups and collected cue tails"));
        Finish(true);
    }
}
void AONE06SurvivalCheck::Finish(bool Complete)
{
    if (bFinished) return;
    if (!bFinishing)
    {
        bFinishing=true; bComplete=Complete;
        if (GetWorld() && UGameplayStatics::IsGamePaused(this)) UGameplayStatics::SetGamePaused(this,false);
        if (Wall.IsValid()) Wall->Destroy();
        if (TimedTarget.IsValid()) TimedTarget->Destroy();
        if (Player) Player->ReleaseHeldInputs();
    }
    else bComplete=bComplete && Complete;
    if (bCaptureRequested)
    {
        const bool Captured=Capture->FinishCaptureAndWait();
        if (!Captured && !Capture->HasFailed() && !bEndingPlay) return;
        const bool Valid=Captured && !Capture->HasFailed() && Capture->GetFrameCount()>0;
        Check(Valid,Valid?TEXT("Full viewport frames drained and master WAV finalized with real audio tail"):
            FString::Printf(TEXT("Requested capture failed or was interrupted: %s"),*Capture->GetFailureReason()));
    }
    bFinished=true; bFinishing=false; FinishedReal=FPlatformTime::Seconds();
    const int32 Frames=bCaptureRequested?Capture->GetFrameCount():0;
    auto Root=MakeShared<FJsonObject>(); Root->SetBoolField(TEXT("complete"),bComplete);
    Root->SetStringField(TEXT("method"),TEXT("Real engine gameplay clocks and ordinary production components; explicit fixture damage, placement and packet APIs. One actual logical-cursor/controller-LMB pistol shot tests a declared fixed standing 112 HP target while both collected timed effects are active. Optional engine viewport/master-mix media is declared separately. No native-input, performance or perceptual-audio claim; pause is actual engine pause. Full map-travel restart is a separate check."));
    Root->SetBoolField(TEXT("capture_requested"),bCaptureRequested);
    Root->SetNumberField(TEXT("captured_frames"),Frames);
    Root->SetStringField(TEXT("capture_status"),!bCaptureRequested?TEXT("NOT_REQUESTED"):Capture->IsComplete()?TEXT("PASS"):TEXT("FAILED_OR_INTERRUPTED"));
    if (bCaptureRequested)
    {
        Root->SetStringField(TEXT("capture_directory"),TEXT("Media"));
        Root->SetStringField(TEXT("capture_failure_reason"),Capture->GetFailureReason());
    }
    Root->SetArrayField(TEXT("observations"),Observations);
    FString Json; const auto Writer=TJsonWriterFactory<>::Create(&Json); FJsonSerializer::Serialize(Root,Writer);
    const bool ObservationsSaved=FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("observations.json")));
    Check(ObservationsSaved,TEXT("Actual sampled observation record saved"));
    const bool AssertionsSaved=FFileHelper::SaveStringToFile(ChecksCsv,*(Folder/TEXT("checks.csv")));
    if (!AssertionsSaved) { ++Checks; ++Failures; UE_LOG(LogTemp,Error,TEXT("ONE06_SURVIVAL FAIL | Assertion CSV could not be saved")); }
    FString Summary=FString::Printf(TEXT("Complete: %d\nChecks: %d\nFailures: %d\nFrames: %d\n"),bComplete,Checks,Failures,Frames);
    if (!FFileHelper::SaveStringToFile(Summary,*(Folder/TEXT("checks.txt")))) { ++Failures; UE_LOG(LogTemp,Error,TEXT("ONE06_SURVIVAL FAIL | Summary could not be saved")); }
    UE_LOG(LogTemp,Display,TEXT("ONE06_SURVIVAL_COMPLETE complete=%d failures=%d checks=%d frames=%d"),bComplete,Failures,Checks,Frames);
}
void AONE06SurvivalCheck::EndPlay(const EEndPlayReason::Type Reason)
{ bEndingPlay=true; if (!bFinished) { Check(false,TEXT("Survival fixture interrupted before completion")); Finish(false); } Super::EndPlay(Reason); }
