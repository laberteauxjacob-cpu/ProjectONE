#pragma once
#include "CoreMinimal.h"

/** Scheduling shared by the live weapon and deterministic rate/hitch tests. */
namespace ONEWeaponTiming
{
    inline double Interval(double Configured) { return FMath::Max(.04,Configured); }
    inline bool IsDue(double Now,double Deadline) { return Now+.00001>=Deadline; }
    inline double AfterDischarge(double PreviousDeadline,double ActualTime,double Configured,bool bEstablished)
    {
        const double Step=Interval(Configured);
        // Preserve fractional tick phase, never a whole missing discharge.
        // The caller emits at most one actual shot per evaluated game frame.
        return bEstablished && ActualTime-PreviousDeadline<Step ? PreviousDeadline+Step : ActualTime+Step;
    }
}
