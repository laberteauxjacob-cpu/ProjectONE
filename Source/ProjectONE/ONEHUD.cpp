#include "ONEHUD.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEWeaponComponent.h"
#include "ONEInteractionComponent.h"
#include "ONEProgressionMachine.h"
#include "ONE04MachinePresentation.h"
#include "ONE03MovementCheck.h"
#include "ONE03PresentationCheck.h"
#include "ONE03PhysicalityCheck.h"
#include "ONE04PresentationCheck.h"
#include "ONE05PresentationCheck.h"
#include "ONE05MotionCheck.h"
#include "ONEPowerUpComponent.h"
#include "HAL/IConsoleManager.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"

namespace ONEUI
{
    const FLinearColor Paper(.94f,.97f,.93f),Muted(.56f,.66f,.67f),Teal(.30f,.88f,.76f);
    const FLinearColor Gold(1.f,.80f,.20f),Red(1.f,.25f,.20f),Ink(.018f,.027f,.035f,.94f);
    FString FamilyName(EONEWeaponFamily F)
    { return F==EONEWeaponFamily::Pistol ? TEXT("M1911") : F==EONEWeaponFamily::Carbine ? TEXT("M4A1") : F==EONEWeaponFamily::Shotgun ? TEXT("Remington 870") : TEXT("Opening"); }
}
using namespace ONEUI;

void AONEHUD::BeginPlay()
{
    Super::BeginPlay();
    auto Load=[](const TCHAR* Name)
    { return LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/ONE/UI/Candidate05/%s.%s"),Name,Name)); };
    Glyphs=Load(TEXT("T_UI05Glyphs")); Numerals=Load(TEXT("T_UI05Numerals")); PanelMask=Load(TEXT("T_UI05Panel"));
    for (const TCHAR* Name : {TEXT("T_UI05_M4A1"),TEXT("T_UI05_Overcurrent"),TEXT("T_UI05_Remington870"),TEXT("T_UI05_Gravebreaker"),TEXT("T_UI05_M1911"),TEXT("T_UI05_LastWord")}) WeaponIcons.Add(Load(Name));
    if (!HasCompleteArtwork()) UE_LOG(LogTemp,Error,TEXT("ONE05_UI_ARTWORK_MISSING: import all nine original Candidate05 UI textures before visual validation."));
}
bool AONEHUD::HasCompleteArtwork() const
{
    if (!Glyphs || !Numerals || !PanelMask || WeaponIcons.Num()!=6) return false;
    for (const auto& Texture:WeaponIcons) if (!Texture) return false;
    return true;
}
int32 AONEHUD::UIContext() const
{
    const auto* GM=GetWorld() ? GetWorld()->GetAuthGameMode<AONEGameMode>() : nullptr;
    if (GM && GM->IsGameOver()) return 3;
    if (PlayerOwner && PlayerOwner->IsPaused()) return 2;
    return bTools ? 1 : 0;
}
bool AONEHUD::IsPointerUIActive() const { return UIContext()!=0; }
void AONEHUD::ToggleTools() { if (UIContext()<2) bTools=!bTools; PressedAction=EONEUIAction::None; Buttons.Reset(); }
void AONEHUD::CloseTools() { bTools=false; PressedAction=EONEUIAction::None; Buttons.Reset(); }
void AONEHUD::CycleShakeStrength()
{
    if (auto* Setting=IConsoleManager::Get().FindConsoleVariable(TEXT("one.CameraShake.Strength")))
    {
        const float Current=FMath::Clamp(Setting->GetFloat(),0.f,1.f);
        Setting->Set(Current>.75f?.5f:Current>.01f?0.f:1.f,ECVF_SetByGameSetting);
    }
}
bool AONEHUD::HandlePointerPressed(const FVector2D& Position)
{
    PressedAction=EONEUIAction::None; PressedContext=UIContext();
    if (!PressedContext) return false;
    for (const FButton& B:Buttons) if (B.Enabled && B.Rect.IsInside(Position)) { PressedAction=B.Action; break; }
    return true;
}
bool AONEHUD::HandlePointerReleased(const FVector2D& Position)
{
    const EONEUIAction Action=PressedAction; PressedAction=EONEUIAction::None;
    if (!IsPointerUIActive()) return false;
    if (PressedContext!=UIContext() || Action==EONEUIAction::None) return true;
    for (const FButton& B:Buttons) if (B.Enabled && B.Action==Action && B.Rect.IsInside(Position))
    {
        if (auto* PC=Cast<AONEPlayerController>(PlayerOwner)) PC->ExecuteUIAction(Action);
        break;
    }
    return true;
}
bool AONEHUD::GetActionBounds(EONEUIAction Action,FBox2D& Rect) const
{
    for (const FButton& B:Buttons) if (B.Action==Action && B.Enabled) { Rect=B.Rect; return true; }
    return false;
}
void AONEHUD::RecordPanel(FName Name,float X,float Y,float Width,float Height)
{ LayoutBounds.Add(Name,FBox2D(FVector2D(X,Y),FVector2D(X+Width,Y+Height))); }
int32 AONEHUD::GetLayoutOverlapCount() const
{
    // Only peer essential panels are recorded. Intentional inner bars, keycaps,
    // menu dimmers and modal overlays are not peer layout collisions.
    TArray<FBox2D> Rects; LayoutBounds.GenerateValueArray(Rects); int32 Result=0;
    for (int32 I=0;I<Rects.Num();++I) for (int32 J=I+1;J<Rects.Num();++J)
        if (FMath::Min(Rects[I].Max.X,Rects[J].Max.X)-FMath::Max(Rects[I].Min.X,Rects[J].Min.X)>.5f &&
            FMath::Min(Rects[I].Max.Y,Rects[J].Max.Y)-FMath::Max(Rects[I].Min.Y,Rects[J].Min.Y)>.5f) ++Result;
    return Result;
}
float AONEHUD::Advance(TCHAR C,float Height) const { return Height*(C==TEXT(' ')?.32f:.57f); }
float AONEHUD::TextWidth(const FString& Value,float Height) const
{ float Result=0; for (TCHAR C:Value) Result+=Advance(C,Height); return Result; }
void AONEHUD::Text(const FString& Value,float X,float Y,float Height,FLinearColor Color)
{
    if (!Glyphs) { DrawText(Value,Color,X,Y,GEngine->GetMediumFont(),Height/24.f,false); return; }
    for (TCHAR C:Value)
    {
        const int32 Code=(C>=32 && C<=126) ? int32(C) : int32('?');
        if (Code!=32)
        {
            const int32 Index=Code-32;
            DrawTexture(Glyphs,X,Y,Height*(64.f/96.f),Height,(Index%16)/16.f,(Index/16)/6.f,1.f/16.f,1.f/6.f,Color,BLEND_Translucent);
        }
        X+=Advance(C,Height);
    }
}
void AONEHUD::CenterText(const FString& Value,float X,float Y,float Height,FLinearColor Color)
{ Text(Value,X-TextWidth(Value,Height)*.5f,Y,Height,Color); }
float AONEHUD::Number(int32 Value,float X,float Y,float Height,FLinearColor Color)
{
    const FString Digits=FString::FromInt(FMath::Max(0,Value));
    if (!Numerals) { Text(Digits,X,Y,Height,Color); return TextWidth(Digits,Height); }
    for (TCHAR C:Digits)
    { DrawTexture(Numerals,X,Y,Height*.8f,Height,(int32(C)-int32('0'))*.1f,0,.1f,1,Color,BLEND_Translucent); X+=Height*.66f; }
    return Digits.Len()*Height*.66f;
}
void AONEHUD::Rounded(float X,float Y,float Width,float Height,FLinearColor Color,float Radius)
{
    if (Width<=0 || Height<=0) return;
    if (!PanelMask) { DrawRect(Color,X,Y,Width,Height); return; }
    const float R=FMath::Min3(Radius*K,Width*.5f,Height*.5f),U=.25f;
    const float XS[4]={X,X+R,X+Width-R,X+Width},YS[4]={Y,Y+R,Y+Height-R,Y+Height};
    const float UV[4]={0,U,1-U,1};
    for (int32 I=0;I<3;++I) for (int32 J=0;J<3;++J)
        DrawTexture(PanelMask,XS[I],YS[J],XS[I+1]-XS[I],YS[J+1]-YS[J],UV[I],UV[J],UV[I+1]-UV[I],UV[J+1]-UV[J],Color,BLEND_Translucent);
}
void AONEHUD::Keycap(const FString& Key,float X,float Y,float Width)
{
    Rounded(X,Y,Width*K,28*K,FLinearColor(.15f,.21f,.23f,.98f),6);
    const float Font=FMath::Min(18.f,(Width-7.f)/FMath::Max(1,Key.Len())/.57f)*K;
    CenterText(Key,X+Width*K*.5f,Y+(28*K-Font)*.5f,Font,Paper);
}
void AONEHUD::Button(EONEUIAction Action,const FString& Value,const FString& Key,float X,float Y,float Width,float Height,bool Primary,bool Enabled)
{
    const FBox2D Rect(FVector2D(X,Y),FVector2D(X+Width,Y+Height));
    float MX=0,MY=0; const bool Hover=PlayerOwner->GetMousePosition(MX,MY) && Rect.IsInside(FVector2D(MX,MY));
    FLinearColor Fill=Primary ? Teal : FLinearColor(.105f,.155f,.175f,.98f);
    if (Hover && Enabled) Fill=Fill*1.22f;
    Fill.A=Enabled ? 1.f : .45f; Rounded(X,Y,Width,Height,Fill,10);
    const float Font=FMath::Min(23*K,(Width-(Key.IsEmpty()?32:72)*K)/FMath::Max(1,Value.Len())/.57f);
    Text(Value,X+16*K,Y+(Height-Font)*.5f,Font,Enabled ? (Primary ? Ink : Paper) : Muted);
    if (!Key.IsEmpty()) Keycap(Key,X+Width-52*K,Y+(Height-28*K)*.5f,40);
    Buttons.Add({Action,Rect,Enabled});
}
void AONEHUD::Icon(EONEWeaponFamily Family,bool Upgraded,float X,float Y,float Width,float Height,float Alpha)
{
    const int32 Index=int32(Family)*2+(Upgraded?1:0);
    if (Family==EONEWeaponFamily::Invalid || !WeaponIcons.IsValidIndex(Index) || !WeaponIcons[Index]) return;
    const float IH=FMath::Min(Height,Width*384.f/1024.f),IW=IH*1024.f/384.f;
    DrawTexture(WeaponIcons[Index],X+(Width-IW)*.5f,Y+(Height-IH)*.5f,IW,IH,0,0,1,1,FLinearColor(1,1,1,Alpha),BLEND_Translucent);
}
TArray<FString> AONEHUD::Wrap(const FString& Value,float Width,float Height) const
{
    TArray<FString> Words,Lines; Value.ParseIntoArray(Words,TEXT(" "),true); FString Line;
    for (const FString& Word:Words)
    {
        const FString Next=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;
        if (!Line.IsEmpty() && TextWidth(Next,Height)>Width) { Lines.Add(Line); Line=Word; } else Line=Next;
    }
    if (!Line.IsEmpty()) Lines.Add(Line);
    return Lines;
}
void AONEHUD::UpdateSurvivalPresentation(const AONEPlayer* Player,const AONEGameMode* GM)
{
    const double Now=GetWorld()->GetTimeSeconds();
    const auto* Effects=GM->GetPowerUps();
    const FGuid Run=Effects?Effects->GetRunId():FGuid();
    if (VisualPlayer.Get()!=Player || VisualRun!=Run)
    {
        VisualPlayer=const_cast<AONEPlayer*>(Player); VisualRun=Run; VisualTime=Now;
        ObservedCombatGainSerial=ObservedPickupSerial=0;
        DisplayedCombatGain=0; GainUpdatedAt=PickupPresentedAt=-10.; LastPickupType=EONEPowerUpType::Count;
        HealthEdgeSeverity=0.f; DamagePulseStrength=0.f;
    }
    const float Dt=FMath::Clamp(float(Now-VisualTime),0.f,.1f); VisualTime=Now;
    if (GM->IsGameOver() || Player->GetHealth()<=0.f)
    {
        HealthEdgeSeverity=DamagePulseStrength=0.f; DisplayedCombatGain=0;
        LastPickupType=EONEPowerUpType::Count; return;
    }
    const float Missing=1.f-FMath::Clamp(Player->GetHealth()/FMath::Max(1.f,Player->GetMaxHealth()),0.f,1.f);
    // Gameplay time freezes in pause. Full health always clears both layers,
    // including reset/restore; there is no persistent full-screen red fill.
    HealthEdgeSeverity=Missing<=KINDA_SMALL_NUMBER?0.f:FMath::FInterpTo(HealthEdgeSeverity,Missing,Dt,12.f);
    DamagePulseStrength=Missing<=KINDA_SMALL_NUMBER?0.f:FMath::Clamp((.38f-Player->GetDamageReactionAge())/.38f,0.f,1.f);
    const uint64 GainSerial=GM->GetCombatGainSerial();
    if (GainSerial!=ObservedCombatGainSerial)
    {
        ObservedCombatGainSerial=GainSerial;
        const int32 Gain=FMath::Max(0,GM->GetLastCombatGain());
        if (Gain>0)
        {
            const int64 Carry=Now-GainUpdatedAt<.45?DisplayedCombatGain:0;
            DisplayedCombatGain=int32(FMath::Min<int64>(999999,Carry+Gain)); GainUpdatedAt=Now;
        }
    }
    if (Now-GainUpdatedAt>1.15) DisplayedCombatGain=0;
    if (Effects && Effects->GetPickupNotificationSerial()!=ObservedPickupSerial)
    {
        ObservedPickupSerial=Effects->GetPickupNotificationSerial();
        LastPickupType=Effects->GetLastCollectedType(); PickupPresentedAt=Now;
    }
}
void AONEHUD::DrawHealthEdges(float W,float H)
{
    const float Severity=FMath::Clamp(HealthEdgeSeverity,0.f,1.f),Pulse=DamagePulseStrength;
    if (Severity<=KINDA_SMALL_NUMBER && Pulse<=KINDA_SMALL_NUMBER) return;
    const float Depth=FMath::Min(FMath::Min(W,H)*.14f,(18.f+90.f*Severity+18.f*Pulse)*K);
    const float Alpha=FMath::Clamp(.30f*FMath::Pow(Severity,1.25f)+.14f*Pulse,0.f,.42f);
    constexpr int32 Bands=20;
    for (int32 I=0;I<Bands;++I)
    {
        const float T=(I+.5f)/Bands,Inset=Depth*I/Bands,Thickness=Depth/Bands+.25f;
        const FLinearColor C(.80f-.28f*Severity,.025f-.017f*Severity,.010f,Alpha*FMath::Square(1.f-T));
        DrawRect(C,0,Inset,W,Thickness); DrawRect(C,0,H-Inset-Thickness,W,Thickness);
        DrawRect(C,Inset,Depth,Thickness,FMath::Max(0.f,H-2.f*Depth));
        DrawRect(C,W-Inset-Thickness,Depth,Thickness,FMath::Max(0.f,H-2.f*Depth));
    }
}
void AONEHUD::PowerUpIcon(EONEPowerUpType Type,float X,float Y,float Size,FLinearColor Color)
{
    const float U=Size/32.f; FLinearColor Cut=Ink; Cut.A=Color.A;
    if (Type==EONEPowerUpType::InstaKill)
    {
        Rounded(X+3*U,Y,26*U,24*U,Color,8*U/K);
        Rounded(X+8*U,Y+20*U,16*U,10*U,Color,2*U/K);
        Rounded(X+7*U,Y+8*U,7*U,7*U,Cut,2*U/K); Rounded(X+18*U,Y+8*U,7*U,7*U,Cut,2*U/K);
        DrawRect(Cut,X+15*U,Y+17*U,3*U,5*U);
        for (int32 I=0;I<3;++I) DrawRect(Cut,X+(11+I*4)*U,Y+25*U,2*U,5*U);
    }
    else if (Type==EONEPowerUpType::DoublePoints)
    {
        DrawLine(X+2*U,Y+11*U,X+12*U,Y+23*U,Color,3*U); DrawLine(X+2*U,Y+23*U,X+12*U,Y+11*U,Color,3*U);
        Text(TEXT("2"),X+14*U,Y,32*U,Color);
    }
    else
    {
        Rounded(X,Y+6*U,32*U,24*U,Color,3*U/K);
        DrawRect(Cut,X+2*U,Y+10*U,28*U,4*U); DrawRect(Cut,X+5*U,Y+6*U,3*U,24*U); DrawRect(Cut,X+24*U,Y+6*U,3*U,24*U);
        for (int32 I=0;I<3;++I) DrawRect(Cut,X+(11+I*4)*U,Y+18*U,2*U,8*U);
    }
}
void AONEHUD::DrawPowerUps(const AONEGameMode* GM,float Left,float Top,float W)
{
    VisiblePowerUpTimers=0; if (GM->IsGameOver()) return;
    const auto* Effects=GM->GetPowerUps(); if (!Effects) return;
    for (const auto Type:{EONEPowerUpType::InstaKill,EONEPowerUpType::DoublePoints})
    {
        const float Remaining=Effects->GetRemainingSeconds(Type); if (Remaining<=0.f) continue;
        const float Y=Top+(104+VisiblePowerUpTimers*49)*K;
        const FLinearColor Color=Type==EONEPowerUpType::InstaKill?Red:Gold;
        Rounded(Left,Y,186*K,43*K,Ink,10);
        RecordPanel(Type==EONEPowerUpType::InstaKill?TEXT("InstaKillTimer"):TEXT("DoublePointsTimer"),Left,Y,186*K,43*K);
        PowerUpIcon(Type,Left+9*K,Y+7*K,28*K,Color);
        Text(Type==EONEPowerUpType::InstaKill?TEXT("INSTA-KILL"):TEXT("DOUBLE POINTS"),Left+46*K,Y+6*K,12*K,Color);
        Text(FString::Printf(TEXT("%ds"),FMath::CeilToInt(Remaining)),Left+46*K,Y+21*K,18*K,Remaining<=5.f?Gold:Paper);
        ++VisiblePowerUpTimers;
    }
    const float Age=float(GetWorld()->GetTimeSeconds()-PickupPresentedAt);
    if (LastPickupType!=EONEPowerUpType::Count && Age>=0.f && Age<2.3f)
    {
        const FLinearColor Color=LastPickupType==EONEPowerUpType::InstaKill?Red:LastPickupType==EONEPowerUpType::DoublePoints?Gold:Teal;
        const FString Label=LastPickupType==EONEPowerUpType::InstaKill?
            FString::Printf(TEXT("INSTA-KILL - %ds"),FMath::CeilToInt(Effects->GetRemainingSeconds(LastPickupType))):
            LastPickupType==EONEPowerUpType::DoublePoints?
            FString::Printf(TEXT("DOUBLE POINTS - %ds"),FMath::CeilToInt(Effects->GetRemainingSeconds(LastPickupType))):TEXT("MAX AMMO");
        const float X=W*.5f-179*K,Y=Top+258*K;
        FLinearColor Fill=Ink; Fill.A*=FMath::Clamp((2.3f-Age)/.25f,0.f,1.f);
        Rounded(X,Y,358*K,48*K,Fill,12); RecordPanel(TEXT("PickupNotice"),X,Y,358*K,48*K);
        PowerUpIcon(LastPickupType,X+13*K,Y+10*K,28*K,Color); Text(Label,X+56*K,Y+14*K,21*K,Color);
    }
}
void AONEHUD::DrawUpgradeStatus(const AONEPlayer* Player,float W,float Top)
{
    if (UIContext()>=2) return;
    for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It)
    {
        if (It->IsBox()) continue;
        const bool Lost=It->WasLastLossFor(Player) && It->GetTimeSinceWeaponLost()<=4.f;
        const bool Owned=It->IsOwnedBy(Player);
        const auto State=It->GetState();
        if (!Lost && !(Owned && (State==EONEMachineState::Active || State==EONEMachineState::Handoff || State==EONEMachineState::Ready || State==EONEMachineState::Collecting))) continue;
        const float X=W*.5f-225*K,Y=Top+166*K;
        const bool Ready=State==EONEMachineState::Ready && !Lost,Warn=Ready && It->IsExpiryWarning();
        Rounded(X,Y,450*K,76*K,Ink,13); RecordPanel(TEXT("UpgradeStatus"),X,Y,450*K,76*K);
        if (Lost)
        {
            CenterText(TEXT("WEAPON LOST"),W*.5f,Y+10*K,23*K,Red);
            CenterText(FamilyName(It->GetLastLostFamily())+TEXT(" expired - no refund"),W*.5f,Y+43*K,18*K,Paper);
        }
        else if (Ready)
        {
            CenterText(FString::Printf(TEXT("%s %ds"),Warn?TEXT("WEAPON EXPIRES IN"):TEXT("UPGRADE READY -"),FMath::CeilToInt(It->GetReadySecondsRemaining())),W*.5f,Y+10*K,23*K,Warn?Red:Gold);
            CenterText(TEXT("Approach to receive - no F"),W*.5f,Y+43*K,18*K,Paper);
        }
        else if (State==EONEMachineState::Collecting)
        {
            CenterText(TEXT("UPGRADE RETURNED"),W*.5f,Y+10*K,23*K,Teal);
            CenterText(TEXT("Restored to its reserved slot"),W*.5f,Y+43*K,18*K,Paper);
        }
        else
        {
            CenterText(FString::Printf(TEXT("PACK-A-PUNCH - %ds"),FMath::CeilToInt(FMath::Max(0.f,It->ProcessingDuration-It->GetStateElapsed()))),W*.5f,Y+10*K,23*K,Teal);
            CenterText(FString::Printf(TEXT("Return nearby - ready lasts %ds"),FMath::CeilToInt(It->ReadyLifetime)),W*.5f,Y+43*K,18*K,Paper);
        }
        break; // Bounded compact banner; the focused offer remains independent.
    }
}
bool AONEHUD::DrawBoxReel(const AONEPlayer* Player,float W,float Top)
{
    AONEProgressionMachine* Box=nullptr;
    for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It)
        if (It->IsBox() && It->IsOwnedBy(Player) && (It->GetState()==EONEMachineState::Active || It->GetState()==EONEMachineState::Ready)) { Box=*It; break; }
    if (!Box)
    { ReelMachine.Reset(); ReelFamily=PreviousReelFamily=EONEWeaponFamily::Invalid; ReelCycle=INDEX_NONE; ReelReceipt=0; return false; }
    const auto* Visual=Box->GetPresentation(); if (!Visual) return false;
    const bool Ready=Box->GetState()==EONEMachineState::Ready;
    const auto Family=Ready ? Box->GetRewardFamily() : Visual->GetPreviewFamily();
    const int32 Cycle=Visual->GetCycleCueCount(); const float Now=GetWorld()->GetTimeSeconds();
    if (ReelMachine.Get()!=Box || ReelReceipt!=Box->GetPaymentReceipt())
    { PreviousReelFamily=EONEWeaponFamily::Invalid; ReelFamily=Family; ReelChangedAt=Now; }
    else if (Family!=ReelFamily || Cycle!=ReelCycle)
    { PreviousReelFamily=ReelFamily; ReelFamily=Family; ReelChangedAt=Now; }
    ReelMachine=Box; ReelReceipt=Box->GetPaymentReceipt(); ReelCycle=Cycle;
    const float X=W*.5f-192*K,Height=(Ready?108.f:140.f)*K;
    Rounded(X,Top,384*K,Height,Ink,16); RecordPanel(TEXT("BoxReel"),X,Top,384*K,Height);
    CenterText(Ready?TEXT("BOX REWARD"):TEXT("BLACKSITE BOX"),W*.5f,Top+10*K,17*K,Ready?Gold:Muted);
    const float Blend=Ready ? 1.f : FMath::Clamp((Now-ReelChangedAt)/.075f,0.f,1.f);
    if (Ready)
    {
        Icon(ReelFamily,false,X+12*K,Top+30*K,145*K,48*K);
        Text(FamilyName(Family),X+165*K,Top+43*K,21*K,Gold);
        FString Reminder=TEXT("Return to the box to collect");
        if (Box->CanReach(Player))
        {
            Reminder=TEXT("Hold F at the box to collect");
            if (const auto* Interaction=Player->GetInteractionComponent();Interaction &&
                Interaction->GetOffer().Machine.Get()==Box && !Interaction->GetOffer().bEnabled)
                Reminder=Player->GetWeaponComponent()->IsMagazineReloadCommitted()?TEXT("Finish reloading to collect"):TEXT("Reward waiting - see nearby prompt");
        }
        CenterText(Reminder,W*.5f,Top+86*K,15*K,Muted);
    }
    else
    {
        if (Blend<1.f) Icon(PreviousReelFamily,false,X+52*K,Top+30*K-12*K*Blend,280*K,62*K,1.f-Blend);
        Icon(ReelFamily,false,X+52*K,Top+30*K+12*K*(1.f-Blend),280*K,62*K,Blend);
        CenterText(FamilyName(Family),W*.5f,Top+88*K,23*K,Paper);
        Rounded(X+28*K,Top+123*K,328*K,4*K,FLinearColor(.14f,.20f,.22f),2);
        Rounded(X+28*K,Top+123*K,328*K*FMath::Clamp(Box->GetStateElapsed()/FMath::Max(.1f,Box->RollDuration),0.f,1.f),4*K,Teal,2);
    }
    return true;
}
void AONEHUD::DrawTools(const AONEGameMode* GM,float W,float H)
{
    DrawRect(FLinearColor(.007f,.013f,.018f,.62f),0,0,W,H);
    const float X=W*.5f-365*K,Y=H*.5f-388*K;
    Rounded(X,Y,730*K,776*K,Ink,20);
    Text(TEXT("HELP + SANDBOX"),X+28*K,Y+24*K,32*K,Paper);
    Text(TEXT("WASD move   Shift run   Mouse aim   LMB fire"),X+28*K,Y+72*K,19*K,Muted);
    Text(TEXT("R reload   1 / 2 slots   Tab / wheel switch"),X+28*K,Y+98*K,19*K,Muted);
    Text(TEXT("Hold RMB head / Left Ctrl low   Esc pause"),X+28*K,Y+124*K,19*K,Muted);
    Text(TEXT("Box: hold F   Upgrade: tap F / auto-return"),X+28*K,Y+150*K,19*K,Muted);
    Button(EONEUIAction::ToggleSandbox,GM->IsSandbox()?TEXT("Return to rounds"):TEXT("Enter sandbox"),TEXT("F1"),X+28*K,Y+188*K,425*K,42*K);
    const auto* Shake=IConsoleManager::Get().FindConsoleVariable(TEXT("one.CameraShake.Strength"));
    const int32 ShakePercent=FMath::RoundToInt(FMath::Clamp(Shake?Shake->GetFloat():1.f,0.f,1.f)*100.f);
    Button(EONEUIAction::CycleShakeStrength,ShakePercent>0?FString::Printf(TEXT("Shake %d%%"),ShakePercent):TEXT("Shake Off"),TEXT(""),X+468*K,Y+188*K,234*K,42*K);
    const bool Enabled=GM->IsSandbox();
    Text(TEXT("ENEMIES"),X+28*K,Y+248*K,19*K,Teal);
    Button(EONEUIAction::SpawnOne,TEXT("Spawn one"),TEXT("F2"),X+28*K,Y+276*K,329*K,42*K,false,Enabled);
    Button(EONEUIAction::SpawnSix,TEXT("Spawn six"),TEXT("F3"),X+373*K,Y+276*K,329*K,42*K,false,Enabled);
    Text(TEXT("WEAPONS + POINTS"),X+28*K,Y+332*K,19*K,Teal);
    Button(EONEUIAction::Refill,TEXT("Refill ammo"),TEXT("F4"),X+28*K,Y+360*K,329*K,42*K,false,Enabled);
    Button(EONEUIAction::GrantPoints,TEXT("Add 10000 Points"),TEXT("T"),X+373*K,Y+360*K,329*K,42*K,false,Enabled);
    const TCHAR* Labels[4]={TEXT("M1911"),TEXT("M4A1"),TEXT("870"),TEXT("Random")};
    const TCHAR* Keys[4]={TEXT("Z"),TEXT("X"),TEXT("C"),TEXT("V")};
    const EONEUIAction Actions[4]={EONEUIAction::ForcePistol,EONEUIAction::ForceCarbine,EONEUIAction::ForceShotgun,EONEUIAction::RandomBox};
    for (int32 I=0;I<4;++I) Button(Actions[I],Labels[I],Keys[I],X+(28+I*173)*K,Y+414*K,155*K,40*K,false,Enabled);
    Text(TEXT("Next box: ")+GM->GetForcedBoxRewardLabel(),X+28*K,Y+464*K,18*K,Gold);
    Text(TEXT("SCENE + RESET"),X+28*K,Y+494*K,19*K,Teal);
    Button(EONEUIAction::ToggleLighting,GM->IsSandboxDimLighting()?TEXT("Bright"):TEXT("Dim"),TEXT("F7"),X+28*K,Y+522*K,213*K,42*K,false,Enabled);
    Button(EONEUIAction::ClearGore,TEXT("Clean up"),TEXT("F6"),X+258*K,Y+522*K,213*K,42*K,false,Enabled);
    Button(EONEUIAction::ResetSandbox,TEXT("Reset"),TEXT("F5"),X+489*K,Y+522*K,213*K,42*K,false,Enabled);
    Text(TEXT("FORCED PICKUPS - SANDBOX ONLY"),X+28*K,Y+582*K,19*K,Teal);
    Button(EONEUIAction::ForceInstaKill,TEXT("Drop Insta-Kill"),TEXT(""),X+28*K,Y+610*K,213*K,42*K,false,Enabled);
    Button(EONEUIAction::ForceDoublePoints,TEXT("Drop Double Points"),TEXT(""),X+258*K,Y+610*K,213*K,42*K,false,Enabled);
    Button(EONEUIAction::ForceMaxAmmo,TEXT("Drop Max Ammo"),TEXT(""),X+489*K,Y+610*K,213*K,42*K,false,Enabled);
    Text(TEXT("Forced drops do not consume normal death rolls."),X+28*K,Y+664*K,17*K,Muted);
    Button(EONEUIAction::CloseTools,TEXT("Back to game"),TEXT("H"),X+28*K,Y+710*K,674*K,44*K,true);
}
void AONEHUD::DrawMenu(const AONEGameMode* GM,float W,float H)
{
    const bool Dead=GM->IsGameOver();
    const float Settle=Dead?FMath::SmoothStep(0.f,1.f,DeathPresentationProgress):1.f;
    DrawRect(FLinearColor(.007f,.013f,.020f,.79f*(.65f+.35f*Settle)),0,0,W,H);
    const float X=W*.5f-278*K,Y=H*.5f-278*K;
    FLinearColor MenuInk=Ink; MenuInk.A*=.78f+.22f*Settle;
    Rounded(X,Y,556*K,556*K,MenuInk,24);
    FLinearColor Heading=Dead?Gold:Paper; Heading.A=.55f+.45f*Settle;
    CenterText(Dead?TEXT("GAME OVER"):TEXT("PAUSED"),W*.5f,Y+(30-8*(1.f-Settle))*K,51*K,Heading);
    CenterText(Dead?TEXT("Response lost"):TEXT("Containment on hold"),W*.5f,Y+88*K,21*K,Muted);
    Text(TEXT("ROUND"),X+40*K,Y+142*K,21*K,Teal);
    const float RoundFont=FMath::Min(96.f,171.f/(FString::FromInt(FMath::Max(0,GM->GetRound())).Len()*.66f))*K;
    Number(GM->GetRound(),X+30*K,Y+173*K,RoundFont,Paper);
    Text(FString::Printf(TEXT("%d kills"),GM->GetKills()),X+237*K,Y+161*K,28*K,Paper);
    Text(FString::Printf(TEXT("%d Points"),GM->GetPoints()),X+237*K,Y+208*K,28*K,Gold);
    const int32 Seconds=FMath::Max(0,FMath::FloorToInt(GM->GetSurvivalSeconds()));
    Text(FString::Printf(TEXT("Survived %dm %ds"),Seconds/60,Seconds%60),X+40*K,Y+286*K,23*K,Muted);
    float BY=Y+340*K;
    if (!Dead) { Button(EONEUIAction::Resume,TEXT("Resume"),TEXT("Esc"),X+36*K,BY,484*K,52*K,true); BY+=66*K; }
    Button(EONEUIAction::Restart,Dead?TEXT("Try again"):TEXT("Restart"),TEXT("Enter"),X+36*K,BY,484*K,52*K,Dead); BY+=66*K;
    Button(EONEUIAction::Quit,TEXT("Quit"),TEXT("Q"),X+36*K,BY,484*K,44*K);
}
void AONEHUD::DrawHUD()
{
    Super::DrawHUD(); Buttons.Reset(); LayoutBounds.Reset();
    if (!Canvas || !PlayerOwner) return;
    const auto* P=Cast<AONEPlayer>(PlayerOwner->GetPawn()); const auto* GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!P || !GM) return;
    if (GM->IsGameOver())
    {
        if (DeathPresentedAt<0.) DeathPresentedAt=GetWorld()->GetRealTimeSeconds();
        DeathPresentationProgress=FMath::Clamp(float((GetWorld()->GetRealTimeSeconds()-DeathPresentedAt)/.25),0.f,1.f);
    }
    else { DeathPresentedAt=-1.; DeathPresentationProgress=1.f; }
    const float W=Canvas->SizeX,H=Canvas->SizeY;
    K=FMath::Min(W/1600.f,H/900.f);
    const float SafeWidth=FMath::Min(W,1600*K),Left=(W-SafeWidth)*.5f+28*K,Right=W-Left,Bottom=H-28*K,Top=24*K;
    UpdateSurvivalPresentation(P,GM); DrawHealthEdges(W,H);
    Rounded(Left,Top,186*K,91*K,Ink,16); RecordPanel(TEXT("Round"),Left,Top,186*K,91*K);
    Text(TEXT("ROUND"),Left+18*K,Top+12*K,18*K,Muted);
    const int32 RoundValue=FMath::Max(0,GM->GetRound());
    const float RoundHeight=FMath::Min(57.f,69.f/(FString::FromInt(RoundValue).Len()*.66f))*K;
    Number(RoundValue,Left+9*K,Top+30*K+(57*K-RoundHeight)*.5f,RoundHeight,Paper);
    Text(FString::Printf(TEXT("%d left"),GM->GetRemaining()),Left+87*K,Top+54*K,19*K,Muted);
    Keycap(TEXT("H"),Right-142*K,Top,30);
    Text(GM->IsSandbox()?TEXT("SANDBOX"):TEXT("HELP"),Right-103*K,Top+4*K,20*K,GM->IsSandbox()?Teal:Muted);
    RecordPanel(TEXT("Help"),Right-142*K,Top,142*K,28*K);

    const FString Points=FString::FromInt(GM->GetPoints());
    const float PointHeight=FMath::Min(43.f,190.f/FMath::Max(1,Points.Len())/.66f)*K;
    const float PointWidth=Number(GM->GetPoints(),Left-5*K,Bottom-123*K,PointHeight,Gold);
    Text(TEXT("Points"),Left+PointWidth+7*K,Bottom-108*K,22*K,Gold);
    RecordPanel(TEXT("Points"),Left-5*K,Bottom-123*K,PointWidth+100*K,43*K);
    if (DisplayedCombatGain>0 && !IsPointerUIActive())
    {
        const float Age=float(GetWorld()->GetTimeSeconds()-GainUpdatedAt);
        FLinearColor GainColor=Gold; GainColor.A=FMath::Clamp((1.15f-Age)/.35f,0.f,1.f);
        Text(FString::Printf(TEXT("+%d"),DisplayedCombatGain),Left+1*K,Bottom-(154+FMath::Min(Age,.5f)*7)*K,22*K,GainColor);
    }
    Rounded(Left,Bottom-65*K,241*K,65*K,Ink,13); RecordPanel(TEXT("Health"),Left,Bottom-65*K,241*K,65*K);
    const float Health=FMath::Clamp(P->GetHealth()/FMath::Max(1.f,P->GetMaxHealth()),0.f,1.f);
    const FLinearColor HealthColor=Health>.3f?Teal:Red;
    DrawRect(HealthColor,Left+17*K,Bottom-43*K,18*K,6*K); DrawRect(HealthColor,Left+23*K,Bottom-49*K,6*K,18*K);
    Number(FMath::CeilToInt(P->GetHealth()),Left+41*K,Bottom-59*K,39*K,Paper);
    Rounded(Left+16*K,Bottom-15*K,209*K,5*K,FLinearColor(.14f,.21f,.22f),2);
    Rounded(Left+16*K,Bottom-15*K,209*K*Health,5*K,HealthColor,2);

    if (const auto* Weapon=P->GetWeaponComponent())
    {
        const float X=Right-350*K,Y=Bottom-178*K;
        const bool Armed=Weapon->HasUsableWeapon(); const auto& Def=Weapon->GetDefinition();
        const FLinearColor WC=Armed && Def.bUpgraded ? Def.AuraColor : Paper;
        Rounded(X,Y,350*K,178*K,Ink,16); RecordPanel(TEXT("AmmoWeapon"),X,Y,350*K,178*K);
        if (Armed) Icon(Def.Family,Def.bUpgraded,X+135*K,Y+4*K,202*K,77*K);
        if (Armed) Number(Weapon->GetAmmo(),X+9*K,Y+9*K,65*K,Weapon->GetAmmo()>0?Paper:Gold);
        if (Armed) Text(FString::Printf(TEXT("/ %d"),Weapon->GetReserveAmmo()),X+20*K,Y+79*K,22*K,Muted);
        else Text(TEXT("UNARMED"),X+17*K,Y+39*K,32*K,Gold);
        Text(Armed?Def.DisplayName.ToString():TEXT("Acquire or collect a weapon"),X+17*K,Y+106*K,22*K,WC);
        if (Weapon->IsReloading())
        {
            Text(TEXT("RELOADING"),X+17*K,Y+132*K,13*K,Gold);
            Rounded(X+137*K,Y+138*K,194*K,3*K,FLinearColor(.14f,.21f,.22f),2);
            Rounded(X+137*K,Y+138*K,194*K*Weapon->GetOperationProgress(),3*K,Gold,2);
        }
        else if (Armed && Weapon->GetAmmo()==0) Text(Weapon->GetReserveAmmo()==0?TEXT("NO RESERVE"):TEXT("RELOAD"),X+17*K,Y+132*K,13*K,Gold);
        const int32 SlotCount=FMath::Max(1,Weapon->GetWeaponCount());
        const float SlotPitch=336.f/SlotCount;
        for (int32 Slot=0;Slot<SlotCount;++Slot)
        {
            const auto* State=Weapon->GetSlotState(Slot); const auto* D=Weapon->GetDefinitionForWeapon(Slot);
            const float SX=X+(12+Slot*SlotPitch)*K,SY=Y+146*K; const bool Selected=Weapon->GetEquippedIndex()==Slot;
            Rounded(SX,SY,(SlotPitch-10)*K,25*K,Selected?FLinearColor(.13f,.26f,.26f):FLinearColor(.08f,.12f,.14f),6);
            Text(FString::FromInt(Slot+1),SX+4*K,SY+3*K,18*K,Selected?Teal:Muted);
            FString Name=D ? D->DisplayName.ToString() : TEXT("Empty"); FLinearColor Color=Selected?Paper:Muted;
            if (State && State->Status==EONEWeaponSlotStatus::MachineReserved) { Name=TEXT("Upgrading"); Color=Gold; }
            else if (State && State->Status==EONEWeaponSlotStatus::ReadyToCollect) { Name=TEXT("Return nearby"); Color=Gold; }
            Text(Name,SX+25*K,SY+4*K,FMath::Min(16.f,(SlotPitch-42.f)/FMath::Max(1,Name.Len())/.57f)*K,Color);
        }
        if (!IsPointerUIActive())
        {
            float MX=0,MY=0;
            if (PlayerOwner->GetMousePosition(MX,MY))
            {
                for (int32 S:{-1,1})
                { DrawLine(MX+S*5*K,MY,MX+S*11*K,MY,Paper,1.4f*K); DrawLine(MX,MY+S*5*K,MX,MY+S*11*K,Paper,1.4f*K); }
                DrawRect(Paper,MX-K,MY-K,2*K,2*K);
                const FString AimLabel=FString::Printf(TEXT("%s %d cm"),*P->GetAimHeightLabel(),FMath::RoundToInt(P->GetAimHeightCm()));
                const float AimX=FMath::Clamp(MX+17*K,4*K,FMath::Max(4*K,W-145*K));
                const float AimY=FMath::Clamp(MY+17*K,4*K,FMath::Max(4*K,H-22*K));
                Text(AimLabel,AimX,AimY,14*K,P->IsAdjustingAimHeight()?Gold:Muted);
                const bool Kill=Weapon->WasLastHitKill(); const float Duration=Kill?.42f:.30f,Age=Weapon->GetTimeSinceHit();
                if (Age<Duration)
                {
                    FLinearColor Hit=Kill?Gold:Teal; Hit.A=FMath::Clamp((Duration-Age)/.13f,0.f,1.f);
                    const float Rise=Kill?.06f:.04f,Peak=Kill?1.45f:1.25f;
                    const float Pop=Age<Rise?FMath::Lerp(.75f,Peak,FMath::Clamp(Age/Rise,0.f,1.f)):
                        1.f+(Peak-1.f)*FMath::Exp(-(Age-Rise)/(Kill?.10f:.065f));
                    const float Inner=(Kill?11.f:7.f)*K*Pop,Outer=(Kill?21.f:14.f)*K*Pop;
                    for (int32 S:{-1,1}) for (int32 T:{-1,1}) DrawLine(MX+S*Inner,MY+T*Inner,MX+S*Outer,MY+T*Outer,Hit,(Kill?3.f:2.f)*K);
                    if (Kill) for (int32 S:{-1,1}) for (int32 T:{-1,1}) DrawLine(MX+S*7*K*Pop,MY,MX,MY+T*7*K*Pop,Hit,2*K);
                }
            }
        }
    }
    const bool Overlay=IsPointerUIActive(); bool Reel=false;
    DrawPowerUps(GM,Left,Top,W);
    if (!GM->IsGameOver()) DrawUpgradeStatus(P,W,Top);
    if (!GM->IsGameOver()) Reel=DrawBoxReel(P,W,Top); else { ReelMachine.Reset(); ReelFamily=EONEWeaponFamily::Invalid; }
    if (!Overlay)
    {
        if (const auto* I=P->GetInteractionComponent();I && I->GetOffer().Machine.IsValid())
        {
            const auto& Offer=I->GetOffer(); const float CardWidth=500*K,Font=19*K;
            const TArray<FString> Lines=Wrap(Offer.Detail,CardWidth-38*K,Font);
            const float CH=(85+Lines.Num()*23)*K,X=W*.5f-CardWidth*.5f,Y=Bottom-CH;
            Rounded(X,Y,CardWidth,CH,Ink,14); RecordPanel(TEXT("Context"),X,Y,CardWidth,CH);
            Text(Offer.Title,X+17*K,Y+12*K,23*K,Offer.bEnabled?Teal:Muted);
            for (int32 Row=0;Row<Lines.Num();++Row) Text(Lines[Row],X+17*K,Y+(44+Row*23)*K,Font,Paper);
            if (Offer.Input==EONEInteractionInput::Automatic)
            {
                Text(TEXT("Approach to receive - no F"),X+17*K,Y+CH-30*K,18*K,Offer.bExpiryWarning?Red:Gold);
                Text(FString::Printf(TEXT("%ds"),FMath::CeilToInt(Offer.ReadySecondsRemaining)),X+414*K,Y+CH-32*K,21*K,Offer.bExpiryWarning?Red:Gold);
            }
            else if (Offer.Input==EONEInteractionInput::Hold || Offer.Input==EONEInteractionInput::Tap)
            {
                Keycap(TEXT("F"),X+17*K,Y+CH-34*K,28);
                Text(I->RequiresRelease()?TEXT("Release F"):Offer.bEnabled?
                    (Offer.Input==EONEInteractionInput::Tap?TEXT("Tap to deposit"):TEXT("Hold to interact")):TEXT("Unavailable"),
                    X+54*K,Y+CH-30*K,18*K,Offer.bEnabled?Gold:Muted);
                if (Offer.Input==EONEInteractionInput::Hold)
                {
                    Rounded(X+267*K,Y+CH-22*K,214*K,4*K,FLinearColor(.14f,.20f,.22f),2);
                    Rounded(X+267*K,Y+CH-22*K,214*K*FMath::Clamp(I->GetProgress(),0.f,1.f),4*K,Gold,2);
                }
            }
        }
        if (!Reel && GM->IsIntermission())
        {
            Rounded(W*.5f-187*K,Top,374*K,62*K,Ink,14); RecordPanel(TEXT("RoundNotice"),W*.5f-187*K,Top,374*K,62*K);
            CenterText(GM->GetRound()==0?TEXT("CONTAINMENT BREACHED"):TEXT("ROUND COMPLETE"),W*.5f,Top+10*K,21*K,Teal);
            CenterText(FString::Printf(TEXT("Next wave in %d"),FMath::Max(0,FMath::CeilToInt(GM->GetCountdown()))),W*.5f,Top+36*K,18*K,Muted);
        }
        const float DamageAge=P->GetDamageReactionAge();
        if (DamageAge<.45f)
        {
            FLinearColor C=Red; C.A=.65f*FMath::Clamp((.45f-DamageAge)/.25f,0.f,1.f);
            FVector2D Center,From;
            if (PlayerOwner->ProjectWorldLocationToScreen(P->GetActorLocation(),Center) && PlayerOwner->ProjectWorldLocationToScreen(P->GetActorLocation()-P->GetDamageReactionDirection()*100,From))
            {
                const FVector2D Direction=(From-Center).GetSafeNormal(),Tangent(-Direction.Y,Direction.X),Tip=Center+Direction*76*K;
                DrawLine(Tip.X,Tip.Y,Tip.X-Direction.X*10*K+Tangent.X*11*K,Tip.Y-Direction.Y*10*K+Tangent.Y*11*K,C,3*K);
                DrawLine(Tip.X,Tip.Y,Tip.X-Direction.X*10*K-Tangent.X*11*K,Tip.Y-Direction.Y*10*K-Tangent.Y*11*K,C,3*K);
            }
        }
    }
    FString Segment;
    for (TActorIterator<AONE03MovementCheck> It(GetWorld());It;++It) { Segment=It->GetSegmentLabel(); break; }
    for (TActorIterator<AONE03PresentationCheck> It(GetWorld());It;++It) { Segment=It->GetSegmentLabel(); break; }
    for (TActorIterator<AONE03PhysicalityCheck> It(GetWorld());It;++It) { Segment=It->GetSegmentLabel(); break; }
    for (TActorIterator<AONE04PresentationCheck> It(GetWorld());It;++It) { Segment=It->GetSegmentLabel(); break; }
    for (TActorIterator<AONE05PresentationCheck> It(GetWorld());It;++It) { Segment=It->GetSegmentLabel(); break; }
    for (TActorIterator<AONE05MotionCheck> It(GetWorld());It;++It) { Segment=It->GetSegmentLabel(); break; }
    if (!Segment.IsEmpty())
    {
        const float SH=FMath::Min(18.f,1050.f/FMath::Max(1,Segment.Len())/.57f)*K;
        Rounded(W*.5f-550*K,H*.72f,1100*K,30*K,Ink,7); CenterText(Segment,W*.5f,H*.72f+4*K,SH,Gold);
    }
    if (UIContext()>=2) DrawMenu(GM,W,H); else if (bTools) DrawTools(GM,W,H);
}
