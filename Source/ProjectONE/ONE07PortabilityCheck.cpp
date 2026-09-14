#include "ONE07PortabilityCheck.h"
#include "ONEPlayer.h"
#include "ONEGameMode.h"
#include "ONEZombie.h"
#include "ONEHealthComponent.h"
#include "ONEInfectedVariant.h"
#include "ONEInfectedAnimInstance.h"
#include "ONEZombieAudioComponent.h"
#include "ONEPowerUpComponent.h"
#include "ONEBloodSubsystem.h"
#include "ONE07RegionalTrace.h"
#include "ONE06CaptureComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimSequence.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"
#include "CoreGlobals.h"

AONE07PortabilityCheck::AONE07PortabilityCheck()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.bTickEvenWhenPaused=true;
    PrimaryActorTick.TickGroup=TG_PostUpdateWork;
    Capture=CreateDefaultSubobject<UONE06CaptureComponent>(TEXT("OptionalEngineCapture"));
}
void AONE07PortabilityCheck::BeginPlay()
{
    Super::BeginPlay(); StartedReal=FPlatformTime::Seconds(); PhaseAt=GetWorld()->GetTimeSeconds();
    Folder=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir()/TEXT("Candidate07/Portability")/
        (FDateTime::UtcNow().ToString(TEXT("%Y%m%d_%H%M%S"))+TEXT("_")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(8)));
    Check(IFileManager::Get().MakeDirectory(*Folder,true),TEXT("Private portability output directory created"));
    Timeline=TEXT("frame,world_seconds,phase,variant_index,variant,actor_id,state,health,player_health,actor_x,actor_y,actor_z,pelvis_x,pelvis_y,pelvis_z,speed,live,kills,points,eligible_deaths,fall_count,recovery_count,contact_count,physics_bodies,awake_bodies,attack_contacts,attack_damage,foot_cues,attack_cues,hit_cues,fall_cues,body_cues,death_cues,active_voices,head,left_arm,right_arm,left_leg,rebase_cm\n");
    bCaptureRequested=FParse::Param(FCommandLine::Get(),TEXT("ONE06Capture"));
    if (bCaptureRequested)
    {
        bCaptureStarted=Capture->BeginCapture(Folder/TEXT("Media"));
        Check(bCaptureStarted,TEXT("Optional actual viewport and master-audio capture started"));
        Capture->SetPhaseLabel(TEXT("Second-map authored fixture discovery"));
    }
}
void AONE07PortabilityCheck::Check(bool Passed,const FString& Label)
{
    ++Checks; if (!Passed) ++Failures;
    auto Row=MakeShared<FJsonObject>(); Row->SetBoolField(TEXT("pass"),Passed);
    Row->SetNumberField(TEXT("phase"),Phase); Row->SetNumberField(TEXT("variant_index"),VariantIndex);
    Row->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds()); Row->SetStringField(TEXT("label"),Label);
    Assertions.Add(MakeShared<FJsonValueObject>(Row));
    CheckText+=FString::Printf(TEXT("%s | %s\n"),Passed?TEXT("PASS"):TEXT("FAIL"),*Label);
    UE_LOG(LogTemp,Display,TEXT("ONE07_PORTABILITY %s | %s"),Passed?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE07PortabilityCheck::Enter(int32 Next)
{
    Phase=Next; PhaseAt=GetWorld()->GetTimeSeconds();
    const TCHAR* Labels[]={TEXT("Discovery"),TEXT("Normal navigation and attack"),TEXT("Explicit living fall and regional query"),
        TEXT("Actual get-up with absent arm"),TEXT("Recovered pursuit"),TEXT("Fatal arm loss and corpse cut probes"),TEXT("Ordinary presentation cleanup")};
    if (bCaptureStarted && Next>=0 && Next<UE_ARRAY_COUNT(Labels))
        Capture->SetPhaseLabel(FString::Printf(TEXT("Variant %d / %s"),VariantIndex+1,Labels[Next]));
}
bool AONE07PortabilityCheck::PositionPlayer(const FVector& Anchor)
{
    Player->ReleaseHeldInputs(); Player->GetCharacterMovement()->StopMovementImmediately();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE07PortabilityFloor),false,Player);
    if (Subject) Params.AddIgnoredActor(Subject);
    FHitResult Floor;
    const bool Supported=GetWorld()->LineTraceSingleByObjectType(Floor,Anchor+FVector(0,0,350),Anchor-FVector(0,0,350),
        FCollisionObjectQueryParams(ECC_WorldStatic),Params) && Floor.ImpactNormal.Z>.85f;
    Check(Supported,TEXT("Declared player setup has actual supporting map collision"));
    if (!Supported) return false;
    const FVector Position(Floor.ImpactPoint.X,Floor.ImpactPoint.Y,Floor.ImpactPoint.Z+Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2.f);
    const bool Clear=!GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,
        FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(),Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),Params);
    Check(Clear,TEXT("Declared player setup capsule is clear of real map obstacles"));
    if (!Clear) return false;
    Player->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
    Player->GetCapsuleComponent()->UpdateOverlaps(); return true;
}
bool AONE07PortabilityCheck::Discover()
{
    int32 Starts=0,Spawns=0,Anchors=0,Existing=0;
    for (TActorIterator<APlayerStart> It(GetWorld());It;++It) { PlayerAnchor=It->GetActorLocation(); ++Starts; }
    for (TActorIterator<AActor> It(GetWorld());It;++It)
    {
        if (It->ActorHasTag(TEXT("ONE_Spawn"))) { SpawnAnchor=It->GetActorLocation(); ++Spawns; }
        if (It->ActorHasTag(TEXT("ONE06_PickupFixture"))) ++Anchors;
    }
    for (TActorIterator<AONEZombie> It(GetWorld());It;++It) ++Existing;
    const bool Authored=GetWorld()->GetOutermost()->GetName().EndsWith(TEXT("/Portability06")) && Starts==1 && Spawns==1 && Anchors==1;
    Check(Authored,TEXT("Existing Portability06 map supplies one PlayerStart, navigation spawn and pickup marker"));
    Check(Mode->IsSandbox() && Mode->GetClass()==AONEGameMode::StaticClass() && Existing==0 && Mode->GetRemaining()==0,
        TEXT("Ordinary GameMode enters disclosed empty opt-in sandbox without fixture enemy substitution"));
    if (!Authored || Failures) return false;
    RetreatAnchor=PlayerAnchor-(SpawnAnchor-PlayerAnchor).GetSafeNormal2D()*180.f;
    // Read the same production pool used by ordinary infected initialization.
    const auto Paths=AONEZombie::GetProductionVariantPaths();
    Check(Paths.Num()==3,TEXT("Production appearance pool contains exactly the requested three variants"));
    TSet<FName> Ids;
    for (const auto& Path:Paths)
    {
        auto* Variant=LoadObject<UONEInfectedVariant>(nullptr,*Path);
        const bool Unique=Variant && !Variant->VariantId.IsNone() && !Ids.Contains(Variant->VariantId);
        Check(Unique,TEXT("Each production appearance path loads a unique actual variant definition"));
        if (!Unique) return false;
        Ids.Add(Variant->VariantId); Variants.Add(Variant);
    }
    PreviousDropChance=Mode->GetPowerUps()->TotalDropChance;
    Mode->GetPowerUps()->TotalDropChance=0.f; bChangedDropChance=true;
    Check(!Mode->GetPowerUps()->IsActive(EONEPowerUpType::InstaKill) && !Mode->GetPowerUps()->IsActive(EONEPowerUpType::DoublePoints),
        TEXT("No timed modifiers contaminate explicit regional damage and receipt probes"));
    return Failures==0;
}
bool AONE07PortabilityCheck::StartVariant()
{
    if (!Variants.IsValidIndex(VariantIndex) || !PositionPlayer(PlayerAnchor)) return false;
    Player->GetHealthComponent()->Restore(); // Disclosed episode reset, never modifies the infected.
    StartKills=Mode->GetKills(); StartPoints=Mode->GetPoints(); StartEligibleDeaths=Mode->GetPowerUps()->GetDropStats().EligibleDeaths;
    Subject=Mode->SpawnSandboxEnemyAt(SpawnAnchor,Variants[VariantIndex]);
    Check(Subject!=nullptr,TEXT("Normal sandbox navigation and collision checks create the requested registered appearance"));
    if (!Subject) return false;
    SubjectIdentity=Subject->GetUniqueID(); SpawnPosition=Subject->GetActorLocation(); PeakVoices=0;
    bSawGetUp=false; bFallenProbe=false;
    const auto* Anim=Cast<UONEInfectedAnimInstance>(Subject->GetMesh()->GetAnimInstance());
    Check(Subject->GetVariantId()==Variants[VariantIndex]->VariantId && Anim && Anim->HasRequiredClips(),
        TEXT("Spawned variant uses the infected-only graph and all required actual clips"));
    const auto* Variant=Variants[VariantIndex].Get();
    bool Bound=Subject->GetMesh()->GetSkeletalMeshAsset()==Variant->Core.Get() && Subject->GetMesh()->GetPhysicsAsset()==Variant->BodyPhysics.Get();
    const USkeletalMeshComponent* Parts[]={Subject->HeadMesh,Subject->ArmLeftMesh,Subject->ArmRightMesh,Subject->LegLeftMesh};
    const TSoftObjectPtr<USkeletalMesh> Meshes[]={Variant->Head,Variant->ArmLeft,Variant->ArmRight,Variant->LegLeft};
    const TSoftObjectPtr<UPhysicsAsset> Physics[]={Variant->HeadPhysics,Variant->ArmLeftPhysics,Variant->ArmRightPhysics,Variant->LegLeftPhysics};
    for (int32 I=0;I<UE_ARRAY_COUNT(Parts);++I)
        Bound&=Parts[I]->GetSkeletalMeshAsset()!=nullptr && Parts[I]->GetSkeletalMeshAsset()==Meshes[I].Get() &&
            Parts[I]->GetPhysicsAsset()!=nullptr && Parts[I]->GetPhysicsAsset()==Physics[I].Get();
    Check(Bound,TEXT("All five actual modular meshes and physics assets come from this appearance definition"));
    bool ClipsBound=Anim && Variant->Clips.Num()>=14;
    for (const auto& Clip:Variant->Clips) ClipsBound&=Anim && Clip.Value.Get() && Anim->FindClip(Clip.Key)==Clip.Value.Get();
    Check(ClipsBound,TEXT("All declared appearance clips are loaded and bound without presentation fallback"));
    Check(Subject->GetHealth()==112.f && Subject->ShambleSpeed==100.f && Subject->PursuitSpeed==195.f && Subject->AttackDamage==19.f &&
        Subject->HasHead() && Subject->HasLeftArm() && Subject->HasRightArm() && Subject->HasLeftLeg(),
        TEXT("Every appearance starts with the same 112 HP, 100/195 movement, 19 strike and complete anatomy"));
    Check(Subject->ZombieAudio && Subject->ZombieAudio->GetVoiceVariation()==Variants[VariantIndex]->VoiceVariation &&
        Subject->ZombieAudio->IsLivingAudioEnabled() && Mode->GetRemaining()==1,TEXT("Actual appearance audio salt and single registered live identity are present"));
    Enter(1); return Failures==0;
}
EONEWeaponHitOutcome AONE07PortabilityCheck::Probe(EONEHitRegion Region,float Damage,float Trauma)
{
    const auto Context=Mode->BeginCombatDischarge(); LastPacket=FONEWeaponDamagePacket(); LastPacket.ShotId=Context.DischargeId;
    const FName Bone=Region==EONEHitRegion::Head?FName(TEXT("head")):Region==EONEHitRegion::ArmLeft?FName(TEXT("upperarm_r")):
        Region==EONEHitRegion::ArmRight?FName(TEXT("upperarm_l")):Region==EONEHitRegion::LegLeft?FName(TEXT("thigh_r")):FName(TEXT("spine_01"));
    const FVector Direction=(SpawnAnchor-PlayerAnchor).GetSafeNormal2D();
    LastPacket.Get(Region).AddPellet(Damage,Trauma,Subject->GetMesh()->GetSocketLocation(Bone),Direction,-Direction,Bone); LastPacket.Finalize();
    const auto Outcome=Subject->ReceiveWeaponDamageOutcome(LastPacket);
    const int32 Award=Mode->RecordCombatAward(Context,Subject,Outcome,Subject->WasLastKillHeadshot());
    const int32 Expected=Outcome==EONEWeaponHitOutcome::LiveHit?10:Outcome==EONEWeaponHitOutcome::NewKill?110:0;
    Check(Award==Expected,TEXT("Declared regional packet receives the expected ordinary combat receipt award"));
    const float Health=Subject->GetHealth(); const int32 Kills=Mode->GetKills();
    Check(Subject->ReceiveWeaponDamageOutcome(LastPacket)==EONEWeaponHitOutcome::Rejected && Subject->GetHealth()==Health && Mode->GetKills()==Kills &&
        Mode->RecordCombatAward(Context,Subject,Outcome,Subject->WasLastKillHeadshot())==0,TEXT("Repeating the same explicit packet and receipt cannot change health, kill count or points"));
    Mode->EndCombatDischarge(Context); return Outcome;
}
bool AONE07PortabilityCheck::QueryBody(const TCHAR* Label)
{
    auto* Region=Subject->BodyRegion.Get();
    BodyRayStart=Region->GetComponentLocation()-Region->GetRightVector()*80.f;
    BodyRayEnd=Region->GetComponentLocation()+Region->GetRightVector()*80.f;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE07PortabilityRegion),false,Player);
    FHitResult Hit;
    // An explicit component-axis query, not a player cursor or aim-height test.
    const bool HitBody=ONE07RegionalTrace::TraceCapsule(Region,Hit,BodyRayStart,BodyRayEnd,Params) && Hit.GetActor()==Subject && Hit.GetComponent()==Region;
    Check(HitBody,FString(Label)+TEXT(": unchanged production regional query accepts the evaluated body"));
    return HitBody;
}
void AONE07PortabilityCheck::Observe()
{
    if (!IsValid(Subject)) return;
    const auto* Z=Subject.Get(); const auto* A=Z->ZombieAudio.Get();
    const FVector P=Z->GetActorLocation(),Pelvis=Z->GetMesh()->GetSocketLocation(TEXT("pelvis"));
    const int32 Voices=A?A->GetActiveVoiceCount():0; PeakVoices=FMath::Max(PeakVoices,Voices);
    Timeline+=FString::Printf(TEXT("%llu,%.6f,%d,%d,%s,%u,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%d,%d,%d,%llu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%.6f\n"),
        GFrameCounter,GetWorld()->GetTimeSeconds(),Phase,VariantIndex,*Z->GetVariantId().ToString(),Z->GetUniqueID(),int32(Z->GetCombatState()),Z->GetHealth(),Player->GetHealth(),
        P.X,P.Y,P.Z,Pelvis.X,Pelvis.Y,Pelvis.Z,Z->GetVelocity().Size2D(),Mode->GetRemaining(),Mode->GetKills(),Mode->GetPoints(),Mode->GetPowerUps()->GetDropStats().EligibleDeaths,
        Z->GetLivingFallCount(),Z->GetRecoveryCount(),Z->GetPhysicalContactCount(),Z->GetActivePhysicsBodyCount(),Z->GetAwakePhysicsBodyCount(),Z->GetAttackContactAttemptCount(),Z->GetAttackDamageDispatchCount(),
        A?A->GetFootCueCount():0,A?A->GetAttackCueCount():0,A?A->GetHitCueCount():0,A?A->GetFallCueCount():0,A?A->GetBodyCueCount():0,A?A->GetDeathCueCount():0,Voices,
        Z->HasHead(),Z->HasLeftArm(),Z->HasRightArm(),Z->HasLeftLeg(),Z->GetRecoveryRebaseErrorCm()); ++ObservedFrames;
    if (Voices>2 || P.ContainsNaN() || Pelvis.ContainsNaN() || !FMath::IsFinite(Z->GetHealth()))
        Check(false,TEXT("Finite actual body/health and at most two owned infected voices on each observed frame"));
}
void AONE07PortabilityCheck::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); const double Real=FPlatformTime::Seconds(),Now=GetWorld()->GetTimeSeconds();
    if (bFinished) { if (Real-FinishedReal>.5) FPlatformMisc::RequestExit(false); return; }
    if (bFinishing) { Finish(bFinishComplete); return; }
    if (Real-StartedReal>180.) { Check(false,TEXT("Bounded 180-second portability timeout")); Finish(false); return; }
    if (bCaptureRequested && Capture->HasFailed()) { Check(false,TEXT("Optional capture failed: ")+Capture->GetFailureReason()); bCaptureFailure=true; Finish(false); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Mode) Mode=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!Player || !Mode || !Player->GetController()) return;
    if (Failures) { Finish(false); return; }
    if (Player->IsDead() || Mode->IsGameOver()) { Check(false,TEXT("Fixture player remains alive during the disclosed episodes")); Finish(false); return; }
    const double Age=Now-PhaseAt;
    if (Phase==0) { if (Age>1. && (!Discover() || !StartVariant())) Finish(false); return; }
    if (Phase<6 && !IsValid(Subject)) { Check(false,TEXT("Current variant retains its real actor until requested cleanup")); Finish(false); return; }
    Observe();
    if ((Phase==1 && Age>14.) || ((Phase==2 || Phase==3) && Age>25.) || (Phase==4 && Age>8.))
    { Check(false,TEXT("Required navigation, attack or recovery observation did not occur within its gameplay-time bound")); Finish(false); return; }
    if (Phase==1 && Subject->GetAttackDamageDispatchCount()>0)
    {
        Check(Subject->GetAttackDamageDispatchCount()==1 && FMath::IsNearlyEqual(Player->GetHealth(),Player->GetMaxHealth()-19.f,.01f) &&
            FVector::Dist2D(SpawnPosition,Subject->GetActorLocation())>100.f,TEXT("Normal AI approaches over 100 cm and its first real attack dispatch removes exactly 19 player HP"));
        Check(Subject->ZombieAudio->GetFootCueCount()>0 && Subject->ZombieAudio->GetAttackCueCount()>0,
            TEXT("Actual gait and attack-effort events dispatch recorded contact/attack cues on this map"));
        if (!PositionPlayer(RetreatAnchor)) { Finish(false); return; }
        Check(Probe(EONEHitRegion::Body,1.f)==EONEWeaponHitOutcome::LiveHit && FMath::IsNearlyEqual(Subject->GetHealth(),111.f,.01f),
            TEXT("Explicit one-HP body packet uses the living damage receiver without changing health defaults"));
        Check(Probe(EONEHitRegion::ArmLeft,1.f,Subject->ArmSeverThreshold)==EONEWeaponHitOutcome::LiveHit && !Subject->HasLeftArm() && Subject->HasRightArm(),
            TEXT("Explicit actual threshold trauma removes anatomical left arm while the actor lives"));
        FallHealth=Subject->GetHealth(); FallPoints=Mode->GetPoints(); DamageAtFall=Subject->GetAttackDamageDispatchCount();
        Check(Subject->TryLivingFall((SpawnAnchor-PlayerAnchor).GetSafeNormal2D()*220.f+FVector(0,0,15),TEXT("explicit_second_map_probe")),
            TEXT("Declared bounded impulse enters the real living-fall state"));
        Check(Subject->IsLivingFallen() && Subject->GetHealth()==FallHealth && Subject->GetUniqueID()==SubjectIdentity && Mode->GetRemaining()==1 &&
            Mode->GetKills()==StartKills && Mode->GetPoints()==FallPoints && Mode->GetPowerUps()->GetDropStats().EligibleDeaths==StartEligibleDeaths,
            TEXT("Fall preserves health and identity with no points, death roll or live-population removal"));
        Check(Subject->GetRagdollTransitionErrorCm()<.1f,TEXT("Actual sampled fall handoff preserves evaluated pose within 0.1 cm")); Enter(2);
    }
    else if (Phase==2 && Age>.5)
    {
        Check(Subject->IsLivingFallen() && QueryBody(TEXT("Living fallen")),TEXT("Living physical body is queryable before recovery"));
        Check(Probe(EONEHitRegion::Body,1.f)==EONEWeaponHitOutcome::LiveHit && FMath::IsNearlyEqual(Subject->GetHealth(),FallHealth-1.f,.01f),
            TEXT("Living fallen receiver accepts real health loss without death"));
        FallHealth=Subject->GetHealth(); bFallenProbe=true; Enter(3);
    }
    else if (Phase==3)
    {
        bSawGetUp|=Subject->IsGettingUp();
        if (Subject->GetHealth()!=FallHealth || Subject->GetUniqueID()!=SubjectIdentity || Subject->HasLeftArm() || Subject->GetAttackDamageDispatchCount()!=DamageAtFall ||
            Mode->GetKills()!=StartKills || Mode->GetPowerUps()->GetDropStats().EligibleDeaths!=StartEligibleDeaths)
        { Check(false,TEXT("Every fall/get-up observation retains injured identity, absent arm and canceled pending attack")); Finish(false); return; }
        if (Subject->GetRecoveryCount()>0)
        {
            Check(bFallenProbe && bSawGetUp && Subject->GetRecoveryCount()==1 && !Subject->IsLivingFallen() && !Subject->IsGettingUp() &&
                Subject->GetCharacterMovement()->IsMovingOnGround() && Subject->GetCapsuleComponent()->GetCollisionEnabled()==ECollisionEnabled::QueryAndPhysics,
                TEXT("Actual get-up completes once into the normal grounded walking capsule"));
            Check(Subject->GetRecoveryRebaseErrorCm()<.5f && Subject->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft)==0 &&
                Subject->ArmLeftRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision && Subject->UpperArmLeftRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision,
                TEXT("Recovery preserves snapshot placement and leaves missing-arm query/physical bodies absent"));
            SpawnPosition=Subject->GetActorLocation(); Enter(4);
        }
    }
    else if (Phase==4 && FVector::Dist2D(SpawnPosition,Subject->GetActorLocation())>30.f)
    {
        Check(Subject->GetVelocity().Size2D()>15.f && Subject->GetHealth()==FallHealth,TEXT("Recovered injured variant resumes actual navigation toward the player"));
        Check(Probe(EONEHitRegion::ArmRight,1.f,Subject->ArmSeverThreshold)==EONEWeaponHitOutcome::NewKill && Subject->IsDead() &&
            !Subject->HasLeftArm() && !Subject->HasRightArm() && Mode->GetKills()==StartKills+1 && Mode->GetRemaining()==0,
            TEXT("Second supported arm loss follows the ordinary fatal-loss rule and one registered death"));
        Check(Mode->GetPowerUps()->GetDropStats().EligibleDeaths==StartEligibleDeaths+1 && Mode->GetPoints()==StartPoints+140,
            TEXT("Three live impacts and one body-classified fatal loss award 140 total points and one death eligibility event"));
        Check(Probe(EONEHitRegion::Head,1.f,Subject->HeadSeverThreshold)==EONEWeaponHitOutcome::CorpseHit &&
            Probe(EONEHitRegion::LegLeft,1.f,Subject->LegSeverThreshold)==EONEWeaponHitOutcome::CorpseHit && !Subject->HasHead() && !Subject->HasLeftLeg(),
            TEXT("Corpse packets exercise the remaining head and supported leg cut without resurrection"));
        Enter(5);
    }
    else if (Phase==5 && Age>1.)
    {
        Check(QueryBody(TEXT("Dead")) && Subject->GetHealth()==0.f && Subject->GetRecoveryCount()==1 && Subject->GetSeverCount()==4 && Mode->GetKills()==StartKills+1 &&
            Mode->GetPoints()==StartPoints+140 && Mode->GetPowerUps()->GetDropStats().EligibleDeaths==StartEligibleDeaths+1,
            TEXT("Dead evaluated body retains all four cuts without extra health, recovery, kills, points or death rolls"));
        Check(Subject->HeadRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision && Subject->UpperArmRightRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision &&
            Subject->ArmRightRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision && Subject->UpperLegLeftRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision &&
            Subject->LegLeftRegion->GetCollisionEnabled()==ECollisionEnabled::NoCollision && Subject->GetRegionPhysicsBodyCount(EONEHitRegion::Head)==0 &&
            Subject->GetRegionPhysicsBodyCount(EONEHitRegion::ArmRight)==0 && Subject->GetRegionPhysicsBodyCount(EONEHitRegion::LegLeft)==0,
            TEXT("Removed anatomy has no surviving invisible regional query or physical chain"));
        auto* Audio=Subject->ZombieAudio.Get();
        Check(Audio->GetHitCueCount()>0 && Audio->GetFallCueCount()==1 && Audio->GetDeathCueCount()==1 && !Audio->IsLivingAudioEnabled() && PeakVoices>0 && PeakVoices<=2,
            TEXT("Real hit, fall and one death event reached audio with bounded playing voices; living cues stop at death"));
        auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>();
        Check(Blood && Blood->GetCorpseCount()==1 && Blood->GetPieceCount()==4,TEXT("Ordinary blood subsystem owns one corpse and four actual detached parts"));
        RetiredAudio=Audio; RetiredBody=Subject->BodyRegion; Mode->ClearSandboxPresentation(); Enter(6);
    }
    else if (Phase==6 && Age>.5)
    {
        auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>(); int32 Actors=0;
        for (TActorIterator<AONEZombie> It(GetWorld());It;++It) ++Actors;
        Check(!IsValid(Subject) && Actors==0 && Mode->GetRemaining()==0 && Blood && Blood->GetCorpseCount()==0 && Blood->GetPieceCount()==0 && Blood->GetWoundCount()==0 && Blood->GetDropletCount()==0,
            TEXT("Ordinary presentation clear retires corpse, detached parts, wound sources and live registrations"));
        Check((!RetiredAudio || (!RetiredAudio->IsRegistered() && !RetiredAudio->IsLivingAudioEnabled() && RetiredAudio->GetActiveVoiceCount()==0)) &&
            (!RetiredBody || !RetiredBody->IsRegistered()),TEXT("Retirement unregisters old queries and stops all owned infected voices"));
        FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE07PortabilityRetired),false,Player); FHitResult Hit;
        Check(!ONE07RegionalTrace::TraceWeaponSegment(GetWorld(),Hit,BodyRayStart,BodyRayEnd,Params) || !Cast<AONEZombie>(Hit.GetActor()),
            TEXT("The original dead-body ray cannot hit an invisible retired infected"));
        ++CompletedVariants; ++VariantIndex; Subject=nullptr; RetiredAudio=nullptr; RetiredBody=nullptr;
        if (VariantIndex==Variants.Num()) Finish(true); else if (!StartVariant()) Finish(false);
    }
}
void AONE07PortabilityCheck::Finish(bool Complete)
{
    if (bFinished) return;
    if (!bFinishing)
    {
        bFinishing=true; bFinishComplete=Complete;
        if (Player) Player->ReleaseHeldInputs();
        if (IsValid(Subject)) Subject->Destroy();
        if (Mode) Mode->ClearSandboxPresentation();
        if (bChangedDropChance && Mode) { Mode->GetPowerUps()->TotalDropChance=PreviousDropChance; bChangedDropChance=false; }
    }
    if (bCaptureRequested && Capture->HasFailed())
    {
        if (!bCaptureFailure) { Check(false,TEXT("Optional capture failed: ")+Capture->GetFailureReason()); bCaptureFailure=true; }
        bFinishComplete=false;
    }
    else if (bCaptureStarted)
    {
        if (!Capture->FinishCaptureAndWait()) return;
        Check(Capture->IsComplete() && Capture->GetFrameCount()>0,TEXT("Actual optional images and audio drained before fixture exit"));
    }
    WriteResults(bFinishComplete);
}
void AONE07PortabilityCheck::WriteResults(bool Complete)
{
    if (bFinished) return; bFinished=true; FinishedReal=FPlatformTime::Seconds();
    Check(!Complete || CompletedVariants==3,TEXT("Completion requires all three full appearance episodes"));
    Check(FFileHelper::SaveStringToFile(Timeline,*(Folder/TEXT("observations.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM),TEXT("Actual observed-frame timeline saved"));
    auto Report=MakeShared<FJsonObject>(); Report->SetStringField(TEXT("schema"),TEXT("one07.portability.v1"));
    Report->SetBoolField(TEXT("complete"),Complete); Report->SetNumberField(TEXT("variants_completed"),CompletedVariants);
    Report->SetStringField(TEXT("map"),GetWorld()->GetOutermost()->GetName());
    Report->SetStringField(TEXT("method"),TEXT("Three production appearances forced individually before normal sandbox spawn initialization, using this map's authored PlayerStart/spawn and actual floor/navigation. Pursuit and attack run normally. Setup teleports/restores only the player once per episode and retreats after the first strike. Damage/trauma/receipts, component-axis regional queries and one bounded fall are explicit probes, not weapon discharges, player aim or natural knockdowns. Drop chance is temporarily zero while actual death eligibility is counted, then restored. Ordinary ClearSandboxPresentation drives cleanup. Audio event/playing-voice observations are not an audition. No native input, continuous playback, visual acceptance or performance claim."));
    Report->SetNumberField(TEXT("checks"),Checks); Report->SetNumberField(TEXT("failures"),Failures); Report->SetArrayField(TEXT("assertions"),Assertions);
    Report->SetNumberField(TEXT("observed_frames"),ObservedFrames); Report->SetNumberField(TEXT("elapsed_real_seconds"),FinishedReal-StartedReal);
    Report->SetBoolField(TEXT("capture_requested"),bCaptureRequested); Report->SetNumberField(TEXT("captured_frames"),Capture->GetFrameCount());
    Report->SetBoolField(TEXT("capture_complete"),bCaptureStarted && Capture->IsComplete());
    TArray<TSharedPtr<FJsonValue>> Names;
    for (const auto& Variant:Variants) Names.Add(MakeShared<FJsonValueString>(Variant->VariantId.ToString())); Report->SetArrayField(TEXT("variants"),Names);
    FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json));
    if (!FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("checks.json")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) Check(false,TEXT("Portability JSON could not be saved"));
    if (!FFileHelper::SaveStringToFile(CheckText,*(Folder/TEXT("checks.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)) { ++Failures; UE_LOG(LogTemp,Error,TEXT("ONE07_PORTABILITY FAIL | Assertions could not be saved")); }
    UE_LOG(LogTemp,Display,TEXT("ONE07_PORTABILITY_COMPLETE complete=%d variants=%d checks=%d failures=%d observed_frames=%d frames=%d"),Complete,CompletedVariants,Checks,Failures,ObservedFrames,Capture->GetFrameCount());
}
void AONE07PortabilityCheck::EndPlay(const EEndPlayReason::Type Reason)
{
    if (!bFinished) { Check(false,TEXT("Portability fixture interrupted before completion")); Finish(false); if (!bFinished) WriteResults(false); }
    Super::EndPlay(Reason);
}
