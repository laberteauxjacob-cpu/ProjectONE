#include "Misc/AutomationTest.h"
#include "ONEWeaponComponent.h"
#include "ONEWeaponCatalog.h"
#include "ONEPlayer.h"
#include "ONEHealthComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace ONE04InventoryTest
{
    struct FFixture
    {
        UWorld* World=nullptr;
        AONEPlayer* Player=nullptr;
        UONEWeaponComponent* Weapon=nullptr;
        FFixture()
        {
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
            FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            USkeletalMesh* FixtureMesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Response.SK_Response"));
            if (!FixtureMesh) return;
            // These inventory-only worlds do not dispatch actor BeginPlay.
            // Install the real skeleton before native components register so
            // bone-attached presentation can query valid sockets throughout.
            Spawn.CustomPreSpawnInitalization=[FixtureMesh](AActor* Actor)
            { if (auto* Pawn=Cast<AONEPlayer>(Actor)) Pawn->GetMesh()->SetSkeletalMesh(FixtureMesh); };
            Player=World ? World->SpawnActor<AONEPlayer>(FVector(0,0,100),FRotator::ZeroRotator,Spawn) : nullptr;
            if (Player) { Player->Health->Restore(); Weapon=Player->GetWeaponComponent(); Weapon->ResetStarterLoadout(); }
        }
        ~FFixture() { if (World) World->DestroyWorld(false); }
    };
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE04CatalogTimingTest,"ProjectONE.Inventory.SixIndependentVariantsAndTiming",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE04CatalogTimingTest::RunTest(const FString& Parameters)
{
    const auto Rows=ONEWeaponCatalog::BuildDefaults(); TestEqual(TEXT("Six catalog rows, not six slots"),Rows.Num(),6);
    TSet<FName> Ids;
    for (const auto& D:Rows) { Ids.Add(D.Id); TestEqual(TEXT("Six distinct shot variations per effective weapon"),D.ShotSounds.Num(),6); }
    TestEqual(TEXT("Every effective variant has stable unique identity"),Ids.Num(),6);
    for (EONEWeaponFamily Family:{EONEWeaponFamily::Carbine,EONEWeaponFamily::Shotgun,EONEWeaponFamily::Pistol})
    {
        const auto* Base=Rows.FindByPredicate([Family](const auto& D){return D.Family==Family && !D.bUpgraded;});
        const auto* Upgrade=Rows.FindByPredicate([Family](const auto& D){return D.Family==Family && D.bUpgraded;});
        if (!TestNotNull(TEXT("Base family exists"),Base) || !TestNotNull(TEXT("Upgrade family exists"),Upgrade)) return false;
        const double RateError=FMath::Abs(double(Upgrade->FireInterval)*1.15-double(Base->FireInterval));
        TestTrue(FString::Printf(TEXT("%s rate increases15%%: base %.9f upgraded %.9f ratio error %.12f"),*Base->Id.ToString(),Base->FireInterval,Upgrade->FireInterval,RateError),RateError<1.e-6);
        TestTrue(TEXT("Damage doubles without modifying base"),FMath::IsNearlyEqual(Upgrade->Damage,2.f*Base->Damage));
        TestEqual(TEXT("Pellet count stays bounded and unchanged"),Upgrade->Pellets,Base->Pellets);
        for (const auto& O:Base->Operations)
        {
            const auto* U=Upgrade->Operations.FindByPredicate([&](const auto& X){return X.Operation==O.Operation;});
            if (!TestNotNull(TEXT("Upgrade retains every mechanical operation"),U)) return false;
            const float Rate=O.Operation==EONEWeaponOperation::Fire || O.Operation==EONEWeaponOperation::Pump ? 1.15f : 1.f;
            TestTrue(TEXT("Operation duration scales only for firing/pump"),FMath::IsNearlyEqual(U->Duration*Rate,O.Duration));
            TestEqual(TEXT("All mechanical events retained"),U->Events.Num(),O.Events.Num());
            for (int32 I=0;I<O.Events.Num();++I)
            { TestTrue(TEXT("Event phase remains coherent with operation rate"),FMath::IsNearlyEqual(U->Events[I].Time*Rate,O.Events[I].Time)); TestTrue(TEXT("Event occurs inside operation"),U->Events[I].Time<=U->Duration); }
        }
        TestEqual(TEXT("Only Last Word gets exactly one extra victim"),Upgrade->AdditionalVictims,Family==EONEWeaponFamily::Pistol ? 1 : 0);
        TestEqual(TEXT("No base penetration"),Base->AdditionalVictims,0);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE04ReservationTest,"ProjectONE.Inventory.ExactInstanceReservationAndRollback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE04ReservationTest::RunTest(const FString& Parameters)
{
    ONE04InventoryTest::FFixture F; auto* W=F.Weapon; if (!TestNotNull(TEXT("Inventory fixture"),W)) return false;
    TestEqual(TEXT("Starter is M1911"),W->GetDefinition().Id,FName(TEXT("P1911")));
    TestEqual(TEXT("Pistol starts7/56"),W->GetAmmo(),7); TestEqual(TEXT("Pistol reserve56"),W->GetReserveAmmo(),56);
    TestNull(TEXT("Empty second slot has no definition"),W->GetDefinitionForWeapon(1));
    W->AddReserveAmmo(-10); const auto Original=*W->GetSlotState(0);
    W->SetHandoffLocked(true); FONEWeaponReservation Token;
    TestTrue(TEXT("Accepted handoff reserves despite input lock"),W->ReserveEquippedForUpgrade(Token));
    TestFalse(TEXT("Only weapon deposit leaves unarmed"),W->HasUsableWeapon());
    TestEqual(TEXT("Unarmed selected index is safe sentinel"),W->GetEquippedIndex(),INDEX_NONE);
    TestEqual(TEXT("Safe unarmed damage is zero"),W->GetDefinition().Damage,0.f);
    W->RefillAllAmmo(); W->GrantRoundAmmo(); W->AddReserveAmmo(100);
    TestEqual(TEXT("Refill/reward cannot mutate reserved ammo"),W->GetReserveAmmoForWeapon(0),Original.Reserve);
    TestFalse(TEXT("Reserved family excluded from box"),W->IsFamilyRollEligible(EONEWeaponFamily::Pistol));
    TestFalse(TEXT("Reserved slot cannot equip"),W->SelectWeapon(0));
    auto Forged=Token; ++Forged.InstanceId;
    TestFalse(TEXT("Wrong instance cannot release reservation"),W->RollbackUpgrade(Forged));
    Token.Before.Ammo=999; Token.Before.Reserve=999;
    TestTrue(TEXT("Valid identity rolls back once"),W->RollbackUpgrade(Token));
    TestEqual(TEXT("Rollback restores authoritative snapshot, not caller mutation"),W->GetAmmoForWeapon(0),Original.Ammo);
    TestEqual(TEXT("Rollback preserves exact predeposit reserve"),W->GetReserveAmmoForWeapon(0),Original.Reserve);
    TestEqual(TEXT("Rollback keeps exact owned instance"),W->GetSlotState(0)->InstanceId,Original.InstanceId);
    TestFalse(TEXT("Repeated rollback cannot repeat recovery"),W->RollbackUpgrade(Token));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE04AcquisitionTest,"ProjectONE.Inventory.AcquisitionPlansAndUpgradeRefill",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE04AcquisitionTest::RunTest(const FString& Parameters)
{
    ONE04InventoryTest::FFixture F; auto* W=F.Weapon; if (!TestNotNull(TEXT("Inventory fixture"),W)) return false;
    auto Plan=W->BuildAcquisitionPlan(EONEWeaponFamily::Carbine);
    TestTrue(TEXT("Box fills actual empty slot1"),Plan.Kind==EONEWeaponAcquisitionKind::FillEmpty && Plan.Slot==1);
    W->SetHandoffLocked(true); W->SetHandoffLocked(false);
    TestFalse(TEXT("Stale hold fingerprint cannot collect"),W->ApplyAcquisitionPlan(Plan));
    Plan=W->BuildAcquisitionPlan(EONEWeaponFamily::Carbine);
    TestTrue(TEXT("Revalidated box collects"),W->ApplyAcquisitionPlan(Plan));
    TestFalse(TEXT("Collection plan cannot replay"),W->ApplyAcquisitionPlan(Plan));
    TestTrue(TEXT("Pistol and carbine are separate owned instances"),W->GetSlotState(0)->InstanceId!=W->GetSlotState(1)->InstanceId);
    W->CancelAllOperations(); FONEWeaponReservation Token;
    TestTrue(TEXT("Pistol may be reserved while carbine remains"),W->ReserveEquippedForUpgrade(Token));
    TestTrue(TEXT("Other owned weapon stays usable"),W->HasUsableWeapon());
    TestTrue(TEXT("Ready marks, but does not deliver"),W->MarkUpgradeReady(Token));
    TestFalse(TEXT("Ready cannot repeat"),W->MarkUpgradeReady(Token));
    TestTrue(TEXT("Ready still cannot equip original slot"),W->GetSlotState(0)->Status==EONEWeaponSlotStatus::ReadyToCollect);
    W->SetHandoffLocked(true);
    TestTrue(TEXT("Retrieval callback collects under brief handoff lock"),W->CollectUpgrade(Token));
    W->SetHandoffLocked(false);
    TestEqual(TEXT("Upgrade returned to exact instance"),W->GetSlotState(0)->InstanceId,Token.InstanceId);
    TestEqual(TEXT("Last Word effective capacity refilled once"),W->GetAmmoForWeapon(0),14);
    TestEqual(TEXT("Last Word maximum reserve awarded once"),W->GetReserveAmmoForWeapon(0),168);
    TestFalse(TEXT("Duplicate collection cannot refill again"),W->CollectUpgrade(Token));
    const auto Duplicate=W->BuildAcquisitionPlan(EONEWeaponFamily::Pistol);
    TestTrue(TEXT("Full duplicate is explicit rather than a replacement"),Duplicate.Kind==EONEWeaponAcquisitionKind::AlreadyFull && Duplicate.Slot==0);
    TestTrue(TEXT("Full duplicate may be consumed without downgrade"),W->ApplyAcquisitionPlan(Duplicate));
    TestTrue(TEXT("Duplicate retains upgraded instance"),W->GetSlotState(0)->bUpgraded && W->GetSlotState(0)->InstanceId==Token.InstanceId);
    TestEqual(TEXT("New base catalog remains seven rounds"),W->GetCatalogDefinition(EONEWeaponFamily::Pistol,false)->Capacity,7);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE04RunBoundaryTest,"ProjectONE.Inventory.ResetRejectsOldMachineCallbacks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE04RunBoundaryTest::RunTest(const FString& Parameters)
{
    ONE04InventoryTest::FFixture F; auto* W=F.Weapon; if (!TestNotNull(TEXT("Inventory fixture"),W)) return false;
    FONEWeaponReservation Old; TestTrue(TEXT("Reserve old run"),W->ReserveEquippedForUpgrade(Old));
    const uint64 Instance=Old.InstanceId;
    W->InvalidateMachineTransactions();
    TestFalse(TEXT("Invalidated completion cannot deliver"),W->MarkUpgradeReady(Old));
    TestEqual(TEXT("Technical cleanup restores same original instance"),W->GetSlotState(0)->InstanceId,Instance);
    W->ResetStarterLoadout();
    TestTrue(TEXT("Restart gives a new run and instance"),W->GetRunId()!=Old.RunId && W->GetSlotState(0)->InstanceId!=Instance);
    TestFalse(TEXT("Old ready callback rejected"),W->MarkUpgradeReady(Old));
    TestFalse(TEXT("Old delivery rejected"),W->CollectUpgrade(Old));
    TestFalse(TEXT("Old refund recovery cannot restore a prior weapon"),W->RollbackUpgrade(Old));
    TestEqual(TEXT("Restart retains exactly two slots"),W->GetWeaponCount(),2);
    TestTrue(TEXT("Restart slot1 Empty"),W->GetSlotState(1)->Status==EONEWeaponSlotStatus::Empty);
    return true;
}
#endif
