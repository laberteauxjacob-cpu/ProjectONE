#include "ONEHUD.h"
#include "ONEGameMode.h"
#include "ONEPlayer.h"
#include "ONEWeaponComponent.h"
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
    Label("B-07 / CONTAINMENT HALL", M+17*K,M+45*K,.72f*K,Dim);
    Panel(W-M-236*K,M,236*K,86*K);
    Label(FString::Printf(TEXT("ROUND  %02d"),FMath::Max(1,GM->GetRound())),W-M-218*K,M+12*K,1.10f*K,Amber);
    Label(FString::Printf(TEXT("%d REMAINING    /    %06d PTS"),GM->GetRemaining(),GM->GetPoints()),W-M-218*K,M+46*K,.65f*K);
    Panel(M,H-M-102*K,276*K,102*K);
    Label("RESPONSE / VITAL SIGNS",M+17*K,H-M-90*K,.68f*K,Dim);
    const float Health = FMath::Clamp(P->GetHealth()/FMath::Max(1.f,P->GetMaxHealth()),0.f,1.f);
    DrawRect(FLinearColor(.12f,.18f,.19f),M+17*K,H-M-57*K,188*K,7*K);
    DrawRect(Health>.3f?Teal:FLinearColor(.91f,.24f,.14f),M+17*K,H-M-57*K,188*K*Health,7*K);
    Label(FString::Printf(TEXT("%03d"),FMath::CeilToInt(P->GetHealth())),M+217*K,H-M-68*K,.85f*K);
    Label("WASD MOVE   SHIFT RUN",M+17*K,H-M-31*K,.62f*K,Dim);
    Label("ESC PAUSE",M+17*K,H-M-14*K,.62f*K,Dim);
    if (const UONEWeaponComponent* Weapon = P->GetWeaponComponent())
    {
        Panel(W-M-276*K,H-M-102*K,276*K,102*K);
        const float X = W-M-259*K;
        Label("AR-01 / RESPONSE CARBINE",X,H-M-90*K,.68f*K,Dim);
        Label(FString::Printf(TEXT("%02d"),Weapon->GetAmmo()),X,H-M-64*K,1.65f*K,Weapon->GetAmmo()?FLinearColor::White:Amber);
        Label(FString::Printf(TEXT("/ %03d"),Weapon->GetReserveAmmo()),X+67*K,H-M-47*K,.9f*K,Dim);
        Label(Weapon->IsReloading()?TEXT("RELOADING"):(Weapon->GetTimeSinceEmpty()<.45f?TEXT("EMPTY / PRESS R"):TEXT("LMB FIRE   R RELOAD")),X,H-M-24*K,.63f*K,Weapon->IsReloading()||Weapon->GetAmmo()==0?Amber:Dim);
        if (Weapon->IsReloading()) DrawRect(Amber,X,H-M-7*K,240*K*Weapon->GetReloadProgress(),2*K);
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
    if (GetWorld()->GetTimeSeconds()-P->LastDamageTime<.22f)
    {
        const FLinearColor Damage(.72f,.12f,.055f,.72f);
        DrawRect(Damage,0,0,W,3); DrawRect(Damage,0,H-3,W,3);
        DrawRect(Damage,0,0,3,H); DrawRect(Damage,W-3,0,3,H);
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
