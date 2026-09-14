#include "ONE07RegionalTrace.h"
#include "ONEZombie.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include <cmath>

namespace ONE07RegionalTrace
{
    namespace Details
    {
        // Relative to centimeters: this tie tolerance is one micrometer. It
        // never enlarges geometry or changes the ray/intersection calculation.
        constexpr double HitTieCm=.0001;
        template<typename F> void Roots(double A,double HalfB,double C,F&& Accept)
        {
            if (A<=0.) return;
            const double Discriminant=std::fma(-A,C,HalfB*HalfB);
            if (Discriminant<0.) return;
            const double Root=std::sqrt(Discriminant);
            const double Q=-HalfB-std::copysign(Root,HalfB);
            if (Q==0.) { Accept(-HalfB/A); return; }
            const double T0=Q/A,T1=C/Q;
            Accept(FMath::Min(T0,T1)); Accept(FMath::Max(T0,T1));
        }
        bool Present(UCapsuleComponent* Capsule)
        {
            const auto* Zombie=Cast<AONEZombie>(Capsule->GetOwner());
            if (!Zombie) return true;
            const FHitResult Identity(Capsule->GetOwner(),Capsule,FVector::ZeroVector,FVector::UpVector);
            return Zombie->GetHitRegion(Identity)!=EONEHitRegion::Invalid;
        }
    }
    bool IntersectCapsuleSegment(const FVector& Start,const FVector& End,const FVector& CoreA,
        const FVector& CoreB,double Radius,FCapsuleContact& Out)
    {
        Out={};
        if (Start.ContainsNaN() || End.ContainsNaN() || CoreA.ContainsNaN() || CoreB.ContainsNaN() ||
            !FMath::IsFinite(Radius) || Radius<=0.) return false;
        const FVector Delta=End-Start,Core=CoreB-CoreA;
        const double Length2=Delta.SizeSquared(),CoreLength=Core.Size();
        if (!FMath::IsFinite(Length2) || Length2<=0.) return false;
        const FVector Axis=CoreLength>0.?Core/CoreLength:FVector::UpVector;
        const FVector Origin=Start-CoreA;
        const double Along=FVector::DotProduct(Origin,Axis),DirectionAlong=FVector::DotProduct(Delta,Axis);
        const FVector ClosestAtStart=CoreA+Axis*FMath::Clamp(Along,0.,CoreLength);
        const double Distance2=(Start-ClosestAtStart).SizeSquared();
        if (Distance2<=Radius*Radius)
        {
            Out.bStartInside=true; Out.Position=Start;
            Out.Normal=(Start-ClosestAtStart).GetSafeNormal();
            if (Out.Normal.IsNearlyZero()) Out.Normal=-Delta.GetSafeNormal();
            Out.PenetrationDepth=Radius-std::sqrt(FMath::Max(0.,Distance2));
            return true;
        }
        double Best=2.;
        auto Accept=[&Best](double Time) { if (FMath::IsFinite(Time) && Time>=0. && Time<=1.) Best=FMath::Min(Best,Time); };
        if (CoreLength>0.)
        {
            // Use perpendicular vectors directly. Subtracting axial squares
            // from total squares caused the confirmed Chaos side-ray failure.
            const FVector RadialOrigin=Origin-Axis*Along,RadialDelta=Delta-Axis*DirectionAlong;
            Details::Roots(RadialDelta.SizeSquared(),FVector::DotProduct(RadialOrigin,RadialDelta),
                RadialOrigin.SizeSquared()-Radius*Radius,[&](double Time)
                { const double Axial=Along+Time*DirectionAlong; if (Axial>=0. && Axial<=CoreLength) Accept(Time); });
        }
        for (const FVector& Center:{CoreA,CoreB})
        {
            const FVector Offset=Start-Center;
            Details::Roots(Length2,FVector::DotProduct(Offset,Delta),Offset.SizeSquared()-Radius*Radius,Accept);
        }
        if (Best>1.) return false;
        Out.Time=Best; Out.Position=Start+Delta*Best;
        const FVector Closest=CoreA+Axis*FMath::Clamp(FVector::DotProduct(Out.Position-CoreA,Axis),0.,CoreLength);
        Out.Normal=(Out.Position-Closest).GetSafeNormal();
        return true;
    }
    bool TraceCapsule(UCapsuleComponent* Capsule,FHitResult& Out,const FVector& Start,const FVector& End,
        const FCollisionQueryParams& Params)
    {
        Out=FHitResult(Start,End);
        if (!IsValid(Capsule) || !Capsule->IsRegistered() || !Capsule->IsQueryCollisionEnabled() ||
            !Capsule->GetOwner() || !Capsule->GetOwner()->GetActorEnableCollision() || Params.bIgnoreBlocks ||
            Capsule->GetCollisionResponseToChannel(ECC_Visibility)!=ECR_Block || !Details::Present(Capsule) ||
            Params.GetIgnoredSourceObjects().Contains(Capsule->GetOwner()->GetUniqueID()) ||
            Params.GetIgnoredComponents().Contains(Capsule->GetUniqueID())) return false;
        if ((Params.MobilityType==EQueryMobilityType::Static && Capsule->GetMobility()!=EComponentMobility::Static) ||
            (Params.MobilityType==EQueryMobilityType::Dynamic && Capsule->GetMobility()==EComponentMobility::Static)) return false;
        const FBodyInstance* Body=Capsule->GetBodyInstance();
        if (!Body || !Body->IsValidBodyInstance() || (Params.IgnoreMask & Body->GetMaskFilter())!=0) return false;
        const double Radius=Capsule->GetScaledCapsuleRadius(),HalfHeight=Capsule->GetScaledCapsuleHalfHeight();
        if (Radius<=0. || HalfHeight<Radius) return false; // No invented replacement for invalid geometry.
        const FVector Center=Capsule->GetComponentLocation(),Axis=Capsule->GetUpVector();
        const FVector Delta=End-Start; const double Length2=Delta.SizeSquared();
        if (Length2<=0. || !FMath::IsFinite(Length2)) return false;
        const FVector Near=Start+Delta*FMath::Clamp(FVector::DotProduct(Center-Start,Delta)/Length2,0.,1.);
        if (FVector::DistSquared(Near,Center)>HalfHeight*HalfHeight) return false; // Conservative rejection only.
        const FVector CoreOffset=Axis*(HalfHeight-Radius);
        FCapsuleContact Contact;
        if (!IntersectCapsuleSegment(Start,End,Center-CoreOffset,Center+CoreOffset,Radius,Contact)) return false;
        Out=FHitResult(Capsule->GetOwner(),Capsule,Contact.Position,Contact.Normal);
        Out.bBlockingHit=true; Out.bStartPenetrating=Contact.bStartInside;
        Out.TraceStart=Start; Out.TraceEnd=End; Out.Time=float(Contact.Time); Out.Distance=float(std::sqrt(Length2)*Contact.Time);
        Out.Location=Out.ImpactPoint=Contact.Position; Out.Normal=Out.ImpactNormal=Contact.Normal;
        Out.PenetrationDepth=float(Contact.PenetrationDepth);
        // Match the existing primitive query's NAME_None: weapon damage chooses
        // the nearest evaluated regional bone after the unchanged region hit.
        if (Params.bReturnPhysicalMaterial) Out.PhysMaterial=Body->GetSimplePhysicalMaterial();
        return true;
    }
    bool TraceWeaponSegment(UWorld* World,FHitResult& Out,const FVector& Start,const FVector& End,
        const FCollisionQueryParams& Params)
    {
        Out=FHitResult(Start,End); if (!World) return false;
        const double Length=FVector::Dist(Start,End);
        if (!FMath::IsFinite(Length) || Length<=0.) return false;
        const FVector Direction=(End-Start)/Length;
        FHitResult Regional; bool HasRegional=false;
        double RegionalDistance=0.;
        FCollisionQueryParams WorldParams=Params;
        for (TActorIterator<AONEZombie> It(World);It;++It)
        {
            if (!IsValid(*It) || !It->HasActorBegunPlay() || !It->GetActorEnableCollision() ||
                Params.GetIgnoredSourceObjects().Contains(It->GetUniqueID())) continue;
            for (UCapsuleComponent* Capsule:{It->HeadRegion.Get(),It->BodyRegion.Get(),It->UpperArmLeftRegion.Get(),It->ArmLeftRegion.Get(),
                It->UpperArmRightRegion.Get(),It->ArmRightRegion.Get(),It->UpperLegLeftRegion.Get(),It->LegLeftRegion.Get(),
                It->UpperLegRightRegion.Get(),It->LegRightRegion.Get()})
            {
                if (!IsValid(Capsule)) continue;
                if (!Details::Present(Capsule)) { WorldParams.AddIgnoredComponent(Capsule); continue; }
                FHitResult Candidate;
                if (!TraceCapsule(Capsule,Candidate,Start,End,Params)) continue;
                const double CandidateDistance=FVector::DotProduct(FVector(Candidate.ImpactPoint)-Start,Direction);
                if (!HasRegional || CandidateDistance<RegionalDistance-Details::HitTieCm ||
                    (FMath::Abs(CandidateDistance-RegionalDistance)<=Details::HitTieCm &&
                        Capsule->GetUniqueID()<Regional.GetComponent()->GetUniqueID()))
                { Regional=Candidate; RegionalDistance=CandidateDistance; HasRegional=true; }
            }
        }
        const bool HasWorld=World->LineTraceSingleByChannel(Out,Start,End,ECC_Visibility,WorldParams);
        const double WorldDistance=HasWorld?FVector::DotProduct(FVector(Out.ImpactPoint)-Start,Direction):0.;
        if (HasRegional && (!HasWorld || RegionalDistance<WorldDistance-Details::HitTieCm)) { Out=Regional; return true; }
        return HasWorld;
    }
}
