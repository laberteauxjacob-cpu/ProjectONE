#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/PoseSnapshot.h"
#include "ONEInfectedAttackDefinition.h"
#include "ONEInfectedAnimInstance.generated.h"

class UAnimSequence;

enum class EONEInfectedMotionState : uint8 { Locomotion, Attack, HeavyHit, Stumble, Falling, Fallen, GetUp, Dead };

/** A game-thread presentation snapshot. The actor remains authoritative for every transition. */
struct FONEInfectedAnimationState
{
    EONEInfectedMotionState Motion=EONEInfectedMotionState::Locomotion;
    float StateAgeSeconds=0.f;
    uint64 ActionSerial=0;
    FName ActionClip;
    float ActionDurationSeconds=0.f;
    float ActionLeadInSeconds=0.f;
    float RecoveryLocomotionAlpha=0.f;
    float RecoverySnapshotAlpha=0.f;
    float GaitPhaseOffset=0.f;
    float EntryForwardSpeed=0.f;
    float CommittedTravelCm=0.f;
    FVector ContactDirectionWorld=FVector::ZeroVector;
    float ContactStrength=0.f;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FONEInfectedFootContactDelegate,bool /* bAnatomicalLeft */);
DECLARE_MULTICAST_DELEGATE_OneParam(FONEInfectedAttackEffortDelegate,int32 /* Family */);

/** Infected-only authored graph. No player pivots, weapon actions or Response assets. */
UCLASS(Transient, Blueprintable)
class PROJECTONE_API UONEInfectedAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UONEInfectedAnimInstance();
    virtual void NativeInitializeAnimation() override;
    virtual void NativePostEvaluateAnimation() override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    void ConfigureClips(const TMap<FName,TSoftObjectPtr<UAnimSequence>>& Overrides);
    UAnimSequence* FindClip(FName Key) const;
    bool HasRequiredClips() const;
    void ResetMotionEvents();
    void QueueFootContact(bool bAnatomicalLeft);
    FONEInfectedFootContactDelegate OnFootContact;
    FONEInfectedAttackEffortDelegate OnAttackEffort;
    UPROPERTY(Transient) FPoseSnapshot CapturedDeathPose;
    UPROPERTY(Transient) FPoseSnapshot CapturedRecoveryPose;
    // Read by the proxy; actor transitions invalidate both pending and loop identities.
    uint64 MotionEventGeneration=1;
private:
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UAnimSequence>> Clips;
    UPROPERTY(Transient) TMap<FName,TSoftObjectPtr<UAnimSequence>> ClipOverrides;
    uint8 PendingFeet=0;
    double LastFootTime[2]={-100.,-100.};
    double LastMovingTime=-100.;
};
