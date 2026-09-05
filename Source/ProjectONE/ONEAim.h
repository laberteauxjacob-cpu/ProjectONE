#pragma once
#include "CoreMinimal.h"

/** Shared character-centred intent and post-pose muzzle convergence. Collision
 * remains the weapon's shoulder/muzzle traces, including point-blank targets. */
namespace ONEAim
{
    FVector ResolveIntent(const FVector& Origin, const FVector& CursorPoint,
        const FVector& PreviousDirection, float CenterRadius=4.f);
    FVector ResolveShotDirection(const FVector& Origin, const FVector& CursorPoint,
        const FVector& IntendedDirection, const FVector& EvaluatedMuzzle,
        float ConvergenceAhead=120.f, float MaximumPitchDegrees=35.f);
    bool IsForwardSegment(const FVector& Start, const FVector& End, const FVector& IntendedDirection);
}
