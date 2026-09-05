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
}

enum class EONE05VoiceGroup : uint8 { Breath, Action, Environment };

/** Per-world shared voice limits and source cache; destroyed with the encounter. */
UCLASS()
class PROJECTONE_API UONE05AudioWorldSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    USoundBase* Sound(FName Name);
    USoundConcurrency* Group(EONE05VoiceGroup Which) const;
private:
    UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TArray<TObjectPtr<USoundConcurrency>> Groups;
};
