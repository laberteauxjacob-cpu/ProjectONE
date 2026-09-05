#include "ONEZombieAudioComponent.h"
#include "ONE05Audio.h"
#include "Components/AudioComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Sound/SoundAttenuation.h"

UONEZombieAudioComponent::UONEZombieAudioComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickInterval=.15f;
}
UAudioComponent* UONEZombieAudioComponent::MakeVoice(const TCHAR* Name,bool Action)
{
    auto* Voice=NewObject<UAudioComponent>(GetOwner(),Name);
    GetOwner()->AddInstanceComponent(Voice);
    if (GetOwner()->GetRootComponent()) Voice->SetupAttachment(GetOwner()->GetRootComponent());
    Voice->bAutoActivate=false; Voice->bAutoDestroy=false; Voice->bStopWhenOwnerDestroyed=true;
    Voice->bAllowSpatialization=true; Voice->bOverrideAttenuation=true;
    Voice->AttenuationOverrides.bAttenuate=true; Voice->AttenuationOverrides.bSpatialize=true;
    Voice->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
    Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(Action?150.f:100.f);
    Voice->AttenuationOverrides.FalloffDistance=Action?1350.f:850.f;
    Voice->bOverridePriority=true; Voice->Priority=Action?2.f:.4f;
    if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>())
        if (auto* Concurrency=Audio->Group(Action?EONE05VoiceGroup::Action:EONE05VoiceGroup::Breath)) Voice->ConcurrencySet.Add(Concurrency);
    Voice->SetRelativeLocation(FVector(0,0,40)); Voice->RegisterComponent();
    return Voice;
}
void UONEZombieAudioComponent::BeginPlay()
{
    Super::BeginPlay(); Random.Initialize(int32(GetOwner()->GetUniqueID())*7919+505);
    BreathVoice=MakeVoice(TEXT("InfectedBreathVoice"),false); ActionVoice=MakeVoice(TEXT("InfectedActionVoice"),true);
    NextBreath=GetWorld()->GetTimeSeconds()+Random.FRandRange(1.5f,5.5f);
}
int32 UONEZombieAudioComponent::Choose(int32 Count,int32& Previous)
{
    int32 Result=Random.RandRange(1,Count);
    if (Result==Previous && Count>1) Result=Result%Count+1;
    Previous=Result; return Result;
}
void UONEZombieAudioComponent::Play(UAudioComponent* Voice,const FString& Stem,int32 Index,float Gain)
{
    if (!Voice || bShutdown) return;
    auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>(); if (!Audio) return;
    const FName Name(*FString::Printf(TEXT("S_Zombie%s_%02d"),*Stem,Index));
    Voice->Stop(); Voice->SetSound(Audio->Sound(Name)); Voice->SetPitchMultiplier(1.f);
    Voice->SetVolumeMultiplier(Gain*ONE05Audio::GetZombieGain());
    if (Voice->Sound) Voice->Play();
}
void UONEZombieAudioComponent::SetPursuing(bool Pursuing)
{
    if (bDead || bShutdown) return;
    if (Pursuing && !bPursuing) NextBreath=FMath::Min(NextBreath,GetWorld()->GetTimeSeconds()+Random.FRandRange(.4f,1.6f));
    bPursuing=Pursuing;
}
void UONEZombieAudioComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (bShutdown) return;
    if (BreathVoice) BreathVoice->SetVolumeMultiplier(BreathGain*ONE05Audio::GetZombieGain());
    if (ActionVoice) ActionVoice->SetVolumeMultiplier(ActionGain*ONE05Audio::GetZombieGain());
    if (bDead) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if (Now>=NextBreath)
    {
        NextBreath=Now+Random.FRandRange(bPursuing?2.4f:4.f,bPursuing?4.8f:7.5f);
        if (!ActionVoice || !ActionVoice->IsPlaying())
        {
            BreathGain=bPursuing?.58f:.4f;
            Play(BreathVoice,bPursuing?TEXT("Pursuit"):TEXT("Breath"),bPursuing?Choose(4,LastPursuit):Choose(4,LastBreath),BreathGain);
        }
    }
}
void UONEZombieAudioComponent::NotifyAttack(int32 Variant)
{
    if (!IsLivingAudioEnabled() || GetWorld()->GetTimeSeconds()<NextAttack) return;
    NextAttack=GetWorld()->GetTimeSeconds()+.4; NextHit=GetWorld()->GetTimeSeconds()+.18;
    if (BreathVoice) BreathVoice->Stop();
    ActionGain=.95f; Play(ActionVoice,TEXT("Attack"),FMath::Clamp(Variant,0,2)*2+Random.RandRange(1,2),ActionGain);
    ++AttackCueCount;
}
void UONEZombieAudioComponent::NotifyHit(bool Heavy)
{
    if (!IsLivingAudioEnabled() || GetWorld()->GetTimeSeconds()<NextHit) return;
    NextHit=GetWorld()->GetTimeSeconds()+(Heavy?.28:.36);
    // Anticipation remains audible; minor bullets cannot replace an incoming cue.
    if (!Heavy && GetWorld()->GetTimeSeconds()<NextAttack) return;
    if (BreathVoice) BreathVoice->Stop();
    ActionGain=Heavy?.82f:.62f; Play(ActionVoice,TEXT("Hit"),Choose(4,LastHit),ActionGain); ++HitCueCount;
}
void UONEZombieAudioComponent::StopLiving()
{
    bDead=true; bPursuing=false;
    if (BreathVoice) BreathVoice->Stop();
    if (ActionVoice) ActionVoice->Stop();
}
void UONEZombieAudioComponent::NotifyDeath()
{
    if (bDead || bShutdown) return;
    StopLiving(); ActionGain=.82f;
    Play(ActionVoice,TEXT("Death"),Choose(4,LastDeath),ActionGain); ++DeathCueCount;
}
void UONEZombieAudioComponent::Shutdown()
{
    StopLiving(); bShutdown=true; SetComponentTickEnabled(false);
}
int32 UONEZombieAudioComponent::GetActiveVoiceCount() const
{
    return int32(BreathVoice && BreathVoice->IsPlaying())+int32(ActionVoice && ActionVoice->IsPlaying());
}
void UONEZombieAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Shutdown(); Super::EndPlay(Reason);
}
