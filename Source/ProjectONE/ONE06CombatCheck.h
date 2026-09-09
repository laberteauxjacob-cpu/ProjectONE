#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponComponent.h"
#include "ONE06CombatCheck.generated.h"
class AONEPlayer;
class AONEPlayerController;
class AONEZombie;
class AONEGameMode;
class UONE06CaptureComponent;

/** Explicit scene fixtures, ordinary logical cursor and controller input.
 * Numerical traces are evidence of collision behavior, not native playtesting. */
UCLASS()
class PROJECTONE_API AONE06CombatCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE06CombatCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Enter(int32 Next);
    void Key(const FKey& Key,bool Down);
    void Release();
    void Aim(float Distance,float Height,float Side=0.f);
    void PrepareVariant();
    void PrepareTrial();
    void ReviewShot();
    void ClearScene();
    void Finish();
    AONEZombie* Target(float Distance,float Side,float Health);
    AActor* Wall(const FVector& Position,const FVector& Extent);
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEPlayerController> Controller;
    UPROPERTY() TObjectPtr<AONEGameMode> Mode;
    UPROPERTY() TObjectPtr<UONE06CaptureComponent> Capture;
    UPROPERTY() TArray<TObjectPtr<AActor>> Fixtures;
    TArray<FONEProjectilePath> Baseline;
    FVector Origin=FVector::ZeroVector,BaselineMuzzle=FVector::ZeroVector,BaselineAim=FVector::ZeroVector;
    int32 Phase=0,Variant=0,Trial=0,Checks=0,Failures=0,ShotsBefore=0,PointsBefore=0;
    int32 VariantLimit=6;
    float PhaseAt=0;
    double Started=0,FinishedAt=0;
    bool bFinished=false,bFinishing=false;
    FString Folder,Report,Csv;
};
