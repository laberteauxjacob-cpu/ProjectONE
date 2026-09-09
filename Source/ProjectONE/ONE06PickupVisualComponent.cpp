#include "ONE06PickupVisualComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "ONE05Audio.h"

UONE06PickupVisualComponent::UONE06PickupVisualComponent()
{
    PrimaryComponentTick.bCanEverTick=false;
    SetMobility(EComponentMobility::Movable);
}

UStaticMeshComponent* UONE06PickupVisualComponent::Part(const TCHAR* Name,const TCHAR* Shape,
    const FVector& Position,const FVector& Scale,UMaterialInstanceDynamic* Material,float Yaw)
{
    auto* Mesh=NewObject<UStaticMeshComponent>(GetOwner(),Name);
    GetOwner()->AddInstanceComponent(Mesh); Mesh->SetupAttachment(Emblem);
    Mesh->SetMobility(EComponentMobility::Movable);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape)));
    Mesh->SetRelativeLocation(Position); Mesh->SetRelativeRotation(FRotator(0,Yaw,0)); Mesh->SetRelativeScale3D(Scale);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    Mesh->SetGenerateOverlapEvents(false); Mesh->SetCanEverAffectNavigation(false); Mesh->SetCastShadow(false);
    Mesh->SetMaterial(0,Material); Mesh->RegisterComponent(); Parts.Add(Mesh); return Mesh;
}

void UONE06PickupVisualComponent::Configure(EONEPowerUpType Type)
{
    if (bConfigured || !GetOwner() || Type==EONEPowerUpType::Count) return;
    Kind=Type;
    Color=Type==EONEPowerUpType::InstaKill ? FLinearColor(1.f,.12f,.025f) :
        Type==EONEPowerUpType::DoublePoints ? FLinearColor(1.f,.68f,.07f) : FLinearColor(.03f,.82f,.60f);
    auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Pickups/Candidate06/M_Pickup06.M_Pickup06"));
    if (!Base) { UE_LOG(LogTemp,Error,TEXT("ONE06_PICKUP_MATERIAL_MISSING: run create_candidate06_pickup_art.py in editor.")); return; }
    BodyMaterial=UMaterialInstanceDynamic::Create(Base,this);
    InkMaterial=UMaterialInstanceDynamic::Create(Base,this);
    GlowMaterial=UMaterialInstanceDynamic::Create(Base,this);
    BodyMaterial->SetVectorParameterValue(TEXT("Tint"),Color); BodyMaterial->SetScalarParameterValue(TEXT("Emission"),.65f);
    InkMaterial->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.012f,.018f,.020f)); InkMaterial->SetScalarParameterValue(TEXT("Emission"),.15f);
    GlowMaterial->SetVectorParameterValue(TEXT("Tint"),Color); GlowMaterial->SetScalarParameterValue(TEXT("Emission"),2.f);
    Emblem=NewObject<USceneComponent>(GetOwner(),TEXT("PickupEmblem")); GetOwner()->AddInstanceComponent(Emblem);
    Emblem->SetupAttachment(this); Emblem->SetMobility(EComponentMobility::Movable); Emblem->RegisterComponent();
    // The broad face points upward to remain readable from an ordinary overhead
    // camera. World rotation is a restrained oscillation, never a fast spin.
    if (Type==EONEPowerUpType::InstaKill)
    {
        Part(TEXT("SkullCranium"),TEXT("Sphere"),FVector(0,4,0),FVector(.34f,.32f,.085f),BodyMaterial);
        Part(TEXT("SkullJaw"),TEXT("Cube"),FVector(0,-11,0),FVector(.23f,.14f,.07f),BodyMaterial);
        Part(TEXT("SkullLeftEye"),TEXT("Sphere"),FVector(-8,4,4),FVector(.095f,.105f,.02f),InkMaterial);
        Part(TEXT("SkullRightEye"),TEXT("Sphere"),FVector(8,4,4),FVector(.095f,.105f,.02f),InkMaterial);
        Part(TEXT("SkullNose"),TEXT("Cube"),FVector(0,-5,4),FVector(.045f,.06f,.018f),InkMaterial,45.f);
        Part(TEXT("SkullMouth"),TEXT("Cube"),FVector(0,-12,4),FVector(.16f,.045f,.018f),InkMaterial);
        for (int32 I=0;I<3;++I)
            Part(*FString::Printf(TEXT("SkullTooth%d"),I),TEXT("Cube"),FVector(-6+I*6,-16,1),FVector(.045f,.085f,.065f),BodyMaterial);
    }
    else if (Type==EONEPowerUpType::DoublePoints)
    {
        Part(TEXT("TimesA"),TEXT("Cube"),FVector(-13,0,0),FVector(.22f,.045f,.065f),BodyMaterial,45.f);
        Part(TEXT("TimesB"),TEXT("Cube"),FVector(-13,0,0),FVector(.22f,.045f,.065f),BodyMaterial,-45.f);
        Part(TEXT("TwoTop"),TEXT("Cube"),FVector(10,13,0),FVector(.19f,.045f,.065f),BodyMaterial);
        Part(TEXT("TwoUpperRight"),TEXT("Cube"),FVector(18,7,0),FVector(.045f,.15f,.065f),BodyMaterial);
        Part(TEXT("TwoCenter"),TEXT("Cube"),FVector(10,0,0),FVector(.19f,.045f,.065f),BodyMaterial);
        Part(TEXT("TwoLowerLeft"),TEXT("Cube"),FVector(2,-7,0),FVector(.045f,.15f,.065f),BodyMaterial);
        Part(TEXT("TwoBase"),TEXT("Cube"),FVector(10,-14,0),FVector(.21f,.045f,.065f),BodyMaterial);
    }
    else
    {
        Part(TEXT("AmmoCrate"),TEXT("Cube"),FVector(0,0,0),FVector(.38f,.28f,.17f),BodyMaterial);
        Part(TEXT("CrateLid"),TEXT("Cube"),FVector(0,0,9),FVector(.41f,.31f,.035f),InkMaterial);
        Part(TEXT("CrateBandLeft"),TEXT("Cube"),FVector(-14,0,11),FVector(.035f,.32f,.022f),GlowMaterial);
        Part(TEXT("CrateBandRight"),TEXT("Cube"),FVector(14,0,11),FVector(.035f,.32f,.022f),GlowMaterial);
        for (int32 I=0;I<3;++I)
        {
            const float X=-7+I*7;
            Part(*FString::Printf(TEXT("AmmoMarkBody%d"),I),TEXT("Cube"),FVector(X,-1,11),FVector(.04f,.13f,.018f),BodyMaterial);
            Part(*FString::Printf(TEXT("AmmoMarkTip%d"),I),TEXT("Sphere"),FVector(X,6,11),FVector(.04f,.055f,.018f),BodyMaterial);
        }
    }
    Part(TEXT("CloseGlowBase"),TEXT("Cylinder"),FVector(0,0,-7),FVector(.46f,.40f,.008f),GlowMaterial);
    Glow=NewObject<UPointLightComponent>(GetOwner(),TEXT("PickupCloseGlow")); GetOwner()->AddInstanceComponent(Glow);
    Glow->SetupAttachment(Emblem); Glow->SetRelativeLocation(FVector(0,0,10)); Glow->SetMobility(EComponentMobility::Movable);
    Glow->SetLightColor(Color); Glow->SetIntensity(32.f); Glow->SetAttenuationRadius(64.f); Glow->SetCastShadows(false); Glow->RegisterComponent();
    const TCHAR* Name=Type==EONEPowerUpType::InstaKill?TEXT("S_Pickup06_InstaKill"):
        Type==EONEPowerUpType::DoublePoints?TEXT("S_Pickup06_DoublePoints"):TEXT("S_Pickup06_MaxAmmo");
    CollectionSound=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/ONE/Audio/Candidate06/%s.%s"),Name,Name));
    if (!CollectionSound) UE_LOG(LogTemp,Error,TEXT("ONE06_PICKUP_AUDIO_MISSING: %s"),Name);
    bConfigured=true; SetRemainingLifetime(30.f,30.f);
}

void UONE06PickupVisualComponent::SetRemainingLifetime(float Remaining,float Total)
{
    if (!bConfigured || bCollected || !Emblem) return;
    const float Age=FMath::Max(0.f,Total-Remaining),Warning=FMath::Clamp((5.f-Remaining)/5.f,0.f,1.f);
    VisualOpacity=FMath::Clamp(Remaining/.85f,0.f,1.f);
    const float Pulse=1.f-.22f*Warning*(.5f+.5f*FMath::Sin(Age*2.f*PI*1.3f));
    Emblem->SetRelativeLocation(FVector(0,0,28.f+2.5f*FMath::Sin(Age*2.f*PI/2.8f)));
    Emblem->SetRelativeRotation(FRotator(0,12.f*FMath::Sin(Age*.45f),0));
    Emblem->SetRelativeScale3D(FVector(1.f-.08f*Warning));
    BodyMaterial->SetScalarParameterValue(TEXT("Opacity"),VisualOpacity);
    InkMaterial->SetScalarParameterValue(TEXT("Opacity"),VisualOpacity);
    GlowMaterial->SetScalarParameterValue(TEXT("Opacity"),VisualOpacity*.32f);
    GlowMaterial->SetScalarParameterValue(TEXT("Emission"),2.f*Pulse);
    Glow->SetIntensity(32.f*Pulse*VisualOpacity);
}

void UONE06PickupVisualComponent::PlayCollectionCue()
{
    if (!bConfigured || bCollected) return;
    bCollected=true; ++CollectionCueCount;
    for (UStaticMeshComponent* Mesh:Parts) if (Mesh) Mesh->SetVisibility(false);
    if (Glow) Glow->SetIntensity(0.f);
    if (!CollectionSound) return;
    CueVoice=NewObject<UAudioComponent>(GetOwner(),TEXT("PickupCollectionCue")); GetOwner()->AddInstanceComponent(CueVoice);
    CueVoice->SetupAttachment(this); CueVoice->bAutoActivate=false; CueVoice->bAutoDestroy=false; CueVoice->bStopWhenOwnerDestroyed=true;
    CueVoice->bOverrideAttenuation=true; CueVoice->AttenuationOverrides.bAttenuate=true; CueVoice->AttenuationOverrides.bSpatialize=true;
    CueVoice->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
    CueVoice->AttenuationOverrides.AttenuationShapeExtents=FVector(150.f); CueVoice->AttenuationOverrides.FalloffDistance=650.f;
    if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>()) CueVoice->ConcurrencySet.Add(Audio->Group(EONE05VoiceGroup::Action));
    CueVoice->SetSound(CollectionSound); CueVoice->SetVolumeMultiplier(.7f); CueVoice->RegisterComponent(); CueVoice->Play();
}
bool UONE06PickupVisualComponent::IsConfigured() const
{
    if (!bConfigured || !BodyMaterial || !InkMaterial || !GlowMaterial || !CollectionSound || Parts.IsEmpty() || Parts.Num()>12) return false;
    for (const UStaticMeshComponent* Mesh:Parts) if (!Mesh || !Mesh->GetStaticMesh()) return false;
    return true;
}
int32 UONE06PickupVisualComponent::GetVisiblePartCount() const
{ int32 Count=0; for (const UStaticMeshComponent* Mesh:Parts) if (Mesh && Mesh->IsVisible() && Mesh->GetStaticMesh()) ++Count; return Count; }
float UONE06PickupVisualComponent::GetGlowIntensity() const { return Glow?Glow->Intensity:0.f; }
void UONE06PickupVisualComponent::Shutdown()
{ if (CueVoice) CueVoice->Stop(); if (Glow) Glow->SetIntensity(0.f); for (UStaticMeshComponent* Mesh:Parts) if (Mesh) Mesh->SetVisibility(false); }
void UONE06PickupVisualComponent::EndPlay(const EEndPlayReason::Type Reason)
{ Shutdown(); Super::EndPlay(Reason); }
