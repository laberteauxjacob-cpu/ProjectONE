#include "ONE03PhysicsAssets.h"
#if WITH_EDITOR
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "AI/Navigation/NavCollisionBase.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "EngineUtils.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

namespace
{
    struct FONE03BodySpec
    {
        FName Bone;
        FVector Center=FVector::ZeroVector, Dimensions=FVector::ZeroVector;
        FQuat Rotation=FQuat::Identity;
        float Radius=0,Length=0,Mass=1;
        bool bBox=false;
    };
    bool SaveONE03Asset(UObject* Asset)
    {
        UPackage* Package=Asset->GetOutermost();
        const FString File=FPackageName::LongPackageNameToFilename(Package->GetName(),FPackageName::GetAssetPackageExtension());
        IFileManager::Get().MakeDirectory(*FPaths::GetPath(File),true);
        Package->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags=RF_Public|RF_Standalone; Args.SaveFlags=SAVE_NoError;
        return UPackage::SavePackage(Package,Asset,*File,Args);
    }
    TSharedPtr<FJsonValue> VectorONE03(const FVector& V)
    {
        TArray<TSharedPtr<FJsonValue>> Values;
        for (double N:{V.X,V.Y,V.Z}) Values.Add(MakeShared<FJsonValueNumber>(N));
        return MakeShared<FJsonValueArray>(Values);
    }
}
#endif

bool UONE03PhysicsAssets::BuildInfectedAssets()
{
#if WITH_EDITOR
    // Derive from the complete accepted skeleton, never from the core's missing
    // limb vertices. Body shapes are explicit and editable in this source.
    USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Infected.SK_Infected"));
    if (!Mesh) return false;
    const FReferenceSkeleton& Ref=Mesh->GetRefSkeleton();
    TArray<FTransform> RefCS;
    for (int32 I=0;I<Ref.GetNum();++I)
    {
        const int32 Parent=Ref.GetParentIndex(I);
        RefCS.Add(Parent==INDEX_NONE ? Ref.GetRefBonePose()[I] : Ref.GetRefBonePose()[I]*RefCS[Parent]);
    }
    auto BonePose=[&](FName Name)->FTransform
    {
        const int32 I=Ref.FindBoneIndex(Name);
        checkf(I!=INDEX_NONE,TEXT("Required infected physics bone missing: %s"),*Name.ToString());
        return RefCS[I];
    };
    TArray<FONE03BodySpec> Specs;
    auto Box=[&](FName Bone,FVector Center,FVector Dimensions,float Mass)
    {
        FONE03BodySpec S; S.Bone=Bone; S.Center=Center; S.Dimensions=Dimensions; S.Mass=Mass; S.bBox=true; Specs.Add(S);
    };
    auto Capsule=[&](FName Bone,FVector A,FVector B,float Radius,float Mass)
    {
        FONE03BodySpec S; S.Bone=Bone; S.Center=(A+B)*.5f; S.Rotation=FQuat::FindBetweenNormals(FVector::UpVector,(B-A).GetSafeNormal());
        S.Radius=Radius; S.Length=FMath::Max(.1f,float(FVector::Distance(A,B))-2*Radius); S.Mass=Mass; Specs.Add(S);
    };
    const FVector Pelvis=BonePose(TEXT("pelvis")).GetLocation();
    Box(TEXT("pelvis"),Pelvis+FVector(0,0,5),FVector(22,27,17),12);
    Box(TEXT("spine_01"),BonePose(TEXT("spine_01")).GetLocation()+FVector(0,0,8),FVector(22,29,19),13);
    Box(TEXT("spine_02"),BonePose(TEXT("spine_02")).GetLocation()+FVector(0,0,9),FVector(24,33,19),12);
    const FVector Head=BonePose(TEXT("head")).GetLocation();
    Capsule(TEXT("head"),Head+FVector(0,0,1),Head+FVector(0,0,20),9.5f,5);
    for (const TCHAR* Side:{TEXT("l"),TEXT("r")})
    {
        const FName Upper(*FString::Printf(TEXT("upperarm_%s"),Side)),Lower(*FString::Printf(TEXT("lowerarm_%s"),Side)),Hand(*FString::Printf(TEXT("hand_%s"),Side));
        const FVector Shoulder=BonePose(Upper).GetLocation(),Elbow=BonePose(Lower).GetLocation(),Wrist=BonePose(Hand).GetLocation();
        const FVector UpperDirection=(Elbow-Shoulder).GetSafeNormal(),LowerDirection=(Wrist-Elbow).GetSafeNormal();
        Capsule(Upper,Shoulder+UpperDirection*4.8f,Elbow-UpperDirection*1.5f,5.2f,2.4f);
        Capsule(Lower,Elbow+LowerDirection*1.5f,Wrist-LowerDirection,4.2f,1.5f);
        Capsule(Hand,Wrist,Wrist+LowerDirection*10.f,3.6f,.7f);
        const FName Thigh(*FString::Printf(TEXT("thigh_%s"),Side)),Calf(*FString::Printf(TEXT("calf_%s"),Side)),Foot(*FString::Printf(TEXT("foot_%s"),Side));
        const FVector Hip=BonePose(Thigh).GetLocation(),Knee=BonePose(Calf).GetLocation(),Ankle=BonePose(Foot).GetLocation();
        const FVector ThighDirection=(Knee-Hip).GetSafeNormal(),CalfDirection=(Ankle-Knee).GetSafeNormal();
        // Intact thighs cover the proximal region and follow their live joint.
        // The detached left part is trimmed below when building its asset.
        Capsule(Thigh,Hip+ThighDirection*3.f,Knee-ThighDirection*1.5f,6.8f,7.f);
        Capsule(Calf,Knee+CalfDirection*1.5f,Ankle-CalfDirection*1.5f,4.9f,4.f);
        Box(Foot,Ankle+FVector(7,0,-4),FVector(26,11,12),1.f);
    }
    const FString Base=TEXT("/Game/ONE/Characters/Candidate03/");
    const FString MaterialName=TEXT("PM_Infected_C03");
    UPackage* MaterialPackage=CreatePackage(*(Base+MaterialName));
    UPhysicalMaterial* Material=FindObject<UPhysicalMaterial>(MaterialPackage,*MaterialName);
    if (!Material) { Material=NewObject<UPhysicalMaterial>(MaterialPackage,*MaterialName,RF_Public|RF_Standalone); FAssetRegistryModule::AssetCreated(Material); }
    Material->Friction=.65f; Material->Restitution=.05f;
    if (!SaveONE03Asset(Material)) return false;
    TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("source"),TEXT("Source/ProjectONE/ONE03PhysicsAssets.cpp"));
    Report->SetStringField(TEXT("reference_mesh"),TEXT("/Game/ONE/Characters/SK_Infected"));
    Report->SetStringField(TEXT("anatomy"),TEXT("Source _r is anatomical LEFT; _l is RIGHT after accepted import reflection."));
    TArray<TSharedPtr<FJsonValue>> Assets;
    const TArray<FString> Names={TEXT("PA_Infected_C03"),TEXT("PA_Infected_Head_C03"),TEXT("PA_Infected_ArmLeft_C03"),TEXT("PA_Infected_ArmRight_C03"),TEXT("PA_Infected_LegLeft_C03")};
    for (int32 Part=0;Part<Names.Num();++Part)
    {
        UPackage* Package=CreatePackage(*(Base+Names[Part]));
        UPhysicsAsset* PA=FindObject<UPhysicsAsset>(Package,*Names[Part]);
        if (!PA) { PA=NewObject<UPhysicsAsset>(Package,*Names[Part],RF_Public|RF_Standalone); FAssetRegistryModule::AssetCreated(PA); }
        PA->SkeletalBodySetups.Reset(); PA->ConstraintSetup.Reset(); PA->CollisionDisableTable.Reset();
        TMap<FName,int32> BodyIndices;
        TArray<TSharedPtr<FJsonValue>> Bodies,Constraints;
        auto Includes=[&](FName Bone)
        {
            if (Part==0) return true;
            if (Part==1) return Bone==TEXT("head");
            const FName Root=Part==2 ? TEXT("upperarm_r") : Part==3 ? TEXT("upperarm_l") : TEXT("thigh_r");
            int32 I=Ref.FindBoneIndex(Bone);
            while (I!=INDEX_NONE) { if (Ref.GetBoneName(I)==Root) return true; I=Ref.GetParentIndex(I); }
            return false;
        };
        for (FONE03BodySpec S:Specs)
        {
            if (!Includes(S.Bone)) continue;
            if (Part==4 && S.Bone==TEXT("thigh_r"))
            {
                const FVector Hip=BonePose(S.Bone).GetLocation(),Knee=BonePose(TEXT("calf_r")).GetLocation();
                const FVector Direction=(Knee-Hip).GetSafeNormal();
                const FVector A=FMath::Lerp(Hip,Knee,.38f),B=Knee-Direction*1.5f;
                S.Center=(A+B)*.5f; S.Length=FMath::Max(.1f,float(FVector::Distance(A,B))-2*S.Radius);
            }
            USkeletalBodySetup* Body=NewObject<USkeletalBodySetup>(PA,NAME_None,RF_Transactional);
            Body->BoneName=S.Bone; Body->PhysicsType=PhysType_Default; Body->CollisionTraceFlag=CTF_UseSimpleAsComplex;
            const FTransform Local=FTransform(S.Rotation,S.Center).GetRelativeTransform(BonePose(S.Bone));
            if (S.bBox)
            { FKBoxElem Shape; Shape.Center=Local.GetLocation(); Shape.Rotation=Local.Rotator(); Shape.X=S.Dimensions.X; Shape.Y=S.Dimensions.Y; Shape.Z=S.Dimensions.Z; Body->AggGeom.BoxElems.Add(Shape); }
            else
            { FKSphylElem Shape; Shape.Center=Local.GetLocation(); Shape.Rotation=Local.Rotator(); Shape.Radius=S.Radius; Shape.Length=S.Length; Body->AggGeom.SphylElems.Add(Shape); }
            if (S.Bone==TEXT("pelvis"))
            {
                // Keep simple proximal-left-thigh coverage with the surviving
                // core. It must not disappear with TermBodiesBelow(thigh_r).
                const FVector Hip=BonePose(TEXT("thigh_r")).GetLocation();
                const FVector Cut=FMath::Lerp(Hip,BonePose(TEXT("calf_r")).GetLocation(),.38f);
                const FTransform Stump=FTransform(FQuat::FindBetweenNormals(FVector::UpVector,(Cut-Hip).GetSafeNormal()),(Hip+Cut)*.5f).GetRelativeTransform(BonePose(S.Bone));
                FKSphylElem Shape; Shape.Center=Stump.GetLocation(); Shape.Rotation=Stump.Rotator();
                Shape.Radius=6.2f; Shape.Length=FMath::Max(.1f,float(FVector::Distance(Hip,Cut))-12.4f); Body->AggGeom.SphylElems.Add(Shape);
            }
            FBodyInstance& BI=Body->DefaultInstance;
            BI.SetMassOverride(S.Mass,true); BI.LinearDamping=.9f; BI.AngularDamping=3.f;
            BI.SetMaxDepenetrationVelocity(140.f); BI.SetPositionSolverIterationCount(8); BI.SetVelocitySolverIterationCount(2);
            BI.SetMaxAngularVelocityInRadians(12.f,false);
            BI.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); BI.SetObjectType(ECC_PhysicsBody);
            BI.SetResponseToAllChannels(ECR_Block); BI.SetResponseToChannel(ECC_Pawn,ECR_Ignore); BI.SetResponseToChannel(ECC_Visibility,ECR_Ignore); BI.SetResponseToChannel(ECC_Camera,ECR_Ignore);
            BI.SetPhysMaterialOverride(Material); BI.SleepFamily=ESleepFamily::Normal;
            BodyIndices.Add(S.Bone,PA->SkeletalBodySetups.Add(Body));
            TSharedPtr<FJsonObject> J=MakeShared<FJsonObject>(); J->SetStringField(TEXT("bone"),S.Bone.ToString()); J->SetNumberField(TEXT("mass_kg"),S.Mass);
            J->SetStringField(TEXT("shape"),S.bBox ? TEXT("box") : TEXT("capsule")); J->SetField(TEXT("component_center_cm"),VectorONE03(S.Center));
            J->SetField(TEXT("local_center_cm"),VectorONE03(Local.GetLocation())); J->SetField(TEXT("local_rotation_degrees"),VectorONE03(Local.Rotator().Euler()));
            J->SetField(TEXT("box_dimensions_cm"),VectorONE03(S.Dimensions)); J->SetNumberField(TEXT("radius_cm"),S.Radius); J->SetNumberField(TEXT("cylinder_length_cm"),S.Length);
            J->SetBoolField(TEXT("retains_proximal_left_thigh_capsule"),S.Bone==TEXT("pelvis"));
            if (S.Bone==TEXT("pelvis"))
            {
                const FKSphylElem& Shape=Body->AggGeom.SphylElems[0];
                TSharedPtr<FJsonObject> Extra=MakeShared<FJsonObject>();
                Extra->SetField(TEXT("bind_local_center_cm"),VectorONE03(Shape.Center));
                Extra->SetField(TEXT("bind_local_rotation_degrees"),VectorONE03(Shape.Rotation.Euler()));
                Extra->SetNumberField(TEXT("radius_cm"),Shape.Radius); Extra->SetNumberField(TEXT("cylinder_length_cm"),Shape.Length);
                Extra->SetStringField(TEXT("runtime"),TEXT("Disabled while left thigh is intact; enabled and fitted to captured thigh-to-pelvis pose on sever."));
                J->SetObjectField(TEXT("proximal_left_thigh_capsule"),Extra);
            }
            Bodies.Add(MakeShared<FJsonValueObject>(J));
        }
        PA->UpdateBodySetupIndexMap();
        for (USkeletalBodySetup* Body:PA->SkeletalBodySetups)
        {
            int32 Parent=Ref.GetParentIndex(Ref.FindBoneIndex(Body->BoneName));
            while (Parent!=INDEX_NONE && !BodyIndices.Contains(Ref.GetBoneName(Parent))) Parent=Ref.GetParentIndex(Parent);
            if (Parent==INDEX_NONE) continue;
            const FName ParentName=Ref.GetBoneName(Parent);
            UPhysicsConstraintTemplate* Constraint=NewObject<UPhysicsConstraintTemplate>(PA,NAME_None,RF_Transactional);
            FConstraintInstance& C=Constraint->DefaultInstance;
            C.JointName=Body->BoneName; C.ConstraintBone1=Body->BoneName; C.ConstraintBone2=ParentName;
            // Component-space joint frames map through the actual bone bases.
            // Hinge centers are biased within their allowed cone, retaining
            // the rest pose while limiting reverse knee/elbow bending.
            const FTransform Joint(FQuat::Identity,BonePose(Body->BoneName).GetLocation());
            const FString Bone=Body->BoneName.ToString();
            const bool Hinge=Bone.StartsWith(TEXT("lowerarm")) || Bone.StartsWith(TEXT("calf"));
            const bool Limb=Bone.StartsWith(TEXT("upperarm")) || Bone.StartsWith(TEXT("thigh"));
            const float PitchBias=Bone.StartsWith(TEXT("calf")) ? -50.f : Bone.StartsWith(TEXT("lowerarm")) ? 50.f : 0.f;
            const FTransform ParentJoint(FRotator(PitchBias,0,0),Joint.GetLocation());
            C.SetRefFrame(EConstraintFrame::Frame1,Joint.GetRelativeTransform(BonePose(Body->BoneName)));
            C.SetRefFrame(EConstraintFrame::Frame2,ParentJoint.GetRelativeTransform(BonePose(ParentName)));
            C.SetLinearXLimit(LCM_Locked,0); C.SetLinearYLimit(LCM_Locked,0); C.SetLinearZLimit(LCM_Locked,0);
            const float Swing1=Hinge?12.f:Limb?55.f:22.f,Swing2=Hinge?55.f:Limb?40.f:20.f,Twist=Hinge?8.f:Limb?35.f:18.f;
            C.SetAngularSwing1Limit(ACM_Limited,Swing1); C.SetAngularSwing2Limit(ACM_Limited,Swing2); C.SetAngularTwistLimit(ACM_Limited,Twist);
            C.SetDisableCollision(true); C.SetProjectionParams(true,.15f,.05f,5.f,30.f);
            PA->ConstraintSetup.Add(Constraint);
            PA->DisableCollision(BodyIndices[Body->BoneName],BodyIndices[ParentName]);
            TSharedPtr<FJsonObject> J=MakeShared<FJsonObject>(); J->SetStringField(TEXT("child"),Bone); J->SetStringField(TEXT("parent"),ParentName.ToString());
            J->SetField(TEXT("anchor_component_cm"),VectorONE03(Joint.GetLocation())); J->SetNumberField(TEXT("hinge_center_pitch_degrees"),PitchBias); J->SetField(TEXT("swing1_swing2_twist_degrees"),VectorONE03(FVector(Swing1,Swing2,Twist))); Constraints.Add(MakeShared<FJsonValueObject>(J));
        }
        PA->UpdateBoundsBodiesArray(); PA->SetPreviewMesh(Mesh); PA->PostEditChange();
        if (!SaveONE03Asset(PA)) return false;
        TSharedPtr<FJsonObject> A=MakeShared<FJsonObject>(); A->SetStringField(TEXT("asset"),Base+Names[Part]); A->SetArrayField(TEXT("bodies"),Bodies); A->SetArrayField(TEXT("constraints"),Constraints);
        Assets.Add(MakeShared<FJsonValueObject>(A));
        UE_LOG(LogTemp,Display,TEXT("ONE03_PHYSICS_ASSET %s bodies=%d constraints=%d"),*Names[Part],Bodies.Num(),Constraints.Num());
    }
    Report->SetArrayField(TEXT("assets"),Assets); Report->SetStringField(TEXT("status"),TEXT("GENERATED; runtime fitting and settling require separate gameplay validation"));
    FString JSON; const auto Writer=TJsonWriterFactory<>::Create(&JSON); FJsonSerializer::Serialize(Report.ToSharedRef(),Writer);
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03"); IFileManager::Get().MakeDirectory(*Folder,true);
    return FFileHelper::SaveStringToFile(JSON,*(Folder/TEXT("PhysicsAssetManifest.json")));
#else
    return false;
#endif
}

FString UONE03PhysicsAssets::InspectEnvironmentCollision()
{
#if WITH_EDITOR
    TArray<TSharedPtr<FJsonValue>> Rows;
    for (const TCHAR* Name:{TEXT("SM_FloorModule"),TEXT("SM_WallBay"),TEXT("SM_CutawayBarrier"),TEXT("SM_PressureDoor"),TEXT("SM_PowerRack"),TEXT("SM_PressureVessel"),TEXT("SM_LabConsole"),TEXT("SM_ResearchBench")})
    {
        const FString Path=FString::Printf(TEXT("/Game/ONE/Art/Environment/%s.%s"),Name,Name);
        const UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if (!Mesh || !Mesh->GetBodySetup()) return TEXT("");
        const FBox Render=Mesh->GetBoundingBox();
        TSharedPtr<FJsonObject> J=MakeShared<FJsonObject>();
        J->SetStringField(TEXT("mesh"),Name); J->SetField(TEXT("render_min_cm"),VectorONE03(Render.Min)); J->SetField(TEXT("render_max_cm"),VectorONE03(Render.Max));
        J->SetNumberField(TEXT("simple_shape_count"),Mesh->GetBodySetup()->AggGeom.GetElementCount());
        TArray<TSharedPtr<FJsonValue>> Hulls;
        for (const FKConvexElem& Hull:Mesh->GetBodySetup()->AggGeom.ConvexElems)
        {
            FBox Bounds(ForceInit); int32 Outside=0;
            for (const FVector& V:Hull.VertexData)
            {
                const FVector P=Hull.GetTransform().TransformPosition(V); Bounds+=P;
                if (!Render.ExpandBy(2.f).IsInsideOrOn(P)) ++Outside;
            }
            TSharedPtr<FJsonObject> H=MakeShared<FJsonObject>();
            H->SetField(TEXT("min_cm"),VectorONE03(Bounds.Min)); H->SetField(TEXT("max_cm"),VectorONE03(Bounds.Max));
            H->SetNumberField(TEXT("vertices"),Hull.VertexData.Num()); H->SetNumberField(TEXT("outside_render_bounds_2cm"),Outside); Hulls.Add(MakeShared<FJsonValueObject>(H));
        }
        J->SetArrayField(TEXT("convex_hulls"),Hulls);
        if (const UNavCollisionBase* Nav=Mesh->GetNavCollision())
        {
            auto Geometry=[&](const FNavCollisionConvex& Geo)
            {
                TSharedPtr<FJsonObject> G=MakeShared<FJsonObject>(); FBox Bounds(ForceInit); int32 Outside=0;
                for (const FVector& V:Geo.VertexBuffer) { Bounds+=V; if (!Render.ExpandBy(2.f).IsInsideOrOn(V)) ++Outside; }
                G->SetNumberField(TEXT("vertices"),Geo.VertexBuffer.Num()); G->SetNumberField(TEXT("outside_render_bounds_2cm"),Outside);
                if (Geo.VertexBuffer.Num()) { G->SetField(TEXT("min_cm"),VectorONE03(Bounds.Min)); G->SetField(TEXT("max_cm"),VectorONE03(Bounds.Max)); }
                return G;
            };
            J->SetObjectField(TEXT("nav_convex"),Geometry(Nav->GetConvexCollision()));
            J->SetObjectField(TEXT("nav_triangles"),Geometry(Nav->GetTriMeshCollision()));
        }
        Rows.Add(MakeShared<FJsonValueObject>(J));
    }
    TSharedPtr<FJsonObject> Report=MakeShared<FJsonObject>(); Report->SetArrayField(TEXT("assets"),Rows);
    FString JSON; const auto Writer=TJsonWriterFactory<>::Create(&JSON); FJsonSerializer::Serialize(Report.ToSharedRef(),Writer); return JSON;
#else
    return TEXT("");
#endif
}

bool UONE03PhysicsAssets::RebuildNavigationAndWait(UWorld* World)
{
#if WITH_EDITOR
    auto* Nav=World?FNavigationSystem::GetCurrent<UNavigationSystemV1>(World):nullptr;
    if (!Nav) return false;
    Nav->Build();
    int32 DataCount=0;
    for (TActorIterator<ANavigationData> It(World);It;++It) { It->EnsureBuildCompletion(); ++DataCount; }
    return DataCount>0 && !Nav->IsNavigationBuildInProgress();
#else
    return false;
#endif
}
