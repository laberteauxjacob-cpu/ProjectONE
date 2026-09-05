#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEInteractionComponent.h"
#include "ONEProgressionMachine.generated.h"
class UBoxComponent;
class UONE04MachinePresentation;
class AONEPlayer;

UENUM()
enum class EONEMachineState : uint8 { Idle, Handoff, Active, Ready, Collecting, Closing, Disabled };

/** Stateful physical machines. Payment and reservation are committed together
 * on the game thread; weak owner/run/instance identities guard every later event. */
UCLASS(Abstract)
class PROJECTONE_API AONEProgressionMachine : public AActor
{
    GENERATED_BODY()
public:
    AONEProgressionMachine();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool CanReach(const AONEPlayer* Player) const;
    bool CanContact(const AONEPlayer* Player) const;
    FVector GetInteractionPoint() const;
    FONEInteractionOffer BuildOffer(AONEPlayer* Player) const;
    bool CommitOffer(AONEPlayer* Player,const FONEInteractionOffer& Offer);
    void CancelUnacceptedAction(AONEPlayer* Player);
    void InvalidateRun();
    void RecoverTechnicalFailure();
    bool IsBox() const { return bIsBox; }
    EONEMachineState GetState() const { return State; }
    float GetStateElapsed() const { return StateElapsed; }
    EONEWeaponFamily GetRewardFamily() const { return RewardFamily; }
    const FONEWeaponReservation& GetReservation() const { return Reservation; }
    uint64 GetPaymentReceipt() const { return PaymentReceipt; }
    int32 GetAcceptedCount() const { return AcceptedCount; }
    int32 GetDeliveredCount() const { return DeliveredCount; }
    UONE04MachinePresentation* GetPresentation() const { return Presentation; }
    static constexpr int32 BoxPrice=950;
    static constexpr int32 UpgradePrice=5000;
    static constexpr float BoxSeconds=5.f;
    static constexpr float UpgradeSeconds=9.f;
    UPROPERTY(EditAnywhere,Category="Machine",meta=(ClampMin="3",ClampMax="8")) float RollDuration=5.f;
    UPROPERTY(EditAnywhere,Category="Machine",meta=(ClampMin="8",ClampMax="10")) float ProcessingDuration=9.f;
    /** X=M1911, Y=M4A1, Z=Remington 870. Reserved families remain excluded. */
    UPROPERTY(EditAnywhere,Category="Machine",meta=(ClampMin="0")) FVector RollWeights=FVector(1,1,1);
protected:
    bool bIsBox=true;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UBoxComponent> Collision;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONE04MachinePresentation> Presentation;
private:
    void SetState(EONEMachineState NewState);
    void UpdatePresentation();
    void AcceptUpgrade();
    void FinishAction();
    bool IsCurrentOwner() const;
    bool CanDeposit(AONEPlayer* Player,FString& Reason) const;
    float RollWeight(EONEWeaponFamily Family) const;
    EONEMachineState State=EONEMachineState::Idle;
    float StateElapsed=0,NextCycle=0,ActionReleaseAt=0,ActiveDuration=5.f;
    uint64 Epoch=1,OwnerRunId=0,HandoffRevision=0,PaymentReceipt=0;
    int32 HandoffSlot=INDEX_NONE,CycleIndex=0,AcceptedCount=0,DeliveredCount=0;
    uint64 HandoffInstance=0;
    EONEWeaponFamily RewardFamily=EONEWeaponFamily::Invalid;
    TArray<EONEWeaponFamily> RollPool;
    TWeakObjectPtr<AONEPlayer> Customer;
    FONEWeaponReservation Reservation;
    bool bDelivered=false,bInvalidated=false,bCollectedVisual=false,bOwnsAction=false,bOutputVariant=false;
};

UCLASS()
class PROJECTONE_API AONEMysteryBox : public AONEProgressionMachine
{
    GENERATED_BODY()
public:
    AONEMysteryBox();
};
UCLASS()
class PROJECTONE_API AONEUpgradeMachine : public AONEProgressionMachine
{
    GENERATED_BODY()
public:
    AONEUpgradeMachine();
};
