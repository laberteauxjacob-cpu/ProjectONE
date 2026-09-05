#include "ONEWeaponComponent.h"
#include "ONEPlayer.h"
#include "ONEZombie.h"
#include "ONEBloodSubsystem.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
UONEWeaponComponent::UONEWeaponComponent() { PrimaryComponentTick.bCanEverTick=true; }
void UONEWeaponComponent::BeginPlay()
{
    Super::BeginPlay(); Ammo=MagazineSize;
    ShotSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/ONE/Audio/S_CarbineShot.S_CarbineShot"));
    ReloadSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/ONE/Audio/S_Reload.S_Reload"));
    EmptySound=LoadObject<USoundBase>(nullptr,TEXT("/Game/ONE/Audio/S_Empty.S_Empty"));
    ImpactSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/ONE/Audio/S_Impact.S_Impact"));
}
float UONEWeaponComponent::GetReloadElapsed() const { return bReloading ? GetWorld()->GetTimeSeconds()-ReloadStart : 0.f; }
float UONEWeaponComponent::GetReloadProgress() const { return bReloading ? FMath::Clamp(GetReloadElapsed()/ReloadDuration,0.f,1.f) : 0.f; }
float UONEWeaponComponent::GetTimeSinceShot() const { return GetWorld()->GetTimeSeconds()-LastShot; }
float UONEWeaponComponent::GetTimeSinceEmpty() const { return GetWorld()->GetTimeSeconds()-LastEmpty; }
float UONEWeaponComponent::GetTimeSinceHit() const { return GetWorld()->GetTimeSeconds()-LastHit; }
void UONEWeaponComponent::BeginReload()
{
    if (bReloading || Ammo==MagazineSize || ReserveAmmo<=0) return;
    bReloading=true; ReloadStart=GetWorld()->GetTimeSeconds();
    if (ReloadSound) UGameplayStatics::PlaySound2D(this,ReloadSound,.38f);
}
void UONEWeaponComponent::TickComponent(float Dt,ELevelTick Tick,FActorComponentTickFunction* ThisTick)
{
    Super::TickComponent(Dt,Tick,ThisTick);
    AONEPlayer* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead()) return;
    if (bReloading && GetReloadElapsed() >= ReloadDuration)
    {
        const int32 Take=FMath::Min(MagazineSize-Ammo,ReserveAmmo);
        Ammo+=Take; ReserveAmmo-=Take; bReloading=false;
    }
    if (bTrigger && !bReloading && GetTimeSinceShot()>=FireInterval) Fire();
}
void UONEWeaponComponent::Fire()
{
    AONEPlayer* P=Cast<AONEPlayer>(GetOwner()); if (!P) return;
    if (Ammo<=0)
    {
        if (GetTimeSinceEmpty()>.35f)
        {
            LastEmpty=GetWorld()->GetTimeSeconds();
            if (EmptySound) UGameplayStatics::PlaySound2D(this,EmptySound,.6f);
        }
        return;
    }
    --Ammo; LastShot=GetWorld()->GetTimeSeconds(); P->FlashMuzzle();
    FVector Start=P->GetMuzzleLocation();
    if (ShotSound) UGameplayStatics::PlaySoundAtLocation(this,ShotSound,Start,.6f,FMath::FRandRange(.96f,1.04f));
    FVector Direction=(P->GetAimPoint()-Start).GetSafeNormal();
    if (Direction.IsNearlyZero()) Direction=P->GetActorForwardVector();
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(Carbine),false,P);
    // Cover adjacent to the shoulder blocks the shot before it can emerge through a wall.
    const FVector Shoulder=P->GetActorLocation()+FVector(0,0,42);
    bool bHit=GetWorld()->LineTraceSingleByChannel(Hit,Shoulder,Start,ECC_Visibility,Params);
    if (!bHit) bHit=GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Direction*Range,ECC_Visibility,Params);
    const FVector End=bHit ? Hit.ImpactPoint : Start+Direction*Range;
    if (auto* Blood=GetWorld()->GetSubsystem<UONEBloodSubsystem>()) Blood->Shot(Start,End);
    if (bHit)
    {
        if (ImpactSound) UGameplayStatics::PlaySoundAtLocation(this,ImpactSound,Hit.ImpactPoint,.28f);
        if (auto* Z=Cast<AONEZombie>(Hit.GetActor()))
        {
            if (!Z->IsDead())
            {
                Z->ReceiveBullet(Hit,Direction,Damage);
                LastHit=GetWorld()->GetTimeSeconds(); bLastHitKill=Z->IsDead();
            }
        }
    }
}
