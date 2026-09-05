#pragma once
#include "CoreMinimal.h"
class USkeletalMeshComponent;

namespace ONEPhysicsRuntime
{
    // Actual rigid-body rest history; never an authored pose or actor orientation.
    struct FRestState
    {
        double DisturbedAt=-1,LastSample=-1,WindowStart=-1,NextSupportAttempt=-1;
        TMap<FName,FTransform> WindowPose;
        int32 ManualSleepEvents=0,ExplicitWakeEvents=0,ContactWakeEvents=0;
        int32 FreezeEvents=0,ResumeEvents=0,FrozenBodyCount=0;
        float FreezePositionErrorCm=BIG_NUMBER,FreezeAngleErrorDegrees=BIG_NUMBER;
        float ResumePositionErrorCm=BIG_NUMBER,ResumeAngleErrorDegrees=BIG_NUMBER;
        float StableSeconds=0,MaxLinear=0,MaxAngular=0,PoseDriftCm=0,PoseDriftDegrees=0;
        bool WasSleeping=false,Frozen=false;
    };
    void ResetRest(USkeletalMeshComponent* Mesh,FRestState& State,bool Wake);
    // Call after physics, no more than approximately 10Hz. A supported stable
    // pose may become kinematic; collisions remain enabled. Fresh damage/sever
    // resumes its current physical pose, while contact alone leaves it frozen.
    void UpdateRest(USkeletalMeshComponent* Mesh,FRestState& State);
    struct FStartResult { float PositionErrorCm=BIG_NUMBER,AngleErrorDegrees=BIG_NUMBER,StumpFitErrorCm=BIG_NUMBER; int32 SimulatedBodies=0; };
    FStartResult Start(USkeletalMeshComponent* Mesh,const FVector& InheritedVelocity,const TArray<FName>& MissingRoots);
    // Full-body pelvis supplemental capsule only. Intact: disable. Missing:
    // fit to captured thigh/pelvis pose, enable and return measured center error.
    // No shared asset mutation; detached parts have no pelvis body and skip it.
    float ConfigureLeftStump(USkeletalMeshComponent* Mesh,bool Missing);
    int32 Count(USkeletalMeshComponent* Mesh,bool AwakeOnly=false,FName ChainRoot=NAME_None);
    int32 ExistingChainBodies(USkeletalMeshComponent* Mesh,FName ChainRoot);
}
