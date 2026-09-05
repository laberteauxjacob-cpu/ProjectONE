#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameFramework/Actor.h"
#include "ONEBloodSubsystem.generated.h"
class UProceduralMeshComponent;
class UPoseableMeshComponent;
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
private:
    void BuildGeometry();
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Mesh;
    TArray<FVector> Positions,Velocities;
    float Age=0;
    bool bShot=false;
    FVector ShotEnd;
};

UCLASS()
class PROJECTONE_API AONEGorePiece : public AActor
{
    GENERATED_BODY()
public:
    AONEGorePiece();
    virtual void Tick(float Dt) override;
    void Initialize(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction);
private:
    UPROPERTY() TObjectPtr<UPoseableMeshComponent> Piece;
    FVector Velocity;
    FRotator Spin;
    float Age=0;
    bool bSettled=false;
};

/** Per-world bounded presentation; no effect or detached part blocks movement/navigation. */
UCLASS()
class PROJECTONE_API UONEBloodSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    void Impact(const FVector& Position,const FVector& Direction,bool bSever);
    void Pool(const FVector& Position,float Size);
    void Shot(const FVector& Start,const FVector& End);
    void Detach(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction);
    void RegisterCorpse(AActor* Corpse);
    void ClearPresentation();
    int32 GetDecalCount() const;
    int32 GetPieceCount() const;
    int32 GetCorpseCount() const;
private:
    void LoadMaterials();
    static void BoundActors(TArray<TWeakObjectPtr<AActor>>& Actors,int32 Max);
    UPROPERTY() TObjectPtr<UMaterialInterface> BloodMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> FleshMaterial;
    UPROPERTY() TObjectPtr<UMaterialInterface> MuzzleMaterial;
    TArray<TWeakObjectPtr<AActor>> Effects,Pieces,Corpses;
    TArray<TWeakObjectPtr<UDecalComponent>> Decals;
};
