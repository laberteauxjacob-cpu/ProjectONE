#include "Misc/AutomationTest.h"
#include "ONEHealthComponent.h"
#include "ONE06CombatAwards.h"
#include "ONEPowerUpComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "UObject/StrongObjectPtr.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    struct FRecoveryWorld
    {
        UWorld* World=nullptr;
        UONEHealthComponent* Health=nullptr;
        FRecoveryWorld()
        {
            if (!GEngine) return;
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
            if (!World) return;
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
            if (auto* Owner=World->SpawnActor<AActor>())
            {
                Health=NewObject<UONEHealthComponent>(Owner);
                Owner->AddInstanceComponent(Health);
                Health->RegisterComponent();
            }
            // No BeginPlay or world ticking: the registered production component
            // receives explicit gameplay deltas to test exact delay boundaries.
        }
        ~FRecoveryWorld()
        {
            if (!World) return;
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(false);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
        }
    };
    void RecoveryStep(UONEHealthComponent* Health,float Seconds)
    { Health->TickComponent(Seconds,LEVELTICK_All,nullptr); }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06RecoveryTest,"ProjectONE.Candidate06.Survival.AcceptedDamageDelayAndMaximum",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06RecoveryTest::RunTest(const FString&)
{
    FRecoveryWorld Fixture;
    auto* Health=Fixture.Health;
    if (!TestNotNull(TEXT("Health component has a real actor and engine world"),Health)) return false;
    if (!TestTrue(TEXT("Production TickComponent requires registered fixture components"),Health->IsRegistered())) return false;
    Health->Restore(); Health->ApplyDamage(50.f); RecoveryStep(Health,30.f);
    TestEqual(TEXT("Shared health defaults never give an infected regeneration"),Health->Health,50.f);
    Health->EnablePlayerRegeneration(true); Health->Restore();
    int32 RecoveryChanges=0,DamageChanges=0;
    Health->OnHealthChanged.AddLambda([&](float Before,float After,float,bool Recovery)
    { if (Recovery && After>Before) ++RecoveryChanges; if (!Recovery && After<Before) ++DamageChanges; });
    TestTrue(TEXT("First health-reducing hit is accepted"),Health->ApplyDamage(50.f));
    RecoveryStep(Health,14.9f);
    TestEqual(TEXT("No healing before15 seconds"),Health->Health,50.f);
    TestFalse(TEXT("Zero damage is rejected"),Health->ApplyDamage(0.f));
    TestFalse(TEXT("Negative damage is rejected"),Health->ApplyDamage(-10.f));
    TestFalse(TEXT("Nonfinite damage is rejected"),Health->ApplyDamage(std::numeric_limits<float>::quiet_NaN()));
    RecoveryStep(Health,.2f);
    TestTrue(TEXT("Threshold crossing heals only the eligible0.1s portion"),FMath::IsNearlyEqual(Health->Health,51.f,.001f));
    TestTrue(TEXT("Ignored attempts did not restart the delay"),Health->IsRegenerating());
    TestTrue(TEXT("A new real hit interrupts active recovery"),Health->ApplyDamage(19.f));
    TestFalse(TEXT("Recovery state stops immediately on accepted damage"),Health->IsRegenerating());
    TestEqual(TEXT("Every accepted hit restores the full15s wait"),Health->GetRegenerationDelayRemaining(),15.f);
    const float Damaged=Health->Health;
    RecoveryStep(Health,10.f); RecoveryStep(Health,4.9f);
    TestEqual(TEXT("Repeated hit establishes its own complete delay"),Health->Health,Damaged);
    Health->SetMaximumHealth(200.f);
    RecoveryStep(Health,.2f);
    TestTrue(TEXT("Recovery uses current200 max:20HP/s without resetting delay"),FMath::IsNearlyEqual(Health->Health,Damaged+2.f,.002f));
    RecoveryStep(Health,100.f);
    TestEqual(TEXT("A long gameplay delta clamps at current maximum"),Health->Health,200.f);
    TestEqual(TEXT("Full health clears persistent injury fraction"),Health->GetMissingHealthFraction(),0.f);
    Health->ApplyDamage(1000.f); RecoveryStep(Health,100.f);
    TestEqual(TEXT("Ordinary regeneration cannot revive a dead player"),Health->Health,0.f);
    TestFalse(TEXT("Death cleared active recovery"),Health->IsRegenerating());
    Health->SetMaximumHealth(300.f,true);
    TestEqual(TEXT("Maximum-health modifier cannot revive"),Health->Health,0.f);
    Health->Restore(); Health->ApplyDamage(30.f); Health->InvalidateRecovery(); RecoveryStep(Health,100.f);
    TestEqual(TEXT("Lifecycle invalidation clears old accepted-hit clock"),Health->Health,270.f);
    TestTrue(TEXT("Health observers receive accepted damage and recovery separately"),DamageChanges>=4 && RecoveryChanges>=3);
    Health->OnHealthChanged.Clear();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06CombatAwardsTest,"ProjectONE.Candidate06.Rewards.DischargeVictimAndRunDedup",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06CombatAwardsTest::RunTest(const FString&)
{
    FONECombatAwardLedger Awards; const FGuid Run=FGuid::NewGuid(); Awards.Reset(Run);
    const TStrongObjectPtr<UONEHealthComponent> VictimA(NewObject<UONEHealthComponent>());
    const TStrongObjectPtr<UONEHealthComponent> VictimB(NewObject<UONEHealthComponent>());
    const TStrongObjectPtr<UONEHealthComponent> VictimC(NewObject<UONEHealthComponent>());
    const FObjectKey A(VictimA.Get()),B(VictimB.Get()),C(VictimC.Get());
    auto Shot=Awards.Begin(1,false);
    TestEqual(TEXT("Nonlethal living impact gives10"),Awards.Resolve(Shot,A,EONEWeaponHitOutcome::LiveHit,false,true),10);
    TestEqual(TEXT("Repeated regions/pellets of one shell never duplicate impact"),Awards.Resolve(Shot,A,EONEWeaponHitOutcome::LiveHit,true,true),0);
    Awards.RegisteredDeath(B);
    TestEqual(TEXT("Penetrated victim body kill gives10+100"),Awards.Resolve(Shot,B,EONEWeaponHitOutcome::NewKill,false,false),110);
    TestEqual(TEXT("Lethal packet/death replay gives nothing"),Awards.Resolve(Shot,B,EONEWeaponHitOutcome::NewKill,false,false),0);
    TestEqual(TEXT("Corpse hit gives no impact reward"),Awards.Resolve(Shot,C,EONEWeaponHitOutcome::CorpseHit,false,false),0);
    TestEqual(TEXT("Unregistered cleanup cannot manufacture a kill"),Awards.Resolve(Shot,C,EONEWeaponHitOutcome::NewKill,true,false),0);
    Awards.End(Shot);
    TestEqual(TEXT("Closed discharge cannot accept late callback"),Awards.Resolve(Shot,C,EONEWeaponHitOutcome::LiveHit,false,true),0);
    auto Head=Awards.Begin(1,false); Awards.RegisteredDeath(C);
    TestEqual(TEXT("Qualified head-dominant lethal event gives10+120"),Awards.Resolve(Head,C,EONEWeaponHitOutcome::NewKill,true,false),130);
    Awards.End(Head);
    auto Double=Awards.Begin(2,true);
    TestTrue(TEXT("Insta-Kill and double points coexist in one immutable context"),Double.bInstaKill && Double.PointMultiplier==2);
    TestEqual(TEXT("Double Points impact is20 once"),Awards.Resolve(Double,A,EONEWeaponHitOutcome::LiveHit,false,true),20);
    Awards.RegisteredDeath(B); Awards.RegisteredDeath(C);
    TestEqual(TEXT("Doubled body kill totals220"),Awards.Resolve(Double,B,EONEWeaponHitOutcome::NewKill,false,false),220);
    TestEqual(TEXT("Doubled head kill totals260"),Awards.Resolve(Double,C,EONEWeaponHitOutcome::NewKill,true,false),260);
    auto Forged=Double; Forged.PointMultiplier=4;
    TestEqual(TEXT("Caller cannot multiply the captured award again"),Awards.Resolve(Forged,A,EONEWeaponHitOutcome::LiveHit,false,true),0);
    const auto Old=Double; Awards.Reset(FGuid::NewGuid());
    auto Fresh=Awards.Begin(1,false);
    TestEqual(TEXT("New run rejects old run identity even when discharge number restarts"),Awards.Resolve(Old,A,EONEWeaponHitOutcome::LiveHit,false,true),0);
    TestEqual(TEXT("New run can award its own distinct event"),Awards.Resolve(Fresh,A,EONEWeaponHitOutcome::LiveHit,false,true),10);
    for (int32 I=0;I<300;++I) { auto Context=Awards.Begin(1,false); Awards.End(Context); }
    TestEqual(TEXT("Receipt memory is bounded"),Awards.GetRetainedDischarges(),FONECombatAwardLedger::MaximumDischarges);
    TestEqual(TEXT("Eviction never makes an old event awardable again"),Awards.Resolve(Fresh,B,EONEWeaponHitOutcome::LiveHit,false,true),0);
    Awards.Invalidate();
    TestFalse(TEXT("Death/reset rejects new discharge snapshots"),Awards.Begin(2,true).IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06PowerUpRulesTest,"ProjectONE.Candidate06.PowerUps.ChanceWeightsAndRefresh",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06PowerUpRulesTest::RunTest(const FString&)
{
    TestTrue(TEXT("Zero sample passes a positive1% chance"),ONEPowerUpRules::PassesChance(0.f,.01f));
    TestTrue(TEXT("Just below1% passes"),ONEPowerUpRules::PassesChance(.009999f,.01f));
    TestFalse(TEXT("Exact1% boundary misses"),ONEPowerUpRules::PassesChance(.01f,.01f));
    TestFalse(TEXT("Zero configured chance never passes"),ONEPowerUpRules::PassesChance(0.f,0.f));
    TestFalse(TEXT("Out-of-range sample is rejected"),ONEPowerUpRules::PassesChance(1.f,1.f));
    TestTrue(TEXT("Equal weights first interval selects skull"),ONEPowerUpRules::WeightedType(.1f,FVector(1,1,1))==EONEPowerUpType::InstaKill);
    TestTrue(TEXT("Equal weights middle interval selects double points"),ONEPowerUpRules::WeightedType(.5f,FVector(1,1,1))==EONEPowerUpType::DoublePoints);
    TestTrue(TEXT("Equal weights final interval selects ammunition"),ONEPowerUpRules::WeightedType(.9f,FVector(1,1,1))==EONEPowerUpType::MaxAmmo);
    TestTrue(TEXT("Zero-weight types cannot be selected even at sample0"),ONEPowerUpRules::WeightedType(0.f,FVector(0,0,1))==EONEPowerUpType::MaxAmmo);
    TestTrue(TEXT("All disabled weights decline delivery explicitly"),ONEPowerUpRules::WeightedType(.5f,FVector::ZeroVector)==EONEPowerUpType::Count);
    FRandomStream Stream(606),Unrelated(18); int32 Total=0; int32 Types[3]={};
    for (int32 I=0;I<100000;++I)
    {
        if (!ONEPowerUpRules::PassesChance(Stream.GetFraction(),.01f)) continue;
        ++Total; ++Types[int32(ONEPowerUpRules::WeightedType(Stream.GetFraction(),FVector(1,1,1)))];
        Unrelated.GetFraction(); // independent streams cannot perturb the drop draw
    }
    TestTrue(TEXT("Seeded100000-death sample has a plausible total1% rate"),Total>700 && Total<1300);
    for (int32 I=0;I<3;++I) TestTrue(TEXT("Each equally weighted type receives a plausible share"),Types[I]>180 && Types[I]<500);
    FONEPowerUpModifiers Effects;
    Effects.Refresh(EONEPowerUpType::InstaKill,30.f); Effects.Advance(11.f);
    TestEqual(TEXT("Gameplay elapsed seconds reduce timed effect"),Effects.Remaining(EONEPowerUpType::InstaKill),19.f);
    Effects.Refresh(EONEPowerUpType::DoublePoints,30.f); Effects.Refresh(EONEPowerUpType::InstaKill,30.f);
    TestEqual(TEXT("Same effect refreshes to30 rather than adding49"),Effects.Remaining(EONEPowerUpType::InstaKill),30.f);
    Effects.Advance(0.f);
    TestEqual(TEXT("No elapsed gameplay time preserves paused duration"),Effects.Remaining(EONEPowerUpType::DoublePoints),30.f);
    Effects.Refresh(EONEPowerUpType::MaxAmmo,30.f);
    TestEqual(TEXT("Max Ammo never becomes a parallel timed effect"),Effects.Remaining(EONEPowerUpType::MaxAmmo),0.f);
    Effects.Advance(30.f);
    TestEqual(TEXT("Exact lifetime expires Insta-Kill"),Effects.Remaining(EONEPowerUpType::InstaKill),0.f);
    TestEqual(TEXT("Different timed effects coexist then expire"),Effects.Remaining(EONEPowerUpType::DoublePoints),0.f);
    Effects.Refresh(EONEPowerUpType::DoublePoints,30.f); Effects.Reset();
    TestEqual(TEXT("Reset clears every retained modifier"),Effects.Remaining(EONEPowerUpType::DoublePoints),0.f);
    return true;
}
#endif
