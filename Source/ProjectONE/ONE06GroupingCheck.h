#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponComponent.h"
#include "ONE06GroupingCheck.generated.h"
class AONEPlayer;
class AONEPlayerController;
class AONEGameMode;
class AONEZombie;
class UONE06CaptureComponent;

/** Opt-in measured grouping fixture. Direct setup is declared; all shots use
 * ordinary logical cursor projection, controller input and weapon collision. */
UCLASS()
class PROJECTONE_API AONE06GroupingCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE06GroupingCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    struct FRecordedShot
    {
        TArray<FONEProjectilePath> Paths;
        FVector Muzzle=FVector::ZeroVector,Aim=FVector::ZeroVector;
    };
    void Check(bool Pass,const FString& Label);
    void Enter(int32 Next);
    void Key(bool Down);
    void ClearScene();
    void PreparePattern();
    void PrepareTrial();
    void RecordShot();
    void CompleteTrial();
    void Finish(bool Complete);
    bool MakeWall();
    bool MakeTarget();
    int32 RequiredShots() const { return Pattern==1?12:3; }
    float Distance() const;
    FString PatternName() const;
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEPlayerController> Controller;
    UPROPERTY() TObjectPtr<AONEGameMode> Mode;
    UPROPERTY() TObjectPtr<UONE06CaptureComponent> Capture;
    UPROPERTY() TObjectPtr<AActor> Wall;
    UPROPERTY() TObjectPtr<AONEZombie> Target;
    TArray<FRecordedShot> Baseline;
    FVector Origin=FVector::ZeroVector;
    float Widths[3]={};
    float FirstSpread=0,LastSpread=0,PeakBloom=0;
    double MinY=0,MaxY=0,MinZ=0,MaxZ=0,PairOriginError=0,PairDirectionError=0;
    double StartedReal=0,FinishedReal=0,PhaseAt=0;
    int32 Phase=0,Pattern=0,DistanceIndex=0,Scene=0,Shot=0;
    int32 Checks=0,Failures=0,SeenShots=0,TrialShotsBefore=0,PointsBefore=0;
    int32 Projectiles=0,WallHits=0,EnemyContacts=0,Trials=0;
    bool bFinished=false,bFinishing=false,bComplete=false,bEndingPlay=false;
    FString Folder,ChecksCsv,RaysCsv,GroupsCsv;
};
