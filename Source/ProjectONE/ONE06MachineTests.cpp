#include "Misc/AutomationTest.h"
#include "ONE06MachineRules.h"
#include "ONEProgressionMachine.h"
#include "ONE04MachinePresentation.h"
#include "ONEPlayer.h"
#include "ONEHealthComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06ReadyDeadlineTest,"ProjectONE.Candidate06.Machines.ReadyBoundaryOrdering",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06ReadyDeadlineTest::RunTest(const FString&)
{
    using namespace ONE06MachineRules;
    TestTrue(TEXT("Standing in reach when ready returns immediately"),ResolveReady(0,15,true)==EReadyResolution::Collect);
    TestTrue(TEXT("Absent owner keeps reservation before deadline"),ResolveReady(14.999,15,false)==EReadyResolution::Wait);
    TestTrue(TEXT("Reachable owner wins exact fifteen-second boundary"),ResolveReady(15,15,true)==EReadyResolution::Collect);
    TestTrue(TEXT("Absent owner expires at exact boundary"),ResolveReady(15,15,false)==EReadyResolution::Expire);
    TestTrue(TEXT("A late approach cannot rescue expired ownership"),ResolveReady(15.001,15,true)==EReadyResolution::Expire);
    TestTrue(TEXT("Frame hitch preserves the deadline instead of adding time"),ResolveReady(15.32,15,true)==EReadyResolution::Expire);
    TestTrue(TEXT("Editable deadline uses the same equality policy"),ResolveReady(7,7,true)==EReadyResolution::Collect);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06MachineApproachTest,"ProjectONE.Candidate06.Machines.RelocatedApproachesAndWalls",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06MachineApproachTest::RunTest(const FString&)
{
    const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
    if (!TestNotNull(TEXT("Independent machine reach world"),World)) return false;
    FActorSpawnParameters Spawn; Spawn.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    USkeletalMesh* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Response.SK_Response"));
    Spawn.CustomPreSpawnInitalization=[Mesh](AActor* A)
    { if (auto* P=Cast<AONEPlayer>(A)) P->GetMesh()->SetSkeletalMesh(Mesh); };
    auto* P=World->SpawnActor<AONEPlayer>(FVector::ZeroVector,FRotator::ZeroRotator,Spawn);
    auto* M=World->SpawnActor<AONEUpgradeMachine>(FVector(870,-430,110),FRotator(0,63,0),Spawn);
    bool Valid=P && M;
    if (Valid)
    {
        P->GetHealthComponent()->Restore();
        // No arena, actor-name lookup or BeginPlay is needed for the reach
        // contract. Rotated component transforms and actual collision apply.
        const FTransform Frame=M->GetPresentation()->GetComponentTransform();
        for (const FVector Local:{FVector(235,0,98),FVector(60,170,98),FVector(60,-170,98),FVector(160,130,98)})
        {
            P->SetActorLocation(Frame.TransformPosition(Local));
            TestTrue(FString::Printf(TEXT("Front/side approach %s reaches relocated rotated machine"),*Local.ToString()),M->CanContact(P));
        }
        P->SetActorLocation(Frame.TransformPosition(FVector(60,201,98)));
        TestFalse(TEXT("Beyond editable two-meter radius is rejected"),M->CanReach(P));
        P->SetActorLocation(Frame.TransformPosition(FVector(-100,0,98)));
        TestFalse(TEXT("Rear approach does not reach through the machine"),M->CanReach(P));
        P->SetActorLocation(Frame.TransformPosition(FVector(235,0,98)));
        AActor* Wall=World->SpawnActor<AActor>();
        UBoxComponent* Shape=NewObject<UBoxComponent>(Wall); Wall->SetRootComponent(Shape);
        Shape->SetBoxExtent(FVector(4,95,140)); Shape->SetCollisionObjectType(ECC_WorldStatic);
        Shape->SetCollisionEnabled(ECollisionEnabled::QueryOnly); Shape->SetCollisionResponseToAllChannels(ECR_Block);
        Shape->RegisterComponent(); Wall->SetActorLocation(Frame.TransformPosition(FVector(190,0,105)));
        Wall->SetActorRotation(M->GetActorRotation());
        TestFalse(TEXT("Real intervening world collision blocks deposit/automatic collection"),M->CanContact(P));
    }
    else TestTrue(TEXT("Machine/player actors created"),false);
    World->DestroyWorld(false);
    return Valid;
}
#endif
