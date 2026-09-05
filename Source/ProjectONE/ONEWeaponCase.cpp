#include "ONEWeaponCase.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CsvProfiler.h"
AONEWeaponCase::AONEWeaponCase()
{
    PrimaryActorTick.bCanEverTick=true;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("EjectedCase")); RootComponent=Mesh;
    // Movement queries only the static room. Cases never block pawns, shots or
    // one another and are not navigation obstacles.
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetCastShadow(false);
}
void AONEWeaponCase::Initialize(UStaticMesh* Asset,const FVector& EmissionVelocity,const FVector& OwnerVelocity,
    const FVector& Spin,float Radius,float Lifetime,int32 SourceWeapon,uint64 ShotId)
{
    Mesh->SetStaticMesh(Asset);
    InheritedVelocity=OwnerVelocity; InitialVelocity=EmissionVelocity+OwnerVelocity;
    Velocity=InitialVelocity; AngularVelocity=Spin;
    CollisionRadius=FMath::Max(.1f,Radius); WeaponIndex=SourceWeapon; SourceShotId=ShotId;
    SetLifeSpan(FMath::Max(.1f,Lifetime));
}
void AONEWeaponCase::StepFlight(float Dt)
{
    const FVector From=GetActorLocation();
    Velocity.Z+=GetWorld()->GetGravityZ()*Dt;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ONECaseFlight),false,this);
    if (GetWorld()->SweepSingleByObjectType(Hit,From,From+Velocity*Dt,FQuat::Identity,
        FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeSphere(CollisionRadius),Params))
    {
        const FVector Normal=Hit.ImpactNormal.GetSafeNormal();
        SetActorLocation(Hit.Location+Normal*.06f);
        const float Into=FVector::DotProduct(Velocity,Normal);
        if (Into<0.f)
        {
            ++BounceCount;
            Velocity=(Velocity-Normal*Into)*.58f-Normal*Into*.30f;
            AngularVelocity*=.52f;
        }
        if (Normal.Z>.65f && (Velocity.SizeSquared()<FMath::Square(46.f) || BounceCount>=7))
        {
            const FVector Along=FVector::VectorPlaneProject(GetActorForwardVector(),Normal).GetSafeNormal();
            SetActorRotation(FRotationMatrix::MakeFromXZ(Along.IsNearlyZero() ? FVector::ForwardVector : Along,Normal).ToQuat());
            Velocity=FVector::ZeroVector; AngularVelocity=FVector::ZeroVector;
            bSettled=true; SetActorTickEnabled(false);
        }
    }
    else SetActorLocation(From+Velocity*Dt);
    if (!bSettled) AddActorWorldRotation(FRotator(AngularVelocity.Y,AngularVelocity.Z,AngularVelocity.X)*Dt);
}
void AONEWeaponCase::Tick(float Dt)
{
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(ONECaseFlight);
    Super::Tick(Dt);
    // Bound collision work at hitches; a cosmetic case may lose excess time
    // rather than tunnel through the room or consume unbounded substeps.
    float Remaining=FMath::Min(Dt,.1f);
    for (int32 Step=0;Step<12 && Remaining>KINDA_SMALL_NUMBER && !bSettled;++Step)
    { const float H=FMath::Min(Remaining,1.f/120.f); StepFlight(H); Remaining-=H; }
}
