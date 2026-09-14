#pragma once
#include "CoreMinimal.h"
#include "CollisionQueryParams.h"
class UCapsuleComponent;
class UWorld;

namespace ONE07RegionalTrace
{
    struct FCapsuleContact
    {
        double Time=0;
        FVector Position=FVector::ZeroVector,Normal=FVector::ZeroVector;
        bool bStartInside=false;
        double PenetrationDepth=0;
    };
    // Exact finite segment against the union of a cylinder and its end spheres.
    // CoreA/CoreB are sphere centers; no clipping, padding or ray redirection.
    PROJECTONE_API bool IntersectCapsuleSegment(const FVector& Start,const FVector& End,
        const FVector& CoreA,const FVector& CoreB,double Radius,FCapsuleContact& Out);
    // The current registered component transform and scaled dimensions are the
    // geometry contract. The same filtering is used by the weapon wrapper.
    PROJECTONE_API bool TraceCapsule(UCapsuleComponent* Capsule,FHitResult& Out,
        const FVector& Start,const FVector& End,const FCollisionQueryParams& Params);
    // One ordinary world Visibility trace, merged with exact regional queries.
    // Existing world hits win distance ties; walls therefore retain occlusion.
    PROJECTONE_API bool TraceWeaponSegment(UWorld* World,FHitResult& Out,
        const FVector& Start,const FVector& End,const FCollisionQueryParams& Params);
}
