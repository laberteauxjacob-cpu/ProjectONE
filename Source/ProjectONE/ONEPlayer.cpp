#include "ONEPlayer.h"
#include "ONEAim.h"
#include "ONEHealthComponent.h"
#include "ONEWeaponComponent.h"
#include "ONEInteractionComponent.h"
#include "ONEProgressionMachine.h"
#include "Animation/AnimSequence.h"
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
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"

AONEPlayer::AONEPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    Health = CreateDefaultSubobject<UONEHealthComponent>(TEXT("Health"));
    Weapon = CreateDefaultSubobject<UONEWeaponComponent>(TEXT("Weapon"));
    Interaction = CreateDefaultSubobject<UONEInteractionComponent>(TEXT("Interaction"));
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
    Gun->SetCanEverAffectNavigation(false);
    ForeEnd=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShotgunForeEnd"));
    ForeEnd->SetupAttachment(Gun);
    ForeEnd->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ForeEnd->SetCanEverAffectNavigation(false);
    Slide=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PistolSlide"));
    Slide->SetupAttachment(Gun);
    Slide->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Slide->SetCanEverAffectNavigation(false);
    LoadingShell=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LoadingShell"));
    LoadingShell->SetupAttachment(GetMesh(),TEXT("hand_l"));
    LoadingShell->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LoadingShell->SetCanEverAffectNavigation(false);
    LoadingShell->SetVisibility(false);
    SeatedMagazine=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SeatedMagazine"));
    SeatedMagazine->SetupAttachment(Gun);
    HeldMagazine=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldMagazine"));
    HeldMagazine->SetupAttachment(GetMesh(),TEXT("hand_l"));
    for (auto* Part:TArray<UStaticMeshComponent*>{SeatedMagazine,HeldMagazine})
    { Part->SetCollisionEnabled(ECollisionEnabled::NoCollision); Part->SetCanEverAffectNavigation(false); }
    HeldMagazine->SetVisibility(false);
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
    MuzzleFlashMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("AttachedMuzzleFlash"));
    MuzzleFlashMesh->SetupAttachment(Gun);
    MuzzleFlashMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MuzzleFlashMesh->SetCanEverAffectNavigation(false);
    MuzzleFlashMesh->SetCastShadow(false);
    MuzzleFlashMesh->SetVisibility(false);
    HeldAuraMesh=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("HeldUpgradeAura"));
    HeldAuraMesh->SetupAttachment(Gun);
    HeldAuraMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeldAuraMesh->SetCanEverAffectNavigation(false);
    HeldAuraMesh->SetCastShadow(false);
    HeldAuraMesh->SetVisibility(false);
    HeldAuraLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("HeldUpgradeLight"));
    HeldAuraLight->SetupAttachment(Gun);
    HeldAuraLight->SetCastShadows(false);
    HeldAuraLight->SetAttenuationRadius(95.f);
    HeldAuraLight->SetIntensity(0.f);
    HeldAuraLight->SetVisibility(false);
}
void AONEPlayer::BeginPlay()
{
    Super::BeginPlay();
    // CharacterMovement already ticks before this actor and the mesh. The mesh
    // must also wait for our aim/body update, or actor yaw rotates yesterday's
    // evaluated feet for a frame before the root correction and pivot IK run.
    GetMesh()->AddTickPrerequisiteActor(this);
    if (auto* Asset = LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Response.SK_Response"))) GetMesh()->SetSkeletalMesh(Asset);
    if (const USkeletalMesh* Asset=GetMesh()->GetSkeletalMeshAsset())
    {
        const FReferenceSkeleton& Skeleton=Asset->GetRefSkeleton();
        FTransform Bind=FTransform::Identity;
        for (int32 Bone=Skeleton.FindBoneIndex(TEXT("weapon_r"));Bone!=INDEX_NONE;Bone=Skeleton.GetParentIndex(Bone)) Bind*=Skeleton.GetRefBonePose()[Bone];
        // The importer changes bone bases. Cancel the actual reference basis so the
        // authored +X barrel aligns with +X in the mesh's reference pose.
        Gun->SetRelativeRotation(Bind.GetRotation().Inverse());
        FTransform ShellBind=FTransform::Identity;
        for (int32 Bone=Skeleton.FindBoneIndex(TEXT("hand_l"));Bone!=INDEX_NONE;Bone=Skeleton.GetParentIndex(Bone)) ShellBind*=Skeleton.GetRefBonePose()[Bone];
        LoadingShell->SetRelativeRotation(ShellBind.GetRotation().Inverse());
        LoadingShell->SetRelativeLocation(ShellBind.GetRotation().Inverse().RotateVector(FVector(6,0,2.8f)));
        HeldMagazine->SetRelativeRotation(ShellBind.GetRotation().Inverse());
        HandReferenceInverse=ShellBind.GetRotation().Inverse();
        HeldMagazine->SetRelativeLocation(ShellBind.GetRotation().Inverse().RotateVector(FVector(-10.5f,0,0)));
    }
    BuildMuzzleFlash();
    Weapon->RefreshEquippedPresentation();
    for (const TCHAR* Family:{TEXT("Pistol"),TEXT("Carbine"),TEXT("Shotgun")})
        for (const TCHAR* Action:{TEXT("Handoff"),TEXT("Retrieve")})
        {
            const FString Key=FString(Family)+Action;
            const FString Asset=TEXT("A_Response_C04_")+Key;
            if (auto* Clip=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/ONE/Animations/Candidate04/")+Asset+TEXT(".")+Asset)))
                MachineClips.Add(FName(*Key),Clip);
        }
    GetMesh()->SetAnimInstanceClass(UONEAnimInstance::StaticClass());
    IntendedAimDirection=GetActorForwardVector().GetSafeNormal2D();
    AimPoint = GetAimOrigin() + IntendedAimDirection*500;
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
    Input->BindAction(TEXT("Weapon1"),IE_Pressed,this,&AONEPlayer::SelectCarbine);
    Input->BindAction(TEXT("Weapon2"),IE_Pressed,this,&AONEPlayer::SelectShotgun);
    Input->BindAction(TEXT("CycleWeapon"),IE_Pressed,this,&AONEPlayer::CycleWeapon);
    Input->BindAction(TEXT("Interact"),IE_Pressed,this,&AONEPlayer::StartInteract);
    Input->BindAction(TEXT("Interact"),IE_Released,this,&AONEPlayer::StopInteract);
}
void AONEPlayer::MoveForward(float V) { if (!IsDead() && !bMachineAction) AddMovementInput(FVector(0,-1,0),V); }
void AONEPlayer::MoveRight(float V) { if (!IsDead() && !bMachineAction) AddMovementInput(FVector(1,0,0),V); }
void AONEPlayer::StartFire() { if (!IsDead()) Weapon->SetTrigger(true); }
void AONEPlayer::StopFire() { Weapon->SetTrigger(false); }
void AONEPlayer::Reload() { if (!IsDead()) Weapon->BeginReload(); }
void AONEPlayer::StartSprint() { SetSprintHeld(true); }
void AONEPlayer::StopSprint() { SetSprintHeld(false); }
void AONEPlayer::SetSprintHeld(bool Held)
{
    const bool bTrace=FParse::Param(FCommandLine::Get(),TEXT("ONE03InputTrace"));
    if (bTrace) UE_LOG(LogTemp,Display,TEXT("ONE03_SPRINT_REQUEST held=%d previous=%d paused=%d dead=%d operation=%d ammo=%d reserve=%d"),
        Held,bSprint,UGameplayStatics::IsGamePaused(this),IsDead(),Weapon ? int32(Weapon->GetOperation()) : -1,Weapon ? Weapon->GetAmmo() : -1,Weapon ? Weapon->GetReserveAmmo() : -1);
    if (Held && (IsDead() || bMachineAction || UGameplayStatics::IsGamePaused(this))) return;
    const bool bStarted=Held && !bSprint;
    bSprint=Held;
    GetCharacterMovement()->MaxWalkSpeed=bSprint ? RunSpeed : WalkSpeed;
    // Candidate05 commits magazine reloads. Sprint changes movement only;
    // neither manual nor automatic reload waits for the Shift key to release.
    if (bTrace) UE_LOG(LogTemp,Display,TEXT("ONE03_SPRINT_RESULT held=%d started=%d operation=%d interrupts=%d ammo=%d reserve=%d"),
        bSprint,bStarted,Weapon ? int32(Weapon->GetOperation()) : -1,Weapon ? Weapon->GetSprintReloadInterruptCount() : -1,Weapon ? Weapon->GetAmmo() : -1,Weapon ? Weapon->GetReserveAmmo() : -1);
}
void AONEPlayer::ClearReloadPresentation()
{
    LoadingShell->SetVisibility(false);
    HeldMagazine->SetVisibility(false);
    SeatedMagazine->SetVisibility(!bSuppressCarried && Weapon && Weapon->GetDefinition().MagazineMesh.IsValid() && Weapon->ShouldShowSeatedMagazine());
}
void AONEPlayer::SelectCarbine() { Weapon->SelectWeapon(0); }
void AONEPlayer::SelectShotgun() { Weapon->SelectWeapon(1); }
void AONEPlayer::CycleWeapon() { Weapon->CycleWeapon(); }
void AONEPlayer::StartInteract() { if (!IsDead()) Interaction->Press(); }
void AONEPlayer::StopInteract() { Interaction->Release(); }
void AONEPlayer::ReleaseHeldInputs()
{
    SetSprintHeld(false); Weapon->ClearHeldInput(); ClearMuzzleFlash(); Interaction->Cancel(true);
    if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->CancelUnacceptedMachineActions(this);
}
float AONEPlayer::GetPivotFootWeight(int32 Foot) const
{
    // Release each captured support into its authored swing, not a shared
    // one-frame rotation of both feet. This correction only runs on sharp pivots.
    const float T=FMath::Clamp((PivotElapsed-PivotReleaseAt[Foot])/.13f,0.f,1.f);
    return 1.f-T*T*(3.f-2.f*T);
}
void AONEPlayer::CapturePivotFeet()
{
    const FVector Forward=FRotator(0,BodyFacingYaw,0).Vector();
    const FName Feet[]={TEXT("foot_l"),TEXT("foot_r")};
    const FName Knees[]={TEXT("calf_l"),TEXT("calf_r")};
    for (int32 I=0;I<2;++I)
    {
        PivotFeet[I]=GetMesh()->GetSocketTransform(Feet[I],RTS_World);
        PivotKnees[I]=GetMesh()->GetSocketLocation(Knees[I])+Forward*30.f;
        PivotReleaseAt[I]=0.f;
    }
    if (bTurningInPlace)
    {
        // Source _l opens a positive UE turn; _r opens a negative one.
        // The other support stays fixed until the second authored step begins.
        const int32 Trailing=TurnDirection>0 ? 1 : 0;
        PivotReleaseAt[Trailing]=FMath::Clamp((AuthoredTurnDuration*.5f-TurnTime)/
            FMath::Clamp(AimAngularSpeed*AuthoredTurnDuration/90.f*1.5f,1.f,4.f),0.f,.22f);
    }
    PivotElapsed=0.f;
}
void AONEPlayer::UpdateBodyFacing(float Dt,float AimYaw)
{
    PivotElapsed+=Dt;
    if (!bFacingInitialized)
    {
        BodyFacingYaw=AimYaw;
        PreviousAimYaw=AimYaw;
        bFacingInitialized=true;
    }
    const float MeasuredRate=FMath::Abs(FMath::FindDeltaAngleDegrees(PreviousAimYaw,AimYaw))/FMath::Max(.001f,Dt);
    // Respond immediately to acceleration; let the step finish when the mouse
    // stops, rather than dropping playback speed halfway through a footfall.
    AimAngularSpeed=FMath::Max(MeasuredRate,FMath::FInterpTo(AimAngularSpeed,0.f,Dt,7.f));
    PreviousAimYaw=AimYaw;
    const float Speed=GetVelocity().Size2D();
    float Delta=FMath::FindDeltaAngleDegrees(BodyFacingYaw,AimYaw);
    if (Speed>20.f)
    {
        // Moving feet already take steps. Preserve the direction cycle while the
        // pelvis catches aim changes; upper-body aim is evaluated independently.
        bTurningInPlace=false;
        BodyFacingYaw=FMath::FixedTurn(BodyFacingYaw,AimYaw,Dt*540.f);
    }
    else
    {
        if (bTurningInPlace && Delta*TurnDirection < -28.f)
        {
            TurnDirection=Delta>=0 ? 1 : -1;
            TurnStartYaw=BodyFacingYaw;
            TurnTime=0.f;
            CapturePivotFeet();
        }
        if (!bTurningInPlace && FMath::Abs(Delta)>TurnTriggerAngle)
        {
            bTurningInPlace=true;
            TurnDirection=Delta>=0 ? 1 : -1;
            TurnStartYaw=BodyFacingYaw;
            TurnTime=0.f;
        }
        if (bTurningInPlace)
        {
            // Same two-step yaw curve as the source clip. A quicker mouse sweep
            // accelerates the step, rather than spinning a static idle pose.
            const float FollowRate=AimAngularSpeed*AuthoredTurnDuration/90.f*1.5f;
            const float Rate=FMath::Clamp(FMath::Max(FollowRate,FMath::Abs(Delta)/55.f),1.f,4.f);
            TurnTime=FMath::Min(AuthoredTurnDuration,TurnTime+Dt*Rate);
            const float T=FMath::Clamp(TurnTime/FMath::Max(.01f,AuthoredTurnDuration),0.f,1.f);
            auto Smooth=[](float V) { V=FMath::Clamp(V,0.f,1.f); return V*V*(3.f-2.f*V); };
            const float Fraction=T<.5f ? .5f*Smooth(T*2.f) : .5f+.5f*Smooth((T-.5f)*2.f);
            BodyFacingYaw=FRotator::NormalizeAxis(TurnStartYaw+TurnDirection*90.f*Fraction);
            if (T>=1.f) bTurningInPlace=false;
        }
    }
    // Keep aim responsive and the waist bounded during instantaneous reversals.
    // Capture the evaluated feet before changing actor yaw; the animation graph
    // solves them in world space and releases them into consecutive swing steps.
    Delta=FMath::FindDeltaAngleDegrees(BodyFacingYaw,AimYaw);
    if (FMath::Abs(Delta)>MaximumAimOffset)
    {
        const float Adjustment=Delta-FMath::Sign(Delta)*MaximumAimOffset;
        if (FMath::Abs(Adjustment)>4.f) CapturePivotFeet();
        BodyFacingYaw=FRotator::NormalizeAxis(BodyFacingYaw+Adjustment);
        if (bTurningInPlace) TurnStartYaw=FRotator::NormalizeAxis(TurnStartYaw+Adjustment);
    }
}
void AONEPlayer::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bMachineAction) MachineActionTime+=Dt;
    UpdateMuzzleFlash(Dt);
    UpdateHeldAura();
    ForeEnd->SetRelativeLocation(FVector(-Weapon->GetDefinition().PumpTravel*Weapon->GetPumpFraction(),0,0));
    Slide->SetRelativeLocation(FVector(-Weapon->GetDefinition().SlideTravel*Weapon->GetSlideFraction(),0,0));
    LoadingShell->SetVisibility(!bSuppressCarried && !IsDead() && Weapon->ShouldShowLoadingShell());
    SeatedMagazine->SetVisibility(!bSuppressCarried && Weapon->GetDefinition().MagazineMesh.IsValid() && Weapon->ShouldShowSeatedMagazine());
    HeldMagazine->SetVisibility(!bSuppressCarried && !IsDead() && Weapon->GetDefinition().MagazineMesh.IsValid() && Weapon->ShouldShowHeldMagazine());
    if (IsDead()) return;
    GetCharacterMovement()->MaxWalkSpeed = bSprint ? RunSpeed : WalkSpeed;
    if (bMachineAction) AimPoint=MachineFocus;
    else if (bAimOverride) AimPoint=OverrideAimPoint;
    else if (auto* PC = Cast<APlayerController>(GetController()))
    {
        FVector Origin,Direction;
        if (PC->DeprojectMousePositionToWorld(Origin,Direction))
        {
            FHitResult Hit;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(MouseAim),false,this);
            if (GetWorld()->LineTraceSingleByChannel(Hit,Origin,Origin+Direction*10000,ECC_Visibility,Params) && Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Infected")))
            {
                AimPoint=Hit.ImpactPoint;
                // Pick inside the exact anatomical primitive under the cursor,
                // along the same camera ray. A camera-facing entry surface is
                // a grazing aim point from the muzzle's different viewpoint.
                // Actual shoulder/muzzle traces still decide damage and cover.
                FHitResult Exit;
                if (auto* Region=Hit.GetComponent(); Region && Region->LineTraceComponent(Exit,Origin+Direction*10000,Origin,Params) &&
                    Exit.BoneName==Hit.BoneName &&
                    FVector::DotProduct(Exit.ImpactPoint-Hit.ImpactPoint,Direction)>0.f)
                    AimPoint=(Hit.ImpactPoint+Exit.ImpactPoint)*.5f;
            }
            else
            {
                if (FMath::Abs(Direction.Z)>.001f)
                {
                    const double T=(GetAimOrigin().Z-Origin.Z)/Direction.Z;
                    if (T>0 && FMath::IsFinite(T)) AimPoint=Origin+Direction*T;
                }
            }
        }
    }
    IntendedAimDirection=ONEAim::ResolveIntent(GetAimOrigin(),AimPoint,IntendedAimDirection,AimCenterRadius);
    const float AimYaw=IntendedAimDirection.Rotation().Yaw;
    UpdateBodyFacing(Dt,AimYaw);
    SetActorRotation(FRotator(0,AimYaw,0));
}
FVector AONEPlayer::GetShotDirection(const FVector& EvaluatedMuzzle) const
{
    return ONEAim::ResolveShotDirection(GetAimOrigin(),AimPoint,IntendedAimDirection,EvaluatedMuzzle,AimConvergenceAhead,AimMaximumPitch);
}
float AONEPlayer::GetDamageReactionAge() const
{
    return GetWorld() ? GetWorld()->GetTimeSeconds()-LastDamageTime : 100.f;
}
FVector AONEPlayer::GetMuzzleLocation() const { return Gun->GetComponentTransform().TransformPosition(Weapon->GetDefinition().Muzzle); }
void AONEPlayer::BuildMuzzleFlash()
{
    // Original tapered, lobed volume in muzzle-local +X. Vertex alpha softens
    // the tips; crossed hot-core surfaces avoid a flat camera-facing rectangle.
    TArray<FVector> Vertices,Normals; TArray<int32> Indices;
    TArray<FVector2D> UV; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
    const float X[]={0.f,.17f,.46f,1.f};
    const float Radius[]={.12f,.72f,.42f,0.f};
    const FLinearColor Tint=Weapon->GetDefinition().FlashLightColor;
    const bool Upgraded=Weapon->GetDefinition().bUpgraded;
    FLinearColor Color[]={FMath::Lerp(Tint,FLinearColor::White,Upgraded?.25f:.75f),Tint,Tint*.55f,Tint*.25f};
    Color[0].A=.88f; Color[1].A=.80f; Color[2].A=.25f; Color[3].A=0;
    constexpr int32 Sides=8;
    for (int32 Ring=0;Ring<4;++Ring) for (int32 Side=0;Side<Sides;++Side)
    {
        const float Angle=Side*2.f*PI/Sides;
        const float Lobe=Side%2==0 ? 1.f : .43f;
        const FVector Radial(0,FMath::Cos(Angle),FMath::Sin(Angle));
        Vertices.Add(FVector(X[Ring],0,0)+Radial*Radius[Ring]*Lobe);
        Normals.Add(Radial); UV.Add(FVector2D(X[Ring],float(Side)/Sides));
        Colors.Add(Color[Ring]); Tangents.Add(FProcMeshTangent(1,0,0));
    }
    for (int32 Ring=0;Ring<3;++Ring) for (int32 Side=0;Side<Sides;++Side)
    {
        const int32 A=Ring*Sides+Side,B=Ring*Sides+(Side+1)%Sides,C=A+Sides,D=B+Sides;
        Indices.Append({A,C,B,B,C,D});
    }
    for (int32 Plane=0;Plane<3;++Plane)
    {
        const float Angle=Plane*PI/3.f; const FVector Across(0,FMath::Cos(Angle)*.12f,FMath::Sin(Angle)*.12f);
        const int32 Base=Vertices.Num();
        Vertices.Append({-Across,Across,FVector(.63f,0,0)});
        Normals.Append({FVector::UpVector,FVector::UpVector,FVector::UpVector});
        UV.Append({FVector2D(0,0),FVector2D(0,1),FVector2D(1,.5f)});
        FLinearColor Core=FMath::Lerp(Tint,FLinearColor::White,Upgraded?.55f:.9f),Tip=Tint; Core.A=.9f; Tip.A=0;
        Colors.Append({Core,Core,Tip});
        Tangents.Append({FProcMeshTangent(1,0,0),FProcMeshTangent(1,0,0),FProcMeshTangent(1,0,0)});
        Indices.Append({Base,Base+1,Base+2});
    }
    MuzzleFlashMesh->CreateMeshSection_LinearColor(0,Vertices,Indices,Normals,UV,Colors,Tangents,false);
    if (auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_MuzzleFlash_C03.M_MuzzleFlash_C03")))
    {
        MuzzleFlashMaterial=UMaterialInstanceDynamic::Create(Material,this);
        MuzzleFlashMesh->SetMaterial(0,MuzzleFlashMaterial);
    }
    ClearMuzzleFlash();
}
void AONEPlayer::FlashMuzzle()
{
    const auto& D=Weapon->GetDefinition();
    MuzzleDuration=FMath::Max(.01f,D.FlashDuration*FMath::FRandRange(.9f,1.1f));
    MuzzleTime=MuzzleDuration; MuzzlePeakIntensity=D.FlashIntensity*FMath::FRandRange(.92f,1.08f);
    MuzzleFlashMesh->SetRelativeRotation(FRotator(0,0,FMath::FRandRange(0.f,360.f)));
    MuzzleFlashMesh->SetRelativeScale3D(FVector(D.FlashLength*FMath::FRandRange(.88f,1.12f),D.FlashRadius,D.FlashRadius));
    // Weapon ticks after pose evaluation. Illuminate immediately so the first
    // rendered flash and its light use the same discharge and muzzle transform.
    MuzzleFlashMesh->SetVisibility(true);
    if (MuzzleFlashMaterial) MuzzleFlashMaterial->SetScalarParameterValue(TEXT("FlashAlpha"),1.f);
    MuzzleLight->SetIntensity(MuzzlePeakIntensity);
}
void AONEPlayer::UpdateMuzzleFlash(float Dt)
{
    if (MuzzleTime<=0.f) return;
    MuzzleTime=FMath::Max(0.f,MuzzleTime-Dt);
    if (MuzzleTime<=0.f) { ClearMuzzleFlash(); return; }
    const float Remaining=MuzzleTime/FMath::Max(.001f,MuzzleDuration);
    const float Envelope=Remaining*Remaining;
    MuzzleLight->SetIntensity(MuzzlePeakIntensity*Envelope);
    if (MuzzleFlashMaterial) MuzzleFlashMaterial->SetScalarParameterValue(TEXT("FlashAlpha"),Envelope);
}
void AONEPlayer::ClearMuzzleFlash()
{
    MuzzleTime=0.f; MuzzleLight->SetIntensity(0.f); MuzzleFlashMesh->SetVisibility(false);
    if (MuzzleFlashMaterial) MuzzleFlashMaterial->SetScalarParameterValue(TEXT("FlashAlpha"),0.f);
}
bool AONEPlayer::IsMuzzleFlashVisible() const { return MuzzleTime>0.f && MuzzleFlashMesh->IsVisible(); }
float AONEPlayer::GetMuzzleFlashIntensity() const { return MuzzleLight->Intensity; }
FTransform AONEPlayer::GetMuzzleFlashTransform() const { return MuzzleFlashMesh->GetComponentTransform(); }
void AONEPlayer::ClearWeaponEffects()
{
    ClearMuzzleFlash();
    HeldAuraMesh->SetVisibility(false); HeldAuraLight->SetVisibility(false); HeldAuraLight->SetIntensity(0.f);
    ForeEnd->SetRelativeLocation(FVector::ZeroVector); LoadingShell->SetVisibility(false); HeldMagazine->SetVisibility(false);
}
void AONEPlayer::BuildHeldAura(const FONEWeaponDefinition& D)
{
    HeldAuraMesh->ClearAllMeshSections();
    HeldAuraColor=D.bUpgraded ? D.AuraColor : FLinearColor::Transparent;
    if (!D.bUpgraded) { UpdateHeldAura(); return; }
    // Two thin, tapered energy contours hug the upper receiver. They leave the
    // magazine, grip, slide and pump exposed and use one permanent component.
    TArray<FVector> V,N; TArray<int32> I; TArray<FVector2D> UV;
    TArray<FLinearColor> C; TArray<FProcMeshTangent> T;
    const float End=D.Muzzle.X*(D.Family==EONEWeaponFamily::Pistol?.82f:.53f);
    const float Start=D.Family==EONEWeaponFamily::Pistol ? -5.f : -10.f;
    const float Side=D.Family==EONEWeaponFamily::Pistol ? 2.7f : 4.f;
    for (int32 Rail:{-1,1}) for (int32 Segment=0;Segment<8;++Segment)
    {
        const float A=Segment/8.f,B=(Segment+1)/8.f;
        const float YA=Rail*(Side+FMath::Sin(A*PI)*.7f),YB=Rail*(Side+FMath::Sin(B*PI)*.7f);
        const float ZA=D.Muzzle.Z+1.4f+FMath::Sin(A*PI)*.6f,ZB=D.Muzzle.Z+1.4f+FMath::Sin(B*PI)*.6f;
        const FVector PA(FMath::Lerp(Start,End,A),YA,ZA),PB(FMath::Lerp(Start,End,B),YB,ZB);
        const float WA=.7f*FMath::Sin(A*PI)+.10f,WB=.7f*FMath::Sin(B*PI)+.10f;
        const int32 Base=V.Num();
        V.Append({PA-FVector(0,WA,0),PA+FVector(0,WA,0),PB-FVector(0,WB,0),PB+FVector(0,WB,0)});
        I.Append({Base,Base+2,Base+1,Base+1,Base+2,Base+3});
        for (int32 K=0;K<4;++K)
        {
            N.Add(FVector::UpVector); UV.Add(FVector2D(K<2?A:B,K%2)); T.Add(FProcMeshTangent(1,0,0));
            FLinearColor Color=D.AuraColor; Color.A=.5f; C.Add(Color);
        }
    }
    HeldAuraMesh->CreateMeshSection_LinearColor(0,V,I,N,UV,C,T,false);
    if (auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/ONE/Materials/M_Tracer_C04.M_Tracer_C04")))
        HeldAuraMesh->SetMaterial(0,Material);
    HeldAuraLight->SetRelativeLocation(FVector((Start+End)*.5f,0,D.Muzzle.Z+3.f));
    HeldAuraLight->SetLightColor(D.AuraColor);
    UpdateHeldAura();
}
void AONEPlayer::UpdateHeldAura()
{
    const bool Visible=Weapon && Weapon->HasUsableWeapon() && Weapon->GetDefinition().bUpgraded &&
        !bSuppressCarried && !bMachineAction && !IsDead() && Gun->IsVisible() && HeldAuraColor.A>0;
    HeldAuraMesh->SetVisibility(Visible); HeldAuraLight->SetVisibility(Visible);
    const float Pulse=GetWorld() ? .94f+.06f*FMath::Sin(GetWorld()->GetTimeSeconds()*4.f) : 1.f;
    HeldAuraLight->SetIntensity(Visible ? 75.f*Pulse : 0.f);
}
bool AONEPlayer::IsHeldAuraVisible() const { return HeldAuraMesh->IsVisible(); }
float AONEPlayer::GetHeldAuraLightIntensity() const { return HeldAuraLight->Intensity; }
void AONEPlayer::ApplyWeaponPresentation(const FONEWeaponDefinition& Definition)
{
    Gun->SetStaticMesh(Definition.Mesh.Get());
    Gun->SetVisibility(!bSuppressCarried);
    Slide->SetStaticMesh(Definition.SlideMesh.Get());
    Slide->SetVisibility(!bSuppressCarried && Definition.SlideMesh.IsValid());
    ForeEnd->SetStaticMesh(Definition.ForeEndMesh.Get());
    ForeEnd->SetVisibility(!bSuppressCarried && Definition.ForeEndMesh.IsValid());
    LoadingShell->SetStaticMesh(Definition.ShellMesh.Get());
    LoadingShell->SetVisibility(false);
    SeatedMagazine->SetStaticMesh(Definition.MagazineMesh.Get());
    SeatedMagazine->SetVisibility(!bSuppressCarried && Definition.MagazineMesh.IsValid() && Weapon->ShouldShowSeatedMagazine());
    HeldMagazine->SetStaticMesh(Definition.MagazineMesh.Get());
    HeldMagazine->SetVisibility(false);
    HeldMagazine->SetRelativeLocation(HandReferenceInverse.RotateVector(Definition.MagazineHandOffset));
    LoadingShell->SetRelativeLocation(HandReferenceInverse.RotateVector(Definition.ShellHandOffset));
    MuzzleLight->SetRelativeLocation(Definition.Muzzle);
    MuzzleLight->SetLightColor(Definition.FlashLightColor);
    MuzzleLight->SetAttenuationRadius(Definition.FlashLightRadius);
    MuzzleFlashMesh->SetRelativeLocation(Definition.Muzzle);
    BuildMuzzleFlash();
    BuildHeldAura(Definition);
}
void AONEPlayer::ClearEquippedPresentation()
{
    ClearWeaponEffects();
    for (auto* Part:{Gun.Get(),ForeEnd.Get(),Slide.Get(),LoadingShell.Get(),SeatedMagazine.Get(),HeldMagazine.Get()})
    { Part->SetStaticMesh(nullptr); Part->SetVisibility(false); }
}
FTransform AONEPlayer::GetWeaponWorldTransform() const { return Gun->GetComponentTransform(); }
FTransform AONEPlayer::GetMagazineReleaseTransform() const { return SeatedMagazine->GetComponentTransform(); }
void AONEPlayer::BeginMachineAction(EONEWeaponFamily Family,bool bRetrieving,const FVector& FocusPoint)
{
    // Lock first: it settles elapsed reload events before canceling future work.
    Weapon->SetHandoffLocked(true); Weapon->ClearHeldInput(); SetSprintHeld(false);
    GetCharacterMovement()->StopMovementImmediately();
    MachineActionTime=0; bMachineAction=true; MachineFocus=FocusPoint;
    const TCHAR* Prefix=Family==EONEWeaponFamily::Pistol ? TEXT("Pistol") : Family==EONEWeaponFamily::Shotgun ? TEXT("Shotgun") : TEXT("Carbine");
    MachineActionClip=MachineClips.FindRef(FName(*(FString(Prefix)+(bRetrieving?TEXT("Retrieve"):TEXT("Handoff")))));
}
void AONEPlayer::EndMachineAction()
{
    bMachineAction=false; MachineActionClip=nullptr; Weapon->SetHandoffLocked(false); SuppressCarriedPresentation(false);
}
UAnimSequence* AONEPlayer::GetMachineActionAnimation(float& Time) const
{
    Time=MachineActionTime; return bMachineAction ? MachineActionClip.Get() : nullptr;
}
void AONEPlayer::SuppressCarriedPresentation(bool bSuppress)
{
    bSuppressCarried=bSuppress;
    if (bSuppress)
    {
        ClearWeaponEffects();
        for (auto* Part:{Gun.Get(),ForeEnd.Get(),Slide.Get(),LoadingShell.Get(),SeatedMagazine.Get(),HeldMagazine.Get()}) Part->SetVisibility(false);
    }
    else Weapon->RefreshEquippedPresentation();
}
float AONEPlayer::GetHealth() const { return Health->Health; }
float AONEPlayer::GetMaxHealth() const { return Health->MaxHealth; }
bool AONEPlayer::IsDead() const { return Health->IsDead(); }
void AONEPlayer::ReceiveAttack(float Damage,const FVector& From)
{
    const float Now = GetWorld()->GetTimeSeconds();
    if (IsDead() || Now-LastDamageTime < .55f) return;
    LastDamageTime = Now;
    DamageReactionDirection=(GetActorLocation()-From).GetSafeNormal2D();
    Health->ApplyDamage(Damage);
    if (auto* Blood = GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->Impact(GetActorLocation()+FVector(0,0,30),(GetActorLocation()-From).GetSafeNormal(),false);
    if (IsDead())
    {
        Interaction->Cancel(true);
        EndMachineAction();
        Weapon->CancelAllOperations();
        GetCharacterMovement()->StopMovementImmediately();
        if (auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>()) GM->PlayerDied();
    }
}
