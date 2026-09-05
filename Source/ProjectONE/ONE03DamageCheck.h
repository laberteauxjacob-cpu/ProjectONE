#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ONEWeaponTypes.h"
#include "ONE03DamageCheck.generated.h"
class AONEPlayer;
class AONEZombie;
class AONEGorePiece;
/** Opt-in anatomical damage transactions and remaining-arm contact checks. */
UCLASS()
class PROJECTONE_API AONE03DamageCheck : public AActor
{
    GENERATED_BODY()
public:
    AONE03DamageCheck();
    virtual void BeginPlay() override;
    virtual void Tick(float Dt) override;
private:
    void Check(bool Pass,const FString& Label);
    void Next(int32 NewStage);
    void Finish();
    AONEZombie* SpawnFixture(const FVector& Offset,bool Frozen=true);
    FONEWeaponDamagePacket MakePacket(AONEZombie* Zombie,EONEHitRegion Region,float Damage,float Trauma,FName Bone);
    bool TraceArm(AONEZombie* Zombie,bool Left,FHitResult& Hit) const;
    void CheckTransactions();
    UPROPERTY() TObjectPtr<AONEPlayer> Player;
    UPROPERTY() TObjectPtr<AONEZombie> Target;
    TWeakObjectPtr<AONEGorePiece> LatePiece;
    int32 Stage=0,Checks=0,Failures=0;
    float StageStart=0.f;
    float SettledFor=0.f;
    double StartReal=0,FinishReal=0;
    bool bFinished=false;
    uint64 NextShot=930000;
    FTransform LateThighWorld=FTransform::Identity,LateThighInPelvis=FTransform::Identity;
    FONEWeaponDamagePacket LatePacket;
    int32 LatePoints=0,LateKills=0,LateTransactions=0,LateCorpseTransactions=0,LateSevers=0;
    FString Report;
};
