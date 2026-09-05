#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponTypes.h"
#include "ONEPhysicsRuntime.h"
#include "TimerManager.h"
#include "ONEBloodSubsystem.generated.h"
class UProceduralMeshComponent;
class USkeletalMeshComponent;
class UMaterialInterface;
class UDecalComponent;

UCLASS()
class PROJECTONE_API AONEBloodEffect : public AActor
{
    GENERATED_BODY()
public:
    AONEBloodEffect();
    virtual void Tick(float Dt) override;
    void InitSpray(const FVector& Direction,UMaterialInterface* Material,bool bSever);
    void InitShot(const FVector& End,UMaterialInterface* Material);
    void InitManagedDrops(UMaterialInterface* Material);
    void UpdateManagedDrops(const TArray<FVector>& Points,const TArray<FVector>& Motion);
private:
    void BuildGeometry();
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
    TArray<FVector> Positions,Velocities;
    float Age=0;
    bool bShot=false,bManaged=false;
    FVector ShotEnd;
};

UCLASS()
class PROJECTONE_API AONEGorePiece : public AActor
{
    GENERATED_BODY()
public:
    AONEGorePiece();
    void Initialize(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction);
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    USkeletalMeshComponent* GetPieceMesh() const { return Piece; }
    FTransform GetCapturedSourceBoneTransform() const { return CapturedSourceBoneTransform; }
    int32 GetActivePhysicsBodyCount() const;
    int32 GetAwakePhysicsBodyCount() const;
    float GetTransitionErrorCm() const { return TransitionErrorCm; }
    const ONEPhysicsRuntime::FRestState& GetRestState() const { return RestState; }
private:
    void ObserveRest();
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> Piece;
    FTransform CapturedSourceBoneTransform=FTransform::Identity;
    float TransitionErrorCm=BIG_NUMBER;
    ONEPhysicsRuntime::FRestState RestState;
    FTimerHandle RestTimer;
};

struct FONEBleedingWound
{
    TWeakObjectPtr<USkeletalMeshComponent> Source;
    EONEHitRegion Region=EONEHitRegion::Body;
    FName Bone=NAME_None;
    FVector LocalAnchor=FVector::ZeroVector,LocalNormal=FVector::UpVector;
    float Remaining=0,Expires=0,NextDrop=0;
    bool bHeavyBleed=false;
};
struct FONEBloodDrop
{
    TWeakObjectPtr<AActor> Owner;
    FVector Position,LastTrace,Velocity;
    float Volume=0,Age=0;
};
struct FONEBloodPool
{
    TWeakObjectPtr<UDecalComponent> Decal;
    FVector Position,Normal;
    float Volume=0,Radius=0,TargetRadius=0;
};
struct FONEPendingBloodSurface { FVector Position; float Volume=0,Expires=0; };

/** Finite weak-source bleeding, projected growth and bounded physical remains. */
UCLASS()
class PROJECTONE_API UONEBloodSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    void Impact(const FVector& Position,const FVector& Direction,bool bSever);
    void Pool(const FVector& Position,float Size);
    void Shot(const FVector& Start,const FVector& End);
    AONEGorePiece* Detach(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction);
    void AddWound(USkeletalMeshComponent* Source,EONEHitRegion Region,FName Bone,const FVector& Position,const FVector& Normal,float Volume,bool bHeavyBleed=false);
    void RemoveSourcesForActor(AActor* Actor);
    void RegisterCorpse(AActor* Corpse);
    void ClearPresentation();
    int32 GetDecalCount() const;
    int32 GetPieceCount() const;
    int32 GetCorpseCount() const;
    int32 GetWoundCount() const;
    int32 GetDropletCount() const { return Drops.Num(); }
    int32 GetPoolCount() const;
    float GetRemainingBloodVolume() const;
    float GetDepositedBloodVolume() const { return DepositedVolume; }
    float GetLargestPoolRadius() const;
    float GetPoolRenderRadiusErrorCm() const;
    FString DescribePools() const;
    int32 GetProjectionTracesLastStep() const { return LastStepTraces; }
    int32 GetMaximumProjectionTraces() const { return 12; }
    int32 GetGeneration() const { return Generation; }
    AONEGorePiece* GetLastDetachedPiece() const;
private:
    void LoadMaterials();
    void EnsureScheduler();
    void StepBlood();
    bool TakeTraceBudget();
    void ProjectSurface(const FVector& Position,float Volume);
    void Deposit(const FHitResult& Hit,float Volume);
    void TrackDecal(UDecalComponent* Decal);
    static void BoundActors(TArray<TWeakObjectPtr<AActor>>& Actors,int32 Max);
    UPROPERTY() TObjectPtr<UMaterialInterface> BloodMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> PoolMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> FleshMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> MuzzleMaterial;
    TArray<TWeakObjectPtr<AActor>> Effects,Pieces,Corpses;
    TArray<TWeakObjectPtr<UDecalComponent>> Decals;
    TArray<FONEBleedingWound> Wounds;
    TArray<FONEBloodDrop> Drops;
    TArray<FONEBloodPool> Pools;
    TArray<FONEPendingBloodSurface> PendingSurfaces;
    TWeakObjectPtr<AONEBloodEffect> DropRenderer;
    FTimerHandle BloodTimer;
    int32 Generation=0,DropCursor=0,LastStepTraces=0,FrameTraces=0;
    uint64 TraceFrame=MAX_uint64;
    float DepositedVolume=0;
    bool bClearing=false,bInsideStep=false;
};
