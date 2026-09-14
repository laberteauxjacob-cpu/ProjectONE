#include "ONE07FootstepComponent.h"
#include "ONE05Audio.h"
#include "ONEPlayer.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"

UONE07FootstepComponent::UONE07FootstepComponent()
{
    PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
void UONE07FootstepComponent::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Player=Cast<AONEPlayer>(GetOwner())) if (Player->GetMesh()) AddTickPrerequisiteComponent(Player->GetMesh());
}
void UONE07FootstepComponent::TickComponent(float Dt,ELevelTick Type,FActorComponentTickFunction* Function)
{
    Super::TickComponent(Dt,Type,Function);
    auto* Player=Cast<AONEPlayer>(GetOwner());
    if (!Player || Player->IsDead() || !Player->GetCharacterMovement()->IsMovingOnGround() ||
        Player->GetVelocity().SizeSquared2D()<FMath::Square(15.f) || Player->GetMesh()->IsSimulatingPhysics())
    { Raised[0]=Raised[1]=false; PreviousClearance[0]=PreviousClearance[1]=0.f; return; }
    const FName Toes[]={TEXT("toe_r"),TEXT("toe_l")};
    const FName Feet[]={TEXT("foot_r"),TEXT("foot_l")};
    const double Now=GetWorld()->GetTimeSeconds();
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ONE07PlayerFeet),false,Player); Query.bReturnPhysicalMaterial=true;
    for (int32 I=0;I<2;++I)
    {
        auto* Mesh=Player->GetMesh(); const FName Bone=Mesh->GetBoneIndex(Toes[I])!=INDEX_NONE?Toes[I]:Feet[I];
        if (Mesh->GetBoneIndex(Bone)==INDEX_NONE) continue;
        const FVector Foot=Mesh->GetSocketLocation(Bone); FHitResult Hit;
        if (!GetWorld()->LineTraceSingleByObjectType(Hit,Foot+FVector(0,0,12),Foot-FVector(0,0,45),
            FCollisionObjectQueryParams(ECC_WorldStatic),Query) || Hit.ImpactNormal.Z<.55f)
        { Raised[I]=false; PreviousClearance[I]=0.f; continue; }
        const float Clearance=Foot.Z-Hit.ImpactPoint.Z;
        if (Clearance>9.f) Raised[I]=true;
        if (Raised[I] && Clearance<=6.f && Clearance<PreviousClearance[I] && Now>=NextAllowed[I])
        {
            if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>()) Audio->PlayFootContact(Hit,true);
            ++FootContactCount; Raised[I]=false; NextAllowed[I]=Now+.18;
        }
        PreviousClearance[I]=Clearance;
    }
}
