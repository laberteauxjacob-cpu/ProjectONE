#include "ONEBloodSubsystem.h"
#include "ProceduralMeshComponent.h"
#include "ONESnapshotAnimInstance.h"
#include "ONEPhysicsRuntime.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "CoreGlobals.h"

CSV_DECLARE_CATEGORY_EXTERN(ONEPhysicality);

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
void AONEBloodEffect::InitManagedDrops(UMaterialInterface* Material)
{ bManaged=true; Mesh->SetMaterial(0,Material); SetActorTickEnabled(false); }
void AONEBloodEffect::UpdateManagedDrops(const TArray<FVector>& Points,const TArray<FVector>& Motion)
{ Positions=Points; Velocities=Motion; BuildGeometry(); }

AONEGorePiece::AONEGorePiece()
{
    PrimaryActorTick.bCanEverTick=false;
    Piece=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("PhysicalSeveredPart"));
    RootComponent=Piece;
    Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Piece->SetCanEverAffectNavigation(false);
    Piece->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}
void AONEGorePiece::Initialize(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction)
{
    if (!Part || !PoseSource || !Part->GetSkeletalMeshAsset()) { Destroy(); return; }
    CSV_SCOPED_TIMING_STAT(ONEPhysicality,PartInitialize);
    CapturedSourceBoneTransform=PoseSource->GetSocketTransform(Bone,RTS_World);
    FPoseSnapshot Snapshot; PoseSource->SnapshotPose(Snapshot);
    Piece->SetSkeletalMesh(Part->GetSkeletalMeshAsset());
    Piece->SetWorldTransform(Part->GetComponentTransform());
    for (int32 I=0;I<Part->GetNumMaterials();++I) Piece->SetMaterial(I,Part->GetMaterial(I));
    const TCHAR* PartName=Bone==TEXT("head") ? TEXT("Head") : Bone==TEXT("upperarm_r") ? TEXT("ArmLeft") :
        Bone==TEXT("upperarm_l") ? TEXT("ArmRight") : TEXT("LegLeft");
    const FString Asset=FString::Printf(TEXT("/Game/ONE/Characters/Candidate03/PA_Infected_%s_C03.PA_Infected_%s_C03"),PartName,PartName);
    Piece->SetPhysicsAsset(LoadObject<UPhysicsAsset>(nullptr,*Asset),true);
    Piece->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    Piece->SetAnimInstanceClass(UONESnapshotAnimInstance::StaticClass());
    if (auto* Anim=Cast<UONESnapshotAnimInstance>(Piece->GetAnimInstance())) Anim->CapturedPose=Snapshot;
    Piece->TickAnimation(0.f,false); Piece->RefreshBoneTransforms();
    const FVector Inherited=PoseSource->IsSimulatingPhysics(Bone) ? PoseSource->GetPhysicsLinearVelocity(Bone) :
        PoseSource->GetOwner() ? PoseSource->GetOwner()->GetVelocity() : FVector::ZeroVector;
    const auto Started=ONEPhysicsRuntime::Start(Piece,Inherited,{});
    ONEPhysicsRuntime::ResetRest(Piece,RestState,false);
    TransitionErrorCm=Started.PositionErrorCm;
    if (!Started.SimulatedBodies)
    { UE_LOG(LogTemp,Error,TEXT("ONE_PART_PHYSICS missing bodies for %s"),PartName); }
    else Piece->AddImpulseAtLocation(Direction.GetSafeNormal()*180.f+FVector(0,0,45.f),CapturedSourceBoneTransform.GetLocation(),Bone);
    SetLifeSpan(18.f);
    if (Started.SimulatedBodies)
    {
        FTimerManagerTimerParameters Parameters; Parameters.bLoop=true; Parameters.bMaxOncePerFrame=true;
        GetWorld()->GetTimerManager().SetTimer(RestTimer,this,&AONEGorePiece::ObserveRest,.1f,Parameters);
    }
}
void AONEGorePiece::ObserveRest() { ONEPhysicsRuntime::UpdateRest(Piece,RestState); }
void AONEGorePiece::EndPlay(const EEndPlayReason::Type Reason)
{ GetWorld()->GetTimerManager().ClearTimer(RestTimer); if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->RemoveSourcesForActor(this); Super::EndPlay(Reason); }
int32 AONEGorePiece::GetActivePhysicsBodyCount() const { return ONEPhysicsRuntime::Count(Piece); }
int32 AONEGorePiece::GetAwakePhysicsBodyCount() const { return ONEPhysicsRuntime::Count(Piece,true); }

void UONEBloodSubsystem::Initialize(FSubsystemCollectionBase& Collection) { Super::Initialize(Collection); }
void UONEBloodSubsystem::Deinitialize()
{ ClearPresentation(); Super::Deinitialize(); }
void UONEBloodSubsystem::EnsureScheduler()
{
    if (!bClearing && !GetWorld()->GetTimerManager().IsTimerActive(BloodTimer))
    {
        FTimerManagerTimerParameters Parameters; Parameters.bLoop=true; Parameters.bMaxOncePerFrame=true;
        GetWorld()->GetTimerManager().SetTimer(BloodTimer,this,&UONEBloodSubsystem::StepBlood,.1f,Parameters);
    }
}
void UONEBloodSubsystem::LoadMaterials()
{
    if (!BloodMaterial) BloodMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_Blood.M_Blood"));
    if (!PoolMaterial) PoolMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_BloodPool_C03.M_BloodPool_C03"));
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
    if (bClearing || Position.ContainsNaN() || Direction.ContainsNaN()) return;
    LoadMaterials();
    if (auto* E=GetWorld()->SpawnActor<AONEBloodEffect>(Position,FRotator::ZeroRotator))
    { E->InitSpray(Direction,FleshMaterial,bSever); Effects.Add(E); BoundActors(Effects,32); }
    // The impact spray is brief. Continuing fluid belongs to the weak wound
    // scheduler, which projects each deposited drop and grows compatible pools.
}
void UONEBloodSubsystem::Shot(const FVector& Start,const FVector& End)
{
    LoadMaterials();
    if (auto* E=GetWorld()->SpawnActor<AONEBloodEffect>(Start,FRotator::ZeroRotator))
    { E->InitShot(End,MuzzleMaterial); Effects.Add(E); BoundActors(Effects,32); }
}
void UONEBloodSubsystem::Pool(const FVector& Position,float Size)
{
    if (bClearing || Position.ContainsNaN() || !FMath::IsFinite(Size) || Size<=0) return;
    LoadMaterials(); EnsureScheduler();
    const float Volume=FMath::Square(FMath::Clamp(Size,5.f,80.f)-5.f)/36.f+.1f;
    if (TakeTraceBudget()) ProjectSurface(Position,Volume);
    else if (PendingSurfaces.Num()<96) PendingSurfaces.Add({Position,Volume,float(GetWorld()->GetTimeSeconds()+2.f)});
}
bool UONEBloodSubsystem::TakeTraceBudget()
{
    if (TraceFrame!=GFrameCounter) { TraceFrame=GFrameCounter; FrameTraces=0; }
    if (FrameTraces>=12) return false;
    ++FrameTraces; if (bInsideStep) ++LastStepTraces;
    return true;
}
void UONEBloodSubsystem::ProjectSurface(const FVector& Position,float Volume)
{
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(BloodSurface),false);
    if (GetWorld()->LineTraceSingleByObjectType(Hit,Position+FVector(0,0,20),Position-FVector(0,0,350),FCollisionObjectQueryParams(ECC_WorldStatic),Params)) Deposit(Hit,Volume);
}
void UONEBloodSubsystem::TrackDecal(UDecalComponent* Decal)
{
    if (!Decal) return;
    Decals.Add(Decal);
    Decals.RemoveAll([](const auto& D){return !D.IsValid();});
    while (Decals.Num()>90) { if (Decals[0].IsValid()) Decals[0]->DestroyComponent(); Decals.RemoveAt(0); }
}
void UONEBloodSubsystem::Deposit(const FHitResult& Hit,float Volume)
{
    if (!PoolMaterial || Volume<=0.f) return;
    FONEBloodPool* PoolRecord=nullptr;
    for (auto& Existing:Pools)
        if (Existing.Decal.IsValid() && FVector::DotProduct(Existing.Normal,Hit.ImpactNormal)>.95f &&
            FMath::Abs(FVector::DotProduct(Hit.ImpactPoint-Existing.Position,Existing.Normal))<1.5f &&
            FVector::Dist(Hit.ImpactPoint,Existing.Position)<FMath::Max(15.f,Existing.Radius*.6f))
        { PoolRecord=&Existing; break; }
    if (!PoolRecord)
    {
        FRotator Rotation=Hit.ImpactNormal.Rotation(); Rotation.Roll=FMath::FRandRange(0.f,360.f);
        auto* D=UGameplayStatics::SpawnDecalAtLocation(GetWorld(),PoolMaterial,FVector(4,1,.8f),Hit.ImpactPoint,Rotation,150.f);
        if (!D) return;
        D->SetWorldScale3D(FVector(1,3,3));
        D->SetFadeIn(0.f,.12f); D->SetFadeOut(140.f,10.f,false); D->FadeScreenSize=.001f;
        TrackDecal(D); FONEBloodPool New; New.Decal=D; New.Position=Hit.ImpactPoint; New.Normal=Hit.ImpactNormal; New.Radius=3.f;
        PoolRecord=&Pools.Add_GetRef(New);
    }
    PoolRecord->Volume=FMath::Min(180.f,PoolRecord->Volume+Volume);
    PoolRecord->TargetRadius=FMath::Clamp(5.f+FMath::Sqrt(PoolRecord->Volume)*6.f,5.f,80.f);
    DepositedVolume+=Volume;
}
AONEGorePiece* UONEBloodSubsystem::Detach(USkeletalMeshComponent* Part,USkeletalMeshComponent* PoseSource,FName Bone,const FVector& Direction)
{
    if (auto* G=GetWorld()->SpawnActor<AONEGorePiece>())
    { G->Initialize(Part,PoseSource,Bone,Direction); Pieces.Add(G); BoundActors(Pieces,18); return G; }
    return nullptr;
}
void UONEBloodSubsystem::RegisterCorpse(AActor* Corpse) { Corpses.Add(Corpse); BoundActors(Corpses,14); }
void UONEBloodSubsystem::AddWound(USkeletalMeshComponent* Source,EONEHitRegion Region,FName Bone,const FVector& Position,const FVector& Normal,float Volume,bool bHeavyBleed)
{
    if (bClearing || !IsValid(Source) || !IsValid(Source->GetOwner()) || Source->GetBoneIndex(Bone)==INDEX_NONE ||
        Position.ContainsNaN() || Normal.ContainsNaN() || !FMath::IsFinite(Volume) || Volume<=0) return;
    LoadMaterials(); EnsureScheduler();
    const float Now=GetWorld()->GetTimeSeconds();
    Wounds.RemoveAll([Now](const auto& W){return !W.Source.IsValid() || W.Remaining<=0 || W.Expires<=Now;});
    FONEBleedingWound* Wound=Wounds.FindByPredicate([&](const auto& W){return W.Source==Source && W.Region==Region;});
    if (!Wound)
    {
        int32 Owned=0,Oldest=INDEX_NONE;
        for (int32 I=0;I<Wounds.Num();++I) if (Wounds[I].Source.IsValid() && Wounds[I].Source->GetOwner()==Source->GetOwner())
        { ++Owned; if (Oldest==INDEX_NONE) Oldest=I; }
        if (Owned>=3) Wounds.RemoveAt(Oldest);
        if (Wounds.Num()>=96) Wounds.RemoveAt(0);
        FONEBleedingWound New; New.Source=Source; New.Region=Region; New.NextDrop=Now;
        Wound=&Wounds.Add_GetRef(New);
        // A small bone-attached wound mark provides local injury presentation;
        // it is not a claimed mesh cavity and shares the total decal budget.
        if (BloodMaterial)
        {
            auto* Mark=UGameplayStatics::SpawnDecalAttached(BloodMaterial,FVector(3,bHeavyBleed?10.f:6.f,bHeavyBleed?8.f:5.f),Source,Bone,
                Position,Normal.Rotation(),EAttachLocation::KeepWorldPosition,28.f);
            if (Mark) { Mark->FadeScreenSize=.0005f; Mark->SetFadeOut(22.f,6.f,false); TrackDecal(Mark); }
        }
    }
    const FTransform Anchor=Source->GetSocketTransform(Bone,RTS_World);
    Wound->Bone=Bone; Wound->LocalAnchor=Anchor.InverseTransformPosition(Position).GetClampedToMaxSize(45.f);
    Wound->LocalNormal=Anchor.InverseTransformVectorNoScale(Normal.GetSafeNormal(SMALL_NUMBER,FVector::UpVector));
    Wound->bHeavyBleed|=bHeavyBleed; Wound->Remaining=FMath::Min(48.f,Wound->Remaining+Volume);
    Wound->Expires=Now+(Wound->bHeavyBleed ? 8.f : 3.f);
}
void UONEBloodSubsystem::RemoveSourcesForActor(AActor* Actor)
{
    Wounds.RemoveAll([Actor](const auto& W){return !W.Source.IsValid() || W.Source->GetOwner()==Actor;});
    Drops.RemoveAll([Actor](const auto& D){return !D.Owner.IsValid() || D.Owner==Actor;});
}
void UONEBloodSubsystem::StepBlood()
{
    CSV_SCOPED_TIMING_STAT(ONEPhysicality,BloodFixedStep);
    if (bClearing) return;
    bInsideStep=true; LastStepTraces=0;
    constexpr float Step=.1f;
    const float Now=GetWorld()->GetTimeSeconds();
    Wounds.RemoveAll([Now](const auto& W){return !W.Source.IsValid() || !IsValid(W.Source->GetOwner()) || W.Remaining<=0.f || W.Expires<=Now;});
    Drops.RemoveAll([](const auto& D){return !D.Owner.IsValid() || D.Age>=2.5f;});
    for (auto& Drop:Drops) { Drop.Age+=Step; Drop.Velocity.Z-=650.f*Step; Drop.Position+=Drop.Velocity*Step; }
    const int32 Count=Drops.Num();
    for (int32 K=0;K<Count && Count>0;++K)
    {
        const int32 I=(DropCursor+K)%Count;
        if (!TakeTraceBudget()) break;
        auto& Drop=Drops[I];
        FHitResult Hit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(BloodDropContact),false);
        if (GetWorld()->LineTraceSingleByObjectType(Hit,Drop.LastTrace,Drop.Position,FCollisionObjectQueryParams(ECC_WorldStatic),Params))
        { Deposit(Hit,Drop.Volume); Drop.Age=3.f; }
        else Drop.LastTrace=Drop.Position;
    }
    DropCursor=Count>0 ? (DropCursor+12)%Count : 0;
    Drops.RemoveAll([](const auto& D){return D.Age>=2.5f;});
    for (auto& Wound:Wounds)
    {
        if (Now<Wound.NextDrop || Drops.Num()>=48) continue;
        auto* Source=Wound.Source.Get(); if (!Source) continue;
        const FTransform Anchor=Source->GetSocketTransform(Wound.Bone,RTS_World);
        const FVector Normal=Anchor.TransformVectorNoScale(Wound.LocalNormal).GetSafeNormal();
        const FVector Inherited=Source->IsSimulatingPhysics(Wound.Bone) ? Source->GetPhysicsLinearVelocity(Wound.Bone) : Source->GetOwner()->GetVelocity();
        FONEBloodDrop New; New.Owner=Source->GetOwner(); New.Position=Anchor.TransformPosition(Wound.LocalAnchor)+Normal*1.5f; New.LastTrace=New.Position;
        New.Velocity=Inherited.GetClampedToMaxSize(420.f)*.28f+Normal*20.f+FVector(FMath::FRandRange(-12.f,12.f),FMath::FRandRange(-12.f,12.f),-60.f);
        New.Volume=FMath::Min(Wound.Remaining,Wound.bHeavyBleed ? .7f : .45f); Wound.Remaining-=New.Volume;
        Wound.NextDrop=Now+(Wound.bHeavyBleed ? .18f : .35f); Drops.Add(New);
    }
    PendingSurfaces.RemoveAll([Now](const auto& P){return P.Expires<=Now;});
    while (!PendingSurfaces.IsEmpty() && TakeTraceBudget())
    { const auto Pending=PendingSurfaces[0]; PendingSurfaces.RemoveAt(0); ProjectSurface(Pending.Position,Pending.Volume); }
    Pools.RemoveAll([](const auto& P){return !P.Decal.IsValid();});
    for (auto& PoolRecord:Pools)
    {
        if (FMath::IsNearlyEqual(PoolRecord.Radius,PoolRecord.TargetRadius,.001f)) continue;
        PoolRecord.Radius=FMath::FInterpConstantTo(PoolRecord.Radius,PoolRecord.TargetRadius,Step,10.f);
        // Transform updates retain the decal proxy's original fade timestamps.
        // Recreating its render state every growth step restarted the 0.12s
        // fade-in every 0.1s and repeatedly suppressed the growing pool.
        PoolRecord.Decal->SetWorldScale3D(FVector(1,PoolRecord.Radius,PoolRecord.Radius));
    }
    if (!Drops.IsEmpty() && !DropRenderer.IsValid())
        if (auto* Renderer=GetWorld()->SpawnActor<AONEBloodEffect>()) { Renderer->InitManagedDrops(FleshMaterial); DropRenderer=Renderer; }
    if (DropRenderer.IsValid())
    {
        TArray<FVector> Points,Motion;
        for (const auto& Drop:Drops) { Points.Add(Drop.Position); Motion.Add(Drop.Velocity); }
        DropRenderer->UpdateManagedDrops(Points,Motion);
    }
    CSV_CUSTOM_STAT(ONEPhysicality,Wounds,Wounds.Num(),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,Droplets,Drops.Num(),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,Pools,Pools.Num(),ECsvCustomStatOp::Set);
    CSV_CUSTOM_STAT(ONEPhysicality,ProjectionTraces,LastStepTraces,ECsvCustomStatOp::Set);
    bInsideStep=false;
    if (Wounds.IsEmpty() && Drops.IsEmpty() && PendingSurfaces.IsEmpty() && Pools.IsEmpty())
        GetWorld()->GetTimerManager().ClearTimer(BloodTimer);
}
void UONEBloodSubsystem::ClearPresentation()
{
    bClearing=true; ++Generation; GetWorld()->GetTimerManager().ClearTimer(BloodTimer);
    Wounds.Reset(); Drops.Reset(); Pools.Reset(); PendingSurfaces.Reset();
    if (DropRenderer.IsValid()) DropRenderer->Destroy(); DropRenderer.Reset();
    for (auto* Collection : {&Effects,&Pieces,&Corpses})
    {
        for (const auto& Actor : *Collection) if (Actor.IsValid()) Actor->Destroy();
        Collection->Reset();
    }
    for (const auto& Decal : Decals) if (Decal.IsValid()) Decal->DestroyComponent();
    Decals.Reset();
    DepositedVolume=0; LastStepTraces=0; FrameTraces=0; DropCursor=0; bInsideStep=false; bClearing=false;
}
int32 UONEBloodSubsystem::GetDecalCount() const { return Decals.FilterByPredicate([](const auto& C){return C.IsValid();}).Num(); }
int32 UONEBloodSubsystem::GetPieceCount() const { return Pieces.FilterByPredicate([](const auto& C){return C.IsValid();}).Num(); }
int32 UONEBloodSubsystem::GetCorpseCount() const { return Corpses.FilterByPredicate([](const auto& C){return C.IsValid();}).Num(); }
int32 UONEBloodSubsystem::GetWoundCount() const { return Wounds.FilterByPredicate([](const auto& W){return W.Source.IsValid() && W.Remaining>0.f;}).Num(); }
int32 UONEBloodSubsystem::GetPoolCount() const { return Pools.FilterByPredicate([](const auto& P){return P.Decal.IsValid();}).Num(); }
float UONEBloodSubsystem::GetRemainingBloodVolume() const
{ float Volume=0; for (const auto& W:Wounds) if (W.Source.IsValid()) Volume+=W.Remaining; for (const auto& D:Drops) if (D.Owner.IsValid()) Volume+=D.Volume; return Volume; }
float UONEBloodSubsystem::GetLargestPoolRadius() const
{ float Radius=0; for (const auto& P:Pools) if (P.Decal.IsValid()) Radius=FMath::Max(Radius,P.Radius); return Radius; }
float UONEBloodSubsystem::GetPoolRenderRadiusErrorCm() const
{
    float Error=0;
    for (const auto& P:Pools) if (P.Decal.IsValid())
    {
        const FVector Extent=P.Decal->DecalSize*P.Decal->GetComponentScale();
        Error=FMath::Max(Error,float(FMath::Abs(Extent.Y-P.Radius)));
        Error=FMath::Max(Error,float(FMath::Abs(Extent.Z-P.Radius*.8f)));
    }
    return Error;
}
FString UONEBloodSubsystem::DescribePools() const
{
    FString Result;
    for (const auto& P:Pools) if (P.Decal.IsValid())
        Result+=FString::Printf(TEXT("pos=(%s) normal=(%s) radius=%.3f target=%.3f volume=%.3f render_extent=(%s)\n"),
            *P.Position.ToString(),*P.Normal.ToString(),P.Radius,P.TargetRadius,P.Volume,
            *(P.Decal->DecalSize*P.Decal->GetComponentScale()).ToString());
    return Result;
}
AONEGorePiece* UONEBloodSubsystem::GetLastDetachedPiece() const
{ for (int32 I=Pieces.Num()-1;I>=0;--I) if (Pieces[I].IsValid()) return Cast<AONEGorePiece>(Pieces[I].Get()); return nullptr; }
