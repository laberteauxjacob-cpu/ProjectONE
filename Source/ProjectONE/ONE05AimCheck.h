#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONE05AimCheck.generated.h"
class AONEPlayer;
class AONEZombie;

/** Opt-in arena collision regression; fixtures are explicit and separate
 * from the ordinary-input review recordings. */
UCLASS()
class PROJECTONE_API AONE05AimCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE05AimCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 Value);
    void PrepareVariant();
    void PrepareTrial();
    void ClearScene();
    void Finish();
    void Key(bool Pressed);
    void AimAt(const FVector& Point);
    AONEZombie* MakeTarget(const FVector& BodyCenter);
    AActor* MakeCover(const FVector& Center,float Yaw);
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEZombie> Target;
    UPROPERTY() TObjectPtr<AActor> Cover;
    int32 Stage=0,Variant=0,Trial=0,Checks=0,Failures=0,Shots=0;
    int32 VariantLimit=6,HeadingLimit=8;
    float StageAt=0,TargetHealth=0;
    double Started=0,Ended=0;
    bool bFinished=false,bProjectedAim=false;
    FVector Origin=FVector::ZeroVector,Intent=FVector::ForwardVector;
    FString Report,Csv;
};
