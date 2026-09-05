#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ONEHealthComponent.generated.h"

UCLASS(ClassGroup=(ONE), meta=(BlueprintSpawnableComponent))
class PROJECTONE_API UONEHealthComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UONEHealthComponent();
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Health") float MaxHealth = 100.f;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Health") float Health = 100.f;
    virtual void BeginPlay() override;
    bool ApplyDamage(float Amount);
    bool IsDead() const { return Health <= 0.f; }
    void Restore() { Health = MaxHealth; }
};
