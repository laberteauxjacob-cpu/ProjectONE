#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEZombieAudioComponent.generated.h"
class UAudioComponent;

/** Event-driven living/death audio. No damage, attack timing or AI ownership. */
UCLASS(ClassGroup=(ONE))
class PROJECTONE_API UONEZombieAudioComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEZombieAudioComponent();
    virtual void BeginPlay() override;
    virtual void TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void SetPursuing(bool Pursuing);
    void NotifyAttack(int32 Variant);
    void NotifyHit(bool Heavy);
    void NotifyDeath();
    void StopLiving();
    void Shutdown();
    bool IsLivingAudioEnabled() const { return !bDead && !bShutdown; }
    int32 GetActiveVoiceCount() const;
    int32 GetAttackCueCount() const { return AttackCueCount; }
    int32 GetHitCueCount() const { return HitCueCount; }
    int32 GetDeathCueCount() const { return DeathCueCount; }
private:
    UAudioComponent* MakeVoice(const TCHAR* Name,bool Action);
    void Play(UAudioComponent* Voice,const FString& Stem,int32 Index,float Gain);
    int32 Choose(int32 Count,int32& Previous);
    UPROPERTY() TObjectPtr<UAudioComponent> BreathVoice;
    UPROPERTY() TObjectPtr<UAudioComponent> ActionVoice;
    bool bDead=false,bShutdown=false,bPursuing=false;
    double NextBreath=0,NextHit=0,NextAttack=0;
    int32 LastBreath=0,LastPursuit=0,LastHit=0,LastDeath=0;
    int32 AttackCueCount=0,HitCueCount=0,DeathCueCount=0;
    float BreathGain=.45f,ActionGain=.85f;
    FRandomStream Random;
};
