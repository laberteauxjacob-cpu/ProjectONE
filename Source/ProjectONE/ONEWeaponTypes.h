#pragma once
#include "CoreMinimal.h"
#include "ONEWeaponTypes.generated.h"
class UAnimSequence;
class USoundBase;
class UStaticMesh;

UENUM(BlueprintType)
enum class EONEWeaponOperation : uint8 { Ready, Equip, Fire, Pump, MagazineReload, ShellStart, ShellInsert, ShellEnd };
UENUM(BlueprintType)
enum class EONEWeaponEvent : uint8 { Sound, MagazineOut, MagazineCommit, ShellCommit, ShellEject, WeaponSwap, PumpLock };
UENUM(BlueprintType)
enum class EONEHitRegion : uint8 { Body, Head, Arm, Invalid };
USTRUCT(BlueprintType)
struct FONEWeaponTimedEvent
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) float Time=0.f;
    UPROPERTY(EditAnywhere) EONEWeaponEvent Event=EONEWeaponEvent::Sound;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<USoundBase> Sound;
};
USTRUCT(BlueprintType)
struct FONEWeaponOperationDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) EONEWeaponOperation Operation=EONEWeaponOperation::Ready;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0.01")) float Duration=.2f;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> Animation;
    UPROPERTY(EditAnywhere) TArray<FONEWeaponTimedEvent> Events;
};
USTRUCT(BlueprintType)
struct FONEWeaponDefinition
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere) FName Id=TEXT("AR01");
    UPROPERTY(EditAnywhere) FText DisplayName;
    UPROPERTY(EditAnywhere) bool bAutomatic=true;
    UPROPERTY(EditAnywhere) bool bShellReload=false;
    UPROPERTY(EditAnywhere) bool bPumpAction=false;
    UPROPERTY(EditAnywhere,meta=(ClampMin="1")) int32 Capacity=24;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0")) int32 InitialReserve=192;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0")) int32 ReserveLimit=270;
    UPROPERTY(EditAnywhere,meta=(ClampMin="1",ClampMax="16")) int32 Pellets=1;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0")) float Damage=32.f;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0.04")) float FireInterval=.16f;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="15")) float SpreadDegrees=.35f;
    UPROPERTY(EditAnywhere) float Range=2800.f;
    UPROPERTY(EditAnywhere) float FalloffStart=1400.f;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="1")) float MinimumDamageFraction=.65f;
    UPROPERTY(EditAnywhere) float HeadTraumaScale=2.f;
    UPROPERTY(EditAnywhere) float ArmTraumaScale=1.f;
    UPROPERTY(EditAnywhere) float HeavyStaggerThreshold=10000.f;
    UPROPERTY(EditAnywhere) FVector Muzzle=FVector(58,0,14);
    UPROPERTY(EditAnywhere) FVector EjectionPoint=FVector(5,-4.5,13.5);
    UPROPERTY(EditAnywhere) float PumpTravel=9.f;
    UPROPERTY(EditAnywhere) float PumpRearTime=.21f;
    UPROPERTY(EditAnywhere) float PumpForwardTime=.44f;
    UPROPERTY(EditAnywhere) float FlashDuration=.045f;
    UPROPERTY(EditAnywhere) float FlashIntensity=18000.f;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> ForeEndMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> ShellMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> MagazineMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> ReadyAnimation;
    UPROPERTY(EditAnywhere) TArray<TSoftObjectPtr<USoundBase>> ShotSounds;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<USoundBase> EmptySound;
    UPROPERTY(EditAnywhere) TArray<TSoftObjectPtr<USoundBase>> FleshSounds;
    UPROPERTY(EditAnywhere) TArray<TSoftObjectPtr<USoundBase>> ConcreteSounds;
    UPROPERTY(EditAnywhere) TArray<TSoftObjectPtr<USoundBase>> MetalSounds;
    UPROPERTY(EditAnywhere) TArray<FONEWeaponOperationDefinition> Operations;
};
USTRUCT(BlueprintType)
struct FONECarriedWeaponState
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere) int32 Ammo=0;
    UPROPERTY(VisibleAnywhere) int32 Reserve=0;
    UPROPERTY(VisibleAnywhere) bool bNeedsPump=false;
    UPROPERTY(VisibleAnywhere) bool bCaseEjected=false;
};
/** One transaction per victim per actual discharge, after all pellet traces. */
struct FONEWeaponDamagePacket
{
    uint64 ShotId=0;
    float BodyDamage=0,HeadDamage=0,ArmDamage=0,HeadTrauma=0,ArmTrauma=0;
    float HeavyStaggerThreshold=10000.f;
    FVector Position=FVector::ZeroVector,Direction=FVector::ForwardVector;
    int32 Pellets=0;
};
