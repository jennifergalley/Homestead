#include "HomesteadWorld.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadCharacter.h"
#include "HomesteadEstateTerrain.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInterface.h"

using HomesteadWorldCommon::IsMvpWoodlandId;
using HomesteadWorldCommon::ProfileChunkPublishing;

bool AHomesteadWorld::ResolveGeneratedTreeVisual(const Homestead::ResourceNode& Node, UStaticMesh*& Mesh,
    FHomesteadOuterTreeInstance& Instance)
{
    Homestead::Generation::GeneratedEntity Entity;
    if (bFixedEstate && Node.kind == Homestead::ResourceKind::ForestTree)
    {
        // Estate trees are baked placements, not generated entities: pick a period-plausible
        // broadleaf or conifer and a stable yaw and size from the placement id. The MVP woodland's
        // trees (ids 560000+) take the MVP palette: 60% broadleaf, 35% fir, 5% jacaranda.
        const uint32 Hash = HashCombine(GetTypeHash(Node.id), 0x9E3779B9u);
        const bool bMvpWoodland = IsMvpWoodlandId(Node.id);
        const uint32 PaletteRoll = Hash % 100;
        Entity.paletteRole = bMvpWoodland
            ? (PaletteRoll < 60 ? Homestead::Generation::TreePaletteRole::BroadleafMature
                : PaletteRoll < 95 ? Homestead::Generation::TreePaletteRole::ConiferMature
                : Homestead::Generation::TreePaletteRole::WoodlandAccent)
            : Hash % 5 == 0 ? Homestead::Generation::TreePaletteRole::ConiferMature
            : Homestead::Generation::TreePaletteRole::BroadleafMature;
        Entity.variantIndex = 0;
        Entity.yawDegrees = static_cast<decltype(Entity.yawDegrees)>((Hash >> 8) % 360);
        // AssignTreePalette's MVP sizes: 0.90 up to 1.04 (broadleaf), 1.05 (fir) or 1.06 (jacaranda).
        const uint32 Spread = Entity.paletteRole == Homestead::Generation::TreePaletteRole::BroadleafMature ? 141
            : Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferMature ? 151 : 161;
        Entity.scalePermille = static_cast<decltype(Entity.scalePermille)>(900 + (Hash >> 16) % (bMvpWoodland ? Spread : 260));
    }
    else if (Node.kind != Homestead::ResourceKind::ForestTree
        || Homestead::Generation::FindEntity(Descriptor, Node.key, Entity) != Homestead::Generation::Status::Ok)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Generated tree key cannot resolve; no visual substitute."));
        return false;
    }
    FString RequestedPath;
    switch (Entity.paletteRole)
    {
    case Homestead::Generation::TreePaletteRole::BroadleafMature:
        RequestedPath = TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland");
        break;
    case Homestead::Generation::TreePaletteRole::ConiferMature:
        RequestedPath = TEXT("/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir.SM_MatureFir");
        break;
    case Homestead::Generation::TreePaletteRole::WoodlandAccent:
        RequestedPath = TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_Jacaranda.SM_Jacaranda");
        break;
    default:
        UE_LOG(LogHomesteadWorld, Error, TEXT("Generated mature tree has illegal palette role %d."),
            static_cast<int32>(Entity.paletteRole));
        return false;
    }
    Mesh = LoadObject<UStaticMesh>(nullptr, *RequestedPath);
    const int32 ExpectedSlots = Entity.paletteRole
        == Homestead::Generation::TreePaletteRole::ConiferMature ? 4 : 3;
    if (!Mesh || !Mesh->GetBodySetup() || Mesh->GetBodySetup()->AggGeom.SphylElems.Num() != 1
        || Mesh->GetStaticMaterials().Num() != ExpectedSlots
        || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 3)
    {
        UE_LOG(LogHomesteadWorld, Error,
            TEXT("Generated tree role=%d variant=%u has missing native material/LOD/collision at %s; no substitute."),
            static_cast<int32>(Entity.paletteRole), Entity.variantIndex, *RequestedPath);
        return false;
    }
    for (int32 Slot = 0; Slot < ExpectedSlots; ++Slot)
        if (!Mesh->GetMaterial(Slot))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Generated tree material slot %d is missing at %s."),
                Slot, *RequestedPath);
            return false;
        }
    const FRotator Rotation(0, Entity.yawDegrees, 0);
    const float Scale = Entity.scalePermille / 1000.0f;
    const auto& Capsule = Mesh->GetBodySetup()->AggGeom.SphylElems[0];
    const FVector Anchor(Capsule.Center.X, Capsule.Center.Y, Mesh->GetBoundingBox().Min.Z);
    // Measured from each mesh's LOD0: how far the underside of the root flare rim rises above the
    // mesh's lowest vertex, and how far the flare reaches from the trunk axis. The fir's skirt
    // curls up to 30 cm and the jacaranda's surface roots lift up to 19 cm, so sinking by less leaves
    // the flare floating with a shadow under it.
    const bool bConifer = Entity.paletteRole == Homestead::Generation::TreePaletteRole::ConiferMature;
    const bool bAccent = Entity.paletteRole == Homestead::Generation::TreePaletteRole::WoodlandAccent;
    const float RimLift = bConifer ? 30.0f : bAccent ? 19.0f : 3.0f;
    const float Footprint = bConifer ? 76.0f : bAccent ? 240.0f : 41.0f;
    const float Radius = FMath::Max(Capsule.Radius, Footprint) * Scale;
    const float Embed = 4.0f + RimLift * Scale;
    float RootGround = CachedGroundHeight(Node.position.x, Node.position.y);
    for (const float Reach : {0.5f, 1.0f})
        for (const FVector2D Direction : {FVector2D(1,0), FVector2D(-1,0), FVector2D(0,1),
            FVector2D(0,-1), FVector2D(0.7071f,0.7071f), FVector2D(-0.7071f,0.7071f),
            FVector2D(0.7071f,-0.7071f), FVector2D(-0.7071f,-0.7071f)})
            RootGround = FMath::Min(RootGround, CachedGroundHeight(
                Node.position.x + Direction.X * Radius * Reach, Node.position.y + Direction.Y * Radius * Reach));
    const FVector Base(Node.position.x, Node.position.y, RootGround - Embed);
    Instance.MeshPath = Mesh->GetPathName();
    Instance.Transform = FTransform(Rotation, Base - Rotation.RotateVector(Anchor * Scale), FVector(Scale));
    Instance.PaletteRole = static_cast<int32>(Entity.paletteRole);
    Instance.VariantIndex = Entity.variantIndex;
    return true;
}

void AHomesteadWorld::ClearOuterTreeBatches()
{
    for (auto& Entry : OuterTreeBatches)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    OuterTreeBatches.Reset();
    OuterTreeInstances.Reset();
}

bool AHomesteadWorld::RebuildOuterTreeBatches(const Homestead::Simulation& Simulation)
{
    const double Started = FPlatformTime::Seconds();
    double BatchSetupMilliseconds = 0;
    double InstanceMilliseconds = 0;
    double TeardownMilliseconds = 0;
    double RegisterMilliseconds = 0;
    double TreeBuildMilliseconds = 0;
    struct FBuildEntry
    {
        FString Key;
        UStaticMesh* Mesh = nullptr;
        FHomesteadOuterTreeInstance Instance;
    };

    TArray<FBuildEntry> Desired;
    TSet<FString> DesiredKeys;
    for (const auto& Chunk : TerrainChunks)
    {
        if (Chunk.Value.bCollision) continue;
        Homestead::Generation::ChunkBaseline Baseline;
        if (!GetChunkBaseline(Simulation.GetState().world,
            {Chunk.Key.X, Chunk.Key.Y}, Baseline))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Outer tree generation failed."));
            return false;
        }
        for (const auto& Entity : Baseline.entities)
        {
            if (Entity.kind != Homestead::Generation::EntityKind::ForestTree) continue;
            Homestead::ResourceNode Node;
            Node.key = Entity.key;
            Node.kind = Homestead::ResourceKind::ForestTree;
            Node.position = {static_cast<double>(Entity.xCm), static_cast<double>(Entity.yCm)};
            const auto Edit = std::lower_bound(Simulation.GetState().resourceEdits.begin(),
                Simulation.GetState().resourceEdits.end(), Entity.key,
                [](const Homestead::ResourceEdit& Value,
                    const Homestead::Generation::GeneratedEntityKey& Key) { return Value.key < Key; });
            if (Edit != Simulation.GetState().resourceEdits.end() && Edit->key == Entity.key)
                Node.cleared = Edit->cleared;
            if (Node.cleared) continue;
            FBuildEntry Entry;
            Entry.Key = FString::Printf(TEXT("%d,%d,%u"),
                Node.key.chunk.x, Node.key.chunk.y, Node.key.localId);
            if (DesiredKeys.Contains(Entry.Key))
            {
                UE_LOG(LogHomesteadWorld, Error, TEXT("Duplicate generated outer tree key %s."), *Entry.Key);
                return false;
            }
            if (!ResolveGeneratedTreeVisual(Node, Entry.Mesh, Entry.Instance))
                return false;
            DesiredKeys.Add(Entry.Key);
            Desired.Add(MoveTemp(Entry));
        }
    }
    Desired.Sort([](const FBuildEntry& A, const FBuildEntry& B)
    {
        const int32 PathOrder = A.Instance.MeshPath.Compare(B.Instance.MeshPath, ESearchCase::CaseSensitive);
        return PathOrder == 0 ? A.Key < B.Key : PathOrder < 0;
    });
    const double DesiredPrepared = FPlatformTime::Seconds();

    TMap<FString, UHierarchicalInstancedStaticMeshComponent*> PreparedBatches;
    TMap<FString, FHomesteadOuterTreeInstance> PreparedInstances;
    auto DiscardPrepared = [&PreparedBatches]()
    {
        for (auto& Entry : PreparedBatches)
            if (IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
    };
    for (const FBuildEntry& Entry : Desired)
    {
        UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
        if (auto** Existing = PreparedBatches.Find(Entry.Instance.MeshPath))
            Batch = *Existing;
        else
        {
            const double SetupStarted = FPlatformTime::Seconds();
            Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            if (!Batch)
            {
                DiscardPrepared();
                UE_LOG(LogHomesteadWorld, Error, TEXT("Could not allocate outer tree batch for %s."),
                    *Entry.Instance.MeshPath);
                return false;
            }
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->SetStaticMesh(Entry.Mesh);
            if (!ApplyCameraSafeFoliageMaterials(*Batch))
            {
                Batch->DestroyComponent();
                DiscardPrepared();
                return false;
            }
            Batch->bOverrideMinLOD = true;
            Batch->MinLOD = OuterMatureTreeMinLOD;
            Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetCollisionResponseToAllChannels(ECR_Ignore);
            Batch->SetGenerateOverlapEvents(false);
            Batch->SetCanEverAffectNavigation(false);
            Batch->ComponentTags.Add(TEXT("GeneratedOuterTreeBatch"));
            Batch->bAutoRebuildTreeOnInstanceChanges = false;
            PreparedBatches.Add(Entry.Instance.MeshPath, Batch);
            BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
        }
        const double InstanceStarted = FPlatformTime::Seconds();
        Batch->AddInstance(Entry.Instance.Transform);
        InstanceMilliseconds += (FPlatformTime::Seconds() - InstanceStarted) * 1000;
        PreparedInstances.Add(Entry.Key, Entry.Instance);
    }

    const double TeardownStarted = FPlatformTime::Seconds();
    ClearOuterTreeBatches();
    TeardownMilliseconds = (FPlatformTime::Seconds() - TeardownStarted) * 1000;
    for (auto& Entry : PreparedBatches)
    {
        Entry.Value->bAutoRebuildTreeOnInstanceChanges = true;
        const double RegisterStarted = FPlatformTime::Seconds();
        Entry.Value->RegisterComponent();
        RegisterMilliseconds += (FPlatformTime::Seconds() - RegisterStarted) * 1000;
        const double TreeStarted = FPlatformTime::Seconds();
        Entry.Value->BuildTreeIfOutdated(false, true);
        TreeBuildMilliseconds += (FPlatformTime::Seconds() - TreeStarted) * 1000;
        OuterTreeBatches.Add(Entry.Key, Entry.Value);
    }
    OuterTreeInstances = MoveTemp(PreparedInstances);
    LastOuterTreePrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    if (ProfileChunkPublishing())
        OuterTreeProfile = FString::Printf(
            TEXT("CHUNK_STAGE outer_tree desired_ms=%.3f batch_setup_ms=%.3f add_instances_ms=%.3f teardown_ms=%.3f register_ms=%.3f build_tree_ms=%.3f total_ms=%.3f\n"),
            (DesiredPrepared - Started) * 1000, BatchSetupMilliseconds, InstanceMilliseconds,
            TeardownMilliseconds, RegisterMilliseconds, TreeBuildMilliseconds,
            LastOuterTreePrepareMilliseconds);
    UE_LOG(LogHomesteadWorld, Display,
        TEXT("Generated outer mature trees rebuilt: batches=%d instances=%d; collision/navigation/overlap disabled."),
        OuterTreeBatches.Num(), OuterTreeInstances.Num());
    return true;
}

void AHomesteadWorld::ClearActiveTreeBatches()
{
    for (auto& Entry : ActiveTreeBatches)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    for (auto& Entry : ActiveTreeCollisions)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    ActiveTreeBatches.Reset();
    ActiveTreeCollisions.Reset();
    ActiveTreeInstances.Reset();
    ActiveTreeLayoutSignature.Reset();
}

bool AHomesteadWorld::RebuildActiveTreeBatches(const Homestead::Simulation& Simulation)
{
    const double Started = FPlatformTime::Seconds();
    double BatchSetupMilliseconds = 0;
    double InstanceMilliseconds = 0;
    double CollisionSetupMilliseconds = 0;
    double TeardownMilliseconds = 0;
    double RegisterMilliseconds = 0;
    double TreeBuildMilliseconds = 0;
    double CollisionRegisterMilliseconds = 0;
    struct FBuildEntry
    {
        FString Key;
        UStaticMesh* Mesh = nullptr;
        FHomesteadActiveTreeInstance Instance;
    };

    TArray<FBuildEntry> Desired;
    TSet<FString> DesiredKeys;
    for (const auto& Node : Simulation.GetState().resources)
    {
        if (Node.kind != Homestead::ResourceKind::ForestTree || Node.cleared) continue;
        FBuildEntry Entry;
        Entry.Key = FString::Printf(TEXT("%d,%d,%u"),
            Node.key.chunk.x, Node.key.chunk.y, Node.key.localId);
        if (DesiredKeys.Contains(Entry.Key))
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Duplicate generated active tree key %s."), *Entry.Key);
            return false;
        }
        if (!ResolveGeneratedTreeVisual(Node, Entry.Mesh, Entry.Instance.Visual))
            return false;
        const auto& Capsule = Entry.Mesh->GetBodySetup()->AggGeom.SphylElems[0];
        const FVector Scale = Entry.Instance.Visual.Transform.GetScale3D();
        Entry.Instance.CapsuleRadius = Capsule.GetScaledRadius(Scale);
        Entry.Instance.CapsuleHalfHeight = Capsule.GetScaledHalfLength(Scale);
        Entry.Instance.CollisionTransform = FTransform(
            Entry.Instance.Visual.Transform.TransformRotation(Capsule.Rotation.Quaternion()),
            Entry.Instance.Visual.Transform.TransformPosition(Capsule.Center));
        Entry.Instance.ResourceId = Node.id;
        DesiredKeys.Add(Entry.Key);
        Desired.Add(MoveTemp(Entry));
    }
    Desired.Sort([](const FBuildEntry& A, const FBuildEntry& B)
    {
        const int32 PathOrder = A.Instance.Visual.MeshPath.Compare(
            B.Instance.Visual.MeshPath, ESearchCase::CaseSensitive);
        return PathOrder == 0 ? A.Key < B.Key : PathOrder < 0;
    });
    const double DesiredPrepared = FPlatformTime::Seconds();

    TMap<FString, UHierarchicalInstancedStaticMeshComponent*> PreparedBatches;
    TMap<FString, UCapsuleComponent*> PreparedCollisions;
    TMap<FString, FHomesteadActiveTreeInstance> PreparedInstances;
    TSet<FString> ReusedCollisionKeys;
    auto DiscardPrepared = [&PreparedBatches, &PreparedCollisions, &ReusedCollisionKeys]()
    {
        for (auto& Entry : PreparedBatches)
            if (IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
        for (auto& Entry : PreparedCollisions)
            if (!ReusedCollisionKeys.Contains(Entry.Key) && IsValid(Entry.Value))
                Entry.Value->DestroyComponent();
    };
    for (const FBuildEntry& Entry : Desired)
    {
        UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
        if (auto** Existing = PreparedBatches.Find(Entry.Instance.Visual.MeshPath))
            Batch = *Existing;
        else
        {
            const double SetupStarted = FPlatformTime::Seconds();
            Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
            if (!Batch)
            {
                DiscardPrepared();
                UE_LOG(LogHomesteadWorld, Error, TEXT("Could not allocate active tree batch for %s."),
                    *Entry.Instance.Visual.MeshPath);
                return false;
            }
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Static);
            Batch->SetStaticMesh(Entry.Mesh);
            if (!ApplyCameraSafeFoliageMaterials(*Batch))
            {
                Batch->DestroyComponent();
                DiscardPrepared();
                return false;
            }
            Batch->bOverrideMinLOD = true;
            Batch->MinLOD = ActiveMatureTreeMinLOD;
            Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Batch->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Batch->SetCollisionResponseToAllChannels(ECR_Ignore);
            Batch->SetGenerateOverlapEvents(false);
            Batch->SetCanEverAffectNavigation(false);
            Batch->ComponentTags.Add(TEXT("GeneratedActiveTreeBatch"));
            Batch->bAutoRebuildTreeOnInstanceChanges = false;
            PreparedBatches.Add(Entry.Instance.Visual.MeshPath, Batch);
            BatchSetupMilliseconds += (FPlatformTime::Seconds() - SetupStarted) * 1000;
        }
        const double InstanceStarted = FPlatformTime::Seconds();
        Batch->AddInstance(Entry.Instance.Visual.Transform);
        InstanceMilliseconds += (FPlatformTime::Seconds() - InstanceStarted) * 1000;

        const auto* PreviousInstance = ActiveTreeInstances.Find(Entry.Key);
        const auto* PreviousCollision = ActiveTreeCollisions.Find(Entry.Key);
        if (PreviousInstance && PreviousCollision && IsValid(PreviousCollision->Get())
            && PreviousInstance->ResourceId == Entry.Instance.ResourceId
            && PreviousInstance->Visual.MeshPath == Entry.Instance.Visual.MeshPath
            && PreviousInstance->Visual.Transform.Equals(Entry.Instance.Visual.Transform)
            && PreviousInstance->CollisionTransform.Equals(Entry.Instance.CollisionTransform)
            && FMath::IsNearlyEqual(PreviousInstance->CapsuleRadius, Entry.Instance.CapsuleRadius)
            && FMath::IsNearlyEqual(PreviousInstance->CapsuleHalfHeight, Entry.Instance.CapsuleHalfHeight))
        {
            PreparedCollisions.Add(Entry.Key, PreviousCollision->Get());
            PreparedInstances.Add(Entry.Key, Entry.Instance);
            ReusedCollisionKeys.Add(Entry.Key);
            continue;
        }

        const double CollisionStarted = FPlatformTime::Seconds();
        auto* Collision = NewObject<UCapsuleComponent>(this);
        if (!Collision)
        {
            DiscardPrepared();
            UE_LOG(LogHomesteadWorld, Error, TEXT("Could not allocate active tree collision for %s."),
                *Entry.Key);
            return false;
        }
        Collision->SetupAttachment(GetRootComponent());
        Collision->SetMobility(EComponentMobility::Static);
        Collision->SetCapsuleSize(Entry.Instance.CapsuleRadius, Entry.Instance.CapsuleHalfHeight, false);
        Collision->SetRelativeTransform(Entry.Instance.CollisionTransform);
        Collision->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Collision->SetGenerateOverlapEvents(false);
        Collision->SetCanEverAffectNavigation(false);
        Collision->SetCastShadow(false);
        Collision->SetVisibility(false);
        Collision->SetHiddenInGame(true);
        Collision->ComponentTags.Add(TEXT("GeneratedForestTreeCollision"));
        Collision->ComponentTags.Add(FName(*(FString(TEXT("TreeKey_")) + Entry.Key)));
        Collision->ComponentTags.Add(*FString::Printf(TEXT("Resource_%d"), Entry.Instance.ResourceId));
        PreparedCollisions.Add(Entry.Key, Collision);
        PreparedInstances.Add(Entry.Key, Entry.Instance);
        CollisionSetupMilliseconds += (FPlatformTime::Seconds() - CollisionStarted) * 1000;
    }

    const double TeardownStarted = FPlatformTime::Seconds();
    for (auto& Entry : ActiveTreeBatches)
        if (IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    ActiveTreeBatches.Reset();
    for (auto& Entry : ActiveTreeCollisions)
        if (!ReusedCollisionKeys.Contains(Entry.Key) && IsValid(Entry.Value.Get()))
            Entry.Value->DestroyComponent();
    ActiveTreeCollisions.Reset();
    ActiveTreeInstances.Reset();
    TeardownMilliseconds = (FPlatformTime::Seconds() - TeardownStarted) * 1000;
    for (auto& Entry : PreparedBatches)
    {
        Entry.Value->bAutoRebuildTreeOnInstanceChanges = true;
        const double RegisterStarted = FPlatformTime::Seconds();
        Entry.Value->RegisterComponent();
        RegisterMilliseconds += (FPlatformTime::Seconds() - RegisterStarted) * 1000;
        const double TreeStarted = FPlatformTime::Seconds();
        Entry.Value->BuildTreeIfOutdated(false, true);
        TreeBuildMilliseconds += (FPlatformTime::Seconds() - TreeStarted) * 1000;
        ActiveTreeBatches.Add(Entry.Key, Entry.Value);
    }
    for (auto& Entry : PreparedCollisions)
    {
        if (ReusedCollisionKeys.Contains(Entry.Key))
        {
            ActiveTreeCollisions.Add(Entry.Key, Entry.Value);
            continue;
        }
        const double RegisterStarted = FPlatformTime::Seconds();
        Entry.Value->RegisterComponent();
        CollisionRegisterMilliseconds += (FPlatformTime::Seconds() - RegisterStarted) * 1000;
        ActiveTreeCollisions.Add(Entry.Key, Entry.Value);
    }
    ActiveTreeInstances = MoveTemp(PreparedInstances);
    LastActiveTreePrepareMilliseconds = (FPlatformTime::Seconds() - Started) * 1000;
    if (ProfileChunkPublishing())
        ActiveTreeProfile = FString::Printf(
            TEXT("CHUNK_STAGE active_tree desired_ms=%.3f batch_setup_ms=%.3f add_instances_ms=%.3f collision_setup_ms=%.3f teardown_ms=%.3f register_ms=%.3f build_tree_ms=%.3f collision_register_ms=%.3f total_ms=%.3f\n"),
            (DesiredPrepared - Started) * 1000, BatchSetupMilliseconds, InstanceMilliseconds,
            CollisionSetupMilliseconds, TeardownMilliseconds, RegisterMilliseconds,
            TreeBuildMilliseconds, CollisionRegisterMilliseconds, LastActiveTreePrepareMilliseconds);
    UE_LOG(LogHomesteadWorld, Display,
        TEXT("Generated active mature trees rebuilt: batches=%d instances=%d collisions=%d."),
        ActiveTreeBatches.Num(), ActiveTreeInstances.Num(), ActiveTreeCollisions.Num());
    return true;
}

bool AHomesteadWorld::TreeChopTarget(int32 ResourceId, FVector2D& Centre, float& Radius) const
{
    // Trunk centroid and radius 80-100 cm up each mesh's trunk section (measured from LOD0): the
    // collision capsules wrap the whole trunk and crown, and the jacaranda's forked trunk stands
    // well off its origin.
    struct FChop { const TCHAR* Mesh; FVector2D Offset; float Radius; };
    static const FChop Chops[] = {
        {TEXT("SM_TreeSmall02_Woodland"), FVector2D(9.1, 4.0), 9.0f},
        {TEXT("SM_MatureFir"), FVector2D(3.6, -0.2), 13.0f},
        {TEXT("SM_Jacaranda"), FVector2D(8.5, -37.3), 55.0f},
    };
    for (const auto& Entry : ActiveTreeInstances)
    {
        if (Entry.Value.ResourceId != ResourceId) continue;
        const FTransform& Transform = Entry.Value.Visual.Transform;
        for (const FChop& Chop : Chops)
            if (Entry.Value.Visual.MeshPath.Contains(Chop.Mesh))
            {
                const FVector At = GetActorTransform().TransformPosition(Transform.TransformPosition(FVector(Chop.Offset, 90.0)));
                Centre = FVector2D(At.X, At.Y);
                Radius = Chop.Radius * Transform.GetScale3D().X;
                return true;
            }
        const FVector At = GetActorTransform().TransformPosition(Entry.Value.CollisionTransform.GetLocation());
        Centre = FVector2D(At.X, At.Y);
        Radius = Entry.Value.CapsuleRadius;
        return true;
    }
    return false;
}

void AHomesteadWorld::FinishFallingTree()
{
    for (USceneComponent* Part : FallingParts)
        if (IsValid(Part)) Part->DestroyComponent();
    FallingParts.Reset();
    FallingRest.Reset();
    bTreeFalling = bTreeLanded = bLandingPending = false;
}

bool AHomesteadWorld::BeginFelling(int32 ResourceId)
{
    FinishFallingTree();
    for (const auto& Entry : ActiveTreeInstances)
    {
        if (Entry.Value.ResourceId != ResourceId) continue;
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Entry.Value.Visual.MeshPath);
        if (!Mesh) return false;
        auto* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(GetRootComponent());
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Mesh);
        Part->bOverrideMinLOD = true;
        Part->MinLOD = ActiveMatureTreeMinLOD;
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        if (!ApplyCameraSafeFoliageMaterials(*Part))
        {
            Part->DestroyComponent();
            return false;
        }
        Part->SetRelativeTransform(Entry.Value.Visual.Transform);
        Part->RegisterComponent();
        FallingParts.Add(Part);
        break;
    }
    // Saplings are ordinary resource visuals: take them over so the rebuild leaves them be.
    if (FallingParts.IsEmpty())
        if (FHomesteadWorldVisual* Visual = ResourceVisuals.Find(ResourceId))
        {
            for (USceneComponent* Part : Visual->Components)
                if (IsValid(Part))
                {
                    Part->SetMobility(EComponentMobility::Movable);
                    FallingParts.Add(Part);
                }
            Visual->Components.Reset();
            ResourceVisuals.Remove(ResourceId);
            bRefreshInputsKnown = false;
        }
    if (FallingParts.IsEmpty()) return false;
    FBox Bounds(ForceInit);
    for (USceneComponent* Part : FallingParts)
    {
        FallingRest.Add(Part->GetComponentTransform());
        Bounds += Part->Bounds.GetBox();
    }
    FallPivot = FallingParts[0]->GetComponentLocation();
    FallHeight = FMath::Max(100.0f, static_cast<float>(Bounds.Max.Z - FallPivot.Z));
    FallAngle = FallRate = FallLying = 0;
    FallBounces = 0;
    return true;
}

void AHomesteadWorld::DropFelledTree(FVector2D AwayFrom)
{
    if (FallingParts.IsEmpty() || bTreeFalling) return;
    FVector2D Away = FVector2D(FallPivot.X, FallPivot.Y) - AwayFrom;
    if (!Away.Normalize()) Away = FVector2D(1, 0);
    FallAxis = FVector::CrossProduct(FVector::UpVector, FVector(Away, 0)).GetSafeNormal();
    // A notched trunk starts to lean slowly, then gravity takes it.
    FallAngle = 0.03f;
    FallRate = 0.05f;
    bTreeFalling = true;
}

bool AHomesteadWorld::TakeFelledTreeLanding(FVector& Where)
{
    if (!bLandingPending) return false;
    bLandingPending = false;
    Where = FallPivot;
    return true;
}

void AHomesteadWorld::UpdateFallingTree(float DeltaSeconds)
{
    if (FallingParts.IsEmpty() || !bTreeFalling) return;
    const float Dt = FMath::Min(DeltaSeconds, 1.0f / 20.0f);
    // A rod pivoting on its base: angular acceleration 3g sin(angle) / 2L.
    constexpr float Landed = 1.47f;
    float Sink = 0;
    if (!bTreeLanded)
    {
        FallRate += 1.5f * 980.0f / FallHeight * FMath::Sin(FallAngle) * Dt;
        FallAngle += FallRate * Dt;
        if (FallAngle >= Landed)
        {
            FallAngle = Landed;
            if (FallBounces == 0 && FallRate > 0.4f)
            {
                bLandingPending = true;
                FallRate = -FallRate * 0.12f;
                ++FallBounces;
            }
            else
            {
                FallRate = 0;
                bTreeLanded = true;
            }
        }
    }
    else
    {
        // Lies a few seconds, then sinks into the forest floor.
        FallLying += Dt;
        Sink = 160.0f * FMath::Square(FMath::Clamp((FallLying - 4.0f) / 1.8f, 0.0f, 1.0f));
        if (FallLying > 6.0f)
        {
            FinishFallingTree();
            return;
        }
    }
    const FQuat Turn(FallAxis, FallAngle);
    for (int32 Index = 0; Index < FallingParts.Num(); ++Index)
    {
        if (!IsValid(FallingParts[Index])) continue;
        const FTransform& Rest = FallingRest[Index];
        const FVector Location = FallPivot + Turn.RotateVector(Rest.GetLocation() - FallPivot) - FVector(0, 0, Sink);
        FallingParts[Index]->SetWorldLocationAndRotation(Location, Turn * Rest.GetRotation());
    }
}
