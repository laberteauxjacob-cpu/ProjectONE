#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ONEGameMode.generated.h"

class AONEZombie;
class AONEPlayer;
enum class EONEWeaponFamily : uint8;
class ULightComponent;
class UONEAmbientAudioComponent;
UCLASS()
class PROJECTONE_API AONEGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AONEGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void NotifyZombieKilled(AONEZombie* Zombie, int32 Reward = 100);
    void PlayerDied();
    uint64 NewMachineReceipt() { return ++NextMachineReceipt; }
    bool TrySpendPoints(int32 Cost,uint64 Receipt);
    bool RefundPointsOnce(uint64 Receipt);
    void CancelUnacceptedMachineActions(AONEPlayer* Player);
    void GrantSandboxPoints();
    void SetForcedBoxReward(EONEWeaponFamily Family);
    EONEWeaponFamily GetForcedBoxReward() const { return ForcedBoxReward; }
    EONEWeaponFamily ConsumeForcedBoxReward();
    FString GetForcedBoxRewardLabel() const;
    int32 GetSandboxGrantedPoints() const { return SandboxGrantedPoints; }
    UFUNCTION(BlueprintCallable) void RestartScene();
    void ToggleSandbox();
    void SpawnSandboxEnemies(int32 Count);
    AONEZombie* SpawnSandboxEnemyAt(const FVector& Location);
    void RefillSandboxAmmo();
    void ClearSandboxPresentation();
    void SetSandboxDimLighting(bool Dim);
    void ToggleSandboxLighting() { SetSandboxDimLighting(!bDimLighting); }
    bool IsSandboxDimLighting() const { return bDimLighting; }
    int32 GetSandboxLightCount() const { return SandboxLightIntensities.Num(); }
    bool IsSandbox() const { return bSandbox; }
    int32 GetRound() const { return Round; }
    int32 GetPoints() const { return Points; }
    int32 GetRemaining() const { return Alive.Num() + ToSpawn; }
    int32 GetKills() const { return Kills; }
    bool IsGameOver() const { return bGameOver; }
    bool IsIntermission() const { return bIntermission; }
    float GetCountdown() const { return Countdown; }
    float GetSurvivalSeconds() const { return SurvivalSeconds; }
    UONEAmbientAudioComponent* GetAmbientAudio() const { return AmbientAudio; }
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEAmbientAudioComponent> AmbientAudio;
    UPROPERTY(EditAnywhere, Category="Rounds") float IntermissionSeconds = 7.f;
    UPROPERTY(EditAnywhere, Category="Rounds") float SpawnInterval = 1.3f;
    UPROPERTY(EditAnywhere, Category="Rounds") int32 MaximumActive = 18;
    UPROPERTY(EditAnywhere, Category="Rounds") TSubclassOf<AONEZombie> ZombieClass;
private:
    void StartRound();
    void SpawnEnemy();
    TSet<TWeakObjectPtr<AONEZombie>> Alive;
    TArray<FVector> SpawnLocations;
    int32 Round = 0, Points = 0, Kills = 0, ToSpawn = 0;
    bool bGameOver = false, bIntermission = true;
    bool bSandbox = false;
    bool bDimLighting = false;
    TMap<TWeakObjectPtr<ULightComponent>,float> SandboxLightIntensities;
    float Countdown = 5.f, SpawnClock = 0.f;
    float SurvivalSeconds=0.f;
    uint64 NextMachineReceipt=0;
    TMap<uint64,int32> MachineReceipts;
    int32 SandboxGrantedPoints=0;
    EONEWeaponFamily ForcedBoxReward;
};
