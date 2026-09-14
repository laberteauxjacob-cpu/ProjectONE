#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponTypes.h"
#include "ONE07PortabilityCheck.generated.h"
class AONEPlayer;
class AONEGameMode;
class AONEZombie;
class UONEInfectedVariant;
class UONEZombieAudioComponent;
class UONE06CaptureComponent;
class UCapsuleComponent;
class FJsonValue;

/** Opt-in second-map probe. Normal navigation/attacks; declared regional/fall setup. */
UCLASS()
class PROJECTONE_API AONE07PortabilityCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE07PortabilityCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Check(bool Passed,const FString& Label);
    void Enter(int32 Next);
    bool Discover();
    bool PositionPlayer(const FVector& Anchor);
    bool StartVariant();
    EONEWeaponHitOutcome Probe(EONEHitRegion Region,float Damage,float Trauma=0.f);
    bool QueryBody(const TCHAR* Label);
    void Observe();
    void Finish(bool Complete);
    void WriteResults(bool Complete);
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEGameMode> Mode;
    UPROPERTY() TObjectPtr<AONEZombie> Subject;
    UPROPERTY() TArray<TObjectPtr<UONEInfectedVariant>> Variants;
    UPROPERTY() TObjectPtr<UONEZombieAudioComponent> RetiredAudio;
    UPROPERTY() TObjectPtr<UCapsuleComponent> RetiredBody;
    UPROPERTY() TObjectPtr<UONE06CaptureComponent> Capture;
    FVector PlayerAnchor=FVector::ZeroVector,SpawnAnchor=FVector::ZeroVector,RetreatAnchor=FVector::ZeroVector;
    FVector SpawnPosition=FVector::ZeroVector,BodyRayStart=FVector::ZeroVector,BodyRayEnd=FVector::ZeroVector;
    FONEWeaponDamagePacket LastPacket;
    TArray<TSharedPtr<FJsonValue>> Assertions;
    FString Folder,Timeline,CheckText;
    int32 Phase=0,VariantIndex=0,CompletedVariants=0,Checks=0,Failures=0,ObservedFrames=0;
    int32 StartKills=0,StartPoints=0,FallPoints=0,PeakVoices=0,DamageAtFall=0;
    uint32 SubjectIdentity=0;
    uint64 StartEligibleDeaths=0;
    float FallHealth=0.f,PreviousDropChance=0.f;
    double StartedReal=0.,FinishedReal=0.,PhaseAt=0.;
    bool bSawGetUp=false,bFallenProbe=false,bFinished=false,bFinishing=false,bFinishComplete=false;
    bool bCaptureRequested=false,bCaptureStarted=false,bCaptureFailure=false,bChangedDropChance=false;
};
