#include "ONEInfectedAttackDefinition.h"
#include "Animation/AnimSequence.h"

namespace ONEInfectedAttackDetails
{
    bool FiniteNonnegative(float Value) { return FMath::IsFinite(Value) && Value>=0.f; }
    float Entry(float Speed) { return FMath::IsFinite(Speed) ? FMath::Clamp(Speed,0.f,195.f) : 0.f; }
}

TArray<FONEInfectedAttackDefinition> ONEInfectedAttacks::Defaults()
{
    TArray<FONEInfectedAttackDefinition> Result;
    for (int32 Family=0;Family<3;++Family)
    {
        const int32 Count=Family==2 ? 1 : 2;
        for (int32 Side=0;Side<Count;++Side)
        {
            FONEInfectedAttackDefinition D;
            D.Family=EONEInfectedAttackFamily(Family);
            D.Id=FName(*FString::Printf(TEXT("%s%s"),Family==0?TEXT("Swipe"):Family==1?TEXT("Rake"):TEXT("TwoHand"),
                Family==2?TEXT(""):Side==0?TEXT("Left"):TEXT("Right")));
            D.AnimationKey=D.Id;
            const FString Asset=TEXT("A_Infected_C07_")+D.Id.ToString();
            D.Animation=TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/ONE/Animations/Candidate07/")+Asset+TEXT(".")+Asset));
            D.RequiredLimbs=Family==2 ? ONEInfectedLimbs::BothArms : Side==0 ? ONEInfectedLimbs::LeftArm : ONEInfectedLimbs::RightArm;
            // Unreal anatomical left is negative local Y. This is independent of bone naming.
            D.PreferredBearingDegrees=Family==2 ? 0.f : Side==0 ? -25.f : 25.f;
            D.PreferredEntrySpeed=Family==0?160.f:Family==1?90.f:25.f;
            D.SelectionWeight=Family==2?1.25f:1.f;
            if (Family==1)
            { D.ContactTime=.48f; D.Duration=1.08f; D.StepDistanceCm=12.f; D.StepEnd=.34f; D.FollowThroughEnd=.68f; D.RecoveryLocomotionStart=.75f; D.SteeringRelease=.75f; }
            if (Family==2)
            { D.ContactTime=.54f; D.Duration=1.12f; D.StepDistanceCm=14.f; D.StepEnd=.38f; D.FollowThroughEnd=.74f; D.RecoveryLocomotionStart=.80f; D.SteeringRelease=.80f; }
            FONEInfectedPhaseMarker Effort; Effort.TimeSeconds=.12f; Effort.Event=EONEInfectedMotionEvent::AttackEffort;
            D.Events.Add(Effort);
            FONEInfectedPhaseMarker Plant; Plant.TimeSeconds=D.StepEnd;
            Plant.Event=Family==2 || Side==0?EONEInfectedMotionEvent::LeftFootPlant:EONEInfectedMotionEvent::RightFootPlant;
            D.Events.Add(Plant);
            Result.Add(D);
        }
    }
    return Result;
}

bool ONEInfectedAttacks::IsValid(const FONEInfectedAttackDefinition& D)
{
    using namespace ONEInfectedAttackDetails;
    if (D.Id.IsNone() || D.AnimationKey.IsNone() || uint8(D.Family)>uint8(EONEInfectedAttackFamily::TwoHand) ||
        D.RequiredLimbs<=0 || (D.RequiredLimbs&~int32(ONEInfectedLimbs::BothArms))!=0 ||
        (D.Family==EONEInfectedAttackFamily::TwoHand ? D.RequiredLimbs!=3 : D.RequiredLimbs==3) ||
        !FiniteNonnegative(D.MinStartDistanceCm) || !FiniteNonnegative(D.MaxStartDistanceCm) ||
        D.MinStartDistanceCm>D.MaxStartDistanceCm || D.MaxStartDistanceCm>88.f ||
        !FMath::IsFinite(D.MaxBearingDegrees) || D.MaxBearingDegrees<=0 || D.MaxBearingDegrees>90.f ||
        !FMath::IsFinite(D.PreferredBearingDegrees) || FMath::Abs(D.PreferredBearingDegrees)>D.MaxBearingDegrees ||
        !FiniteNonnegative(D.PreferredEntrySpeed) || D.PreferredEntrySpeed>195.f ||
        !FMath::IsFinite(D.SelectionWeight) || D.SelectionWeight<=0 ||
        !FiniteNonnegative(D.ContactTime) || D.ContactTime<=0 || !FiniteNonnegative(D.FollowThroughEnd) ||
        !FiniteNonnegative(D.RecoveryLocomotionStart) || !FiniteNonnegative(D.SteeringRelease) ||
        !FiniteNonnegative(D.Duration) || D.Duration<D.SteeringRelease || D.Duration<=D.RecoveryLocomotionStart ||
        D.ContactTime>D.FollowThroughEnd || D.FollowThroughEnd>D.RecoveryLocomotionStart ||
        D.SteeringRelease<D.RecoveryLocomotionStart || !FiniteNonnegative(D.StepDistanceCm) || D.StepDistanceCm<=0 || D.StepDistanceCm>18.f ||
        !FiniteNonnegative(D.StepEnd) || D.StepEnd<=0 || D.StepEnd>D.ContactTime || 1.5f*D.StepDistanceCm/D.StepEnd>195.f) return false;
    float Last=-1;
    for (const auto& Marker:D.Events)
    {
        if (!FiniteNonnegative(Marker.TimeSeconds) || Marker.TimeSeconds<Last || Marker.TimeSeconds>D.Duration ||
            uint8(Marker.Event)>uint8(EONEInfectedMotionEvent::Cloth)) return false;
        Last=Marker.TimeSeconds;
    }
    return true;
}

bool ONEInfectedAttacks::IsEligible(const FONEInfectedAttackDefinition& D,const FONEInfectedAttackContext& C)
{
    return IsValid(D) && C.bGrounded && C.bCanCommit && FMath::IsFinite(C.DistanceCm) && FMath::IsFinite(C.BearingDegrees) &&
        FMath::IsFinite(C.ForwardSpeed) && (C.AvailableLimbs&D.RequiredLimbs)==D.RequiredLimbs &&
        C.DistanceCm>=D.MinStartDistanceCm && C.DistanceCm<=D.MaxStartDistanceCm && FMath::Abs(C.BearingDegrees)<=D.MaxBearingDegrees &&
        (C.PreferredFamily==INDEX_NONE || C.PreferredFamily==int32(D.Family));
}

int32 ONEInfectedAttacks::Select(TConstArrayView<FONEInfectedAttackDefinition> Definitions,const FONEInfectedAttackContext& C,FRandomStream& Random)
{
    bool Alternative=false;
    for (const auto& D:Definitions) if (IsEligible(D,C) && D.Id!=C.PreviousPerformance) { Alternative=true; break; }
    TArray<TPair<int32,double>,TInlineAllocator<8>> Pool;
    double Total=0;
    for (int32 I=0;I<Definitions.Num();++I)
    {
        const auto& D=Definitions[I];
        if (!IsEligible(D,C) || (Alternative && D.Id==C.PreviousPerformance)) continue;
        const float BearingFit=1.f-FMath::Clamp(FMath::Abs(C.BearingDegrees-D.PreferredBearingDegrees)/90.f,0.f,1.f);
        const float SpeedFit=1.f-FMath::Clamp(FMath::Abs(C.ForwardSpeed-D.PreferredEntrySpeed)/195.f,0.f,1.f);
        const double Weight=double(D.SelectionWeight)*(1.+.65*BearingFit+.5*SpeedFit);
        Total+=Weight; Pool.Emplace(I,Total);
    }
    if (Pool.IsEmpty() || !FMath::IsFinite(Total) || Total<=0) return INDEX_NONE;
    const double Draw=Random.FRand()*Total;
    for (const auto& Item:Pool) if (Draw<Item.Value) return Item.Key;
    return Pool.Last().Key;
}

float ONEInfectedAttacks::EffectiveStepEnd(const FONEInfectedAttackDefinition& D,float EntryForwardSpeed)
{
    if (!IsValid(D)) return 0;
    const float Speed=ONEInfectedAttackDetails::Entry(EntryForwardSpeed);
    // Limiting the initial Hermite tangent to 3 preserves monotonicity. Shorten
    // braking instead of discarding entry momentum or increasing displacement.
    return Speed>0 ? FMath::Min(D.StepEnd,3.f*D.StepDistanceCm/Speed) : D.StepEnd;
}
float ONEInfectedAttacks::StepPosition(const FONEInfectedAttackDefinition& D,float Age,float EntryForwardSpeed)
{
    const float End=EffectiveStepEnd(D,EntryForwardSpeed);
    if (End<=0 || !FMath::IsFinite(Age) || Age<=0) return 0;
    const float U=FMath::Clamp(Age/End,0.f,1.f);
    const float Tangent=ONEInfectedAttackDetails::Entry(EntryForwardSpeed)*End/D.StepDistanceCm;
    return D.StepDistanceCm*((-2*U+3)*U*U+(U*U*U-2*U*U+U)*Tangent);
}
float ONEInfectedAttacks::StepSpeed(const FONEInfectedAttackDefinition& D,float Age,float EntryForwardSpeed)
{
    const float End=EffectiveStepEnd(D,EntryForwardSpeed);
    if (End<=0 || !FMath::IsFinite(Age) || Age<0 || Age>=End) return 0;
    const float U=Age/End, Tangent=ONEInfectedAttackDetails::Entry(EntryForwardSpeed)*End/D.StepDistanceCm;
    return FMath::Max(0.f,D.StepDistanceCm/End*((-6*U+6)*U+(3*U*U-4*U+1)*Tangent));
}
float ONEInfectedAttacks::StepDelta(const FONEInfectedAttackDefinition& D,float PreviousAge,float Age,float EntryForwardSpeed)
{
    if (!FMath::IsFinite(PreviousAge) || !FMath::IsFinite(Age) || Age<=PreviousAge) return 0;
    return FMath::Max(0.f,StepPosition(D,Age,EntryForwardSpeed)-StepPosition(D,PreviousAge,EntryForwardSpeed));
}
float ONEInfectedAttacks::RecoveryAlpha(const FONEInfectedAttackDefinition& D,float Age)
{
    if (!IsValid(D) || !FMath::IsFinite(Age)) return 0;
    const float U=FMath::Clamp((Age-D.RecoveryLocomotionStart)/(D.Duration-D.RecoveryLocomotionStart),0.f,1.f);
    return U*U*(3.f-2.f*U);
}

TArray<FONEInfectedCrossedEvent> ONEInfectedAttacks::AdvanceEvents(FONEInfectedPhaseCursor& C,
    TConstArrayView<FONEInfectedPhaseMarker> Markers,double Time,float Duration,bool bLoop,bool bEnabled,int32 MaxEvents)
{
    TArray<FONEInfectedCrossedEvent> Result;
    if (!bEnabled || !FMath::IsFinite(Time) || Time<0 || !FMath::IsFinite(Duration) || Duration<=0 || MaxEvents<=0)
    { if (C.bPrimed) C.Reset(); return Result; }
    MaxEvents=FMath::Clamp(MaxEvents,1,8);
    if (!C.bPrimed || Time<C.PreviousTime)
    { if (C.bPrimed) C.Reset(); C.PreviousTime=Time; C.bPrimed=true; return Result; }
    const double Previous=C.PreviousTime; C.PreviousTime=Time;
    if (Time==Previous) return Result;
    // At most two cycle indices need visiting: the current partial loop and
    // the end of the preceding loop. Never iterate over a long hitch's history.
    const double Start=bLoop ? FMath::Max(Previous,Time-double(Duration)) : FMath::Min(Previous,double(Duration));
    const double End=bLoop ? Time : FMath::Min(Time,double(Duration));
    if (bLoop && Time/double(Duration)>double(MAX_int32)) return Result;
    const int64 FirstCycle=bLoop ? int64(FMath::FloorToDouble(Start/Duration)) : 0;
    const int64 LastCycle=bLoop ? int64(FMath::FloorToDouble(End/Duration)) : 0;
    for (int64 Cycle=FirstCycle;Cycle<=LastCycle;++Cycle)
    {
        for (int32 I=0;I<Markers.Num();++I)
        {
            const auto& Marker=Markers[I];
            if (!FMath::IsFinite(Marker.TimeSeconds) || Marker.TimeSeconds<0 || Marker.TimeSeconds>Duration ||
                (bLoop && Marker.TimeSeconds==Duration) || uint8(Marker.Event)>uint8(EONEInfectedMotionEvent::Cloth)) continue;
            const double EventTime=double(Cycle)*Duration+Marker.TimeSeconds;
            if (EventTime>Start && EventTime<=End)
            {
                if (Result.Num()<MaxEvents) Result.Add({Marker.Event,I,uint64(Cycle),C.Generation});
                else ++C.DroppedEvents;
            }
        }
    }
    return Result;
}
