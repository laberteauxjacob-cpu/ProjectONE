#include "ONEWeaponComponent.h"
#include "ONEWeaponCatalog.h"
#include "ONEWeaponMagazine.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponCase.h"
#include "ONEBloodSubsystem.h"
#include "Animation/AnimSequence.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "CoreGlobals.h"

namespace
{
    uint64 NextDischargeId=0;
    uint64 NextMagazineReleaseId=0;
    USoundBase* Choose(const TArray<TSoftObjectPtr<USoundBase>>& Sounds)
    { return Sounds.IsEmpty() ? nullptr : Sounds[FMath::RandHelper(Sounds.Num())].Get(); }
    FName RegionalImpactBone(const AONEZombie* Zombie,EONEHitRegion Region,const FHitResult& Hit)
    {
        // Query-only region primitives do not supply a skeletal BoneName. Find
        // the nearest evaluated bone within that region, never the other limb.
        if (!Hit.BoneName.IsNone()) return Hit.BoneName;
        const FName Body[]={TEXT("pelvis"),TEXT("spine_01"),TEXT("spine_02"),TEXT("neck")};
        const FName Head[]={TEXT("head")};
        const FName LeftArm[]={TEXT("upperarm_r"),TEXT("lowerarm_r"),TEXT("hand_r")};
        const FName RightArm[]={TEXT("upperarm_l"),TEXT("lowerarm_l"),TEXT("hand_l")};
        const FName LeftLeg[]={TEXT("thigh_r"),TEXT("calf_r"),TEXT("foot_r")};
        const FName RightLeg[]={TEXT("thigh_l"),TEXT("calf_l"),TEXT("foot_l")};
        const FName* Bones=Body; int32 Count=UE_ARRAY_COUNT(Body);
        switch (Region)
        {
            case EONEHitRegion::Head: Bones=Head; Count=UE_ARRAY_COUNT(Head); break;
            case EONEHitRegion::ArmLeft: Bones=LeftArm; Count=UE_ARRAY_COUNT(LeftArm); break;
            case EONEHitRegion::ArmRight: Bones=RightArm; Count=UE_ARRAY_COUNT(RightArm); break;
            case EONEHitRegion::LegLeft: Bones=LeftLeg; Count=UE_ARRAY_COUNT(LeftLeg); break;
            case EONEHitRegion::LegRight: Bones=RightLeg; Count=UE_ARRAY_COUNT(RightLeg); break;
            default: break;
        }
        FName Best=Bones[0]; double Distance=TNumericLimits<double>::Max();
        for (int32 I=0;I<Count;++I)
        {
            const double D=FVector::DistSquared(Hit.ImpactPoint,Zombie->GetMesh()->GetSocketLocation(Bones[I]));
            if (D<Distance) { Distance=D; Best=Bones[I]; }
        }
        return Best;
    }
}

UONEWeaponComponent::UONEWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    PrimaryComponentTick.TickGroup=TG_PostPhysics;
    WeaponDefinitions=ONEWeaponCatalog::BuildDefaults();
}

void UONEWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    // Wait for this frame's aim and completed skeletal evaluation, including its
    // parallel evaluation task, before sampling attached muzzle/port transforms.
    if (auto* P=Cast<AONEPlayer>(GetOwner()))
    { AddTickPrerequisiteActor(P); AddTickPrerequisiteComponent(P->GetMesh()); }
    // Catalog rows are independent of the exactly two owned inventory slots.
    Carried.SetNum(2);
    auto Keep=[this](const auto& Ref) { if (UObject* Object=Ref.LoadSynchronous()) LoadedAssets.AddUnique(Object); };
    for (int32 I=0;I<WeaponDefinitions.Num();++I)
    {
        auto& D=WeaponDefinitions[I]; D.Capacity=FMath::Max(1,D.Capacity); D.Pellets=FMath::Clamp(D.Pellets,1,16);
        Keep(D.Mesh); Keep(D.SlideMesh); Keep(D.ForeEndMesh); Keep(D.ShellMesh); Keep(D.EjectedCaseMesh); Keep(D.MagazineMesh); Keep(D.ReadyAnimation); Keep(D.EmptySound);
        for (const auto& S:D.ShotSounds) Keep(S);
        for (const auto& S:D.FleshSounds) Keep(S);
        for (const auto& S:D.ConcreteSounds) Keep(S);
        for (const auto& S:D.MetalSounds) Keep(S);
        for (auto& O:D.Operations)
        {
            Keep(O.Animation); O.Duration=FMath::Max(.01f,O.Duration);
            O.Events.StableSort([](const auto& A,const auto& B) { return A.Time<B.Time; });
            for (const auto& E:O.Events) Keep(E.Sound);
        }
    }
    ResetStarterLoadout();
}
int32 UONEWeaponComponent::GetAmmoForWeapon(int32 I) const { return Carried.IsValidIndex(I) ? Carried[I].Ammo : 0; }
int32 UONEWeaponComponent::GetReserveAmmoForWeapon(int32 I) const { return Carried.IsValidIndex(I) ? Carried[I].Reserve : 0; }
int32 UONEWeaponComponent::GetLastShotSoundIndexForWeapon(int32 I) const { return Carried.IsValidIndex(I) ? Carried[I].LastShotSoundIndex : INDEX_NONE; }
int32 UONEWeaponComponent::GetEjectionCountForWeapon(int32 I) const { return Carried.IsValidIndex(I) ? Carried[I].EjectionCount : 0; }
int32 UONEWeaponComponent::GetLiveCaseCount() const
{ int32 Count=0; for (const auto& C:Cases) if (C.IsValid()) ++Count; return Count; }
AONEWeaponCase* UONEWeaponComponent::GetLastEjectedCase() const
{ for (int32 I=Cases.Num()-1;I>=0;--I) if (Cases[I].IsValid()) return Cases[I].Get(); return nullptr; }
bool UONEWeaponComponent::NeedsPump(int32 I) const { return Carried.IsValidIndex(I) && Carried[I].bNeedsPump; }
FText UONEWeaponComponent::GetWeaponName() const { return GetDefinition().DisplayName; }
float UONEWeaponComponent::GetTimeSinceShot() const { return GetWorld()->GetTimeSeconds()-LastShot; }
float UONEWeaponComponent::GetTimeSinceEmpty() const { return GetWorld()->GetTimeSeconds()-LastEmpty; }
float UONEWeaponComponent::GetTimeSinceHit() const { return GetWorld()->GetTimeSeconds()-LastHit; }
const FONEWeaponOperationDefinition* UONEWeaponComponent::FindOperation(int32 I,EONEWeaponOperation Op) const
{
    if (const auto* D=GetDefinitionForWeapon(I)) return D->Operations.FindByPredicate([Op](const auto& O) { return O.Operation==Op; });
    return nullptr;
}
float UONEWeaponComponent::GetOperationElapsed() const { return Operation==EONEWeaponOperation::Ready ? 0.f : GetWorld()->GetTimeSeconds()-OperationStart; }
float UONEWeaponComponent::GetOperationDuration() const { const auto* O=FindOperation(OperationIndex,Operation); return O ? O->Duration : .01f; }
float UONEWeaponComponent::GetOperationProgress() const { return FMath::Clamp(GetOperationElapsed()/GetOperationDuration(),0.f,1.f); }
bool UONEWeaponComponent::IsReloading() const
{ return Operation==EONEWeaponOperation::MagazineReload || Operation==EONEWeaponOperation::ShellStart || Operation==EONEWeaponOperation::ShellInsert || Operation==EONEWeaponOperation::ShellEnd; }
float UONEWeaponComponent::GetReloadElapsed() const { return IsReloading() ? GetOperationElapsed() : 0.f; }
float UONEWeaponComponent::GetReloadProgress() const { return IsReloading() ? GetOperationProgress() : 0.f; }
bool UONEWeaponComponent::CanFire() const
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    return HasUsableWeapon() && !bHandoffLocked && Carried[EquippedIndex].bMagazinePresent && P && !P->IsDead() && !UGameplayStatics::IsGamePaused(this) && GetAmmo()>0 && !NeedsPump(EquippedIndex) &&
        (Operation==EONEWeaponOperation::Ready || (GetDefinition().bAutomatic && Operation==EONEWeaponOperation::Fire)) && GetTimeSinceShot()>=GetDefinition().FireInterval;
}
void UONEWeaponComponent::SetTrigger(bool Held)
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (Held && (!HasUsableWeapon() || bHandoffLocked || !P || P->IsDead() || UGameplayStatics::IsGamePaused(this))) return;
    if (Held && !bTrigger)
    {
        bPendingShot=true;
        // A per-shell reload may be interrupted; an already inserted shell remains earned.
        if (GetDefinition().bShellReload && IsReloading() && Operation!=EONEWeaponOperation::ShellEnd)
        { AdvanceOperationEvents(); StartOperation(EONEWeaponOperation::ShellEnd); }
    }
    bTrigger=Held;
    // A semi-auto press is one buffered command: releasing the button during the
    // closing pose must not erase it. Explicit input cancellation clears it.
    if (!Held && GetDefinition().bAutomatic) bPendingShot=false;
}
void UONEWeaponComponent::BeginReload()
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    // Held sprint has priority over both R and automatic reload, even while still.
    // Ignored R presses are not queued to fight the player's escape on later ticks.
    if (!HasUsableWeapon() || bHandoffLocked || !P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || P->IsSprintRequested() ||
        (Operation!=EONEWeaponOperation::Ready && Operation!=EONEWeaponOperation::Fire) ||
        NeedsPump(EquippedIndex) || (GetAmmo()>=GetDefinition().Capacity && Carried[EquippedIndex].bMagazinePresent) || (GetReserveAmmo()<=0 && Carried[EquippedIndex].bMagazinePresent)) return;
    bPendingShot=false; bReloadStartedEmpty=GetAmmo()==0;
    StartOperation(GetDefinition().bShellReload ? EONEWeaponOperation::ShellStart : EONEWeaponOperation::MagazineReload);
}
void UONEWeaponComponent::CancelReload()
{
    if (!IsReloading()) return;
    // Input can arrive before this component's tick. Honor transfers whose event
    // time has already elapsed, then invalidate all later events atomically.
    if (!UGameplayStatics::IsGamePaused(this)) AdvanceOperationEvents();
    StopOperationAudio(); Operation=EONEWeaponOperation::Ready; ++OperationSerial; NextEvent=0;
    bPendingShot=false;
    if (auto* P=Cast<AONEPlayer>(GetOwner())) P->ClearReloadPresentation();
}
void UONEWeaponComponent::InterruptReloadForSprint()
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || !IsReloading()) return;
    CancelReload(); ++SprintReloadInterrupts;
}
bool UONEWeaponComponent::CanAutoReload() const
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    return HasUsableWeapon() && !bHandoffLocked && P && !P->IsDead() && !UGameplayStatics::IsGamePaused(this) && !P->IsSprintRequested() &&
        Operation==EONEWeaponOperation::Ready && !NeedsPump(EquippedIndex) && GetAmmo()==0 && GetReserveAmmo()>0;
}
void UONEWeaponComponent::StopOperationAudio()
{
    for (auto& A:OperationAudio) if (A.IsValid()) A->Stop();
    OperationAudio.Reset();
}
void UONEWeaponComponent::CancelAllOperations()
{
    StopOperationAudio();
    for (auto& A:ShotAudio) if (A.IsValid()) A->Stop();
    ShotAudio.Reset();
    bTrigger=false; bPendingShot=false; PendingIndex=-1; Operation=EONEWeaponOperation::Ready; NextEvent=0; ++OperationSerial;
    if (auto* P=Cast<AONEPlayer>(GetOwner())) P->ClearWeaponEffects();
}
bool UONEWeaponComponent::SelectWeapon(int32 I)
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (bHandoffLocked || !P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || !IsSlotAvailable(I) || (I==EquippedIndex && PendingIndex<0)) return false;
    if (IsReloading()) CancelReload();
    CancelAllOperations();
    if (I==EquippedIndex) { if (NeedsPump(I)) StartOperation(EONEWeaponOperation::Pump); return true; }
    PendingIndex=I; ++InventoryRevision; StartOperation(EONEWeaponOperation::Equip,I); return true;
}
void UONEWeaponComponent::RefillAllAmmo()
{
    if (bHandoffLocked) return;
    CancelAllOperations();
    for (int32 I=0;I<Carried.Num();++I) RefillSlot(I,false);
    LastShot=-100; LastEmpty=-100; RefreshEquippedPresentation();
}
void UONEWeaponComponent::AddReserveAmmo(int32 Count)
{ if (HasUsableWeapon() && !bHandoffLocked) Carried[EquippedIndex].Reserve=static_cast<int32>(FMath::Clamp(static_cast<int64>(GetReserveAmmo())+Count,static_cast<int64>(0),static_cast<int64>(GetDefinition().ReserveLimit))); }
void UONEWeaponComponent::GrantRoundAmmo()
{
    for (int32 I=0;I<Carried.Num();++I) if (IsSlotAvailable(I))
    { const auto& D=*GetDefinitionForWeapon(I); Carried[I].Reserve=FMath::Clamp(Carried[I].Reserve+FMath::Max(0,D.RoundReserveReward),0,D.ReserveLimit); }
}
void UONEWeaponComponent::RefreshEquippedPresentation()
{
    const auto& D=GetDefinition(); MagazineSize=D.Capacity; FireInterval=D.FireInterval; Damage=D.Damage; Range=D.Range;
    const auto* R=FindOperation(EquippedIndex,D.bShellReload ? EONEWeaponOperation::ShellInsert : EONEWeaponOperation::MagazineReload); ReloadDuration=R ? R->Duration : 0.f;
    if (auto* P=Cast<AONEPlayer>(GetOwner())) { if (HasUsableWeapon()) P->ApplyWeaponPresentation(D); else P->ClearEquippedPresentation(); }
}
void UONEWeaponComponent::PlayMechanical(USoundBase* S)
{
    if (!S) return;
    OperationAudio.RemoveAll([](const auto& C){return !C.IsValid() || !C->IsPlaying();});
    if (auto* P=Cast<AONEPlayer>(GetOwner())) if (auto* Audio=UGameplayStatics::SpawnSoundAttached(S,P->Gun,NAME_None,FVector::ZeroVector,EAttachLocation::KeepRelativeOffset,true,.55f))
    { Audio->bIsUISound=false; OperationAudio.Add(Audio); }
}
void UONEWeaponComponent::StartOperation(EONEWeaponOperation Next,int32 I)
{
    StopOperationAudio(); Operation=Next; OperationIndex=I<0 ? EquippedIndex : I;
    OperationStart=GetWorld()->GetTimeSeconds(); NextEvent=0; ++OperationSerial;
    if (const auto* O=FindOperation(OperationIndex,Operation))
        while (NextEvent<O->Events.Num() && O->Events[NextEvent].Time<=0.f) ProcessWeaponEvent(O->Events[NextEvent++]);
}
void UONEWeaponComponent::ProcessWeaponEvent(const FONEWeaponTimedEvent& E)
{
    if (!IsSlotAvailable(OperationIndex) || (bHandoffLocked && Operation!=EONEWeaponOperation::Equip)) return;
    auto& State=Carried[OperationIndex]; const auto& D=*GetDefinitionForWeapon(OperationIndex);
    switch (E.Event)
    {
        case EONEWeaponEvent::MagazineOut: DropMagazine(OperationIndex); break;
        case EONEWeaponEvent::MagazineCommit:
        {
            const int32 N=FMath::Min(D.Capacity-State.Ammo,State.Reserve);
            State.Ammo+=N; State.Reserve-=N; State.bMagazinePresent=true; if (N>0) ++MagazinesCommitted;
            if (auto* P=Cast<AONEPlayer>(GetOwner())) P->ClearReloadPresentation();
            break;
        }
        case EONEWeaponEvent::ShellCommit:
            if (State.Reserve>0 && State.Ammo<D.Capacity) { ++State.Ammo; --State.Reserve; ++ShellsInserted; } break;
        case EONEWeaponEvent::WeaponSwap:
            if (PendingIndex>=0) { EquippedIndex=PendingIndex; PendingIndex=-1; ++InventoryRevision; RefreshEquippedPresentation(); } break;
        case EONEWeaponEvent::PumpLock: State.bNeedsPump=false; break;
        case EONEWeaponEvent::ShellEject:
            EjectCase(OperationIndex);
            break;
        default: break;
    }
    PlayMechanical(E.Sound.Get());
}
void UONEWeaponComponent::EjectCase(int32 I)
{
    if (!IsSlotAvailable(I)) return;
    auto& State=Carried[I]; const auto& D=*GetDefinitionForWeapon(I);
    if (State.bCaseEjected || State.PendingCaseShotId==0) return;
    State.bCaseEjected=true; ++State.EjectionCount; ++CasesEjected;
    if (auto* P=Cast<AONEPlayer>(GetOwner()))
    {
        Cases.RemoveAll([](const auto& C){ return !C.IsValid(); });
        while (Cases.Num()>=FMath::Clamp(MaximumCases,1,64))
        { if (Cases[0].IsValid()) Cases[0]->Destroy(); Cases.RemoveAt(0); }
        P->Gun->UpdateComponentToWorld();
        const FTransform T=P->Gun->GetComponentTransform();
        FActorSpawnParameters Params; Params.Owner=P;
        if (auto* C=GetWorld()->SpawnActor<AONEWeaponCase>(T.TransformPosition(D.EjectionPoint),T.Rotator(),Params))
        {
            const FVector Impulse=D.CaseImpulse+FVector(FMath::FRandRange(-18.f,18.f),FMath::FRandRange(-22.f,22.f),FMath::FRandRange(-15.f,20.f));
            const FVector Spin(FMath::FRandRange(420.f,900.f),FMath::FRandRange(-700.f,700.f),FMath::FRandRange(-550.f,550.f));
            C->Initialize(D.EjectedCaseMesh.Get(),T.TransformVectorNoScale(Impulse),P->GetVelocity(),Spin,D.CaseRadius,
                FMath::Clamp(CaseLifetime,1.f,15.f),I,State.PendingCaseShotId);
            Cases.Add(C);
        }
    }
}
void UONEWeaponComponent::DropMagazine(int32 I)
{
    if (!IsSlotAvailable(I)) return;
    auto& State=Carried[I]; const auto& D=*GetDefinitionForWeapon(I);
    if (!State.bMagazinePresent || D.bShellReload) return;
    State.bMagazinePresent=false; ++State.MagazineDropCount; ++MagazinesDropped;
    const uint64 ReleaseId=++NextMagazineReleaseId;
    if (auto* P=Cast<AONEPlayer>(GetOwner()))
    {
        const FTransform Release=P->GetMagazineReleaseTransform();
        Magazines.RemoveAll([](const auto& M){ return !M.IsValid(); });
        while (Magazines.Num()>=FMath::Clamp(MaximumMagazines,1,32))
        { if (Magazines[0].IsValid()) Magazines[0]->Destroy(); Magazines.RemoveAt(0); }
        FActorSpawnParameters Params; Params.Owner=P;
        if (auto* M=GetWorld()->SpawnActor<AONEWeaponMagazine>(Release.GetLocation(),Release.Rotator(),Params))
        { M->Initialize(D.MagazineMesh.Get(),Release,P->GetVelocity(),MagazineLifetime,State.InstanceId,ReleaseId); Magazines.Add(M); }
        P->ClearReloadPresentation();
    }
}
int32 UONEWeaponComponent::GetLiveMagazineCount() const
{ int32 N=0; for (const auto& M:Magazines) if (M.IsValid()) ++N; return N; }
AONEWeaponMagazine* UONEWeaponComponent::GetLastDroppedMagazine() const
{ for (int32 I=Magazines.Num()-1;I>=0;--I) if (Magazines[I].IsValid()) return Magazines[I].Get(); return nullptr; }
USoundBase* UONEWeaponComponent::ChooseShotSound(int32 I)
{
    const auto& Bank=GetDefinitionForWeapon(I)->ShotSounds; auto& State=Carried[I];
    TArray<int32,TInlineAllocator<8>> Valid;
    for (int32 N=0;N<Bank.Num();++N) if (Bank[N].IsValid() && N!=State.LastShotSoundIndex) Valid.Add(N);
    if (Valid.IsEmpty() && Bank.IsValidIndex(State.LastShotSoundIndex) && Bank[State.LastShotSoundIndex].IsValid()) Valid.Add(State.LastShotSoundIndex);
    LastShotSoundIndex=Valid.IsEmpty() ? INDEX_NONE : Valid[FMath::RandHelper(Valid.Num())];
    State.LastShotSoundIndex=LastShotSoundIndex;
    return Bank.IsValidIndex(LastShotSoundIndex) ? Bank[LastShotSoundIndex].Get() : nullptr;
}
void UONEWeaponComponent::FinishOperation()
{
    const EONEWeaponOperation Finished=Operation;
    StopOperationAudio(); Operation=EONEWeaponOperation::Ready; ++OperationSerial;
    if (Finished==EONEWeaponOperation::Equip || Finished==EONEWeaponOperation::Fire)
    { if (!bHandoffLocked && NeedsPump(EquippedIndex)) StartOperation(EONEWeaponOperation::Pump); }
    else if (Finished==EONEWeaponOperation::ShellStart || Finished==EONEWeaponOperation::ShellInsert)
        StartOperation(GetAmmo()<GetDefinition().Capacity && GetReserveAmmo()>0 ? EONEWeaponOperation::ShellInsert : EONEWeaponOperation::ShellEnd);
}
void UONEWeaponComponent::AdvanceOperationEvents()
{
    const int32 Serial=OperationSerial;
    if (const auto* O=FindOperation(OperationIndex,Operation))
        while (OperationSerial==Serial && NextEvent<O->Events.Num() && O->Events[NextEvent].Time<=GetOperationElapsed()) ProcessWeaponEvent(O->Events[NextEvent++]);
}
void UONEWeaponComponent::TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Dt,Tick,ThisTick);
    const auto* P=Cast<AONEPlayer>(GetOwner()); if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || (bHandoffLocked && Operation!=EONEWeaponOperation::Equip)) return;
    if (Operation!=EONEWeaponOperation::Ready)
    {
        const int32 Serial=OperationSerial;
        AdvanceOperationEvents();
        if (Serial==OperationSerial && GetOperationElapsed()>=GetOperationDuration()) FinishOperation();
    }
    if (!HasUsableWeapon() || bHandoffLocked) return;
    if (Operation==EONEWeaponOperation::Ready && NeedsPump(EquippedIndex)) StartOperation(EONEWeaponOperation::Pump);
    // Pump/equip/fire completion comes first. Only the empty equipped weapon may
    // reload automatically; a held trigger is never required to enter this path.
    if (CanAutoReload()) { BeginReload(); if (IsReloading()) ++AutomaticReloads; }
    const bool WantsShot=GetDefinition().bAutomatic ? bTrigger : bPendingShot;
    if (WantsShot && CanFire()) { Fire(); bPendingShot=false; }
    else if (WantsShot && Operation==EONEWeaponOperation::Ready && GetAmmo()<=0 && GetTimeSinceEmpty()>.35f)
    { LastEmpty=GetWorld()->GetTimeSeconds(); PlayMechanical(GetDefinition().EmptySound.Get()); bPendingShot=false; }
}
UAnimSequence* UONEWeaponComponent::GetReadyAnimation() const { return GetDefinition().ReadyAnimation.Get(); }
UAnimSequence* UONEWeaponComponent::GetActionAnimation(float& Time) const
{
    Time=GetOperationElapsed(); const auto* O=FindOperation(OperationIndex,Operation);
    UAnimSequence* Animation=Operation==EONEWeaponOperation::Ready || !O ? nullptr : O->Animation.Get();
    if (Animation && O->Duration>0.f) Time=FMath::Clamp(Time/O->Duration,0.f,1.f)*Animation->GetPlayLength();
    return Animation;
}
float UONEWeaponComponent::GetPumpFraction() const
{
    if (Operation!=EONEWeaponOperation::Pump) return 0.f;
    const float T=GetOperationElapsed(); const auto& D=GetDefinition();
    const float Fraction=T<D.PumpRearTime ? FMath::Clamp(T/FMath::Max(.001f,D.PumpRearTime),0.f,1.f) : FMath::Clamp(1.f-(T-D.PumpRearTime)/FMath::Max(.001f,D.PumpForwardTime-D.PumpRearTime),0.f,1.f);
    // Match the authored support-hand curve in each rearward/forward segment.
    return Fraction*Fraction*(3.f-2.f*Fraction);
}
float UONEWeaponComponent::GetSlideFraction() const
{
    if (!HasUsableWeapon() || GetDefinition().Family!=EONEWeaponFamily::Pistol) return 0.f;
    auto Smooth=[](float Value) { const float X=FMath::Clamp(Value,0.f,1.f); return X*X*(3.f-2.f*X); };
    if (Operation==EONEWeaponOperation::MagazineReload)
        return bReloadStartedEmpty ? 1.f-Smooth((GetOperationElapsed()-1.4f)/.06f) : 0.f;
    if (Operation==EONEWeaponOperation::Fire)
    {
        const float T=GetOperationProgress()*.18f;
        return T<.025f ? Smooth(T/.025f) : 1.f-Smooth((T-.025f)/.045f);
    }
    return GetAmmo()==0 ? 1.f : 0.f;
}
bool UONEWeaponComponent::ShouldShowLoadingShell() const
{ const float T=GetOperationElapsed(); const float Insert=FindEventTime(EONEWeaponEvent::ShellCommit,.6f); return Operation==EONEWeaponOperation::ShellInsert && T>=Insert*.2f && T<Insert; }
bool UONEWeaponComponent::ShouldShowSeatedMagazine() const
{ return HasUsableWeapon() && Carried[EquippedIndex].bMagazinePresent; }
bool UONEWeaponComponent::ShouldShowHeldMagazine() const
{ return Operation==EONEWeaponOperation::MagazineReload && GetOperationElapsed()>=GetDefinition().MagazineFreshTime && !ShouldShowSeatedMagazine(); }
float UONEWeaponComponent::FindEventTime(EONEWeaponEvent Type,float Fallback) const
{
    if (const auto* O=FindOperation(OperationIndex,Operation))
        if (const auto* E=O->Events.FindByPredicate([Type](const auto& E){ return E.Event==Type; })) return E->Time;
    return Fallback;
}
void UONEWeaponComponent::ClearEjectedCases()
{ for (auto& C:Cases) if (C.IsValid()) C->Destroy(); Cases.Reset(); for (auto& M:Magazines) if (M.IsValid()) M->Destroy(); Magazines.Reset(); }

void UONEWeaponComponent::Fire()
{
    auto* P=Cast<AONEPlayer>(GetOwner()); if (!P || !CanFire()) return;
    const auto& D=GetDefinition(); auto& State=Carried[EquippedIndex];
    --State.Ammo; State.bNeedsPump=D.bPumpAction; State.bCaseEjected=false;
    LastShot=GetWorld()->GetTimeSeconds(); LastShotId=++NextDischargeId; ++ShotsFired;
    State.PendingCaseShotId=LastShotId;
    P->Gun->UpdateComponentToWorld();
    LastShotMuzzle=P->GetMuzzleLocation(); LastShotFrame=GFrameCounter;
    LastShotPoseFrame=P->GetMesh()->GetCurrentBoneTransformFrame();
    LastShotPoseRevision=P->GetMesh()->GetBoneTransformRevisionNumber();
    StartOperation(EONEWeaponOperation::Fire); P->FlashMuzzle();
    const FVector Start=LastShotMuzzle;
    if (auto* S=ChooseShotSound(EquippedIndex))
        if (auto* A=UGameplayStatics::SpawnSoundAttached(S,P->Gun,NAME_None,FVector::ZeroVector,EAttachLocation::KeepRelativeOffset,true,D.bPumpAction ? .82f : .62f,1.f))
        { A->bIsUISound=false; ShotAudio.RemoveAll([](const auto& C){return !C.IsValid() || !C->IsPlaying();}); ShotAudio.Add(A); }
    FVector Direction=(P->GetAimPoint()-Start).GetSafeNormal(); if (Direction.IsNearlyZero()) Direction=P->GetActorForwardVector();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ONEWeapon),false,P);
    FHitResult Obstruction; const FVector Shoulder=P->GetActorLocation()+FVector(0,0,42);
    const bool bObstructed=GetWorld()->LineTraceSingleByChannel(Obstruction,Shoulder,Start,ECC_Visibility,Params);
    TMap<AONEZombie*,FONEWeaponDamagePacket> Victims;
    TArray<FHitResult> SurfaceHits;
    auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>();
    // Trace every pellet against one scene snapshot. Resolve each victim only once afterward.
    for (int32 I=0;I<D.Pellets;++I)
    {
        const FVector Ray=FMath::VRandCone(Direction,FMath::DegreesToRadians(D.SpreadDegrees));
        FHitResult Hit=Obstruction;
        FVector TraceStart=Start,TraceEnd=Start+Ray*D.Range,LastEnd=TraceEnd;
        FCollisionQueryParams PelletParams=Params;
        // The single extra victim is bounded independently of the pellet count.
        // Ignore the entire previous actor, not just one of its region shapes.
        const int32 Extra=FMath::Clamp(D.AdditionalVictims,0,1);
        for (int32 Depth=0;Depth<=Extra;++Depth)
        {
            const bool bHit=(Depth==0 && bObstructed) || GetWorld()->LineTraceSingleByChannel(Hit,TraceStart,TraceEnd,ECC_Visibility,PelletParams);
            LastEnd=bHit ? Hit.ImpactPoint : TraceEnd;
            if (!bHit) break;
            if (auto* Z=Cast<AONEZombie>(Hit.GetActor()))
            {
                const EONEHitRegion Region=Z->GetHitRegion(Hit); if (Region==EONEHitRegion::Invalid) break;
                const float Falloff=FMath::Clamp((FVector::Distance(Start,LastEnd)-D.FalloffStart)/FMath::Max(1.f,D.Range-D.FalloffStart),0.f,1.f);
                const float HitDamage=D.Damage*FMath::Lerp(1.f,D.MinimumDamageFraction,Falloff)*(Depth==0 ? 1.f : FMath::Clamp(D.PenetrationDamageFraction,0.f,1.f));
                auto& Packet=Victims.FindOrAdd(Z); Packet.ShotId=LastShotId; Packet.HeavyStaggerThreshold=D.HeavyStaggerThreshold;
                float TraumaScale=0.f;
                if (Region==EONEHitRegion::Head) TraumaScale=D.HeadTraumaScale;
                else if (Region==EONEHitRegion::ArmLeft || Region==EONEHitRegion::ArmRight) TraumaScale=D.ArmTraumaScale;
                else if (Region==EONEHitRegion::LegLeft || Region==EONEHitRegion::LegRight) TraumaScale=D.LegTraumaScale;
                Packet.Get(Region).AddPellet(HitDamage,HitDamage*TraumaScale,LastEnd,Ray,Hit.ImpactNormal,RegionalImpactBone(Z,Region,Hit));
                if (Depth>=Extra || bObstructed) break;
                PelletParams.AddIgnoredActor(Z);
                TraceStart=LastEnd+Ray*.05f;
                if (FVector::DotProduct(TraceEnd-TraceStart,Ray)<=0.f) break;
            }
            else
            {
                if (SurfaceHits.Num()<2 && !SurfaceHits.ContainsByPredicate([&Hit](const auto& Previous){ return Previous.GetComponent()==Hit.GetComponent(); })) SurfaceHits.Add(Hit);
                break; // World cover always stops penetration; range endpoint never extends.
            }
        }
        if (Blood && (D.Pellets==1 || I<3)) Blood->Shot(Start,LastEnd,D.TraceColor);
    }
    bLastHitKill=false; LastShotVictimCount=0;
    int32 FleshVoices=0;
    for (auto& Pair:Victims)
    {
        auto& Packet=Pair.Value; Packet.Finalize();
        if (Pair.Key->ReceiveWeaponDamage(Packet))
        {
            ++LastShotVictimCount; LastHit=GetWorld()->GetTimeSeconds(); bLastHitKill|=Pair.Key->IsDead();
            if (FleshVoices<2) if (auto* S=Choose(D.FleshSounds))
            { UGameplayStatics::PlaySoundAtLocation(this,S,Packet.GetImpactPosition(),.55f); ++FleshVoices; }
        }
    }
    for (const auto& Hit:SurfaceHits)
    {
        const bool Metal=(Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Metal"))) || (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("Metal")));
        if (auto* S=Choose(Metal ? D.MetalSounds : D.ConcreteSounds)) UGameplayStatics::PlaySoundAtLocation(this,S,Hit.ImpactPoint,.4f);
    }
}
