#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponTypes.h"
#include "ONE06PortabilityCheck.generated.h"

class AONEPlayer;
class AONEGameMode;
class AONEZombie;
class AONEPowerUpPickup;
class AONEMysteryBox;
class AONEUpgradeMachine;
class AONEProgressionMachine;
class UONE06CaptureComponent;
class FJsonValue;

/** Explicit second-map integration fixture, never spawned in ordinary play. */
UCLASS()
class PROJECTONE_API AONE06PortabilityCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE06PortabilityCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Check(bool Pass,const FString& Label);
    void Enter(int32 Next);
    void Key(const FKey& Input,bool Down);
    void Position(const FVector& XY);
    bool Approach(AONEProgressionMachine* Machine);
    bool Observe(float DeltaSeconds);
    void Finish(bool Complete);
    void WriteResults(bool Complete);
    UPROPERTY() TObjectPtr<UONE06CaptureComponent> Capture;
    AONEPlayer* Player=nullptr;
    AONEGameMode* Mode=nullptr;
    AONEMysteryBox* Box=nullptr;
    AONEUpgradeMachine* Upgrade=nullptr;
    TWeakObjectPtr<AONEZombie> Target;
    TWeakObjectPtr<AONEPowerUpPickup> Pickup;
    FVector PickupAnchor=FVector::ZeroVector;
    float FloorZ=0.f,InjuredHealth=0.f;
    FONEWeaponReservation Returned,Expired;
    uint64 ExpiryReceipt=0,NotificationsBefore=0;
    int32 Phase=0,Checks=0,Failures=0,ObservedFrames=0,PointsBefore=0,ShotsBefore=0,TapsBefore=0,HoldsBefore=0;
    int32 PointsAfterExpiryDeposit=0;
    double StartedReal=0.,FinishedReal=0.,PhaseAt=0.,DamageAt=-1.,FirstAcceptedAt=0.,SecondAcceptedAt=0.,ReadyAt=0.;
    bool bFinished=false,bFinishing=false,bFinishComplete=false,bCaptureRequested=false,bCaptureStarted=false,bCaptureFailureRecorded=false;
    bool bSawPreDelay=false,bSawRecovery=false,bRecovered=false,bSawReadyWarning=false;
    FString Folder,Timeline;
    TArray<TSharedPtr<FJsonValue>> Assertions;
};
