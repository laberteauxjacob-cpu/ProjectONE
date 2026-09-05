#include "ONESnapshotAnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "AnimNodes/AnimNode_PoseSnapshot.h"

struct FONESnapshotProxy : FAnimInstanceProxy
{
    FAnimNode_PoseSnapshot Pose;
    FONESnapshotProxy(UAnimInstance* Instance):FAnimInstanceProxy(Instance) { Pose.Mode=ESnapshotSourceMode::SnapshotPin; }
    virtual FAnimNode_Base* GetCustomRootNode() override { return &Pose; }
    virtual void PreUpdate(UAnimInstance* Instance,float Dt) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Dt);
        if (const auto* Snapshot=Cast<UONESnapshotAnimInstance>(Instance)) Pose.Snapshot=Snapshot->CapturedPose;
        // Native custom-root nodes are not gathered from a generated AnimBP.
        // Cache target bone names explicitly for cross-mesh snapshot mapping.
        Pose.PreUpdate(Instance);
    }
};
UONESnapshotAnimInstance::UONESnapshotAnimInstance() { bUseMultiThreadedAnimationUpdate=false; }
FAnimInstanceProxy* UONESnapshotAnimInstance::CreateAnimInstanceProxy() { return new FONESnapshotProxy(this); }
void UONESnapshotAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete static_cast<FONESnapshotProxy*>(Proxy); }
