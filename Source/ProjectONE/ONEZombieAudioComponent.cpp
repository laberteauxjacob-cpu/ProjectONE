#include "ONEZombieAudioComponent.h"
#include "ONE05Audio.h"
#include "ONEZombie.h"
#include "ONEInfectedAnimInstance.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "Sound/SoundAttenuation.h"

UONEZombieAudioComponent::UONEZombieAudioComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
}
UAudioComponent* UONEZombieAudioComponent::MakeVoice(const TCHAR* Name,bool Action)
{
    auto* Voice=NewObject<UAudioComponent>(GetOwner(),Name);
    GetOwner()->AddInstanceComponent(Voice);
    if (GetOwner()->GetRootComponent()) Voice->SetupAttachment(GetOwner()->GetRootComponent());
    Voice->bAutoActivate=false; Voice->bAutoDestroy=false; Voice->bStopWhenOwnerDestroyed=true;
    Voice->bAllowSpatialization=true; Voice->bOverrideAttenuation=true;
    Voice->AttenuationOverrides.bAttenuate=true; Voice->AttenuationOverrides.bSpatialize=true;
    Voice->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
    Voice->AttenuationOverrides.AttenuationShapeExtents=FVector(Action?150.f:100.f);
    Voice->AttenuationOverrides.FalloffDistance=Action?1350.f:850.f;
    Voice->bOverridePriority=true; Voice->Priority=Action?2.f:.4f;
    if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>())
        if (auto* Concurrency=Audio->Group(Action?EONE05VoiceGroup::Action:EONE05VoiceGroup::Breath)) Voice->ConcurrencySet.Add(Concurrency);
    Voice->SetRelativeLocation(FVector(0,0,40)); Voice->RegisterComponent();
    return Voice;
}
void UONEZombieAudioComponent::BeginPlay()
{
    Super::BeginPlay(); SeedPresentationRandom();
    BreathVoice=MakeVoice(TEXT("InfectedBreathVoice"),false); ActionVoice=MakeVoice(TEXT("InfectedActionVoice"),true);
    if (auto* Zombie=Cast<AONEZombie>(GetOwner())) if (Zombie->GetMesh()) AddTickPrerequisiteComponent(Zombie->GetMesh());
    UpdateVoiceLocation();
    NextBreath=GetWorld()->GetTimeSeconds()+PresentationRandom.FRandRange(1.5f,5.5f);
}
void UONEZombieAudioComponent::SeedPresentationRandom()
{
    const uint32 OwnerSeed=GetOwner()?GetOwner()->GetUniqueID():0u;
    PresentationRandom.Initialize(static_cast<int32>(HashCombine(OwnerSeed,GetTypeHash(VoiceVariation))));
    bPresentationSeeded=true;
}
void UONEZombieAudioComponent::ConfigureVoiceVariation(int32 Salt)
{
    if (bPresentationSeeded && VoiceVariation==Salt) return;
    VoiceVariation=Salt;
    // Applying an appearance does not restart an audible event or its cooldown.
    // Subsequent choices still use distinct recorded clips at unchanged pitch.
    if (HasBegunPlay()) SeedPresentationRandom();
}
int32 UONEZombieAudioComponent::Choose(int32 Count,int32& Previous)
{
    int32 Result=PresentationRandom.RandRange(1,Count);
    if (Result==Previous && Count>1) Result=Result%Count+1;
    Previous=Result; return Result;
}
void UONEZombieAudioComponent::Play(UAudioComponent* Voice,const FString& Stem,int32 Index,float Gain)
{
    if (!Voice || bShutdown) return;
    auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>(); if (!Audio) return;
    const FName Name(*FString::Printf(TEXT("S_C07_Zombie%s_%02d"),*Stem,Index));
    UpdateVoiceLocation();
    Voice->Stop(); Voice->SetSound(Audio->Sound(Name)); Voice->SetPitchMultiplier(1.f);
    Voice->SetVolumeMultiplier(Gain*ONE05Audio::GetZombieGain());
    if (Voice->Sound) Voice->Play();
}
void UONEZombieAudioComponent::SetPursuing(bool Pursuing)
{
    if (bDead || bShutdown) return;
    if (Pursuing && !bPursuing) NextBreath=FMath::Min(NextBreath,GetWorld()->GetTimeSeconds()+PresentationRandom.FRandRange(.4f,1.6f));
    bPursuing=Pursuing;
}
void UONEZombieAudioComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    if (bShutdown) return;
    UpdateVoiceLocation();
    if (BreathVoice) BreathVoice->SetVolumeMultiplier(BreathGain*ONE05Audio::GetZombieGain());
    if (ActionVoice) ActionVoice->SetVolumeMultiplier(ActionGain*ONE05Audio::GetZombieGain());
    if (bDead) return;
    ObserveAttackFootMotion();
    const double Now=GetWorld()->GetTimeSeconds();
    if (Now>=NextBreath)
    {
        NextBreath=Now+PresentationRandom.FRandRange(bPursuing?2.4f:4.f,bPursuing?4.8f:7.5f);
        if (!ActionVoice || !ActionVoice->IsPlaying())
        {
            BreathGain=bPursuing?.58f:.4f;
            Play(BreathVoice,bPursuing?TEXT("Pursuit"):TEXT("Breath"),bPursuing?Choose(4,LastPursuit):Choose(4,LastBreath),BreathGain);
        }
    }
}
void UONEZombieAudioComponent::NotifyAttack(int32 Variant)
{
    if (!IsLivingAudioEnabled() || GetWorld()->GetTimeSeconds()<NextAttack) return;
    NextAttack=GetWorld()->GetTimeSeconds()+.4; NextHit=GetWorld()->GetTimeSeconds()+.18;
    if (BreathVoice) BreathVoice->Stop();
    ActionGain=.95f; Play(ActionVoice,TEXT("Attack"),FMath::Clamp(Variant,0,2)*2+PresentationRandom.RandRange(1,2),ActionGain);
    ++AttackCueCount;
}
void UONEZombieAudioComponent::NotifyHit(bool Heavy)
{
    if (!IsLivingAudioEnabled() || GetWorld()->GetTimeSeconds()<NextHit) return;
    NextHit=GetWorld()->GetTimeSeconds()+(Heavy?.28:.36);
    // Anticipation remains audible; minor bullets cannot replace an incoming cue.
    if (!Heavy && GetWorld()->GetTimeSeconds()<NextAttack) return;
    if (BreathVoice) BreathVoice->Stop();
    ActionGain=Heavy?.82f:.62f; Play(ActionVoice,Heavy?TEXT("HeavyHit"):TEXT("Hit"),Heavy?Choose(4,LastHeavyHit):Choose(4,LastHit),ActionGain); ++HitCueCount;
}
void UONEZombieAudioComponent::UpdateVoiceLocation()
{
    auto* Zombie=Cast<AONEZombie>(GetOwner()); if (!Zombie || !Zombie->GetMesh()) return;
    auto* Mesh=Zombie->GetMesh(); const FName Bone=Zombie->HasHead()?FName(TEXT("head")):FName(TEXT("spine_02"));
    const FVector Location=Mesh->GetBoneIndex(Bone)!=INDEX_NONE?Mesh->GetSocketLocation(Bone):Mesh->GetComponentLocation();
    if (BreathVoice) BreathVoice->SetWorldLocation(Location);
    if (ActionVoice) ActionVoice->SetWorldLocation(Location);
}
void UONEZombieAudioComponent::NotifyFootContact(bool bLeftFoot)
{
    auto* Zombie=Cast<AONEZombie>(GetOwner()); const int32 I=bLeftFoot?0:1;
    if (!IsLivingAudioEnabled() || !Zombie || !GetWorld() || Zombie->IsDead() || Zombie->IsLivingFallen() || Zombie->IsGettingUp() ||
        !Zombie->GetCharacterMovement()->IsMovingOnGround()) return;
    const auto State=Zombie->GetInfectedAnimationState();
    if (State.Motion!=EONEInfectedMotionState::Locomotion && State.Motion!=EONEInfectedMotionState::Attack) return;
    const double Now=GetWorld()->GetTimeSeconds();
    // The caller already evaluated the planting foot against the ground. An
    // attack brakes before that plant, so instantaneous velocity alone loses
    // its sound. Match the graph's recent-motion window, within this action
    // only; stationary locomotion and an old attack never gain timer footsteps.
    const bool RecentAttackMotion=State.Motion==EONEInfectedMotionState::Attack &&
        State.ActionSerial==FootMotionActionSerial && Now>=LastAttackFootMotion && Now-LastAttackFootMotion<=.22;
    const double SpeedSquared=Zombie->GetVelocity().SizeSquared2D();
    const bool Moving=FMath::IsFinite(SpeedSquared) && SpeedSquared>=FMath::Square(15.f);
    if ((!RecentAttackMotion && !Moving) || Now<NextFoot[I]) return;
    auto* Mesh=Zombie->GetMesh(); const FName Toe=bLeftFoot?FName(TEXT("toe_r")):FName(TEXT("toe_l"));
    const FName Foot=bLeftFoot?FName(TEXT("foot_r")):FName(TEXT("foot_l"));
    const FVector Position=Mesh->GetSocketLocation(Mesh->GetBoneIndex(Toe)!=INDEX_NONE?Toe:Foot);
    FHitResult Hit; FCollisionQueryParams Query(SCENE_QUERY_STAT(ONE07InfectedFoot),false,Zombie); Query.bReturnPhysicalMaterial=true;
    if (GetWorld()->LineTraceSingleByObjectType(Hit,Position+FVector(0,0,12),Position-FVector(0,0,22),
        FCollisionObjectQueryParams(ECC_WorldStatic),Query) && Hit.ImpactNormal.Z>.55f)
    {
        NextFoot[I]=Now+.16;
        if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>()) Audio->PlayFootContact(Hit,false);
        ++FootCueCount;
    }
}
void UONEZombieAudioComponent::ObserveAttackFootMotion()
{
    const auto* Zombie=Cast<AONEZombie>(GetOwner());
    if (!Zombie || !GetWorld() || !Zombie->GetCharacterMovement()->IsMovingOnGround() ||
        Zombie->GetCombatState()!=EONEZombieState::Attack)
    { LastAttackFootMotion=-100.; FootMotionActionSerial=MAX_uint64; return; }
    const auto State=Zombie->GetInfectedAnimationState();
    if (FootMotionActionSerial!=State.ActionSerial)
    { LastAttackFootMotion=-100.; FootMotionActionSerial=State.ActionSerial; }
    const double SpeedSquared=Zombie->GetVelocity().SizeSquared2D();
    if (FMath::IsFinite(SpeedSquared) && SpeedSquared>FMath::Square(8.f)) LastAttackFootMotion=GetWorld()->GetTimeSeconds();
}
void UONEZombieAudioComponent::NotifyFall()
{
    if (!IsLivingAudioEnabled() || GetWorld()->GetTimeSeconds()<NextFall) return;
    NextFall=GetWorld()->GetTimeSeconds()+.6; NextBreath=NextFall+1.;
    if (BreathVoice) BreathVoice->Stop();
    ActionGain=.72f; Play(ActionVoice,TEXT("Fall"),Choose(4,LastFall),ActionGain); ++FallCueCount;
}
void UONEZombieAudioComponent::NotifyBodyContact(const FVector& Location,float NormalImpactSpeedCmPerSec)
{
    if (bShutdown || !FMath::IsFinite(NormalImpactSpeedCmPerSec) || NormalImpactSpeedCmPerSec<65.f ||
        GetWorld()->GetTimeSeconds()<NextBodyContact) return;
    NextBodyContact=GetWorld()->GetTimeSeconds()+.18;
    if (auto* Audio=GetWorld()->GetSubsystem<UONE05AudioWorldSubsystem>())
    { Audio->PlayBodyContact(Location,NormalImpactSpeedCmPerSec); ++BodyCueCount; }
}
void UONEZombieAudioComponent::StopLiving()
{
    bDead=true; bPursuing=false;
    LastAttackFootMotion=-100.; FootMotionActionSerial=MAX_uint64;
    if (BreathVoice) BreathVoice->Stop();
    if (ActionVoice) ActionVoice->Stop();
}
void UONEZombieAudioComponent::NotifyDeath()
{
    if (bDead || bShutdown) return;
    StopLiving(); ActionGain=.82f;
    Play(ActionVoice,TEXT("Death"),Choose(4,LastDeath),ActionGain); ++DeathCueCount;
}
void UONEZombieAudioComponent::Shutdown()
{
    StopLiving(); bShutdown=true; SetComponentTickEnabled(false);
}
int32 UONEZombieAudioComponent::GetActiveVoiceCount() const
{
    return int32(BreathVoice && BreathVoice->IsPlaying())+int32(ActionVoice && ActionVoice->IsPlaying());
}
void UONEZombieAudioComponent::EndPlay(const EEndPlayReason::Type Reason)
{
    Shutdown(); Super::EndPlay(Reason);
}
