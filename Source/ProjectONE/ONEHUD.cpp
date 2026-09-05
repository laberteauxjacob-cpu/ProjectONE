#include "ONEHUD.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
#include "ONEInteractionComponent.h"
#include "ONE03MovementCheck.h"
#include "ONE03PresentationCheck.h"
#include "ONE03PhysicalityCheck.h"
#include "ONE04PresentationCheck.h"
#include "EngineUtils.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AONEHUD::Label(const FString& Text, float X, float Y, float Scale, FLinearColor Color)
{
    const float ScreenScale=Canvas?FMath::Clamp(Canvas->SizeX/1600.f,.72f,1.3f):1.f;
    DrawText(Text, Color, X, Y, GEngine->GetMediumFont(), FMath::Max(Scale,.85f*ScreenScale), false);
}
void AONEHUD::Panel(float X, float Y, float W, float H)
{
    DrawRect(FLinearColor(.017f,.028f,.035f,.90f), X,Y,W,H);
    DrawRect(FLinearColor(.20f,.65f,.63f,.9f), X,Y,2,H);
}
void AONEHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner) return;
    const AONEPlayer* P = Cast<AONEPlayer>(PlayerOwner->GetPawn());
    const AONEGameMode* GM = GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!P || !GM) return;
    const float W = Canvas->SizeX, H = Canvas->SizeY;
    const float K = FMath::Clamp(W/1600.f, .72f, 1.3f);
    const float M = 28*K;
    const FLinearColor Teal(.34f,.86f,.79f), Dim(.47f,.59f,.61f), Amber(.98f,.64f,.24f);
    Panel(M,M,276*K,86*K);
    Label("PROJECT ONE", M+17*K,M+12*K,1.25f*K);
    Label("B-07 / CANDIDATE 04", M+17*K,M+45*K,.72f*K,Dim);
    Panel(W-M-236*K,M,236*K,86*K);
    Label(GM->IsSandbox()?TEXT("DEVELOPER SANDBOX"):FString::Printf(TEXT("ROUND  %02d"),FMath::Max(1,GM->GetRound())),W-M-218*K,M+12*K,1.10f*K,Amber);
    Label(FString::Printf(TEXT("%d REMAINING    /    %06d PTS"),GM->GetRemaining(),GM->GetPoints()),W-M-218*K,M+46*K,.65f*K);
    Panel(M,H-M-102*K,276*K,102*K);
    Label("RESPONSE / VITAL SIGNS",M+17*K,H-M-90*K,.68f*K,Dim);
    const float Health = FMath::Clamp(P->GetHealth()/FMath::Max(1.f,P->GetMaxHealth()),0.f,1.f);
    DrawRect(FLinearColor(.12f,.18f,.19f),M+17*K,H-M-57*K,188*K,7*K);
    DrawRect(Health>.3f?Teal:FLinearColor(.91f,.24f,.14f),M+17*K,H-M-57*K,188*K*Health,7*K);
    Label(FString::Printf(TEXT("%03d"),FMath::CeilToInt(P->GetHealth())),M+217*K,H-M-68*K,.85f*K);
    Label("WASD MOVE   SHIFT RUN",M+17*K,H-M-31*K,.62f*K,Dim);
    Label("ESC PAUSE   F1 SANDBOX",M+17*K,H-M-14*K,.62f*K,Dim);
    if (const UONEWeaponComponent* Weapon = P->GetWeaponComponent())
    {
        Panel(W-M-360*K,H-M-202*K,360*K,202*K);
        const float X = W-M-343*K, Y=H-M-202*K;
        const bool Armed=Weapon->HasUsableWeapon();
        const FLinearColor WeaponColor=Weapon->GetDefinition().bUpgraded ? Weapon->GetDefinition().AuraColor : Teal;
        Label(Armed?Weapon->GetWeaponName().ToString():TEXT("UNARMED"),X,Y+12*K,.86f*K,WeaponColor);
        Label(Armed?FString::Printf(TEXT("%02d"),Weapon->GetAmmo()):TEXT("--"),X,Y+36*K,1.65f*K,Weapon->GetAmmo()?FLinearColor::White:Amber);
        Label(Armed?FString::Printf(TEXT("/ %03d"),Weapon->GetReserveAmmo()):TEXT("/ --"),X+67*K,Y+53*K,.9f*K,Dim);
        FString Operation=TEXT("READY");
        const auto* Equipped=Weapon->GetSlotState(Weapon->GetEquippedIndex());
        if (P->IsInMachineAction()) Operation=TEXT("WEAPON HANDOFF");
        else if (!Armed) Operation=TEXT("COLLECT OR ACQUIRE A WEAPON");
        else if (Weapon->IsReloading()) Operation=TEXT("RELOADING / SHIFT TO CANCEL");
        else if (Equipped && !Equipped->bMagazinePresent) Operation=TEXT("MAGAZINE REMOVED / R TO RELOAD");
        else if (Weapon->GetOperation()==EONEWeaponOperation::Equip) Operation=TEXT("EQUIPPING");
        else if (Weapon->NeedsPump(Weapon->GetEquippedIndex())) Operation=TEXT("PUMP ACTION");
        else if (Weapon->GetAmmo()==0) Operation=Weapon->GetReserveAmmo()==0?TEXT("EMPTY / NO RESERVE"):P->IsSprintRequested()?TEXT("EMPTY / RELEASE SHIFT TO RELOAD"):TEXT("EMPTY / AUTO RELOAD");
        Label(Operation,X,Y+81*K,.74f*K,Weapon->IsBusy()||Weapon->GetAmmo()==0?Amber:Dim);
        if (Weapon->IsBusy()) DrawRect(Amber,X,Y+104*K,326*K*Weapon->GetOperationProgress(),2*K);
        for (int32 Slot=0;Slot<2;++Slot)
        {
            const bool Selected=Weapon->GetEquippedIndex()==Slot;
            const auto* State=Weapon->GetSlotState(Slot); const auto* D=Weapon->GetDefinitionForWeapon(Slot);
            FString Text=FString::Printf(TEXT("%s %d  EMPTY"),Selected?TEXT(">"):TEXT(" "),Slot+1);
            FLinearColor Color=Selected?Teal:Dim;
            if (State && D)
            {
                const TCHAR* Status=State->Status==EONEWeaponSlotStatus::MachineReserved ? TEXT("UPGRADING") :
                    State->Status==EONEWeaponSlotStatus::ReadyToCollect ? TEXT("READY TO COLLECT") : nullptr;
                const FString Supply=Status ? Status : FString::Printf(TEXT("%02d / %03d"),State->Ammo,State->Reserve);
                Text=FString::Printf(TEXT("%s %d  %s   %s"),Selected?TEXT(">"):TEXT(" "),Slot+1,*D->DisplayName.ToString(),*Supply);
                if (Status) Color=Amber; else if (D->bUpgraded) Color=D->AuraColor;
            }
            Label(Text,X,Y+(113+23*Slot)*K,.78f*K,Color);
        }
        Label("TAB / WHEEL SWITCH    R RELOAD",X,Y+162*K,.73f*K,Dim);
        Label("LMB FIRE    SHIFT CANCEL + SPRINT",X,Y+183*K,.73f*K,Dim);
        if (Weapon->GetTimeSinceHit()<.18f)
        {
            float MX,MY;
            if (PlayerOwner->GetMousePosition(MX,MY))
            {
                const FLinearColor C=Weapon->WasLastHitKill()?Amber:Teal;
                for (int32 S=-1;S<=1;S+=2) for (int32 T=-1;T<=1;T+=2)
                    DrawLine(MX+S*5,MY+T*5,MX+S*11,MY+T*11,C,1.5f);
            }
        }
    }
    if (const auto* Interaction=P->GetInteractionComponent(); Interaction && Interaction->GetOffer().Machine.IsValid() && !GM->IsGameOver())
    {
        const auto& Offer=Interaction->GetOffer();
        const float X=W*.5f-360*K,Y=H-M-126*K;
        Panel(X,Y,720*K,126*K);
        Label(Offer.Title,X+18*K,Y+10*K,.86f*K,Teal);
        TArray<FString> Words; Offer.Detail.ParseIntoArray(Words,TEXT(" "),true);
        FString Line; float Row=0;
        for (const FString& Word:Words)
        {
            const FString Next=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;
            float TextWidth,TextHeight; GetTextSize(Next,TextWidth,TextHeight,GEngine->GetMediumFont(),.85f*K);
            if (!Line.IsEmpty() && TextWidth>680*K)
            { Label(Line,X+18*K,Y+(39+22*Row)*K,.78f*K,Offer.bEnabled?FLinearColor::White:Dim); Line=Word; ++Row; }
            else Line=Next;
        }
        if (!Line.IsEmpty()) Label(Line,X+18*K,Y+(39+22*Row)*K,.78f*K,Offer.bEnabled?FLinearColor::White:Dim);
        Label(Interaction->RequiresRelease()?TEXT("RELEASE F BEFORE NEXT ACTION"):Offer.bEnabled?TEXT("HOLD F"):TEXT("WAITING"),X+18*K,Y+93*K,.78f*K,Offer.bEnabled?Amber:Dim);
        DrawRect(FLinearColor(.13f,.20f,.21f),X+248*K,Y+101*K,454*K,5*K);
        DrawRect(Amber,X+248*K,Y+101*K,454*K*FMath::Clamp(Interaction->GetProgress(),0.f,1.f),5*K);
    }
    if (GetWorld()->GetTimeSeconds()-P->LastDamageTime<.22f)
    {
        const FLinearColor Damage(.72f,.12f,.055f,.72f);
        DrawRect(Damage,0,0,W,3); DrawRect(Damage,0,H-3,W,3);
        DrawRect(Damage,0,0,3,H); DrawRect(Damage,W-3,0,3,H);
    }
    if (GM->IsSandbox())
    {
        for (int32 Distance : {0,200,500,1000})
        {
            FVector2D Screen,PlayerScreen;
            PlayerOwner->ProjectWorldLocationToScreen(P->GetActorLocation(),PlayerScreen);
            if (PlayerOwner->ProjectWorldLocationToScreen(FVector(-500+Distance,250,16),Screen) &&
                Screen.X>0 && Screen.X<W-35*K && Screen.Y>M+110*K && Screen.Y<H-20*K &&
                !(FMath::Abs(Screen.X-PlayerScreen.X)<70*K && FMath::Abs(Screen.Y-PlayerScreen.Y)<105*K) &&
                !(Screen.X<M+290*K && Screen.Y>H-M-118*K) &&
                !(Screen.X>W-M-380*K && Screen.Y>H-M-222*K))
                Label(FString::Printf(TEXT("%dm"),Distance/100),Screen.X,Screen.Y,.85f*K,Dim);
        }
        Panel(W*.5f-310*K,M,620*K,132*K);
        Label("SANDBOX  /  F1 RETURN TO ROUNDS",W*.5f-292*K,M+9*K,.76f*K,Teal);
        Label("F2 +1 INFECTED   F3 +6   F4 REFILL AVAILABLE",W*.5f-292*K,M+33*K,.7f*K);
        Label(FString::Printf(TEXT("F5 RESET   F6 CLEANUP   F7 LIGHTS: %s"),GM->IsSandboxDimLighting()?TEXT("DIM"):TEXT("BRIGHT")),W*.5f-292*K,M+56*K,.7f*K,Dim);
        Label("T +10,000 TEST PTS   Z PISTOL / X M4 / C 870 / V RANDOM",W*.5f-292*K,M+79*K,.7f*K,Dim);
        Label(FString::Printf(TEXT("NEXT BOX: %s    TEST POINTS GRANTED: %d"),*GM->GetForcedBoxRewardLabel(),GM->GetSandboxGrantedPoints()),W*.5f-292*K,M+103*K,.7f*K,Amber);
    }
    for (TActorIterator<AONE03MovementCheck> It(GetWorld());It;++It)
    {
        Panel(W*.5f-360*K,M+146*K,720*K,36*K);
        Label(It->GetSegmentLabel(),W*.5f-342*K,M+154*K,.82f*K,Amber);
        break;
    }
    for (TActorIterator<AONE03PresentationCheck> It(GetWorld());It;++It)
    {
        Panel(W*.5f-390*K,M+146*K,780*K,36*K);
        Label(It->GetSegmentLabel(),W*.5f-372*K,M+154*K,.82f*K,Amber);
        break;
    }
    for (TActorIterator<AONE03PhysicalityCheck> It(GetWorld());It;++It)
    {
        Panel(W*.5f-390*K,M+146*K,780*K,36*K);
        Label(It->GetSegmentLabel(),W*.5f-372*K,M+154*K,.82f*K,Amber);
        break;
    }
    for (TActorIterator<AONE04PresentationCheck> It(GetWorld());It;++It)
    {
        Panel(W*.5f-400*K,M+146*K,800*K,36*K);
        Label(It->GetSegmentLabel(),W*.5f-382*K,M+154*K,.82f*K,Amber);
        break;
    }
    if (GM->IsIntermission() && !GM->IsGameOver())
    {
        Panel(W*.5f-171*K,M,342*K,62*K);
        Label(GM->GetRound()==0?TEXT("CONTAINMENT BREACHED"):TEXT("SECTOR TEMPORARILY CLEAR"),W*.5f-151*K,M+9*K,.76f*K,Amber);
        Label(FString::Printf(TEXT("Next wave in %d seconds"),FMath::Max(0,FMath::CeilToInt(GM->GetCountdown()))),W*.5f-151*K,M+34*K,.68f*K);
    }
    if (GM->IsGameOver() || PlayerOwner->IsPaused())
    {
        DrawRect(FLinearColor(.008f,.015f,.020f,.78f),0,0,W,H);
        const float X = W*.5f-230*K, Y = H*.5f-126*K;
        Panel(X,Y,460*K,252*K);
        Label(GM->IsGameOver()?TEXT("RESPONSE LOST"):TEXT("SIMULATION PAUSED"),X+30*K,Y+30*K,1.2f*K,GM->IsGameOver()?Amber:Teal);
        Label(FString::Printf(TEXT("ROUND %02d   /   %d KILLS   /   %06d PTS"),GM->GetRound(),GM->GetKills(),GM->GetPoints()),X+30*K,Y+86*K,.75f*K);
        Label("ENTER   Restart containment",X+30*K,Y+137*K,.85f*K);
        Label(GM->IsGameOver()?TEXT("Q   Quit"):TEXT("ESC   Resume       Q   Quit"),X+30*K,Y+182*K,.75f*K,Dim);
    }
}
