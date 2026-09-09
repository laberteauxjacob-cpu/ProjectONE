#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE06CombatTypes.h"
#include "ONE06SurvivalCheck.generated.h"

class AONEPlayer;
class AONEGameMode;
class AONEPowerUpPickup;
class AONEZombie;
class UONEHealthComponent;
class UONEPowerUpComponent;
class UONE06CaptureComponent;
class FJsonObject;
class FJsonValue;

UCLASS()
class PROJECTONE_API AONE06SurvivalCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE06SurvivalCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Check(bool Pass,const FString& Label);
    void Finish(bool Complete);
    void Observe();
    void Enter(int32 Next);
    AONEPowerUpPickup* Spawn(EONEPowerUpType Type,const FVector& Near);
    void PositionAt(AONEPowerUpPickup* Pickup,float HorizontalOffset=0.f);
    void StartCollection();
    void StartTimedCombat();
    void StartPauseFixture();
    void AwardFixture(bool Headshot);
    AONEPlayer* Player=nullptr;
    AONEGameMode* Mode=nullptr;
    UONEHealthComponent* Health=nullptr;
    UONEPowerUpComponent* Powers=nullptr;
    UPROPERTY() TObjectPtr<UONE06CaptureComponent> Capture;
    TWeakObjectPtr<AONEPowerUpPickup> Current,Expiring,DeadRejected;
    TWeakObjectPtr<AONEZombie> TimedTarget;
    TWeakObjectPtr<AActor> Wall;
    FVector OriginalPosition=FVector::ZeroVector;
    FString Folder,ChecksCsv;
    TArray<TSharedPtr<FJsonValue>> Observations;
    double StartedReal=0.,FinishedReal=0.,PhaseStarted=0.,DamageAt=0.,PausedReal=0.,PausedWorld=0.,DropAt=0.;
    float SnapshotHealth=0.f,PausedDelay=0.f,PausedInsta=0.f,PausedDouble=0.f,PausedLifetime=0.f;
    int32 Phase=0,Checks=0,Failures=0,CollectIndex=0,ShotsBefore=0,CasesBefore=0,MagazinesBefore=0;
    int32 TimedPointsBefore=0,TimedKillsBefore=0;
    uint64 NotificationBefore=0,ExpiredBefore=0;
    bool bFinished=false,bComplete=false,bPreDelayChecked=false;
    bool bCaptureRequested=false,bFinishing=false,bEndingPlay=false;
    FONECombatDischargeContext BeforeDeathContext;
};
