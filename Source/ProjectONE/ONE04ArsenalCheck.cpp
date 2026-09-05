#include "ONE04ArsenalCheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEGameMode.h"
#include "ONEWeaponComponent.h"
#include "ONEWeaponMagazine.h"
#include "ONEWeaponCase.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "AIController.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

namespace ONE04ArsenalFixture
{
    const FVector PlayerOrigin(-1000,4000,800);
    EONEWeaponFamily Family(int32 Variant) { return Variant<2 ? EONEWeaponFamily::Pistol : Variant<4 ? EONEWeaponFamily::Carbine : EONEWeaponFamily::Shotgun; }
}
AONE04ArsenalCheck::AONE04ArsenalCheck()
{ PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void AONE04ArsenalCheck::BeginPlay()
{
    Super::BeginPlay(); StartReal=StageReal=FPlatformTime::Seconds(); StageStart=GetWorld()->GetTimeSeconds();
    Csv=TEXT("world_seconds,stage,variant,shots,ejections,magazine_drops,live_magazines,ammo,reserve,operation\n");
}
void AONE04ArsenalCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    auto R=MakeShared<FJsonObject>(); R->SetNumberField(TEXT("stage"),Stage); R->SetNumberField(TEXT("variant"),Variant);
    R->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds()); R->SetBoolField(TEXT("pass"),Pass); R->SetStringField(TEXT("label"),Label);
    Records.Add(MakeShared<FJsonValueObject>(R));
    UE_LOG(LogTemp,Display,TEXT("ONE04_ARSENAL %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE04ArsenalCheck::Next(int32 NewStage)
{ Stage=NewStage; StageStart=GetWorld()->GetTimeSeconds(); StageReal=FPlatformTime::Seconds(); }
void AONE04ArsenalCheck::Key(const FKey& InKey,EInputEvent Event)
{
    if (auto* PC=Player ? Cast<AONEPlayerController>(Player->GetController()) : nullptr)
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(InKey,Event,Event==IE_Released?0.f:1.f));
}
void AONE04ArsenalCheck::Pulse(const FKey& InKey)
{ Key(InKey,IE_Pressed); Releases.Add({InKey,GetWorld()->GetTimeSeconds()+.035f}); }
AActor* AONE04ArsenalCheck::StaticBox(const FVector& Position,const FVector& Extent)
{
    AActor* A=GetWorld()->SpawnActor<AActor>(); if (!A) return nullptr;
    auto* B=NewObject<UBoxComponent>(A); A->SetRootComponent(B); A->AddInstanceComponent(B);
    B->SetMobility(EComponentMobility::Movable); B->SetBoxExtent(Extent); B->SetCollisionObjectType(ECC_WorldStatic);
    B->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); B->SetCollisionResponseToAllChannels(ECR_Block); B->SetCanEverAffectNavigation(false);
    B->RegisterComponent(); A->SetActorLocation(Position); return A;
}
AONEZombie* AONE04ArsenalCheck::TargetAt(float Distance)
{
    FActorSpawnParameters P; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    auto* Z=GetWorld()->SpawnActor<AONEZombie>(ONE04ArsenalFixture::PlayerOrigin+FVector(Distance,0,0),FRotator(0,180,0),P);
    if (!Z) { Check(false,TEXT("Isolated query target spawned")); return nullptr; }
    Z->Health->MaxHealth=10000.f; Z->Health->Restore();
    Z->HeadSeverThreshold=Z->ArmSeverThreshold=Z->LegSeverThreshold=10000.f;
    Z->SetActorTickEnabled(false); Z->GetCharacterMovement()->DisableMovement();
    if (auto* AI=Cast<AAIController>(Z->GetController())) AI->StopMovement();
    // This isolated damage fixture deliberately fixes the evaluated pose;
    // disabling the actor alone would leave its independently ticking mesh live.
    Z->GetMesh()->TickAnimation(0.f,false); Z->GetMesh()->RefreshBoneTransforms();
    Z->GetMesh()->bPauseAnims=true; Z->GetMesh()->SetComponentTickEnabled(false);
    // Match the center of the real torso query to the horizontal test ray.
    const FVector Offset=Z->BodyRegion->GetComponentLocation()-Z->GetActorLocation();
    Z->SetActorLocation(Player->GetMuzzleLocation()+FVector(Distance,0,0)-Offset);
    Targets.Add(Z); return Z;
}
void AONE04ArsenalCheck::ClearTargets()
{ for (const auto& Ref:Targets) if (auto* Z=Ref.Get(); IsValid(Z)) Z->Destroy(); Targets.Reset(); if (IsValid(Cover)) Cover->Destroy(); Cover=nullptr; }
void AONE04ArsenalCheck::PrepareVariant(int32 Index)
{
    Variant=Index; ClearTargets(); Player->ReleaseHeldInputs(); Releases.Reset();
    Player->GetCharacterMovement()->StopMovementImmediately(); Player->SetActorLocation(ONE04ArsenalFixture::PlayerOrigin);
    Player->SetAimOverride(true,ONE04ArsenalFixture::PlayerOrigin+FVector(6000,0,40));
    auto* W=Player->GetWeaponComponent(); W->ResetStarterLoadout();
    if (ONE04ArsenalFixture::Family(Index)!=EONEWeaponFamily::Pistol)
        Check(W->ApplyAcquisitionPlan(W->BuildAcquisitionPlan(ONE04ArsenalFixture::Family(Index))),TEXT("Declared isolated weapon fixture acquires family through owned-slot API"));
    Next(1);
}
void AONE04ArsenalCheck::CompleteVariant()
{
    ClearTargets();
    if (++Variant<6) PrepareVariant(Variant);
    else
    {
        auto* W=Player->GetWeaponComponent(); W->ResetStarterLoadout();
        Player->SetActorLocation(ONE04ArsenalFixture::PlayerOrigin); Player->SetAimOverride(true,ONE04ArsenalFixture::PlayerOrigin+FVector(6000,0,40));
        BudgetCount=0; Drops=W->GetMagazineDropCount(); Next(40);
    }
}
void AONE04ArsenalCheck::PrepareRayCase(int32 Index)
{
    RayCase=Index; ClearTargets();
    Player->GetCharacterMovement()->StopMovementImmediately(); Player->SetActorLocation(ONE04ArsenalFixture::PlayerOrigin);
    const FVector Muzzle=Player->GetMuzzleLocation(); Player->SetAimOverride(true,Muzzle+FVector(6000,0,0));
    const auto& D=Player->GetWeaponComponent()->GetDefinition();
    TargetAt(400.f);
    if (Index==3) TargetAt(D.Range-100.f);
    else if (Index==4) TargetAt(D.Range+100.f);
    else TargetAt(650.f);
    if (Index==0) TargetAt(900.f);
    if (Index==1 || Index==2) Cover=StaticBox(Muzzle+FVector(Index==1?500.f:200.f,0,0),FVector(12,140,140));
    Check(Targets.Num()==(Index==0?3:2) && (Index!=1 && Index!=2 || IsValid(Cover)),TEXT("Declared horizontal ray fixture contains required targets and explicit cover"));
    Shots=Player->GetWeaponComponent()->GetTotalShotsFired(); Next(61);
}
void AONE04ArsenalCheck::CheckRayCase()
{
    auto* W=Player->GetWeaponComponent();
    Check(W->GetTotalShotsFired()==Shots+1,TEXT("Penetration scenario is one actual semi-auto discharge"));
    if (Targets.Num()<2) return;
    const float First=10000.f-Targets[0]->GetHealth(),Second=10000.f-Targets[1]->GetHealth();
    if (RayCase==0)
    {
        Check(FMath::IsNearlyEqual(First,W->GetDefinition().Damage,.05f) && FMath::IsNearlyEqual(Second,First*.6f,.05f),FString::Printf(TEXT("Last Word damages torso then one reduced second victim: %.4f / %.4f"),First,Second));
        Check(Targets[0]->GetDamageTransactionCount()==1 && Targets[1]->GetDamageTransactionCount()==1 && Targets[2]->GetDamageTransactionCount()==0 && Targets[2]->GetHealth()==10000.f && W->GetLastShotVictimCount()==2,TEXT("One discharge visits each first/second victim once and never chains to third"));
    }
    else if (RayCase==1) Check(First>0 && Second==0 && W->GetLastShotVictimCount()==1,TEXT("World cover behind first victim stops the second penetration trace"));
    else if (RayCase==2) Check(First==0 && Second==0 && W->GetLastShotVictimCount()==0,TEXT("World cover before first victim prevents every damage transaction"));
    else if (RayCase==3) Check(First>0 && Second>0 && Second<First*.6f && W->GetLastShotVictimCount()==2,TEXT("Control target inside original range is penetrated with normal range falloff"));
    else if (RayCase==4) Check(First>0 && Second==0 && W->GetLastShotVictimCount()==1,TEXT("Target beyond original muzzle range is not reached by resetting range at first victim"));
    else Check(First>0 && Second==0 && W->GetLastShotVictimCount()==1,TEXT("Base M1911 retains single-victim behavior in identical aligned geometry"));
}
void AONE04ArsenalCheck::Finish()
{
    if (bFinished) return;
    Key(EKeys::W,IE_Released); Key(EKeys::LeftShift,IE_Released); Key(EKeys::R,IE_Released); Key(EKeys::LeftMouseButton,IE_Released);
    if (Player) { Player->ReleaseHeldInputs(); Player->GetWeaponComponent()->ClearEjectedCases(); }
    ClearTargets(); if (IsValid(Floor)) Floor->Destroy();
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("candidate"),TEXT("04"));
    R->SetStringField(TEXT("fixture"),TEXT("Isolated collision floor outside arena and frozen, unregistered torso-query targets with10000 health/sever thresholds. Actual catalog damage/spread/range/timings unchanged. Variant acquisition/reserve/ready/collection APIs bypass only machine wait for isolated weapon setup. Fire/R/Shift/W route through production PlayerController InputKey. Magazine cap fixture extends already-emitted actor lifespans to120s only to isolate eviction; separate test measures genuine8s expiry. This is not native OS input, art/audio review, navigation/score or performance proof."));
    R->SetNumberField(TEXT("checks"),Checks); R->SetNumberField(TEXT("failures"),Failures); R->SetArrayField(TEXT("assertions"),Records);
    FString Json; FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate04/Arsenal"); IFileManager::Get().MakeDirectory(*Folder,true);
    if (!FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("checks.json"))) || !FFileHelper::SaveStringToFile(Csv,*(Folder/TEXT("timeline.csv")))) ++Failures;
    UE_LOG(LogTemp,Display,TEXT("ONE04_ARSENAL_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
void AONE04ArsenalCheck::Tick(float Dt)
{
    Super::Tick(Dt); const double RealNow=FPlatformTime::Seconds(); const float Now=GetWorld()->GetTimeSeconds();
    if (bFinished) { if (RealNow-FinishedReal>.4) FPlatformMisc::RequestExit(false); return; }
    if (RealNow-StartReal>175 || RealNow-StageReal>14) { Check(false,FString::Printf(TEXT("Bounded arsenal timeout stage%d variant%d"),Stage,Variant)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!GM) GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !GM || !Player->GetController()) return;
    for (int32 I=Releases.Num()-1;I>=0;--I) if (Now>=Releases[I].Value) { Key(Releases[I].Key,IE_Released); Releases.RemoveAt(I); }
    Player->Health->Restore(); auto* W=Player->GetWeaponComponent(); const float T=Now-StageStart;
    if (Now-LastTrace>.1f)
    { LastTrace=Now; Csv+=FString::Printf(TEXT("%.6f,%d,%d,%d,%d,%d,%d,%d,%d,%d\n"),Now,Stage,Variant,W->GetTotalShotsFired(),W->GetEjectionCount(),W->GetMagazineDropCount(),W->GetLiveMagazineCount(),W->GetAmmo(),W->GetReserveAmmo(),int32(W->GetOperation())); }
    switch (Stage)
    {
    case 0: if (T>.7f)
    {
        Check(GM->IsSandbox(),TEXT("Arsenal check uses explicit sandbox without normal waves"));
        Floor=StaticBox(FVector(0,4000,700),FVector(2600,1000,10)); Check(IsValid(Floor),TEXT("Isolated actual-collision floor fixture created")); PrepareVariant(0);
    } break;
    case 1: if (!W->IsBusy() && W->HasUsableWeapon() && T>.3f)
    {
        if (Variant%2)
        {
            FONEWeaponReservation Token;
            const bool Ok=W->ReserveEquippedForUpgrade(Token) && W->MarkUpgradeReady(Token) && W->CollectUpgrade(Token);
            Check(Ok,TEXT("Declared effective-upgrade fixture uses same-instance owned transaction methods"));
        }
        Next(2);
    } break;
    case 2: if (!W->IsBusy() && W->CanFire() && T>.3f)
    {
        const auto& D=W->GetDefinition(); const auto* Expected=W->GetCatalogDefinition(ONE04ArsenalFixture::Family(Variant),Variant%2!=0);
        Check(Expected && D.Id==Expected->Id && W->GetAmmo()==D.Capacity && W->GetReserveAmmo()==(Variant%2?D.ReserveLimit:D.InitialReserve) && Player->Gun->GetStaticMesh()==D.Mesh.Get(),TEXT("Effective variant equips its actual catalog body, loaded capacity and supply"));
        TargetAt(150.f); Shots=W->GetTotalShotsFired(); Ejections=W->GetEjectionCount(); Drops=W->GetMagazineDropCount();
        bSawShotBeforeEject=bSawPumpBeforeEject=bEarlyCase=bLateCase=false;
        Player->SetAimOverride(true,Player->GetMuzzleLocation()+FVector(6000,0,0)); Pulse(EKeys::LeftMouseButton); Next(3);
    } break;
    case 3:
    if (W->GetDefinition().bPumpAction && W->GetTotalShotsFired()==Shots+1)
    {
        const auto* Pump=W->GetDefinition().Operations.FindByPredicate([](const auto& O){return O.Operation==EONEWeaponOperation::Pump;});
        const auto* Eject=Pump ? Pump->Events.FindByPredicate([](const auto& E){return E.Event==EONEWeaponEvent::ShellEject;}) : nullptr;
        if (W->GetOperation()==EONEWeaponOperation::Fire)
        { bSawShotBeforeEject=true; bEarlyCase|=W->GetEjectionCount()!=Ejections; }
        if (W->GetOperation()==EONEWeaponOperation::Pump && Eject)
        {
            if (W->GetOperationElapsed()<Eject->Time)
            { bSawPumpBeforeEject=true; bEarlyCase|=W->GetEjectionCount()!=Ejections; }
            else bLateCase|=W->GetEjectionCount()!=Ejections+1;
        }
    }
    if (T>1.15f && !W->IsBusy() && !W->NeedsPump(W->GetEquippedIndex()))
    {
        const auto& D=W->GetDefinition();
        Check(W->GetTotalShotsFired()==Shots+1 && W->GetAmmo()==D.Capacity-1,TEXT("Actual variant discharge consumes exactly one cartridge/shell"));
        Check(W->GetEjectionCount()==Ejections+1 && W->GetLastEjectedCase() && W->GetLastEjectedCase()->GetSourceShotId()==W->GetLastShotId(),TEXT("Exactly one source-identified case appears at the required fire/pump event"));
        if (D.bPumpAction) Check(bSawShotBeforeEject && bSawPumpBeforeEject && !bEarlyCase && !bLateCase,TEXT("Observed discharge and pre-event pump retain shell; ejection occurs only when effective pump event becomes due"));
        const float Actual=Targets.Num()==1 ? 10000.f-Targets[0]->GetHealth() : 0.f;
        Check(Targets.Num()==1 && Targets[0]->GetDamageTransactionCount()==1 && Actual>0 && Actual<=D.Damage*D.Pellets+.1f,FString::Printf(TEXT("Real regional dispatch applies bounded effective damage once: %.4f maximum%.4f"),Actual,D.Damage*D.Pellets));
        if (D.Pellets==1) Check(FMath::IsNearlyEqual(Actual,D.Damage,.05f),TEXT("Single-projectile torso damage matches effective variant exactly"));
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); Inserts=W->GetShellInsertCount();
        if (D.bShellReload) { Pulse(EKeys::R); Next(25); }
        else
        {
            const auto* Reload=D.Operations.FindByPredicate([](const auto& O){return O.Operation==EONEWeaponOperation::MagazineReload;});
            const auto* Event=Reload?Reload->Events.FindByPredicate([](const auto& E){return E.Event==EONEWeaponEvent::MagazineOut;}):nullptr;
            DropTime=Event?Event->Time:.28f; Pulse(EKeys::R); Next(10);
        }
    } break;
    case 10: if (W->IsReloading() && W->GetOperationElapsed()>=DropTime*.45f)
    { Key(EKeys::LeftShift,IE_Pressed); if (Variant==0) Key(EKeys::W,IE_Pressed); Next(11); } break;
    case 11: if (T>.04f)
    {
        Check(W->IsMagazineReloadCommitted() && W->ShouldShowSeatedMagazine() && W->GetMagazineDropCount()==Drops && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve,TEXT("Sprint before removal preserves committed reload and seated magazine before its real event"));
        Next(13);
    } break;
    case 13: if (W->GetMagazineDropCount()>Drops)
    {
        Dropped=W->GetLastDroppedMagazine(); DropAt=Now;
        Check(W->GetMagazineDropCount()==Drops+1 && !W->ShouldShowSeatedMagazine() && !W->ShouldShowHeldMagazine() && W->GetAmmo()==Ammo,TEXT("Removal emits exactly one old magazine before fresh prop/transfer"));
        if (Dropped.IsValid())
        {
            Check(Dropped->GetSourceInstanceId()==W->GetSlotState(W->GetEquippedIndex())->InstanceId && Dropped->GetMagazineMesh()->GetStaticMesh()==W->GetDefinition().MagazineMesh.Get(),TEXT("Dropped old magazine matches effective assembly and exact owned instance"));
            Check(FVector::Dist(Dropped->GetReleaseTransform().GetLocation(),Player->GetMagazineReleaseTransform().GetLocation())<2.f,TEXT("Release samples current seated-magazine world pose rather than feet/muzzle"));
            Check(Dropped->GetMagazineMesh()->GetCollisionEnabled()==ECollisionEnabled::NoCollision && !Dropped->GetMagazineMesh()->CanEverAffectNavigation(),TEXT("Cosmetic magazine cannot block pawn, shots or navigation"));
            if (Variant==0) Check(Dropped->GetInheritedVelocity().Size2D()>200.f && FVector::Dist(Dropped->GetInheritedVelocity(),Player->GetVelocity())<8.f,TEXT("Dropped magazine inherits actual moving player's velocity"));
        }
        else Check(false,TEXT("Mechanical removal created a physical cosmetic actor"));
        Key(EKeys::LeftShift,IE_Pressed); Key(EKeys::W,IE_Released); Next(14);
    } break;
    case 14: if (T>.12f)
    {
        Check(W->IsMagazineReloadCommitted() && !W->ShouldShowSeatedMagazine() && !W->CanFire() && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve,TEXT("Sprint after drop preserves committed reload, unearned ammunition and absent magazine"));
        if (Dropped.IsValid()) Check(Dropped->GetVelocity().Z<Dropped->GetInitialVelocity().Z-50.f,TEXT("Released magazine follows world gravity independently of current gun pose"));
        Key(EKeys::LeftShift,IE_Released); Shots=W->GetTotalShotsFired(); Pulse(EKeys::LeftMouseButton); Next(15);
    } break;
    case 15: if (T>.3f)
    {
        Check(W->IsMagazineReloadCommitted() && W->GetTotalShotsFired()==Shots && !W->HasAcceptedFramePress(),TEXT("Fire during magazine reload is rejected immediately without interrupting or banking a discharge"));
        Pulse(EKeys::R); Next(16);
    } break;
    case 16: if (W->IsReloading() && W->GetOperationElapsed()>DropTime+.08f)
    {
        Check(W->GetMagazineDropCount()==Drops+1 && W->GetAmmo()==Ammo,TEXT("Repeated R leaves committed clock advancing and cannot duplicate the old magazine drop")); Next(17);
    } break;
    case 17: if (W->ShouldShowHeldMagazine())
    { Check(!W->ShouldShowSeatedMagazine() && W->GetAmmo()==Ammo,TEXT("Fresh held replacement appears distinctly before actual ammo commit")); Next(18); } break;
    case 18: if (!W->IsBusy() && T>.25f)
    {
        Check(W->GetAmmo()==W->GetDefinition().Capacity && W->GetReserveAmmo()==Reserve-1 && W->ShouldShowSeatedMagazine() && W->GetMagazineDropCount()==Drops+1 && W->GetTotalShotsFired()==Shots,TEXT("Committed insertion fills exactly missing round and restores magazine without repeated drop or deferred fire"));
        Check(Dropped.IsValid() && Dropped->IsSettled() && Dropped->GetBounceCount()>0,TEXT("Old magazine bounces and settles on real supporting collision"));
        if (Dropped.IsValid()) RestPosition=Dropped->GetActorLocation(); Next(19);
    } break;
    case 19: if (T>.35f)
    {
        Check(Dropped.IsValid() && FVector::Dist(RestPosition,Dropped->GetActorLocation())<.05f,TEXT("Settled old magazine remains at stable world support after reload completes"));
        if (Variant==0) Next(20); else CompleteVariant();
    } break;
    case 20: if (Now-DropAt>7.8f)
    { Check(Now-DropAt<8.f && Dropped.IsValid() && W->GetLiveMagazineCount()==1,TEXT("Unmodified production magazine remains alive immediately before eight-second expiry")); Next(21); } break;
    case 21: if (Now-DropAt>8.2f)
    { Check(!Dropped.IsValid() && W->GetLiveMagazineCount()==0,TEXT("Unmodified production magazine expires at bounded eight-second lifetime")); CompleteVariant(); } break;
    case 25: if (W->GetOperation()==EONEWeaponOperation::ShellInsert && W->GetOperationElapsed()>.3f)
    { Key(EKeys::LeftShift,IE_Pressed); Next(26); } break;
    case 26: if (T>.1f)
    {
        Check(W->IsReloading() && W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && W->GetMagazineDropCount()==Drops,TEXT("Sprint preserves shotgun loading before its genuine shell commit without a magazine actor"));
        Key(EKeys::LeftShift,IE_Released); Pulse(EKeys::LeftMouseButton); Next(27);
    } break;
    case 27: if (T>.4f && !W->IsBusy())
    {
        Check(W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && W->GetTotalShotsFired()==Shots+1,TEXT("Loaded shell interrupt closes without earning a shell or banking the interrupt click"));
        Pulse(EKeys::R); Next(28);
    } break;
    case 28: if (!W->IsBusy() && T>1.8f)
    {
        Check(W->GetAmmo()==W->GetDefinition().Capacity && W->GetReserveAmmo()==Reserve-1 && W->GetShellInsertCount()==Inserts+1 && W->GetMagazineDropCount()==Drops && W->GetEjectionCount()==Ejections+1,TEXT("Shotgun resumes genuine one-shell transfer with unchanged exact pump ejection and no magazine actor")); CompleteVariant();
    } break;
    case 40: if (W->CanFire() && T>.1f) { Pulse(EKeys::LeftMouseButton); Next(41); } break;
    case 41: if (!W->IsBusy() && T>.3f) { Pulse(EKeys::R); Next(42); } break;
    case 42: if (W->GetMagazineDropCount()==Drops+BudgetCount+1)
    {
        auto* M=W->GetLastDroppedMagazine();
        if (M) { M->SetLifeSpan(120.f); if (BudgetCount==0) FirstBudget=M; }
        Check(M!=nullptr,TEXT("Cap fixture retains a real event-emitted magazine; only its test lifespan is extended")); Next(43);
    } break;
    case 43: if (!W->IsBusy() && T>.3f)
    {
        ++BudgetCount;
        Check(W->GetLiveMagazineCount()<=W->MaximumMagazines,TEXT("Live magazine count never exceeds production budget"));
        if (BudgetCount<W->MaximumMagazines+2) Next(40);
        else
        {
            Check(W->GetLiveMagazineCount()==W->MaximumMagazines && !FirstBudget.IsValid() && W->GetMagazineDropCount()==Drops+BudgetCount,TEXT("Budget evicts oldest physical magazine despite extended lifetime, without duplicating events"));
            W->ClearEjectedCases(); Check(W->GetLiveMagazineCount()==0 && W->GetLiveCaseCount()==0,TEXT("Existing presentation cleanup clears magazines and cases together"));
            W->ResetStarterLoadout(); FONEWeaponReservation R;
            Check(W->ReserveEquippedForUpgrade(R) && W->MarkUpgradeReady(R) && W->CollectUpgrade(R),TEXT("Penetration fixture obtains effective Last Word through owned-instance API")); Next(60);
        }
    } break;
    case 60: if (!W->IsBusy() && W->CanFire() && T>.3f) { PrepareRayCase(0); } break;
    case 61: if (T>.15f) { Pulse(EKeys::LeftMouseButton); Next(62); } break;
    case 62: if (T>.35f && !W->IsBusy())
    {
        CheckRayCase();
        if (RayCase<4) PrepareRayCase(RayCase+1);
        else if (RayCase==4) { ClearTargets(); W->ResetStarterLoadout(); Next(63); }
        else Finish();
    } break;
    case 63: if (T>.3f && W->CanFire()) PrepareRayCase(5); break;
    default: break;
    }
}
