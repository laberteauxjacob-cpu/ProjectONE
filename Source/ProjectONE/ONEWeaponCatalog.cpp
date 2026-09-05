#include "ONEWeaponCatalog.h"
#include "Animation/AnimSequence.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundBase.h"

namespace ONEWeaponCatalogPrivate
{
    template<class T> TSoftObjectPtr<T> Asset(const FString& Folder,const FString& Name)
    { int32 Slash=INDEX_NONE; Name.FindLastChar(TEXT('/'),Slash); return TSoftObjectPtr<T>(FSoftObjectPath(Folder+Name+TEXT(".")+Name.Mid(Slash+1))); }
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
}

TArray<FONEWeaponDefinition> ONEWeaponCatalog::BuildDefaults()
{
    using namespace ONEWeaponCatalogPrivate;
    TArray<FONEWeaponDefinition> WeaponDefinitions;
    FONEWeaponDefinition Carbine;
    Carbine.Id=TEXT("AR01"); Carbine.Family=EONEWeaponFamily::Carbine; Carbine.DisplayName=FText::FromString(TEXT("M4A1"));
    Carbine.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_M4A1_Body"));
    Carbine.MagazineMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_M4A1_Magazine"));
    Carbine.Muzzle=FVector(54.5f,0,14);
    Carbine.ReadyAnimation=Clip(TEXT("A_Response_Idle"));
    Carbine.EmptySound=Sound(TEXT("S_CarbineEmpty"));
    Carbine.EjectedCaseMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/"),TEXT("SM_RifleBrass_C03"));
    Carbine.EjectionPoint=FVector(3.f,-3.7f,14.f);
    for (int32 I=1;I<=6;++I) Carbine.ShotSounds.Add(Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate03/"),FString::Printf(TEXT("S_C03_CarbineShot_%02d"),I)));
    Carbine.FleshSounds={Sound(TEXT("S_FleshImpact_01")),Sound(TEXT("S_FleshImpact_02")),Sound(TEXT("S_FleshImpact_03"))};
    Carbine.ConcreteSounds={Sound(TEXT("S_ConcreteImpact_01")),Sound(TEXT("S_ConcreteImpact_02"))};
    Carbine.MetalSounds={Sound(TEXT("S_MetalImpact_01")),Sound(TEXT("S_MetalImpact_02"))};
    AddOperation(Carbine,EONEWeaponOperation::Equip,.36f,TEXT("A_Response_Equip"),{Event(.18f,EONEWeaponEvent::WeaponSwap,TEXT("S_WeaponEquip"))});
    AddOperation(Carbine,EONEWeaponOperation::Fire,.2f,TEXT("A_Response_Fire"),{Event(0.f,EONEWeaponEvent::ShellEject,nullptr)});
    AddOperation(Carbine,EONEWeaponOperation::MagazineReload,2.1f,TEXT("Candidate04/A_Response_C04_CarbineReload"),{
        Event(.4f,EONEWeaponEvent::MagazineOut,TEXT("S_CarbineMagOut")),Event(1.2f,EONEWeaponEvent::MagazineCommit,TEXT("S_CarbineMagIn")),Event(1.74f,EONEWeaponEvent::Sound,TEXT("S_CarbineBolt"))});
    WeaponDefinitions.Add(Carbine);
    FONEWeaponDefinition Shotgun=Carbine;
    Shotgun.Id=TEXT("SG01"); Shotgun.Family=EONEWeaponFamily::Shotgun; Shotgun.DisplayName=FText::FromString(TEXT("Remington 870"));
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
    Shotgun.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_Remington870_Body"));
    Shotgun.ForeEndMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_Remington870_ForeEnd"));
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
    FONEWeaponDefinition Pistol=Carbine;
    Pistol.Id=TEXT("P1911"); Pistol.Family=EONEWeaponFamily::Pistol; Pistol.DisplayName=FText::FromString(TEXT("M1911"));
    Pistol.bAutomatic=false; Pistol.Capacity=7; Pistol.InitialReserve=56; Pistol.ReserveLimit=84; Pistol.RoundReserveReward=14;
    Pistol.Damage=28.f; Pistol.FireInterval=.24f; Pistol.SpreadDegrees=.25f;
    Pistol.Range=2400.f; Pistol.FalloffStart=1000.f; Pistol.MinimumDamageFraction=.55f;
    Pistol.Muzzle=FVector(16.5f,0,5.5f); Pistol.EjectionPoint=FVector(5.7f,-1.5f,5.5f);
    Pistol.MagazineFreshTime=.64f;
    Pistol.MagazineHandOffset=FVector(.5f,0,1.8f);
    Pistol.FlashLength=13.f; Pistol.FlashRadius=3.f; Pistol.FlashIntensity=13000.f; Pistol.FlashLightRadius=165.f;
    Pistol.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_M1911_Body"));
    Pistol.SlideMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_M1911_Slide"));
    Pistol.MagazineMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_M1911_Magazine"));
    Pistol.EjectedCaseMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_M1911_Case"));
    Pistol.ReadyAnimation=Clip(TEXT("Candidate04/A_Response_C04_PistolReady"));
    Pistol.EmptySound=Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate04/"),TEXT("S_C04_PistolEmpty"));
    Pistol.ShotSounds.Reset();
    for (int32 I=1;I<=6;++I) Pistol.ShotSounds.Add(Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate04/"),FString::Printf(TEXT("S_C04_M1911Shot_%02d"),I)));
    Pistol.Operations.Reset();
    AddOperation(Pistol,EONEWeaponOperation::Equip,.36f,TEXT("Candidate04/A_Response_C04_PistolEquip"),{Event(.18f,EONEWeaponEvent::WeaponSwap,TEXT("S_WeaponEquip"))});
    AddOperation(Pistol,EONEWeaponOperation::Fire,.18f,TEXT("Candidate04/A_Response_C04_PistolFire"),{Event(0.f,EONEWeaponEvent::ShellEject,nullptr)});
    AddOperation(Pistol,EONEWeaponOperation::MagazineReload,1.8f,TEXT("Candidate04/A_Response_C04_PistolReload"),{
        Event(.28f,EONEWeaponEvent::MagazineOut,nullptr),Event(1.1f,EONEWeaponEvent::MagazineCommit,nullptr),Event(1.4f,EONEWeaponEvent::Sound,nullptr)});
    const TCHAR* PistolMechanics[]={TEXT("S_C04_PistolMagOut"),TEXT("S_C04_PistolMagIn"),TEXT("S_C04_PistolSlide")};
    for (int32 I=0;I<3;++I) Pistol.Operations.Last().Events[I].Sound=Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate04/"),PistolMechanics[I]);
    WeaponDefinitions.Add(Pistol);
    // Effective rows are independent copies. Upgrading an owned instance never
    // mutates base catalog tuning or future box rewards.
    for (int32 I=0;I<3;++I)
    {
        FONEWeaponDefinition D=WeaponDefinitions[I]; D.bUpgraded=true;
        D.Id=FName(*(D.Id.ToString()+TEXT("_UP"))); D.Damage*=2.f; D.FireInterval/=1.15f;
        D.InitialReserve*=2; D.ReserveLimit*=2;
        const TCHAR* Name=I==0 ? TEXT("Overcurrent") : I==1 ? TEXT("Gravebreaker") : TEXT("Last Word");
        const TCHAR* MeshPrefix=I==0 ? TEXT("SM_Overcurrent") : I==1 ? TEXT("SM_Gravebreaker") : TEXT("SM_LastWord");
        D.DisplayName=FText::FromString(Name);
        D.AuraColor=I==0 ? FLinearColor(.05f,.85f,1.f) : I==1 ? FLinearColor(1.f,.24f,.035f) : FLinearColor(.60f,.12f,1.f);
        D.TraceColor=D.AuraColor; D.FlashLightColor=FMath::Lerp(D.AuraColor,FLinearColor::White,.48f);
        D.Mesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),FString(MeshPrefix)+TEXT("_Body"));
        if (I==1) D.ForeEndMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),FString(MeshPrefix)+TEXT("_ForeEnd"));
        else D.MagazineMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),FString(MeshPrefix)+TEXT("_Magazine"));
        if (I==2) D.SlideMesh=Asset<UStaticMesh>(TEXT("/Game/ONE/Art/Weapons/Candidate04/"),TEXT("SM_LastWord_Slide"));
        D.Capacity=I==0 ? 36 : I==1 ? 8 : 14;
        if (I==0) D.SpreadDegrees=.25f;
        if (I==1) D.HeavyStaggerThreshold=60.f;
        if (I==2) D.AdditionalVictims=1;
        D.PumpRearTime/=1.15f; D.PumpForwardTime/=1.15f;
        for (auto& O:D.Operations) if (O.Operation==EONEWeaponOperation::Fire || O.Operation==EONEWeaponOperation::Pump)
        { O.Duration/=1.15f; for (auto& E:O.Events) E.Time/=1.15f; }
        D.ShotSounds.Reset();
        const TCHAR* SoundPrefix=I==0 ? TEXT("Overcurrent") : I==1 ? TEXT("Gravebreaker") : TEXT("LastWord");
        for (int32 N=1;N<=6;++N) D.ShotSounds.Add(Asset<USoundBase>(TEXT("/Game/ONE/Audio/Weapons/Candidate04/"),FString::Printf(TEXT("S_C04_%sShot_%02d"),SoundPrefix,N)));
        WeaponDefinitions.Add(D);
    }
    return WeaponDefinitions;
}

const FONEWeaponDefinition& ONEWeaponCatalog::Unarmed()
{
    static const FONEWeaponDefinition D=[] {
        FONEWeaponDefinition V; V.Id=TEXT("Unarmed"); V.Family=EONEWeaponFamily::Invalid;
        V.DisplayName=FText::FromString(TEXT("Unarmed")); V.Capacity=0; V.InitialReserve=0; V.ReserveLimit=0;
        V.RoundReserveReward=0; V.Damage=0; V.Range=0; V.Pellets=0; V.bAutomatic=false;
        return V;
    }();
    return D;
}
