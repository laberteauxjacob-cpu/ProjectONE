#include "ONE03CaseCheck.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
#include "ONEWeaponCase.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

AONE03CaseCheck::AONE03CaseCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE03CaseCheck::BeginPlay()
{
    Super::BeginPlay();
    StartReal=StageReal=FPlatformTime::Seconds(); StageStart=GetWorld()->GetTimeSeconds();
    Report=TEXT("Candidate03 Stage C: actual emitted case lifecycle\nProduction weapon actions and case actors; no screenshot/audio capture.\nA separate budget fixture extends already-emitted cases to60s so natural6s expiry cannot hide cap eviction. Normal lifetime is tested before that fixture. No damage, ammo capacity, cadence, collision or motion tuning is changed.\n\n");
    Timeline=TEXT("world_seconds,stage,weapon,shot_id,x,y,z,vx,vy,vz,bounces,settled,live_cases\n");
}
void AONE03CaseCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE03_CASE %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE03CaseCheck::Next(int32 NextStage)
{
    Stage=NextStage; StageStart=GetWorld()->GetTimeSeconds(); StageReal=FPlatformTime::Seconds();
    UE_LOG(LogTemp,Display,TEXT("ONE03_CASE_STAGE %d"),Stage);
}
int32 AONE03CaseCheck::ActualCaseCount() const
{
    int32 Count=0;
    for (TActorIterator<AONEWeaponCase> It(GetWorld());It;++It) if (IsValid(*It)) ++Count;
    return Count;
}
void AONE03CaseCheck::Prepare(int32 Slot)
{
    Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(FVector(-250,300,98));
    auto* W=Player->GetWeaponComponent(); W->ClearEjectedCases(); W->RefillAllAmmo(); W->SelectWeapon(Slot);
}
void AONE03CaseCheck::InspectCase(AONEWeaponCase* Case,int32 Slot,uint64 Shot)
{
    Check(IsValid(Case),TEXT("Committed ejection created an actual case actor"));
    if (!IsValid(Case)) return;
    auto* Mesh=Case->GetCaseMesh();
    Check(Mesh && Mesh->GetStaticMesh(),TEXT("Case actor has a loaded visible case mesh"));
    Check(Case->GetWeaponIndex()==Slot && Case->GetSourceShotId()==Shot && Shot!=0,
        FString::Printf(TEXT("Case identity retains source slot%d and discharge%llu"),Slot,Shot));
    Check(Mesh && Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision &&
        Mesh->GetCollisionResponseToChannel(ECC_Pawn)==ECR_Ignore && !Mesh->CanEverAffectNavigation(),
        TEXT("Case visual mesh ignores pawns, disables collision and cannot affect navigation"));
    Check(Case->GetCollisionRadius()>0 && Case->GetInitialVelocity().Size()>50.f,
        TEXT("Case has a nonzero swept collision radius and actual launch velocity"));
    const FVector Expected=Case->GetInitialVelocity()+FVector(0,0,GetWorld()->GetGravityZ()*Case->GetGameTimeSinceCreation());
    Check(Case->GetBounceCount()==0 && FVector::Dist(Case->GetCaseVelocity(),Expected)<30.f,
        TEXT("Observed airborne velocity contains the initial combined impulse, within one tick of gravity"));
}
void AONE03CaseCheck::ObserveLifecycle()
{
    auto* Case=FocusCase.Get(); if (!Case) return;
    const float Now=GetWorld()->GetTimeSeconds();
    const FVector Velocity=Case->GetCaseVelocity();
    const float Step=Now-PreviousSampleTime;
    if (bHaveSample && Step>.00001f && PreviousBounces==0 && Case->GetBounceCount()==0 && !Case->IsSettled())
    {
        GravitySum+=(Velocity.Z-PreviousVelocity.Z)/Step; ++GravitySamples;
        const FVector PositionVelocity=(Case->GetActorLocation()-PreviousLocation)/Step;
        if (Step<.1f)
        {
            ++HorizontalSamples;
            MaxHorizontalVelocityError=FMath::Max(MaxHorizontalVelocityError,
                static_cast<float>((PositionVelocity-(Velocity+PreviousVelocity)*.5f).Size2D()));
        }
    }
    if (Case->GetBounceCount()>0) bSawBounce=true;
    if (Case->GetBounceCount()>0 && Velocity.Z>5.f) bSawRebound=true;
    if (Case->IsSettled())
    {
        if (!bSawSettled)
        {
            bSawSettled=true; SettledAt=Now; SettledLocation=Case->GetActorLocation();
            FHitResult Floor;
            const bool Hit=GetWorld()->LineTraceSingleByObjectType(Floor,SettledLocation+FVector(0,0,10),
                SettledLocation-FVector(0,0,25),FCollisionObjectQueryParams(ECC_WorldStatic));
            const float Gap=Hit ? SettledLocation.Z-Case->GetCollisionRadius()-Floor.ImpactPoint.Z : BIG_NUMBER;
            Check(Hit && Floor.ImpactNormal.Z>.9f && FMath::Abs(Gap)<1.5f,
                FString::Printf(TEXT("Settled swept sphere has actual floor support; surface gap %.3fcm"),Gap));
        }
        bStableSettled &= FVector::Dist(Case->GetActorLocation(),SettledLocation)<.15f && Velocity.Size()<1.f;
    }
    const FVector P=Case->GetActorLocation();
    Timeline+=FString::Printf(TEXT("%.6f,%d,%d,%llu,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d,%d\n"),
        Now,Stage,Case->GetWeaponIndex(),Case->GetSourceShotId(),P.X,P.Y,P.Z,Velocity.X,Velocity.Y,Velocity.Z,
        Case->GetBounceCount(),Case->IsSettled()?1:0,ActualCaseCount());
    PreviousVelocity=Velocity; PreviousLocation=P; PreviousBounces=Case->GetBounceCount(); PreviousSampleTime=Now; bHaveSample=true;
}
void AONE03CaseCheck::Finish()
{
    if (bFinished) return;
    UGameplayStatics::SetGamePaused(this,false);
    if (Player) { Player->ReleaseHeldInputs(); Player->GetWeaponComponent()->ClearEjectedCases(); }
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    Report+=FString::Printf(TEXT("\nChecks: %d\nFailures: %d\n"),Checks,Failures);
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03/Cases");
    IFileManager::Get().MakeDirectory(*Folder,true);
    const bool SavedReport=FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")));
    const bool SavedTimeline=FFileHelper::SaveStringToFile(Timeline,*(Folder/TEXT("case_motion.csv")));
    if (!SavedReport || !SavedTimeline) ++Failures;
    UE_LOG(LogTemp,Display,TEXT("ONE03_CASE_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
void AONE03CaseCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    const double Real=FPlatformTime::Seconds();
    if (bFinished) { if (Real-FinishedReal>.4) FPlatformMisc::RequestExit(false); return; }
    if (Real-StartReal>100 || Real-StageReal>35)
    { Check(false,FString::Printf(TEXT("Case scenario timed out at stage%d"),Stage)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>(); if (!Player || !GM) return;
    GM->MaximumActive=0; Player->Health->Restore();
    Player->SetAimOverride(true,Player->GetActorLocation()+FVector(1000,0,35));
    auto* W=Player->GetWeaponComponent();
    const float Now=GetWorld()->GetTimeSeconds(),T=Now-StageStart;
    if (Stage>=3 && Stage<=5) ObserveLifecycle();
    switch (Stage)
    {
    case 0: if (T>.8f)
    {
        Check(GM->IsSandbox(),TEXT("Case check uses actual sandbox without automatic waves"));
        Prepare(0); Next(1);
    } break;
    case 1: if (T>.3f && W->CanFire())
    {
        Check(ActualCaseCount()==0 && W->GetLiveCaseCount()==0,TEXT("Case fixture begins with no leftover actors"));
        Shots=W->GetTotalShotsFired(); Ejections=W->GetEjectionCountForWeapon(0);
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); W->SetTrigger(true); Next(2);
    } break;
    case 2: if (W->GetTotalShotsFired()>Shots)
    {
        W->SetTrigger(false); FocusCase=W->GetLastEjectedCase(); BirthObserved=Now;
        Check(W->GetTotalShotsFired()==Shots+1 && W->GetEjectionCountForWeapon(0)==Ejections+1 &&
            ActualCaseCount()==1 && W->GetLiveCaseCount()==1,TEXT("One rifle discharge creates exactly one actual rifle case immediately"));
        Check(W->GetAmmo()==Ammo-1 && W->GetReserveAmmo()==Reserve,TEXT("Rifle case emission accompanies one ammo debit and no reserve transfer"));
        InspectCase(FocusCase.Get(),0,W->GetLastShotId());
        if (!FocusCase.IsValid()) { Finish(); break; }
        LaunchRotation=FocusCase->GetActorQuat();
        Check(FocusCase->GetInheritedVelocity().Size()<1.f,TEXT("Stationary rifle case inherits zero pawn velocity"));
        const FVector Port=Player->Gun->GetComponentTransform().TransformPosition(W->GetDefinition().EjectionPoint);
        Check(FVector::Dist(FocusCase->GetActorLocation(),Port)<28.f,TEXT("Observed first rifle case position is near the actual moving ejection port"));
        Check(FocusCase->GetLifeSpan()<=W->GetCaseLifetime()+.05f && FocusCase->GetLifeSpan()>W->GetCaseLifetime()-.35f,
            TEXT("Production case begins with the configured finite lifetime"));
        Next(3);
    } break;
    case 3: if (T>.10f && FocusCase.IsValid())
    {
        Check(LaunchRotation.AngularDistance(FocusCase->GetActorQuat())>.08f,TEXT("Actual airborne case rotates after emission"));
        PauseLocation=FocusCase->GetActorLocation(); PauseVelocity=FocusCase->GetCaseVelocity(); PauseLife=FocusCase->GetLifeSpan();
        Player->ReleaseHeldInputs(); UGameplayStatics::SetGamePaused(this,true); Next(4);
    } break;
    case 4: if (Real-StageReal>.35)
    {
        Check(FocusCase.IsValid() && FVector::Dist(FocusCase->GetActorLocation(),PauseLocation)<.01f &&
            FVector::Dist(FocusCase->GetCaseVelocity(),PauseVelocity)<.01f && FMath::Abs(FocusCase->GetLifeSpan()-PauseLife)<.01f,
            TEXT("Pause freezes actual airborne case position, velocity and lifetime"));
        UGameplayStatics::SetGamePaused(this,false); Next(5);
    } break;
    case 5: if (!FocusCase.IsValid())
    {
        const float Age=Now-BirthObserved;
        const float Acceleration=GravitySamples>0 ? GravitySum/GravitySamples : 0;
        Check(GravitySamples>=5 && Acceleration<-300.f && Acceleration>-1500.f,
            FString::Printf(TEXT("Observed free-flight vertical acceleration %.2fcm/s2 over%d real samples"),Acceleration,GravitySamples));
        Check(HorizontalSamples>=5 && MaxHorizontalVelocityError<15.f,
            FString::Printf(TEXT("Observed free-flight positions follow horizontal case velocity; maximum error %.3fcm/s"),MaxHorizontalVelocityError));
        Check(bSawBounce && bSawRebound && bSawSettled && SettledAt>0 && Now-SettledAt>.5f && bStableSettled,
            TEXT("Actual rifle case bounces, reaches supported rest and stays motionless before expiry"));
        Check(Age>=W->GetCaseLifetime()-.35f && Age<=W->GetCaseLifetime()+.35f && ActualCaseCount()==0 && W->GetLiveCaseCount()==0,
            FString::Printf(TEXT("Unmodified production case expires after %.3fs of world time"),Age));
        Prepare(0); Next(10);
    } break;
    case 10:
        Player->AddMovementInput(FVector(0,1,0));
        if (T>.45f && W->CanFire() && Player->GetVelocity().Size2D()>150.f)
        { Shots=W->GetTotalShotsFired(); Ejections=W->GetEjectionCountForWeapon(0); W->SetTrigger(true); Next(11); }
        break;
    case 11:
        Player->AddMovementInput(FVector(0,1,0));
        if (W->GetTotalShotsFired()>Shots)
        {
            W->SetTrigger(false); auto* Case=W->GetLastEjectedCase();
            InspectCase(Case,0,W->GetLastShotId());
            const FVector Inherited=Case ? Case->GetInheritedVelocity() : FVector::ZeroVector;
            Check(Case && Inherited.Size2D()>150.f && FVector::Dist(Inherited,Player->GetVelocity())<8.f,
                FString::Printf(TEXT("Moving rifle case inherits observed pawn velocity %.2fcm/s; pawn %.2fcm/s"),Inherited.Size2D(),Player->GetVelocity().Size2D()));
            Check(Case && (Case->GetInitialVelocity()-Inherited).Size()>50.f && W->GetEjectionCountForWeapon(0)==Ejections+1,
                TEXT("Moving case retains a separate ejection impulse in its combined initial velocity"));
            Prepare(1); Next(20);
        } break;
    case 20: if (W->GetEquippedIndex()==1 && W->CanFire() && T>.3f)
    {
        Shots=W->GetTotalShotsFired(); Ejections=W->GetEjectionCountForWeapon(1); Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo();
        for (const auto& Operation:W->GetDefinition().Operations) if (Operation.Operation==EONEWeaponOperation::Pump)
            for (const auto& Event:Operation.Events) if (Event.Event==EONEWeaponEvent::ShellEject) PumpEventTime=Event.Time;
        W->SetTrigger(true); Next(21);
    } break;
    case 21: if (W->GetTotalShotsFired()>Shots)
    {
        W->SetTrigger(false); PumpShot=W->GetLastShotId();
        Check(W->GetTotalShotsFired()==Shots+1 && W->GetAmmo()==Ammo-1 && W->GetReserveAmmo()==Reserve,
            TEXT("Shotgun discharge debits one loaded round"));
        Check(W->GetEjectionCountForWeapon(1)==Ejections && ActualCaseCount()==0 && W->NeedsPump(1),
            TEXT("Shotgun discharge retains its spent shell until the pump event")); Next(22);
    } break;
    case 22: if (W->GetOperation()==EONEWeaponOperation::Pump)
    {
        Check(W->GetOperationElapsed()<PumpEventTime && W->GetEjectionCountForWeapon(1)==Ejections && ActualCaseCount()==0,
            TEXT("Switch fixture intercepts the actual pump before its ejection event"));
        W->SelectWeapon(0); Next(23);
    } break;
    case 23: if (W->GetEquippedIndex()==0 && !W->IsBusy() && T>.6f)
    {
        Check(W->NeedsPump(1) && W->GetEjectionCountForWeapon(1)==Ejections && ActualCaseCount()==0,
            TEXT("Holstered pre-ejection shotgun retains obligation and creates no hidden shell"));
        W->SelectWeapon(1); Next(24);
    } break;
    case 24: if (W->GetOperation()==EONEWeaponOperation::Pump && W->GetEquippedIndex()==1)
    {
        Check(W->GetOperationElapsed()<PumpEventTime && ActualCaseCount()==0,TEXT("Resumed pump begins before the pending shell event")); Next(25);
    } break;
    case 25: if (W->GetEjectionCountForWeapon(1)>Ejections)
    {
        FocusCase=W->GetLastEjectedCase(); InspectCase(FocusCase.Get(),1,PumpShot);
        Check(W->GetOperation()==EONEWeaponOperation::Pump && W->GetOperationElapsed()>=PumpEventTime &&
            W->GetOperationElapsed()<PumpEventTime+FMath::Max(.06f,Dt*2.f),
            FString::Printf(TEXT("Shell actor appears on pump event %.3fs; observed %.3fs"),PumpEventTime,W->GetOperationElapsed()));
        Check(W->GetEjectionCountForWeapon(1)==Ejections+1 && ActualCaseCount()==1 && W->GetLiveCaseCount()==1,
            TEXT("Interrupted/resumed pump creates exactly one physical shell"));
        W->SelectWeapon(0); Next(26);
    } break;
    case 26: if (W->GetEquippedIndex()==0 && !W->IsBusy() && T>.6f)
    { W->SelectWeapon(1); Next(27); } break;
    case 27: if (W->GetEquippedIndex()==1 && !W->IsBusy() && T>1.f)
    {
        Check(FocusCase.IsValid() && W->GetLastEjectedCase()==FocusCase.Get() && ActualCaseCount()==1 &&
            W->GetEjectionCountForWeapon(1)==Ejections+1 && !W->NeedsPump(1),
            TEXT("Second switch after ejection finishes pump using the same shell actor with no duplicate"));
        Check(W->GetAmmoForWeapon(1)==Ammo-1 && W->GetReserveAmmoForWeapon(1)==Reserve && W->GetTotalShotsFired()==Shots+1,
            TEXT("Two pump interruptions preserve ammunition and the original discharge identity"));
        Prepare(0); ExtendedCaseShots.Reset(); Next(30);
    } break;
    case 30: if (W->GetEquippedIndex()==0 && W->CanFire())
    {
        Shots=W->GetTotalShotsFired(); Ejections=W->GetEjectionCountForWeapon(0);
        Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo(); TargetShots=W->GetMaximumCases()+3;
        Check(TargetShots>3 && TargetShots<=100,TEXT("Budget fixture has a finite supported configured case cap"));
        if (TargetShots<=3 || TargetShots>100) { Finish(); break; }
        W->SetTrigger(true); Next(31);
    } break;
    case 31:
        // This artificial TTL extension isolates eviction. Every emission and
        // reload still uses the real weapon, cadence, ammo and motion code.
        // Candidate05 discards the old hold when a reload starts. This case
        // budget fixture deliberately begins a fresh burst once ready again.
        if (W->CanFire() && !W->IsAutomaticBurstActive()) { W->SetTrigger(false); W->SetTrigger(true); }
        for (TActorIterator<AONEWeaponCase> It(GetWorld());It;++It) if (IsValid(*It) && !ExtendedCaseShots.Contains(It->GetSourceShotId()))
        {
            if (ExtendedCaseShots.IsEmpty()) FirstBudgetCase=*It;
            ExtendedCaseShots.Add(It->GetSourceShotId()); It->SetLifeSpan(60.f);
        }
        MaximumLive=FMath::Max(MaximumLive,ActualCaseCount());
        if (ActualCaseCount()>W->GetMaximumCases()) { Check(false,TEXT("Actual actor count exceeded configured case cap")); Finish(); break; }
        if (W->GetTotalShotsFired()-Shots>=TargetShots)
        {
            W->SetTrigger(false);
            Check(W->GetTotalShotsFired()-Shots==TargetShots && W->GetEjectionCountForWeapon(0)-Ejections==TargetShots && ExtendedCaseShots.Num()==TargetShots,
                TEXT("Sustained real rifle fire produces one distinct physical case per committed discharge"));
            Check(MaximumLive==W->GetMaximumCases() && ActualCaseCount()==W->GetMaximumCases() &&
                W->GetLiveCaseCount()==W->GetMaximumCases() && !FirstBudgetCase.IsValid(),
                FString::Printf(TEXT("Case cap%d evicts oldest actor before extended lifetime, bounding actual world actors"),W->GetMaximumCases()));
            Check(W->GetAmmo()+W->GetReserveAmmo()==Ammo+Reserve-TargetShots && W->GetAmmoForWeapon(1)==6 && W->GetReserveAmmoForWeapon(1)==36,
                TEXT("Cap stress and natural reload preserve total ammunition and holstered weapon state"));
            Ammo=W->GetAmmo(); Reserve=W->GetReserveAmmo();
            W->ClearEjectedCases(); Next(32);
        } break;
    case 32: if (T>.15f)
    {
        Check(ActualCaseCount()==0 && W->GetLiveCaseCount()==0,TEXT("Explicit cleanup removes all case actors and live tracking entries"));
        Check(W->GetAmmo()==Ammo && W->GetReserveAmmo()==Reserve && W->GetEjectionCountForWeapon(0)==Ejections+TargetShots,
            TEXT("Presentation cleanup preserves ammo and cumulative committed ejection count"));
        Finish();
    } break;
    default: break;
    }
}
