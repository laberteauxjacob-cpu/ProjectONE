#pragma once
#include "CoreMinimal.h"
#include "ONEWeaponTypes.h"

namespace ONE06Ballistics
{
    float DamageAtDistance(const FONEWeaponDefinition& Definition,float Travel,int32 PreviousLivingBodies);
    bool CanDamageBody(const FONEWeaponDefinition& Definition,float Travel,int32 PreviousLivingBodies);
    FVector SampleDirection(FRandomStream& Stream,const FVector& Intent,float SpreadDegrees);
    constexpr int32 MaximumContactsPerProjectile=16;
}
