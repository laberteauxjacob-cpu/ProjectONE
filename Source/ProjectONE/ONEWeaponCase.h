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
    void Initialize(UStaticMesh* Asset,const FVector& InitialVelocity);
    virtual void Tick(float Dt) override;
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mesh;
    FVector Velocity=FVector::ZeroVector;
    bool bSettled=false;
};
