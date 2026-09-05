#include "ONE04MachinePresentation.h"
#include "ONEWeaponTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DECLARE_CATEGORY_EXTERN(ONEProgression);

namespace
{
    float Smooth(float Value) { const float T=FMath::Clamp(Value,0.f,1.f); return T*T*(3.f-2.f*T); }
    FTransform BlendPose(const FTransform& A,const FTransform& B,float T)
    {
        return FTransform(FQuat::Slerp(A.GetRotation(),B.GetRotation(),T).GetNormalized(),
            FMath::Lerp(A.GetLocation(),B.GetLocation(),T),FVector::OneVector);
    }
}

UONE04MachinePresentation::UONE04MachinePresentation()
{
    PrimaryComponentTick.bCanEverTick=false;
    SetMobility(EComponentMobility::Movable);
}

UStaticMeshComponent* UONE04MachinePresentation::MakePart(const TCHAR* Name,const TCHAR* AssetName,const FVector& Location)
{
    auto* C=NewObject<UStaticMeshComponent>(GetOwner(),Name);
    GetOwner()->AddInstanceComponent(C);C->SetupAttachment(this);
    C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
    const FString Path=FString::Printf(TEXT("/Game/ONE/Machines/Candidate04/%s.%s"),AssetName,AssetName);
    C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));
    C->SetRelativeLocation(Location);C->RegisterComponent();Parts.Add(C);
    if (!C->GetStaticMesh()) UE_LOG(LogTemp,Error,TEXT("C04 machine visual mesh missing: %s"),AssetName);
    for (int32 I=0;I<C->GetNumMaterials();++I)
        if (UMaterialInterface* Material=C->GetMaterial(I))
            if (Material->GetName().StartsWith(TEXT("M_C04_Lamp")) || Material->GetName().StartsWith(TEXT("M_C04_Violet")))
                if (UMaterialInstanceDynamic* MID=C->CreateDynamicMaterialInstance(I)) EmitterMaterials.Add(MID);
    return C;
}

UStaticMeshComponent* UONE04MachinePresentation::MakePreviewPart(const TCHAR* Name)
{
    auto* C=NewObject<UStaticMeshComponent>(GetOwner(),Name);GetOwner()->AddInstanceComponent(C);C->SetupAttachment(PreviewRoot);
    C->SetMobility(EComponentMobility::Movable);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetCollisionResponseToAllChannels(ECR_Ignore);C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);
    C->SetCastShadow(false);C->RegisterComponent();PreviewParts.Add(C);return C;
}

UAudioComponent* UONE04MachinePresentation::MakeAudio(const TCHAR* Name)
{
    auto* A=NewObject<UAudioComponent>(GetOwner(),Name);GetOwner()->AddInstanceComponent(A);A->SetupAttachment(this);
    A->bAutoActivate=false;A->bAutoDestroy=false;A->bStopWhenOwnerDestroyed=true;
    A->bOverrideAttenuation=true;A->AttenuationOverrides.bAttenuate=true;
    A->AttenuationOverrides.bSpatialize=true;A->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
    A->AttenuationOverrides.AttenuationShapeExtents=FVector(160);
    A->AttenuationOverrides.FalloffDistance=1050;A->SetRelativeLocation(FVector(15,0,100));A->RegisterComponent();return A;
}

USoundBase* UONE04MachinePresentation::Sound(const FString& Name)
{
    if (const auto* Existing=Sounds.Find(Name)) return Existing->Get();
    const FString Full=TEXT("S_C04_")+Name;
    USoundBase* Loaded=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/ONE/Audio/Machines/Candidate04/%s.%s"),*Full,*Full));
    Sounds.Add(Name,Loaded);return Loaded;
}

void UONE04MachinePresentation::PlayCue(const TCHAR* Name,float Gain)
{
    if (CueVoices.IsEmpty()) return;
    UAudioComponent* Voice=CueVoices[NextCueVoice++%CueVoices.Num()];Voice->Stop();Voice->SetSound(Sound(Name));
    Voice->SetVolumeMultiplier(Gain);Voice->SetPitchMultiplier(1.f);if (Voice->Sound) Voice->Play();
}

void UONE04MachinePresentation::Configure(bool Box)
{
    if (bConfigured) { ensureMsgf(bBox==Box,TEXT("Machine kind cannot change after presentation construction")); return; }
    if (!GetOwner() || !GetWorld()) return;
    bBox=Box;bConfigured=true;
    if (bBox)
    {
        MakePart(TEXT("ContainmentBody"),TEXT("SM_C04_BoxBody"));
        Lid=MakePart(TEXT("PhysicalLid"),TEXT("SM_C04_BoxLid"),FVector(-48,0,96));
        Rotor=MakePart(TEXT("ContainmentRotor"),TEXT("SM_C04_BoxRotor"),FVector(0,0,52));
        for (int32 I=0;I<2;++I)
        {
            Locks.Add(MakePart(*FString::Printf(TEXT("LidLock%d"),I),TEXT("SM_C04_BoxLock"),FVector(59,I ? 62 : -62,89)));
            Sleeves.Add(MakePart(*FString::Printf(TEXT("LidStrutSleeve%d"),I),TEXT("SM_C04_HydraulicSleeve")));
            Rods.Add(MakePart(*FString::Printf(TEXT("LidStrutRod%d"),I),TEXT("SM_C04_HydraulicRod")));
        }
    }
    else
    {
        MakePart(TEXT("ProcessorBody"),TEXT("SM_C04_UpgradeBody"));
        Cradle=MakePart(TEXT("MovingIntakeCradle"),TEXT("SM_C04_UpgradeCradle"),FVector(94,0,107));
        for (int32 I=0;I<2;++I)
        {
            Clamps.Add(MakePart(*FString::Printf(TEXT("IntakeClamp%d"),I),TEXT("SM_C04_UpgradeClamp")));
            Rings.Add(MakePart(*FString::Printf(TEXT("ProcessingRing%d"),I),TEXT("SM_C04_UpgradeRing"),FVector(-15,I ? 37 : -37,130)));
            Shields.Add(MakePart(*FString::Printf(TEXT("ChamberShield%d"),I),TEXT("SM_C04_UpgradeShield"),FVector(-40,I ? 70 : -70,139)));
        }
    }
    PreviewRoot=NewObject<USceneComponent>(GetOwner(),TEXT("ActualWeaponPreview"));GetOwner()->AddInstanceComponent(PreviewRoot);
    PreviewRoot->SetupAttachment(this);PreviewRoot->SetMobility(EComponentMobility::Movable);PreviewRoot->RegisterComponent();
    MakePreviewPart(TEXT("PreviewBody"));MakePreviewPart(TEXT("PreviewMagazine"));
    MakePreviewPart(TEXT("PreviewForeEnd"));MakePreviewPart(TEXT("PreviewSlide"));
    MachineLight=NewObject<UPointLightComponent>(GetOwner(),TEXT("MachineStateLight"));GetOwner()->AddInstanceComponent(MachineLight);
    MachineLight->SetupAttachment(this);MachineLight->SetMobility(EComponentMobility::Movable);
    MachineLight->SetIntensityUnits(ELightUnits::Lumens);MachineLight->SetAttenuationRadius(400);MachineLight->SetCastShadows(false);
    // Illuminate the cradle from the upper field assembly, away from the
    // receiver and hands, so inverse-square falloff does not erase their detail.
    MachineLight->SetSourceRadius(12);MachineLight->SetRelativeLocation(bBox ? FVector(12,0,116) : FVector(40,0,195));MachineLight->RegisterComponent();
    PreviewLight=NewObject<UPointLightComponent>(GetOwner(),TEXT("PreviewAuraLight"));GetOwner()->AddInstanceComponent(PreviewLight);
    PreviewLight->SetupAttachment(PreviewRoot);PreviewLight->SetMobility(EComponentMobility::Movable);
    PreviewLight->SetIntensityUnits(ELightUnits::Lumens);PreviewLight->SetAttenuationRadius(120);PreviewLight->SetCastShadows(false);
    PreviewLight->SetSourceRadius(7);PreviewLight->SetRelativeLocation(FVector(10,0,8));PreviewLight->RegisterComponent();
    IdleLoop=MakeAudio(TEXT("MachineIdleLoop"));ProcessLoop=MakeAudio(TEXT("MachineProcessLoop"));
    for (int32 I=0;I<4;++I) CueVoices.Add(MakeAudio(*FString::Printf(TEXT("MachineCue%d"),I)));
    const TArray<FString> Names=bBox ? TArray<FString>{TEXT("BoxIdle"),TEXT("BoxActivate"),TEXT("BoxCycle01"),TEXT("BoxCycle02"),TEXT("BoxCycle03"),TEXT("BoxReveal"),TEXT("BoxCollect"),TEXT("BoxClose")} :
        TArray<FString>{TEXT("UpgradeIdle"),TEXT("UpgradeIntake"),TEXT("UpgradeActivate"),TEXT("UpgradeProcess"),TEXT("UpgradeOutput"),TEXT("UpgradeComplete"),TEXT("UpgradeCollect"),TEXT("UpgradeClose")};
    for (const FString& Name:Names) Sound(Name);
    IdleLoop->SetSound(Sound(bBox?TEXT("BoxIdle"):TEXT("UpgradeIdle")));
    ProcessLoop->SetSound(bBox ? nullptr : Sound(TEXT("UpgradeProcess")));
    UpdateVisual(EONE04MachineVisualState::Idle,0,1);
}

FTransform UONE04MachinePresentation::Mount(const FVector& Location) const
{
    FVector Centered=Location;Centered.Y-=PreviewCenterX;
    return FTransform(FRotator(0,90,0),Centered,FVector::OneVector)*GetComponentTransform();
}
FTransform UONE04MachinePresentation::GetIntakeWorldTransform() const { return Mount(bBox ? FVector(0,0,139) : FVector(94,0,107)); }
FTransform UONE04MachinePresentation::GetOutputWorldTransform() const { return GetIntakeWorldTransform(); }
FTransform UONE04MachinePresentation::GetPreviewWorldTransform() const { return PreviewRoot ? PreviewRoot->GetComponentTransform() : GetOutputWorldTransform(); }
void UONE04MachinePresentation::SetPreviewWorld(const FTransform& Transform)
{
    if (!PreviewRoot) return;
    FTransform Pose=Transform;Pose.SetScale3D(FVector::OneVector);PreviewRoot->SetWorldTransform(Pose);
}

void UONE04MachinePresentation::SetPreview(const FONEWeaponDefinition* Definition)
{
    if (!bConfigured || PreviewParts.Num()!=4) return;
    const bool HadPreview=bPreviewValid;const FTransform Previous=GetPreviewWorldTransform();
    ExpectedPreviewParts=0;bPreviewValid=false;PreviewAura=FLinearColor::Transparent;
    PreviewFamily=Definition ? Definition->Family : EONEWeaponFamily::Invalid;
    TArray<UStaticMesh*> Meshes;
    if (Definition)
    {
        Meshes={Definition->Mesh.LoadSynchronous(),Definition->MagazineMesh.LoadSynchronous(),Definition->ForeEndMesh.LoadSynchronous(),Definition->SlideMesh.LoadSynchronous()};
        ExpectedPreviewParts=1+!Definition->MagazineMesh.IsNull()+!Definition->ForeEndMesh.IsNull()+!Definition->SlideMesh.IsNull();
        bPreviewValid=Meshes[0]!=nullptr;PreviewAura=Definition->AuraColor;
    }
    FBox PreviewBounds(ForceInit);
    for (int32 I=0;I<4;++I)
    {
        UStaticMesh* Mesh=Meshes.IsValidIndex(I)?Meshes[I]:nullptr;
        PreviewParts[I]->SetStaticMesh(Mesh);PreviewParts[I]->SetRelativeTransform(FTransform::Identity);
        PreviewParts[I]->SetVisibility(Mesh!=nullptr);
        if (Mesh) PreviewBounds+=Mesh->GetBoundingBox();
    }
    PreviewCenterX=PreviewBounds.IsValid?PreviewBounds.GetCenter().X:0;
    if (!Definition) { bTransferring=false;bRetrieving=false;PreviewRoot->SetVisibility(false,true); }
    else SetPreviewWorld(HadPreview?Previous:GetOutputWorldTransform());
    PreviewLight->SetLightColor(PreviewAura);PreviewLight->SetIntensity(bPreviewValid && PreviewAura.A>0 ? 15.f : 0.f);
}

void UONE04MachinePresentation::BeginTransferFrom(const FTransform& ActualGunWorld)
{
    if (!PreviewRoot || !bPreviewValid) return;
    TransferStart=ActualGunWorld;TransferStart.SetScale3D(FVector::OneVector);TransferStartTime=GetWorld()->GetTimeSeconds();
    bTransferring=true;bRetrieving=false;SetPreviewWorld(TransferStart);PreviewRoot->SetVisibility(true,true);
    TransferStartErrorCm=FVector::Distance(ActualGunWorld.GetLocation(),GetPreviewWorldTransform().GetLocation());
}
void UONE04MachinePresentation::BeginRetrievalTo(const FTransform& HandWorld)
{
    if (!PreviewRoot || !bPreviewValid) return;
    RetrievalTarget=HandWorld;RetrievalTarget.SetScale3D(FVector::OneVector);
    if (bRetrieving) return; // Following the current hand does not restart the transfer.
    RetrievalStart=GetPreviewWorldTransform();RetrievalStartTime=GetWorld()->GetTimeSeconds();bRetrieving=true;bTransferring=false;
    SetPreviewWorld(RetrievalStart);RetrievalStartErrorCm=FVector::Distance(RetrievalStart.GetLocation(),GetPreviewWorldTransform().GetLocation());
}

void UONE04MachinePresentation::PlayCycleCue()
{
    if (!bBox || !bConfigured || PreviousState!=EONE04MachineVisualState::Active) return;
    const FString Name=FString::Printf(TEXT("BoxCycle%02d"),1+CycleCueCount%3);++CycleCueCount;PlayCue(*Name,.8f);
}
void UONE04MachinePresentation::UpdateEmitters(const FLinearColor& Color,float Gain,float Intensity)
{
    for (UMaterialInstanceDynamic* MID:EmitterMaterials) if (MID) { MID->SetVectorParameterValue(TEXT("GlowColor"),Color);MID->SetScalarParameterValue(TEXT("GlowStrength"),Gain); }
    MachineLight->SetLightColor(Color);MachineLight->SetIntensity(Intensity);
}
void UONE04MachinePresentation::UpdateStrut(int32 Index,const FVector& Bottom,const FVector& Top)
{
    const FVector Delta=Top-Bottom;const FVector Direction=Delta.GetSafeNormal();const float Length=Delta.Length();
    const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Direction);
    const float SleeveLength=FMath::Min(28.f,Length*.63f);
    Sleeves[Index]->SetRelativeTransform(FTransform(Rotation,Bottom+Direction*(SleeveLength*.5f),FVector(1,1,SleeveLength)));
    Rods[Index]->SetRelativeTransform(FTransform(Rotation,Bottom+Direction*((Length+SleeveLength)*.5f),FVector(1,1,FMath::Max(1.f,Length-SleeveLength))));
}

void UONE04MachinePresentation::UpdateVisual(EONE04MachineVisualState State,float Elapsed,float Duration)
{
    CSV_SCOPED_TIMING_STAT(ONEProgression,MachineVisual);
    if (!bConfigured) return;
    if (State==EONE04MachineVisualState::Disabled) { Shutdown();return; }
    Elapsed=FMath::Max(0.f,Elapsed);Duration=FMath::Max(.01f,Duration);
    const bool Active=State==EONE04MachineVisualState::Active,Ready=State==EONE04MachineVisualState::Ready;
    if (State!=PreviousState)
    {
        if (!IdleLoop->IsPlaying() && IdleLoop->Sound) IdleLoop->FadeIn(.4f,.6f);
        if (Active)
        {
            bClampCue=false;bOutputCue=false;
            PlayCue(bBox?TEXT("BoxActivate"):TEXT("UpgradeIntake"),.9f);
        }
        else if (Ready)
        {
            ProcessLoop->FadeOut(.15f,0);PlayCue(bBox?TEXT("BoxReveal"):TEXT("UpgradeComplete"),.95f);
        }
        else if (State==EONE04MachineVisualState::Closing)
        {
            ProcessLoop->FadeOut(.12f,0);PlayCue(bBox?TEXT("BoxCollect"):TEXT("UpgradeCollect"),.8f);
            PlayCue(bBox?TEXT("BoxClose"):TEXT("UpgradeClose"),.8f);
        }
        else ProcessLoop->FadeOut(.15f,0);
        PreviousState=State;
    }
    const double Now=GetWorld()->GetTimeSeconds();
    FVector PreviewMount=bBox ? FVector(0,0,139) : FVector(94,0,107);
    if (bBox)
    {
        float Open=Ready?1.f:0.f;
        if (Active) Open=Smooth((Elapsed-.16f)/.55f);
        if (State==EONE04MachineVisualState::Closing) Open=1.f-Smooth(Elapsed/Duration);
        LidOpenDegrees=108.f*Open;Lid->SetRelativeRotation(FRotator(LidOpenDegrees,0,0));
        const float Unlock=Active?Smooth(Elapsed/.17f):Open;
        for (int32 I=0;I<2;++I)
        {
            const float Side=I?1.f:-1.f;Locks[I]->SetRelativeLocation(FVector(59,Side*(62+13*Unlock),89));
            const FVector Top=Lid->GetRelativeTransform().TransformPosition(FVector(45,Side*80,0));
            UpdateStrut(I,FVector(-28,Side*80,50),Top);
        }
        Rotor->SetRelativeRotation(FRotator(0,float(Now*(Active?37.:8.)),0));
        const float Pulse=Active ? .91f+.09f*FMath::Sin(float(Now)*8.f) : 1.f;
        UpdateEmitters(FLinearColor(.42f,.13f,1.f),Ready?5.f:(Active?4.5f*Pulse:1.f),Ready?4200.f:(Active?6000.f*Pulse:360.f));
        if (Active || Ready) PreviewMount.Z+=.7f*FMath::Sin(float(Now)*2.1f);
    }
    else
    {
        const float T=Active?Elapsed/Duration*9.f:0.f;
        float Feed=0.f;
        if (Active) Feed=T<1.2f ? Smooth((T-.35f)/.85f) : (T>7.7f?1.f-Smooth((T-7.7f)/1.3f):1.f);
        PreviewMount=FMath::Lerp(FVector(94,0,107),FVector(-5,0,121),Feed);Cradle->SetRelativeLocation(PreviewMount);
        const float Clamped=Active ? FMath::Min(Smooth((T-.60f)/.28f),1.f-Smooth((T-8.38f)/.38f)) : 0.f;
        for (int32 I=0;I<2;++I)
        {
            const float Side=I?1.f:-1.f;
            Clamps[I]->SetRelativeLocation(PreviewMount+FVector(Side*FMath::Lerp(38.f,22.f,Clamped),Side*24,0));
            Clamps[I]->SetRelativeRotation(FRotator(0,I?0:180,0));
            Rings[I]->SetRelativeRotation(FRotator(float(Now)*(Active?38.f:3.f)*Side,0,0));
            Shields[I]->SetRelativeLocation(FVector(-40,Side*(70-22*Feed),139));
        }
        if (Active && T>=.75f && !bClampCue) { bClampCue=true;PlayCue(TEXT("UpgradeActivate"),.8f); }
        if (Active && T>=1.2f && T<7.7f)
        {
            if (!ProcessLoop->IsPlaying() && ProcessLoop->Sound) ProcessLoop->FadeIn(.25f,.4f);
            ProcessLoop->SetVolumeMultiplier(FMath::Lerp(.4f,.85f,(T-1.2f)/6.5f));
        }
        if (Active && T>=7.7f && !bOutputCue) { bOutputCue=true;ProcessLoop->FadeOut(.2f,0);PlayCue(TEXT("UpgradeOutput"),.85f); }
        const float Pulse=Active?.85f+.15f*FMath::Sin(float(Now)*11.f):1.f;
        const FLinearColor Color=Ready?FLinearColor(.15f,1.f,.55f):(Active?FLinearColor(.17f,.65f,1.f):FLinearColor(1.f,.36f,.075f));
        UpdateEmitters(Color,Ready?5.f:(Active?5.f*Pulse:1.2f),Ready?1600.f:(Active?1800.f*Pulse:450.f));
    }
    if (bRetrieving)
    {
        const float T=Smooth(float(Now-RetrievalStartTime)/.18f);SetPreviewWorld(BlendPose(RetrievalStart,RetrievalTarget,T));
    }
    else if (bTransferring)
    {
        const float T=Smooth(float(Now-TransferStartTime)/.42f);SetPreviewWorld(BlendPose(TransferStart,Mount(PreviewMount),T));
        if (T>=1.f) bTransferring=false;
    }
    else SetPreviewWorld(Mount(PreviewMount));
    const bool Show=bPreviewValid && (bRetrieving || bTransferring || Ready || (Active && (!bBox || Elapsed>=.57f)));
    PreviewRoot->SetVisibility(Show,false);
    for (UStaticMeshComponent* Part:PreviewParts) Part->SetVisibility(Show && Part->GetStaticMesh()!=nullptr);
    PreviewLight->SetVisibility(Show && PreviewAura.A>0);
    PreviewLight->SetIntensity(Show && PreviewAura.A>0 ? 15.f : 0.f);
    IdleLoop->SetVolumeMultiplier(Active?.22f:.6f);
}

bool UONE04MachinePresentation::HasCompletePreview() const
{
    if (!bPreviewValid || ExpectedPreviewParts<=0) return false;
    int32 Count=0;for (const UStaticMeshComponent* Part:PreviewParts) Count+=Part && Part->GetStaticMesh()!=nullptr;
    return Count==ExpectedPreviewParts;
}
bool UONE04MachinePresentation::IsConfigured() const
{
    if (!bConfigured || Parts.IsEmpty() || PreviewParts.Num()!=4 || !PreviewRoot || !MachineLight || !PreviewLight || !IdleLoop || !ProcessLoop) return false;
    for (const UStaticMeshComponent* Part:Parts) if (!Part || !Part->GetStaticMesh()) return false;
    for (const auto& Entry:Sounds) if (!Entry.Value) return false;
    return true;
}
int32 UONE04MachinePresentation::GetVisiblePreviewPartCount() const
{
    int32 Count=0;for (const UStaticMeshComponent* Part:PreviewParts) Count+=Part && Part->IsVisible() && Part->GetStaticMesh()!=nullptr;return Count;
}
int32 UONE04MachinePresentation::GetActiveLoopCount() const { return (IdleLoop && IdleLoop->IsPlaying())+(ProcessLoop && ProcessLoop->IsPlaying()); }
float UONE04MachinePresentation::GetMachineLightIntensity() const { return MachineLight?MachineLight->Intensity:0.f; }
void UONE04MachinePresentation::Shutdown()
{
    if (IdleLoop) IdleLoop->Stop();if (ProcessLoop) ProcessLoop->Stop();for (UAudioComponent* Voice:CueVoices) if (Voice) Voice->Stop();
    if (MachineLight) MachineLight->SetIntensity(0);if (PreviewLight) PreviewLight->SetIntensity(0);
    if (PreviewRoot) PreviewRoot->SetVisibility(false,true);
    for (UMaterialInstanceDynamic* MID:EmitterMaterials) if (MID) MID->SetScalarParameterValue(TEXT("GlowStrength"),0);
    bTransferring=false;bRetrieving=false;PreviousState=EONE04MachineVisualState::Disabled;
}
void UONE04MachinePresentation::EndPlay(const EEndPlayReason::Type Reason) { Shutdown();Super::EndPlay(Reason); }
