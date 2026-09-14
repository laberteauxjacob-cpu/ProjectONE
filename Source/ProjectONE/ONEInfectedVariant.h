#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ONEInfectedVariant.generated.h"
class USkeletalMesh;
class UPhysicsAsset;
class UAnimSequence;

/** Appearance and compatible rig assets only; all variants use AONEZombie's
 * shared health, movement, attack, damage and reward rules. */
UCLASS(BlueprintType)
class PROJECTONE_API UONEInfectedVariant : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Identity") FName VariantId;
    UPROPERTY(EditAnywhere, Category="Identity") FText DisplayName;
    UPROPERTY(EditAnywhere, Category="Meshes") TSoftObjectPtr<USkeletalMesh> Core;
    UPROPERTY(EditAnywhere, Category="Meshes") TSoftObjectPtr<USkeletalMesh> Head;
    UPROPERTY(EditAnywhere, Category="Meshes") TSoftObjectPtr<USkeletalMesh> ArmLeft;
    UPROPERTY(EditAnywhere, Category="Meshes") TSoftObjectPtr<USkeletalMesh> ArmRight;
    UPROPERTY(EditAnywhere, Category="Meshes") TSoftObjectPtr<USkeletalMesh> LegLeft;
    UPROPERTY(EditAnywhere, Category="Physics") TSoftObjectPtr<UPhysicsAsset> BodyPhysics;
    UPROPERTY(EditAnywhere, Category="Physics") TSoftObjectPtr<UPhysicsAsset> HeadPhysics;
    UPROPERTY(EditAnywhere, Category="Physics") TSoftObjectPtr<UPhysicsAsset> ArmLeftPhysics;
    UPROPERTY(EditAnywhere, Category="Physics") TSoftObjectPtr<UPhysicsAsset> ArmRightPhysics;
    UPROPERTY(EditAnywhere, Category="Physics") TSoftObjectPtr<UPhysicsAsset> LegLeftPhysics;
    UPROPERTY(EditAnywhere, Category="Motion") TMap<FName,TSoftObjectPtr<UAnimSequence>> Clips;
    // Stable presentation salt; never a weapon, reward or gameplay random seed.
    UPROPERTY(EditAnywhere, Category="Audio") int32 VoiceVariation=0;
};
