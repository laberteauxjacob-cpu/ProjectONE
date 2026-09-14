#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONE07FootstepComponent.generated.h"

/** Player-only post-pose foot contacts; no animation graph or gait clock ownership. */
UCLASS(ClassGroup=(ONE))
class PROJECTONE_API UONE07FootstepComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONE07FootstepComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function) override;
    int32 GetFootContactCount() const { return FootContactCount; }
private:
    bool Raised[2]={false,false};
    float PreviousClearance[2]={0.f,0.f};
    double NextAllowed[2]={0.,0.};
    int32 FootContactCount=0;
};
