#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEWeaponTypes.h"
#include "ONEInteractionComponent.generated.h"
class AONEPlayer;
class AONEProgressionMachine;

enum class EONEInteractionAction : uint8 { None, BuyBox, CollectBox, DepositUpgrade, CollectUpgrade };
struct FONEInteractionOffer
{
    TWeakObjectPtr<AONEProgressionMachine> Machine;
    EONEInteractionAction Action=EONEInteractionAction::None;
    FString Title,Detail;
    bool bEnabled=false;
    int32 Price=0,Slot=INDEX_NONE;
    uint64 Epoch=0,RunId=0,Revision=0,InstanceId=0;
    FONEWeaponAcquisitionPlan Acquisition;
    bool SameContext(const FONEInteractionOffer& B) const
    {
        return Machine==B.Machine && Action==B.Action && Epoch==B.Epoch && RunId==B.RunId &&
            Revision==B.Revision && InstanceId==B.InstanceId && Slot==B.Slot && Acquisition==B.Acquisition;
    }
};

/** One production input path for both machines; a completed/canceled press
 * cannot become another action until its release edge has arrived. */
UCLASS(ClassGroup=(ONE))
class PROJECTONE_API UONEInteractionComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEInteractionComponent();
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    void Press();
    void Release();
    void Cancel(bool bReleaseInput=false);
    FONEInteractionOffer FindOffer() const;
    const FONEInteractionOffer& GetOffer() const { return Focus; }
    float GetProgress() const { return HoldElapsed/FMath::Clamp(HoldDuration,.1f,2.f); }
    bool IsHeld() const { return bHeld; }
    bool RequiresRelease() const { return bLatched; }
    int32 GetCompletedHolds() const { return CompletedHolds; }
    static constexpr float HoldSeconds=.4f;
    UPROPERTY(EditAnywhere,Category="Interaction",meta=(ClampMin="0.1",ClampMax="2.0")) float HoldDuration=.4f;
private:
    FONEInteractionOffer Focus,Started;
    bool bHeld=false,bLatched=false;
    float HoldElapsed=0;
    int32 CompletedHolds=0;
};
