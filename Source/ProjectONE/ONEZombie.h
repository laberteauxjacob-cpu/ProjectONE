#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ONEWeaponTypes.h"
#include "ONEPhysicsRuntime.h"
#include "TimerManager.h"
#include "ONEZombie.generated.h"
class UONEHealthComponent;
class USphereComponent;
class UCapsuleComponent;
class AONEPlayer;
class UONEZombieAudioComponent;

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
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void ReceiveBullet(const FHitResult& Hit,const FVector& Direction,float Damage);
    EONEHitRegion GetHitRegion(const FHitResult& Hit) const;
    // True means an accepted live transaction OR a cosmetic corpse transaction.
    // Corpse transactions never modify health, score or the live damage counter.
    bool ReceiveWeaponDamage(const FONEWeaponDamagePacket& Packet);
    EONEWeaponHitOutcome ReceiveWeaponDamageOutcome(const FONEWeaponDamagePacket& Packet);
    // The same entry point is used by pursuit and deterministic attack probes.
    bool TryStartAttack(AONEPlayer* Victim,int32 PreferredFamily=INDEX_NONE);
    FName GetAttackClipKey() const;
    int32 GetAttackFamily() const { return AttackFamily; }
    float GetCurrentAttackContactTime() const;
    float GetCurrentAttackDuration() const;
    bool IsAttackContactConsumed() const { return bContactDelivered; }
    int32 GetAttackContactAttemptCount() const { return AttackContactAttempts; }
    int32 GetAttackDamageDispatchCount() const { return AttackDamageDispatches; }
    float GetMinorReactionAge() const;
    FVector GetMinorReactionDirection() const { return MinorReactionDirection; }
    float GetMinorReactionStrength() const { return MinorReactionStrength; }
    bool IsHeavyReaction() const { return bHeavyReaction; }
    int32 GetDamageTransactionCount() const { return DamageTransactions; }
    int32 GetSeverCount() const { return SeverCount; }
    bool IsDead() const;
    bool HasLeftArm() const { return !bLeftArmSevered; }
    bool HasRightArm() const { return !bRightArmSevered; }
    bool HasLeftLeg() const { return !bLeftLegSevered; }
    bool HasHead() const { return !bHeadSevered; }
    bool IsRagdollActive() const { return bRagdollActive; }
    float GetRagdollTransitionErrorCm() const { return RagdollPositionError; }
    float GetRagdollTransitionAngleDegrees() const { return RagdollAngleError; }
    float GetStumpCollisionFitErrorCm() const { return StumpFitError; }
    int32 GetActivePhysicsBodyCount() const;
    int32 GetAwakePhysicsBodyCount() const;
    const ONEPhysicsRuntime::FRestState& GetRestState() const { return RestState; }
    int32 GetRegionPhysicsBodyCount(EONEHitRegion Region) const;
    int32 GetCorpseTransactionCount() const { return CorpseTransactions; }
    float GetLegQueryCoverageErrorCm(EONEHitRegion Region) const;
    float GetReferenceLegQuerySeparationCm() const { return ReferenceLegQuerySeparation; }
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
    UPROPERTY(EditAnywhere, Category="Infected") float MinorReactionInterval=.13f;
    UPROPERTY(EditAnywhere, Category="Infected") float HeadSeverThreshold=32.f;
    UPROPERTY(EditAnywhere, Category="Infected") float ArmSeverThreshold=50.f;
    UPROPERTY(EditAnywhere, Category="Infected") float LegSeverThreshold=70.f;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEHealthComponent> Health;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UONEZombieAudioComponent> ZombieAudio;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> HeadMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> ArmLeftMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> ArmRightMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> LegLeftMesh;
    // Compatibility aliases: accepted source *_l is anatomical RIGHT after import.
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> ArmMesh;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> HeadRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> ArmRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> UpperArmRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> ArmLeftRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> UpperArmLeftRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> ArmRightRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> UpperArmRightRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> LegLeftRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> UpperLegLeftRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> LegRightRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> UpperLegRightRegion;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> BodyRegion;
private:
    void ChangeState(EONEZombieState Next);
    void Die(const FVector& Direction,EONEHitRegion ImpactRegion,FName ImpactBone,const FVector& ImpactPosition,float Impulse);
    void Sever(EONEHitRegion Region,const FVector& Direction);
    bool IsRegionPresent(EONEHitRegion Region) const;
    FName ResolveRegionBone(EONEHitRegion Region,FName Requested) const;
    void GetCutWorld(EONEHitRegion Region,FVector& Point,FVector& Normal) const;
    void StopPursuit();
    bool RequiredAttackArmsPresent() const;
    void TickAttack(float Dt);
    void ObserveRest();
    UPROPERTY() TObjectPtr<AONEPlayer> Target;
    EONEZombieState State=EONEZombieState::Pursue;
    float StateStart=0,LastReaction=-100,NextAttack=0,NextPath=0;
    float RegionalTrauma[FONEWeaponDamagePacket::RegionCount]={};
    bool bHeavyReaction=false;
    int32 AttackFamily=0,AttackSerial=0,AttackContactAttempts=0,AttackDamageDispatches=0;
    uint8 RequiredAttackArms=0;
    FVector AttackHeading=FVector::ForwardVector,AttackStartPosition=FVector::ZeroVector;
    float MinorReactionStart=-100.f,MinorReactionStrength=0.f;
    FVector MinorReactionDirection=FVector::ForwardVector;
    int32 DamageTransactions=0,CorpseTransactions=0,SeverCount=0;
    TArray<uint64> RecentShotIds;
    bool bHeadSevered=false,bLeftArmSevered=false,bRightArmSevered=false,bLeftLegSevered=false,bContactDelivered=false;
    bool bRagdollActive=false;
    ONEPhysicsRuntime::FRestState RestState;
    FTimerHandle RestTimer;
    float RagdollPositionError=BIG_NUMBER,RagdollAngleError=BIG_NUMBER;
    float StumpFitError=BIG_NUMBER;
    float ReferenceLegQuerySeparation=-BIG_NUMBER;
};
