#include "ONEWeaponCase.h"
#include "ONE05Audio.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
    bShotgunShell=Asset && Asset->GetName().Contains(TEXT("ShotgunShell"));
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
    Params.bReturnPhysicalMaterial=true;
    if (GetWorld()->SweepSingleByObjectType(Hit,From,From+Velocity*Dt,FQuat::Identity,
        FCollisionObjectQueryParams(ECC_WorldStatic),FCollisionShape::MakeSphere(CollisionRadius),Params))
    {
        const FVector Normal=Hit.ImpactNormal.GetSafeNormal();
        SetActorLocation(Hit.Location+Normal*.06f);
        const float Into=FVector::DotProduct(Velocity,Normal);
        if (Into<0.f)
        {
            const double Now=GetWorld()->GetTimeSeconds();
            if (-Into>=65.f && ContactCueCount<3 && Now>=NextContactAudio)
            {
                if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>())
                {
                    const TCHAR* Stem=ONE05Audio::IsMetalContact(Hit)?TEXT("CasingMetal"):(bShotgunShell?TEXT("ShellConcrete"):TEXT("CasingConcrete"));
                    const float Gain=FMath::Clamp(-Into/400.f,.12f,.5f)*(ContactCueCount==0?1.f:.35f);
                    if (Audio->PlayFoley(Stem,3,Hit.ImpactPoint,Gain,.3f)) ++ContactCueCount;
                }
                NextContactAudio=Now+.14;
            }
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
