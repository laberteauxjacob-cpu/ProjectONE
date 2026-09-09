#include "ONE06Ballistics.h"

float ONE06Ballistics::DamageAtDistance(const FONEWeaponDefinition& D,float Travel,int32 Bodies)
{
    if (!FMath::IsFinite(Travel) || Travel<0.f || Travel>FMath::Max(0.f,D.Range)) return 0.f;
    const float Falloff=FMath::Clamp((Travel-D.FalloffStart)/FMath::Max(1.f,D.Range-D.FalloffStart),0.f,1.f);
    return FMath::Max(0.f,D.Damage)*FMath::Lerp(1.f,FMath::Clamp(D.MinimumDamageFraction,0.f,1.f),Falloff)*
        FMath::Pow(FMath::Clamp(D.Penetration.DamageRetainedPerBody,0.f,1.f),FMath::Max(0,Bodies));
}
bool ONE06Ballistics::CanDamageBody(const FONEWeaponDefinition& D,float Travel,int32 Bodies)
{
    return Bodies>=0 && Bodies<FMath::Clamp(D.Penetration.MaximumBodies,1,5) &&
        (Bodies==0 || D.Penetration.AdditionalBodyRange<=0.f || Travel<=D.Penetration.AdditionalBodyRange) &&
        DamageAtDistance(D,Travel,Bodies)>=FMath::Max(.01f,D.Penetration.MinimumDamage);
}
FVector ONE06Ballistics::SampleDirection(FRandomStream& Stream,const FVector& Intent,float Degrees)
{
    // Always consume the same two samples, including zero-spread comparisons.
    const float U=Stream.FRand(),V=Stream.FRand();
    const FVector Axis=Intent.GetSafeNormal(SMALL_NUMBER,FVector::ForwardVector);
    FVector Right,Up; Axis.FindBestAxisVectors(Right,Up);
    const float Radius=FMath::Sqrt(U)*FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(Degrees,0.f,15.f)));
    const float Angle=2.f*PI*V;
    return (Axis+Right*(Radius*FMath::Cos(Angle))+Up*(Radius*FMath::Sin(Angle))).GetSafeNormal();
}
