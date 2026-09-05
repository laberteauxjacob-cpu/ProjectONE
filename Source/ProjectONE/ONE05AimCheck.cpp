#include "ONE05AimCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEWeaponComponent.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "ONEAim.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

namespace ONE05AimFixture
{
    EONEWeaponFamily Family(int32 V)
    { return V<2 ? EONEWeaponFamily::Pistol : V<4 ? EONEWeaponFamily::Carbine : EONEWeaponFamily::Shotgun; }
}
AONE05AimCheck::AONE05AimCheck()
{ PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void AONE05AimCheck::BeginPlay()
{
    Super::BeginPlay(); Started=FPlatformTime::Seconds(); StageAt=GetWorld()->GetTimeSeconds();
    bProjectedAim=FParse::Param(FCommandLine::Get(),TEXT("ONE05ProjectedAim"));
    FParse::Value(FCommandLine::Get(),TEXT("ONE05AimVariants="),VariantLimit);
    FParse::Value(FCommandLine::Get(),TEXT("ONE05AimHeadings="),HeadingLimit);
    VariantLimit=FMath::Clamp(VariantLimit,1,6); HeadingLimit=FMath::Clamp(HeadingLimit,1,8);
    Report=TEXT("Actual arena collision probe with declared targets, cover, aim overrides and production controller dispatch. Rendering and native OS input are separate review gates.\n");
    Report+=bProjectedAim ? TEXT("Aim mode: actual projected viewport cursor through production mouse picking; no aim override.\n") : TEXT("Aim mode: explicit world-point override.\n");
    Report+=FString::Printf(TEXT("Declared fixture scope: %d variants, %d headings, 10 cases; expected discharges %d.\n"),VariantLimit,HeadingLimit,VariantLimit*HeadingLimit*10);
    Csv=TEXT("variant,trial,heading,case,shots,victims,obstructed,tracers,intent_dot,ray_x,ray_y,ray_z,muzzle_x,muzzle_y,muzzle_z,pose_frame,shot_frame,contact_pellets,aim_x,aim_y,aim_z,intent_x,intent_y,fixture_dot\n");
}
void AONE05AimCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    const FString Line=FString::Printf(TEXT("ONE05_AIM %s | variant=%d trial=%d %s"),Pass?TEXT("PASS"):TEXT("FAIL"),Variant,Trial,*Label);
    Report+=Line+TEXT("\n"); UE_LOG(LogTemp,Display,TEXT("%s"),*Line);
}
void AONE05AimCheck::Next(int32 Value) { Stage=Value; StageAt=GetWorld()->GetTimeSeconds(); }
void AONE05AimCheck::Key(bool Pressed)
{
    if (auto* PC=Player ? Cast<AONEPlayerController>(Player->GetController()) : nullptr)
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,Pressed?IE_Pressed:IE_Released,Pressed?1.f:0.f));
}
void AONE05AimCheck::AimAt(const FVector& Point)
{
    if (!bProjectedAim) { Player->SetAimOverride(true,Point); return; }
    Player->SetAimOverride(false,Point);
    FVector2D Screen;
    if (auto* PC=Cast<AONEPlayerController>(Player->GetController()); PC && PC->ProjectWorldLocationToScreen(Point,Screen))
        PC->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
    else Check(false,TEXT("World aim fixture projects into an actual rendered viewport"));
}
void AONE05AimCheck::ClearScene()
{
    if (IsValid(Target)) Target->Destroy(); Target=nullptr;
    if (IsValid(Cover)) Cover->Destroy(); Cover=nullptr;
}
void AONE05AimCheck::PrepareVariant()
{
    ClearScene(); Key(false); Player->ReleaseHeldInputs();
    auto* W=Player->GetWeaponComponent(); W->ResetStarterLoadout();
    if (ONE05AimFixture::Family(Variant)!=EONEWeaponFamily::Pistol)
        Check(W->ApplyAcquisitionPlan(W->BuildAcquisitionPlan(ONE05AimFixture::Family(Variant))),TEXT("Explicit scene fixture acquires requested base family"));
    Trial=0; AimAt(Origin+FVector(500,0,42)); Next(1);
}
AONEZombie* AONE05AimCheck::MakeTarget(const FVector& BodyCenter)
{
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Z=GetWorld()->SpawnActor<AONEZombie>(Origin+Intent*150,(-Intent).Rotation(),Spawn);
    if (!Z) return nullptr;
    Z->AttackDamage=0; Z->Health->MaxHealth=100000.f; Z->Health->Restore();
    Z->HeadSeverThreshold=Z->ArmSeverThreshold=Z->LegSeverThreshold=100000.f;
    Z->SetActorTickEnabled(false); Z->GetCharacterMovement()->DisableMovement();
    if (auto* AI=Cast<AAIController>(Z->GetController())) AI->StopMovement();
    Z->GetMesh()->TickAnimation(0.f,false); Z->GetMesh()->RefreshBoneTransforms();
    Z->GetMesh()->bPauseAnims=true; Z->GetMesh()->SetComponentTickEnabled(false);
    const FVector BodyOffset=Z->BodyRegion->GetComponentLocation()-Z->GetActorLocation();
    // Keep the actor's ordinary standing height. Raising the torso to the
    // player's shoulder would conceal real close-range vertical aim errors.
    Z->SetActorLocation(FVector(BodyCenter.X-BodyOffset.X,BodyCenter.Y-BodyOffset.Y,Origin.Z));
    return Z;
}
AActor* AONE05AimCheck::MakeCover(const FVector& Center,float Yaw)
{
    AActor* A=GetWorld()->SpawnActor<AActor>(); if (!A) return nullptr;
    auto* B=NewObject<UBoxComponent>(A); A->SetRootComponent(B); A->AddInstanceComponent(B);
    B->SetMobility(EComponentMobility::Movable); B->SetBoxExtent(FVector(8,60,100));
    B->SetCollisionObjectType(ECC_WorldStatic); B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    B->SetCollisionResponseToAllChannels(ECR_Block); B->SetCanEverAffectNavigation(false);
    // Ray-only fixture: do not depenetrate the player's capsule while testing
    // the real shoulder/muzzle visibility traces against this box.
    B->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    B->RegisterComponent(); A->SetActorLocation(Center); A->SetActorRotation(FRotator(0,Yaw,0)); return A;
}
void AONE05AimCheck::PrepareTrial()
{
    ClearScene(); Key(false); Player->ReleaseHeldInputs();
    // Ordinary arena, real collision regions and actual evaluated player mesh.
    // No new map is used. This fixed scene probe is not a player-input recording.
    Player->GetCharacterMovement()->StopMovementImmediately(); Player->SetActorLocation(Origin);
    Intent=FRotator(0,(Trial/10)*45.f,0).Vector();
    AimAt(Player->GetAimOrigin()+Intent*600);
    Next(3);
}
void AONE05AimCheck::Finish()
{
    if (bFinished) return;
    Key(false); ClearScene(); bFinished=true; Ended=FPlatformTime::Seconds();
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate05/AimCheck")/FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"));
    IFileManager::Get().MakeDirectory(*Folder,true);
    Report+=FString::Printf(TEXT("ONE05_AIM_COMPLETE failures=%d checks=%d\n"),Failures,Checks);
    FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    FFileHelper::SaveStringToFile(Csv,*(Folder/TEXT("rays.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UE_LOG(LogTemp,Display,TEXT("ONE05_AIM_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
void AONE05AimCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    if (bFinished) { if (FPlatformTime::Seconds()-Ended>.3) FPlatformMisc::RequestExit(false); return; }
    if (FPlatformTime::Seconds()-Started>420) { Check(false,TEXT("Bounded aim scene timeout")); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player || !Player->GetController()) return;
    auto* W=Player->GetWeaponComponent();
    const float T=GetWorld()->GetTimeSeconds()-StageAt;
    switch (Stage)
    {
    case 0: if (T>.8f)
    {
        const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
        Check(GM && GM->IsSandbox(),TEXT("Aim fixture uses explicit sandbox and no waves"));
        Origin=Player->GetActorLocation(); PrepareVariant();
    } break;
    case 1: if (W->CanFire() && T>.2f)
    {
        if (Variant%2)
        {
            FONEWeaponReservation Token;
            Check(W->ReserveEquippedForUpgrade(Token) && W->MarkUpgradeReady(Token) && W->CollectUpgrade(Token),
                TEXT("Declared upgraded scene fixture preserves same-instance inventory path"));
        }
        Next(2);
    } break;
    case 2: if (W->CanFire() && T>.2f)
    {
        Check(W->GetDefinition().Family==ONE05AimFixture::Family(Variant) && W->GetDefinition().bUpgraded==bool(Variant%2),TEXT("Actual equipped effective variant matches trial"));
        Check(Player->IsHeldAuraVisible()==bool(Variant%2),TEXT("Held upgrade aura follows effective variant without base leakage"));
        PrepareTrial();
    } break;
    case 3: if (T>.15f && W->CanFire())
    {
        const int32 Case=Trial%10;
        const FVector Shoulder=Player->GetAimOrigin(),Muzzle=Player->GetMuzzleLocation();
        const double Barrel=FVector::DotProduct(Muzzle-Shoulder,Intent);
        FVector Cursor=Shoulder+Intent*600;
        if (Case==0) Cursor=Shoulder;
        if (Case==1) Cursor=Shoulder+Intent*8;
        if (Case==2) Cursor=Shoulder+Intent*(Barrel+2);
        if (Case==3)
        {
            Target=MakeTarget(Shoulder+Intent*60);
            Check(IsValid(Target),TEXT("Real close-body query target spawned at60cm from player"));
            Cursor=IsValid(Target) ? Target->BodyRegion->GetComponentLocation() : Shoulder+Intent*60;
        }
        if (Case==4)
        {
            Target=MakeTarget(Shoulder+Intent*450);
            Check(IsValid(Target),TEXT("Distant regional target spawned"));
            Cursor=IsValid(Target) ? Target->BodyRegion->GetComponentLocation() : Shoulder+Intent*450;
        }
        if (Case==5 || Case==6)
        {
            Target=MakeTarget(Shoulder+Intent*300);
            Cover=MakeCover(Case==5 ? Muzzle+Intent*40 : FMath::Lerp(Shoulder,Muzzle,.55f),Intent.Rotation().Yaw);
            Check(IsValid(Target) && IsValid(Cover),TEXT("Real cover blocks the target at specified muzzle side"));
            Cursor=Shoulder+Intent*300;
        }
        if (Case==7)
        {
            // Deliberate fast reversal, not a restriction on world direction.
            Intent=-Intent; Cursor=Shoulder+Intent*12;
        }
        if (Case==8 || Case==9)
        {
            // Keep the low-cover target beyond every barrel so its upper body
            // cannot itself win the authoritative physical obstruction trace.
            Target=MakeTarget(Shoulder+Intent*(Case==8 ? 150 : Barrel+25));
            Check(IsValid(Target),TEXT("Real-height target exercises contact-prefix cover or forward barrel edge"));
            Cursor=IsValid(Target) ? Target->BodyRegion->GetComponentLocation() : Shoulder+Intent*60;
            if (Case==8)
            {
                Cover=MakeCover(FMath::Lerp(Shoulder,Cursor,.5f),Intent.Rotation().Yaw);
                if (IsValid(Cover)) CastChecked<UBoxComponent>(Cover->GetRootComponent())->SetBoxExtent(FVector(5,25,3));
            }
        }
        TargetHealth=IsValid(Target) ? Target->GetHealth() : 0;
        AimAt(Cursor); Shots=W->GetTotalShotsFired(); Next(4);
    } break;
    case 4: if (T>.055f && W->CanFire())
    {
        if (Trial%10==8 && IsValid(Cover))
        {
            // The projected cursor can select a different anatomical region
            // than the requested body center. Put this declared small cover
            // fixture on the actual contact direction, before the barrel plane.
            const FVector Shoulder=Player->GetAimOrigin();
            const FVector Contact=ONEAim::ResolveShotDirection(Shoulder,Player->GetAimPoint(),Player->GetIntendedAimDirection(),Shoulder,
                Player->AimConvergenceAhead,Player->AimMaximumPitch);
            Cover->SetActorLocation(Shoulder+Contact*25);
            Cover->SetActorRotation(Player->GetIntendedAimDirection().Rotation());
        }
        Key(true); Next(5);
    } break;
    case 5: if (W->GetTotalShotsFired()>Shots)
    {
        Key(false);
        const FVector Ray=W->GetLastShotDirection(),Muzzle=W->GetLastShotMuzzle();
        const FVector ActualIntent=Player->GetIntendedAimDirection(),Aim=Player->GetAimPoint();
        const double Dot=FVector::DotProduct(Ray,ActualIntent);
        Check(W->GetTotalShotsFired()==Shots+1,TEXT("Production controller press produces one scene-query discharge"));
        Check(FVector::Dist2D(Player->GetActorLocation(),Origin)<.5,TEXT("Ray fixture preserves the declared player origin without cover depenetration"));
        Check(!Ray.ContainsNaN() && FMath::Abs(Ray.Size()-1.0)<.001 && Dot>.5,TEXT("Evaluated shot remains finite and agrees with character-centred cursor intent"));
        Check(W->GetLastShotPoseFrame()==uint32(W->GetLastShotFrame()),TEXT("Discharge uses current-frame evaluated skeletal muzzle"));
        const int32 Case=Trial%10;
        if (Case==3 || Case==4 || Case==9) Check(IsValid(Target) && Target->GetHealth()<TargetHealth && W->GetLastShotVictimCount()==1,
            TEXT("Close or distant target receives actual regional damage without a minimum-range immunity ring"));
        if (Case==5 || Case==6 || Case==8) Check(IsValid(Target) && Target->GetHealth()==TargetHealth && W->GetLastShotVictimCount()==0,
            TEXT("World cover prevents target damage including shoulder-to-muzzle obstruction"));
        if (Case==6) Check(W->WasLastShotMuzzleObstructed() && W->GetLastShotForwardTracerCount()==0,
            TEXT("Behind-muzzle obstruction resolves collision without a backward muzzle tracer"));
        if (Case==8 && !bProjectedAim) Check(!W->WasLastShotMuzzleObstructed() && W->GetLastShotContactPelletCount()==W->GetDefinition().Pellets && W->GetLastShotForwardTracerCount()==0,
            TEXT("Low cover stops every contact pellet while the physical muzzle clears it; no backward tracer"));
        Csv+=FString::Printf(TEXT("%d,%d,%d,%d,%d,%d,%d,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%u,%llu,%d,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f\n"),
            Variant,Trial,(Trial/10)*45,Case,W->GetTotalShotsFired(),W->GetLastShotVictimCount(),W->WasLastShotMuzzleObstructed(),
            W->GetLastShotForwardTracerCount(),Dot,Ray.X,Ray.Y,Ray.Z,Muzzle.X,Muzzle.Y,Muzzle.Z,W->GetLastShotPoseFrame(),W->GetLastShotFrame(),
            W->GetLastShotContactPelletCount(),Aim.X,Aim.Y,Aim.Z,ActualIntent.X,ActualIntent.Y,FVector::DotProduct(Ray,Intent));
        Next(6);
    }
    else if (T>3) { Check(false,TEXT("Eligible production press did not discharge")); Finish(); }
    break;
    case 6: if (!W->IsBusy() && !W->NeedsPump(W->GetEquippedIndex()) && T>.12f)
    {
        ClearScene(); W->RefillAllAmmo();
        if (++Trial<HeadingLimit*10) PrepareTrial();
        else if (++Variant<VariantLimit) PrepareVariant();
        else Finish();
    } break;
    }
}
