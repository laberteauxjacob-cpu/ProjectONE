#include "ONEAim.h"

FVector ONEAim::ResolveIntent(const FVector& Origin,const FVector& CursorPoint,
    const FVector& PreviousDirection,float CenterRadius)
{
    FVector Previous=PreviousDirection.ContainsNaN() ? FVector::ForwardVector : PreviousDirection.GetSafeNormal2D();
    if (Previous.IsNearlyZero()) Previous=FVector::ForwardVector;
    if (Origin.ContainsNaN() || CursorPoint.ContainsNaN()) return Previous;
    const FVector Delta=CursorPoint-Origin;
    const double Radius=FMath::Clamp(double(CenterRadius),0.1,20.0);
    return Delta.SizeSquared2D()>Radius*Radius ? Delta.GetSafeNormal2D() : Previous;
}

FVector ONEAim::ResolveShotDirection(const FVector& Origin,const FVector& CursorPoint,
    const FVector& IntendedDirection,const FVector& EvaluatedMuzzle,float ConvergenceAhead,float MaximumPitchDegrees)
{
    const FVector Intent=ResolveIntent(Origin,Origin,IntendedDirection);
    if (Origin.ContainsNaN() || EvaluatedMuzzle.ContainsNaN() || CursorPoint.ContainsNaN()) return Intent;
    // A target already ahead of the muzzle retains its actual height/distance.
    // Extending every close target by120cm flattened legitimate torso/leg aim.
    // Only a target behind the muzzle needs virtual forward convergence.
    const double Ahead=FMath::Clamp(double(ConvergenceAhead),30.0,300.0);
    const double CursorDistance=FVector::DotProduct(CursorPoint-Origin,Intent);
    const double MuzzleDistance=FVector::DotProduct(EvaluatedMuzzle-Origin,Intent);
    FVector Target=CursorPoint;
    if (CursorDistance<=MuzzleDistance+.1)
    { Target=Origin+Intent*(MuzzleDistance+Ahead); Target.Z=CursorPoint.Z; }
    FVector Delta=Target-EvaluatedMuzzle;
    const double Forward=FVector::DotProduct(Delta,Intent);
    const FVector Lateral=FVector(Delta.X,Delta.Y,0)-Intent*Forward;
    // Limit a nearly coincident point beside the barrel to45degrees of yaw.
    // This keeps the full-circle intent while avoiding a sideways singularity.
    if (Lateral.SizeSquared()>Forward*Forward)
    {
        const FVector XY=Intent*Forward+Lateral.GetSafeNormal()*Forward;
        Delta.X=XY.X; Delta.Y=XY.Y;
    }
    const double Horizontal=Delta.Size2D();
    const double Limit=Horizontal*FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(double(MaximumPitchDegrees),5.0,60.0)));
    Delta.Z=FMath::Clamp(Delta.Z,-Limit,Limit);
    const FVector Result=Delta.GetSafeNormal();
    return Result.ContainsNaN() || Result.IsNearlyZero() || FVector::DotProduct(Result,Intent)<=0 ? Intent : Result;
}

bool ONEAim::IsForwardSegment(const FVector& Start,const FVector& End,const FVector& IntendedDirection)
{
    return !Start.ContainsNaN() && !End.ContainsNaN() && !IntendedDirection.ContainsNaN() &&
        FVector::DotProduct(End-Start,IntendedDirection)>0.1;
}
