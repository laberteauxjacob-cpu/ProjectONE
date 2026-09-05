#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEValidation.generated.h"
class AONEZombie;
UCLASS()
class PROJECTONE_API AONEValidation : public AActor
{
    GENERATED_BODY()
public:
    AONEValidation();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass, const FString& Label);
    void Capture(const FString& Label);
    void SaveAndExit();
    AONEZombie* SpawnTest(const FVector& Location);
    void Hit(AONEZombie* Zombie, FName Bone, float Damage);
    float Elapsed=0, StageTime=0;
    float LastRecordFrame=-1;
    int32 Stage=0, Failed=0, BenchmarkCount=0;
    int32 AmmoBefore=0, ReserveBefore=0, PointsBefore=0;
    float HealthBefore=0, InitialDistance=0;
    FVector InitialArmLocation=FVector::ZeroVector;
    FString Entries;
    TArray<float> FrameTimes;
    UPROPERTY() TObjectPtr<AONEZombie> ArmTest;
    UPROPERTY() TObjectPtr<AONEZombie> HeadTest;
    UPROPERTY() TObjectPtr<AONEZombie> IntactTest;
    UPROPERTY() TObjectPtr<AONEZombie> AttackTest;
};
