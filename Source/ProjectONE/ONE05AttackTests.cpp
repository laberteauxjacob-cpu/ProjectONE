#include "Misc/AutomationTest.h"
#include "ONE05AttackMotion.h"
#include "ONEZombie.h"
#include "ONEPlayer.h"
#include "ONEHealthComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/SkeletalMesh.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    struct FAttackWorld
    {
        UWorld* World=nullptr;
        AONEZombie* Zombie=nullptr;
        AONEPlayer* Player=nullptr;
        FAttackWorld()
        {
            // Unlike the older inventory fixture, this world advances real
            // engine time. LevelTick's XR/frame callbacks require a registered
            // engine world context even though actors never enter BeginPlay.
            FWorldContext& Context=GEngine->CreateNewWorldContext(EWorldType::Game);
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
            Context.SetCurrentWorld(World);
            FActorSpawnParameters P; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            USkeletalMesh* InfectedMesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/Candidate03/SK_Infected_Core.SK_Infected_Core"));
            USkeletalMesh* ResponseMesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/SK_Response.SK_Response"));
            P.CustomPreSpawnInitalization=[InfectedMesh,ResponseMesh](AActor* Actor)
            {
                if (auto* Character=Cast<ACharacter>(Actor))
                {
                    // Install accepted reference bones before child sockets
                    // register, but do not start the gameplay animation graph
                    // or load the not-yet-imported C05 animation bank.
                    auto* Mesh=Character->GetMesh();
                    Mesh->SetAnimInstanceClass(nullptr);
                    Mesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                    Mesh->SetSkeletalMesh(Cast<AONEZombie>(Actor)?InfectedMesh:ResponseMesh);
                }
            };
            Zombie=World->SpawnActor<AONEZombie>(FVector(0,0,100),FRotator::ZeroRotator,P);
            Player=World->SpawnActor<AONEPlayer>(FVector(80,0,100),FRotator::ZeroRotator,P);
            Zombie->GetHealthComponent()->Restore(); Player->GetHealthComponent()->Restore();
            for (AActor* Actor:{static_cast<AActor*>(Zombie),static_cast<AActor*>(Player)})
            {
                Actor->SetActorTickEnabled(false);
                TInlineComponentArray<UActorComponent*> Components(Actor);
                for (auto* Component:Components) Component->SetComponentTickEnabled(false);
            }
            // No BeginPlay: audio voices, pursuit and the weapon graph remain
            // inactive. Time/physics advance below; only the attack actor's
            // production Tick is explicitly invoked once per fixed timestep.
        }
        ~FAttackWorld()
        {
            if (World)
            {
                // Keep context valid throughout component/physics teardown.
                GEngine->ShutdownWorldNetDriver(World);
                World->DestroyWorld(false);
                World->SetPhysicsScene(nullptr);
                GEngine->DestroyWorldContext(World);
                World=nullptr;
            }
        }
        void Advance(float Seconds)
        {
            for (int32 I=0;I<FMath::CeilToInt(Seconds*120.f);++I)
            { World->Tick(LEVELTICK_All,1.f/120.f); Zombie->Tick(1.f/120.f); }
        }
        void Wall()
        {
            AActor* A=World->SpawnActor<AActor>();
            UBoxComponent* Box=NewObject<UBoxComponent>(A); A->SetRootComponent(Box);
            Box->SetBoxExtent(FVector(3,80,100)); Box->SetCollisionObjectType(ECC_WorldStatic);
            Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Box->SetCollisionResponseToAllChannels(ECR_Block);
            Box->RegisterComponent(); A->SetActorLocation(FVector(40,0,100));
        }
    };
    FONEWeaponDamagePacket Packet(uint64 Id,EONEHitRegion Region,float Damage,float Trauma=0.f)
    {
        FONEWeaponDamagePacket P; P.ShotId=Id;
        P.Get(Region).AddPellet(Damage,Trauma,FVector(0,0,130),FVector::ForwardVector,-FVector::ForwardVector,NAME_None);
        P.Finalize(); return P;
    }
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05AttackFamilyTest,"ProjectONE.Candidate05.Attacks.ThreeFamiliesOneContact",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05AttackFamilyTest::RunTest(const FString&)
{
    for (int32 Family=0;Family<3;++Family)
    {
        FAttackWorld F;
        TestTrue(TEXT("Eligible family enters real windup"),F.Zombie->TryStartAttack(F.Player,Family));
        TestEqual(TEXT("Selected family retained"),F.Zombie->GetAttackFamily(),Family);
        const float Initial=F.Player->GetHealth(),Contact=F.Zombie->GetCurrentAttackContactTime();
        F.Advance(Contact-.03f);
        TestEqual(TEXT("No damage before authored contact"),F.Player->GetHealth(),Initial);
        F.Advance(.07f);
        TestEqual(TEXT("Contact dispatches exactly19 damage"),F.Player->GetHealth(),Initial-19.f);
        F.Advance(.15f);
        TestEqual(TEXT("Follow-through cannot damage again"),F.Zombie->GetAttackDamageDispatchCount(),1);
        TestEqual(TEXT("Contact event consumed once"),F.Zombie->GetAttackContactAttemptCount(),1);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05AttackCancellationTest,"ProjectONE.Candidate05.Attacks.WallHeadingLimbAndDeathCancel",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05AttackCancellationTest::RunTest(const FString&)
{
    {
        FAttackWorld F; F.Wall(); F.Zombie->TryStartAttack(F.Player,0); F.Advance(.55f);
        TestEqual(TEXT("World-static cover rejects contact"),F.Zombie->GetAttackDamageDispatchCount(),0);
        TestTrue(TEXT("Blocked contact still consumed"),F.Zombie->IsAttackContactConsumed());
    }
    {
        FAttackWorld F; F.Zombie->TryStartAttack(F.Player,1); F.Player->SetActorLocation(FVector(0,80,100)); F.Advance(.55f);
        TestEqual(TEXT("Moving out of committed arc misses"),F.Zombie->GetAttackDamageDispatchCount(),0);
        TestTrue(TEXT("Heading remains fixed after dodge"),F.Zombie->GetActorForwardVector().Equals(FVector::ForwardVector,.001));
    }
    {
        FAttackWorld F; F.Zombie->TryStartAttack(F.Player,0);
        TestEqual(TEXT("First swipe requires anatomical left"),F.Zombie->GetAttackClipKey(),FName(TEXT("C05_SwipeLeft")));
        F.Zombie->ReceiveWeaponDamage(Packet(5001,EONEHitRegion::ArmLeft,1,50)); F.Advance(.55f);
        TestFalse(TEXT("Required arm was severed"),F.Zombie->HasLeftArm());
        TestEqual(TEXT("Missing arm cancels scheduled damage"),F.Zombie->GetAttackDamageDispatchCount(),0);
        TestTrue(TEXT("Minor hit does not replace attack state"),F.Zombie->GetCombatState()==EONEZombieState::Attack);
    }
    {
        FAttackWorld F; F.Zombie->TryStartAttack(F.Player,2); F.Zombie->ReceiveWeaponDamage(Packet(5002,EONEHitRegion::Body,500)); F.Advance(.6f);
        TestTrue(TEXT("Lethal packet ends attack"),F.Zombie->IsDead());
        TestEqual(TEXT("Death cancels pending contact"),F.Zombie->GetAttackDamageDispatchCount(),0);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05ReactionOutcomeTest,"ProjectONE.Candidate05.Feedback.LiveKillCorpseAndMinorContinuity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05ReactionOutcomeTest::RunTest(const FString&)
{
    FAttackWorld F;
    F.Zombie->TryStartAttack(F.Player,0);
    for (int32 I=0;I<5;++I)
    {
        TestTrue(TEXT("Each live transaction classified live"),F.Zombie->ReceiveWeaponDamageOutcome(Packet(5100+I,EONEHitRegion::Body,1))==EONEWeaponHitOutcome::LiveHit);
        TestTrue(TEXT("Minor burst does not restart state"),F.Zombie->GetCombatState()==EONEZombieState::Attack);
        F.Advance(.05f);
    }
    TestTrue(TEXT("Reaction has nonzero strength"),F.Zombie->GetMinorReactionStrength()>0.f);
    TestTrue(TEXT("Duplicate is rejected"),F.Zombie->ReceiveWeaponDamageOutcome(Packet(5104,EONEHitRegion::Body,1))==EONEWeaponHitOutcome::Rejected);
    TestTrue(TEXT("Lethal live packet is a new kill"),F.Zombie->ReceiveWeaponDamageOutcome(Packet(5200,EONEHitRegion::Body,500))==EONEWeaponHitOutcome::NewKill);
    TestTrue(TEXT("Later corpse packet is cosmetic, not another kill"),F.Zombie->ReceiveWeaponDamageOutcome(Packet(5201,EONEHitRegion::Body,10))==EONEWeaponHitOutcome::CorpseHit);
    TestEqual(TEXT("Corpse transaction counted separately"),F.Zombie->GetCorpseTransactionCount(),1);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE05AttackBudgetTest,"ProjectONE.Candidate05.Attacks.BoundedStepAndLimbMasks",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE05AttackBudgetTest::RunTest(const FString&)
{
    for (int32 Family=0;Family<3;++Family)
    {
        const auto P=ONE05AttackMotion::Profile(Family); float Distance=0;
        for (int32 I=0;I<10000;++I) Distance+=ONE05AttackMotion::StepSpeed(Family,(I+.5f)*P.StepEnd/10000.f)*P.StepEnd/10000.f;
        TestTrue(TEXT("Integrated step stays within authored budget"),FMath::IsNearlyEqual(Distance,P.StepDistance,.003f));
        TestTrue(TEXT("Step completes before damaging contact"),P.StepEnd<P.Contact);
        TestEqual(TEXT("No movement during recovery"),ONE05AttackMotion::StepSpeed(Family,P.Contact),0.f);
    }
    TestFalse(TEXT("Two-hand contact requires both arms"),ONE05AttackMotion::ArmsAvailable(3,true,false));
    TestTrue(TEXT("Left swipe remains valid with right absent"),ONE05AttackMotion::ArmsAvailable(1,true,false));
    TestFalse(TEXT("Missing active arm cannot substitute another"),ONE05AttackMotion::ArmsAvailable(1,false,true));
    return true;
}
#endif
