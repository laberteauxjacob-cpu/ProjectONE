#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponMagazine.generated.h"
class USceneComponent;
class UStaticMeshComponent;
class UStaticMesh;
/** Cosmetic old magazine. It is never inventory, ammunition or a nav obstacle. */
UCLASS()
class PROJECTONE_API AONEWeaponMagazine : public AActor
{
    GENERATED_BODY()
public:
    AONEWeaponMagazine();
    void Initialize(UStaticMesh* Asset,const FTransform& ReleaseTransform,const FVector& OwnerVelocity,float Lifetime,uint64 InstanceId,uint64 ReleaseId);
    virtual void Tick(float Dt) override;
    UStaticMeshComponent* GetMagazineMesh() const { return Mesh; }
    FVector GetVelocity() const override { return Velocity; }
    FVector GetInitialVelocity() const { return InitialVelocity; }
    FVector GetInheritedVelocity() const { return InheritedVelocity; }
    FTransform GetReleaseTransform() const { return InitialRelease; }
    bool IsSettled() const { return bSettled; }
    int32 GetBounceCount() const { return Bounces; }
    uint64 GetSourceInstanceId() const { return SourceInstanceId; }
    uint64 GetReleaseId() const { return SourceReleaseId; }
private:
    void StepFlight(float Dt);
    UPROPERTY() TObjectPtr<USceneComponent> Center;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    FVector Extent=FVector(2),Velocity=FVector::ZeroVector,InitialVelocity=FVector::ZeroVector,InheritedVelocity=FVector::ZeroVector,Spin=FVector::ZeroVector;
    FTransform InitialRelease=FTransform::Identity;
    uint64 SourceInstanceId=0,SourceReleaseId=0;
    int32 Bounces=0;
    bool bSettled=false;
};
