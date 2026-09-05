#include "ONE05UICheck.h"
#include "ONEPlayer.h"
#include "ONEPlayerController.h"
#include "ONEHUD.h"
#include "ONEGameMode.h"
#include "ONEWeaponComponent.h"
#include "ONEHealthComponent.h"
#include "ONEAmbientAudioComponent.h"
#include "ONEProgressionMachine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "InputKeyEventArgs.h"
#include "CoreGlobals.h"
#include "UnrealClient.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "HAL/PlatformMisc.h"

namespace ONE05UIFixture
{
    bool bRestartPending=false;
    int32 RestartChecks=0,RestartFailures=0,RestartWidth=0,RestartHeight=0;
    uint64 PreviousRun=0;
    double OriginalStartedReal=0;
    TWeakObjectPtr<AONEProgressionMachine> PreviousBox;
    FString RestartFolder,RestartInputs;
    TArray<TSharedPtr<FJsonValue>> RestartRecords,RestartFrames;
}
AONE05UICheck::AONE05UICheck()
{ PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true; PrimaryActorTick.TickGroup=TG_PostUpdateWork; }
void AONE05UICheck::BeginPlay()
{
    Super::BeginPlay(); StartedReal=StageReal=FPlatformTime::Seconds();
    InputCsv=TEXT("real_seconds,world_seconds,frame,stage,key,event,handled,x,y\n");
    UGameViewportClient::OnScreenshotCaptured().AddUObject(this,&AONE05UICheck::Screenshot);
    if (ONE05UIFixture::bRestartPending)
    {
        ONE05UIFixture::bRestartPending=false; Stage=90;
        Checks=ONE05UIFixture::RestartChecks; Failures=ONE05UIFixture::RestartFailures;
        Width=ONE05UIFixture::RestartWidth; Height=ONE05UIFixture::RestartHeight;
        StartedReal=ONE05UIFixture::OriginalStartedReal;
        Folder=ONE05UIFixture::RestartFolder; InputCsv=MoveTemp(ONE05UIFixture::RestartInputs);
        Records=MoveTemp(ONE05UIFixture::RestartRecords); Frames=MoveTemp(ONE05UIFixture::RestartFrames);
    }
}
void AONE05UICheck::EndPlay(const EEndPlayReason::Type Reason)
{ UGameViewportClient::OnScreenshotCaptured().RemoveAll(this); Super::EndPlay(Reason); }
void AONE05UICheck::Check(bool Pass,const FString& Label)
{
    ++Checks; if (!Pass) ++Failures;
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("label"),Label); R->SetBoolField(TEXT("pass"),Pass);
    R->SetNumberField(TEXT("stage"),Stage); R->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds());
    Records.Add(MakeShared<FJsonValueObject>(R));
    UE_LOG(LogTemp,Display,TEXT("ONE05_UI %s | %s"),Pass?TEXT("PASS"):TEXT("FAIL"),*Label);
}
void AONE05UICheck::Next(int32 NewStage) { Stage=NewStage; StageReal=FPlatformTime::Seconds(); }
void AONE05UICheck::Key(const FKey& Button,EInputEvent Event)
{
    if (!Controller) return;
    float X=0,Y=0; Controller->GetMousePosition(X,Y);
    const bool Handled=Controller->InputKey(FInputKeyEventArgs::CreateSimulated(Button,Event,Event==IE_Released?0.f:1.f));
    InputCsv+=FString::Printf(TEXT("%.6f,%.6f,%llu,%d,%s,%d,%d,%.2f,%.2f\n"),FPlatformTime::Seconds()-StartedReal,
        GetWorld()->GetTimeSeconds(),GFrameCounter,Stage,*Button.ToString(),int32(Event),Handled,X,Y);
}
bool AONE05UICheck::BeginClick(EONEUIAction Action,int32 FollowingStage)
{
    FBox2D Rect(ForceInit);
    const bool Found=HUD && HUD->GetActionBounds(Action,Rect);
    const bool Valid=Found && Rect.bIsValid && Rect.Min.X>=0 && Rect.Min.Y>=0 && Rect.Max.X<=Width && Rect.Max.Y<=Height && Rect.GetSize().GetMin()>5;
    Check(Valid,
        FString::Printf(TEXT("Drawn action%d has a nonempty in-viewport hit rectangle"),int32(Action)));
    if (!Valid) { Finish(); return false; }
    ClickCenter=Rect.GetCenter(); Controller->SetMouseLocation(FMath::RoundToInt(ClickCenter.X),FMath::RoundToInt(ClickCenter.Y));
    Clicking=Action; ClickNext=FollowingStage; ClickPhase=1; ClickReal=FPlatformTime::Seconds();
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("kind"),TEXT("drawn_button_target")); R->SetNumberField(TEXT("action"),int32(Action));
    R->SetNumberField(TEXT("x"),ClickCenter.X); R->SetNumberField(TEXT("y"),ClickCenter.Y);
    R->SetNumberField(TEXT("min_x"),Rect.Min.X); R->SetNumberField(TEXT("min_y"),Rect.Min.Y); R->SetNumberField(TEXT("max_x"),Rect.Max.X); R->SetNumberField(TEXT("max_y"),Rect.Max.Y);
    Records.Add(MakeShared<FJsonValueObject>(R)); return true;
}
void AONE05UICheck::AdvanceClick()
{
    if (FPlatformTime::Seconds()-ClickReal<.12) return;
    if (ClickPhase==1)
    {
        float X=0,Y=0; const bool Located=Controller->GetMousePosition(X,Y);
        Check(Located && FVector2D::Distance(FVector2D(X,Y),ClickCenter)<2.f,TEXT("Controller cursor readback reaches the actual drawn button center"));
        Key(EKeys::LeftMouseButton,IE_Pressed); ClickPhase=2; ClickReal=FPlatformTime::Seconds();
    }
    else
    {
        const EONEUIAction Action=Clicking;
        // The old actor may be replaced by Restart's OpenLevel after this call.
        Key(EKeys::LeftMouseButton,IE_Released); ClickPhase=0; Next(ClickNext);
        if (Action==EONEUIAction::Restart)
        {
            // Include bounds, cursor, press and release evidence created after
            // the pre-restart snapshot; OpenLevel executes after this tick.
            ONE05UIFixture::bRestartPending=true;
            ONE05UIFixture::RestartChecks=Checks; ONE05UIFixture::RestartFailures=Failures;
            ONE05UIFixture::RestartInputs=InputCsv; ONE05UIFixture::RestartRecords=Records;
        }
        if (Action==EONEUIAction::Quit)
        {
            bQuitRequested=IsEngineExitRequested();
            Check(bQuitRequested,TEXT("Production Quit button requests engine exit after matched pointer release")); Finish();
        }
    }
}
void AONE05UICheck::Capture(const FString& Name)
{
    Check(PendingFrame.IsEmpty(),TEXT("Sparse screenshot requests do not overlap"));
    if (!PendingFrame.IsEmpty()) return;
    PendingFrame=Name+TEXT(".png"); FScreenshotRequest::RequestScreenshot(Folder/PendingFrame,true,false);
}
void AONE05UICheck::Screenshot(int32 ImageWidth,int32 ImageHeight,const TArray<FColor>& Colors)
{
    if (PendingFrame.IsEmpty()) return;
    const bool Saved=FImageUtils::SaveImageByExtension(*(Folder/PendingFrame),FImageView(Colors.GetData(),ImageWidth,ImageHeight));
    Check(Saved && ImageWidth==Width && ImageHeight==Height,TEXT("Real rendered PNG preserves requested viewport dimensions"));
    if (Saved)
    {
        auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("file"),PendingFrame); R->SetNumberField(TEXT("width"),ImageWidth); R->SetNumberField(TEXT("height"),ImageHeight);
        R->SetNumberField(TEXT("world_seconds"),GetWorld()->GetTimeSeconds()); R->SetNumberField(TEXT("frame"),double(GFrameCounter));
        R->SetNumberField(TEXT("points"),GM?GM->GetPoints():-1); R->SetBoolField(TEXT("paused"),UGameplayStatics::IsGamePaused(this));
        R->SetBoolField(TEXT("tools"),HUD && HUD->IsToolsOpen()); R->SetBoolField(TEXT("dead"),Player && Player->IsDead());
        R->SetNumberField(TEXT("ammo"),Player?Player->GetWeaponComponent()->GetAmmo():-1); R->SetNumberField(TEXT("reserve"),Player?Player->GetWeaponComponent()->GetReserveAmmo():-1);
        Frames.Add(MakeShared<FJsonValueObject>(R));
    }
    PendingFrame.Empty();
}
void AONE05UICheck::Finish()
{
    if (bFinished) return;
    bFinished=true; FinishedReal=FPlatformTime::Seconds();
    auto R=MakeShared<FJsonObject>(); R->SetStringField(TEXT("candidate"),TEXT("05"));
    R->SetStringField(TEXT("fixture"),TEXT("Rendered production Controller InputKey events and SetMouseLocation at read-back drawn HUD button rectangles, with press/release separated by real frames. Not native OS input. Actual sandbox10000-point button grant followed by explicit TrySpendPoints8500 fixture exposes1500 typography. One safe-contact teleport to existing mystery box precedes a real production F-hold so death/restart invalidates an active machine. Fatal ReceiveAttack is an explicit death fixture. Sparse original viewport PNGs only; no screenshot replacement, audio recording or performance claim. Final Quit is verified by engine exit request; runner must also verify process exit."));
    R->SetNumberField(TEXT("width"),Width); R->SetNumberField(TEXT("height"),Height); R->SetNumberField(TEXT("checks"),Checks); R->SetNumberField(TEXT("failures"),Failures);
    R->SetBoolField(TEXT("quit_requested_via_button"),bQuitRequested); R->SetArrayField(TEXT("assertions"),Records); R->SetArrayField(TEXT("frames"),Frames);
    FString Json; FJsonSerializer::Serialize(R,TJsonWriterFactory<>::Create(&Json));
    if (Folder.IsEmpty()) { Folder=FPaths::ProjectSavedDir()/TEXT("Candidate05/UIUnknown"); IFileManager::Get().MakeDirectory(*Folder,true); }
    const bool JsonSaved=FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("checks.json")));
    const bool InputSaved=FFileHelper::SaveStringToFile(InputCsv,*(Folder/TEXT("input.csv")));
    if (!JsonSaved || !InputSaved) ++Failures;
    UE_LOG(LogTemp,Display,TEXT("ONE05_UI_COMPLETE failures=%d checks=%d width=%d height=%d"),Failures,Checks,Width,Height);
}
void AONE05UICheck::Tick(float Dt)
{
    Super::Tick(Dt); const double Real=FPlatformTime::Seconds();
    if (bFinished) { if (Real-FinishedReal>.6) FPlatformMisc::RequestExit(false); return; }
    if (Real-StartedReal>35 || Real-StageReal>6) { Check(false,FString::Printf(TEXT("Bounded rendered UI timeout at stage%d"),Stage)); Finish(); return; }
    if (!Player) Player=Cast<AONEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if (!Player) return;
    if (!Controller) Controller=Cast<AONEPlayerController>(Player->GetController());
    if (!Controller) return;
    if (!HUD) HUD=Cast<AONEHUD>(Controller->GetHUD());
    if (!GM) GM=GetWorld()->GetAuthGameMode<AONEGameMode>();
    if (!HUD || !GM) return;
    if (ClickPhase>0) { AdvanceClick(); return; }
    const double T=Real-StageReal; auto* W=Player->GetWeaponComponent();
    switch (Stage)
    {
    case 0: if (T>.8)
    {
        Controller->GetViewportSize(Width,Height); Folder=FPaths::ProjectSavedDir()/FString::Printf(TEXT("Candidate05/UI%dx%d"),Width,Height); IFileManager::Get().MakeDirectory(*Folder,true);
        Check(Width>=1280 && Height>=720 && GM->IsSandbox(),TEXT("Rendered UI fixture has a supported viewport and explicit sandbox"));
        Check(HUD->HasCompleteArtwork(),TEXT("Custom HUD artwork resolves before visual capture"));
        Check(GM->GetPoints()==0 && W->GetDefinition().Id==FName(TEXT("P1911")) && W->GetAmmo()==7 && W->GetReserveAmmo()==56 && !W->GetDefinitionForWeapon(1),TEXT("Starter UI corresponds to actual zero points, seven loaded, fifty-six reserve and empty slot two"));
        Check(!HUD->IsToolsOpen() && !HUD->IsPointerUIActive() && HUD->GetLayoutOverlapCount()==0,TEXT("Default compact gameplay HUD has no expanded tray or overlapping measured panels"));
        Shots=W->GetTotalShotsFired(); OriginalRun=W->GetRunId(); Capture(TEXT("01_starter_0_7_56")); Next(1);
    } break;
    case 1: if (T>.2 && PendingFrame.IsEmpty()) { Key(EKeys::H,IE_Pressed); Next(2); } break;
    case 2: if (T>.12) { Key(EKeys::H,IE_Released); Next(3); } break;
    case 3: if (T>.2)
    {
        Check(HUD->IsToolsOpen() && HUD->IsPointerUIActive() && Controller->bShowMouseCursor,TEXT("Production H opens tool tray and exposes the UI pointer"));
        Check(HUD->GetLayoutOverlapCount()==0,TEXT("Expanded tools preserve separated measured gameplay panels"));
        Capture(TEXT("02_expanded_tools")); BeginClick(EONEUIAction::GrantPoints,4);
    } break;
    case 4: if (T>.2)
    {
        Check(GM->GetPoints()==10000 && GM->GetSandboxGrantedPoints()==10000 && W->GetTotalShotsFired()==Shots,TEXT("Drawn Grant Points button grants exactly10000 through production UI without firing"));
        Check(GM->TrySpendPoints(8500,GM->NewMachineReceipt()) && GM->GetPoints()==1500,TEXT("Declared exact8500 spend fixture exposes genuine1500 point display"));
        BeginClick(EONEUIAction::CloseTools,5);
    } break;
    case 5: if (T>.2)
    {
        Check(!HUD->IsToolsOpen() && !HUD->IsPointerUIActive() && W->GetTotalShotsFired()==Shots && W->GetAmmo()==7,TEXT("Back to game button closes tray without allowing its release to fire"));
        Check(HUD->GetLayoutOverlapCount()==0,TEXT("Compact1500-point HUD retains separate point/health/weapon regions"));
        Capture(TEXT("03_compact_1500")); Next(6);
    } break;
    case 6: if (T>.2 && PendingFrame.IsEmpty()) { Key(EKeys::H,IE_Pressed); Next(7); } break;
    case 7: if (T>.12) { Key(EKeys::H,IE_Released); Next(8); } break;
    case 8: if (T>.2) { Check(HUD->IsToolsOpen(),TEXT("Fresh H reopens tools")); Key(EKeys::H,IE_Pressed); Next(9); } break;
    case 9: if (T>.12) { Key(EKeys::H,IE_Released); Next(10); } break;
    case 10: if (T>.2) { Check(!HUD->IsToolsOpen(),TEXT("H also closes the tray through its normal binding")); Key(EKeys::Escape,IE_Pressed); Next(11); } break;
    case 11: if (T>.12) { Key(EKeys::Escape,IE_Released); Next(12); } break;
    case 12: if (T>.2)
    {
        Check(UGameplayStatics::IsGamePaused(this) && HUD->IsPointerUIActive() && !HUD->IsToolsOpen(),TEXT("Escape enters pause menu and closes any tools"));
        FrozenSurvival=GM->GetSurvivalSeconds(); Capture(TEXT("04_pause")); Next(13);
    } break;
    case 13: if (T>.6 && PendingFrame.IsEmpty())
    {
        Check(GM->GetSurvivalSeconds()==FrozenSurvival && W->GetTotalShotsFired()==Shots,TEXT("Paused survival clock and weapon remain frozen across real elapsed time")); BeginClick(EONEUIAction::Resume,14);
    } break;
    case 14: if (T>.3)
    {
        Check(!UGameplayStatics::IsGamePaused(this) && !HUD->IsPointerUIActive() && GM->GetSurvivalSeconds()>FrozenSurvival && W->GetTotalShotsFired()==Shots,TEXT("Resume button restarts live time without leaking its pointer release into a shot"));
        for (TActorIterator<AONEMysteryBox> It(GetWorld());It;++It) { Box=*It; break; }
        Check(Box.IsValid(),TEXT("Existing physical mystery box is available for old-callback restart fixture"));
        if (!Box.IsValid()) { Finish(); break; }
        Player->SetActorLocation(Box->GetInteractionPoint()+Box->GetActorForwardVector()*45.f); Player->GetCharacterMovement()->StopMovementImmediately(); Next(15);
    } break;
    case 15: if (T>.3) { Key(EKeys::F,IE_Pressed); Next(16); } break;
    case 16: if (T>.7) { Key(EKeys::F,IE_Released); Next(17); } break;
    case 17: if (T>.2)
    {
        Check(Box.IsValid() && Box->GetState()==EONEMachineState::Active && Box->GetAcceptedCount()==1 && GM->GetPoints()==550,TEXT("Production F hold starts a real paid box roll before death fixture"));
        Check(GM->GetAmbientAudio() && GM->GetAmbientAudio()->GetActiveVoiceCount()>0,TEXT("Ambient voices are active before the death cleanup test"));
        Player->ReceiveAttack(1000,Player->GetActorLocation()+FVector(100,0,0)); Next(18);
    } break;
    case 18: if (T>.3)
    {
        Check(Player->IsDead() && GM->IsGameOver() && HUD->IsPointerUIActive(),TEXT("Explicit fatal-damage fixture exposes the real death menu"));
        Check(GM->GetAmbientAudio() && GM->GetAmbientAudio()->GetActiveVoiceCount()==0,TEXT("Death shuts down the previously active ambient voices"));
        FrozenSurvival=GM->GetSurvivalSeconds(); SparseCues=GM->GetAmbientAudio()?GM->GetAmbientAudio()->GetSparseCueCount():0;
        Capture(TEXT("05_death")); Next(19);
    } break;
    case 19: if (T>.6 && PendingFrame.IsEmpty())
    {
        Check(GM->GetSurvivalSeconds()==FrozenSurvival && GM->GetAmbientAudio() && GM->GetAmbientAudio()->GetSparseCueCount()==SparseCues && W->GetTotalShotsFired()==Shots,TEXT("Dead survival time freezes and no new ambient cue or leaked UI shot appears"));
        ONE05UIFixture::RestartChecks=Checks; ONE05UIFixture::RestartFailures=Failures;
        ONE05UIFixture::RestartWidth=Width; ONE05UIFixture::RestartHeight=Height; ONE05UIFixture::PreviousRun=OriginalRun; ONE05UIFixture::PreviousBox=Box;
        ONE05UIFixture::OriginalStartedReal=StartedReal;
        ONE05UIFixture::RestartFolder=Folder; ONE05UIFixture::RestartInputs=InputCsv; ONE05UIFixture::RestartRecords=Records; ONE05UIFixture::RestartFrames=Frames;
        BeginClick(EONEUIAction::Restart,20);
    } break;
    case 90: if (T>.8)
    {
        Check(!Player->IsDead() && !UGameplayStatics::IsGamePaused(this) && !HUD->IsToolsOpen() && !HUD->IsPointerUIActive(),TEXT("Try Again button actually reloads the level into live compact gameplay"));
        Check(W->GetRunId()!=ONE05UIFixture::PreviousRun && W->GetDefinition().Id==FName(TEXT("P1911")) && W->GetAmmo()==7 && W->GetReserveAmmo()==56 && !W->GetDefinitionForWeapon(1) && W->GetTotalShotsFired()==0 && !W->HasAcceptedFramePress() && !W->IsAutomaticBurstActive() && GM->GetPoints()==0,TEXT("Restart resets owned slots, points and held input without turning the restart click into a shot"));
        int32 Machines=0; bool Idle=true;
        for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It) { ++Machines; Idle&=It->GetState()==EONEMachineState::Idle && It->GetPaymentReceipt()==0 && It->GetAcceptedCount()==0; }
        Check(!ONE05UIFixture::PreviousBox.IsValid() && Machines>=2 && Idle,TEXT("Restart destroys previous active box and creates clean idle machine instances"));
        Check(HUD->GetLayoutOverlapCount()==0,TEXT("Restart HUD remains within separated measured layout bounds")); Capture(TEXT("06_restarted_starter")); Next(91);
    } break;
    case 91: if (T>.2 && PendingFrame.IsEmpty()) { Key(EKeys::Escape,IE_Pressed); Next(92); } break;
    case 92: if (T>.12) { Key(EKeys::Escape,IE_Released); Next(93); } break;
    case 93: if (T>.2) { Check(UGameplayStatics::IsGamePaused(this) && Frames.Num()==6,TEXT("All six original UI screenshots completed before final pause/quit")); BeginClick(EONEUIAction::Quit,94); } break;
    default: break;
    }
}
