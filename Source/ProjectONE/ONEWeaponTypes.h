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
enum class EONEHitRegion : uint8 { Body, Head, ArmLeft, ArmRight, LegLeft, LegRight, Invalid };
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
    UPROPERTY(EditAnywhere,meta=(ClampMin="0")) int32 RoundReserveReward=48;
    UPROPERTY(EditAnywhere,meta=(ClampMin="1",ClampMax="16")) int32 Pellets=1;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0")) float Damage=32.f;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0.04")) float FireInterval=.16f;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="15")) float SpreadDegrees=.35f;
    UPROPERTY(EditAnywhere) float Range=2800.f;
    UPROPERTY(EditAnywhere) float FalloffStart=1400.f;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="1")) float MinimumDamageFraction=.65f;
    UPROPERTY(EditAnywhere) float HeadTraumaScale=2.f;
    UPROPERTY(EditAnywhere) float ArmTraumaScale=1.f;
    UPROPERTY(EditAnywhere) float LegTraumaScale=1.f;
    UPROPERTY(EditAnywhere) float HeavyStaggerThreshold=10000.f;
    UPROPERTY(EditAnywhere) FVector Muzzle=FVector(59.4,0,14);
    UPROPERTY(EditAnywhere) FVector EjectionPoint=FVector(5,-4.5,13.5);
    UPROPERTY(EditAnywhere) float PumpTravel=9.f;
    UPROPERTY(EditAnywhere) float PumpRearTime=.21f;
    UPROPERTY(EditAnywhere) float PumpForwardTime=.44f;
    UPROPERTY(EditAnywhere) float FlashDuration=.045f;
    UPROPERTY(EditAnywhere) float FlashIntensity=18000.f;
    UPROPERTY(EditAnywhere) float FlashLength=21.f;
    UPROPERTY(EditAnywhere) float FlashRadius=4.3f;
    UPROPERTY(EditAnywhere) float FlashLightRadius=195.f;
    UPROPERTY(EditAnywhere) FLinearColor FlashLightColor=FLinearColor(1.f,.63f,.28f);
    UPROPERTY(EditAnywhere) FVector CaseImpulse=FVector(40,-185,140);
    UPROPERTY(EditAnywhere) float CaseRadius=.55f;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> ForeEndMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> ShellMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> EjectedCaseMesh;
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
    UPROPERTY(VisibleAnywhere) uint64 PendingCaseShotId=0;
    UPROPERTY(VisibleAnywhere) int32 EjectionCount=0;
    UPROPERTY(VisibleAnywhere) int32 LastShotSoundIndex=INDEX_NONE;
};
/** One spatial aggregate for one anatomical region in a discharge. */
struct FONEWeaponRegionDamage
{
    float Damage=0.f,Trauma=0.f;
    int32 Pellets=0;
    FVector Position=FVector::ZeroVector,Direction=FVector::ZeroVector,Normal=FVector::ZeroVector;
    FName Bone=NAME_None;
    void AddPellet(float HitDamage,float HitTrauma,const FVector& Point,const FVector& Ray,const FVector& SurfaceNormal,FName HitBone)
    {
        if (!FMath::IsFinite(HitDamage) || HitDamage<=0.f || Point.ContainsNaN() || Ray.ContainsNaN() || SurfaceNormal.ContainsNaN()) return;
        const float AddedTrauma=FMath::IsFinite(HitTrauma) ? FMath::Max(0.f,HitTrauma) : 0.f;
        const FVector NewPosition=Position+Point*HitDamage,NewDirection=Direction+Ray*HitDamage,NewNormal=Normal+SurfaceNormal*HitDamage;
        if (!FMath::IsFinite(Damage+HitDamage) || !FMath::IsFinite(Trauma+AddedTrauma) || NewPosition.ContainsNaN() || NewDirection.ContainsNaN() || NewNormal.ContainsNaN()) return;
        Damage+=HitDamage; Trauma+=AddedTrauma; ++Pellets;
        Position=NewPosition; Direction=NewDirection; Normal=NewNormal;
        Weight+=HitDamage;
        if (HitDamage>LargestPelletDamage) { LargestPelletDamage=HitDamage; Bone=HitBone; }
    }
    void Finalize()
    {
        if (Weight>0.f) { Position/=Weight; Weight=0.f; }
        Direction=Direction.GetSafeNormal(SMALL_NUMBER,FVector::ForwardVector);
        Normal=Normal.GetSafeNormal(SMALL_NUMBER,-Direction);
    }
private:
    float Weight=0.f,LargestPelletDamage=0.f;
};
/** One transaction per victim per actual discharge, after all pellet traces.
 *  Array indices are anatomical regions; imported source *_r is actual left.
 *  Position/direction/normal in each nonempty region are finalized before send.
 *  Directly authored entries (including probes) supply a finite world position
 *  and normalized direction/normal. AddPellet users call Finalize after tracing;
 *  repeating Finalize does not divide an already averaged position again.
 */
struct FONEWeaponDamagePacket
{
    static constexpr int32 RegionCount=static_cast<int32>(EONEHitRegion::Invalid);
    uint64 ShotId=0;
    float HeavyStaggerThreshold=10000.f;
    FONEWeaponRegionDamage Regions[RegionCount];
    static bool IsValidRegion(EONEHitRegion Region) { return static_cast<int32>(Region)>=0 && static_cast<int32>(Region)<RegionCount; }
    FONEWeaponRegionDamage& Get(EONEHitRegion Region) { check(IsValidRegion(Region)); return Regions[static_cast<int32>(Region)]; }
    const FONEWeaponRegionDamage& Get(EONEHitRegion Region) const { check(IsValidRegion(Region)); return Regions[static_cast<int32>(Region)]; }
    void Finalize() { for (auto& Region:Regions) Region.Finalize(); }
    float GetTotalDamage() const { float Total=0.f; for (const auto& Region:Regions) Total+=FMath::Max(0.f,Region.Damage); return Total; }
    int32 GetPellets() const { int32 Total=0; for (const auto& Region:Regions) Total+=FMath::Max(0,Region.Pellets); return Total; }
    FVector GetImpactPosition() const
    {
        FVector Point=FVector::ZeroVector; float Total=0.f;
        for (const auto& Region:Regions) { const float D=FMath::Max(0.f,Region.Damage); Point+=Region.Position*D; Total+=D; }
        return Total>0.f ? Point/Total : FVector::ZeroVector;
    }
};
