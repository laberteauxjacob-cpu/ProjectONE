#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEAmbientAudioComponent.generated.h"
class UAudioComponent;

/** Restrained original facility sources; the game mode owns encounter lifetime. */
UCLASS(ClassGroup=(ONE))
class PROJECTONE_API UONEAmbientAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEAmbientAudioComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void SetEnabled(bool Enabled);
    void Shutdown();
    int32 GetActiveVoiceCount() const;
    int32 GetSparseCueCount() const { return SparseCueCount; }
private:
    UAudioComponent* MakeVoice(const TCHAR* Name,const FVector& Location,float Inner,float Outer,bool Sparse);
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Loops;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> SparseVoices;
    bool bEnabled=true,bShutdown=false;
    double NextDrip=0,NextMetal=0;
    int32 SparseCueCount=0,DripIndex=0,MetalIndex=0;
    FRandomStream Random;
};
