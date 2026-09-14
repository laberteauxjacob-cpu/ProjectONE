#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ONE07PhysicalityCheck.generated.h"
class AONEZombie;
class AONEPlayer;
class AONEPlayerController;
class AONEGameMode;
class UONE06CaptureComponent;
class AStaticMeshActor;

/** Opt-in recorded runtime evidence. Setup/packet probes are explicitly logged;
 * the separate encounter mode only sends production controller input. */
UCLASS()
class PROJECTONE_API AONE07PhysicalityCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE07PhysicalityCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Check(bool Pass,const FString& Label);
    void EnterPhase();
    void Observe(float Dt);
    void Key(const FKey& Key,bool Down);
    void ReleaseKeys();
    void DriveEncounter(float Dt);
    void Finish();
    void WriteResults();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEPlayerController> Controller;
    UPROPERTY() TObjectPtr<AONEGameMode> Mode;
    UPROPERTY() TObjectPtr<UONE06CaptureComponent> Capture;
    UPROPERTY() TArray<TObjectPtr<AONEZombie>> Subjects;
    UPROPERTY() TObjectPtr<AStaticMeshActor> ProbeObstacle;
    TSet<FKey> Held;
    float Elapsed=0,PhaseStart=0,NextSample=0,NextFire=0,ReleaseFireAt=0;
    int32 Phase=-1,Checks=0,Failures=0,HealthRestores=0,InputEdges=0,Action=0;
    int32 StartKills=0,StartRemaining=0;
    bool bCapture=false,bEncounter=false,bFinishing=false,bWritten=false;
    float FinishAt=0;
    FVector Origin=FVector(0,360,98);
    FString Folder,Label,Report,Telemetry,Inputs;
};
