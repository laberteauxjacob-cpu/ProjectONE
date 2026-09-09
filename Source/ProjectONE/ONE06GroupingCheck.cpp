#include "ONE06GroupingCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONE06CaptureComponent.h"
#include "ONE06ImpactSubsystem.h"
#include "ONEBloodSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "AIController.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DateTime.h"
#include "Misc/CommandLine.h"

namespace ONE06GroupingDetails
{
    FString Number(double Value) { return FString::Printf(TEXT("%.9f"),Value); }
    void VectorCells(TArray<FString>& Cells,const FVector& V)
    { Cells.Add(Number(V.X)); Cells.Add(Number(V.Y)); Cells.Add(Number(V.Z)); }
}
AONE06GroupingCheck::AONE06GroupingCheck()
{ PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
float AONE06GroupingCheck::Distance() const
{ const float Distances[]={300.f,650.f,1000.f}; return Distances[FMath::Clamp(DistanceIndex,0,2)]; }
FString AONE06GroupingCheck::PatternName() const
{ return Pattern==0?TEXT("M4A1_short_3"):Pattern==1?TEXT("M4A1_sustained_12"):TEXT("870_three_shells"); }
void AONE06GroupingCheck::BeginPlay()
{
    Super::BeginPlay(); StartedReal=FPlatformTime::Seconds(); PhaseAt=GetWorld()->GetTimeSeconds();
    Folder=FPaths::ProjectSavedDir()/TEXT("Candidate06/GroupingCheck")/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8));
    IFileManager::Get().MakeDirectory(*Folder,true);
    ChecksCsv=TEXT("index,world_seconds,pattern,distance_cm,scene,pass,label\n");
    RaysCsv=TEXT("pattern,weapon,distance_cm,scene,shot,pellet,row_kind,engine_frame,shot_id,world_seconds,shot_world_seconds,spread_degrees,bloom_after_shot,origin_x,origin_y,origin_z,direction_x,direction_y,direction_z,end_x,end_y,end_z,muzzle_x,muzzle_y,muzzle_z,aim_x,aim_y,aim_z,wall_hit,contact_index,contact_distance,contact_damage,contact_region,contact_corpse,contact_x,contact_y,contact_z,spread_seed\n");
    GroupsCsv=TEXT("pattern,weapon,distance_cm,scene,shots,projectiles,wall_hits,enemy_contacts,points_delta,first_spread,last_spread,peak_bloom,recovered_bloom,min_wall_y,max_wall_y,min_wall_z,max_wall_z,width_y,height_z,diagonal_span,pair_max_origin_error,pair_max_direction_error,spread_seed\n");
    if (FParse::Param(FCommandLine::Get(),TEXT("ONE06Capture")))
    {
        Capture=NewObject<UONE06CaptureComponent>(this); AddInstanceComponent(Capture); Capture->RegisterComponent();
        Check(Capture->BeginCapture(FPaths::ConvertRelativePathToFull(Folder/TEXT("Media"))),TEXT("Optional viewport and master-mix capture starts"));
    }
}
void AONE06GroupingCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    FString Quoted=Label; Quoted.ReplaceInline(TEXT("\""),TEXT("\"\""));
    ChecksCsv+=FString::Printf(TEXT("%d,%.6f,%s,%.0f,%d,%d,\"%s\"\n"),Checks,GetWorld()->GetTimeSeconds(),*PatternName(),Distance(),Scene,Pass,*Quoted);
    UE_LOG(LogTemp,Display,TEXT("ONE06_GROUPING %s | %s distance=%.0f scene=%d %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*PatternName(),Distance(),Scene,*Label);
}
void AONE06GroupingCheck::Enter(int32 Next)
{
    Phase=Next; PhaseAt=GetWorld()->GetTimeSeconds();
    if (Capture) Capture->SetPhaseLabel(FString::Printf(TEXT("%s / %.0f cm / %s / phase %d"),*PatternName(),Distance(),Scene?TEXT("standing infected"):TEXT("wall only"),Next));
}
void AONE06GroupingCheck::Key(bool Down)
{ if (Controller) Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton,Down?IE_Pressed:IE_Released,Down?1.f:0.f)); }
void AONE06GroupingCheck::ClearScene()
{
    if (IsValid(Target)) Target->Destroy(); Target=nullptr;
    if (IsValid(Wall)) Wall->Destroy(); Wall=nullptr;
    if (auto* Marks=GetWorld()->GetSubsystem<UONE06ImpactSubsystem>()) Marks->Clear();
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->ClearPresentation();
}
bool AONE06GroupingCheck::MakeWall()
{
    Wall=GetWorld()->SpawnActor<AActor>(); if (!Wall) return false;
    auto* Box=NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Box); Wall->AddInstanceComponent(Box);
    Box->SetBoxExtent(FVector(5,240,200)); Box->SetMobility(EComponentMobility::Movable);
    Box->SetCollisionObjectType(ECC_WorldStatic); Box->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Box->SetCollisionResponseToAllChannels(ECR_Ignore); Box->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Box->SetCanEverAffectNavigation(false); Box->RegisterComponent();
    const float Floor=Origin.Z-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Wall->SetActorLocation(FVector(Origin.X+Distance(),Origin.Y,Floor+150.f));
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")); if (!Mesh) return false;
    auto* Surface=NewObject<UStaticMeshComponent>(Wall); Wall->AddInstanceComponent(Surface); Surface->SetupAttachment(Box);
    Surface->SetMobility(EComponentMobility::Movable);
    Surface->SetStaticMesh(Mesh); Surface->SetRelativeScale3D(FVector(.1f,4.8f,4.f));
    Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision); Surface->SetCanEverAffectNavigation(false); Surface->RegisterComponent();
    return true;
}
bool AONE06GroupingCheck::MakeTarget()
{
    const float Floor=Origin.Z-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Target=GetWorld()->SpawnActor<AONEZombie>(FVector(Origin.X+180.f,Origin.Y,Floor+88.f),FRotator(0,180,0),Params);
    if (!Target || !Mode->RegisterZombie(Target)) return false;
    Target->Health->SetMaximumHealth(100000.f); Target->Health->Restore();
    Target->HeadSeverThreshold=Target->ArmSeverThreshold=Target->LegSeverThreshold=100000.f;
    Target->AttackDamage=0; Target->SetActorTickEnabled(false); Target->GetCharacterMovement()->DisableMovement();
    if (auto* AI=Cast<AAIController>(Target->GetController())) AI->StopMovement();
    Target->GetMesh()->TickAnimation(0.f,false); Target->GetMesh()->RefreshBoneTransforms(); Target->GetMesh()->bPauseAnims=true;
    return true;
}
void AONE06GroupingCheck::PreparePattern()
{
    Key(false); Player->ReleaseHeldInputs(); ClearScene(); Baseline.Reset();
    DistanceIndex=0; Scene=0; for (float& Width:Widths) Width=0.f;
    Player->GetMesh()->bPauseAnims=false;
    auto* W=Player->GetWeaponComponent(); W->ResetStarterLoadout();
    const auto Family=Pattern==2?EONEWeaponFamily::Shotgun:EONEWeaponFamily::Carbine;
    const bool Acquired=W->ApplyAcquisitionPlan(W->BuildAcquisitionPlan(Family));
    Check(Acquired,TEXT("Declared fixture acquires the base weapon through the normal owned-slot API"));
    if (!Acquired) { Finish(false); return; }
    Player->SetAimOverride(false,FVector::ZeroVector);
    FVector2D Screen;
    const FVector Point=Origin+FVector(650.f,0,Player->TorsoAimHeight-Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
    const bool Projected=Player->ProjectLogicalWorld(Point,Screen);
    Check(Projected,TEXT("Ordinary logical camera projects a fixed torso-height cursor"));
    if (!Projected) { Finish(false); return; }
    Controller->SetMouseLocation(FMath::RoundToInt(Screen.X),FMath::RoundToInt(Screen.Y));
    Enter(1);
}
void AONE06GroupingCheck::PrepareTrial()
{
    Key(false); Player->ReleaseHeldInputs(); ClearScene();
    auto* W=Player->GetWeaponComponent(); W->RefillAllAmmo(); W->ClearEjectedCases(); W->ResetSpreadStream(606430+Pattern);
    Check(W->GetAmmo()==W->GetDefinition().Capacity && W->GetReserveAmmo()==FMath::Clamp(W->GetDefinition().InitialReserve,0,W->GetDefinition().ReserveLimit) &&
        FMath::IsNearlyEqual(W->GetCurrentSpreadDegrees(),W->GetDefinition().SpreadDegrees,1.e-6f) && Player->GetVelocity().IsNearlyZero(),
        TEXT("Paired setup starts with a full magazine, ordinary initial reserve, zero bloom and a stationary player"));
    if (Scene==0) Baseline.Reset();
    Shot=Projectiles=WallHits=EnemyContacts=0; FirstSpread=LastSpread=PeakBloom=0;
    MinY=MinZ=BIG_NUMBER; MaxY=MaxZ=-BIG_NUMBER; PairOriginError=PairDirectionError=0;
    const bool WallReady=MakeWall(); Check(WallReady,TEXT("Visible solid wall has actual blocking collision at the declared distance"));
    if (!WallReady) { Finish(false); return; }
    if (Scene)
    {
        const bool TargetReady=MakeTarget(); Check(TargetReady,TEXT("Declared high-HP infected stands at normal floor height 180 cm away"));
        if (!TargetReady) { Finish(false); return; }
    }
    SeenShots=TrialShotsBefore=W->GetTotalShotsFired(); PointsBefore=Mode->GetPoints(); Enter(2);
}
void AONE06GroupingCheck::RecordShot()
{
    auto* W=Player->GetWeaponComponent(); const auto& Paths=W->GetLastProjectilePaths();
    Check(W->GetTotalShotsFired()==SeenShots+1,TEXT("Every actual discharge is observed without a skipped counter transition"));
    SeenShots=W->GetTotalShotsFired();
    Check(Paths.Num()==W->GetDefinition().Pellets,TEXT("Recorded projectile count matches the actual weapon definition"));
    const auto* State=W->GetSlotState(W->GetEquippedIndex()); const float Bloom=State?State->SpreadBloom:-1.f;
    const float Spread=Paths.IsEmpty()?-1.f:Paths[0].SpreadDegrees;
    if (Shot==0) FirstSpread=Spread;
    else if (Pattern<2) Check(Spread>LastSpread+.001f,TEXT("Established automatic fire increases actual per-shot spread"));
    LastSpread=Spread; PeakBloom=FMath::Max(PeakBloom,Bloom);
    FRecordedShot Recorded; Recorded.Paths=Paths; Recorded.Muzzle=W->GetLastShotMuzzle(); Recorded.Aim=Player->GetAimPoint();
    if (Scene==0) Baseline.Add(Recorded);
    else
    {
        const bool Available=Baseline.IsValidIndex(Shot) && Baseline[Shot].Paths.Num()==Paths.Num();
        Check(Available,TEXT("Paired empty-scene discharge has every matching projectile"));
        if (Available)
        {
            Check(Recorded.Muzzle.Equals(Baseline[Shot].Muzzle,.02) && Recorded.Aim.Equals(Baseline[Shot].Aim,.02),TEXT("Paired scene preserves physical muzzle pose and ordinary logical cursor aim"));
            for (int32 I=0;I<Paths.Num();++I)
            {
                PairOriginError=FMath::Max(PairOriginError,FVector::Dist(Paths[I].Origin,Baseline[Shot].Paths[I].Origin));
                PairDirectionError=FMath::Max(PairDirectionError,FVector::Dist(Paths[I].Direction,Baseline[Shot].Paths[I].Direction));
                Check(Paths[I].Origin.Equals(Baseline[Shot].Paths[I].Origin,.02) && Paths[I].Direction.Equals(Baseline[Shot].Paths[I].Direction,2.e-6) &&
                    FMath::IsNearlyEqual(Paths[I].SpreadDegrees,Baseline[Shot].Paths[I].SpreadDegrees,1.e-6f),TEXT("Same spread state and seed keep the complete initial ray unchanged by an infected"));
            }
        }
    }
    for (int32 I=0;I<Paths.Num();++I)
    {
        const auto& Path=Paths[I]; ++Projectiles;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE06GroupingSurface),false,Player); if (Target) Params.AddIgnoredActor(Target);
        FHitResult Solid;
        const bool HitWall=GetWorld()->LineTraceSingleByChannel(Solid,Path.Origin,Path.End+Path.Direction*.2,ECC_Visibility,Params) &&
            Solid.GetActor()==Wall && Solid.ImpactPoint.Equals(Path.End,.05);
        Check(HitWall,TEXT("Actual projectile endpoint is on the declared solid wall"));
        if (HitWall)
        { ++WallHits; MinY=FMath::Min(MinY,Path.End.Y); MaxY=FMath::Max(MaxY,Path.End.Y); MinZ=FMath::Min(MinZ,Path.End.Z); MaxZ=FMath::Max(MaxZ,Path.End.Z); }
        TArray<FString> Base={PatternName(),W->GetDefinition().Id.ToString(),ONE06GroupingDetails::Number(Distance()),FString::FromInt(Scene),FString::FromInt(Shot),FString::FromInt(I),TEXT("projectile"),
            FString::Printf(TEXT("%llu"),static_cast<unsigned long long>(W->GetLastShotFrame())),FString::Printf(TEXT("%llu"),static_cast<unsigned long long>(W->GetLastShotId())),
            ONE06GroupingDetails::Number(GetWorld()->GetTimeSeconds()),ONE06GroupingDetails::Number(GetWorld()->GetTimeSeconds()-W->GetTimeSinceShot()),ONE06GroupingDetails::Number(Path.SpreadDegrees),ONE06GroupingDetails::Number(Bloom)};
        ONE06GroupingDetails::VectorCells(Base,Path.Origin); ONE06GroupingDetails::VectorCells(Base,Path.Direction); ONE06GroupingDetails::VectorCells(Base,Path.End); ONE06GroupingDetails::VectorCells(Base,Recorded.Muzzle); ONE06GroupingDetails::VectorCells(Base,Recorded.Aim);
        Base.Add(HitWall?TEXT("1"):TEXT("0"));
        auto Row=Base; Row.Append({TEXT("-1"),TEXT("0"),TEXT("0"),TEXT("-1"),TEXT("0"),TEXT("0"),TEXT("0"),TEXT("0")});
        Row.Add(FString::FromInt(606430+Pattern));
        RaysCsv+=FString::Join(Row,TEXT(","))+TEXT("\n");
        for (int32 C=0;C<Path.Contacts.Num();++C)
        {
            const auto& Contact=Path.Contacts[C];
            if (Contact.Victim.Get()==Target && !Contact.bCorpse) ++EnemyContacts;
            Row=Base; Row[6]=TEXT("contact"); Row.Add(FString::FromInt(C)); Row.Add(ONE06GroupingDetails::Number(Contact.Distance)); Row.Add(ONE06GroupingDetails::Number(Contact.Damage));
            Row.Add(FString::FromInt(int32(Contact.Region))); Row.Add(Contact.bCorpse?TEXT("1"):TEXT("0")); ONE06GroupingDetails::VectorCells(Row,Contact.Position);
            Row.Add(FString::FromInt(606430+Pattern));
            RaysCsv+=FString::Join(Row,TEXT(","))+TEXT("\n");
        }
    }
    ++Shot;
}
void AONE06GroupingCheck::CompleteTrial()
{
    auto* W=Player->GetWeaponComponent(); const auto* State=W->GetSlotState(W->GetEquippedIndex());
    const float Recovered=State?State->SpreadBloom:-1.f;
    Check(Shot==RequiredShots() && SeenShots==TrialShotsBefore+RequiredShots() && W->GetTotalShotsFired()==SeenShots,TEXT("Release leaves exactly the requested shots with no delayed or extra discharge"));
    Check(Recovered>=0.f && Recovered<=1.e-5f && FMath::IsNearlyEqual(W->GetCurrentSpreadDegrees(),W->GetDefinition().SpreadDegrees,1.e-5f),TEXT("Released firing fully recovers bloom to the configured standing spread"));
    Check(WallHits==Projectiles && WallHits>1,TEXT("The full observed projectile group reaches the wall"));
    if (Scene) Check(EnemyContacts>0 && Target && !Target->IsDead() && Target->GetHealth()<100000.f && Mode->GetPoints()>PointsBefore,TEXT("Paired occupied scene includes actual living contacts and penetration without fixture death"));
    else Check(EnemyContacts==0 && Mode->GetPoints()==PointsBefore,TEXT("Wall-only grouping creates no victim damage award"));
    if (Pattern==1) Check(LastSpread>FirstSpread+.4f,TEXT("Twelve-shot M4A1 burst has measurable sustained-fire spread growth"));
    const float Width=WallHits>1?float(FVector2D(MaxY-MinY,MaxZ-MinZ).Size()):0.f;
    Check(Width>0.f,TEXT("Measured wall group has a nonzero spatial extent"));
    if (Scene==0)
    {
        if (DistanceIndex>0) Check(Width>Widths[DistanceIndex-1]+.1f,TEXT("The same seeded pattern makes a wider actual wall group at the longer distance"));
        Widths[DistanceIndex]=Width;
    }
    else Check(FMath::IsNearlyEqual(Width,Widths[DistanceIndex],.05f),TEXT("Enemy occupancy changes damage but preserves the measured wall group"));
    TArray<FString> Cells={PatternName(),W->GetDefinition().Id.ToString(),ONE06GroupingDetails::Number(Distance()),FString::FromInt(Scene),FString::FromInt(Shot),FString::FromInt(Projectiles),FString::FromInt(WallHits),
        FString::FromInt(EnemyContacts),FString::FromInt(Mode->GetPoints()-PointsBefore),ONE06GroupingDetails::Number(FirstSpread),ONE06GroupingDetails::Number(LastSpread),ONE06GroupingDetails::Number(PeakBloom),ONE06GroupingDetails::Number(Recovered),
        ONE06GroupingDetails::Number(MinY),ONE06GroupingDetails::Number(MaxY),ONE06GroupingDetails::Number(MinZ),ONE06GroupingDetails::Number(MaxZ),ONE06GroupingDetails::Number(MaxY-MinY),ONE06GroupingDetails::Number(MaxZ-MinZ),ONE06GroupingDetails::Number(Width),ONE06GroupingDetails::Number(PairOriginError),ONE06GroupingDetails::Number(PairDirectionError)};
    Cells.Add(FString::FromInt(606430+Pattern));
    GroupsCsv+=FString::Join(Cells,TEXT(","))+TEXT("\n"); ++Trials;
    if (Scene==0) { Scene=1; PrepareTrial(); }
    else if (++DistanceIndex<3) { Scene=0; PrepareTrial(); }
    else if (++Pattern<3) PreparePattern();
    else { Pattern=2; DistanceIndex=2; Finish(true); }
}
void AONE06GroupingCheck::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const double Real=FPlatformTime::Seconds();
    if (bFinished) { if (Real-FinishedReal>.5) FPlatformMisc::RequestExit(false); return; }
    if (bFinishing) { Finish(bComplete); return; }
    if (Capture && Capture->HasFailed()) { Check(false,TEXT("Optional capture failed: ")+Capture->GetFailureReason()); Finish(false); return; }
    if (Real-StartedReal>150.) { Check(false,TEXT("Grouping exceeded its 150-second active budget; remaining time is reserved for bounded capture finalization")); Finish(false); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player) return;
    if (!Controller) Controller=Cast<AONEPlayerController>(Player->GetController());
    if (!Mode) Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Controller || !Mode) return;
    auto* W=Player->GetWeaponComponent(); const double Age=GetWorld()->GetTimeSeconds()-PhaseAt;
    if (Phase==0 && Age>1.5)
    {
        Check(Mode->IsSandbox(),TEXT("Grouping is an explicit sandbox scene fixture")); Origin=Player->GetActorLocation();
        Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetCharacterMovement()->DisableMovement(); PreparePattern();
    }
    else if (Phase==1 && Age>1.2 && W->CanFire())
    { Player->GetMesh()->bPauseAnims=true; PrepareTrial(); }
    else if (Phase==2 && Age>.35 && W->CanFire())
    { Key(true); Enter(3); }
    else if (Phase==3)
    {
        if (W->GetTotalShotsFired()>SeenShots)
        {
            RecordShot();
            if (Pattern==2 || Shot>=RequiredShots()) Key(false);
            if (Shot>=RequiredShots()) Enter(5);
            else if (Pattern==2) Enter(4);
        }
        else if (Age>3.) { Check(false,TEXT("Requested eligible input failed to produce an observed discharge")); Finish(false); }
    }
    else if (Phase==4 && W->CanFire()) { Key(true); Enter(3); }
    else if (Phase==5 && Age>.8 && W->CanFire()) CompleteTrial();
}
void AONE06GroupingCheck::Finish(bool Complete)
{
    if (bFinished) return;
    if (!bFinishing) { bFinishing=true; bComplete=Complete; Key(false); if (Player) Player->ReleaseHeldInputs(); }
    else bComplete=bComplete && Complete;
    if (Capture)
    {
        const bool Done=Capture->FinishCaptureAndWait();
        if (!Done && !Capture->HasFailed() && !bEndingPlay) return;
        Check(Done && !Capture->HasFailed(),TEXT("Optional capture finalizes actual frames and master audio before exit: ")+Capture->GetFailureReason());
    }
    Check(!bComplete || Trials==18,TEXT("Completed fixture contains all 18 paired distance/pattern trials"));
    const auto Save=[this](const FString& Name,const FString& Text)
    { return FFileHelper::SaveStringToFile(Text,*(Folder/Name),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM); };
    Check(Save(TEXT("rays.csv"),RaysCsv),TEXT("Every actual projectile and contact record saved"));
    Check(Save(TEXT("groups.csv"),GroupsCsv),TEXT("Measured wall groups saved"));
    auto Json=MakeShared<FJsonObject>(); Json->SetStringField(TEXT("schema"),TEXT("one06.grouping.v1")); Json->SetBoolField(TEXT("complete"),bComplete);
    Json->SetNumberField(TEXT("trials"),Trials); Json->SetNumberField(TEXT("failures"),Failures); Json->SetNumberField(TEXT("runtime_real_seconds"),FPlatformTime::Seconds()-StartedReal);
    Json->SetStringField(TEXT("method"),TEXT("Declared direct base-weapon acquisition/ammo reset/private-seed reset, fixed player movement and evaluated pose, one ordinary logical torso cursor; actual Controller InputKey discharges. Paired scene adds one normal-height infected at 180 cm with 100000 HP and high structural thresholds, frozen pursuit/pose. Visible solid walls are 300/650/1000 cm from player origin; range and falloff remain production values. Projectile rows contain actual shot paths; contact rows repeat the path with one real regional contact. Width is the Y/Z bounding-box diagonal of actual wall endpoints, not a fitted circle. No aim override, fabricated rays, native-input claim, performance claim or perceptual audio review."));
    Json->SetStringField(TEXT("capture_directory"),Capture?TEXT("Media"):TEXT("")); Json->SetBoolField(TEXT("capture_complete"),Capture && Capture->IsComplete());
    Json->SetNumberField(TEXT("captured_frames"),Capture?Capture->GetFrameCount():0);
    FString Metadata; const auto Writer=TJsonWriterFactory<>::Create(&Metadata); FJsonSerializer::Serialize(Json,Writer);
    Check(Save(TEXT("observations.json"),Metadata),TEXT("Fixture method and completion metadata saved"));
    if (!Save(TEXT("checks.csv"),ChecksCsv)) { ++Checks; ++Failures; UE_LOG(LogTemp,Error,TEXT("ONE06_GROUPING FAIL | checks.csv could not be saved")); }
    const FString Summary=FString::Printf(TEXT("Complete: %d\nChecks: %d\nFailures: %d\nTrials: %d\nFrames: %d\n"),bComplete,Checks,Failures,Trials,Capture?Capture->GetFrameCount():0);
    if (!Save(TEXT("checks.txt"),Summary)) ++Failures;
    ClearScene(); if (Player) Player->GetMesh()->bPauseAnims=false;
    bFinished=true; bFinishing=false; FinishedReal=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("ONE06_GROUPING_COMPLETE complete=%d failures=%d checks=%d trials=%d frames=%d"),bComplete,Failures,Checks,Trials,Capture?Capture->GetFrameCount():0);
}
void AONE06GroupingCheck::EndPlay(const EEndPlayReason::Type Reason)
{ bEndingPlay=true; if (!bFinished) { Check(false,TEXT("Grouping interrupted before completion")); Finish(false); } Super::EndPlay(Reason); }
