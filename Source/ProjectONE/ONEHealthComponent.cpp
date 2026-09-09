#include "ONEHealthComponent.h"
#include "Kismet/GameplayStatics.h"
UONEHealthComponent::UONEHealthComponent()
{ PrimaryComponentTick.bCanEverTick=true; PrimaryComponentTick.bStartWithTickEnabled=false; }
void UONEHealthComponent::BeginPlay() { Super::BeginPlay(); Restore(); SetComponentTickEnabled(bPlayerRecoveryEligible); }
bool UONEHealthComponent::ApplyDamage(float Amount)
{
    if (IsDead() || !FMath::IsFinite(Amount) || Amount<=0.f) return false;
    const float Before=Health;
    Health=FMath::Max(0.f,Health-Amount);
    if (Health>=Before) return false;
    DamageFreeSeconds=0.; bAcceptedDamage=true; bRegenerating=false;
    if (IsDead()) InvalidateRecovery();
    OnHealthChanged.Broadcast(Before,Health,MaxHealth,false);
    return true;
}
void UONEHealthComponent::EnablePlayerRegeneration(bool Enabled)
{
    bPlayerRecoveryEligible=Enabled;
    SetComponentTickEnabled(Enabled);
    if (!Enabled) InvalidateRecovery();
}
void UONEHealthComponent::InvalidateRecovery()
{ DamageFreeSeconds=0.; bAcceptedDamage=false; bRegenerating=false; }
void UONEHealthComponent::Restore()
{
    const float Before=Health;
    MaxHealth=FMath::IsFinite(MaxHealth) ? FMath::Max(1.f,MaxHealth) : 100.f;
    Health=MaxHealth; InvalidateRecovery();
    if (Before!=Health) OnHealthChanged.Broadcast(Before,Health,MaxHealth,false);
}
void UONEHealthComponent::SetMaximumHealth(float NewMaximum,bool PreserveFraction)
{
    if (!FMath::IsFinite(NewMaximum) || NewMaximum<=0.f) return;
    const float Before=Health,Ratio=MaxHealth>0.f ? FMath::Clamp(Health/MaxHealth,0.f,1.f) : 0.f;
    MaxHealth=NewMaximum;
    Health=FMath::Clamp(PreserveFraction ? Ratio*MaxHealth : Health,0.f,MaxHealth);
    OnHealthChanged.Broadcast(Before,Health,MaxHealth,false);
}
float UONEHealthComponent::GetMissingHealthFraction() const
{ return MaxHealth>0.f ? 1.f-FMath::Clamp(Health/MaxHealth,0.f,1.f) : 0.f; }
float UONEHealthComponent::GetRegenerationDelayRemaining() const
{ return bPlayerRecoveryEligible && bAcceptedDamage && !IsDead() ? FMath::Max(0.f,RegenerationDelay-float(DamageFreeSeconds)) : 0.f; }
void UONEHealthComponent::TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime,TickType,TickFunction);
    bRegenerating=false;
    if (!bPlayerRecoveryEligible || !bAcceptedDamage || IsDead() || !FMath::IsFinite(DeltaTime) || DeltaTime<=0.f ||
        (GetWorld() && UGameplayStatics::IsGamePaused(this))) return;
    const double BeforeTime=DamageFreeSeconds;
    DamageFreeSeconds+=DeltaTime;
    const double Delay=FMath::Max(0.f,RegenerationDelay);
    const double EligibleSeconds=FMath::Max(0.,DamageFreeSeconds-Delay)-FMath::Max(0.,BeforeTime-Delay);
    if (EligibleSeconds<=0. || Health>=MaxHealth || !FMath::IsFinite(RegenerationFractionPerSecond)) return;
    const float Before=Health;
    Health=FMath::Min(MaxHealth,Health+float(EligibleSeconds)*MaxHealth*FMath::Clamp(RegenerationFractionPerSecond,0.f,1.f));
    bRegenerating=Health>Before;
    if (bRegenerating) OnHealthChanged.Broadcast(Before,Health,MaxHealth,true);
}
void UONEHealthComponent::EndPlay(const EEndPlayReason::Type Reason)
{ InvalidateRecovery(); OnHealthChanged.Clear(); Super::EndPlay(Reason); }
