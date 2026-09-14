#include "ONE07PhysicsAssets.h"
#if WITH_EDITOR
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "HAL/FileManager.h"
#endif

bool UONE07PhysicsAssets::BuildInfectedAssets()
{
#if WITH_EDITOR
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/Candidate07/SK_Infected_C07_Maintenance_Core.SK_Infected_C07_Maintenance_Core"));
    if (!Mesh) return false;
    const auto& Ref=Mesh->GetRefSkeleton();
    const int32 HeadIndex=Ref.FindBoneIndex(TEXT("head"));
    if (HeadIndex==INDEX_NONE) return false;
    FTransform HeadBind=FTransform::Identity;
    for (int32 I=HeadIndex;I!=INDEX_NONE;I=Ref.GetParentIndex(I)) HeadBind*=Ref.GetRefBonePose()[I];
    const FString OldBase=TEXT("/Game/ONE/Characters/Candidate03/");
    const FString Base=TEXT("/Game/ONE/Characters/Candidate07/");
    // Preserve the accepted constrained body, solver, mass and cut-plane
    // contract. The new skull is narrower and extends higher than the C03 PA.
    // Never edit or resave any historical asset.
    for (const FString Suffix:{FString(),FString(TEXT("_Head")),FString(TEXT("_ArmLeft")),FString(TEXT("_ArmRight")),FString(TEXT("_LegLeft"))})
    {
        const FString OldName=TEXT("PA_Infected")+Suffix+TEXT("_C03");
        const FString Name=TEXT("PA_Infected")+Suffix+TEXT("_C07");
        auto* Source=LoadObject<UPhysicsAsset>(nullptr,*(OldBase+OldName+TEXT(".")+OldName));
        if (!Source) return false;
        UPhysicsAsset* Asset=nullptr;
        if (FPackageName::DoesPackageExist(Base+Name)) Asset=LoadObject<UPhysicsAsset>(nullptr,*(Base+Name+TEXT(".")+Name));
        else
        {
            UPackage* Package=CreatePackage(*(Base+Name));
            Asset=DuplicateObject<UPhysicsAsset>(Source,Package,*Name);
            if (Asset) { Asset->SetFlags(RF_Public|RF_Standalone); FAssetRegistryModule::AssetCreated(Asset); }
        }
        if (!Asset) return false;
        for (USkeletalBodySetup* Body:Asset->SkeletalBodySetups)
            if (Body && Body->BoneName==TEXT("head"))
            {
                if (Body->AggGeom.SphylElems.Num()!=1 || Body->AggGeom.BoxElems.Num()!=0) return false;
                const FTransform Local=FTransform(FQuat::Identity,FVector(-.2,0,168.7)).GetRelativeTransform(HeadBind);
                auto& Shape=Body->AggGeom.SphylElems[0];
                Shape.Center=Local.GetLocation(); Shape.Rotation=Local.Rotator();
                Shape.Radius=8.5f; Shape.Length=6.6f;
                Body->InvalidatePhysicsData(); Body->CreatePhysicsMeshes();
            }
        Asset->UpdateBodySetupIndexMap(); Asset->UpdateBoundsBodiesArray(); Asset->SetPreviewMesh(Mesh);
        UPackage* Package=Asset->GetOutermost(); Package->MarkPackageDirty();
        const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        if (!UPackage::SavePackage(Package,Asset,*File,Args)) return false;
        UE_LOG(LogTemp,Display,TEXT("ONE07_PHYSICS_ASSET %s bodies=%d head_refit=%d"),*Name,Asset->SkeletalBodySetups.Num(),Suffix.IsEmpty() || Suffix==TEXT("_Head"));
    }
    return true;
#else
    return false;
#endif
}
