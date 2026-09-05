#include "ONEWeaponComponent.h"
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
    template<class T> TSoftObjectPtr<T> Asset(const FString& Folder,const FString& Name)
    { return TSoftObjectPtr<T>(FSoftObjectPath(Folder+Name+TEXT(".")+Name)); }
    TSoftObjectPtr<USoundBase> Sound(const TCHAR* Name) { return Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/"),Name); }
    TSoftObjectPtr<UAnimSequence> Clip(const TCHAR* Name) { return Asset<UAnimSequence>(TEXT("/Game/ONE/Animations/"),Name); }
    void AddOperation(FONEWeaponDefinition& D,EONEWeaponOperation Op,float Duration,const TCHAR* Animation,std::initializer_list<FONEWeaponTimedEvent> Events={})
    {
        FONEWeaponOperationDefinition O; O.Operation=Op; O.Duration=Duration; O.Animation=Clip(Animation);
        for (const auto& E:Events) O.Events.Add(E);
        D.Operations.Add(O);
    }
    FONEWeaponTimedEvent Event(float Time,EONEWeaponEvent Type,const TCHAR* Audio)
    { FONEWeaponTimedEvent E; E.Time=Time; E.Event=Type; if (Audio) E.Sound=Sound(Audio); return E; }
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
    FONEWeaponDefinition Carbine;
    Carbine.Id=TEXT("AR01"); Carbine.DisplayName=FText::FromString(TEXT("5.56 mm Carbine"));
    Carbine.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_Carbine_C02Body"));
    Carbine.MagazineMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_Carbine_Magazine"));
    Carbine.ReadyAnimation=Clip(TEXT("A_Response_Idle"));
    Carbine.EmptySound=Sound(TEXT("S_CarbineEmpty"));
    Carbine.EjectedCaseMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_RifleBrass_C03"));
    Carbine.EjectionPoint=FVector(3.6f,-5.1f,13.7f);
    for (int32 I=1;I<=6;++I) Carbine.ShotSounds.Add(Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate03/"),FString::Printf(TEXT("S_C03_CarbineShot_%02d"),I)));
    Carbine.FleshSounds={Sound(TEXT("S_FleshImpact_01")),Sound(TEXT("S_FleshImpact_02")),Sound(TEXT("S_FleshImpact_03"))};
    Carbine.ConcreteSounds={Sound(TEXT("S_ConcreteImpact_01")),Sound(TEXT("S_ConcreteImpact_02"))};
    Carbine.MetalSounds={Sound(TEXT("S_MetalImpact_01")),Sound(TEXT("S_MetalImpact_02"))};
    AddOperation(Carbine,EONEWeaponOperation::Equip,.36f,TEXT("A_Response_Equip"),{Event(.18f,EONEWeaponEvent::WeaponSwap,TEXT("S_WeaponEquip"))});
    AddOperation(Carbine,EONEWeaponOperation::Fire,.2f,TEXT("A_Response_Fire"),{Event(0.f,EONEWeaponEvent::ShellEject,nullptr)});
    AddOperation(Carbine,EONEWeaponOperation::MagazineReload,2.1f,TEXT("A_Response_CarbineReload"),{
        Event(.4f,EONEWeaponEvent::MagazineOut,TEXT("S_CarbineMagOut")),Event(1.2f,EONEWeaponEvent::MagazineCommit,TEXT("S_CarbineMagIn")),Event(1.74f,EONEWeaponEvent::Sound,TEXT("S_CarbineBolt"))});
    WeaponDefinitions.Add(Carbine);
    FONEWeaponDefinition Shotgun=Carbine;
    Shotgun.Id=TEXT("SG01"); Shotgun.DisplayName=FText::FromString(TEXT("12-Gauge Pump Shotgun"));
    Shotgun.bAutomatic=false; Shotgun.bShellReload=true; Shotgun.bPumpAction=true;
    Shotgun.Capacity=6; Shotgun.InitialReserve=36; Shotgun.ReserveLimit=60;
    Shotgun.RoundReserveReward=8;
    Shotgun.Pellets=8; Shotgun.Damage=15.f; Shotgun.FireInterval=.78f; Shotgun.SpreadDegrees=4.f;
    Shotgun.Range=1400.f; Shotgun.FalloffStart=500.f; Shotgun.MinimumDamageFraction=.2f;
    Shotgun.HeadTraumaScale=1.f; Shotgun.HeavyStaggerThreshold=70.f;
    Shotgun.FlashDuration=.065f; Shotgun.FlashIntensity=27000.f;
    Shotgun.FlashLength=29.f; Shotgun.FlashRadius=6.2f; Shotgun.FlashLightRadius=245.f;
    Shotgun.FlashLightColor=FLinearColor(1.f,.56f,.20f);
    Shotgun.Muzzle=FVector(64.5f,0,14);
    Shotgun.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_PumpShotgun"));
    Shotgun.ForeEndMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_PumpShotgun_ForeEnd"));
    Shotgun.ShellMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_ShotgunShell"));
    Shotgun.EjectedCaseMesh=Shotgun.ShellMesh;
    Shotgun.EjectionPoint=FVector(5,-4.5,13.5);
    Shotgun.CaseRadius=1.05f; Shotgun.CaseImpulse=FVector(35,-165,125);
    Shotgun.MagazineMesh.Reset();
    Shotgun.ReadyAnimation=Clip(TEXT("A_Response_ShotgunReady"));
    Shotgun.EmptySound=Sound(TEXT("S_ShotgunEmpty"));
    Shotgun.ShotSounds.Reset();
    for (int32 I=1;I<=6;++I) Shotgun.ShotSounds.Add(Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate03/"),FString::Printf(TEXT("S_C03_ShotgunShot_%02d"),I)));
    Shotgun.Operations.Reset();
    AddOperation(Shotgun,EONEWeaponOperation::Equip,.36f,TEXT("A_Response_Equip"),{Event(.18f,EONEWeaponEvent::WeaponSwap,TEXT("S_WeaponEquip"))});
    AddOperation(Shotgun,EONEWeaponOperation::Fire,.22f,TEXT("A_Response_ShotgunFire"));
    AddOperation(Shotgun,EONEWeaponOperation::Pump,.56f,TEXT("A_Response_ShotgunPump"),{
        Event(0.f,EONEWeaponEvent::Sound,TEXT("S_ShotgunPumpBack")),Event(.18f,EONEWeaponEvent::ShellEject,nullptr),
        Event(.21f,EONEWeaponEvent::Sound,TEXT("S_ShotgunPumpForward")),Event(.44f,EONEWeaponEvent::PumpLock,TEXT("S_ShotgunPumpLock"))});
    AddOperation(Shotgun,EONEWeaponOperation::ShellStart,.35f,TEXT("A_Response_ShotgunReloadStart"),{Event(0.f,EONEWeaponEvent::Sound,TEXT("S_ShotgunReloadStart"))});
    AddOperation(Shotgun,EONEWeaponOperation::ShellInsert,.9f,TEXT("A_Response_ShotgunReloadShell"),{Event(.6f,EONEWeaponEvent::ShellCommit,TEXT("S_ShotgunShellInsert"))});
    AddOperation(Shotgun,EONEWeaponOperation::ShellEnd,.32f,TEXT("A_Response_ShotgunReloadEnd"),{Event(0.f,EONEWeaponEvent::Sound,TEXT("S_ShotgunReloadEnd"))});
    WeaponDefinitions.Add(Shotgun);
}

void UONEWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    // Wait for this frame's aim and completed skeletal evaluation, including its
    // parallel evaluation task, before sampling attached muzzle/port transforms.
    if (auto* P=Cast<AONEPlayer>(GetOwner()))
    { AddTickPrerequisiteActor(P); AddTickPrerequisiteComponent(P->GetMesh()); }
    // This milestone carries exactly two editable rows; no unbounded inventory framework.
    WeaponDefinitions.SetNum(2); Carried.SetNum(2);
    auto Keep=[this](const auto& Ref) { if (UObject* Object=Ref.LoadSynchronous()) LoadedAssets.AddUnique(Object); };
    for (int32 I=0;I<WeaponDefinitions.Num();++I)
    {
        auto& D=WeaponDefinitions[I]; D.Capacity=FMath::Max(1,D.Capacity); D.Pellets=FMath::Clamp(D.Pellets,1,16);
        Carried[I].Ammo=D.Capacity; Carried[I].Reserve=FMath::Clamp(D.InitialReserve,0,D.ReserveLimit);
        Keep(D.Mesh); Keep(D.ForeEndMesh); Keep(D.ShellMesh); Keep(D.EjectedCaseMesh); Keep(D.MagazineMesh); Keep(D.ReadyAnimation); Keep(D.EmptySound);
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
    RefreshEquippedPresentation();
}
const FONEWeaponDefinition& UONEWeaponComponent::GetDefinition() const { return WeaponDefinitions[EquippedIndex]; }
const FONEWeaponDefinition* UONEWeaponComponent::GetDefinitionForWeapon(int32 I) const { return WeaponDefinitions.IsValidIndex(I) ? &WeaponDefinitions[I] : nullptr; }
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
    return P && !P->IsDead() && !UGameplayStatics::IsGamePaused(this) && GetAmmo()>0 && !NeedsPump(EquippedIndex) &&
        (Operation==EONEWeaponOperation::Ready || (GetDefinition().bAutomatic && Operation==EONEWeaponOperation::Fire)) && GetTimeSinceShot()>=GetDefinition().FireInterval;
}
void UONEWeaponComponent::SetTrigger(bool Held)
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (Held && (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this))) return;
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
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || P->IsSprintRequested() ||
        (Operation!=EONEWeaponOperation::Ready && Operation!=EONEWeaponOperation::Fire) ||
        NeedsPump(EquippedIndex) || GetAmmo()>=GetDefinition().Capacity || GetReserveAmmo()<=0) return;
    bPendingShot=false;
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
    return P && !P->IsDead() && !UGameplayStatics::IsGamePaused(this) && !P->IsSprintRequested() &&
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
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this) || !Carried.IsValidIndex(I) || (I==EquippedIndex && PendingIndex<0)) return false;
    if (IsReloading()) CancelReload();
    CancelAllOperations();
    if (I==EquippedIndex) { if (NeedsPump(I)) StartOperation(EONEWeaponOperation::Pump); return true; }
    PendingIndex=I; StartOperation(EONEWeaponOperation::Equip,I); return true;
}
void UONEWeaponComponent::RefillAllAmmo()
{
    CancelAllOperations();
    for (int32 I=0;I<Carried.Num();++I)
    {
        Carried[I].Ammo=WeaponDefinitions[I].Capacity; Carried[I].Reserve=FMath::Clamp(WeaponDefinitions[I].InitialReserve,0,WeaponDefinitions[I].ReserveLimit);
        Carried[I].bNeedsPump=false; Carried[I].bCaseEjected=false; Carried[I].PendingCaseShotId=0;
    }
    LastShot=-100; LastEmpty=-100; RefreshEquippedPresentation();
}
void UONEWeaponComponent::AddReserveAmmo(int32 Count)
{ if (Carried.IsValidIndex(EquippedIndex)) Carried[EquippedIndex].Reserve=FMath::Clamp(GetReserveAmmo()+Count,0,GetDefinition().ReserveLimit); }
void UONEWeaponComponent::GrantRoundAmmo()
{
    for (int32 I=0;I<Carried.Num();++I)
        Carried[I].Reserve=FMath::Clamp(Carried[I].Reserve+FMath::Max(0,WeaponDefinitions[I].RoundReserveReward),0,WeaponDefinitions[I].ReserveLimit);
}
void UONEWeaponComponent::RefreshEquippedPresentation()
{
    const auto& D=GetDefinition(); MagazineSize=D.Capacity; FireInterval=D.FireInterval; Damage=D.Damage; Range=D.Range;
    const auto* R=FindOperation(EquippedIndex,D.bShellReload ? EONEWeaponOperation::ShellInsert : EONEWeaponOperation::MagazineReload); ReloadDuration=R ? R->Duration : 0.f;
    if (auto* P=Cast<AONEPlayer>(GetOwner())) P->ApplyWeaponPresentation(D);
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
    if (!Carried.IsValidIndex(OperationIndex)) return;
    auto& State=Carried[OperationIndex]; const auto& D=WeaponDefinitions[OperationIndex];
    switch (E.Event)
    {
        case EONEWeaponEvent::MagazineCommit:
        {
            const int32 N=FMath::Min(D.Capacity-State.Ammo,State.Reserve);
            State.Ammo+=N; State.Reserve-=N; if (N>0) ++MagazinesCommitted; break;
        }
        case EONEWeaponEvent::ShellCommit:
            if (State.Reserve>0 && State.Ammo<D.Capacity) { ++State.Ammo; --State.Reserve; ++ShellsInserted; } break;
        case EONEWeaponEvent::WeaponSwap:
            if (PendingIndex>=0) { EquippedIndex=PendingIndex; PendingIndex=-1; RefreshEquippedPresentation(); } break;
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
    auto& State=Carried[I]; const auto& D=WeaponDefinitions[I];
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
USoundBase* UONEWeaponComponent::ChooseShotSound(int32 I)
{
    const auto& Bank=WeaponDefinitions[I].ShotSounds; auto& State=Carried[I];
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
    { if (NeedsPump(EquippedIndex)) StartOperation(EONEWeaponOperation::Pump); }
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
    const auto* P=Cast<AONEPlayer>(GetOwner()); if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this)) return;
    if (Operation!=EONEWeaponOperation::Ready)
    {
        const int32 Serial=OperationSerial;
        AdvanceOperationEvents();
        if (Serial==OperationSerial && GetOperationElapsed()>=GetOperationDuration()) FinishOperation();
    }
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
    return Operation==EONEWeaponOperation::Ready || !O ? nullptr : O->Animation.Get();
}
float UONEWeaponComponent::GetPumpFraction() const
{
    if (Operation!=EONEWeaponOperation::Pump) return 0.f;
    const float T=GetOperationElapsed(); const auto& D=GetDefinition();
    const float Fraction=T<D.PumpRearTime ? FMath::Clamp(T/FMath::Max(.001f,D.PumpRearTime),0.f,1.f) : FMath::Clamp(1.f-(T-D.PumpRearTime)/FMath::Max(.001f,D.PumpForwardTime-D.PumpRearTime),0.f,1.f);
    // Match the authored support-hand curve in each rearward/forward segment.
    return Fraction*Fraction*(3.f-2.f*Fraction);
}
bool UONEWeaponComponent::ShouldShowLoadingShell() const
{ const float T=GetOperationElapsed(); const float Insert=FindEventTime(EONEWeaponEvent::ShellCommit,.6f); return Operation==EONEWeaponOperation::ShellInsert && T>=Insert*.2f && T<Insert; }
bool UONEWeaponComponent::ShouldShowSeatedMagazine() const
{ const float T=GetOperationElapsed(); return Operation!=EONEWeaponOperation::MagazineReload || T<FindEventTime(EONEWeaponEvent::MagazineOut,.4f) || T>=FindEventTime(EONEWeaponEvent::MagazineCommit,1.2f); }
bool UONEWeaponComponent::ShouldShowHeldMagazine() const
{ return Operation==EONEWeaponOperation::MagazineReload && !ShouldShowSeatedMagazine(); }
float UONEWeaponComponent::FindEventTime(EONEWeaponEvent Type,float Fallback) const
{
    if (const auto* O=FindOperation(OperationIndex,Operation))
        if (const auto* E=O->Events.FindByPredicate([Type](const auto& E){ return E.Event==Type; })) return E->Time;
    return Fallback;
}
void UONEWeaponComponent::ClearEjectedCases()
{ for (auto& C:Cases) if (C.IsValid()) C->Destroy(); Cases.Reset(); }

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
        const bool bHit=bObstructed || GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Ray*D.Range,ECC_Visibility,Params);
        const FVector End=bHit ? Hit.ImpactPoint : Start+Ray*D.Range;
        if (Blood && (D.Pellets==1 || I<3)) Blood->Shot(Start,End);
        if (!bHit) continue;
        if (auto* Z=Cast<AONEZombie>(Hit.GetActor()))
        {
            const EONEHitRegion Region=Z->GetHitRegion(Hit); if (Region==EONEHitRegion::Invalid) continue;
            const float Falloff=FMath::Clamp((FVector::Distance(Start,End)-D.FalloffStart)/FMath::Max(1.f,D.Range-D.FalloffStart),0.f,1.f);
            const float HitDamage=D.Damage*FMath::Lerp(1.f,D.MinimumDamageFraction,Falloff);
            auto& Packet=Victims.FindOrAdd(Z); Packet.ShotId=LastShotId; Packet.HeavyStaggerThreshold=D.HeavyStaggerThreshold;
            float TraumaScale=0.f;
            if (Region==EONEHitRegion::Head) TraumaScale=D.HeadTraumaScale;
            else if (Region==EONEHitRegion::ArmLeft || Region==EONEHitRegion::ArmRight) TraumaScale=D.ArmTraumaScale;
            else if (Region==EONEHitRegion::LegLeft || Region==EONEHitRegion::LegRight) TraumaScale=D.LegTraumaScale;
            Packet.Get(Region).AddPellet(HitDamage,HitDamage*TraumaScale,End,Ray,Hit.ImpactNormal,RegionalImpactBone(Z,Region,Hit));
        }
        else if (SurfaceHits.Num()<2 && !SurfaceHits.ContainsByPredicate([&Hit](const auto& Previous){ return Previous.GetComponent()==Hit.GetComponent(); })) SurfaceHits.Add(Hit);
    }
    bLastHitKill=false;
    int32 FleshVoices=0;
    for (auto& Pair:Victims)
    {
        auto& Packet=Pair.Value; Packet.Finalize();
        if (Pair.Key->ReceiveWeaponDamage(Packet))
        {
            LastHit=GetWorld()->GetTimeSeconds(); bLastHitKill|=Pair.Key->IsDead();
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
