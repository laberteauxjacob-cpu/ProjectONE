#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/PoseSnapshot.h"
#include "ONESnapshotAnimInstance.generated.h"

/** Evaluated local pose drives independent part physics or a supported corpse rest. */
UCLASS(Transient)
class PROJECTONE_API UONESnapshotAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UONESnapshotAnimInstance();
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    UPROPERTY(Transient) FPoseSnapshot CapturedPose;
};
