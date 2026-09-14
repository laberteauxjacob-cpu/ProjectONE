#pragma once
#include "CoreMinimal.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "ONEPhysicalAnimationComponent.generated.h"

/** New late-frame spawns can tick before their first complete pose buffers.
 * The engine driver assumes these arrays already exist. Wait for that pose. */
UCLASS()
class PROJECTONE_API UONEPhysicalAnimationComponent : public UPhysicalAnimationComponent
{
    GENERATED_BODY()
public:
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* Tick) override;
private:
    int32 DeferredTicks=0;
};
