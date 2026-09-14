#include "ONE05Audio.h"
#include "ONEGameMode.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "HAL/IConsoleManager.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundAttenuation.h"

static TAutoConsoleVariable<float> CVarONEWeaponGain(TEXT("one.Audio.Weapons"),1.f,TEXT("Project ONE weapon bank group gain, 0..2."));
static TAutoConsoleVariable<float> CVarONEZombieGain(TEXT("one.Audio.Zombies"),.8f,TEXT("Project ONE localized infected voice gain, 0..2."));
static TAutoConsoleVariable<float> CVarONEAmbienceGain(TEXT("one.Audio.Ambience"),.55f,TEXT("Project ONE facility ambience gain, 0..2."));
static TAutoConsoleVariable<float> CVarONEFoleyGain(TEXT("one.Audio.Foley"),.8f,TEXT("Recorded contact and footstep gain, 0..2."));
static TAutoConsoleVariable<float> CVarONEContactReverb(TEXT("one.Audio.ContactReverb"),.12f,TEXT("Contact reverb send, 0..0.4; requires an active scene reverb."));
float ONE05Audio::GetWeaponGain() { return FMath::Clamp(CVarONEWeaponGain.GetValueOnGameThread(),0.f,2.f); }
float ONE05Audio::GetZombieGain() { return FMath::Clamp(CVarONEZombieGain.GetValueOnGameThread(),0.f,2.f); }
float ONE05Audio::GetAmbienceGain() { return FMath::Clamp(CVarONEAmbienceGain.GetValueOnGameThread(),0.f,2.f); }
float ONE05Audio::GetFoleyGain() { return FMath::Clamp(CVarONEFoleyGain.GetValueOnGameThread(),0.f,2.f); }
bool ONE05Audio::IsMetalContact(const FHitResult& Hit)
{
    return UPhysicalMaterial::DetermineSurfaceType(Hit.PhysMaterial.Get())==SurfaceType1 ||
        (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("ONE_AudioMetal"))) ||
        (Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("ONE_AudioMetal")));
}

void UONE05AudioWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    for (int32 Index=0;Index<4;++Index)
    {
        auto* GroupAsset=NewObject<USoundConcurrency>(this);
        GroupAsset->Concurrency.MaxCount=Index==0?4:Index==1?7:Index==2?3:12;
        GroupAsset->Concurrency.bLimitToOwner=false;
        GroupAsset->Concurrency.ResolutionRule=EMaxConcurrentResolutionRule::StopQuietest;
        GroupAsset->Concurrency.VoiceStealReleaseTime=Index==1?.035f:.12f;
        Groups.Add(GroupAsset);
    }
}
USoundBase* UONE05AudioWorldSubsystem::Sound(FName Name)
{
    if (const auto* Existing=Sounds.Find(Name)) return Existing->Get();
    const FString Asset=Name.ToString();
    const TCHAR* Bank=Asset.StartsWith(TEXT("S_C07_"))?TEXT("Candidate07"):TEXT("Candidate05");
    auto* Loaded=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/ONE/Audio/%s/%s.%s"),Bank,*Asset,*Asset));
    Sounds.Add(Name,Loaded);
    return Loaded;
}
bool UONE05AudioWorldSubsystem::PlayFoley(const TCHAR* Stem,int32 Variants,const FVector& Location,float Gain,float Priority)
{
    if (!GetWorld() || Location.ContainsNaN() || Gain<=0.f || Variants<1) return false;
    if (const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) if (GM->IsGameOver()) return false;
    int32& Previous=PreviousVariants.FindOrAdd(Stem);
    int32 Index=FMath::RandRange(1,Variants);
    if (Index==Previous && Variants>1) Index=Index%Variants+1;
    Previous=Index;
    auto* Wave=Sound(FName(*FString::Printf(TEXT("S_C07_%s_%02d"),Stem,Index)));
    if (!Wave) return false;
    UAudioComponent* Voice=nullptr;
    for (const auto& Existing:ContactVoices) if (Existing && !Existing->IsPlaying()) { Voice=Existing.Get(); break; }
    if (!Voice && ContactVoices.Num()<12)
    {
        Voice=NewObject<UAudioComponent>(GetWorld());
        Voice->bAutoActivate=false; Voice->bAutoDestroy=false;
        Voice->bAllowSpatialization=true; Voice->bOverrideAttenuation=true;
        Voice->AttenuationOverrides.bAttenuate=true; Voice->AttenuationOverrides.bSpatialize=true;
        Voice->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
        Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(80.f);
        Voice->AttenuationOverrides.FalloffDistance=1100.f;
        Voice->AttenuationOverrides.bEnableReverbSend=true;
        Voice->AttenuationOverrides.ReverbSendMethod=EReverbSendMethod::Manual;
        Voice->bOverridePriority=true;
        if (auto* Concurrency=Group(EONE05VoiceGroup::Foley)) Voice->ConcurrencySet.Add(Concurrency);
        Voice->RegisterComponentWithWorld(GetWorld()); ContactVoices.Add(Voice);
    }
    if (!Voice)
    {
        // Low-priority casing chatter cannot evict a body or player contact.
        // Reuse the quietest among the lowest-priority eligible voices.
        for (const auto& Existing:ContactVoices)
            if (Existing && Existing->Priority<=Priority && (!Voice || Existing->Priority<Voice->Priority ||
                (Existing->Priority==Voice->Priority && Existing->VolumeMultiplier<Voice->VolumeMultiplier))) Voice=Existing.Get();
    }
    if (!Voice) return false;
    Voice->Stop(); Voice->Priority=Priority;
    Voice->AttenuationOverrides.ManualReverbSendLevel=FMath::Clamp(CVarONEContactReverb.GetValueOnGameThread(),0.f,.4f);
    Voice->SetWorldLocation(Location); Voice->SetSound(Wave); Voice->SetPitchMultiplier(1.f);
    Voice->SetVolumeMultiplier(FMath::Clamp(Gain,0.f,1.f)*ONE05Audio::GetFoleyGain()); Voice->Play();
    ++ContactCueCount; return true;
}
void UONE05AudioWorldSubsystem::PlayFootContact(const FHitResult& Hit,bool bPlayer)
{
    if (!Hit.bBlockingHit) return;
    const TCHAR* Stem=ONE05Audio::IsMetalContact(Hit)?(bPlayer?TEXT("PlayerStepMetal"):TEXT("ZombieStepMetal")):
        (bPlayer?TEXT("PlayerStepConcrete"):TEXT("ZombieStepConcrete"));
    if (PlayFoley(Stem,4,Hit.ImpactPoint,bPlayer?.42f:.32f,bPlayer?1.4f:.65f)) ++FootCueCount;
}
void UONE05AudioWorldSubsystem::PlayBodyContact(const FVector& Location,float NormalImpactSpeedCmPerSec)
{
    if (!FMath::IsFinite(NormalImpactSpeedCmPerSec) || NormalImpactSpeedCmPerSec<65.f) return;
    const float Gain=FMath::GetMappedRangeValueClamped(FVector2D(65,430),FVector2D(.16f,.72f),NormalImpactSpeedCmPerSec);
    if (PlayFoley(TEXT("BodyContact"),6,Location,Gain,1.3f)) ++BodyCueCount;
}
void UONE05AudioWorldSubsystem::StopContacts()
{
    for (const auto& Voice:ContactVoices) if (Voice) Voice->Stop();
}
int32 UONE05AudioWorldSubsystem::GetActiveContactVoiceCount() const
{
    int32 Count=0; for (const auto& Voice:ContactVoices) Count+=int32(Voice && Voice->IsPlaying()); return Count;
}
void UONE05AudioWorldSubsystem::Deinitialize()
{
    StopContacts(); for (const auto& Voice:ContactVoices) if (Voice) Voice->DestroyComponent();
    ContactVoices.Reset(); Sounds.Reset(); Groups.Reset(); Super::Deinitialize();
}
USoundConcurrency* UONE05AudioWorldSubsystem::Group(EONE05VoiceGroup Which) const
{
    const int32 Index=int32(Which); return Groups.IsValidIndex(Index)?Groups[Index].Get():nullptr;
}
