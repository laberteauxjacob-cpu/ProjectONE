#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ONE06ImpactSubsystem.generated.h"
class AActor;
class UInstancedStaticMeshComponent;

/** Finite actual-surface marks for grouping review and ordinary firing.
 * One instanced mesh, 256 reusable slots, 20 gameplay seconds; no collision/nav.
 * The ballistic caller supplies its actual solid-world contact, never a target
 * proxy or a projected display position. */
UCLASS()
class PROJECTONE_API UONE06ImpactSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()
public:
    void AddImpact(const FHitResult& Hit,bool bPellet);
    void Clear();
    int32 GetMarkCount() const { return ActiveCount; }
    int32 GetRecordedCount() const { return RecordedCount; }
    static constexpr int32 Capacity=256;
    static constexpr float LifetimeSeconds=20.f;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual void Deinitialize() override;
private:
    bool EnsureMesh();
    UPROPERTY() TObjectPtr<AActor> Anchor;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Marks;
    TArray<double> ExpiresAt;
    int32 NextSlot=0,ActiveCount=0,RecordedCount=0;
};
