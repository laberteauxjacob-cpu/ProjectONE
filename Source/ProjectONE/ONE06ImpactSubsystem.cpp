#include "ONE06ImpactSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInterface.h"

bool UONE06ImpactSubsystem::EnsureMesh()
{
    if (IsValid(Marks)) return true;
    UWorld* World=GetWorld(); if (!World || !World->IsGameWorld()) return false;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_EyeDark.M_EyeDark"));
    if (!Mesh || !Material) return false;
    FActorSpawnParameters Parameters; Parameters.ObjectFlags|=RF_Transient;
    Parameters.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Anchor=World->SpawnActor<AActor>(AActor::StaticClass(),FVector::ZeroVector,FRotator::ZeroRotator,Parameters);
    if (!Anchor) return false;
    Marks=NewObject<UInstancedStaticMeshComponent>(Anchor,TEXT("ONE06FiniteImpactMarks"));
    Anchor->AddInstanceComponent(Marks); Anchor->SetRootComponent(Marks);
    Marks->SetMobility(EComponentMobility::Movable); Marks->SetStaticMesh(Mesh); Marks->SetMaterial(0,Material);
    Marks->SetCollisionEnabled(ECollisionEnabled::NoCollision); Marks->SetCollisionResponseToAllChannels(ECR_Ignore);
    Marks->SetGenerateOverlapEvents(false); Marks->SetCanEverAffectNavigation(false); Marks->SetCastShadow(false);
    Marks->RegisterComponent(); return true;
}
void UONE06ImpactSubsystem::AddImpact(const FHitResult& Hit,bool bPellet)
{
    if (!Hit.bBlockingHit || Hit.bStartPenetrating || !Hit.Component.IsValid() ||
        Hit.ImpactPoint.ContainsNaN() || Hit.ImpactNormal.ContainsNaN() || !EnsureMesh()) return;
    const FVector Normal=Hit.ImpactNormal.GetSafeNormal(); if (Normal.IsNearlyZero()) return;
    const float Radius=bPellet?.6f:.8f;
    const FTransform Transform(FRotationMatrix::MakeFromZ(Normal).ToQuat(),Hit.ImpactPoint+Normal*.075f,
        FVector(Radius/50.f,Radius/50.f,.0006f));
    const double Expiry=GetWorld()->GetTimeSeconds()+LifetimeSeconds;
    if (ExpiresAt.Num()<Capacity)
    {
        const int32 Slot=Marks->AddInstance(Transform,true);
        if (Slot!=ExpiresAt.Num()) { Clear(); return; }
        ExpiresAt.Add(Expiry); ++ActiveCount;
    }
    else
    {
        const int32 Slot=NextSlot; NextSlot=(NextSlot+1)%Capacity;
        if (ExpiresAt[Slot]<0.) ++ActiveCount;
        Marks->UpdateInstanceTransform(Slot,Transform,true,true,true); ExpiresAt[Slot]=Expiry;
    }
    ++RecordedCount;
}
void UONE06ImpactSubsystem::Tick(float DeltaTime)
{
    if (!IsValid(Marks) || ActiveCount<=0 || !GetWorld()) return;
    const double Now=GetWorld()->GetTimeSeconds();
    bool Changed=false;
    for (int32 Slot=0;Slot<ExpiresAt.Num();++Slot)
        if (ExpiresAt[Slot]>=0. && Now>=ExpiresAt[Slot])
        {
            Marks->UpdateInstanceTransform(Slot,FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector),true,false,true);
            ExpiresAt[Slot]=-1.; --ActiveCount; Changed=true;
        }
    if (Changed) Marks->MarkRenderStateDirty();
}
TStatId UONE06ImpactSubsystem::GetStatId() const
{ RETURN_QUICK_DECLARE_CYCLE_STAT(UONE06ImpactSubsystem,STATGROUP_Tickables); }
void UONE06ImpactSubsystem::Clear()
{
    if (IsValid(Anchor)) Anchor->Destroy();
    Marks=nullptr; Anchor=nullptr; ExpiresAt.Reset(); NextSlot=ActiveCount=RecordedCount=0;
}
void UONE06ImpactSubsystem::Deinitialize()
{ Clear(); Super::Deinitialize(); }
