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
    // Appearance salt affects only recorded cue selection in this component.
    void ConfigureVoiceVariation(int32 Salt);
    int32 GetVoiceVariation() const { return VoiceVariation; }
    void SetPursuing(bool Pursuing);
    void NotifyAttack(int32 Variant);
    void NotifyHit(bool Heavy);
    void NotifyDeath();
    // Called after the animation graph has evaluated a grounded foot plant.
    void NotifyFootContact(bool bLeftFoot);
    void NotifyFall();
    void NotifyBodyContact(const FVector& Location,float NormalImpactSpeedCmPerSec);
    void StopLiving();
    void Shutdown();
    bool IsLivingAudioEnabled() const { return !bDead && !bShutdown; }
    int32 GetActiveVoiceCount() const;
    int32 GetAttackCueCount() const { return AttackCueCount; }
    int32 GetHitCueCount() const { return HitCueCount; }
    int32 GetDeathCueCount() const { return DeathCueCount; }
    int32 GetFootCueCount() const { return FootCueCount; }
    int32 GetFallCueCount() const { return FallCueCount; }
    int32 GetBodyCueCount() const { return BodyCueCount; }
private:
    UAudioComponent* MakeVoice(const TCHAR* Name,bool Action);
    void Play(UAudioComponent* Voice,const FString& Stem,int32 Index,float Gain);
    int32 Choose(int32 Count,int32& Previous);
    void UpdateVoiceLocation();
    void ObserveAttackFootMotion();
    void SeedPresentationRandom();
    UPROPERTY() TObjectPtr<UAudioComponent> BreathVoice;
    UPROPERTY() TObjectPtr<UAudioComponent> ActionVoice;
    bool bDead=false,bShutdown=false,bPursuing=false;
    double NextBreath=0,NextHit=0,NextAttack=0;
    int32 LastBreath=0,LastPursuit=0,LastHit=0,LastDeath=0;
    int32 LastHeavyHit=0,LastFall=0;
    int32 AttackCueCount=0,HitCueCount=0,DeathCueCount=0;
    int32 FootCueCount=0,FallCueCount=0,BodyCueCount=0;
    double NextFoot[2]={0.,0.},NextBodyContact=0.,NextFall=0.;
    double LastAttackFootMotion=-100.;
    uint64 FootMotionActionSerial=MAX_uint64;
    float BreathGain=.45f,ActionGain=.85f;
    int32 VoiceVariation=0;
    bool bPresentationSeeded=false;
    FRandomStream PresentationRandom;
};
