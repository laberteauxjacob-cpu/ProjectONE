#include "Misc/AutomationTest.h"
#include "ONEWeaponTiming.h"
#include "ONEWeaponCatalog.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05BurstPhaseTest,"ProjectONE.Weapons.Candidate05.AutomaticPhaseAndHitch",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05BurstPhaseTest::RunTest(const FString& Parameters)
{
    for (double Rate:{30.,60.,120.}) for (double Interval:{.1,.1/1.15}) for (double Phase:{0.,.37,.89})
    {
        const double Step=1./Rate,Start=Phase*Step,End=Start+10.;
        double Deadline=Start,First=-1,Last=-1; int32 Shots=0;
        for (double Time=Start;Time<End-1.e-8;Time+=Step)
            if (ONEWeaponTiming::IsDue(Time,Deadline))
            {
                if (First<0) First=Time;
                Last=Time; Deadline=ONEWeaponTiming::AfterDischarge(Deadline,Time,Interval,Shots>0); ++Shots;
            }
        const int32 Expected=FMath::RoundToInt(10./Interval);
        TestTrue(FString::Printf(TEXT("%.0ffps phase%.2f interval%.9f sustains expected%d shots, observed%d"),Rate,Phase,Interval,Expected,Shots),FMath::Abs(Shots-Expected)<=1);
        TestTrue(TEXT("Quantization remains below one frame instead of accumulating drift"),FMath::Abs((Last-First)-(Shots-1)*Interval)<=Step+.00002);
        const double HitchTime=Deadline+.47;
        Deadline=ONEWeaponTiming::AfterDischarge(Deadline,HitchTime,Interval,true);
        TestTrue(TEXT("A missed whole interval resets deadline to a full new interval"),FMath::IsNearlyEqual(Deadline,HitchTime+Interval,1.e-10));
        TestFalse(TEXT("No owed catch-up shot on the first frame after hitch"),ONEWeaponTiming::IsDue(HitchTime+Step,Deadline));
        TestTrue(TEXT("A fresh press resets the old burst phase"),FMath::IsNearlyEqual(ONEWeaponTiming::AfterDischarge(-100.,12.,Interval,false),12.+Interval,1.e-10));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05RateCatalogTest,"ProjectONE.Weapons.Candidate05.CatalogCadenceAndUpgradeBanks",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05RateCatalogTest::RunTest(const FString& Parameters)
{
    const auto Rows=ONEWeaponCatalog::BuildDefaults();
    for (const auto& D:Rows)
    {
        if (D.Family==EONEWeaponFamily::Carbine)
        {
            TestTrue(TEXT("M4 and Overcurrent retain automatic operation"),D.bAutomatic);
            TestTrue(TEXT("Explicit M4 interval .100; upgraded rate is exactly fifteen percent faster"),FMath::IsNearlyEqual(double(D.FireInterval),D.bUpgraded?.1/1.15:.1,1.e-7));
            const auto* Fire=D.Operations.FindByPredicate([](const auto& O){return O.Operation==EONEWeaponOperation::Fire;});
            TestTrue(TEXT("Fire visual/event operation fits the actual cadence"),Fire && FMath::IsNearlyEqual(Fire->Duration,D.FireInterval,1.e-6f));
        }
        if (D.bUpgraded)
        {
            TestEqual(TEXT("Each upgrade uses six independent new voices"),D.ShotSounds.Num(),6);
            for (const auto& S:D.ShotSounds) TestTrue(TEXT("Upgraded bank is explicitly Candidate05 source"),S.ToSoftObjectPath().ToString().Contains(TEXT("/Audio/Candidate05/")));
            TestTrue(TEXT("Definition gain stays finite and bounded independently of bus gain"),FMath::IsFinite(D.ShotVolume) && D.ShotVolume>0 && D.ShotVolume<=2);
        }
    }
    return true;
}
#endif
