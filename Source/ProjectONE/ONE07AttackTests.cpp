#include "Misc/AutomationTest.h"
#include "ONEInfectedAttackDefinition.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07AttackDefinitionsTest,"ProjectONE.Candidate07.Attacks.ValidDefinitionsAndPreservedBudgets",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07AttackDefinitionsTest::RunTest(const FString&)
{
    const auto Definitions=ONEInfectedAttacks::Defaults();
    TestEqual(TEXT("Three families have five sided performances"),Definitions.Num(),5);
    TSet<FName> Ids;
    for (const auto& D:Definitions)
    {
        TestTrue(TEXT("Default profile validates before any asset load"),ONEInfectedAttacks::IsValid(D));
        TestFalse(TEXT("Every performance has a unique identity"),Ids.Contains(D.Id)); Ids.Add(D.Id);
        const int32 Family=int32(D.Family);
        const float Contacts[]={.45f,.48f,.54f},Durations[]={.96f,1.08f,1.12f},Budgets[]={18.f,12.f,14.f};
        TestEqual(TEXT("Contact anticipation retains Candidate06 timing"),D.ContactTime,Contacts[Family]);
        TestEqual(TEXT("Attack duration retains Candidate06 timing"),D.Duration,Durations[Family]);
        TestEqual(TEXT("Committed displacement retains Candidate06 budget"),D.StepDistanceCm,Budgets[Family]);
        TestEqual(TEXT("Start range does not increase"),D.MaxStartDistanceCm,88.f);
        TestTrue(TEXT("Recovery permits gait before action completes"),D.RecoveryLocomotionStart>D.ContactTime && D.RecoveryLocomotionStart<D.Duration);
    }
    auto Broken=Definitions[0]; Broken.RequiredLimbs=4;
    TestFalse(TEXT("Unknown limb mask rejected"),ONEInfectedAttacks::IsValid(Broken));
    Broken=Definitions.Last(); Broken.RequiredLimbs=1;
    TestFalse(TEXT("Two-handed performance cannot declare one arm"),ONEInfectedAttacks::IsValid(Broken));
    Broken=Definitions[0]; Broken.SteeringRelease=Broken.ContactTime-.01f;
    TestFalse(TEXT("Steering cannot unlock during the damaging swing"),ONEInfectedAttacks::IsValid(Broken));
    Broken=Definitions[0]; Broken.StepDistanceCm=100;
    TestFalse(TEXT("Unbounded step configuration rejected"),ONEInfectedAttacks::IsValid(Broken));
    Broken=Definitions[0]; Broken.SelectionWeight=std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Non-finite selection weight rejected"),ONEInfectedAttacks::IsValid(Broken));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07AttackSelectionTest,"ProjectONE.Candidate07.Attacks.ContextLimbsAndIndependentSelection",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07AttackSelectionTest::RunTest(const FString&)
{
    const auto Definitions=ONEInfectedAttacks::Defaults();
    FONEInfectedAttackContext Context; Context.DistanceCm=80; Context.ForwardSpeed=195;
    FRandomStream A(7071),B(7071); TSet<FName> Seen;
    double NoiseSink=0;
    for (int32 I=0;I<128;++I)
    {
        const int32 First=ONEInfectedAttacks::Select(Definitions,Context,A);
        NoiseSink+=FMath::FRand(); NoiseSink+=FMath::Rand();
        const int32 Second=ONEInfectedAttacks::Select(Definitions,Context,B);
        TestEqual(TEXT("Global random traffic cannot perturb private attack selection"),First,Second);
        if (!Definitions.IsValidIndex(First)) { AddError(TEXT("Eligible complete pool unexpectedly empty")); return false; }
        const auto& Chosen=Definitions[First];
        TestTrue(TEXT("Selected action is actually eligible"),ONEInfectedAttacks::IsEligible(Chosen,Context));
        TestTrue(TEXT("An eligible alternative prevents immediate repetition"),Chosen.Id!=Context.PreviousPerformance);
        Seen.Add(Chosen.Id); Context.PreviousPerformance=Chosen.Id;
    }
    TestTrue(TEXT("Noise results consumed without changing test stream"),FMath::IsFinite(NoiseSink));
    TestEqual(TEXT("All five compatible performances are reachable"),Seen.Num(),5);
    Context.PreviousPerformance=NAME_None; Context.AvailableLimbs=ONEInfectedLimbs::LeftArm;
    for (int32 I=0;I<24;++I)
    {
        const int32 Index=ONEInfectedAttacks::Select(Definitions,Context,A);
        TestTrue(TEXT("Left-only context retains a valid one-arm action"),Definitions.IsValidIndex(Index));
        if (Definitions.IsValidIndex(Index)) TestEqual(TEXT("No removed arm is silently substituted"),Definitions[Index].RequiredLimbs,int32(ONEInfectedLimbs::LeftArm));
    }
    Context.PreferredFamily=int32(EONEInfectedAttackFamily::TwoHand);
    const int32 Seed=A.GetCurrentSeed();
    TestEqual(TEXT("Forced two-hand preference still obeys limbs"),ONEInfectedAttacks::Select(Definitions,Context,A),INDEX_NONE);
    TestEqual(TEXT("Empty pool does not consume selection randomness"),A.GetCurrentSeed(),Seed);
    Context=FONEInfectedAttackContext(); Context.DistanceCm=88.01f;
    TestEqual(TEXT("Out of range never extends start reach"),ONEInfectedAttacks::Select(Definitions,Context,A),INDEX_NONE);
    Context.DistanceCm=80; Context.BearingDegrees=90;
    TestEqual(TEXT("Behind committed-facing eligibility does not snap attack"),ONEInfectedAttacks::Select(Definitions,Context,A),INDEX_NONE);
    Context.BearingDegrees=0; Context.bGrounded=false;
    TestEqual(TEXT("Falling or airborne actor cannot select"),ONEInfectedAttacks::Select(Definitions,Context,A),INDEX_NONE);
    Context.bGrounded=true; Context.bCanCommit=false;
    TestEqual(TEXT("Recovery state cannot select prematurely"),ONEInfectedAttacks::Select(Definitions,Context,A),INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07AttackMomentumTest,"ProjectONE.Candidate07.Attacks.MomentumBudgetAndRecoveryContinuity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07AttackMomentumTest::RunTest(const FString&)
{
    for (const auto& D:ONEInfectedAttacks::Defaults()) for (float Entry:{0.f,35.f,100.f,195.f})
    {
        TestTrue(TEXT("Entry forward momentum is preserved"),FMath::IsNearlyEqual(ONEInfectedAttacks::StepSpeed(D,0,Entry),Entry,.001f));
        float Previous=0,Integral=0,Maximum=0;
        for (int32 I=0;I<1000;++I)
        {
            const float Age=(I+1)*D.ContactTime/1000.f;
            const float Position=ONEInfectedAttacks::StepPosition(D,Age,Entry);
            const float Speed=ONEInfectedAttacks::StepSpeed(D,(I+.5f)*D.ContactTime/1000.f,Entry);
            Integral+=Speed*D.ContactTime/1000.f; Maximum=FMath::Max(Maximum,Speed);
            if (Position+.0001f<Previous || Position>D.StepDistanceCm+.0001f) { AddError(TEXT("Step lost monotonicity or exceeded budget")); return false; }
            Previous=Position;
        }
        TestTrue(TEXT("Integrated velocity matches original displacement budget"),FMath::IsNearlyEqual(Integral,D.StepDistanceCm,.002f));
        TestTrue(TEXT("No hidden speed above ordinary pursuit"),Maximum<=195.001f);
        TestTrue(TEXT("Hitch-spanning displacement remains exact and bounded"),FMath::IsNearlyEqual(ONEInfectedAttacks::StepDelta(D,-.1f,2.f,Entry),D.StepDistanceCm,.001f));
        TestEqual(TEXT("No backward-time motion"),ONEInfectedAttacks::StepDelta(D,.3f,.2f,Entry),0.f);
        TestEqual(TEXT("No locomotion blend at damaging contact"),ONEInfectedAttacks::RecoveryAlpha(D,D.ContactTime),0.f);
        TestTrue(TEXT("Recovery has meaningful locomotion before completion"),ONEInfectedAttacks::RecoveryAlpha(D,(D.RecoveryLocomotionStart+D.Duration)*.5f)>.49f);
        TestEqual(TEXT("Recovery exits into full locomotion"),ONEInfectedAttacks::RecoveryAlpha(D,D.Duration),1.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07PhaseEventsTest,"ProjectONE.Candidate07.Animation.ExplicitTimeEventBoundariesAndReset",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07PhaseEventsTest::RunTest(const FString&)
{
    FONEInfectedPhaseMarker Right,Left; Right.Event=EONEInfectedMotionEvent::RightFootPlant;
    Left.TimeSeconds=.5f; Left.Event=EONEInfectedMotionEvent::LeftFootPlant;
    TArray<FONEInfectedPhaseMarker> Markers={Right,Left}; FONEInfectedPhaseCursor Cursor;
    auto Advance=[&](double T,bool Enabled=true,int32 Cap=4){return ONEInfectedAttacks::AdvanceEvents(Cursor,Markers,T,1.f,true,Enabled,Cap);};
    TestEqual(TEXT("First sample never invents an initial footstep"),Advance(.1).Num(),0);
    auto Events=Advance(.5); TestEqual(TEXT("Exact phase boundary emits once"),Events.Num(),1);
    if (Events.Num()) TestTrue(TEXT("Correct anatomical foot emitted"),Events[0].Event==EONEInfectedMotionEvent::LeftFootPlant);
    TestEqual(TEXT("Same explicit pose does not emit again"),Advance(.5).Num(),0);
    Events=Advance(1.05); TestEqual(TEXT("Loop crossing emits phase zero once"),Events.Num(),1);
    if (Events.Num()) TestEqual(TEXT("Loop identity advances"),Events[0].Cycle,uint64(1));
    const uint64 Generation=Cursor.Generation;
    TestEqual(TEXT("Disabled stationary/fallen gait emits nothing"),Advance(5,false).Num(),0);
    TestTrue(TEXT("Disable invalidates old event identity"),Cursor.Generation>Generation);
    TestEqual(TEXT("Resume primes without replaying hidden loops"),Advance(5).Num(),0);
    TestEqual(TEXT("Rewinding an action resets instead of traversing backward"),Advance(.1).Num(),0);
    TestTrue(TEXT("Long hitch event work is bounded to the latest loop"),Advance(1000.6).Num()<=2);
    TestEqual(TEXT("Repeated hitch endpoint stays silent"),Advance(1000.6).Num(),0);
    Cursor.Reset(); Advance(.1); Events=Advance(.9,true,1);
    TestEqual(TEXT("Explicit event cap applies"),Events.Num(),1);
    Cursor.Reset();
    auto Attack=ONEInfectedAttacks::Defaults()[0];
    ONEInfectedAttacks::AdvanceEvents(Cursor,Attack.Events,0,Attack.Duration,false,true);
    Events=ONEInfectedAttacks::AdvanceEvents(Cursor,Attack.Events,5,Attack.Duration,false,true);
    TestEqual(TEXT("Effort and foot marker survive a hitch across the entire clip"),Events.Num(),2);
    TestEqual(TEXT("Completed action cannot repeat its cue"),ONEInfectedAttacks::AdvanceEvents(Cursor,Attack.Events,6,Attack.Duration,false,true).Num(),0);
    return true;
}
#endif
