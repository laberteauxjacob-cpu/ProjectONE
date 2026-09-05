#include "ONEHealthComponent.h"
UONEHealthComponent::UONEHealthComponent() { PrimaryComponentTick.bCanEverTick = false; }
void UONEHealthComponent::BeginPlay() { Super::BeginPlay(); Health = MaxHealth; }
bool UONEHealthComponent::ApplyDamage(float Amount)
{
    if (IsDead() || Amount <= 0.f) return false;
    Health = FMath::Max(0.f, Health - Amount);
    return true;
}
