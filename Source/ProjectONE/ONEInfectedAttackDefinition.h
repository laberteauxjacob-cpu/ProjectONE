#pragma once
#include "CoreMinimal.h"
#include "ONEInfectedAttackDefinition.generated.h"

class UAnimSequence;

UENUM(BlueprintType)
enum class EONEInfectedAttackFamily : uint8 { Swipe, Rake, TwoHand };

UENUM(BlueprintType)
enum class EONEInfectedMotionEvent : uint8 { AttackEffort, LeftFootPlant, RightFootPlant, Cloth };

/** Anatomical masks deliberately do not encode the imported skeleton suffixes. */
namespace ONEInfectedLimbs
{
    constexpr uint8 LeftArm=1, RightArm=2, BothArms=3;
}

USTRUCT(BlueprintType)
struct FONEInfectedPhaseMarker
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) float TimeSeconds=0.f;
    UPROPERTY(EditAnywhere) EONEInfectedMotionEvent Event=EONEInfectedMotionEvent::AttackEffort;
};

/** One performed attack, not one enemy subclass. All defaults retain C06 combat tuning. */
USTRUCT(BlueprintType)
struct FONEInfectedAttackDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName Id;
    UPROPERTY(EditAnywhere) FName AnimationKey;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> Animation;
    UPROPERTY(EditAnywhere) EONEInfectedAttackFamily Family=EONEInfectedAttackFamily::Swipe;
    UPROPERTY(EditAnywhere,meta=(ClampMin="1",ClampMax="3")) int32 RequiredLimbs=ONEInfectedLimbs::LeftArm;
    UPROPERTY(EditAnywhere) float MinStartDistanceCm=0.f;
    UPROPERTY(EditAnywhere,meta=(ClampMax="88")) float MaxStartDistanceCm=88.f;
    UPROPERTY(EditAnywhere) float MaxBearingDegrees=65.f;
    UPROPERTY(EditAnywhere) float PreferredBearingDegrees=0.f;
    UPROPERTY(EditAnywhere) float PreferredEntrySpeed=100.f;
    UPROPERTY(EditAnywhere) float SelectionWeight=1.f;
    UPROPERTY(EditAnywhere) float ContactTime=.45f;
    UPROPERTY(EditAnywhere) float FollowThroughEnd=.61f;
    UPROPERTY(EditAnywhere) float RecoveryLocomotionStart=.67f;
    UPROPERTY(EditAnywhere) float SteeringRelease=.67f;
    UPROPERTY(EditAnywhere) float Duration=.96f;
    UPROPERTY(EditAnywhere) float StepDistanceCm=18.f;
    UPROPERTY(EditAnywhere) float StepEnd=.34f;
    UPROPERTY(EditAnywhere) TArray<FONEInfectedPhaseMarker> Events;
};

struct FONEInfectedAttackContext
{
    uint8 AvailableLimbs=ONEInfectedLimbs::BothArms;
    float DistanceCm=0.f;
    float BearingDegrees=0.f;
    float ForwardSpeed=0.f;
    bool bGrounded=true;
    bool bCanCommit=true;
    FName PreviousPerformance;
    int32 PreferredFamily=INDEX_NONE; // Explicit fixture preference still obeys limb/context eligibility.
};

struct FONEInfectedCrossedEvent
{
    EONEInfectedMotionEvent Event=EONEInfectedMotionEvent::AttackEffort;
    int32 MarkerIndex=INDEX_NONE;
    uint64 Cycle=0;
    uint64 Generation=0;
};

/** An explicit-time event cursor: reset/disabled/rewound clocks never replay old events. */
struct FONEInfectedPhaseCursor
{
    double PreviousTime=0;
    uint64 Generation=1;
    int32 DroppedEvents=0;
    bool bPrimed=false;
    void Reset() { PreviousTime=0; bPrimed=false; DroppedEvents=0; ++Generation; }
};

namespace ONEInfectedAttacks
{
    PROJECTONE_API TArray<FONEInfectedAttackDefinition> Defaults();
    PROJECTONE_API bool IsValid(const FONEInfectedAttackDefinition& Definition);
    PROJECTONE_API bool IsEligible(const FONEInfectedAttackDefinition& Definition,const FONEInfectedAttackContext& Context);
    // INDEX_NONE is an empty/invalid pool. An alternative eligible performance excludes immediate repetition.
    PROJECTONE_API int32 Select(TConstArrayView<FONEInfectedAttackDefinition> Definitions,const FONEInfectedAttackContext& Context,FRandomStream& Random);
    PROJECTONE_API float EffectiveStepEnd(const FONEInfectedAttackDefinition& Definition,float EntryForwardSpeed);
    PROJECTONE_API float StepPosition(const FONEInfectedAttackDefinition& Definition,float Age,float EntryForwardSpeed);
    PROJECTONE_API float StepSpeed(const FONEInfectedAttackDefinition& Definition,float Age,float EntryForwardSpeed);
    PROJECTONE_API float StepDelta(const FONEInfectedAttackDefinition& Definition,float PreviousAge,float Age,float EntryForwardSpeed);
    PROJECTONE_API float RecoveryAlpha(const FONEInfectedAttackDefinition& Definition,float Age);
    // Clock and markers use seconds (or both normalized turns with Duration=1).
    // First enabled sample primes without emitting. Long hitches retain only the latest loop and at most MaxEvents.
    PROJECTONE_API TArray<FONEInfectedCrossedEvent> AdvanceEvents(FONEInfectedPhaseCursor& Cursor,
        TConstArrayView<FONEInfectedPhaseMarker> Markers,double Time,float Duration,bool bLoop,bool bEnabled,int32 MaxEvents=4);
}
