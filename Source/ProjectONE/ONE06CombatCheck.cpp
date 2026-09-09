#include "ONE06CombatCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONE06Ballistics.h"
#include "ONE06ImpactSubsystem.h"
#include "ONE06CaptureComponent.h"
#include "ONEBloodSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
    EONEWeaponFamily Family(int32 Variant)
    { return Variant<2?EONEWeaponFamily::Pistol:Variant<4?EONEWeaponFamily::Carbine:EONEWeaponFamily::Shotgun; }
}
AONE06CombatCheck::AONE06CombatCheck()
{ PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void AONE06CombatCheck::BeginPlay()
{
    Super::BeginPlay(); Started=FPlatformTime::Seconds(); PhaseAt=GetWorld()->GetTimeSeconds();
    FParse::Value(FCommandLine::Get(),TEXT("ONE06CombatVariants="),VariantLimit); VariantLimit=FMath::Clamp(VariantLimit,1,6);
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate06/CombatCheck")/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    IFileManager::Get().MakeDirectory(*Folder,true);
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE06Capture")))
    {
        Capture=NewObject<UONE06CaptureComponent>(this); AddInstanceComponent(Capture); Capture->RegisterComponent();
        Check(Capture->BeginCapture(FPaths::ConvertRelativePathToFull(Folder/TEXT("Media"))),TEXT("Opt-in viewport and actual master-mix capture starts"));
    }
    Report+=TEXT("Explicit fixed standing targets and cover; actual logical cursor projection, controller input, normal weapon discharges and real collision. Per-variant pose is held during scene-invariance trials. Modifiers never fire. No aim override. This is not native input. Media exists only when optional capture completes.\n");
    Csv=TEXT("variant,trial,projectile,origin_x,origin_y,origin_z,ray_x,ray_y,ray_z,end_x,end_y,end_z,spread,queries,contact,distance,damage,region,corpse,contact_x,contact_y,contact_z\n");
}
void AONE06CombatCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    const FString Line=FString::Printf(TEXT("ONE06_COMBAT %s | variant=%d trial=%d %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Variant,Trial,*Label);
    Report+=Line+TEXT("\n"); UE_LOG(LogTemp,Display,TEXT("%s"),*Line);
}
void AONE06CombatCheck::Enter(int32 Next) { Phase=Next; PhaseAt=GetWorld()->GetTimeSeconds(); }
void AONE06CombatCheck::Key(const FKey& K,bool Down)
{ if (Controller) Controller->InputKey(FInputKeyEventArgs::CreateSimulated(K,Down?IE_Pressed:IE_Released,Down?1.f:0.f)); }
void AONE06CombatCheck::Release()
{
    for (const FKey& K:{EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::LeftControl}) Key(K,false);
    if (Player) Player->ReleaseHeldInputs();
}
void AONE06CombatCheck::Aim(float Distance,float Height,float Side)
{
    Player->SetAimOverride(false,FVector::ZeroVector);
    FVector2D Screen;
    const FVector Point=Origin+FVector(Distance,Side,Height-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const bool Projected=Player->ProjectLogicalWorld(Point,Screen);
    if (!Projected) Check(false,TEXT("Logical cursor projection is available"));
    else Controller->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
}
void AONE06CombatCheck::ClearScene()
{
    for (AActor* A:Fixtures) if (IsValid(A)) A->Destroy(); Fixtures.Reset();
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->ClearPresentation();
}
AONEZombie* AONE06CombatCheck::Target(float Distance,float Side,float Health)
{
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const float Floor=Origin.Z-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    auto* Z=GetWorld()->SpawnActor<AONEZombie>(FVector(Origin.X+Distance,Origin.Y+Side,Floor+88.f),FRotator(0,180,0),Params);
    if (!Z) { Check(false,TEXT("Standing collision target spawned")); return nullptr; }
    Fixtures.Add(Z); Mode->RegisterZombie(Z); Z->AttackDamage=0;
    Z->Health->MaxHealth=Health; Z->Health->Restore();
    // High-health grouping fixtures also suppress structural death. Normal112HP
    // aiming trials below retain production sever thresholds.
    if (Health>1000.f) Z->HeadSeverThreshold=Z->ArmSeverThreshold=Z->LegSeverThreshold=100000.f;
    Z->SetActorTickEnabled(false); Z->GetCharacterMovement()->DisableMovement();
    if (auto* AI=Cast<AAIController>(Z->GetController())) AI->StopMovement();
    Z->GetMesh()->TickAnimation(0.f,false); Z->GetMesh()->RefreshBoneTransforms(); Z->GetMesh()->bPauseAnims=true;
    return Z;
}
AActor* AONE06CombatCheck::Wall(const FVector& Position,const FVector& Extent)
{
    auto* A=GetWorld()->SpawnActor<AActor>(); if (!A) return nullptr;
    auto* Box=NewObject<UBoxComponent>(A); A->SetRootComponent(Box); A->AddInstanceComponent(Box);
    Box->SetMobility(EComponentMobility::Movable); Box->SetBoxExtent(Extent);
    Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Box->SetCanEverAffectNavigation(false); Box->RegisterComponent(); A->SetActorLocation(Position); Fixtures.Add(A); return A;
}
void AONE06CombatCheck::PrepareVariant()
{
    Release(); ClearScene(); Trial=0; Baseline.Reset();
    Player->GetMesh()->bPauseAnims=false;
    auto* W=Player->GetWeaponComponent(); W->ResetStarterLoadout();
    if (Family(Variant)!=EONEWeaponFamily::Pistol)
        Check(W->ApplyAcquisitionPlan(W->BuildAcquisitionPlan(Family(Variant))),TEXT("Declared fixture installs requested base family"));
    Aim(650,Player->TorsoAimHeight); Enter(10);
}
void AONE06CombatCheck::PrepareTrial()
{
    Release(); ClearScene(); auto* W=Player->GetWeaponComponent(); W->RefillAllAmmo(); W->ResetSpreadStream(6206);
    if (auto* Shake=IConsoleManager::Get().FindConsoleVariable(TEXT("one.CameraShake.Strength"))) Shake->Set(Trial==5?0.f:1.f);
    // A low shot between the standing legs is an intentional miss. This fixed
    // fixture cursor addresses one leg at its declared 9cm lateral offset.
    const bool HeadTrial=Trial==7 || Trial==11;
    Aim(HeadTrial || Trial==8?250:Trial==9?60:650,HeadTrial?Player->HeadAimHeight:Trial==8?Player->LowAimHeight:Player->TorsoAimHeight,Trial==8?9.f:0.f);
    if (Trial==1) Target(250,85,100000);
    if (Trial==2) Target(250,0,100000);
    if (Trial==3 || Trial==6) for (int32 I=0;I<5;++I) Target(150+I*90,0,Trial==6?1.f:100000.f);
    if (Trial==4) { Target(400,0,100000); Wall(Origin+FVector(200,0,30),FVector(6,100,110)); }
    if (Trial==7 || Trial==8) Target(250,0,112);
    if (Trial==9) Target(60,0,112);
    if (Trial==10) { Target(300,0,112); Wall(FMath::Lerp(Player->GetAimOrigin(),Player->GetMuzzleLocation(),.55f),FVector(4,35,70)); }
    if (Trial==11) Target(250,0,25); // Explicit wounded target for an actual lethal head shot and HUD award.
    if (Trial==12) { Target(140,-6,112); Target(220,0,112); Target(300,6,112); }
    ShotsBefore=W->GetTotalShotsFired(); PointsBefore=Mode->GetPoints(); Enter(12);
}
void AONE06CombatCheck::ReviewShot()
{
    auto* W=Player->GetWeaponComponent(); const auto& Paths=W->GetLastProjectilePaths();
    UE_LOG(LogTemp,Display,TEXT("ONE06_COMBAT_AIM variant=%d trial=%d height=%s value=%.2f aim=%s muzzle=%s"),
        Variant,Trial,*Player->GetAimHeightLabel(),Player->GetAimHeightCm(),*Player->GetAimPoint().ToString(),*W->GetLastShotMuzzle().ToString());
    if (Trial==7 || Trial==8) for (AActor* A:Fixtures) if (auto* Z=Cast<AONEZombie>(A))
        UE_LOG(LogTemp,Display,TEXT("ONE06_COMBAT_REGIONS head=%s left_leg=%s right_leg=%s body=%s"),
            *Z->HeadRegion->GetComponentLocation().ToString(),*Z->UpperLegLeftRegion->GetComponentLocation().ToString(),
            *Z->UpperLegRightRegion->GetComponentLocation().ToString(),*Z->BodyRegion->GetComponentLocation().ToString());
    Check(W->GetTotalShotsFired()==ShotsBefore+1,TEXT("One LMB press commits exactly one discharge"));
    Check(Paths.Num()==W->GetDefinition().Pellets,TEXT("One independently sampled path per actual projectile"));
    int32 MaxBodies=0; bool AnyHead=false,AnyLeg=false; int32 ExpectedPoints=0;
    TSet<AONEZombie*> UniqueVictims;
    for (int32 I=0;I<Paths.Num();++I)
    {
        const auto& P=Paths[I]; TSet<AActor*> Seen; float PriorDistance=-1.f,PriorDamage=MAX_flt; int32 Living=0;
        Check(!P.Direction.ContainsNaN() && FMath::Abs(P.Direction.Size()-1.)<.0001 && P.SceneQueries<=ONE06Ballistics::MaximumContactsPerProjectile,
            TEXT("Finite unit direction and bounded contact-query count"));
        Check(FVector::CrossProduct(P.End-P.Origin,P.Direction).Size()<.1,TEXT("Projectile endpoint remains on its immutable sampled line"));
        for (int32 C=0;C<P.Contacts.Num();++C)
        {
            const auto& Hit=P.Contacts[C];
            Check(!Seen.Contains(Hit.Victim.Get()) && Hit.Distance>=PriorDistance && Hit.Distance<=W->GetDefinition().Range+.01f,
                TEXT("Distinct ordered victims share the original range budget"));
            Check(FVector::CrossProduct(Hit.Position-P.Origin,P.Direction).Size()<.1,TEXT("Accepted regional contact lies on the sampled line"));
            if (!Hit.bCorpse) { ++Living; Check(Hit.Damage<=PriorDamage,TEXT("Penetration and distance never restore raw damage")); PriorDamage=Hit.Damage; }
            Seen.Add(Hit.Victim.Get()); PriorDistance=Hit.Distance;
            if (auto* Z=Cast<AONEZombie>(Hit.Victim.Get())) if (!Hit.bCorpse) UniqueVictims.Add(Z);
            AnyHead|=Hit.Region==EONEHitRegion::Head; AnyLeg|=Hit.Region==EONEHitRegion::LegLeft || Hit.Region==EONEHitRegion::LegRight;
            Csv+=FString::Printf(TEXT("%d,%d,%d,%.6f,%.6f,%.6f,%.9f,%.9f,%.9f,%.6f,%.6f,%.6f,%.6f,%d,%d,%.6f,%.6f,%d,%d,%.6f,%.6f,%.6f\n"),
                Variant,Trial,I,P.Origin.X,P.Origin.Y,P.Origin.Z,P.Direction.X,P.Direction.Y,P.Direction.Z,P.End.X,P.End.Y,P.End.Z,P.SpreadDegrees,P.SceneQueries,C,Hit.Distance,Hit.Damage,int32(Hit.Region),Hit.bCorpse,Hit.Position.X,Hit.Position.Y,Hit.Position.Z);
        }
        if (P.Contacts.IsEmpty()) Csv+=FString::Printf(TEXT("%d,%d,%d,%.6f,%.6f,%.6f,%.9f,%.9f,%.9f,%.6f,%.6f,%.6f,%.6f,%d,-1,0,0,-1,0,0,0,0\n"),
            Variant,Trial,I,P.Origin.X,P.Origin.Y,P.Origin.Z,P.Direction.X,P.Direction.Y,P.Direction.Z,P.End.X,P.End.Y,P.End.Z,P.SpreadDegrees,P.SceneQueries);
        Check(Living<=W->GetDefinition().Penetration.MaximumBodies,TEXT("Each projectile obeys this weapon's total-body limit")); MaxBodies=FMath::Max(MaxBodies,Living);
    }
    for (auto* Z:UniqueVictims) ExpectedPoints+=10+(Z->IsDead()?(Z->WasLastKillHeadshot()?120:100):0);
    Check(Mode->GetPoints()-PointsBefore==ExpectedPoints,TEXT("Actual trace awards exactly one impact plus eligible kill per distinct victim"));
    if (Trial==0) { Baseline=Paths; BaselineMuzzle=W->GetLastShotMuzzle(); BaselineAim=Player->GetAimPoint(); }
    if (Trial>=1 && Trial<=5)
    {
        const double RenderShake=FVector::Dist(Controller->PlayerCameraManager->GetCameraLocation(),Player->Camera->GetComponentLocation());
        Check(Trial==5?RenderShake<.001:RenderShake>.005,
            TEXT("Direction comparison includes a measured active render shake or its explicit Off setting"));
        Check(Player->GetAimPoint().Equals(BaselineAim,.02),TEXT("Fixed cursor logical aim is unchanged by enemies, wall or disabled shake"));
        Check(W->GetLastShotMuzzle().Equals(BaselineMuzzle,.02),TEXT("Scene-invariance fixture preserved the same physical muzzle pose"));
        for (int32 I=0;I<Paths.Num() && I<Baseline.Num();++I)
            Check(Paths[I].Direction.Equals(Baseline[I].Direction,.000002),TEXT("Same seed, cursor, pose and weapon state retain exact sampled direction across scene changes"));
    }
    if (Trial==0 || Trial==1 || Trial==4 || Trial==5 || Trial==10)
        Check(UniqueVictims.IsEmpty(),TEXT("Empty space, deliberate miss and intervening cover award no victim hit"));
    if (Trial==2 || Trial==9) Check(!UniqueVictims.IsEmpty(),TEXT("Normal torso aiming reaches an actual standing target"));
    if (Trial==3) Check(MaxBodies>=2,TEXT("At least one actual projectile damages multiple lined-up bodies"));
    if (Trial==6) Check(W->GetLastShotNewKillCount()>=2,TEXT("Actual penetration can produce multiple kills without area damage"));
    if (Trial==7)
    {
        Check(AnyHead,TEXT("Held RMB with LMB reaches the standing head region"));
        if (Family(Variant)!=EONEWeaponFamily::Shotgun)
            Check(W->GetLastShotNewKillCount()==0,TEXT("Single base or upgraded pistol/rifle head impact does not execute a fresh112HP target"));
    }
    if (Trial==8) Check(AnyLeg,TEXT("Held Left Ctrl with LMB reaches a standing leg region"));
    if (Trial==11)
    {
        Check(AnyHead && W->GetLastShotNewKillCount()==1,TEXT("An actual head-height discharge kills the declared wounded target"));
        if (Family(Variant)!=EONEWeaponFamily::Shotgun)
            Check(Mode->GetPoints()-PointsBefore==130,TEXT("Actual lethal head bullet awards 10 impact plus 120 headshot points"));
    }
    if (Trial==12)
    {
        Check(UniqueVictims.Num()>=2,TEXT("One discharge damages multiple nearby normal112HP infected through actual paths"));
        for (AActor* A:Fixtures) if (const auto* Z=Cast<AONEZombie>(A))
        {
            const FString Outcome=FString::Printf(TEXT("NORMAL_CROWD variant=%d health_before=112 health_after=%.3f dead=%d"),Variant,Z->GetHealth(),Z->IsDead());
            Report+=Outcome+TEXT("\n"); UE_LOG(LogTemp,Display,TEXT("%s"),*Outcome);
        }
    }
    if (Trial==10) Check(W->WasLastShotMuzzleObstructed() && W->GetLastShotForwardTracerCount()==0,TEXT("Solid shoulder-to-muzzle cover blocks firing through geometry"));
    if (Trial==4) if (auto* Marks=GetWorld()->GetSubsystem<UONE06ImpactSubsystem>()) Check(Marks->GetMarkCount()>0,TEXT("Actual solid contacts create bounded visible impact instances"));
}
void AONE06CombatCheck::Finish()
{
    if (bFinished) return;
    if (!bFinishing) { bFinishing=true; Release(); }
    if (Capture)
    {
        if (Capture->HasFailed()) Check(false,TEXT("Actual capture failed: ")+Capture->GetFailureReason());
        else if (!Capture->FinishCaptureAndWait()) return;
    }
    ClearScene(); if (Player) Player->GetMesh()->bPauseAnims=false;
    if (auto* Shake=IConsoleManager::Get().FindConsoleVariable(TEXT("one.CameraShake.Strength"))) Shake->Set(1.f);
    bFinished=true; FinishedAt=FPlatformTime::Seconds();
    Report+=FString::Printf(TEXT("ONE06_COMBAT_COMPLETE failures=%d checks=%d\n"),Failures,Checks);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(Csv,*(Folder/TEXT("rays.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("ONE06_COMBAT_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
void AONE06CombatCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished) { if (FPlatformTime::Seconds()-FinishedAt>.3) FPlatformMisc::RequestExit(false); return; }
    if (bFinishing) { Finish(); return; }
    if (Capture) Capture->SetPhaseLabel(FString::Printf(TEXT("variant %d / case %d / phase %d / explicit collision fixture"),Variant,Trial,Phase));
    if (FPlatformTime::Seconds()-Started>180.) { Check(false,TEXT("Bounded180-second combat fixture timeout")); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player) return;
    if (!Controller) Controller=Cast<AONEPlayerController>(Player->GetController());
    if (!Mode) Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Controller || !Mode) return;
    auto* W=Player->GetWeaponComponent(); const float T=GetWorld()->GetTimeSeconds()-PhaseAt;
    switch (Phase)
    {
    case 0: if (T>1.f)
    {
        Check(Mode->IsSandbox(),TEXT("Explicit combat fixture suppresses ordinary waves")); Origin=Player->GetActorLocation();
        ShotsBefore=W->GetTotalShotsFired(); Key(EKeys::RightMouseButton,true); Enter(1);
    } break;
    case 1: if (T>.3f)
    {
        Check(W->GetTotalShotsFired()==ShotsBefore && Player->GetAimHeightLabel()==TEXT("Head"),TEXT("RMB alone selects head height without firing"));
        Key(EKeys::LeftControl,true); Enter(2);
    } break;
    case 2: if (T>.3f)
    {
        Check(W->GetTotalShotsFired()==ShotsBefore && Player->GetAimHeightLabel()==TEXT("Low"),TEXT("Both modifiers select low height without firing"));
        Key(EKeys::RightMouseButton,false); Enter(3);
    } break;
    case 3: if (T>.3f)
    {
        Check(W->GetTotalShotsFired()==ShotsBefore && Player->GetAimHeightLabel()==TEXT("Low"),TEXT("Left Ctrl alone selects low height without firing"));
        Release(); Enter(4);
    } break;
    case 4: if (T>.3f)
    {
        Check(W->GetTotalShotsFired()==ShotsBefore && Player->GetAimHeightLabel()==TEXT("Torso"),TEXT("Released modifiers return to torso without a queued shot")); PrepareVariant();
    } break;
    case 10: if (T>.8f && W->CanFire())
    {
        if (Variant%2) { FONEWeaponReservation Token; Check(W->ReserveEquippedForUpgrade(Token) && W->MarkUpgradeReady(Token) && W->CollectUpgrade(Token),TEXT("Declared fixture upgrades the same inventory instance")); }
        Enter(11);
    } break;
    case 11: if (T>1.f && W->CanFire())
    { Player->GetMesh()->bPauseAnims=true; PrepareTrial(); } break;
    case 12: if (T>.15f)
    {
        // Cursor warps can dispatch an OS modifier snapshot. Establish the
        // declared synthetic height hold after that cursor event has settled.
        if (Trial==7 || Trial==11) Key(EKeys::RightMouseButton,true);
        if (Trial==8) Key(EKeys::LeftControl,true);
        Enter(15);
    } break;
    case 15: if (T>.25f && W->CanFire())
    {
        Check(Player->GetAimHeightLabel()==(Trial==7 || Trial==11?TEXT("Head"):Trial==8?TEXT("Low"):TEXT("Torso")),TEXT("Selected height is still held before LMB dispatch"));
        if (Trial>=1 && Trial<=5) Player->AddPresentationShake(1.8f);
        Key(EKeys::LeftMouseButton,true); Enter(13);
    } break;
    case 13: if (W->GetTotalShotsFired()>ShotsBefore)
    { Key(EKeys::LeftMouseButton,false); ReviewShot(); Enter(14); }
    else if (T>2.f) { Check(false,TEXT("LMB with selected height commits a shot within two seconds")); Finish(); } break;
    case 14: if (T>.25f && W->CanFire())
    { if (++Trial<13) PrepareTrial(); else if (++Variant<VariantLimit) PrepareVariant(); else Finish(); } break;
    }
}
