#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ONEWeaponTypes.h"
#include "ONEZombie.generated.h"
class UONEHealthComponent;
class USphereComponent;
class UCapsuleComponent;
class AONEPlayer;

UENUM(BlueprintType)
enum class EONEZombieState : uint8 { Pursue, Attack, Hit, Dead };

UCLASS()
class PROJECTONE_API AONEZombie : public ACharacter
{
    GENERATED_BODY()
public:
    AONEZombie();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
    void ReceiveBullet(const FHitResult& Hit,const FVector& Direction,float Damage);
    EONEHitRegion GetHitRegion(const FHitResult& Hit) const;
    bool ReceiveWeaponDamage(const FONEWeaponDamagePacket& Packet);
    bool IsHeavyReaction() const { return bHeavyReaction; }
    int32 GetDamageTransactionCount() const { return DamageTransactions; }
    int32 GetSeverCount() const { return SeverCount; }
    bool IsDead() const;
    bool HasLeftArm() const { return !bArmSevered; }
    bool HasHead() const { return !bHeadSevered; }
    float GetHealth() const;
    UONEHealthComponent* GetHealthComponent() const { return Health; }
    EONEZombieState GetCombatState() const { return State; }
    float GetStateElapsed() const;
    UPROPERTY(EditAnywhere, Category="Infected") float ShambleSpeed=100.f;
    UPROPERTY(EditAnywhere, Category="Infected") float PursuitSpeed=195.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredWalkSpeed=100.f;
    UPROPERTY(EditAnywhere, Category="Animation") float AuthoredRunSpeed=195.f;
    UPROPERTY(EditAnywhere, Category="Infected") float AttackRange=88.f;
    UPROPERTY(EditAnywhere, Category="Infected") float AttackDamage=19.f;
    UPROPERTY(EditAnywhere, Category="Infected") float AttackContactTime=.48f;
    UPROPERTY(EditAnywhere, Category="Infected") float AttackDuration=1.f;
    UPROPERTY(EditAnywhere, Category="Infected") float HitReactCooldown=1.1f;
    UPROPERTY(EditAnywhere, Category="Infected") float HeadSeverThreshold=32.f;
    UPROPERTY(EditAnywhere, Category="Infected") float ArmSeverThreshold=50.f;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEHealthComponent> Health;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> ArmMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> HeadRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> ArmRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> UpperArmRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> BodyRegion;
private:
    void ChangeState(EONEZombieState Next);
    void Die(const FVector& Direction);
    void Sever(bool bHead,const FVector& Direction);
    void StopPursuit();
    UPROPERTY() TObjectPtr<AONEPlayer> Target;
    EONEZombieState State=EONEZombieState::Pursue;
    float StateStart=0,LastReaction=-100,NextAttack=0,NextPath=0;
    float ArmDamage=0,HeadTrauma=0;
    bool bHeavyReaction=false;
    int32 DamageTransactions=0,SeverCount=0;
    TArray<uint64> RecentShotIds;
    bool bHeadSevered=false,bArmSevered=false,bContactDelivered=false;
};
