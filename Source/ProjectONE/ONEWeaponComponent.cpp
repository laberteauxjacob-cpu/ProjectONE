#include "ONEWeaponComponent.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEWeaponCase.h"
#include "ONEBloodSubsystem.h"
#include "Animation/AnimSequence.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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
}

UONEWeaponComponent::UONEWeaponComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
    FONEWeaponDefinition Carbine;
    Carbine.Id=TEXT("AR01"); Carbine.DisplayName=FText::FromString(TEXT("AR-01 CARBINE"));
    Carbine.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_Carbine_C02Body"));
    Carbine.MagazineMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_Carbine_Magazine"));
    Carbine.ReadyAnimation=Clip(TEXT("A_Response_Idle"));
    Carbine.EmptySound=Sound(TEXT("S_CarbineEmpty"));
    Carbine.ShotSounds={Sound(TEXT("S_CarbineShot_01")),Sound(TEXT("S_CarbineShot_02")),Sound(TEXT("S_CarbineShot_03"))};
    Carbine.FleshSounds={Sound(TEXT("S_FleshImpact_01")),Sound(TEXT("S_FleshImpact_02")),Sound(TEXT("S_FleshImpact_03"))};
    Carbine.ConcreteSounds={Sound(TEXT("S_ConcreteImpact_01")),Sound(TEXT("S_ConcreteImpact_02"))};
    Carbine.MetalSounds={Sound(TEXT("S_MetalImpact_01")),Sound(TEXT("S_MetalImpact_02"))};
    AddOperation(Carbine,EONEWeaponOperation::Equip,.36f,TEXT("A_Response_Equip"),{Event(.18f,EONEWeaponEvent::WeaponSwap,TEXT("S_WeaponEquip"))});
    AddOperation(Carbine,EONEWeaponOperation::Fire,.2f,TEXT("A_Response_Fire"));
    AddOperation(Carbine,EONEWeaponOperation::MagazineReload,2.1f,TEXT("A_Response_CarbineReload"),{
        Event(.4f,EONEWeaponEvent::MagazineOut,TEXT("S_CarbineMagOut")),Event(1.2f,EONEWeaponEvent::MagazineCommit,TEXT("S_CarbineMagIn")),Event(1.74f,EONEWeaponEvent::Sound,TEXT("S_CarbineBolt"))});
    WeaponDefinitions.Add(Carbine);
    FONEWeaponDefinition Shotgun=Carbine;
    Shotgun.Id=TEXT("SG01"); Shotgun.DisplayName=FText::FromString(TEXT("SG-01 PUMP SHOTGUN"));
    Shotgun.bAutomatic=false; Shotgun.bShellReload=true; Shotgun.bPumpAction=true;
    Shotgun.Capacity=6; Shotgun.InitialReserve=36; Shotgun.ReserveLimit=60;
    Shotgun.Pellets=8; Shotgun.Damage=15.f; Shotgun.FireInterval=.78f; Shotgun.SpreadDegrees=4.f;
    Shotgun.Range=1400.f; Shotgun.FalloffStart=500.f; Shotgun.MinimumDamageFraction=.2f;
    Shotgun.HeadTraumaScale=1.f; Shotgun.HeavyStaggerThreshold=70.f;
    Shotgun.FlashDuration=.065f; Shotgun.FlashIntensity=27000.f;
    Shotgun.Muzzle=FVector(64.5f,0,14);
    Shotgun.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_PumpShotgun"));
    Shotgun.ForeEndMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_PumpShotgun_ForeEnd"));
    Shotgun.ShellMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_ShotgunShell"));
    Shotgun.MagazineMesh.Reset();
    Shotgun.ReadyAnimation=Clip(TEXT("A_Response_ShotgunReady"));
    Shotgun.EmptySound=Sound(TEXT("S_ShotgunEmpty"));
    Shotgun.ShotSounds={Sound(TEXT("S_ShotgunShot_01")),Sound(TEXT("S_ShotgunShot_02")),Sound(TEXT("S_ShotgunShot_03"))};
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
    // This milestone carries exactly two editable rows; no unbounded inventory framework.
    WeaponDefinitions.SetNum(2); Carried.SetNum(2);
    auto Keep=[this](const auto& Ref) { if (UObject* Object=Ref.LoadSynchronous()) LoadedAssets.AddUnique(Object); };
    for (int32 I=0;I<WeaponDefinitions.Num();++I)
    {
        auto& D=WeaponDefinitions[I]; D.Capacity=FMath::Max(1,D.Capacity); D.Pellets=FMath::Clamp(D.Pellets,1,16);
        Carried[I].Ammo=D.Capacity; Carried[I].Reserve=FMath::Clamp(D.InitialReserve,0,D.ReserveLimit);
        Keep(D.Mesh); Keep(D.ForeEndMesh); Keep(D.ShellMesh); Keep(D.MagazineMesh); Keep(D.ReadyAnimation); Keep(D.EmptySound);
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
    return P && !P->IsDead() && GetAmmo()>0 && !NeedsPump(EquippedIndex) &&
        (Operation==EONEWeaponOperation::Ready || (GetDefinition().bAutomatic && Operation==EONEWeaponOperation::Fire)) && GetTimeSinceShot()>=GetDefinition().FireInterval;
}
void UONEWeaponComponent::SetTrigger(bool Held)
{
    if (Held && !bTrigger)
    {
        bPendingShot=true;
        // A per-shell reload may be interrupted; an already inserted shell remains earned.
        if (GetDefinition().bShellReload && IsReloading() && Operation!=EONEWeaponOperation::ShellEnd) StartOperation(EONEWeaponOperation::ShellEnd);
    }
    bTrigger=Held;
    if (!Held) bPendingShot=false;
}
void UONEWeaponComponent::BeginReload()
{
    const auto* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || (Operation!=EONEWeaponOperation::Ready && Operation!=EONEWeaponOperation::Fire) || NeedsPump(EquippedIndex) || GetAmmo()>=GetDefinition().Capacity || GetReserveAmmo()<=0) return;
    bPendingShot=false;
    StartOperation(GetDefinition().bShellReload ? EONEWeaponOperation::ShellStart : EONEWeaponOperation::MagazineReload);
}
void UONEWeaponComponent::CancelReload()
{
    if (IsReloading()) { StopOperationAudio(); Operation=EONEWeaponOperation::Ready; ++OperationSerial; NextEvent=0; }
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
    if (!P || P->IsDead() || !Carried.IsValidIndex(I) || (I==EquippedIndex && Operation!=EONEWeaponOperation::Equip)) return false;
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
        Carried[I].bNeedsPump=false; Carried[I].bCaseEjected=false;
    }
    LastShot=-100; LastEmpty=-100; RefreshEquippedPresentation();
}
void UONEWeaponComponent::AddReserveAmmo(int32 Count)
{ if (Carried.IsValidIndex(EquippedIndex)) Carried[EquippedIndex].Reserve=FMath::Clamp(GetReserveAmmo()+Count,0,GetDefinition().ReserveLimit); }
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
            if (!State.bCaseEjected)
            {
                State.bCaseEjected=true; ++CasesEjected;
                if (auto* P=Cast<AONEPlayer>(GetOwner()))
                {
                    Cases.RemoveAll([](const auto& C){ return !C.IsValid(); });
                    while (Cases.Num()>=16) { if (Cases[0].IsValid()) Cases[0]->Destroy(); Cases.RemoveAt(0); }
                    const FTransform T=P->Gun->GetComponentTransform();
                    if (auto* C=GetWorld()->SpawnActor<AONEWeaponCase>(T.TransformPosition(D.EjectionPoint),T.Rotator()))
                    { C->Initialize(D.ShellMesh.Get(),T.TransformVectorNoScale(FVector(40,-145,115))); Cases.Add(C); }
                }
            }
            break;
        default: break;
    }
    PlayMechanical(E.Sound.Get());
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
void UONEWeaponComponent::TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Dt,Tick,ThisTick);
    const auto* P=Cast<AONEPlayer>(GetOwner()); if (!P || P->IsDead()) return;
    if (Operation!=EONEWeaponOperation::Ready)
    {
        const int32 Serial=OperationSerial;
        if (const auto* O=FindOperation(OperationIndex,Operation))
            while (OperationSerial==Serial && NextEvent<O->Events.Num() && O->Events[NextEvent].Time<=GetOperationElapsed()) ProcessWeaponEvent(O->Events[NextEvent++]);
        if (Serial==OperationSerial && GetOperationElapsed()>=GetOperationDuration()) FinishOperation();
    }
    if (Operation==EONEWeaponOperation::Ready && NeedsPump(EquippedIndex)) StartOperation(EONEWeaponOperation::Pump);
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
    return T<D.PumpRearTime ? FMath::Clamp(T/FMath::Max(.001f,D.PumpRearTime),0.f,1.f) : FMath::Clamp(1.f-(T-D.PumpRearTime)/FMath::Max(.001f,D.PumpForwardTime-D.PumpRearTime),0.f,1.f);
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
    StartOperation(EONEWeaponOperation::Fire); P->FlashMuzzle();
    const FVector Start=P->GetMuzzleLocation();
    if (auto* S=Choose(D.ShotSounds))
        if (auto* A=UGameplayStatics::SpawnSoundAttached(S,P->Gun,NAME_None,FVector::ZeroVector,EAttachLocation::KeepRelativeOffset,true,D.bPumpAction ? .82f : .62f,FMath::FRandRange(.98f,1.02f)))
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
            auto& Packet=Victims.FindOrAdd(Z); Packet.ShotId=LastShotId; Packet.Direction=Direction; Packet.Position+=End; ++Packet.Pellets; Packet.HeavyStaggerThreshold=D.HeavyStaggerThreshold;
            if (Region==EONEHitRegion::Head) { Packet.HeadDamage+=HitDamage; Packet.HeadTrauma+=HitDamage*D.HeadTraumaScale; }
            else if (Region==EONEHitRegion::Arm) { Packet.ArmDamage+=HitDamage; Packet.ArmTrauma+=HitDamage*D.ArmTraumaScale; }
            else Packet.BodyDamage+=HitDamage;
        }
        else if (SurfaceHits.Num()<2 && !SurfaceHits.ContainsByPredicate([&Hit](const auto& Previous){ return Previous.GetComponent()==Hit.GetComponent(); })) SurfaceHits.Add(Hit);
    }
    bLastHitKill=false;
    for (auto& Pair:Victims)
    {
        auto& Packet=Pair.Value; Packet.Position/=FMath::Max(1,Packet.Pellets);
        if (Pair.Key->ReceiveWeaponDamage(Packet))
        {
            LastHit=GetWorld()->GetTimeSeconds(); bLastHitKill|=Pair.Key->IsDead();
            if (auto* S=Choose(D.FleshSounds)) UGameplayStatics::PlaySoundAtLocation(this,S,Packet.Position,.55f);
        }
    }
    for (const auto& Hit:SurfaceHits)
    {
        const bool Metal=(Hit.GetActor() && Hit.GetActor()->ActorHasTag(TEXT("Metal"))) || (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("Metal")));
        if (auto* S=Choose(Metal ? D.MetalSounds : D.ConcreteSounds)) UGameplayStatics::PlaySoundAtLocation(this,S,Hit.ImpactPoint,.4f);
    }
}
