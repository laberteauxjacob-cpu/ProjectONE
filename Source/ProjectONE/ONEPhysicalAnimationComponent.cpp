#include "ONEPhysicalAnimationComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"

void UONEPhysicalAnimationComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* Tick)
{
    auto* Mesh=GetSkeletalMesh();
    if (!Mesh || !Mesh->IsRegistered() || !Mesh->GetSkeletalMeshAsset()) return;
    const int32 Bones=Mesh->GetSkeletalMeshAsset()->GetRefSkeleton().GetNum();
    const auto* Physics=Mesh->GetPhysicsAsset();
    const int32 Local=Mesh->GetBoneSpaceTransforms().Num();
    const int32 Component=Mesh->GetEditableComponentSpaceTransforms().Num();
    if (!Physics || Local<Bones || Component<Bones || Mesh->Bodies.Num()<Physics->SkeletalBodySetups.Num())
    {
        if (++DeferredTicks<=4)
            UE_LOG(LogTemp,Display,TEXT("ONE07_PHYSICS_WAIT actor=%s local=%d component=%d expected=%d bodies=%d"),
                *GetOwner()->GetName(),Local,Component,Bones,Mesh->Bodies.Num());
        return;
    }
    Super::TickComponent(Dt,TickType,Tick);
}
