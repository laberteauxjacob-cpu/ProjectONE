#pragma once
#include "CoreMinimal.h"

/** Timings are shared by the state controller and authored C05 clip inventory. */
namespace ONE05AttackMotion
{
    struct FProfile { float Duration,Contact,StepDistance,StepEnd; };
    inline FProfile Profile(int32 Family)
    {
        if (Family==1) return {1.08f,.48f,12.f,.34f};
        if (Family==2) return {1.12f,.54f,14.f,.38f};
        return {.96f,.45f,18.f,.34f};
    }
    inline float StepSpeed(int32 Family,float Age)
    {
        const FProfile P=Profile(Family);
        if (Age<0 || Age>=P.StepEnd) return 0;
        // Carry forward immediately from the final approach, then finish the step.
        // Its integral is exactly StepDistance; the runtime also enforces that cap.
        const float U=Age/P.StepEnd;
        return (P.StepDistance/P.StepEnd)*(1.f-U)*(1.f+3.f*U);
    }
    inline bool ArmsAvailable(uint8 Required,bool Left,bool Right)
    { return Required!=0 && (!(Required&1) || Left) && (!(Required&2) || Right); }
    inline bool ContactGeometry(const FVector& Origin,const FVector& Heading,const FVector& Victim,float Reach)
    {
        const FVector Delta=Victim-Origin;
        return FMath::Abs(Delta.Z)<=75.f && Delta.SizeSquared2D()<=FMath::Square(Reach) &&
            FVector::DotProduct(Delta.GetSafeNormal2D(),Heading)>.62f;
    }
}
