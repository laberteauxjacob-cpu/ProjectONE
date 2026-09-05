#include "ONEBloodSubsystem.h"
#include "ProceduralMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"

AONEBloodEffect::AONEBloodEffect()
{
    PrimaryActorTick.bCanEverTick=true;
    Mesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Droplets"));
    RootComponent=Mesh;
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetCastShadow(false);
}
void AONEBloodEffect::InitSpray(const FVector& Direction,UMaterialInterface* Material,bool bSever)
{
    bShot=false; Mesh->SetMaterial(0,Material);
    const int32 Count=bSever ? 12 : 7;
    for (int32 I=0;I<Count;++I)
    {
        Positions.Add(FVector::ZeroVector);
        Velocities.Add(Direction*FMath::FRandRange(110.f,290.f)+FMath::VRand()*FMath::FRandRange(45.f,100.f)+FVector(0,0,70));
    }
    SetLifeSpan(.52f); BuildGeometry();
}
void AONEBloodEffect::InitShot(const FVector& End,UMaterialInterface* Material)
{
    bShot=true; ShotEnd=End-GetActorLocation(); Mesh->SetMaterial(0,Material);
    SetLifeSpan(.045f); BuildGeometry();
}
void AONEBloodEffect::BuildGeometry()
{
    TArray<FVector> V,N;
    TArray<int32> T;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    auto Ribbon=[&](FVector A,FVector B,float Width)
    {
        FVector Axis=(B-A).GetSafeNormal();
        FVector Side=FVector::CrossProduct(Axis,FVector::UpVector).GetSafeNormal()*Width;
        if (Side.IsNearlyZero()) Side=FVector(Width,0,0);
        FVector Up=FVector::CrossProduct(Axis,Side).GetSafeNormal()*Width;
        for (FVector Offset:{Side,Up})
        {
            const int32 Base=V.Num();
            V.Append({A-Offset,A+Offset,B+Offset,B-Offset});
            T.Append({Base,Base+1,Base+2,Base,Base+2,Base+3,Base+2,Base+1,Base,Base+3,Base+2,Base});
            UV.Append({FVector2D(0,0),FVector2D(0,1),FVector2D(1,1),FVector2D(1,0)});
            for (int32 K=0;K<4;++K) N.Add(FVector::UpVector);
        }
    };
    if (bShot)
    {
        Ribbon(FVector::ZeroVector,ShotEnd,.55f);
        Ribbon(FVector::ZeroVector,ShotEnd.GetSafeNormal()*19.f,3.2f);
    }
    else
    {
        for (int32 I=0;I<Positions.Num();++I)
        {
            const FVector Axis=Velocities[I].GetSafeNormal();
            Ribbon(Positions[I]-Axis*3.f,Positions[I]+Axis*3.f,1.1f);
        }
    }
    Mesh->CreateMeshSection_LinearColor(0,V,T,N,UV,Colors,Tangents,false);
}
void AONEBloodEffect::Tick(float Dt)
{
    Super::Tick(Dt); Age+=Dt;
    if (bShot) return;
    for (int32 I=0;I<Positions.Num();++I)
    {
        Velocities[I].Z-=650.f*Dt;
        Positions[I]+=Velocities[I]*Dt;
    }
    BuildGeometry();
}

AONEGorePiece::AONEGorePiece()
{
    PrimaryActorTick.bCanEverTick=true;
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("AnimatedSeverPivot"));
    Piece=CreateDefaultSubobject<UPoseableMeshComponent>(TEXT("FrozenSkinnedPart"));
    Piece->SetupAttachment(RootComponent);
    Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Piece->SetCanEverAffectNavigation(false);
}
void AONEGorePiece::Initialize(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction)
{
    if (!Part || !PoseSource || !Part->GetSkeletalMeshAsset()) { Destroy(); return; }
    SetActorLocation(PoseSource->GetSocketLocation(Bone));
    Piece->SetSkinnedAssetAndUpdate(Part->GetSkeletalMeshAsset());
    Piece->SetWorldTransform(Part->GetComponentTransform());
    // Preserve the evaluated bone transforms, including the forearm's current bend.
    Piece->CopyPoseFromSkeletalComponent(PoseSource);
    for (int32 I=0;I<Part->GetNumMaterials();++I) Piece->SetMaterial(I,Part->GetMaterial(I));
    Velocity=Direction*FMath::FRandRange(130.f,210.f)+FVector(0,0,140);
    Spin=FRotator(FMath::FRandRange(-140.f,140.f),110,95);
    SetLifeSpan(18.f);
}
void AONEGorePiece::Tick(float Dt)
{
    Super::Tick(Dt); Age+=Dt;
    if (bSettled) return;
    Velocity.Z-=720.f*Dt;
    FVector From=GetActorLocation(),To=From+Velocity*Dt;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(GoreBounce),false,this);
    if (GetWorld()->LineTraceSingleByObjectType(Hit,From,To-FVector(0,0,7),FCollisionObjectQueryParams(ECC_WorldStatic),Params))
    {
        SetActorLocation(Hit.ImpactPoint+Hit.ImpactNormal*9.f);
        Velocity=FMath::GetReflectionVector(Velocity,Hit.ImpactNormal)*.24f;
        Spin*=.3f;
        if (Velocity.Size()<85.f || Age>2.f)
        {
            bSettled=true;
            if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->Pool(GetActorLocation(),24.f);
        }
    }
    else SetActorLocation(To);
    if (!bSettled) AddActorWorldRotation(Spin*Dt);
    if (Age>3.f) bSettled=true;
}

void UONEBloodSubsystem::Initialize(FSubsystemCollectionBase& Collection) { Super::Initialize(Collection); }
void UONEBloodSubsystem::LoadMaterials()
{
    if (!BloodMaterial) BloodMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_Blood.M_Blood"));
    if (!FleshMaterial) FleshMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_BloodFlesh.M_BloodFlesh"));
    if (!MuzzleMaterial) MuzzleMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_Muzzle.M_Muzzle"));
}
void UONEBloodSubsystem::BoundActors(TArray<TWeakObjectPtr<AActor>>& Actors,int32 Max)
{
    Actors.RemoveAll([](const TWeakObjectPtr<AActor>& A){return !A.IsValid();});
    while (Actors.Num()>Max) { if (Actors[0].IsValid()) Actors[0]->Destroy(); Actors.RemoveAt(0); }
}
void UONEBloodSubsystem::Impact(const FVector& Position,const FVector& Direction,bool bSever)
{
    LoadMaterials();
    if (auto* E=GetWorld()->SpawnActor<AONEBloodEffect>(Position,FRotator::ZeroRotator))
    { E->InitSpray(Direction,FleshMaterial,bSever); Effects.Add(E); BoundActors(Effects,32); }
    Pool(Position+Direction*FMath::FRandRange(15.f,65.f),bSever ? 42.f : FMath::FRandRange(17.f,29.f));
}
void UONEBloodSubsystem::Shot(const FVector& Start,const FVector& End)
{
    LoadMaterials();
    if (auto* E=GetWorld()->SpawnActor<AONEBloodEffect>(Start,FRotator::ZeroRotator))
    { E->InitShot(End,MuzzleMaterial); Effects.Add(E); BoundActors(Effects,32); }
}
void UONEBloodSubsystem::Pool(const FVector& Position,float Size)
{
    LoadMaterials(); if (!BloodMaterial) return;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BloodSurface),false);
    if (!GetWorld()->LineTraceSingleByObjectType(Hit,Position+FVector(0,0,20),Position-FVector(0,0,300),FCollisionObjectQueryParams(ECC_WorldStatic),Params)) return;
    FRotator Rotation=Hit.ImpactNormal.Rotation(); Rotation.Roll=FMath::FRandRange(0.f,360.f);
    if (UDecalComponent* D=UGameplayStatics::SpawnDecalAtLocation(GetWorld(),BloodMaterial,FVector(8,Size,Size*.78f),Hit.ImpactPoint,Rotation,150.f))
    {
        D->SetFadeIn(.0f,.22f); D->SetFadeOut(140.f,10.f,false);
        D->FadeScreenSize=.001f;
        Decals.Add(D);
        Decals.RemoveAll([](const TWeakObjectPtr<UDecalComponent>& C){return !C.IsValid();});
        while (Decals.Num()>90) { if (Decals[0].IsValid()) Decals[0]->DestroyComponent(); Decals.RemoveAt(0); }
    }
}
void UONEBloodSubsystem::Detach(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction)
{
    if (auto* G=GetWorld()->SpawnActor<AONEGorePiece>())
    { G->Initialize(Part,PoseSource,Bone,Direction); Pieces.Add(G); BoundActors(Pieces,18); }
}
void UONEBloodSubsystem::RegisterCorpse(AActor* Corpse) { Corpses.Add(Corpse); BoundActors(Corpses,14); }
void UONEBloodSubsystem::ClearPresentation()
{
    for (auto* Collection : {&Effects,&Pieces,&Corpses})
    {
        for (const auto& Actor : *Collection) if (Actor.IsValid()) Actor->Destroy();
        Collection->Reset();
    }
    for (const auto& Decal : Decals) if (Decal.IsValid()) Decal->DestroyComponent();
    Decals.Reset();
}
int32 UONEBloodSubsystem::GetDecalCount() const { return Decals.FilterByPredicate([](const auto& C){return C.IsValid();}).Num(); }
int32 UONEBloodSubsystem::GetPieceCount() const { return Pieces.FilterByPredicate([](const auto& C){return C.IsValid();}).Num(); }
int32 UONEBloodSubsystem::GetCorpseCount() const { return Corpses.FilterByPredicate([](const auto& C){return C.IsValid();}).Num(); }
