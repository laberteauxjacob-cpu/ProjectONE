#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEPresentationCheck.generated.h"
class AONEPlayer;
class AONEZombie;

/** Opt-in recorded runtime choreography; never instantiated during ordinary play. */
UCLASS()
class PROJECTONE_API AONEPresentationCheck : public AActor
{
    GENERATED_BODY()
public:
    AONEPresentationCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Capture(const FString& Name);
    void Finish();
    void EndPhase();
    AONEZombie* SpawnEnemy(const FVector& Position);
    int32 CurrentPhase=-1,Failures=0,SpeedSamples=0,RecordCount=0,AttackStage=0,PhaseAmmoStart=0;
    float Elapsed=0,PhaseStart=0,FirstRecord=-1,LastRecord=-100,FootTravel=0,SpeedSum=0,HealthBefore=0;
    bool bPhaseCaptured=false,bCrowdSpawned=false,bComplete=false;
    bool bCrowdDamageChecked=false;
    bool bModularSampled=false;
    TArray<FVector> ModularStart;
    TArray<float> ModularTravel;
    FVector FirstLeftFoot,FirstRightFoot;
    FString Report,FrameReport;
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEZombie> Attacker;
    UPROPERTY() TArray<TObjectPtr<AONEZombie>> Crowd;
};
