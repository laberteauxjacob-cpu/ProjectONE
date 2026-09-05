#include "ONEWeaponMagazine.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "ProfilingDebugging/CsvProfiler.h"

AONEWeaponMagazine::AONEWeaponMagazine()
{
    PrimaryActorTick.bCanEverTick=true;
    Center=CreateDefaultSubobject<USceneComponent>(TEXT("MagazineCenter")); RootComponent=Center;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OldMagazine")); Mesh->SetupAttachment(Center);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false);
}
void AONEWeaponMagazine::Initialize(UStaticMesh* Asset,const FTransform& ReleaseTransform,const FVector& OwnerVelocity,float Lifetime,uint64 InstanceId,uint64 ReleaseId)
{
    InitialRelease=ReleaseTransform; SourceInstanceId=InstanceId; SourceReleaseId=ReleaseId;
    Mesh->SetStaticMesh(Asset);
    // Presentation meshes retain a grip-origin pivot. Flight instead uses their
    // actual centered bounds, preserving every visible vertex at release.
    const FBoxSphereBounds Bounds=Asset ? Asset->GetBounds() : FBoxSphereBounds(FVector::ZeroVector,FVector(2),3.f);
    Extent=Bounds.BoxExtent.ComponentMax(FVector(.2f));
    SetActorTransform(FTransform(ReleaseTransform.GetRotation(),ReleaseTransform.TransformPosition(Bounds.Origin),ReleaseTransform.GetScale3D()));
    Mesh->SetRelativeLocation(-Bounds.Origin);
    InheritedVelocity=OwnerVelocity;
    InitialVelocity=Velocity=OwnerVelocity+ReleaseTransform.TransformVectorNoScale(FVector(5.f,FMath::FRandRange(-18.f,18.f),-42.f));
    Spin=FVector(FMath::FRandRange(-150.f,150.f),FMath::FRandRange(-160.f,160.f),FMath::FRandRange(70.f,160.f));
    SetLifeSpan(FMath::Clamp(Lifetime,1.f,20.f));
}
void AONEWeaponMagazine::StepFlight(float Dt)
{
    Velocity.Z+=GetWorld()->GetGravityZ()*Dt;
    const FVector From=GetActorLocation();
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(ONEMagazineFlight),false,this);
    if (GetWorld()->SweepSingleByObjectType(Hit,From,From+Velocity*Dt,GetActorQuat(),FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeBox(Extent),Query))
    {
        const FVector Normal=Hit.ImpactNormal.GetSafeNormal();
        SetActorLocation(Hit.Location+Normal*.08f);
        const float Into=FVector::DotProduct(Velocity,Normal);
        if (Into<0.f) { ++Bounces; Velocity=(Velocity-Normal*Into)*.52f-Normal*Into*.23f; Spin*=.48f; }
        if (Normal.Z>.7f && (Velocity.SizeSquared()<FMath::Square(38.f) || Bounces>=8))
        {
            // The thin side rests against support, then re-sweep downward with
            // that exact box orientation. No floating grip-pivot approximation.
            int32 Thin=Extent.X<Extent.Y ? 0 : 1; if (Extent.Z<Extent[Thin]) Thin=2;
            FVector Axis=FVector::ZeroVector; Axis[Thin]=1.f;
            const FQuat Rest=FQuat::FindBetweenNormals(GetActorQuat().RotateVector(Axis),Normal)*GetActorQuat();
            FHitResult Support;
            const FVector Above=Hit.ImpactPoint+Normal*(Extent.GetMax()+1.f);
            if (GetWorld()->SweepSingleByObjectType(Support,Above,Hit.ImpactPoint-Normal*2.f,Rest,FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeBox(Extent),Query))
            { SetActorLocation(Support.Location+Normal*.06f); SetActorRotation(Rest); }
            Velocity=FVector::ZeroVector; Spin=FVector::ZeroVector; bSettled=true; SetActorTickEnabled(false);
        }
    }
    else SetActorLocation(From+Velocity*Dt);
    if (!bSettled) AddActorWorldRotation(FRotator(Spin.Y,Spin.Z,Spin.X)*Dt);
}
void AONEWeaponMagazine::Tick(float Dt)
{
    CSV_SCOPED_TIMING_STAT_EXCLUSIVE(ONEMagazineFlight);
    Super::Tick(Dt); float Remaining=FMath::Min(Dt,.1f);
    for (int32 N=0;N<12 && Remaining>KINDA_SMALL_NUMBER && !bSettled;++N)
    { const float Step=FMath::Min(Remaining,1.f/120.f); StepFlight(Step); Remaining-=Step; }
}
