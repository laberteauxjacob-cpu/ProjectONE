#include "Misc/AutomationTest.h"
#include "ONEHealthComponent.h"
#include "ONEZombie.h"
#include "Engine/World.h"
#include <limits>
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
    FONEWeaponDamagePacket ONERegionalTestPacket(uint64 Id,EONEHitRegion Region,float Damage,float Trauma=0.f)
    {
        FONEWeaponDamagePacket Packet; Packet.ShotId=Id;
        Packet.Get(Region).AddPellet(Damage,Trauma,FVector(0,0,100),FVector::ForwardVector,-FVector::ForwardVector,NAME_None);
        Packet.Finalize(); return Packet;
    }
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
    FONEWeaponDamagePacket Packet; Packet.ShotId=101;
    for (int32 I=0;I<4;++I) Packet.Get(EONEHitRegion::Body).AddPellet(8,0,FVector(I,0,100),FVector::ForwardVector,-FVector::ForwardVector,TEXT("spine_02"));
    Packet.Finalize();
    TestTrue(TEXT("An aggregated discharge is accepted once"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Four traced pellets carry their summed damage, not four extra applications"),Z->GetHealth(),80.f);
    TestFalse(TEXT("Replaying the same discharge is rejected"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("A duplicate cannot reduce health"),Z->GetHealth(),80.f);
    TestEqual(TEXT("A duplicate cannot create another damage transaction"),Z->GetDamageTransactionCount(),1);
    Packet.ShotId=102;
    TestTrue(TEXT("A different discharge with equal damage remains valid"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Independent discharge subtracts once"),Z->GetHealth(),48.f);
    Packet=ONERegionalTestPacket(103,EONEHitRegion::Body,48);
    TestTrue(TEXT("Exact remaining health is lethal"),Z->ReceiveWeaponDamage(Packet));
    TestTrue(TEXT("Torso packet produces an intact death"),Z->IsDead() && Z->HasHead() && Z->HasLeftArm() && Z->HasRightArm() && Z->HasLeftLeg());
    TestFalse(TEXT("Lethal packet replay is rejected"),Z->ReceiveWeaponDamage(Packet));
    Packet.ShotId=104;
    Z->ReceiveWeaponDamage(Packet);
    TestEqual(TEXT("A later cosmetic corpse transaction cannot change health"),Z->GetHealth(),0.f);
    TestEqual(TEXT("Death occurred after exactly three accepted transactions"),Z->GetDamageTransactionCount(),3);
    TestEqual(TEXT("No region trauma means no severing"),Z->GetSeverCount(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONERegionalTraumaTest,"ProjectONE.Combat.RegionalTraumaBoundaries",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONERegionalTraumaTest::RunTest(const FString& Parameters)
{
    FONECombatTestWorld Fixture;
    AONEZombie* Head=Fixture.Spawn(); AONEZombie* Arm=Fixture.Spawn(); AONEZombie* Mixed=Fixture.Spawn(); AONEZombie* Leg=Fixture.Spawn();
    if (!TestNotNull(TEXT("Head target created"),Head) || !TestNotNull(TEXT("Arm target created"),Arm) || !TestNotNull(TEXT("Mixed target created"),Mixed) || !TestNotNull(TEXT("Leg target created"),Leg)) return false;
    FONEWeaponDamagePacket Packet=ONERegionalTestPacket(201,EONEHitRegion::Head,15,31);
    Head->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Head trauma just below threshold keeps head and life"),Head->HasHead() && !Head->IsDead());
    TestEqual(TEXT("Subthreshold head hit still applies health damage"),Head->GetHealth(),97.f);
    Packet=ONERegionalTestPacket(202,EONEHitRegion::Head,1,1);
    Head->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Cumulative head trauma at threshold severs and kills"),!Head->HasHead() && Head->IsDead());
    TestEqual(TEXT("Head threshold creates exactly one detachment"),Head->GetSeverCount(),1);

    Packet=ONERegionalTestPacket(301,EONEHitRegion::ArmLeft,49,49);
    Arm->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Arm trauma just below threshold preserves the arm"),Arm->HasLeftArm());
    Packet=ONERegionalTestPacket(302,EONEHitRegion::ArmLeft,1,1);
    Arm->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Left-arm threshold preserves the opposite arm and life"),!Arm->HasLeftArm() && Arm->HasRightArm() && !Arm->IsDead() && Arm->HasHead());
    TestTrue(TEXT("Fifty arm damage contributes twenty health damage"),FMath::IsNearlyEqual(Arm->GetHealth(),92.f));
    Packet=ONERegionalTestPacket(303,EONEHitRegion::ArmLeft,50,50);
    TestFalse(TEXT("A missing arm cannot receive another arm-only transaction"),Arm->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("A missing arm cannot detach twice"),Arm->GetSeverCount(),1);
    TestEqual(TEXT("Only the two valid arm transactions counted"),Arm->GetDamageTransactionCount(),2);
    Packet.ShotId=304; Packet.Get(EONEHitRegion::Body).AddPellet(10,0,FVector(0,0,100),FVector::ForwardVector,-FVector::ForwardVector,TEXT("spine_02")); Packet.Finalize();
    TestTrue(TEXT("A mixed packet still damages the intact torso after arm loss"),Arm->ReceiveWeaponDamage(Packet));
    TestTrue(TEXT("Missing-arm damage is excluded from a mixed packet"),FMath::IsNearlyEqual(Arm->GetHealth(),82.f));

    Packet=ONERegionalTestPacket(401,EONEHitRegion::Head,32,32);
    Packet.Get(EONEHitRegion::ArmLeft).AddPellet(50,50,FVector(0,-24,130),FVector::ForwardVector,-FVector::ForwardVector,TEXT("upperarm_r"));
    Packet.Finalize();
    Mixed->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("One mixed packet resolves both eligible head and left-arm severs"),Mixed->IsDead() && !Mixed->HasHead() && !Mixed->HasLeftArm() && Mixed->HasRightArm());
    TestEqual(TEXT("Two eligible regions create exactly two detachments"),Mixed->GetSeverCount(),2);
    TestFalse(TEXT("Replayed multi-region blast cannot detach again"),Mixed->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Multi-region replay leaves one accepted transaction"),Mixed->GetDamageTransactionCount(),1);
    Packet=ONERegionalTestPacket(451,EONEHitRegion::LegLeft,1,Leg->LegSeverThreshold-1.f); Leg->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Left-leg trauma immediately below threshold preserves anatomy and life"),Leg->HasLeftLeg() && !Leg->IsDead());
    Packet=ONERegionalTestPacket(452,EONEHitRegion::LegLeft,1,1); Leg->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Exact cumulative left-leg threshold is fatal with other anatomy intact"),!Leg->HasLeftLeg() && Leg->IsDead() && Leg->HasHead() && Leg->HasLeftArm() && Leg->HasRightArm());
    TestEqual(TEXT("Left-leg boundary produces one detachment"),Leg->GetSeverCount(),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONERegionalSpatialTest,"ProjectONE.Combat.RegionalPelletsKeepSpatialData",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONERegionalSpatialTest::RunTest(const FString& Parameters)
{
    FONEWeaponDamagePacket Packet; Packet.ShotId=501;
    auto& Left=Packet.Get(EONEHitRegion::ArmLeft);
    Left.AddPellet(10,5,FVector(-50,-20,140),FVector::ForwardVector,-FVector::ForwardVector,TEXT("upperarm_r"));
    Left.AddPellet(30,15,FVector(-10,-20,160),FVector::RightVector,-FVector::RightVector,TEXT("hand_r"));
    Packet.Get(EONEHitRegion::ArmRight).AddPellet(20,12,FVector(80,30,125),FVector::ForwardVector,-FVector::ForwardVector,TEXT("upperarm_l"));
    const double Nan=std::numeric_limits<double>::quiet_NaN();
    Left.AddPellet(100,100,FVector(Nan,0,0),FVector::ForwardVector,-FVector::ForwardVector,TEXT("head"));
    Left.AddPellet(100,100,FVector::ZeroVector,FVector(0,Nan,0),-FVector::ForwardVector,TEXT("head"));
    Left.AddPellet(100,100,FVector::ZeroVector,FVector::ForwardVector,FVector(0,0,Nan),TEXT("head"));
    Packet.Finalize();
    TestEqual(TEXT("Invalid spatial pellets do not enter the transaction"),Packet.GetPellets(),3);
    TestEqual(TEXT("Regional damage sums exactly once"),Packet.GetTotalDamage(),60.f);
    TestTrue(TEXT("Left impact retains its own damage-weighted centre"),Left.Position.Equals(FVector(-20,-20,155),.001));
    TestTrue(TEXT("Directions and normals retain per-pellet weighted vectors"),Left.Direction.Equals(FVector(10,30,0).GetSafeNormal(),.001) && Left.Normal.Equals(-Left.Direction,.001));
    TestEqual(TEXT("Representative bone belongs to strongest regional pellet"),Left.Bone,FName(TEXT("hand_r")));
    TestTrue(TEXT("Opposite arm is not folded into left impact position"),Packet.Get(EONEHitRegion::ArmRight).Position.Equals(FVector(80,30,125),.001));
    const FVector Once=Left.Position; Packet.Finalize();
    TestTrue(TEXT("Repeated finalization preserves a finalized position"),Left.Position.Equals(Once,.001));
    TestFalse(TEXT("Invalid hit region cannot index a damage entry"),FONEWeaponDamagePacket::IsValidRegion(EONEHitRegion::Invalid));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONEAnatomicalSidesTest,"ProjectONE.Combat.AnatomicalSidesAndCorpseTransactions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONEAnatomicalSidesTest::RunTest(const FString& Parameters)
{
    FONECombatTestWorld Fixture; AONEZombie* Z=Fixture.Spawn();
    if (!TestNotNull(TEXT("Anatomical target created"),Z)) return false;
    FHitResult Hit; Hit.BoneName=TEXT("upperarm_r");
    TestEqual(TEXT("Reflected source _r maps to actual left arm"),Z->GetHitRegion(Hit),EONEHitRegion::ArmLeft);
    Hit.BoneName=TEXT("upperarm_l");
    TestEqual(TEXT("Reflected source _l maps to actual right arm"),Z->GetHitRegion(Hit),EONEHitRegion::ArmRight);
    Hit.BoneName=TEXT("calf_r"); TestEqual(TEXT("Source right leg maps to actual left leg"),Z->GetHitRegion(Hit),EONEHitRegion::LegLeft);
    Hit.BoneName=TEXT("calf_l"); TestEqual(TEXT("Opposite leg remains its own damage region"),Z->GetHitRegion(Hit),EONEHitRegion::LegRight);
    auto Packet=ONERegionalTestPacket(601,EONEHitRegion::ArmRight,50,50);
    // The region is authoritative. A representative impulse bone cannot select
    // a different detachable part, even in a manually authored transaction.
    Packet.Get(EONEHitRegion::ArmRight).Bone=TEXT("upperarm_r");
    Z->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Right-region trauma cannot detach the opposite left arm"),Z->HasLeftArm() && !Z->HasRightArm() && !Z->IsDead());
    Hit.BoneName=TEXT("upperarm_l"); TestEqual(TEXT("Removed side no longer resolves to a live hit region"),Z->GetHitRegion(Hit),EONEHitRegion::Invalid);
    Packet=ONERegionalTestPacket(602,EONEHitRegion::ArmLeft,50,50); Z->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Both-arm loss follows the fatal policy"),Z->IsDead() && !Z->HasLeftArm() && !Z->HasRightArm());
    const int32 Transactions=Z->GetDamageTransactionCount();
    Packet=ONERegionalTestPacket(603,EONEHitRegion::Head,32,32); Z->ReceiveWeaponDamage(Packet);
    TestTrue(TEXT("Fresh corpse head trauma can detach remaining anatomy without live damage"),!Z->HasHead() && Z->GetHealth()==0.f && Z->GetDamageTransactionCount()==Transactions);
    const int32 Severs=Z->GetSeverCount();
    TestFalse(TEXT("Corpse transaction replay cannot detach twice"),Z->ReceiveWeaponDamage(Packet));
    TestEqual(TEXT("Corpse replay preserves sever count"),Z->GetSeverCount(),Severs);
    return true;
}
#endif
