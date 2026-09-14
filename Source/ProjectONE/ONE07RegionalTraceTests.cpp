#include "Misc/AutomationTest.h"
#include "ONE07RegionalTrace.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07CapsuleIntersectionTest,"ProjectONE.Candidate07.RegionalTrace.ExactSegmentBoundaries",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07CapsuleIntersectionTest::RunTest(const FString&)
{
    using namespace ONE07RegionalTrace;
    FCapsuleContact Hit;
    const FVector A(0,0,-18),B(0,0,18);
    TestTrue(TEXT("Torso cylindrical middle accepts the original side-on segment"),IntersectCapsuleSegment(FVector(0,-80,0),FVector(0,80,0),A,B,14.,Hit));
    TestTrue(TEXT("Side-on entry uses the unexpanded 14 cm radius"),FMath::IsNearlyEqual(Hit.Time,66./160.,1.e-12));
    TestTrue(TEXT("Entry position and normal belong to that unchanged ray"),Hit.Position.Equals(FVector(0,-14,0),1.e-10) && Hit.Normal.Equals(FVector(0,-1,0),1.e-10));
    // Captured real failing torso ray from automation-fallen-third.log, with
    // the actual local ray and radius. This is the same cylinder-middle case,
    // not an adjusted ray selected to avoid the engine's numeric branch.
    const FVector ActualStart(7.6985258523620814e-07,-79.999999833744027,1.1729567717111422e-06);
    const FVector ActualDirection(-9.6231573154526024e-09,0.99999999792180039,-1.4661959646389278e-08);
    TestTrue(TEXT("Recorded side-on torso segment remains a hit"),IntersectCapsuleSegment(ActualStart,ActualStart+ActualDirection*160.,A,B,14.,Hit));
    TestTrue(TEXT("Recorded segment entry remains on its original line"),Hit.Position.Equals(ActualStart+ActualDirection*160.*Hit.Time,1.e-12));
    TestFalse(TEXT("A 0.0001 cm radial near miss is not inflated into contact"),IntersectCapsuleSegment(FVector(-80,14.0001,0),FVector(80,14.0001,0),A,B,14.,Hit));
    TestTrue(TEXT("Exact cylinder tangent is contact"),IntersectCapsuleSegment(FVector(-80,14,0),FVector(80,14,0),A,B,14.,Hit));
    TestTrue(TEXT("Tangent time is the geometric midpoint"),FMath::IsNearlyEqual(Hit.Time,.5,1.e-12));
    TestTrue(TEXT("Axial ray enters the bottom end sphere"),IntersectCapsuleSegment(FVector(0,0,-80),FVector(0,0,80),A,B,14.,Hit));
    TestTrue(TEXT("End sphere keeps the existing 32 cm total half-height"),Hit.Position.Equals(FVector(0,0,-32),1.e-10));
    TestFalse(TEXT("A segment ending before the surface does not hit"),IntersectCapsuleSegment(FVector(0,-80,0),FVector(0,-14.0001,0),A,B,14.,Hit));
    TestTrue(TEXT("A segment endpoint on the surface hits"),IntersectCapsuleSegment(FVector(0,-80,0),FVector(0,-14,0),A,B,14.,Hit));
    TestTrue(TEXT("Endpoint contact time is one"),FMath::IsNearlyEqual(Hit.Time,1.,1.e-12));
    TestTrue(TEXT("Inside origin returns one initial contact"),IntersectCapsuleSegment(FVector::ZeroVector,FVector(0,80,0),A,B,14.,Hit));
    TestTrue(TEXT("Inside origin records time zero and penetration"),Hit.bStartInside && Hit.Time==0. && Hit.PenetrationDepth==14.);
    TestFalse(TEXT("Zero-length segment does not invent a shot"),IntersectCapsuleSegment(FVector::ZeroVector,FVector::ZeroVector,A,B,14.,Hit));
    TestTrue(TEXT("Zero core length is a sphere"),IntersectCapsuleSegment(FVector(-20,0,0),FVector(20,0,0),FVector::ZeroVector,FVector::ZeroVector,8.,Hit));
    TestTrue(TEXT("Sphere entry remains at its existing radius"),Hit.Position.Equals(FVector(-8,0,0),1.e-10));
    for (const FTransform& Transform:{FTransform(FRotator(71,24,-38),FVector(3200,-1700,19)),
        FTransform(FRotator(-117,82,16),FVector(-9000,4000,35))})
    {
        const FVector S=Transform.TransformPosition(FVector(0,-80,0)),E=Transform.TransformPosition(FVector(0,80,0));
        TestTrue(TEXT("Rotated and translated capsule keeps its crossing hit"),IntersectCapsuleSegment(S,E,Transform.TransformPosition(A),Transform.TransformPosition(B),14.,Hit));
        TestTrue(TEXT("Rigid transform preserves entry time"),FMath::IsNearlyEqual(Hit.Time,66./160.,1.e-10));
        TestFalse(TEXT("Rotated near miss remains outside"),IntersectCapsuleSegment(Transform.TransformPosition(FVector(-80,14.0001,0)),
            Transform.TransformPosition(FVector(80,14.0001,0)),Transform.TransformPosition(A),Transform.TransformPosition(B),14.,Hit));
    }
    return true;
}
#endif
