#include "Misc/AutomationTest.h"
#include "ONE06Ballistics.h"
#include "ONEWeaponCatalog.h"
#include "ONEHealthComponent.h"
#include "ONEZombie.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Components/SkeletalMeshComponent.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
namespace
{
    // Production damage and death code in a registered engine world. No
    // BeginPlay, player input, traces, AI, animation graph or media claim.
    struct FCombatWorld
    {
        UWorld* World=nullptr;
        USkeletalMesh* Mesh=nullptr;
        UPhysicsAsset* Physics=nullptr;
        int32 SpawnCount=0;
        FCombatWorld()
        {
            if (!GEngine) return;
            Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ONE/Characters/Candidate03/SK_Infected_Core.SK_Infected_Core"));
            Physics=LoadObject<UPhysicsAsset>(nullptr,TEXT("/Game/ONE/Characters/Candidate03/PA_Infected_C03.PA_Infected_C03"));
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
            if (World) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        }
        ~FCombatWorld()
        {
            if (!World) return;
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(false);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
        }
        AONEZombie* Spawn()
        {
            if (!World || !Mesh || !Physics) return nullptr;
            FActorSpawnParameters P; P.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            P.CustomPreSpawnInitalization=[this](AActor* Actor)
            {
                auto* Z=CastChecked<AONEZombie>(Actor);
                Z->GetMesh()->SetAnimInstanceClass(nullptr);
                Z->GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
                Z->GetMesh()->SetSkeletalMesh(Mesh);
                Z->GetMesh()->SetPhysicsAsset(Physics);
            };
            auto* Z=World->SpawnActor<AONEZombie>(FVector(500.f*SpawnCount++,0,100),FRotator::ZeroRotator,P);
            if (Z)
            {
                Z->GetHealthComponent()->Restore(); Z->SetActorTickEnabled(false);
                TInlineComponentArray<UActorComponent*> Components(Z);
                for (auto* Component:Components) Component->SetComponentTickEnabled(false);
            }
            return Z;
        }
    };
    void AddHit(FONEWeaponDamagePacket& Packet,const AONEZombie* Z,EONEHitRegion Region,float Damage,float Trauma=0.f)
    {
        const FName Bone=Region==EONEHitRegion::Head ? FName(TEXT("head")) : FName(TEXT("spine_01"));
        Packet.Get(Region).AddPellet(Damage,Trauma,Z->GetMesh()->GetSocketLocation(Bone),
            FVector::ForwardVector,-FVector::ForwardVector,Bone);
    }
    FONEWeaponDamagePacket SingleHit(uint64 Id,const AONEZombie* Z,EONEHitRegion Region,float Damage,float Trauma=0.f)
    {
        FONEWeaponDamagePacket Packet; Packet.ShotId=Id;
        AddHit(Packet,Z,Region,Damage,Trauma); Packet.Finalize(); return Packet;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06BallisticProfilesTest,"ProjectONE.Candidate06.Ballistics.SixProfilesOriginalDistanceAndBodyBudgets",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06BallisticProfilesTest::RunTest(const FString&)
{
    const auto Definitions=ONEWeaponCatalog::BuildDefaults();
    if (!TestEqual(TEXT("Three base and three effective upgrade rows"),Definitions.Num(),6)) return false;
    const int32 Limits[]={3,2,2,4,3,3};
    const float Retained[]={.65f,.60f,.60f,.70f,.65f,.65f};
    const float Damage[]={32.f,15.f,28.f,64.f,30.f,56.f};
    const float ExtraRange[]={0.f,650.f,0.f,0.f,800.f,0.f};
    for (int32 I=0;I<Definitions.Num();++I)
    {
        const auto& D=Definitions[I]; const FString Label=D.Id.ToString()+TEXT(": ");
        TestEqual(Label+TEXT("configured body budget"),D.Penetration.MaximumBodies,Limits[I]);
        TestTrue(Label+TEXT("configured retention"),FMath::IsNearlyEqual(D.Penetration.DamageRetainedPerBody,Retained[I],1.e-6f));
        TestEqual(Label+TEXT("configured base damage"),D.Damage,Damage[I]);
        TestEqual(Label+TEXT("configured additional-body range"),D.Penetration.AdditionalBodyRange,ExtraRange[I]);
        TestEqual(Label+TEXT("first close victim receives full damage"),ONE06Ballistics::DamageAtDistance(D,100.f,0),Damage[I]);
        TestEqual(Label+TEXT("falloff starts without an immediate damage step"),ONE06Ballistics::DamageAtDistance(D,D.FalloffStart,0),Damage[I]);
        TestFalse(Label+TEXT("negative body index is invalid"),ONE06Ballistics::CanDamageBody(D,100.f,-1));
        for (int32 Body=0;Body<Limits[I];++Body)
        {
            TestTrue(Label+FString::Printf(TEXT("distinct close victim %d fits the budget"),Body+1),ONE06Ballistics::CanDamageBody(D,100.f,Body));
            TestTrue(Label+FString::Printf(TEXT("victim %d retains damage once per prior body"),Body+1),
                FMath::IsNearlyEqual(ONE06Ballistics::DamageAtDistance(D,100.f,Body),Damage[I]*FMath::Pow(Retained[I],Body),.0001f));
        }
        TestFalse(Label+TEXT("next living victim exceeds the total-body budget"),ONE06Ballistics::CanDamageBody(D,100.f,Limits[I]));
        const float OriginalTravel=D.FalloffStart+(D.Range-D.FalloffStart)*.6f;
        const float Expected=Damage[I]*(.4f+.6f*D.MinimumDamageFraction)*Retained[I];
        TestTrue(Label+TEXT("downstream damage includes original ray distance and retention"),
            FMath::IsNearlyEqual(ONE06Ballistics::DamageAtDistance(D,OriginalTravel,1),Expected,.0001f));
        TestTrue(Label+TEXT("a new trace segment must not restore close-range damage"),
            ONE06Ballistics::DamageAtDistance(D,OriginalTravel,1)<ONE06Ballistics::DamageAtDistance(D,100.f,1));
        TestTrue(Label+TEXT("range endpoint has the configured minimum fraction"),
            FMath::IsNearlyEqual(ONE06Ballistics::DamageAtDistance(D,D.Range,1),Damage[I]*D.MinimumDamageFraction*Retained[I],.0001f));
        TestFalse(Label+TEXT("travel beyond original range cannot hit another victim"),ONE06Ballistics::CanDamageBody(D,D.Range+.1f,0));
        TestEqual(Label+TEXT("past-range damage is zero"),ONE06Ballistics::DamageAtDistance(D,D.Range+.1f,0),0.f);
        TestFalse(Label+TEXT("negative travel is invalid"),ONE06Ballistics::CanDamageBody(D,-1.f,0));
        TestFalse(Label+TEXT("nonfinite travel is invalid"),ONE06Ballistics::CanDamageBody(D,std::numeric_limits<float>::quiet_NaN(),0));
        FONEWeaponDefinition Threshold=D;
        Threshold.Penetration.MinimumDamage=ONE06Ballistics::DamageAtDistance(D,100.f,1);
        TestTrue(Label+TEXT("minimum damage is inclusive"),ONE06Ballistics::CanDamageBody(Threshold,100.f,1));
        Threshold.Penetration.MinimumDamage+=.01f;
        TestFalse(Label+TEXT("below-minimum penetration terminates"),ONE06Ballistics::CanDamageBody(Threshold,100.f,1));
        if (ExtraRange[I]>0.f)
        {
            TestTrue(Label+TEXT("additional-body distance limit is inclusive"),ONE06Ballistics::CanDamageBody(D,ExtraRange[I],1));
            TestFalse(Label+TEXT("additional body beyond its shorter range is rejected"),ONE06Ballistics::CanDamageBody(D,ExtraRange[I]+.1f,1));
            TestTrue(Label+TEXT("first victim still uses ordinary shotgun range"),ONE06Ballistics::CanDamageBody(D,ExtraRange[I]+.1f,0));
        }
    }
    FONEWeaponDefinition Safety=Definitions[0]; Safety.Penetration.MaximumBodies=99;
    Safety.Penetration.DamageRetainedPerBody=1.f;
    TestTrue(TEXT("Even an edited profile permits at most five living bodies: fifth valid"),ONE06Ballistics::CanDamageBody(Safety,100.f,4));
    TestFalse(TEXT("Even an edited profile cannot reach a sixth living body"),ONE06Ballistics::CanDamageBody(Safety,100.f,5));
    TestEqual(TEXT("Per-projectile contact traversal is independently bounded"),ONE06Ballistics::MaximumContactsPerProjectile,16);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06SpreadStreamTest,"ProjectONE.Candidate06.Ballistics.PrivateSpreadStreamIgnoresGlobalRandomNoise",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06SpreadStreamTest::RunTest(const FString&)
{
    const FVector Axis=FVector(3,-5,2).GetSafeNormal();
    for (const auto& D:ONEWeaponCatalog::BuildDefaults())
    {
        FRandomStream Reference(606210),Noisy(606210);
        const FString Label=D.Id.ToString()+TEXT(": ");
        float NoiseSum=0.f; int32 LastNoise=0;
        for (int32 I=0;I<64;++I)
        {
            const FVector A=ONE06Ballistics::SampleDirection(Reference,Axis,D.MaximumSpreadDegrees);
            for (int32 Noise=0;Noise<(I%9)+1;++Noise) { NoiseSum+=FMath::FRand(); LastNoise=FMath::Rand(); }
            const FVector B=ONE06Ballistics::SampleDirection(Noisy,Axis,D.MaximumSpreadDegrees);
            if (!TestTrue(Label+TEXT("cosmetic/global RNG calls cannot move the ballistic ray"),A.Equals(B,1.e-12))) break;
            TestTrue(Label+TEXT("sample is normalized"),FMath::IsNearlyEqual(B.SizeSquared(),1.,1.e-6));
            TestTrue(Label+TEXT("sample stays inside the maximum spread cone"),
                FVector::DotProduct(Axis,B)>=FMath::Cos(FMath::DegreesToRadians(D.MaximumSpreadDegrees))-1.e-6);
        }
        TestEqual(Label+TEXT("noise leaves the private stream at the same state"),Reference.GetCurrentSeed(),Noisy.GetCurrentSeed());
        TestTrue(Label+TEXT("global noise samples were consumed"),FMath::IsFinite(NoiseSum) && NoiseSum>=0.f && LastNoise>=0);
    }
    FRandomStream Zero(606211),Manual(606211);
    TestTrue(TEXT("zero spread returns exact intent"),ONE06Ballistics::SampleDirection(Zero,Axis,0.f).Equals(Axis,1.e-12));
    const float ManualU=Manual.FRand(),ManualV=Manual.FRand();
    TestTrue(TEXT("manual stream samples are valid"),ManualU>=0.f && ManualU<1.f && ManualV>=0.f && ManualV<1.f);
    TestEqual(TEXT("zero-spread comparison still consumes exactly two private samples"),Zero.GetCurrentSeed(),Manual.GetCurrentSeed());
    TestTrue(TEXT("later nonzero samples remain aligned after zero spread"),
        ONE06Ballistics::SampleDirection(Zero,Axis,4.f).Equals(ONE06Ballistics::SampleDirection(Manual,Axis,4.f),1.e-12));
    FRandomStream Degenerate(606212);
    TestTrue(TEXT("zero intent has a finite forward fallback"),
        ONE06Ballistics::SampleDirection(Degenerate,FVector::ZeroVector,0.f).Equals(FVector::ForwardVector,1.e-12));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06FirstHeadDamageTest,"ProjectONE.Candidate06.Combat.BaseHeadDamageCannotBypassHealth",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06FirstHeadDamageTest::RunTest(const FString&)
{
    FCombatWorld F; const auto Definitions=ONEWeaponCatalog::BuildDefaults();
    for (int32 Index:{2,0})
    {
        auto* Z=F.Spawn(); if (!TestNotNull(TEXT("Real infected with accepted skeleton and physics fixture"),Z)) return false;
        const auto& D=Definitions[Index]; const FString Label=D.Id.ToString()+TEXT(": ");
        TestEqual(Label+TEXT("ordinary maximum health is 112"),Z->GetHealth(),112.f);
        TestEqual(Label+TEXT("head multiplier is 1.5"),Z->HeadDamageMultiplier,1.5f);
        const auto Packet=SingleHit(606100+Index,Z,EONEHitRegion::Head,D.Damage,D.Damage*D.HeadTraumaScale);
        TestTrue(Label+TEXT("probe actually exceeds the former trauma sever threshold"),Packet.Get(EONEHitRegion::Head).Trauma>=Z->HeadSeverThreshold);
        TestTrue(Label+TEXT("first base headshot remains a live hit"),Z->ReceiveWeaponDamageOutcome(Packet)==EONEWeaponHitOutcome::LiveHit);
        TestEqual(Label+TEXT("42 pistol / 48 rifle effective head damage"),112.f-Z->GetHealth(),Index==2 ? 42.f : 48.f);
        TestFalse(Label+TEXT("head hit did not kill"),Z->IsDead());
        TestTrue(Label+TEXT("head remains attached despite accumulated trauma"),Z->HasHead());
        TestEqual(Label+TEXT("no sever transaction was emitted"),Z->GetSeverCount(),0);
        TestEqual(Label+TEXT("one health transaction for the entire packet"),Z->GetDamageTransactionCount(),1);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06LethalHeadClassificationTest,"ProjectONE.Candidate06.Combat.LethalDischargeStrictHeadMajority",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06LethalHeadClassificationTest::RunTest(const FString&)
{
    FCombatWorld F; auto* Earlier=F.Spawn();
    if (!TestNotNull(TEXT("Earlier-head fixture exists"),Earlier)) return false;
    TestTrue(TEXT("Earlier pistol headshot is nonlethal"),Earlier->ReceiveWeaponDamageOutcome(SingleHit(606200,Earlier,EONEHitRegion::Head,28.f,56.f))==EONEWeaponHitOutcome::LiveHit);
    TestTrue(TEXT("Later body discharge kills remaining health"),Earlier->ReceiveWeaponDamageOutcome(SingleHit(606201,Earlier,EONEHitRegion::Body,70.f))==EONEWeaponHitOutcome::NewKill);
    TestFalse(TEXT("Earlier head trauma cannot claim a later body-kill bonus"),Earlier->WasLastKillHeadshot());
    const auto Shotgun=ONEWeaponCatalog::BuildDefaults()[1];
    TestEqual(TEXT("Shotgun classification uses one real eight-pellet aggregate"),Shotgun.Pellets,8);
    for (int32 HeadPellets:{1,3,4})
    {
        auto* Z=F.Spawn(); if (!TestNotNull(TEXT("Shotgun classification fixture exists"),Z)) return false;
        FONEWeaponDamagePacket Packet; Packet.ShotId=606210+HeadPellets;
        for (int32 I=0;I<Shotgun.Pellets;++I)
            AddHit(Packet,Z,I<HeadPellets ? EONEHitRegion::Head : EONEHitRegion::Body,Shotgun.Damage,
                I<HeadPellets ? Shotgun.Damage*Shotgun.HeadTraumaScale : 0.f);
        Packet.Finalize();
        TestEqual(TEXT("All eight pellets are retained in one packet"),Packet.GetPellets(),8);
        TestTrue(TEXT("Close-range shell kills through ordinary effective health damage"),Z->ReceiveWeaponDamageOutcome(Packet)==EONEWeaponHitOutcome::NewKill);
        TestEqual(FString::Printf(TEXT("%d head pellets: only an effective head majority qualifies"),HeadPellets),Z->WasLastKillHeadshot(),HeadPellets==4);
        TestEqual(TEXT("A shell changes health once, rather than once per pellet"),Z->GetDamageTransactionCount(),1);
    }
    for (float BodyDamage:{60.f,59.f})
    {
        auto* Z=F.Spawn(); if (!TestNotNull(TEXT("Exact majority boundary fixture exists"),Z)) return false;
        FONEWeaponDamagePacket Packet; Packet.ShotId=BodyDamage==60.f ? 606220 : 606221;
        AddHit(Packet,Z,EONEHitRegion::Head,40.f); AddHit(Packet,Z,EONEHitRegion::Body,BodyDamage); Packet.Finalize();
        TestTrue(TEXT("Boundary aggregate is lethal without forced damage"),Z->ReceiveWeaponDamageOutcome(Packet)==EONEWeaponHitOutcome::NewKill);
        TestEqual(TEXT("Exactly 50% head is false; 60 of 119 effective damage is true"),Z->WasLastKillHeadshot(),BodyDamage<60.f);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE06InstaKillTest,"ProjectONE.Candidate06.Combat.ExplicitInstaKillKeepsOrdinaryImpulseBounds",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE06InstaKillTest::RunTest(const FString&)
{
    FCombatWorld F; auto* Low=F.Spawn();
    if (!TestNotNull(TEXT("Real infected for explicit Insta-Kill"),Low)) return false;
    FONEWeaponDamagePacket Empty; Empty.ShotId=606300; Empty.bForceLethal=true;
    TestTrue(TEXT("Force-lethal flag cannot invent a hit from an empty packet"),Low->ReceiveWeaponDamageOutcome(Empty)==EONEWeaponHitOutcome::Rejected);
    TestEqual(TEXT("Rejected forced packet does not alter health"),Low->GetHealth(),112.f);
    auto Ordinary=SingleHit(606301,Low,EONEHitRegion::Body,1.f);
    TestTrue(TEXT("The same low-damage body contact is ordinarily nonlethal"),Low->ReceiveWeaponDamageOutcome(Ordinary)==EONEWeaponHitOutcome::LiveHit);
    TestEqual(TEXT("Ordinary accepted damage remains one HP"),Low->GetHealth(),111.f);
    auto Forced=SingleHit(606302,Low,EONEHitRegion::Body,1.f); Forced.bForceLethal=true;
    TestTrue(TEXT("Explicit modifier makes a valid one-damage body contact lethal"),Low->ReceiveWeaponDamageOutcome(Forced)==EONEWeaponHitOutcome::NewKill);
    TestTrue(TEXT("Actual actor enters death"),Low->IsDead());
    TestEqual(TEXT("Forced lethal contact removes remaining health"),Low->GetHealth(),0.f);
    TestEqual(TEXT("Insta-Kill does not inflate packet damage"),Forced.GetTotalDamage(),1.f);
    TestEqual(TEXT("Actual death receives the ordinary minimum impulse"),Low->GetLastDeathImpulse(),150.f);
    TestTrue(TEXT("Accepted skeletal physics enters ragdoll"),Low->IsRagdollActive());
    TestTrue(TEXT("Body Insta-Kill preserves the head"),Low->HasHead());
    TestEqual(TEXT("Modifier alone does not invent sever trauma"),Low->GetSeverCount(),0);
    TestFalse(TEXT("Body Insta-Kill cannot receive head qualification"),Low->WasLastKillHeadshot());
    TestTrue(TEXT("Replayed forced packet cannot dispatch death again"),Low->ReceiveWeaponDamageOutcome(Forced)==EONEWeaponHitOutcome::Rejected);
    ++Forced.ShotId;
    TestTrue(TEXT("A fresh forced corpse packet remains cosmetic"),Low->ReceiveWeaponDamageOutcome(Forced)==EONEWeaponHitOutcome::CorpseHit);
    TestEqual(TEXT("Corpse impact cannot replace the recorded death impulse"),Low->GetLastDeathImpulse(),150.f);
    const auto UpgradedShotgun=ONEWeaponCatalog::BuildDefaults()[4];
    for (bool Force:{false,true})
    {
        auto* Z=F.Spawn(); if (!TestNotNull(TEXT("High-damage ordinary/forced comparison fixture"),Z)) return false;
        FONEWeaponDamagePacket Packet; Packet.ShotId=Force?606311:606310; Packet.bForceLethal=Force;
        for (int32 Pellet=0;Pellet<UpgradedShotgun.Pellets;++Pellet) AddHit(Packet,Z,EONEHitRegion::Body,UpgradedShotgun.Damage);
        Packet.Finalize();
        TestTrue(TEXT("Ordinary effective upgraded shell is a lethal contact"),Z->ReceiveWeaponDamageOutcome(Packet)==EONEWeaponHitOutcome::NewKill);
        TestEqual(TEXT("Both ordinary and forced high-damage kills use the same impulse ceiling"),Z->GetLastDeathImpulse(),550.f);
    }
    return true;
}
#endif
