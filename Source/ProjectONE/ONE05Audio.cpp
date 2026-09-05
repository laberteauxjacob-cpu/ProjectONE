#include "ONE05Audio.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundConcurrency.h"
#include "HAL/IConsoleManager.h"

static TAutoConsoleVariable<float> CVarONEWeaponGain(TEXT("one.Audio.Weapons"),1.f,TEXT("Project ONE weapon bank group gain, 0..2."));
static TAutoConsoleVariable<float> CVarONEZombieGain(TEXT("one.Audio.Zombies"),.8f,TEXT("Project ONE localized infected voice gain, 0..2."));
static TAutoConsoleVariable<float> CVarONEAmbienceGain(TEXT("one.Audio.Ambience"),.55f,TEXT("Project ONE facility ambience gain, 0..2."));
float ONE05Audio::GetWeaponGain() { return FMath::Clamp(CVarONEWeaponGain.GetValueOnGameThread(),0.f,2.f); }
float ONE05Audio::GetZombieGain() { return FMath::Clamp(CVarONEZombieGain.GetValueOnGameThread(),0.f,2.f); }
float ONE05Audio::GetAmbienceGain() { return FMath::Clamp(CVarONEAmbienceGain.GetValueOnGameThread(),0.f,2.f); }

void UONE05AudioWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    for (int32 Index=0;Index<3;++Index)
    {
        auto* GroupAsset=NewObject<USoundConcurrency>(this);
        GroupAsset->Concurrency.MaxCount=Index==0?4:Index==1?7:3;
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
    auto* Loaded=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/ONE/Audio/Candidate05/%s.%s"),*Asset,*Asset));
    Sounds.Add(Name,Loaded);
    return Loaded;
}
USoundConcurrency* UONE05AudioWorldSubsystem::Group(EONE05VoiceGroup Which) const
{
    const int32 Index=int32(Which); return Groups.IsValidIndex(Index)?Groups[Index].Get():nullptr;
}
