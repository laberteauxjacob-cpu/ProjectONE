#include "ONEPlayer.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEAnimInstance.h"
#include "ONEBloodSubsystem.h"
#include "ONEGameMode.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"

AONEPlayer::AONEPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    Health = CreateDefaultSubobject<UONEHealthComponent>(TEXT("Health"));
    Weapon = CreateDefaultSubobject<UONEWeaponComponent>(TEXT("Weapon"));
    GetCapsuleComponent()->InitCapsuleSize(28.f, 90.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    GetMesh()->SetRelativeLocation(FVector(0,0,-90));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetAnimationMode(EAnimationMode::AnimationBlueprint);
    GetMesh()->SetAnimInstanceClass(UONEAnimInstance::StaticClass());
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->MaxAcceleration = 1800.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1800.f;
    GetCharacterMovement()->GroundFriction = 8.f;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    Gun = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Carbine"));
    Gun->SetupAttachment(GetMesh(), TEXT("weapon_r"));
    Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    CameraArm->SetupAttachment(RootComponent);
    CameraArm->SetUsingAbsoluteRotation(true);
    CameraArm->SetRelativeRotation(FRotator(-58,-90,0));
    CameraArm->TargetArmLength = 1450;
    CameraArm->TargetOffset = FVector(0,0,30);
    CameraArm->bDoCollisionTest = false;
    CameraArm->bEnableCameraLag = true;
    CameraArm->CameraLagSpeed = 8;
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(CameraArm);
    Camera->FieldOfView = 52.f;
    MuzzleLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash"));
    MuzzleLight->SetupAttachment(Gun);
    MuzzleLight->SetRelativeLocation(FVector(58,0,14));
    MuzzleLight->SetLightColor(FLinearColor(1.f,.53f,.14f));
    MuzzleLight->SetIntensity(0);
    MuzzleLight->SetAttenuationRadius(170);
    MuzzleLight->SetCastShadows(false);
}
void AONEPlayer::BeginPlay()
{
    Super::BeginPlay();
    if (auto* Asset = LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Response.SK_Response"))) GetMesh()->SetSkeletalMesh(Asset);
    if (auto* Asset = LoadObject<UStaticMesh>(nullptr,TEXT("/Game/ONE/Art/Environment/SM_Carbine.SM_Carbine"))) Gun->SetStaticMesh(Asset);
    if (const USkeletalMesh* Asset=GetMesh()->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& Skeleton=Asset->GetRefSkeleton();
        FTransform Bind=FTransform::Identity;
        for (int32 Bone=Skeleton.FindBoneIndex(TEXT("weapon_r"));Bone!=INDEX_NONE;Bone=Skeleton.GetParentIndex(Bone)) Bind*=Skeleton.GetRefBonePose()[Bone];
        // The importer changes bone bases. Cancel the actual reference basis so the
        // authored +X barrel aligns with +X in the mesh's reference pose.
        Gun->SetRelativeRotation(Bind.GetRotation().Inverse());
    }
    GetMesh()->SetAnimInstanceClass(UONEAnimInstance::StaticClass());
    AimPoint = GetActorLocation() + GetActorForwardVector()*500;
}
void AONEPlayer::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"),this,&AONEPlayer::MoveForward);
    Input->BindAxis(TEXT("MoveRight"),this,&AONEPlayer::MoveRight);
    Input->BindAction(TEXT("Fire"),IE_Pressed,this,&AONEPlayer::StartFire);
    Input->BindAction(TEXT("Fire"),IE_Released,this,&AONEPlayer::StopFire);
    Input->BindAction(TEXT("Reload"),IE_Pressed,this,&AONEPlayer::Reload);
    Input->BindAction(TEXT("Run"),IE_Pressed,this,&AONEPlayer::StartSprint);
    Input->BindAction(TEXT("Run"),IE_Released,this,&AONEPlayer::StopSprint);
}
void AONEPlayer::MoveForward(float V) { if (!IsDead()) AddMovementInput(FVector(0,-1,0),V); }
void AONEPlayer::MoveRight(float V) { if (!IsDead()) AddMovementInput(FVector(1,0,0),V); }
void AONEPlayer::StartFire() { if (!IsDead()) Weapon->SetTrigger(true); }
void AONEPlayer::StopFire() { Weapon->SetTrigger(false); }
void AONEPlayer::Reload() { if (!IsDead()) Weapon->BeginReload(); }
void AONEPlayer::StartSprint() { bSprint = true; }
void AONEPlayer::StopSprint() { bSprint = false; }
void AONEPlayer::ReleaseHeldInputs() { bSprint=false; Weapon->SetTrigger(false); }
void AONEPlayer::Tick(float Dt)
{
    Super::Tick(Dt);
    MuzzleTime = FMath::Max(0.f,MuzzleTime-Dt);
    MuzzleLight->SetIntensity(MuzzleTime > 0 ? 18000.f : 0.f);
    if (IsDead()) return;
    GetCharacterMovement()->MaxWalkSpeed = bSprint && !Weapon->IsReloading() ? RunSpeed : WalkSpeed;
    if (bAimOverride) AimPoint=OverrideAimPoint;
    else if (auto* PC = Cast<APlayerController>(GetController()))
    {
        FVector Origin,Direction;
        if (PC->DeprojectMousePositionToWorld(Origin,Direction))
        {
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(MouseAim),false,this);
            if (GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*10000,ECC_Visibility,Params) && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Infected"))) AimPoint = Hit.ImpactPoint;
            else
            {
                float T = (GetActorLocation().Z + 42.f - Origin.Z) / FMath::Min(-.001f,Direction.Z);
                AimPoint = Origin + Direction*T;
            }
        }
    }
    FVector Flat = AimPoint - GetActorLocation(); Flat.Z = 0;
    if (!Flat.IsNearlyZero()) SetActorRotation(FMath::RInterpTo(GetActorRotation(),Flat.Rotation(),Dt,20.f));
}
FVector AONEPlayer::GetMuzzleLocation() const { return Gun->GetComponentTransform().TransformPosition(FVector(58,0,14)); }
void AONEPlayer::FlashMuzzle() { MuzzleTime = .045f; }
float AONEPlayer::GetHealth() const { return Health->Health; }
float AONEPlayer::GetMaxHealth() const { return Health->MaxHealth; }
bool AONEPlayer::IsDead() const { return Health->IsDead(); }
void AONEPlayer::ReceiveAttack(float Damage,const FVector& From)
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (IsDead() || Now-LastDamageTime < .55f) return;
    LastDamageTime = Now;
    Health->ApplyDamage(Damage);
    if (auto* Blood = GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->Impact(GetActorLocation()+FVector(0,0,30),(GetActorLocation()-From).GetSafeNormal(),false);
    if (IsDead())
    {
        Weapon->SetTrigger(false);
        Weapon->CancelReload();
        GetCharacterMovement()->StopMovementImmediately();
        if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->PlayerDied();
    }
}
