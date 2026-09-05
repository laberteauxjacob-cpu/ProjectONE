#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "ONEUITypes.h"
#include "ONE05UICheck.generated.h"
class AONEPlayer;
class AONEPlayerController;
class AONEHUD;
class AONEGameMode;
class AONEProgressionMachine;
class FJsonValue;

/** Rendered, sparse UI regression through the production controller. */
UCLASS()
class PROJECTONE_API AONE05UICheck : public AActor
{
    GENERATED_BODY()
public:
    AONE05UICheck();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NewStage);
    void Key(const FKey& Button,EInputEvent Event);
    bool BeginClick(EONEUIAction Action,int32 FollowingStage);
    void AdvanceClick();
    void Capture(const FString& Name);
    void Screenshot(int32 Width,int32 Height,const TArray<FColor>& Colors);
    void Finish();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEPlayerController> Controller;
    UPROPERTY() TObjectPtr<AONEHUD> HUD;
    UPROPERTY() TObjectPtr<AONEGameMode> GM;
    TWeakObjectPtr<AONEProgressionMachine> Box;
    TArray<TSharedPtr<FJsonValue>> Records,Frames;
    FString Folder,PendingFrame,InputCsv;
    int32 Stage=0,Checks=0,Failures=0,ClickPhase=0,ClickNext=0,Width=0,Height=0;
    int32 Shots=0,SparseCues=0;
    float FrozenSurvival=0;
    uint64 OriginalRun=0;
    EONEUIAction Clicking=EONEUIAction::None;
    FVector2D ClickCenter=FVector2D::ZeroVector;
    double StartedReal=0,StageReal=0,ClickReal=0,FinishedReal=0;
    bool bFinished=false,bQuitRequested=false;
};
