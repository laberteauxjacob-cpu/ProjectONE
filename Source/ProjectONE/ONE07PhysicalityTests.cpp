#include "Misc/AutomationTest.h"
#include "ONEZombie.h"
#include "ONEInfectedAnimInstance.h"
#include "ONEInfectedVariant.h"
#include "ONEGameMode.h"
#include "ONEHealthComponent.h"
#include "ONEPowerUpComponent.h"
#include "ONEPowerUpPickup.h"
#include "ONEAim.h"
#include "ONE07RegionalTrace.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "PhysicsEngine/BodyInstance.h"
#include "Physics/PhysicsInterfaceCore.h"
#include "Chaos/Capsule.h"
#include "Chaos/CollisionFilterData.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_DEV_AUTOMATION_TESTS
namespace ONE07PhysicalityTestDetails
{
    // An isolated engine world with actual imported C07 meshes, animation,
    // collision, registered authority and Chaos ticks. The test has no player,
    // navigation, rendered viewport or audio playback. Falls and regional
    // packets are explicit fixture entry points, not claimed weapon traces.
    struct FWorld
    {
        TStrongObjectPtr<UGameInstance> Instance;
        UWorld* World=nullptr;
        AONEGameMode* Mode=nullptr;
        UONEInfectedVariant* Variant=nullptr;
        AActor* Ground=nullptr;
        FWorld()
        {
            if (!GEngine) return;
            Variant=LoadObject<UONEInfectedVariant>(nullptr,TEXT("/Game/ONE/Characters/Candidate07/DA_Infected_Maintenance.DA_Infected_Maintenance"));
            const auto Options=UWorld::InitializationValues().AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false);
            World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Options);
            if (!World) return;
            Instance.Reset(NewObject<UGameInstance>(GEngine));
            auto& Context=GEngine->CreateNewWorldContext(EWorldType::Game);
            Context.SetCurrentWorld(World); Context.OwningGameInstance=Instance.Get();
            World->SetGameInstance(Instance.Get());
            World->GetWorldSettings()->DefaultGameMode=AONEGameMode::StaticClass();
            FURL URL; URL.Map=TEXT("ONE07PhysicalityAutomation"); URL.AddOption(TEXT("ONESandbox=1"));
            if (!World->SetGameMode(URL)) return;
            Mode=World->GetAuthGameMode<AONEGameMode>();
            Ground=Box(FVector(0,0,-10),FVector(2200,2200,10));
            auto* Spawn=World->SpawnActor<ATargetPoint>(FVector(1800,1800,90),FRotator::ZeroRotator);
            if (Spawn) Spawn->Tags.Add(TEXT("ONE_Spawn"));
            World->InitializeActorsForPlay(URL); World->BeginPlay();
            if (Mode)
            {
                Mode->SetActorTickEnabled(false); // No population generation in the isolated scene.
                Mode->GetPowerUps()->TotalDropChance=0.f; // Count eligible deaths without random pickup placement.
            }
        }
        ~FWorld()
        {
            if (!World) return;
            World->EndPlay(EEndPlayReason::Quit);
            GEngine->ShutdownWorldNetDriver(World);
            World->DestroyWorld(false);
            World->SetPhysicsScene(nullptr);
            GEngine->DestroyWorldContext(World);
        }
        AActor* Box(FVector Location,FVector Extent)
        {
            if (!World) return nullptr;
            auto* Actor=World->SpawnActor<AActor>();
            if (!Actor) return nullptr;
            auto* Shape=NewObject<UBoxComponent>(Actor);
            Actor->SetRootComponent(Shape); Actor->AddInstanceComponent(Shape);
            Shape->SetBoxExtent(Extent); Shape->SetMobility(EComponentMobility::Static);
            Shape->SetCollisionObjectType(ECC_WorldStatic);
            Shape->SetCollisionResponseToAllChannels(ECR_Block);
            Shape->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Shape->SetWorldLocation(Location); Shape->RegisterComponent();
            return Actor;
        }
        AONEZombie* Spawn(FVector Position)
        {
            if (!World || !Mode || !Variant) return nullptr;
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Params.CustomPreSpawnInitalization=[this](AActor* Actor)
            { CastChecked<AONEZombie>(Actor)->VariantOverride=Variant; };
            auto* Zombie=World->SpawnActor<AONEZombie>(Position,FRotator::ZeroRotator,Params);
            return Zombie && Mode->RegisterZombie(Zombie)?Zombie:nullptr;
        }
        void Tick() { World->Tick(LEVELTICK_All,1.f/60.f); }
    };

    FONEWeaponDamagePacket Packet(uint64 Id,AONEZombie* Zombie,EONEHitRegion Region,float Damage,float Trauma=0.f,bool Lethal=false)
    {
        FONEWeaponDamagePacket P; P.ShotId=Id; P.bForceLethal=Lethal;
        const FName Bone=Region==EONEHitRegion::ArmLeft?FName(TEXT("upperarm_r")):FName(TEXT("spine_01"));
        P.Get(Region).AddPellet(Damage,Trauma,Zombie->GetMesh()->GetSocketLocation(Bone),
            FVector::ForwardVector,-FVector::ForwardVector,Bone);
        P.Finalize(); return P;
    }

    // A real frozen corpse is positioned once as explicit fixture setup. After
    // that setup neither actor nor any rigid body is moved by the test. Actual
    // Chaos motion and the ordinary recovery loop must clear occupied anatomy.
    class FRecoveryEffortCheck : public IAutomationLatentCommand
    {
        FAutomationTestBase* Test;
        TUniquePtr<FWorld> Scene;
        AONEZombie* Living=nullptr;
        AONEZombie* Corpse=nullptr;
        AActor* Ceiling=nullptr;
        int32 Phase=0,RestSetupStage=0,ResumeBefore=0;
        uint32 CorpseId=0;
        float PhaseStart=0;
        FVector InitialPelvis=FVector::ZeroVector,InitialTransport=FVector::ZeroVector;
        bool SawEffortMotion=false;
        double Started=FPlatformTime::Seconds();
        bool Finish() { Scene.Reset(); return true; }
        float Age() const { return Scene->World->GetTimeSeconds()-PhaseStart; }
        void Next(int32 P) { Phase=P; PhaseStart=Scene->World->GetTimeSeconds(); }
        bool TimedOut(float Seconds)
        {
            if (Age()<Seconds) return false;
            for (AONEZombie* Z:{Living,Corpse})
            {
                const auto& R=Z->GetRestState(); FHitResult Floor;
                const FVector P=Z->GetMesh()->GetSocketLocation(TEXT("pelvis"));
                const bool Supported=Z->FindRecoveryFloor(P,Floor);
                Test->AddInfo(FString::Printf(TEXT("Rest prerequisite id=%u now=%.3f frozen=%d bodies=%d awake=%d last=%.3f sleeping=%d disturbed=%.3f window=%.3f stable=%.3f max_linear=%.5f max_angular=%.5f drift_cm=%.5f drift_deg=%.5f pelvis=%s floor=%d floor_z=%.3f"),
                    Z->GetUniqueID(),Scene->World->GetTimeSeconds(),R.Frozen,Z->GetActivePhysicsBodyCount(),Z->GetAwakePhysicsBodyCount(),R.LastSample,R.WasSleeping,R.DisturbedAt,R.WindowStart,R.StableSeconds,R.MaxLinear,R.MaxAngular,R.PoseDriftCm,R.PoseDriftDegrees,*P.ToCompactString(),Supported,Floor.ImpactPoint.Z));
            }
            Test->AddError(FString::Printf(TEXT("Recovery effort phase %d rest_stage=%d timed out: efforts=%d blocked=%d travel=%.3f frozen=%d"),
                Phase,RestSetupStage,Living->GetRecoveryEffortCount(),Living->GetRecoveryBlockedCount(),Living->GetRecoveryEffortTravelCm(),Living->GetRestState().Frozen));
            return true;
        }
        void FixtureSleep(AONEZombie* Z,bool Allow)
        {
            // Fixture-only solver control: let the production supported-rest
            // monitor observe its complete quiet pose window. Repeated waking
            // can otherwise reset that window after Chaos sleeps between ticks.
            // No production rest threshold or pose is changed; restore the
            // original per-instance sleep multiplier before the effort trial.
            for (FBodyInstance* Body:Z->GetMesh()->Bodies)
                if (Body && Body->IsValidBodyInstance())
                    FPhysicsCommand::ExecuteWrite(Body->GetPhysicsActorHandle(),[&](const FPhysicsActorHandle& Actor)
                    { FPhysicsInterface::SetSleepThresholdMultiplier_AssumesLocked(Actor,Allow?Body->GetSleepThresholdMultiplier():0.f); });
            if (!Allow) Z->GetMesh()->WakeAllRigidBodies();
        }
    public:
        explicit FRecoveryEffortCheck(FAutomationTestBase* InTest):Test(InTest) {}
        virtual bool Update() override
        {
            if (FPlatformTime::Seconds()-Started>180.) { Test->AddError(TEXT("Recovery effort wall timeout")); return Finish(); }
            if (!Scene)
            {
                Scene=MakeUnique<FWorld>();
                if (!Test->TestTrue(TEXT("Recovery effort has actual world, authority and imported variant"),Scene->World && Scene->Mode && Scene->Variant)) return Finish();
                Living=Scene->Spawn(FVector(0,0,90)); Corpse=Scene->Spawn(FVector(650,0,90));
                if (!Test->TestTrue(TEXT("Two actual registered infected exist"),Living && Corpse)) return Finish();
                CorpseId=Corpse->GetUniqueID(); Next(0); return false;
            }
            Scene->Tick();
            if (!IsValid(Corpse)) { Test->AddError(TEXT("Corpse retired before clearance proof; retirement cannot count as escape")); return Finish(); }
            if (Phase==0)
            {
                if (Age()<.3f) return false;
                if (!Test->TestTrue(TEXT("Effort fixture begins an actual living fall"),Living->TryLivingFall(FVector(230,0,15),TEXT("automation_effort")))) return Finish();
                FixtureSleep(Living,false);
                Next(1); return false;
            }
            if (Phase==1)
            {
                if (Age()<.65f) return false;
                const FVector P=Living->GetMesh()->GetSocketLocation(TEXT("pelvis"));
                Ceiling=Scene->Box(FVector(P.X,P.Y,155),FVector(220,220,10));
                Next(2); return false;
            }
            if (Phase==2)
            {
                if (RestSetupStage==0)
                {
                    if (!Living->GetRestState().Frozen) { if (TimedOut(12.f)) return Finish(); return false; }
                    // Prove escape from a genuinely frozen intact body first.
                    // Limb loss is exercised during get-up below, independently
                    // of whether an asymmetric fallen pose qualifies to freeze.
                    // The corpse's ordinary lifespan starts only now.
                    Test->TestEqual(TEXT("Blocker is an actual lethal damage outcome"),Corpse->ReceiveWeaponDamageOutcome(Packet(9402,Corpse,EONEHitRegion::Body,1.f,0.f,true)),EONEWeaponHitOutcome::NewKill);
                    FixtureSleep(Corpse,false); RestSetupStage=1; Next(2); return false;
                }
                if (!Living->GetRestState().Frozen || !Corpse->GetRestState().Frozen)
                { if (TimedOut(12.f)) return Finish(); return false; }
                FixtureSleep(Living,true); FixtureSleep(Corpse,true);
                Test->TestEqual(TEXT("Static overhead solid never starts an anatomy effort"),Living->GetRecoveryEffortCount(),0);
                Test->TestEqual(TEXT("Supported living rest has no simulated bodies before wake"),Living->GetActivePhysicsBodyCount(),0);
                InitialPelvis=Living->GetMesh()->GetSocketLocation(TEXT("pelvis")); InitialTransport=Living->GetActorLocation();
                const FVector Feet=(Living->GetMesh()->GetSocketLocation(TEXT("foot_r"))+Living->GetMesh()->GetSocketLocation(TEXT("foot_l")))*.5f;
                const FVector Axis=(Living->GetMesh()->GetSocketLocation(TEXT("head"))-Feet).GetSafeNormal2D();
                const FVector Side=FVector::CrossProduct(FVector::UpVector,Axis);
                const FVector Body=Corpse->BodyRegion->GetComponentLocation();
                const FVector Wanted=InitialPelvis+Side*46.f;
                const FVector Delta(Wanted.X-Body.X,Wanted.Y-Body.Y,0);
                // One declared setup translation of the already frozen corpse;
                // both Z support and the evaluated skeletal pose are retained.
                Corpse->SetActorLocation(Corpse->GetActorLocation()+Delta,false,nullptr,ETeleportType::TeleportPhysics);
                Corpse->GetMesh()->TickAnimation(0.f,false); Corpse->GetMesh()->RefreshBoneTransforms();
                Corpse->GetMesh()->UpdateKinematicBonesToAnim(Corpse->GetMesh()->GetComponentSpaceTransforms(),ETeleportType::TeleportPhysics,true,EAllowKinematicDeferral::DisallowDeferral);
                ResumeBefore=Living->GetRestState().ResumeEvents;
                Next(3); return false;
            }
            if (Phase==3)
            {
                if (Age()<.1f) return false;
                FVector At; FRotator Facing; UPrimitiveComponent* Blocker=nullptr;
                Test->TestFalse(TEXT("Ceiling still blocks ordinary standing clearance"),Living->FindRecoverySpace(At,Facing,&Blocker));
                Test->TestNull(TEXT("A solid ceiling cannot authorize an anatomy effort"),Blocker);
                Ceiling->Destroy(); Ceiling=nullptr;
                if (!Test->TestFalse(TEXT("Actual retained corpse blocks the unmodified recovery volumes"),Living->FindRecoverySpace(At,Facing,&Blocker))) return Finish();
                if (!Test->TestTrue(TEXT("Clearance blocker belongs to that actual corpse"),Blocker && Blocker->GetOwner()==Corpse)) return Finish();
                Scene->Ground->Destroy(); Scene->Ground=nullptr;
                Test->TestFalse(TEXT("Missing floor rejects an otherwise actual corpse effort"),Living->BeginRecoveryEffort(Blocker));
                Test->TestEqual(TEXT("Unsupported refusal consumes no effort or wake"),Living->GetRecoveryEffortCount(),0);
                Test->TestTrue(TEXT("Unsupported refusal preserves supported frozen pose"),Living->GetRestState().Frozen);
                Scene->Ground=Scene->Box(FVector(0,0,-10),FVector(2200,2200,10));
                Next(4); return false;
            }
            if (Phase==4)
            {
                const FVector P=Living->GetMesh()->GetSocketLocation(TEXT("pelvis"));
                if (!SawEffortMotion && Living->GetRecoveryEffortCount()>0 && Living->IsLivingFallen() && FVector::Dist2D(P,InitialPelvis)>2.f)
                {
                    SawEffortMotion=true;
                    Test->TestTrue(TEXT("Physical effort moves bones while the disabled transport remains fixed"),Living->GetActorLocation().Equals(InitialTransport,.01f));
                }
                if (!Living->IsGettingUp()) { if (TimedOut(18.f)) return Finish(); return false; }
                Test->TestTrue(TEXT("Actual force effort displaced the retained living body before get-up"),SawEffortMotion);
                Test->TestTrue(TEXT("Recovery effort resumed the real frozen body"),Living->GetRestState().ResumeEvents>ResumeBefore);
                Test->TestTrue(TEXT("Frozen resume preserved evaluated pose continuity"),Living->GetRestState().ResumePositionErrorCm<.5f);
                Test->TestTrue(TEXT("Finite effort count is one through six"),Living->GetRecoveryEffortCount()>0 && Living->GetRecoveryEffortCount()<=6);
                Test->TestTrue(TEXT("Measured effort path stays within the 90 cm budget plus one physics step"),Living->GetRecoveryEffortTravelCm()<=92.f);
                Test->TestTrue(TEXT("Get-up snapshot retains the actual cleared physical pose"),Living->GetRecoveryRebaseErrorCm()<.5f);
                Test->TestEqual(TEXT("Same blocker identity remains present during recovery"),Corpse->GetUniqueID(),CorpseId);
                Test->TestTrue(TEXT("Effort did not delete, revive or change corpse health"),Corpse->IsDead() && Corpse->GetHealth()==0.f);
                Test->TestEqual(TEXT("Actual escape preserves intact living health"),Living->GetHealth(),112.f);
                Test->TestEqual(TEXT("A real non-heavy arm sever is accepted during get-up"),Living->ReceiveWeaponDamageOutcome(Packet(9401,Living,EONEHitRegion::ArmLeft,1.f,50.f)),EONEWeaponHitOutcome::LiveHit);
                Test->TestTrue(TEXT("Non-heavy get-up sever preserves the recovery state"),Living->IsGettingUp());
                Test->TestFalse(TEXT("Get-up sever removes the anatomical left arm"),Living->HasLeftArm());
                Test->TestEqual(TEXT("Get-up sever disables its damage query"),Living->ArmLeftRegion->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
                Test->TestEqual(TEXT("Get-up sever removes its retained physical chain"),Living->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft),0);
                Next(5); return false;
            }
            if (Phase==5)
            {
                if (Living->GetRecoveryCount()==0) { if (TimedOut(4.5f)) return Finish(); return false; }
                Test->TestEqual(TEXT("One supported recovery completes around the still-present corpse"),Living->GetRecoveryCount(),1);
                Test->TestTrue(TEXT("Completion retains exactly the post-sever health"),FMath::IsNearlyEqual(Living->GetHealth(),111.6f,.001f));
                Test->TestFalse(TEXT("Completion cannot restore the severed left arm"),Living->HasLeftArm());
                Test->TestEqual(TEXT("Completion retains the disabled left-arm query"),Living->ArmLeftRegion->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
                Test->TestEqual(TEXT("Completion cannot restore severed-arm bodies"),Living->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft),0);
                Test->TestFalse(TEXT("Leaving full-body recovery clears stale frozen bookkeeping"),Living->GetRestState().Frozen);
                if (Scene->World->GetTimeSeconds()<Living->NextFallAllowed) return false;
                if (!Test->TestTrue(TEXT("Recovered identity can enter a second actual fall"),Living->TryLivingFall(FVector(230,0,15),TEXT("automation_second_fall")))) return Finish();
                Test->TestTrue(TEXT("Second fall enables real retained simulation"),Living->GetActivePhysicsBodyCount()>0);
                Test->TestFalse(TEXT("Second fall rest monitor is not stuck Frozen"),Living->GetRestState().Frozen);
                Test->TestEqual(TEXT("Second fall still has no severed-arm bodies"),Living->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft),0);
                Next(6); return false;
            }
            if (Phase==6)
            {
                if (Age()<.3f) return false;
                Test->TestTrue(TEXT("Second fall's restarted rest timer observes the actual body"),Living->GetRestState().LastSample>=PhaseStart);
                return Finish();
            }
            return false;
        }
    };

    enum class EScenario { FallenDamage, BlockedLimbRecovery, FallCapAndGetUpDeath, RegionalFiltering };
    class FCheck : public IAutomationLatentCommand
    {
        FAutomationTestBase* Test;
        EScenario Scenario;
        TUniquePtr<FWorld> Scene;
        TArray<AONEZombie*> Zombies;
        AActor* Obstruction=nullptr;
        AONEPowerUpPickup* RecoveryPickup=nullptr;
        int32 Phase=0;
        float PhaseStart=0;
        float LethalRecoveryWait=0;
        uint32 Identity=0;
        double WallStart=FPlatformTime::Seconds();
        bool Finish() { Scene.Reset(); return true; }
        void Next(int32 NewPhase) { Phase=NewPhase; PhaseStart=Scene->World->GetTimeSeconds(); }
        float Age() const { return Scene->World->GetTimeSeconds()-PhaseStart; }
        bool WaitLimit(float Limit,const TCHAR* Description)
        {
            if (Age()<Limit) return false;
            Test->AddError(FString::Printf(TEXT("%s did not complete within %.1f gameplay seconds"),Description,Limit));
            return true;
        }
        void QueryGeometryDiagnostics(UCapsuleComponent* Component,const FString& Label,const FVector& Start,const FVector& End)
        {
            const FBodyInstance* Body=Component->GetBodyInstance();
            if (!Body || !Body->IsValidBodyInstance()) return;
            // Read the exact implicit shapes consumed by LineTrace_Geom. These
            // probes diagnose a failure; none substitutes for the strict query.
            FPhysicsCommand::ExecuteRead(Body->GetPhysicsActorHandle(),[&](const FPhysicsActorHandle& Actor)
            {
                const FTransform Pose=FPhysicsInterface::GetGlobalPose_AssumesLocked(Actor);
                const FVector Delta=End-Start;
                const float Length=float(Delta.Size()); // Match LineTrace_Geom's float length.
                const FVector LocalStart=Pose.InverseTransformPositionNoScale(Start);
                const FVector LocalDirection=Pose.InverseTransformVectorNoScale(Delta)/Length;
                const FVector LocalCenter=Pose.InverseTransformPositionNoScale(Component->GetComponentLocation());
                TArray<FPhysicsShapeHandle> Shapes; Body->GetAllShapes_AssumesLocked(Shapes);
                Test->AddInfo(FString::Printf(TEXT("%s geometry region=%s shapes=%d auto_weld=%d welded=%d component_scale=%s body_scale=%s local_start=(%.17g %.17g %.17g) local_direction=(%.17g %.17g %.17g) length=%.17g"),
                    *Label,*Component->GetName(),Shapes.Num(),Body->bAutoWeld,Body->WeldParent!=nullptr,
                    *Component->GetComponentScale().ToString(),*Body->Scale3D.ToString(),
                    LocalStart.X,LocalStart.Y,LocalStart.Z,LocalDirection.X,LocalDirection.Y,LocalDirection.Z,double(Length)));
                for (int32 I=0;I<Shapes.Num();++I)
                {
                    const auto& Shape=Shapes[I]; if (!Shape.IsValid()) continue;
                    const auto& Geometry=Shape.GetGeometry();
                    const auto Filter=FPhysicsInterface::GetShapeFilterData(Shape);
                    auto Probe=[&Geometry](const FVector& Origin,const FVector& Direction,double Distance)
                    {
                        Chaos::FReal Time=0; Chaos::FVec3 Position,Normal; int32 Face=INDEX_NONE;
                        return Geometry.Raycast(Origin,Direction,Distance,0.,Time,Position,Normal,Face);
                    };
                    const bool Original=Probe(LocalStart,LocalDirection,Length);
                    const bool Reverse=Probe(LocalStart+LocalDirection*Length,-LocalDirection,Length);
                    const FVector Diagonal=FVector(1,1,.25).GetSafeNormal();
                    const bool DiagonalHit=Probe(LocalCenter-Diagonal*80.,Diagonal,160.);
                    const bool AxialHit=Probe(LocalCenter-FVector(0,0,80),FVector::UpVector,160.);
                    Chaos::FVec3 CenterNormal;
                    const double CenterPhi=Geometry.PhiWithNormal(LocalCenter,CenterNormal);
                    Test->AddInfo(FString::Printf(TEXT("%s implicit region=%s index=%d type=%d bound=%d query=%d flags=%u visibility_block=%d center_phi=%.17g direct_original=%d reverse=%d diagonal=%d axial=%d"),
                        *Label,*Component->GetName(),I,int32(Geometry.GetType()),Body->IsShapeBoundToBody(Shape),
                        FPhysicsInterface::IsQueryShape(Shape),uint32(Filter.GetFlags()),
                        (Filter.GetQueryBlockChannels() & (uint64(1)<<ECC_Visibility))!=0,CenterPhi,Original,Reverse,DiagonalHit,AxialHit));
                    const auto Collection=FPhysicsInterface::GetGeometryCollection(Shape);
                    if (Collection.GetType()==ECollisionShapeType::Capsule)
                    {
                        const auto& Capsule=Collection.GetCapsuleGeometry();
                        const auto A=Capsule.GetX1f(),B=Capsule.GetX2f();
                        Test->AddInfo(FString::Printf(TEXT("%s capsule region=%s radius=%.17g endpoint_a=(%.17g %.17g %.17g) endpoint_b=(%.17g %.17g %.17g) shape_local=%s"),
                            *Label,*Component->GetName(),double(Capsule.GetRadiusf()),double(A.X),double(A.Y),double(A.Z),
                            double(B.X),double(B.Y),double(B.Z),*FPhysicsInterface::GetLocalTransform(Shape).ToString()));
                        if (Geometry.GetType()==Chaos::ImplicitObjectType::Capsule)
                        {
                            // Mirror only the diagnostic AABB/initial-overlap
                            // intermediates in UE RayCapsule, not its result.
                            Chaos::FVec3 Inv; bool Parallel[3];
                            for (int32 Axis=0;Axis<3;++Axis)
                            { Parallel[Axis]=LocalDirection[Axis]==0.; Inv[Axis]=Parallel[Axis]?0.:1./LocalDirection[Axis]; }
                            Chaos::FReal Entry=0; Chaos::FVec3 EntryPosition;
                            const bool BoundsHit=Capsule.BoundingBox().RaycastFast(LocalStart,LocalDirection,Inv,Parallel,Length,Entry,EntryPosition);
                            if (BoundsHit)
                            {
                                const Chaos::FVec3 Offset=LocalStart+Entry*LocalDirection-Chaos::FVec3(A);
                                const double Projection=Chaos::FVec3::DotProduct(Offset,Capsule.GetAxis());
                                const double Radius2=FMath::Square(double(Capsule.GetRadiusf()));
                                const double Radial2=(Offset-Capsule.GetAxis()*FMath::Clamp(Projection,0.,double(Capsule.GetHeightf()))).SizeSquared();
                                const double QuadraticC=Offset.SizeSquared()-Projection*Projection-Radius2;
                                Test->AddInfo(FString::Printf(TEXT("%s capsule_math region=%s bounds_entry=%.17g axial_projection=%.17g radial2=%.17g radius2=%.17g initial_overlap=%d quadratic_c=%.17g caps_only_branch=%d"),
                                    *Label,*Component->GetName(),double(Entry),Projection,Radial2,Radius2,Radial2<=Radius2,QuadraticC,
                                    Radial2>Radius2 && QuadraticC<=0.));
                            }
                        }
                    }
                }
            });
        }
        void Queries(AONEZombie* Z,const FString& Label)
        {
            Test->TestTrue(Label+TEXT(" left leg query covers evaluated bone chain"),Z->GetLegQueryCoverageErrorCm(EONEHitRegion::LegLeft)<.75f);
            Test->TestTrue(Label+TEXT(" right leg query covers evaluated bone chain"),Z->GetLegQueryCoverageErrorCm(EONEHitRegion::LegRight)<.75f);
            for (UCapsuleComponent* Shape:{Z->HeadRegion.Get(),Z->BodyRegion.Get(),Z->UpperLegLeftRegion.Get(),Z->UpperLegRightRegion.Get()})
            {
                Test->TestEqual(Label+TEXT(" present query remains visibility-blocking"),Shape->GetCollisionResponseToChannel(ECC_Visibility),ECR_Block);
                Test->TestTrue(Label+TEXT(" present query remains enabled"),Shape->GetCollisionEnabled()!=ECollisionEnabled::NoCollision);
                const FVector Center=Shape->GetComponentLocation(),Side=Shape->GetRightVector()*80.f;
                FHitResult Hit;
                const bool Contact=Shape->LineTraceComponent(Hit,Center-Side,Center+Side,FCollisionQueryParams(SCENE_QUERY_STAT(ONE07PhysicalityTest),false));
                const FBodyInstance* Body=Shape->GetBodyInstance();
                const FTransform QueryPose=Body && Body->IsValidBodyInstance()?Body->GetUnrealWorldTransform():FTransform::Identity;
                const float PositionError=Body && Body->IsValidBodyInstance()?float(FVector::Dist(Center,QueryPose.GetLocation())):BIG_NUMBER;
                const float RotationError=Body && Body->IsValidBodyInstance()?FMath::RadiansToDegrees(float(QueryPose.GetRotation().AngularDistance(Shape->GetComponentQuat()))):BIG_NUMBER;
                Test->AddInfo(FString::Printf(TEXT("%s region=%s contact=%d body_valid=%d pose_vs_query_cm=%.6f pose_vs_query_deg=%.6f center=(%.3f %.3f %.3f) query=(%.3f %.3f %.3f) ray_side=(%.3f %.3f %.3f) radius=%.3f halfheight=%.3f"),
                    *Label,*Shape->GetName(),Contact,Body && Body->IsValidBodyInstance(),PositionError,RotationError,
                    Center.X,Center.Y,Center.Z,QueryPose.GetLocation().X,QueryPose.GetLocation().Y,QueryPose.GetLocation().Z,
                    Side.X,Side.Y,Side.Z,Shape->GetScaledCapsuleRadius(),Shape->GetScaledCapsuleHalfHeight()));
                QueryGeometryDiagnostics(Shape,Label,Center-Side,Center+Side);
                if (!Contact) Test->AddInfo(Label+TEXT(" KnownEngineCapsuleIssue: raw Chaos side-on ray missed ")+Shape->GetName()+TEXT("; raw geometry diagnostics are retained above."));
                FHitResult ProductionHit;
                const bool ProductionContact=ONE07RegionalTrace::TraceCapsule(Shape,ProductionHit,Center-Side,Center+Side,
                    FCollisionQueryParams(SCENE_QUERY_STAT(ONE07ProductionRegion),false));
                Test->TestTrue(Label+TEXT(" ")+Shape->GetName()+TEXT(" production posed regional query accepts the unchanged crossing ray"),ProductionContact);
                Test->TestTrue(Label+TEXT(" exact query keeps the original actor and region identity"),ProductionContact && ProductionHit.GetActor()==Z && ProductionHit.GetComponent()==Shape);
            }
        }
        void MissingRegionFilter(AONEZombie* Z)
        {
            auto* Missing=Z->ArmLeftRegion.Get();
            Test->TestEqual(TEXT("Actual sever first disables its query"),Missing->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            // Deliberately re-enable only this query to prove anatomical presence
            // is an independent filter. Restore it before any world simulation.
            Missing->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            const FVector Center=Missing->GetComponentLocation(),Side=Missing->GetRightVector()*80.;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE07MissingRegion),false);
            TInlineComponentArray<UCapsuleComponent*> Components(Z);
            for (auto* Shape:Components) if (Shape!=Missing) Params.AddIgnoredComponent(Shape);
            Test->TestTrue(TEXT("Missing-region fixture has a registered query body"),Missing->IsRegistered() && Missing->GetBodyInstance()->IsValidBodyInstance());
            FHitResult Hit;
            Test->TestFalse(TEXT("Exact capsule query rejects an anatomically absent arm"),ONE07RegionalTrace::TraceCapsule(Missing,Hit,Center-Side,Center+Side,Params));
            Test->TestFalse(TEXT("Merged world query rejects an absent arm even if its collision was re-enabled"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Center-Side,Center+Side,Params));
            Missing->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        }
        void WorldTraceContracts(AONEZombie* First)
        {
            auto* Second=Scene->Spawn(First->GetActorLocation()+FVector(360,0,0));
            if (!Test->TestNotNull(TEXT("Second registered actor exists for distinct-victim traces"),Second)) return;
            auto* Front=First->HeadRegion.Get(); auto* Rear=Second->HeadRegion.Get();
            const FVector Direction=(Rear->GetComponentLocation()-Front->GetComponentLocation()).GetSafeNormal();
            const FVector Start=Front->GetComponentLocation()-Direction*100.,End=Rear->GetComponentLocation()+Direction*100.;
            FCollisionQueryParams Params(SCENE_QUERY_STAT(ONE07WorldRegions),false);
            // Isolate two actual, evaluated head capsules for filtering tests.
            // These are explicit world-query fixtures, not player aiming claims.
            for (auto* Z:{First,Second})
            {
                TInlineComponentArray<UCapsuleComponent*> Components(Z);
                for (auto* Shape:Components) if (Shape!=Front && Shape!=Rear) Params.AddIgnoredComponent(Shape);
            }
            FHitResult Hit;
            Test->TestTrue(TEXT("Merged trace reaches nearest registered victim"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,Params) && Hit.GetActor()==First && Hit.GetComponent()==Front);
            Test->TestTrue(TEXT("Primitive query preserves NAME_None for existing regional impact-bone selection"),Hit.BoneName.IsNone());
            const auto Context=Scene->Mode->BeginCombatDischarge();
            const auto Damage=QueryPacket(9100,First,Hit);
            const auto Outcome=First->ReceiveWeaponDamageOutcome(Damage);
            Test->TestEqual(TEXT("One merged query produces one accepted health transaction"),Outcome,EONEWeaponHitOutcome::LiveHit);
            Test->TestEqual(TEXT("Repeated query identity cannot apply damage twice"),First->ReceiveWeaponDamageOutcome(Damage),EONEWeaponHitOutcome::Rejected);
            Test->TestEqual(TEXT("Merged hit awards one impact"),Scene->Mode->RecordCombatAward(Context,First,Outcome,false),10);
            Test->TestEqual(TEXT("Repeated merged actor identity cannot award twice"),Scene->Mode->RecordCombatAward(Context,First,Outcome,false),0);
            FCollisionQueryParams IgnoreFirst=Params; IgnoreFirst.AddIgnoredActor(First);
            Test->TestTrue(TEXT("Penetration-style ignored actor exposes the second distinct victim"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,IgnoreFirst) && Hit.GetActor()==Second);
            const auto SecondOutcome=Second->ReceiveWeaponDamageOutcome(QueryPacket(9100,Second,Hit));
            Test->TestEqual(TEXT("Same discharge can damage the second distinct victim once"),SecondOutcome,EONEWeaponHitOutcome::LiveHit);
            Test->TestEqual(TEXT("Second victim earns its own single impact award"),Scene->Mode->RecordCombatAward(Context,Second,SecondOutcome,false),10);
            Scene->Mode->EndCombatDischarge(Context);
            IgnoreFirst.AddIgnoredActor(Second);
            Test->TestFalse(TEXT("Ignoring both victims leaves no contact on the same segment"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,IgnoreFirst));
            auto IgnoredComponent=Params; IgnoredComponent.AddIgnoredComponent(Front);
            Test->TestTrue(TEXT("Ignored component exposes the rear victim"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,IgnoredComponent) && Hit.GetActor()==Second);
            Front->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Test->TestTrue(TEXT("Disabled regional query cannot block the rear victim"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,Params) && Hit.GetActor()==Second);
            Front->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Front->SetCollisionResponseToChannel(ECC_Visibility,ECR_Overlap);
            Test->TestTrue(TEXT("Nonblocking regional overlap cannot replace a blocking rear hit"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,Params) && Hit.GetActor()==Second);
            Front->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
            auto IgnoreBlocks=Params; IgnoreBlocks.bIgnoreBlocks=true;
            Test->TestFalse(TEXT("bIgnoreBlocks suppresses analytic and engine blocking hits"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,IgnoreBlocks));
            auto IgnoreTouches=Params; IgnoreTouches.bIgnoreTouches=true;
            Test->TestTrue(TEXT("bIgnoreTouches retains the blocking front region"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,IgnoreTouches) && Hit.GetActor()==First);
            const FVector OriginalScale=Front->GetComponentScale(); Front->SetWorldScale3D(FVector(1.2,.8,1.1));
            const FVector Center=Front->GetComponentLocation(),Side=Front->GetRightVector()*80.;
            Test->TestTrue(TEXT("Component query uses evaluated rotation and nonuniform scaled dimensions"),ONE07RegionalTrace::TraceCapsule(Front,Hit,Center-Side,Center+Side,Params));
            Test->TestTrue(TEXT("Scaled hit remains at the exact current scaled radius"),FMath::IsNearlyEqual(double(Hit.Distance),80.-Front->GetScaledCapsuleRadius(),.0001));
            Front->SetWorldScale3D(OriginalScale);
            const FVector Outside=Front->GetComponentLocation()+Front->GetForwardVector()*(Front->GetScaledCapsuleRadius()+.001);
            const FVector MissSide=Front->GetRightVector()*80.;
            Test->TestFalse(TEXT("Evaluated capsule does not inflate a 0.001 cm near miss"),ONE07RegionalTrace::TraceCapsule(Front,Hit,Outside-MissSide,Outside+MissSide,Params));
            auto IsolateFront=Params; IsolateFront.AddIgnoredActor(Second);
            Test->TestFalse(TEXT("Merged query also preserves that near miss"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Outside-MissSide,Outside+MissSide,IsolateFront));
            AActor* Wall=Scene->Box(Start+Direction*40.,FVector(2,60,60));
            if (!Test->TestNotNull(TEXT("Actual solid wall exists before the front capsule"),Wall)) return;
            Test->TestTrue(TEXT("Ordinary world wall occludes both exact regional hits"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,Params) && Hit.GetActor()==Wall);
            auto IgnoreWall=Params; IgnoreWall.AddIgnoredActor(Wall);
            Test->TestTrue(TEXT("Explicitly ignored wall reveals the front region on the same ray"),ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,Start,End,IgnoreWall) && Hit.GetActor()==First);
        }
        bool FixedFloorQuery(AONEZombie* Z,FHitResult& Result,const FString& Label)
        {
            // The grid is fixed in fixture-world coordinates before any pose
            // is known. It does not project or redirect toward regional bones.
            // These are query probes, not the player's limited-height gun ray.
            for (float X:{-180.f,-120.f,-60.f,0.f,60.f,120.f,180.f})
                for (float Y:{-90.f,-45.f,0.f,45.f,90.f})
                {
                    FHitResult Hit;
                    if (ONE07RegionalTrace::TraceWeaponSegment(Scene->World,Hit,FVector(X,Y,240),FVector(X,Y,-10),
                        FCollisionQueryParams(SCENE_QUERY_STAT(ONE07FixedFloorProbe),false)) && Hit.GetActor()==Z)
                    { Result=Hit; return true; }
                }
            Test->AddError(Label+TEXT(" no fixed downward world ray reached a present regional query"));
            return false;
        }
        FONEWeaponDamagePacket QueryPacket(uint64 Id,AONEZombie* Z,const FHitResult& Hit)
        {
            FONEWeaponDamagePacket P; P.ShotId=Id;
            P.Get(Z->GetHitRegion(Hit)).AddPellet(10.f,0.f,Hit.ImpactPoint,FVector::DownVector,Hit.ImpactNormal,Hit.BoneName);
            P.Finalize(); return P;
        }
        void LowPlaneIsIndependent(const FString& Label)
        {
            // A synthetic logical cursor and muzzle are frozen fixture inputs;
            // scene state is never used by either aim helper. Record any real
            // query result without requiring an unreachable low corpse hit.
            const FVector CursorOrigin(-400,0,500),CursorDirection=(FVector(0,0,65)-CursorOrigin).GetSafeNormal();
            FVector Point=FVector::ZeroVector;
            Test->TestTrue(Label+TEXT(" fixed cursor intersects the selected 65 cm plane"),ONEAim::IntersectCursorPlane(CursorOrigin,CursorDirection,65.f,Point));
            Test->TestTrue(Label+TEXT(" selected aim point remains independent of living/dead pose"),Point.Equals(FVector(0,0,65),.001));
            const FVector Shoulder(-300,0,130),Muzzle(-260,0,125);
            const FVector Intent=ONEAim::ResolveIntent(Shoulder,Point,FVector::ForwardVector,4.f);
            const FVector Direction=ONEAim::ResolveShotDirection(Shoulder,Point,Intent,Muzzle,120.f,35.f);
            FHitResult Hit;
            const bool Contact=Scene->World->LineTraceSingleByChannel(Hit,Muzzle,Muzzle+Direction*1000.f,ECC_Visibility,
                FCollisionQueryParams(SCENE_QUERY_STAT(ONE07FixedLowPlane),false));
            Test->AddInfo(FString::Printf(TEXT("%s fixed low-plane query: direction=(%.6f %.6f %.6f), contact=%d, actor=%s, component=%s. Contact is diagnostic, not a required corpse hit."),
                *Label,Direction.X,Direction.Y,Direction.Z,Contact,*GetNameSafe(Hit.GetActor()),*GetNameSafe(Hit.GetComponent())));
        }
        void KillOnce(AONEZombie* Z,const FString& Label)
        {
            const int32 Kills=Scene->Mode->GetKills(),Remaining=Scene->Mode->GetRemaining(),Points=Scene->Mode->GetPoints();
            const uint64 Deaths=Scene->Mode->GetPowerUps()->GetDropStats().EligibleDeaths;
            const int32 Transactions=Z->GetDamageTransactionCount(),Falls=Z->GetLivingFallCount();
            const auto Context=Scene->Mode->BeginCombatDischarge();
            Test->TestTrue(Label+TEXT(" gets an actual authority discharge receipt"),Context.IsValid());
            const auto P=Packet(Context.DischargeId+10000,Z,EONEHitRegion::Body,1.f,0.f,true);
            const auto Outcome=Z->ReceiveWeaponDamageOutcome(P);
            Test->TestEqual(Label+TEXT(" lethal packet is a new kill"),Outcome,EONEWeaponHitOutcome::NewKill);
            Test->TestEqual(Label+TEXT(" body impact and kill award"),Scene->Mode->RecordCombatAward(Context,Z,Outcome,false),110);
            Test->TestEqual(Label+TEXT(" repeated award is rejected"),Scene->Mode->RecordCombatAward(Context,Z,Outcome,false),0);
            Scene->Mode->EndCombatDischarge(Context);
            Test->TestEqual(Label+TEXT(" repeated damage packet is rejected"),Z->ReceiveWeaponDamageOutcome(P),EONEWeaponHitOutcome::Rejected);
            Scene->Mode->NotifyZombieKilled(Z);
            Test->TestTrue(Label+TEXT(" death remains full-body physics"),Z->IsDead() && Z->IsRagdollActive());
            Test->TestEqual(Label+TEXT(" one kill registration"),Scene->Mode->GetKills(),Kills+1);
            Test->TestEqual(Label+TEXT(" one population removal"),Scene->Mode->GetRemaining(),Remaining-1);
            Test->TestEqual(Label+TEXT(" one eligible drop death"),Scene->Mode->GetPowerUps()->GetDropStats().EligibleDeaths,Deaths+1);
            Test->TestEqual(Label+TEXT(" one point transaction"),Scene->Mode->GetPoints(),Points+110);
            Test->TestEqual(Label+TEXT(" one live damage transaction"),Z->GetDamageTransactionCount(),Transactions+1);
            Test->TestEqual(Label+TEXT(" death does not count another living fall"),Z->GetLivingFallCount(),Falls);
        }
    public:
        FCheck(FAutomationTestBase* InTest,EScenario InScenario):Test(InTest),Scenario(InScenario) {}
        virtual bool Update() override
        {
            if (FPlatformTime::Seconds()-WallStart>180.)
            { Test->AddError(TEXT("Physicality automation exceeded its 180 second wall timeout")); return Finish(); }
            if (!Scene)
            {
                Scene=MakeUnique<FWorld>();
                if (!Test->TestTrue(TEXT("Isolated begun-play world has authority and imported C07 variant"),
                    Scene->World && Scene->World->HasBegunPlay() && Scene->Mode && Scene->Variant)) return Finish();
                if (Scenario==EScenario::FallenDamage) LowPlaneIsIndependent(TEXT("Empty scene:"));
                const int32 Count=Scenario==EScenario::FallCapAndGetUpDeath?5:1;
                for (int32 I=0;I<Count;++I)
                {
                    auto* Z=Scene->Spawn(FVector(I*360.f,0,90));
                    if (!Test->TestNotNull(TEXT("Registered actual infected"),Z)) return Finish();
                    Zombies.Add(Z);
                    auto* Anim=Cast<UONEInfectedAnimInstance>(Z->GetMesh()->GetAnimInstance());
                    if (!Test->TestTrue(TEXT("Actual infected graph has all imported action and recovery clips"),Anim && Anim->HasRequiredClips())) return Finish();
                    Test->TestTrue(TEXT("Living upper-body physical response is enabled"),Z->HasLivingPhysicalResponse());
                    Test->TestEqual(TEXT("Normal living health"),Z->GetHealth(),112.f);
                }
                Identity=Zombies[0]->GetUniqueID(); Next(0); return false;
            }
            Scene->Tick();
            auto* Z=Zombies[0];
            if (Phase==0)
            {
                if (Age()<.3f) return false;
                if (Scenario==EScenario::RegionalFiltering) { WorldTraceContracts(Z); return Finish(); }
                Test->TestTrue(TEXT("Upright reward origin retains the existing actor-location contract"),Z->GetPhysicalRewardLocation().Equals(Z->GetActorLocation(),.001));
                if (Scenario==EScenario::BlockedLimbRecovery)
                {
                    Test->TestEqual(TEXT("Explicit single-arm trauma is a living hit"),
                        Z->ReceiveWeaponDamageOutcome(Packet(9001,Z,EONEHitRegion::ArmLeft,1.f,50.f)),EONEWeaponHitOutcome::LiveHit);
                    Test->TestFalse(TEXT("Anatomical left arm was removed"),Z->HasLeftArm());
                    MissingRegionFilter(Z);
                }
                const int32 Count=Scenario==EScenario::FallCapAndGetUpDeath?4:1;
                for (int32 I=0;I<Count;++I)
                    if (!Test->TestTrue(TEXT("Explicit living fall starts real full-body simulation"),Zombies[I]->TryLivingFall(FVector(230,0,15),TEXT("automation_contact")))) return Finish();
                Test->TestEqual(TEXT("Fall preserves actor identity"),Z->GetUniqueID(),Identity);
                Test->TestEqual(TEXT("Fall preserves registered live population"),Scene->Mode->GetRemaining(),Zombies.Num());
                Test->TestEqual(TEXT("Fall has no kill"),Scene->Mode->GetKills(),0);
                Test->TestEqual(TEXT("Fall has no points"),Scene->Mode->GetPoints(),0);
                Test->TestEqual(TEXT("Fall has no eligible drop roll"),Scene->Mode->GetPowerUps()->GetDropStats().EligibleDeaths,uint64(0));
                Test->TestEqual(TEXT("Fall entry counted once"),Z->GetLivingFallCount(),1);
                Test->TestFalse(TEXT("Repeated fall entry is rejected"),Z->TryLivingFall(FVector(230,0,15),TEXT("automation_contact")));
                Test->TestTrue(TEXT("Fall preserves the sampled pose at physics handoff"),Z->GetRagdollTransitionErrorCm()<.1f);
                if (Scenario==EScenario::FallCapAndGetUpDeath)
                {
                    Test->TestFalse(TEXT("A fifth concurrent living full-body simulation is rejected"),Zombies[4]->TryLivingFall(FVector(230,0,15),TEXT("automation_cap")));
                    Test->TestFalse(TEXT("Cap suppression leaves fifth actor living and upright"),Zombies[4]->IsDead() || Zombies[4]->IsLivingFallen());
                    Test->TestEqual(TEXT("Cap suppression has no fall count"),Zombies[4]->GetLivingFallCount(),0);
                }
                Next(1); return false;
            }
            if (Scenario==EScenario::FallenDamage)
            {
                if (Phase==1)
                {
                    if (Age()<.85f) return false;
                    Test->TestTrue(TEXT("Pose queries sampled while actually fallen"),Z->IsLivingFallen()); Queries(Z,TEXT("Fallen:"));
                    LowPlaneIsIndependent(TEXT("Living fallen:"));
                    FHitResult Hit; if (!FixedFloorQuery(Z,Hit,TEXT("Living fallen:"))) return Finish();
                    const EONEHitRegion Region=Z->GetHitRegion(Hit);
                    const float ExpectedDamage=Region==EONEHitRegion::Head?15.f:
                        (Region==EONEHitRegion::ArmLeft || Region==EONEHitRegion::ArmRight)?4.f:10.f;
                    const float BeforeHealth=Z->GetHealth();
                    const auto P=QueryPacket(9002,Z,Hit);
                    Test->TestEqual(TEXT("A fixed world ray supplies an accepted nonlethal fallen-region packet"),Z->ReceiveWeaponDamageOutcome(P),EONEWeaponHitOutcome::LiveHit);
                    Test->TestTrue(TEXT("Fallen body loses only the actual hit region's ordinary health damage"),FMath::IsNearlyEqual(Z->GetHealth(),BeforeHealth-ExpectedDamage,.001f));
                    Test->TestEqual(TEXT("Repeated fallen query packet cannot damage twice"),Z->ReceiveWeaponDamageOutcome(P),EONEWeaponHitOutcome::Rejected);
                    Test->TestTrue(TEXT("Nonlethal damage does not revive or kill fallen actor"),Z->IsLivingFallen() && !Z->IsDead());
                    const FVector FallenPelvis=Z->GetMesh()->GetSocketLocation(TEXT("pelvis"));
                    Test->TestTrue(TEXT("Reward-origin fixture actually displaced the living pelvis from the capsule horizontally"),FVector::DistSquared2D(FallenPelvis,Z->GetActorLocation())>1.);
                    Test->TestTrue(TEXT("Living fallen reward origin follows the evaluated pelvis"),Z->GetPhysicalRewardLocation().Equals(FallenPelvis,.001));
                    KillOnce(Z,TEXT("Fallen:"));
                    Test->TestTrue(TEXT("Death reward origin uses the evaluated physical pelvis instead of the stale capsule"),Z->GetPhysicalRewardLocation().Equals(Z->GetMesh()->GetSocketLocation(TEXT("pelvis")),.001));
                    Test->TestEqual(TEXT("Reward-origin check preserves the zero-chance deterministic drop fixture"),Scene->Mode->GetPowerUps()->TotalDropChance,0.f);
                    Next(2); return false;
                }
                if (Age()<1.5f) return false;
                Queries(Z,TEXT("Dead:"));
                LowPlaneIsIndependent(TEXT("Floor corpse:"));
                Test->TestTrue(TEXT("Corpse pose has reached the actual floor rather than a standing proxy"),
                    Z->GetMesh()->GetSocketLocation(TEXT("pelvis")).Z<60.f && Z->GetMesh()->GetSocketLocation(TEXT("pelvis")).Z>=-5.f);
                FHitResult Hit; if (!FixedFloorQuery(Z,Hit,TEXT("Floor corpse:"))) return Finish();
                const auto P=QueryPacket(9003,Z,Hit);
                const int32 LiveTransactions=Z->GetDamageTransactionCount(),CorpseTransactions=Z->GetCorpseTransactionCount();
                const int32 Kills=Scene->Mode->GetKills(),Points=Scene->Mode->GetPoints();
                const uint64 Deaths=Scene->Mode->GetPowerUps()->GetDropStats().EligibleDeaths;
                const auto Context=Scene->Mode->BeginCombatDischarge();
                const auto Outcome=Z->ReceiveWeaponDamageOutcome(P);
                Test->TestEqual(TEXT("Fixed floor ray reaches an actual cosmetic corpse transaction"),Outcome,EONEWeaponHitOutcome::CorpseHit);
                Test->TestEqual(TEXT("Corpse query packet cannot award points"),Scene->Mode->RecordCombatAward(Context,Z,Outcome,false),0);
                Scene->Mode->EndCombatDischarge(Context);
                Test->TestEqual(TEXT("Corpse remains at zero health"),Z->GetHealth(),0.f);
                Test->TestEqual(TEXT("Corpse query leaves live damage count unchanged"),Z->GetDamageTransactionCount(),LiveTransactions);
                Test->TestEqual(TEXT("Corpse query records one cosmetic transaction"),Z->GetCorpseTransactionCount(),CorpseTransactions+1);
                Test->TestEqual(TEXT("Corpse query cannot register another kill"),Scene->Mode->GetKills(),Kills);
                Test->TestEqual(TEXT("Corpse query cannot award points indirectly"),Scene->Mode->GetPoints(),Points);
                Test->TestEqual(TEXT("Corpse query cannot roll another drop"),Scene->Mode->GetPowerUps()->GetDropStats().EligibleDeaths,Deaths);
                Test->TestEqual(TEXT("Lethal fallen actor does not recover"),Z->GetRecoveryCount(),0);
                return Finish();
            }
            if (Scenario==EScenario::BlockedLimbRecovery)
            {
                if (Phase==1)
                {
                    // A local overhead solid blocks standing clearance while
                    // allowing the body to settle on the actual floor below.
                    if (Age()<.65f) return false;
                    const FVector Pelvis=Z->GetMesh()->GetSocketLocation(TEXT("pelvis"));
                    Obstruction=Scene->Box(FVector(Pelvis.X,Pelvis.Y,155),FVector(220,220,10));
                    if (!Test->TestNotNull(TEXT("Actual local recovery obstruction"),Obstruction)) return Finish();
                    Next(2); return false;
                }
                if (Phase==2)
                {
                    if (Z->GetRecoveryBlockedCount()==0)
                    { if (WaitLimit(12.f,TEXT("Physical settling and obstructed recovery attempt"))) return Finish(); return false; }
                    Test->TestTrue(TEXT("Local obstruction keeps infected alive and fallen"),Z->IsLivingFallen() && !Z->IsDead());
                    Test->TestEqual(TEXT("Blocked recovery does not fabricate a completion"),Z->GetRecoveryCount(),0);
                    Test->TestTrue(TEXT("Missing arm has no rigid bodies while fallen"),Z->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft)==0);
                    Obstruction->Destroy(); Obstruction=nullptr;
                    const FVector Pelvis=Z->GetMesh()->GetSocketLocation(TEXT("pelvis"));
                    RecoveryPickup=Scene->World->SpawnActor<AONEPowerUpPickup>(FVector(Pelvis.X,Pelvis.Y,40),FRotator::ZeroRotator);
                    if (!Test->TestNotNull(TEXT("Actual nearby pickup collection volume"),RecoveryPickup)) return Finish();
                    RecoveryPickup->Initialize(Scene->Mode->GetPowerUps(),EONEPowerUpType::MaxAmmo,Scene->Mode->GetPowerUps()->GetRunId(),30.f,80.f);
                    Test->TestEqual(TEXT("Pickup remains non-solid WorldDynamic"),RecoveryPickup->Collection->GetCollisionObjectType(),ECC_WorldDynamic);
                    Test->TestEqual(TEXT("Pickup only overlaps transport"),RecoveryPickup->Collection->GetCollisionResponseToChannel(ECC_Pawn),ECR_Overlap);
                    Next(3); return false;
                }
                if (Phase==3)
                {
                    if (!Z->IsGettingUp())
                    { if (WaitLimit(10.f,TEXT("Unblocked local recovery"))) return Finish(); return false; }
                    Test->TestTrue(TEXT("Recovery snapshot rebase preserves world pose"),Z->GetRecoveryRebaseErrorCm()<.5f);
                    Test->TestTrue(TEXT("Nearby available pickup does not block local floor or get-up clearance"),IsValid(RecoveryPickup) && RecoveryPickup->IsAvailable());
                    Queries(Z,TEXT("Get-up:")); Next(4); return false;
                }
                if (Z->GetRecoveryCount()==0)
                { if (WaitLimit(4.5f,TEXT("Authored get-up completion"))) return Finish(); return false; }
                Test->TestEqual(TEXT("Get-up completes once"),Z->GetRecoveryCount(),1);
                Test->TestTrue(TEXT("Uncollected pickup remains available through completed get-up"),IsValid(RecoveryPickup) && RecoveryPickup->IsAvailable());
                Test->TestEqual(TEXT("Get-up retains actor identity"),Z->GetUniqueID(),Identity);
                Test->TestTrue(TEXT("Get-up retains living post-trauma health"),FMath::IsNearlyEqual(Z->GetHealth(),111.6f,.001f));
                Test->TestFalse(TEXT("Get-up cannot restore missing left arm"),Z->HasLeftArm());
                Test->TestEqual(TEXT("Get-up missing arm query remains disabled"),Z->ArmLeftRegion->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
                Test->TestEqual(TEXT("Get-up missing arm bodies remain absent"),Z->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft),0);
                Test->TestTrue(TEXT("Get-up restores living upper-body response"),Z->HasLivingPhysicalResponse());
                Test->TestEqual(TEXT("Get-up has no points"),Scene->Mode->GetPoints(),0);
                KillOnce(Z,TEXT("Recovered:"));
                Test->TestFalse(TEXT("Death cannot restore missing left arm"),Z->HasLeftArm());
                Test->TestEqual(TEXT("Death missing arm bodies remain absent"),Z->GetRegionPhysicsBodyCount(EONEHitRegion::ArmLeft),0);
                return Finish();
            }
            if (Phase==1)
            {
                if (!Z->IsGettingUp())
                { if (WaitLimit(14.f,TEXT("Natural supported get-up after capped falls"))) return Finish(); return false; }
                Test->TestTrue(TEXT("Get-up death fixture is still alive before the lethal packet"),!Z->IsDead() && Z->GetHealth()==112.f);
                const auto Recovery=Z->GetInfectedAnimationState();
                LethalRecoveryWait=Recovery.ActionDurationSeconds+Recovery.ActionLeadInSeconds+.25f;
                Queries(Z,TEXT("Get-up before lethal:")); KillOnce(Z,TEXT("Get-up:")); Next(2); return false;
            }
            if (Age()<LethalRecoveryWait) return false;
            Test->TestTrue(TEXT("Lethal get-up remains dead after the former recovery deadline"),Z->IsDead());
            Test->TestEqual(TEXT("Lethal get-up never completes a recovery"),Z->GetRecoveryCount(),0);
            Test->TestEqual(TEXT("Other living actors are not killed by the transition"),Scene->Mode->GetRemaining(),4);
            return Finish();
        }
    };
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07FallenDamageTest,"ProjectONE.Candidate07.Physicality.FallenDamageAndDeathIdentity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07FallenDamageTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(ONE07PhysicalityTestDetails::FCheck(this,ONE07PhysicalityTestDetails::EScenario::FallenDamage));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07BlockedRecoveryTest,"ProjectONE.Candidate07.Physicality.BlockedLocalRecoveryAndMissingLimb",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07BlockedRecoveryTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(ONE07PhysicalityTestDetails::FCheck(this,ONE07PhysicalityTestDetails::EScenario::BlockedLimbRecovery));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07FallCapTest,"ProjectONE.Candidate07.Physicality.ConcurrentFallCapAndLethalGetUp",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07FallCapTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(ONE07PhysicalityTestDetails::FCheck(this,ONE07PhysicalityTestDetails::EScenario::FallCapAndGetUpDeath));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07WorldRegionFilterTest,"ProjectONE.Candidate07.RegionalTrace.WorldOcclusionAndVictimIdentity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07WorldRegionFilterTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(ONE07PhysicalityTestDetails::FCheck(this,ONE07PhysicalityTestDetails::EScenario::RegionalFiltering));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FONE07RecoveryEffortTest,"ProjectONE.Candidate07.Physicality.FrozenBodyEffortClearsActualCorpse",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FONE07RecoveryEffortTest::RunTest(const FString&)
{
    ADD_LATENT_AUTOMATION_COMMAND(ONE07PhysicalityTestDetails::FRecoveryEffortCheck(this));
    return true;
}
#endif
