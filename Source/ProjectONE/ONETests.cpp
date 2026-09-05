#include "Misc/AutomationTest.h"
#include "ONEHealthComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
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
#endif
