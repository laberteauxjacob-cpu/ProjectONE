#include "Misc/AutomationTest.h"
#include "ONEHealthComponent.h"
#include "ONEZombie.h"
#include "Engine/World.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    struct FONECombatTestWorld
    {
        UWorld* World=nullptr;
        FONECombatTestWorld()
        {
            const UWorld::InitializationValues Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
        }
        ~FONECombatTestWorld() { if (World) World->DestroyWorld(false); }
        AONEZombie* Spawn()
        {
            FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            AONEZombie* Zombie=World ? World->SpawnActor<AONEZombie>(FVector(0,0,100),FRotator::ZeroRotator,Params) : nullptr;
            if (Zombie) Zombie->GetHealthComponent()->Restore();
            return Zombie;
        }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONEHealthInvariantTest,"ProjectONE.Combat.DamageAndDeathAreMonotonic",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONEHealthInvariantTest::RunTest(const FString& Parameters)
{
    UONEHealthComponent* H=NewObject<UONEHealthComponent>();
    H->MaxHealth=100; H->Restore();
    TestFalse(TEXT("Negative damage must not heal"),H->ApplyDamage(-40));
    TestEqual(TEXT("Unchanged after negative damage"),H->Health,100.f);
    TestTrue(TEXT("Ordinary hit accepted"),H->ApplyDamage(32));
    TestEqual(TEXT("Damage subtracts health"),H->Health,68.f);
    TestTrue(TEXT("Lethal hit accepted"),H->ApplyDamage(100));
    TestTrue(TEXT("Dead after lethal hit"),H->IsDead());
    TestEqual(TEXT("Health clamps at zero"),H->Health,0.f);
    TestFalse(TEXT("Already dead ignores repeated damage"),H->ApplyDamage(32));
    TestEqual(TEXT("No negative corpse health"),H->Health,0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONEPacketIdempotenceTest,"ProjectONE.Combat.WeaponPacketsAreIdempotent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONEPacketIdempotenceTest::RunTest(const FString& Parameters)
{
    FONECombatTestWorld Fixture;
    AONEZombie* Z=Fixture.Spawn(); if (!TestNotNull(TEXT("Native infected created"),Z)) return false;
    FONEWeaponDamagePacket Packet; Packet.ShotId=101; Packet.BodyDamage=32; Packet.Pellets=4;
    TestTrue(TEXT("An aggregated discharge is accepted once"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Four traced pellets carry their summed damage, not four extra applications"),Z->GetHealth(),80.f);
    TestFalse(TEXT("Replaying the same discharge is rejected"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("A duplicate cannot reduce health"),Z->GetHealth(),80.f);
    TestEqual(TEXT("A duplicate cannot create another damage transaction"),Z->GetDamageTransactionCount(),1);
    Packet.ShotId=102;
    TestTrue(TEXT("A different discharge with equal damage remains valid"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Independent discharge subtracts once"),Z->GetHealth(),48.f);
    Packet.ShotId=103; Packet.BodyDamage=48;
    TestTrue(TEXT("Exact remaining health is lethal"),Z->ReceiveWeaponDamage(Packet));
    TestTrue(TEXT("Torso packet produces an intact death"),Z->IsDead() && Z->HasHead() && Z->HasLeftArm());
    TestFalse(TEXT("Lethal packet replay is rejected"),Z->ReceiveWeaponDamage(Packet));
    Packet.ShotId=104;
    TestFalse(TEXT("A later discharge cannot damage a corpse"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Death occurred after exactly three accepted transactions"),Z->GetDamageTransactionCount(),3);
    TestEqual(TEXT("No region trauma means no severing"),Z->GetSeverCount(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONERegionalTraumaTest,"ProjectONE.Combat.RegionalTraumaBoundaries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONERegionalTraumaTest::RunTest(const FString& Parameters)
{
    FONECombatTestWorld Fixture;
    AONEZombie* Head=Fixture.Spawn(); AONEZombie* Arm=Fixture.Spawn(); AONEZombie* Mixed=Fixture.Spawn();
    if (!TestNotNull(TEXT("Head target created"),Head) || !TestNotNull(TEXT("Arm target created"),Arm) || !TestNotNull(TEXT("Mixed target created"),Mixed)) return false;
    FONEWeaponDamagePacket Packet; Packet.ShotId=201; Packet.HeadDamage=15; Packet.HeadTrauma=31;
    Head->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Head trauma just below threshold keeps head and life"),Head->HasHead() && !Head->IsDead());
    TestEqual(TEXT("Subthreshold head hit still applies health damage"),Head->GetHealth(),97.f);
    Packet.ShotId=202; Packet.HeadDamage=1; Packet.HeadTrauma=1;
    Head->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Cumulative head trauma at threshold severs and kills"),!Head->HasHead() && Head->IsDead());
    TestEqual(TEXT("Head threshold creates exactly one detachment"),Head->GetSeverCount(),1);

    Packet=FONEWeaponDamagePacket(); Packet.ShotId=301; Packet.ArmDamage=49; Packet.ArmTrauma=49;
    Arm->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Arm trauma just below threshold preserves the arm"),Arm->HasLeftArm());
    Packet.ShotId=302; Packet.ArmDamage=1; Packet.ArmTrauma=1;
    Arm->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Arm threshold removes the arm while the infected survives"),!Arm->HasLeftArm() && !Arm->IsDead() && Arm->HasHead());
    TestTrue(TEXT("Fifty arm damage contributes twenty health damage"),FMath::IsNearlyEqual(Arm->GetHealth(),92.f));
    Packet.ShotId=303; Packet.ArmDamage=50; Packet.ArmTrauma=50;
    TestFalse(TEXT("A missing arm cannot receive another arm-only transaction"),Arm->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("A missing arm cannot detach twice"),Arm->GetSeverCount(),1);
    TestEqual(TEXT("Only the two valid arm transactions counted"),Arm->GetDamageTransactionCount(),2);
    Packet.ShotId=304; Packet.BodyDamage=10;
    TestTrue(TEXT("A mixed packet still damages the intact torso after arm loss"),Arm->ReceiveWeaponDamage(Packet));
    TestTrue(TEXT("Missing-arm damage is excluded from a mixed packet"),FMath::IsNearlyEqual(Arm->GetHealth(),82.f));

    Packet=FONEWeaponDamagePacket(); Packet.ShotId=401; Packet.HeadDamage=32; Packet.HeadTrauma=32; Packet.ArmDamage=50; Packet.ArmTrauma=50; Packet.Pellets=8;
    Mixed->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Simultaneous lethal head and arm trauma prioritizes the head"),Mixed->IsDead() && !Mixed->HasHead() && Mixed->HasLeftArm());
    TestEqual(TEXT("An eight-pellet blast cannot produce two detachments"),Mixed->GetSeverCount(),1);
    TestFalse(TEXT("Replayed multi-region blast cannot detach again"),Mixed->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Multi-region replay leaves one accepted transaction"),Mixed->GetDamageTransactionCount(),1);
    return true;
}
#endif
