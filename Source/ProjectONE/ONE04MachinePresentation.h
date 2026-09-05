#pragma once
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "ONE04MachinePresentation.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UAudioComponent;
class USoundBase;
class UMaterialInstanceDynamic;
struct FONEWeaponDefinition;

UENUM()
enum class EONE04MachineVisualState : uint8 { Idle, Active, Ready, Closing, Disabled };

/** Original machine geometry, evaluated preview transfers, lights and audio only.
 *  The owning progression actor alone owns payment, inventory, rewards and state.
 *  Local +X faces the operator. Weapon assemblies retain their actual scale.
 */
UCLASS(ClassGroup=(ProjectONE))
class PROJECTONE_API UONE04MachinePresentation : public USceneComponent
{
    GENERATED_BODY()
public:
    UONE04MachinePresentation();
    void Configure(bool Box);
    void UpdateVisual(EONE04MachineVisualState State, float Elapsed, float Duration);
    void SetPreview(const FONEWeaponDefinition* Definition);
    void PlayCycleCue();
    void BeginTransferFrom(const FTransform& ActualGunWorld);
    void BeginRetrievalTo(const FTransform& HandWorld);
    FTransform GetIntakeWorldTransform() const;
    FTransform GetOutputWorldTransform() const;
    FTransform GetPreviewWorldTransform() const;
    void Shutdown();
    bool IsConfigured() const;
    bool HasCompletePreview() const;
    int32 GetVisiblePreviewPartCount() const;
    int32 GetActiveLoopCount() const;
    float GetLidOpenDegrees() const { return LidOpenDegrees; }
    float GetMachineLightIntensity() const;
    float GetTransferStartErrorCm() const { return TransferStartErrorCm; }
    float GetRetrievalStartErrorCm() const { return RetrievalStartErrorCm; }
    int32 GetCycleCueCount() const { return CycleCueCount; }
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
private:
    UStaticMeshComponent* MakePart(const TCHAR* Name, const TCHAR* AssetName, const FVector& Location=FVector::ZeroVector);
    UStaticMeshComponent* MakePreviewPart(const TCHAR* Name);
    UAudioComponent* MakeAudio(const TCHAR* Name);
    void PlayCue(const TCHAR* Name, float Gain=1.f);
    USoundBase* Sound(const FString& Name);
    void UpdateEmitters(const FLinearColor& Color, float Gain, float Intensity);
    void UpdateStrut(int32 Index, const FVector& Bottom, const FVector& Top);
    FTransform Mount(const FVector& Location) const;
    void SetPreviewWorld(const FTransform& Transform);
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> EmitterMaterials;
    UPROPERTY() TMap<FString,TObjectPtr<USoundBase>> Sounds;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Lid;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Rotor;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Locks;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Sleeves;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rods;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Cradle;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Clamps;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Rings;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Shields;
    UPROPERTY() TObjectPtr<USceneComponent> PreviewRoot;
    UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> PreviewParts;
    UPROPERTY() TObjectPtr<UPointLightComponent> MachineLight;
    UPROPERTY() TObjectPtr<UPointLightComponent> PreviewLight;
    UPROPERTY() TObjectPtr<UAudioComponent> IdleLoop;
    UPROPERTY() TObjectPtr<UAudioComponent> ProcessLoop;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> CueVoices;
    EONE04MachineVisualState PreviousState=EONE04MachineVisualState::Disabled;
    bool bConfigured=false, bBox=true, bPreviewValid=false, bTransferring=false, bRetrieving=false;
    bool bClampCue=false, bOutputCue=false;
    int32 ExpectedPreviewParts=0, CycleCueCount=0, NextCueVoice=0;
    float PreviewCenterX=0.f, LidOpenDegrees=0.f;
    float TransferStartErrorCm=0.f, RetrievalStartErrorCm=0.f;
    double TransferStartTime=0, RetrievalStartTime=0;
    FTransform TransferStart=FTransform::Identity, RetrievalStart=FTransform::Identity, RetrievalTarget=FTransform::Identity;
    FLinearColor PreviewAura=FLinearColor::Transparent;
};
