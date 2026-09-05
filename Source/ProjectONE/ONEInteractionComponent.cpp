#include "ONEInteractionComponent.h"
#include "ONEProgressionMachine.h"
#include "ONEPlayer.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

UONEInteractionComponent::UONEInteractionComponent()
{
    PrimaryComponentTick.bCanEverTick=true;
}
FONEInteractionOffer UONEInteractionComponent::FindOffer() const
{
    AONEPlayer* P=Cast<AONEPlayer>(GetOwner());
    if (!P || P->IsDead() || UGameplayStatics::IsGamePaused(this)) return {};
    AONEProgressionMachine* Best=nullptr;
    float BestDistance=BIG_NUMBER;
    for (TActorIterator<AONEProgressionMachine> It(GetWorld());It;++It)
    {
        if (!It->CanReach(P)) continue;
        const float D=FVector::DistSquared(P->GetActorLocation(),It->GetInteractionPoint());
        if (D<BestDistance) { BestDistance=D; Best=*It; }
    }
    return Best ? Best->BuildOffer(P) : FONEInteractionOffer{};
}
void UONEInteractionComponent::Press()
{
    if (bHeld || bLatched) return;
    bHeld=true; HoldElapsed=0; Focus=FindOffer(); Started=Focus;
    if (!Started.bEnabled) bLatched=true;
}
void UONEInteractionComponent::Release()
{
    bHeld=false; bLatched=false; HoldElapsed=0; Started={};
}
void UONEInteractionComponent::Cancel(bool bReleaseInput)
{
    HoldElapsed=0; Started={}; bLatched=bHeld;
    if (bReleaseInput) Release();
}
void UONEInteractionComponent::TickComponent(float Dt,ELevelTick TickType,FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(Dt,TickType,TickFunction);
    Focus=FindOffer();
    if (!bHeld || bLatched) return;
    if (!Focus.bEnabled || !Started.SameContext(Focus)) { Cancel(); return; }
    const float Required=FMath::Clamp(HoldDuration,.1f,2.f);
    HoldElapsed=FMath::Min(Required,HoldElapsed+Dt);
    if (HoldElapsed+KINDA_SMALL_NUMBER<Required) return;
    bLatched=true;
    if (AONEProgressionMachine* Machine=Started.Machine.Get())
        if (Machine->CommitOffer(Cast<AONEPlayer>(GetOwner()),Started)) ++CompletedHolds;
    HoldElapsed=0;
    Focus=FindOffer();
}
