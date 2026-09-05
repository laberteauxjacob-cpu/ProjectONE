#include "ONE03DamageCheck.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEGameMode.h"
#include "ONEWeaponComponent.h"
#include "ONEBloodSubsystem.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

AONE03DamageCheck::AONE03DamageCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
}
void AONE03DamageCheck::BeginPlay()
{
    Super::BeginPlay(); StartReal=FPlatformTime::Seconds(); StageStart=GetWorld()->GetTimeSeconds();
    Report=TEXT("Candidate03 Stage D regional damage integration\nRegistered sandbox actors, actual anatomical collision traces, production packet resolution and authored attack timing. Manual region packets isolate trauma/score boundaries; they do not claim a particular shotgun spread landed those regions. Physical fidelity and rendered wound review are separate.\n\n");
}
void AONE03DamageCheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    Report+=FString::Printf(TEXT("%s | %s\n"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE03_DAMAGE %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE03DamageCheck::Next(int32 NewStage)
{ Stage=NewStage; StageStart=GetWorld()->GetTimeSeconds(); }
void AONE03DamageCheck::Finish()
{
    if (bFinished) return;
    bFinished=true; FinishReal=FPlatformTime::Seconds();
    if (Player) Player->ReleaseHeldInputs();
    Report+=FString::Printf(TEXT("\nChecks: %d\nFailures: %d\n"),Checks,Failures);
    const FString Folder=FPaths::ProjectSavedDir()/TEXT("Candidate03/Damage");
    IFileManager::Get().MakeDirectory(*Folder,true);
    if (!FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("checks.txt")))) ++Failures;
    UE_LOG(LogTemp,Display,TEXT("ONE03_DAMAGE_COMPLETE failures=%d checks=%d"),Failures,Checks);
}
AONEZombie* AONE03DamageCheck::SpawnFixture(const FVector& Offset,bool Frozen)
{
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    AONEZombie* Zombie=GM ? GM->SpawnSandboxEnemyAt(Player->GetActorLocation()+Offset) : nullptr;
    Check(Zombie!=nullptr,TEXT("Regional fixture uses a registered reachable sandbox enemy"));
    if (Zombie && Frozen)
    { Zombie->SetActorTickEnabled(false); Zombie->GetCharacterMovement()->StopMovementImmediately(); }
    return Zombie;
}
FONEWeaponDamagePacket AONE03DamageCheck::MakePacket(AONEZombie* Zombie,EONEHitRegion Region,float Damage,float Trauma,FName Bone)
{
    FONEWeaponDamagePacket Packet; Packet.ShotId=++NextShot;
    const FVector Point=Zombie->GetMesh()->GetSocketLocation(Bone);
    Packet.Get(Region).AddPellet(Damage,Trauma,Point,FVector::ForwardVector,-FVector::ForwardVector,Bone);
    Packet.Finalize(); return Packet;
}
bool AONE03DamageCheck::TraceArm(AONEZombie* Zombie,bool Left,FHitResult& Hit) const
{
    const FVector Bone=Zombie->GetMesh()->GetSocketLocation(Left?TEXT("upperarm_r"):TEXT("upperarm_l"));
    const FVector Out=Zombie->GetActorRightVector()*(Left?-1.f:1.f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE03AnatomicalArm),false,Player.Get());
    return GetWorld()->LineTraceSingleByChannel(Hit,Bone+Out*100.f,Bone,ECC_Visibility,Params) && Hit.GetActor()==Zombie;
}
void AONE03DamageCheck::CheckTransactions()
{
    if (!Target) { Finish(); return; }
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    FHitResult LeftTrace,RightTrace;
    for (EONEHitRegion Region:{EONEHitRegion::LegLeft,EONEHitRegion::LegRight})
    {
        Check(Target->GetLegQueryCoverageErrorCm(Region)<.01f,
            FString::Printf(TEXT("Leg region %d has continuous hip-knee-ankle query coverage"),int32(Region)));
        const bool Left=Region==EONEHitRegion::LegLeft;
        const FVector Hip=Target->GetMesh()->GetSocketLocation(Left?TEXT("thigh_r"):TEXT("thigh_l"));
        const FVector Knee=Target->GetMesh()->GetSocketLocation(Left?TEXT("calf_r"):TEXT("calf_l"));
        const FVector Ankle=Target->GetMesh()->GetSocketLocation(Left?TEXT("foot_r"):TEXT("foot_l"));
        const FVector Out=Target->GetActorRightVector()*(Left?-1.f:1.f);
        int32 Sample=0;
        for (const FVector& Point:{FMath::Lerp(Hip,Knee,.6f),Knee,FMath::Lerp(Knee,Ankle,.5f)})
        {
            FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE03LegCoverage),false,Player.Get());
            const bool HitLeg=GetWorld()->LineTraceSingleByChannel(Hit,Point+Out*100.f,Point,ECC_Visibility,Params) && Hit.GetActor()==Target && Target->GetHitRegion(Hit)==Region;
            Check(HitLeg,FString::Printf(TEXT("Actual leg trace region %d sample %d hits its own query: %s"),int32(Region),Sample++,*GetNameSafe(Hit.GetComponent())));
        }
    }
    Check(Target->GetReferenceLegQuerySeparationCm()>0.f,
        FString::Printf(TEXT("Opposite leg query segments remain separate in reference pose: %.3fcm"),Target->GetReferenceLegQuerySeparationCm()));
    const bool LeftHit=TraceArm(Target,true,LeftTrace),RightHit=TraceArm(Target,false,RightTrace);
    Check(LeftHit && Target->GetHitRegion(LeftTrace)==EONEHitRegion::ArmLeft,
        FString::Printf(TEXT("Actual trace at source_r resolves anatomical left; hit=%s region=%d"),*GetNameSafe(LeftTrace.GetComponent()),int32(Target->GetHitRegion(LeftTrace))));
    Check(RightHit && Target->GetHitRegion(RightTrace)==EONEHitRegion::ArmRight,
        FString::Printf(TEXT("Actual trace at source_l resolves anatomical right; hit=%s region=%d"),*GetNameSafe(RightTrace.GetComponent()),int32(Target->GetHitRegion(RightTrace))));
    const int32 StartPoints=GM->GetPoints(),StartKills=GM->GetKills();
    auto Packet=MakePacket(Target,EONEHitRegion::ArmLeft,50,Target->ArmSeverThreshold,TEXT("upperarm_r"));
    Target->ReceiveWeaponDamage(Packet);
    Check(!Target->HasLeftArm() && Target->HasRightArm() && !Target->IsDead() && FMath::IsNearlyEqual(Target->GetHealth(),92.f),
        TEXT("Left-arm trauma removes exactly left anatomy, contributes .4 health damage and preserves life"));
    FHitResult Missing; Missing.BoneName=TEXT("upperarm_r");
    Check(Target->GetHitRegion(Missing)==EONEHitRegion::Invalid,TEXT("Removed left-arm bone cannot fall through into torso damage"));
    const int32 BeforeTransactions=Target->GetDamageTransactionCount(),BeforeSevers=Target->GetSeverCount();
    const float BeforeHealth=Target->GetHealth();
    Packet.Get(EONEHitRegion::Body).AddPellet(1000,0,Target->GetActorLocation(),FVector::ForwardVector,-FVector::ForwardVector,TEXT("spine_02")); Packet.Finalize();
    Check(!Target->ReceiveWeaponDamage(Packet) && Target->GetHealth()==BeforeHealth && Target->GetSeverCount()==BeforeSevers && Target->GetDamageTransactionCount()==BeforeTransactions && GM->GetPoints()==StartPoints,
        TEXT("Duplicate discharge ID blocks added torso damage, severing and scoring"));
    Packet=MakePacket(Target,EONEHitRegion::ArmLeft,1000,1000,TEXT("upperarm_r"));
    Check(!Target->ReceiveWeaponDamage(Packet) && Target->GetHealth()==BeforeHealth,TEXT("Fresh missing-region-only packet is a complete no-op"));
    Packet=MakePacket(Target,EONEHitRegion::ArmLeft,1000,1000,TEXT("upperarm_r"));
    Packet.Get(EONEHitRegion::Body).AddPellet(10,0,Target->GetMesh()->GetSocketLocation(TEXT("spine_02")),FVector::ForwardVector,-FVector::ForwardVector,TEXT("spine_02")); Packet.Finalize();
    Check(Target->ReceiveWeaponDamage(Packet) && FMath::IsNearlyEqual(Target->GetHealth(),BeforeHealth-10.f) && Target->GetSeverCount()==BeforeSevers,
        TEXT("Mixed absent-left and intact-body packet accepts only the valid torso damage"));
    Packet=MakePacket(Target,EONEHitRegion::ArmRight,50,Target->ArmSeverThreshold,TEXT("upperarm_l"));
    Target->ReceiveWeaponDamage(Packet);
    Check(Target->IsDead() && !Target->HasLeftArm() && !Target->HasRightArm() && Target->HasHead() && Target->HasLeftLeg(),TEXT("Loss of the second arm follows the selected fatal policy"));
    Check(GM->GetPoints()==StartPoints+100 && GM->GetKills()==StartKills+1,TEXT("Both-arm fatal transition awards exactly one registered kill and100 points"));
    const int32 LiveTransactions=Target->GetDamageTransactionCount(),CorpseTransactions=Target->GetCorpseTransactionCount();
    Packet=MakePacket(Target,EONEHitRegion::Head,32,Target->HeadSeverThreshold,TEXT("head"));
    Target->ReceiveWeaponDamage(Packet);
    Check(!Target->HasHead() && Target->GetHealth()==0 && Target->GetDamageTransactionCount()==LiveTransactions && Target->GetCorpseTransactionCount()==CorpseTransactions+1 && GM->GetPoints()==StartPoints+100 && GM->GetKills()==StartKills+1,
        TEXT("Fresh corpse head sever is cosmetic: no health, live transaction or second kill award"));
    const int32 CorpseSevers=Target->GetSeverCount();
    Check(!Target->ReceiveWeaponDamage(Packet) && Target->GetSeverCount()==CorpseSevers && Target->GetCorpseTransactionCount()==CorpseTransactions+1,
        TEXT("Corpse discharge replay cannot repeat severing or cosmetic transaction"));

    AONEZombie* Mixed=SpawnFixture(FVector(340,0,0));
    if (Mixed)
    {
        const int32 Points=GM->GetPoints(),Kills=GM->GetKills();
        Packet=MakePacket(Mixed,EONEHitRegion::Head,16,Mixed->HeadSeverThreshold,TEXT("head"));
        for (EONEHitRegion Region:{EONEHitRegion::ArmLeft,EONEHitRegion::ArmRight,EONEHitRegion::LegLeft})
        {
            const bool Leg=Region==EONEHitRegion::LegLeft;
            const FName Bone=Leg?TEXT("thigh_r"):Region==EONEHitRegion::ArmLeft?TEXT("upperarm_r"):TEXT("upperarm_l");
            Packet.Get(Region).AddPellet(1,Leg?Mixed->LegSeverThreshold:Mixed->ArmSeverThreshold,Mixed->GetMesh()->GetSocketLocation(Bone),FVector::ForwardVector,-FVector::ForwardVector,Bone);
        }
        Packet.Finalize(); Mixed->ReceiveWeaponDamage(Packet);
        Check(Mixed->IsDead() && !Mixed->HasHead() && !Mixed->HasLeftArm() && !Mixed->HasRightArm() && !Mixed->HasLeftLeg() && Mixed->GetSeverCount()==4 && Mixed->GetDamageTransactionCount()==1,
            TEXT("One mixed packet resolves all four eligible regions from the pre-shot mask before one death"));
        Check(GM->GetPoints()==Points+100 && GM->GetKills()==Kills+1,TEXT("Four simultaneous severs produce exactly one registered death award"));
        Check(!Mixed->ReceiveWeaponDamage(Packet) && Mixed->GetSeverCount()==4 && GM->GetPoints()==Points+100,TEXT("Mixed live-to-dead replay preserves all four detachments and one award"));
    }
    AONEZombie* Leg=SpawnFixture(FVector(0,-240,0));
    if (Leg)
    {
        Packet=MakePacket(Leg,EONEHitRegion::LegRight,7,1000,TEXT("calf_l")); Leg->ReceiveWeaponDamage(Packet);
        Check(!Leg->IsDead() && Leg->HasLeftLeg() && Leg->GetSeverCount()==0 && Leg->GetHealth()>0 && Leg->GetHealth()<Leg->GetHealthComponent()->MaxHealth,
            TEXT("Right leg is a valid damage region without detaching the supported opposite left leg"));
        const int32 Points=GM->GetPoints(),Kills=GM->GetKills();
        Packet=MakePacket(Leg,EONEHitRegion::LegLeft,1,Leg->LegSeverThreshold,TEXT("thigh_r")); Leg->ReceiveWeaponDamage(Packet);
        Check(Leg->IsDead() && !Leg->HasLeftLeg() && Leg->HasHead() && Leg->HasLeftArm() && Leg->HasRightArm() && GM->GetPoints()==Points+100 && GM->GetKills()==Kills+1,
            TEXT("Left-leg loss alone follows the selected fatal policy and awards one kill"));
    }
}
void AONE03DamageCheck::Tick(float Dt)
{
    Super::Tick(Dt);
    const double Real=FPlatformTime::Seconds();
    if (bFinished) { if (Real-FinishReal>.5) FPlatformMisc::RequestExit(false); return; }
    if (Real-StartReal>40) { Check(false,FString::Printf(TEXT("Regional scenario timed out stage%d"),Stage)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>(); if (!Player || !GM) return;
    // Sandbox disables scheduled waves; its spawn cap must remain positive for
    // the registered fixtures used here.
    const float T=GetWorld()->GetTimeSeconds()-StageStart;
    switch (Stage)
    {
    case 0: if (T>1.f)
    {
        Check(GM->IsSandbox(),TEXT("Regional check uses isolated production sandbox"));
        Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately(); Player->Health->Restore();
        Player->SetActorLocation(FVector(-250,300,98)); Next(1);
    } break;
    case 1: if (T>.5f) { Target=SpawnFixture(FVector(220,0,0)); Next(2); } break;
    case 2: if (T>.5f)
    {
        CheckTransactions(); if (bFinished) return;
        Target=SpawnFixture(FVector(80,0,0),false); Player->Health->Restore(); Next(3);
    } break;
    case 3: if (T>.3f)
    {
        if (!Target) { Finish(); return; }
        auto Packet=MakePacket(Target,EONEHitRegion::ArmLeft,50,Target->ArmSeverThreshold,TEXT("upperarm_r"));
        Target->ReceiveWeaponDamage(Packet); Player->Health->Restore(); Next(4);
    } break;
    case 4: if (T>2.6f)
    {
        Check(Target && !Target->IsDead() && !Target->HasLeftArm() && Target->HasRightArm() && Player->GetHealth()<Player->GetMaxHealth(),TEXT("Actual remaining right arm completes timed contact after left-arm loss"));
        if (Target) Target->Destroy(); Player->Health->Restore();
        Target=SpawnFixture(FVector(80,0,0),false); Next(5);
    } break;
    case 5: if (T>.3f)
    {
        if (!Target) { Finish(); return; }
        auto Packet=MakePacket(Target,EONEHitRegion::ArmRight,50,Target->ArmSeverThreshold,TEXT("upperarm_l"));
        Target->ReceiveWeaponDamage(Packet); Player->Health->Restore(); Next(6);
    } break;
    case 6: if (T>2.6f)
    {
        Check(Target && !Target->IsDead() && Target->HasLeftArm() && !Target->HasRightArm() && Player->GetHealth()<Player->GetMaxHealth(),TEXT("Actual remaining left arm completes timed contact after right-arm loss"));
        if (Target) Target->Destroy(); Player->Health->Restore();
        Target=SpawnFixture(FVector(220,0,0)); Next(7);
    } break;
    case 7: if (T>.3f)
    {
        if (!Target) { Finish(); return; }
        auto Packet=MakePacket(Target,EONEHitRegion::Body,1000,0,TEXT("spine_02"));
        Target->ReceiveWeaponDamage(Packet);
        Check(Target->IsDead() && Target->IsRagdollActive() && Target->HasLeftLeg(),TEXT("Late-sever fixture enters genuine intact-leg ragdoll"));
        SettledFor=0.f; Next(8);
    } break;
    case 8:
    {
        if (!Target) { Check(false,TEXT("Late-sever corpse remained available")); Finish(); return; }
        int32 Bodies=0; float Speed=0.f,AngularSpeed=0.f; bool Finite=true;
        for (FBodyInstance* Body:Target->GetMesh()->Bodies)
            if (Body && Body->IsValidBodyInstance() && Body->IsInstanceSimulatingPhysics())
            {
                ++Bodies;
                const FVector V=Body->GetUnrealWorldVelocity(),W=Body->GetUnrealWorldAngularVelocityInRadians();
                if (V.ContainsNaN() || W.ContainsNaN()) { Finite=false; continue; }
                Speed=FMath::Max(Speed,float(V.Size())); AngularSpeed=FMath::Max(AngularSpeed,float(W.Size()));
            }
        const bool Rest=Bodies>0 && Finite && Speed<5.f && AngularSpeed<.12f;
        SettledFor=Rest ? SettledFor+Dt : 0.f;
        if ((T>1.f && SettledFor>=.5f) || T>8.f)
        {
            Check(SettledFor>=.5f,FString::Printf(TEXT("Late sever waits for sustained actual ragdoll rest; speed %.3fcm/s angular %.3frad/s stable %.3fs"),Speed,AngularSpeed,SettledFor));
            LateThighWorld=Target->GetMesh()->GetSocketTransform(TEXT("thigh_r"),RTS_World);
            LateThighInPelvis=LateThighWorld.GetRelativeTransform(Target->GetMesh()->GetSocketTransform(TEXT("pelvis"),RTS_World));
            LatePoints=GM->GetPoints(); LateKills=GM->GetKills(); LateTransactions=Target->GetDamageTransactionCount();
            LateCorpseTransactions=Target->GetCorpseTransactionCount(); LateSevers=Target->GetSeverCount();
            LatePacket=MakePacket(Target,EONEHitRegion::LegLeft,1,Target->LegSeverThreshold,TEXT("thigh_r"));
            Target->ReceiveWeaponDamage(LatePacket);
            if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) LatePiece=Blood->GetLastDetachedPiece();
            Check(LatePiece.IsValid() && LatePiece->GetActivePhysicsBodyCount()==3 && LatePiece->GetTransitionErrorCm()<.5f,
                TEXT("Late left-leg detachment creates a fitted three-body physical chain"));
            if (LatePiece.IsValid())
            {
                const FTransform Captured=LatePiece->GetCapturedSourceBoneTransform();
                Check(Captured.GetLocation().Equals(LateThighWorld.GetLocation(),.5f) && FMath::RadiansToDegrees(Captured.GetRotation().AngularDistance(LateThighWorld.GetRotation()))<.5f,
                    TEXT("Late detached leg captures the current settled thigh transform rather than the original death pose"));
            }
            Next(9);
        }
    } break;
    case 9: if (T>.2f)
    {
        const FTransform Now=Target->GetMesh()->GetSocketTransform(TEXT("thigh_r"),RTS_World).GetRelativeTransform(Target->GetMesh()->GetSocketTransform(TEXT("pelvis"),RTS_World));
        const float PositionError=FVector::Distance(Now.GetLocation(),LateThighInPelvis.GetLocation());
        const float AngleError=FMath::RadiansToDegrees(Now.GetRotation().AngularDistance(LateThighInPelvis.GetRotation()));
        Check(PositionError<.5f && AngleError<.5f,FString::Printf(TEXT("Retained stump preserves settled thigh-to-pelvis pose after physics update; %.4fcm %.4fdeg"),PositionError,AngleError));
        Check(Target->GetRegionPhysicsBodyCount(EONEHitRegion::LegLeft)==0 && !Target->HasLeftLeg() && Target->GetStumpCollisionFitErrorCm()<.5f,
            TEXT("Late corpse transfer removes every distal collider and fits proximal stump collision to the captured pose"));
        Check(Target->GetHealth()==0.f && GM->GetPoints()==LatePoints && GM->GetKills()==LateKills && Target->GetDamageTransactionCount()==LateTransactions && Target->GetCorpseTransactionCount()==LateCorpseTransactions+1 && Target->GetSeverCount()==LateSevers+1,
            TEXT("Late settled-corpse leg loss is one cosmetic transaction without health or duplicate award"));
        Check(!Target->ReceiveWeaponDamage(LatePacket) && Target->GetCorpseTransactionCount()==LateCorpseTransactions+1 && Target->GetSeverCount()==LateSevers+1,
            TEXT("Late leg transaction replay cannot create another part or cosmetic transaction"));
        Finish();
    } break;
    default: break;
    }
}
