#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponCase.generated.h"
class UStaticMeshComponent;
class UStaticMesh;
UCLASS()
class PROJECTONE_API AONEWeaponCase : public AActor
{
    GENERATED_BODY()
public:
    AONEWeaponCase();
    void Initialize(UStaticMesh* Asset,const FVector& EmissionVelocity,const FVector& OwnerVelocity,
        const FVector& Spin,float Radius,float Lifetime,int32 SourceWeapon,uint64 ShotId);
    virtual void Tick(float Dt) override;
    FVector GetCaseVelocity() const { return Velocity; }
    FVector GetInitialVelocity() const { return InitialVelocity; }
    FVector GetInheritedVelocity() const { return InheritedVelocity; }
    bool IsSettled() const { return bSettled; }
    int32 GetBounceCount() const { return BounceCount; }
    UStaticMeshComponent* GetCaseMesh() const { return Mesh; }
    int32 GetWeaponIndex() const { return WeaponIndex; }
    uint64 GetSourceShotId() const { return SourceShotId; }
    float GetCollisionRadius() const { return CollisionRadius; }
private:
    void StepFlight(float Dt);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    FVector Velocity=FVector::ZeroVector;
    FVector InitialVelocity=FVector::ZeroVector,InheritedVelocity=FVector::ZeroVector,AngularVelocity=FVector::ZeroVector;
    float CollisionRadius=.55f;
    int32 BounceCount=0,WeaponIndex=0;
    uint64 SourceShotId=0;
    bool bSettled=false;
};
