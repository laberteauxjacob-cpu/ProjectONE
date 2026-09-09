#pragma once
#include "CoreMinimal.h"

namespace ONE06MachineRules
{
    enum class EReadyResolution : uint8 { Wait, Collect, Expire };

    // A reachable owner wins the exact deadline boundary. A later observation
    // cannot rescue an expired item. Call once per machine tick, then commit
    // exactly one ownership transition before changing presentation.
    inline EReadyResolution ResolveReady(double Elapsed,double Deadline,bool bReachable)
    {
        if (bReachable && Elapsed<=Deadline) return EReadyResolution::Collect;
        return Elapsed>=Deadline ? EReadyResolution::Expire : EReadyResolution::Wait;
    }

    inline bool WithinUpgradeArea(const FVector& PlayerLocal,float ReachCm,float MinimumX)
    {
        // Local +X is the machine front. This anchor permits front and side
        // approaches without requiring the intake center or reaching the rear.
        const FVector Anchor(60,0,106);
        return PlayerLocal.X>=MinimumX && FMath::Abs(PlayerLocal.Z-Anchor.Z)<=145.f &&
            FVector::DistSquared2D(PlayerLocal,Anchor)<=FMath::Square(ReachCm);
    }
}
