#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE07ProfileCheck.generated.h"
class AONE05PresentationCheck;
class AONEPlayer;

/** Recording-free companion to the C06-compatible ONE05 profile driver. */
UCLASS()
class PROJECTONE_API AONE07ProfileCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE07ProfileCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    struct FCounts
    {
        int32 Live=0,Standing=0,Fallen=0,GetUp=0,Dead=0,SimulatedBodies=0,AwakeBodies=0;
        int32 LivingBodies=0,DeadBodies=0,ZombieVoices=0,AmbientVoices=0,ContactVoices=0;
        int32 ContactEvents=0,Falls=0,Recoveries=0,Blocked=0,Shots=0,Family=-1,Upgraded=0;
        int32 WorldDrops=0,Ammo=0,Reserve=0,Operation=-1;
        float InstaSeconds=0,DoubleSeconds=0;
    };
    struct FFrame
    {
        uint64 Frame=0;
        double WorldSeconds=0;
        float DeltaSeconds=0,ProfileSeconds=-1,Cap=0,ScreenPercentage=0;
        int32 CsvActive=0,Phase=0,Width=0,Height=0,VSync=0;
        FCounts Counts;
    };
    void Check(bool Pass,const FString& Label);
    void RequestFalls(float ProfileSeconds);
    void WriteResults(bool Complete);
    UPROPERTY() TObjectPtr<AONE05PresentationCheck> Driver;
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    TArray<FFrame> Frames;
    TArray<FString> PhaseLabels;
    FString Folder,Report,Weapon=TEXT("M4A1"),FallMode=TEXT("mixed"),FallEvents;
    int32 Requested=6,Checks=0,Failures=0,Episode=0,FallAttempts=0,AcceptedFalls=0;
    int32 CsvRows=0,ExactRows=0,FallenRows=0,GetUpRows=0,DeadRows=0,RequestedWeaponShots=0;
    int32 LastShots=0,SettingsMismatchRows=0;
    double StartedAt=0,CsvStartedAt=-1;
    bool bSeenCsv=false,bWritten=false,bValid=false;
};
