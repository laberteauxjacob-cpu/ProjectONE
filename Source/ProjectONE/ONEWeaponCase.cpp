#include "ONEWeaponCase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
AONEWeaponCase::AONEWeaponCase()
{
    PrimaryActorTick.bCanEverTick=true;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EjectedCase")); RootComponent=Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCanEverAffectNavigation(false);
}
void AONEWeaponCase::Initialize(UStaticMesh* Asset,const FVector& InitialVelocity)
{
    Mesh->SetStaticMesh(Asset); Velocity=InitialVelocity; SetLifeSpan(5.f);
}
void AONEWeaponCase::Tick(float Dt)
{
    Super::Tick(Dt); if (bSettled) return;
    const FVector From=GetActorLocation(); Velocity.Z-=780.f*Dt;
    const FVector To=From+Velocity*Dt;
    FHitResult Hit;
    if (GetWorld()->LineTraceSingleByObjectType(Hit,From,To,FCollisionObjectQueryParams(ECC_WorldStatic)))
    {
        SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*1.4f);
        SetActorRotation(FRotator(0,GetActorRotation().Yaw,0)); bSettled=true;
    }
    else { SetActorLocation(To); AddActorWorldRotation(FRotator(330,220,80)*Dt); }
}
