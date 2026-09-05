#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/PoseSnapshot.h"
#include "ONEAnimInstance.generated.h"
class UAnimSequence;

/** Native authored-sequence graph. Direction cycles share phase; actions layer at spine_01. */
UCLASS(Transient, Blueprintable)
class PROJECTONE_API UONEAnimInstance : public UAnimInstance
{
    GENERATED_BODY()
public:
    UONEAnimInstance();
    virtual void NativeInitializeAnimation() override;
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
    UAnimSequence* FindClip(FName Key) const;
    UPROPERTY(Transient) FPoseSnapshot CapturedDeathPose;
private:
    UPROPERTY(Transient) TMap<FName,TObjectPtr<UAnimSequence>> Clips;
};
