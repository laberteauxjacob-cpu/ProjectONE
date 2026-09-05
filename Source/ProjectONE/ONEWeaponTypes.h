#pragma once
#include "CoreMinimal.h"
#include "ONEWeaponTypes.generated.h"
class UAnimSequence;
class USoundBase;
class UStaticMesh;

UENUM(BlueprintType)
enum class EONEWeaponFamily : uint8 { Carbine, Shotgun, Pistol, Invalid };
UENUM(BlueprintType)
enum class EONEWeaponSlotStatus : uint8 { Empty, Available, MachineReserved, ReadyToCollect };
UENUM(BlueprintType)
enum class EONEWeaponAcquisitionKind : uint8 { Invalid, FillEmpty, Replace, Refill, AlreadyFull };
UENUM(BlueprintType)
enum class EONEWeaponOperation : uint8 { Ready, Equip, Fire, Pump, MagazineReload, ShellStart, ShellInsert, ShellEnd };
UENUM(BlueprintType)
enum class EONEWeaponEvent : uint8 { Sound, MagazineOut, MagazineCommit, ShellCommit, ShellEject, WeaponSwap, PumpLock };
UENUM(BlueprintType)
enum class EONEHitRegion : uint8 { Body, Head, ArmLeft, ArmRight, LegLeft, LegRight, Invalid };
UENUM(BlueprintType)
enum class EONEWeaponHitOutcome : uint8 { Rejected, LiveHit, NewKill, CorpseHit };
UENUM(BlueprintType)
enum class EONEWeaponInputResult : uint8
{
    None, AcceptedShot, AcceptedShellClose, DryFire,
    Unusable, Dead, Paused, Handoff, ReleaseRequired, Reloading,
    Pumping, Equipping, Cooldown, EmptyWithReserve, DryFireRateLimited
};
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
    UPROPERTY(EditAnywhere) EONEWeaponFamily Family=EONEWeaponFamily::Carbine;
    UPROPERTY(EditAnywhere) bool bUpgraded=false;
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
    UPROPERTY(EditAnywhere,meta=(ClampMin="0.04")) float FireInterval=.10f;
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
    UPROPERTY(EditAnywhere) FLinearColor TraceColor=FLinearColor(1.f,.75f,.3f);
    UPROPERTY(EditAnywhere) FLinearColor AuraColor=FLinearColor::Transparent;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="1")) int32 AdditionalVictims=0;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="1")) float PenetrationDamageFraction=.60f;
    UPROPERTY(EditAnywhere) float MagazineFreshTime=.74f;
    UPROPERTY(EditAnywhere) FVector MagazineHandOffset=FVector(-10.5f,0,0);
    UPROPERTY(EditAnywhere) FVector ShellHandOffset=FVector(6,0,2.8f);
    UPROPERTY(EditAnywhere) float SlideTravel=3.f;
    UPROPERTY(EditAnywhere) FVector CaseImpulse=FVector(40,-185,140);
    UPROPERTY(EditAnywhere) float CaseRadius=.55f;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> SlideMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> ForeEndMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> ShellMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> EjectedCaseMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> MagazineMesh;
    UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> ReadyAnimation;
    UPROPERTY(EditAnywhere) TArray<TSoftObjectPtr<USoundBase>> ShotSounds;
    UPROPERTY(EditAnywhere,meta=(ClampMin="0",ClampMax="2")) float ShotVolume=.62f;
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
    UPROPERTY(VisibleAnywhere) uint64 InstanceId=0;
    UPROPERTY(VisibleAnywhere) EONEWeaponFamily Family=EONEWeaponFamily::Invalid;
    UPROPERTY(VisibleAnywhere) bool bUpgraded=false;
    UPROPERTY(VisibleAnywhere) EONEWeaponSlotStatus Status=EONEWeaponSlotStatus::Empty;
    UPROPERTY(VisibleAnywhere) int32 Ammo=0;
    UPROPERTY(VisibleAnywhere) int32 Reserve=0;
    UPROPERTY(VisibleAnywhere) bool bNeedsPump=false;
    UPROPERTY(VisibleAnywhere) bool bCaseEjected=false;
    UPROPERTY(VisibleAnywhere) uint64 PendingCaseShotId=0;
    UPROPERTY(VisibleAnywhere) int32 EjectionCount=0;
    UPROPERTY(VisibleAnywhere) int32 LastShotSoundIndex=INDEX_NONE;
    // A released magazine remains absent until a real insertion event. Resuming
    // a canceled reload cannot emit the same old magazine for a second time.
    UPROPERTY(VisibleAnywhere) bool bMagazinePresent=true;
    UPROPERTY(VisibleAnywhere) int32 MagazineDropCount=0;
};
/** Opaque identity plus inspectable snapshot; the component keeps its own
 *  authoritative copy and never restores caller-edited snapshot values. */
struct FONEWeaponReservation
{
    uint64 RunId=0,ReservationId=0,InstanceId=0;
    int32 Slot=INDEX_NONE;
    FONECarriedWeaponState Before;
    bool IsValid() const { return RunId!=0 && ReservationId!=0 && InstanceId!=0 && Slot>=0 && Slot<2; }
};
struct FONEWeaponAcquisitionPlan
{
    EONEWeaponAcquisitionKind Kind=EONEWeaponAcquisitionKind::Invalid;
    EONEWeaponFamily Family=EONEWeaponFamily::Invalid;
    int32 Slot=INDEX_NONE;
    uint64 RunId=0,Revision=0,ExpectedInstanceId=0;
    bool IsValid() const { return Kind!=EONEWeaponAcquisitionKind::Invalid; }
    bool operator==(const FONEWeaponAcquisitionPlan& B) const
    { return Kind==B.Kind && Family==B.Family && Slot==B.Slot && RunId==B.RunId && Revision==B.Revision && ExpectedInstanceId==B.ExpectedInstanceId; }
    bool operator!=(const FONEWeaponAcquisitionPlan& B) const { return !(*this==B); }
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
