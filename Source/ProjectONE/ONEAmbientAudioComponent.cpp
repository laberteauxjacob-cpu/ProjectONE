#include "ONEAmbientAudioComponent.h"
#include "ONE05Audio.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Sound/SoundAttenuation.h"

UONEAmbientAudioComponent::UONEAmbientAudioComponent()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickInterval=.25f;
}
UAudioComponent* UONEAmbientAudioComponent::MakeVoice(const TCHAR* Name,const FVector& Location,float Inner,float Outer,bool Sparse)
{
    auto* Voice=NewObject<UAudioComponent>(GetOwner(),Name); GetOwner()->AddInstanceComponent(Voice);
    Voice->bAutoActivate=false; Voice->bAutoDestroy=false; Voice->bStopWhenOwnerDestroyed=true;
    Voice->bOverrideAttenuation=true; Voice->bAllowSpatialization=true;
    Voice->AttenuationOverrides.bAttenuate=true; Voice->AttenuationOverrides.bSpatialize=true;
    Voice->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
    Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(Inner); Voice->AttenuationOverrides.FalloffDistance=Outer-Inner;
    Voice->bOverridePriority=true; Voice->Priority=.15f;
    if (Sparse) if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>())
        if (auto* Concurrency=Audio->Group(EONE05VoiceGroup::Environment)) Voice->ConcurrencySet.Add(Concurrency);
    Voice->RegisterComponent(); Voice->SetWorldLocation(Location); return Voice;
}
void UONEAmbientAudioComponent::BeginPlay()
{
    Super::BeginPlay(); Random.Initialize(505193);
    Loops.Add(MakeVoice(TEXT("FacilityVentilation"),FVector(-870,-700,230),500,2800,false));
    Loops.Add(MakeVoice(TEXT("DistantFacilityMotor"),FVector(960,-760,120),230,2600,false));
    SparseVoices.Add(MakeVoice(TEXT("FacilityWater"),FVector(950,-160,125),80,1650,true));
    SparseVoices.Add(MakeVoice(TEXT("FacilityPipe"),FVector(-890,200,170),80,1850,true));
    if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>())
    {
        Loops[0]->SetSound(Audio->Sound(TEXT("S_AmbientVentLoop")));
        Loops[1]->SetSound(Audio->Sound(TEXT("S_AmbientMotorLoop")));
    }
    NextDrip=GetWorld()->GetTimeSeconds()+3.7; NextMetal=GetWorld()->GetTimeSeconds()+8.3;
    SetEnabled(bEnabled);
}
void UONEAmbientAudioComponent::SetEnabled(bool Enabled)
{
    if (bShutdown) return;
    bEnabled=Enabled;
    for (int32 Index=0;Index<Loops.Num();++Index)
    {
        Loops[Index]->SetVolumeMultiplier((Index==0?.55f:.36f)*ONE05Audio::GetAmbienceGain());
        if (bEnabled && !Loops[Index]->IsPlaying() && Loops[Index]->Sound) Loops[Index]->FadeIn(.8f,1.f);
        if (!bEnabled) Loops[Index]->Stop();
    }
    if (!bEnabled) for (const auto& Voice:SparseVoices) if (Voice) Voice->Stop();
}
void UONEAmbientAudioComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (bShutdown || !bEnabled) return;
    for (int32 I=0;I<Loops.Num();++I) Loops[I]->SetVolumeMultiplier((I==0?.55f:.36f)*ONE05Audio::GetAmbienceGain());
    auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>(); if (!Audio || SparseVoices.Num()!=2) return;
    const double Now=GetWorld()->GetTimeSeconds();
    if (Now>=NextDrip)
    {
        NextDrip=Now+Random.FRandRange(4.5f,9.f); DripIndex=DripIndex%3+1;
        auto* Voice=SparseVoices[0].Get(); Voice->SetSound(Audio->Sound(FName(*FString::Printf(TEXT("S_AmbientDrip_%02d"),DripIndex))));
        Voice->SetVolumeMultiplier(.58f*ONE05Audio::GetAmbienceGain()); if (Voice->Sound) Voice->Play(); ++SparseCueCount;
    }
    if (Now>=NextMetal)
    {
        NextMetal=Now+Random.FRandRange(9.f,17.f); MetalIndex=MetalIndex%3+1;
        auto* Voice=SparseVoices[1].Get(); Voice->SetSound(Audio->Sound(FName(*FString::Printf(TEXT("S_AmbientPipe_%02d"),MetalIndex))));
        Voice->SetVolumeMultiplier(.46f*ONE05Audio::GetAmbienceGain()); if (Voice->Sound) Voice->Play(); ++SparseCueCount;
    }
}
void UONEAmbientAudioComponent::Shutdown()
{
    SetEnabled(false); bShutdown=true; SetComponentTickEnabled(false);
}
int32 UONEAmbientAudioComponent::GetActiveVoiceCount() const
{
    int32 Count=0; for (const auto& Voice:Loops) Count+=int32(Voice && Voice->IsPlaying());
    for (const auto& Voice:SparseVoices) Count+=int32(Voice && Voice->IsPlaying()); return Count;
}
void UONEAmbientAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Shutdown(); Super::EndPlay(Reason);
}
