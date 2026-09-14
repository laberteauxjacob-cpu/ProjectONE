#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ONE05Audio.generated.h"
class USoundBase;
class USoundConcurrency;
class UAudioComponent;

namespace ONE05Audio
{
    PROJECTONE_API float GetWeaponGain();
    PROJECTONE_API float GetZombieGain();
    PROJECTONE_API float GetAmbienceGain();
    PROJECTONE_API float GetFoleyGain();
    PROJECTONE_API bool IsMetalContact(const FHitResult& Hit);
}

enum class EONE05VoiceGroup : uint8 { Breath, Action, Environment, Foley };

/** Per-world shared voice limits and source cache; destroyed with the encounter. */
UCLASS()
class PROJECTONE_API UONE05AudioWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    USoundBase* Sound(FName Name);
    USoundConcurrency* Group(EONE05VoiceGroup Which) const;
    bool PlayFoley(const TCHAR* Stem,int32 Variants,const FVector& Location,float Gain,float Priority=1.f);
    void PlayFootContact(const FHitResult& Hit,bool bPlayer);
    void PlayBodyContact(const FVector& Location,float NormalImpactSpeedCmPerSec);
    void StopContacts();
    int32 GetActiveContactVoiceCount() const;
    uint64 GetContactCueCount() const { return ContactCueCount; }
    uint64 GetFootCueCount() const { return FootCueCount; }
    uint64 GetBodyCueCount() const { return BodyCueCount; }
private:
    UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TArray<TObjectPtr<USoundConcurrency>> Groups;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> ContactVoices;
    TMap<FString,int32> PreviousVariants;
    uint64 ContactCueCount=0,FootCueCount=0,BodyCueCount=0;
};
