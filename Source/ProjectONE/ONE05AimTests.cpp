#include "Misc/AutomationTest.h"
#include "ONEAim.h"
#include "ONEWeaponCatalog.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05AimCircleTest,"ProjectONE.Aim.CloseCursorFullCircle",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05AimCircleTest::RunTest(const FString& Parameters)
{
    const FVector Origin(100,-50,132);
    int32 Cases=0,OldReversals=0;
    for (const auto& D:ONEWeaponCatalog::BuildDefaults())
    {
        for (int32 Angle=0;Angle<360;Angle+=15)
        {
            const FRotator Facing(0,Angle,0);
            const FVector Intended=Facing.Vector();
            // A translated evaluated weapon fixture includes the grip/socket
            // offset and each actual catalog barrel, not a camera-origin ray.
            const FVector Muzzle=Origin+Facing.RotateVector(D.Muzzle+FVector(25,16,-5));
            const FVector Center=ONEAim::ResolveIntent(Origin,Origin,Intended);
            TestTrue(TEXT("Exact-centre cursor preserves the last deliberate facing"),Center.Equals(Intended,1.e-6));
            for (double Distance:{0.0,2.0,8.0,25.0,70.0,150.0,800.0})
            {
                const FVector Cursor=Origin+Intended*Distance;
                const FVector Intent=ONEAim::ResolveIntent(Origin,Cursor,Intended);
                const FVector Shot=ONEAim::ResolveShotDirection(Origin,Cursor,Intent,Muzzle);
                TestTrue(TEXT("Every family and cursor radius retains finite forward shot intent"),
                    !Shot.ContainsNaN() && FMath::Abs(Shot.Size()-1.0)<1.e-6 && FVector::DotProduct(Shot,Intended)>.5);
                if (Distance==8 && FVector::DotProduct((Cursor-Muzzle).GetSafeNormal(),Intended)<0) ++OldReversals;
                ++Cases;
            }
        }
    }
    TestEqual(TEXT("All six variants,24 headings,seven cursor radii are exercised"),Cases,6*24*7);
    TestEqual(TEXT("Old muzzle-to-near-cursor normalization reverses in every catalog fixture"),OldReversals,6*24);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05AimHeightTest,"ProjectONE.Aim.RegionConvergenceAndObstructionSegments",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05AimHeightTest::RunTest(const FString& Parameters)
{
    const FVector Origin(0,0,132),Muzzle(82,16,139),Intent=FVector::ForwardVector;
    const FVector Head(600,0,175),Leg(600,0,45);
    const FVector HeadRay=ONEAim::ResolveShotDirection(Origin,Head,Intent,Muzzle);
    const FVector LegRay=ONEAim::ResolveShotDirection(Origin,Leg,Intent,Muzzle);
    TestTrue(TEXT("Far head and leg targeting keep distinct actual target heights"),HeadRay.Z>0 && LegRay.Z<0);
    TestTrue(TEXT("Far region convergence reaches the original selected point"),
        HeadRay.Equals((Head-Muzzle).GetSafeNormal(),1.e-6) && LegRay.Equals((Leg-Muzzle).GetSafeNormal(),1.e-6));
    const FVector CloseTorso(145,0,116);
    TestTrue(TEXT("A valid forward standing torso keeps its real convergence before120cm"),
        ONEAim::ResolveShotDirection(Origin,CloseTorso,Intent,Muzzle).Equals((CloseTorso-Muzzle).GetSafeNormal(),1.e-6));
    const FVector BesideMuzzle(82.5,0,116);
    const FVector BesideRay=ONEAim::ResolveShotDirection(Origin,BesideMuzzle,Intent,Muzzle);
    TestTrue(TEXT("A point just beside the barrel retains bounded yaw/pitch and forward intent"),
        FVector::DotProduct(BesideRay,Intent)>.5 && FMath::Abs(BesideRay.Rotation().Pitch)<=35.001 && FMath::Abs(BesideRay.Rotation().Yaw)<=45.001);
    const FVector Extreme=ONEAim::ResolveShotDirection(Origin,FVector(0,0,1.e9),Intent,Muzzle);
    TestTrue(TEXT("Near-centre vertical extremes stay within the editable pitch bound"),
        FMath::Abs(Extreme.Rotation().Pitch)<=35.001 && FVector::DotProduct(Extreme,Intent)>.5);
    const FVector Invalid(std::numeric_limits<double>::quiet_NaN(),0,0);
    TestTrue(TEXT("Invalid projection preserves valid facing"),ONEAim::ResolveIntent(Origin,Invalid,Intent).Equals(Intent));
    TestTrue(TEXT("Invalid convergence cannot emit a NaN ray"),ONEAim::ResolveShotDirection(Origin,Invalid,Intent,Muzzle).Equals(Intent));
    TestFalse(TEXT("An obstruction behind the muzzle must not create a backward tracer"),ONEAim::IsForwardSegment(Muzzle,FVector(25,0,132),Intent));
    TestFalse(TEXT("A zero-length impact has no visible tracer segment"),ONEAim::IsForwardSegment(Muzzle,Muzzle,Intent));
    TestTrue(TEXT("A very close hit in front of the muzzle retains its tracer"),ONEAim::IsForwardSegment(Muzzle,Muzzle+Intent,Intent));
    TestTrue(TEXT("Deliberately aiming around the character allows world-negative directions"),
        ONEAim::IsForwardSegment(Muzzle,Muzzle-Intent*100,-Intent));
    return true;
}
#endif
