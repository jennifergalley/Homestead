#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadRegionalTerrainAdapter.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "HAL/PlatformMemory.h"
#include "Materials/Material.h"
#include "MaterialShared.h"
#include "Misc/FileHelper.h"
#include "PhysicsEngine/BodySetup.h"
#include "ProceduralMeshComponent.h"
#include "Serialization/JsonSerializer.h"
#include "StaticMeshResources.h"

void AHomesteadVisualPlaytest::RecordGeneratedInventory()
{
    namespace Gen = Homestead::Generation;
    auto* Landscape = PC->Landscape.Get();
    if (!Landscape)
    {
        Observations.Add(TEXT("FAILED generated inventory: no world actor."));
        return;
    }
    const auto& State = PC->State();
    bool Valid = Landscape->IsPreparedFor(State) && Landscape->TerrainChunks.Num() == 25;
    int32 Colliding = 0, ActiveTrees = 0, ActiveBatchInstances = 0, ActiveCollisions = 0;
    int32 OuterTrees = 0, OuterBatchInstances = 0;
    int32 Grass = 0, Ferns = 0, Flowers = 0;
    int32 ExpectedGrass = 0, ExpectedFerns = 0, ExpectedFlowers = 0;
    int32 GrassBatchComponents = 0, FernBatchComponents = 0, FlowerBatchComponents = 0;
    bool CoverPoliciesValid = true, CoverRepresentativeTransformsExact = true;
    double PositionError = 0, NormalError = 0;
    double RegionalOffsetMin = DBL_MAX, RegionalOffsetMax = -DBL_MAX;
    uint16 RegionalRidgeMin = MAX_uint16, RegionalRidgeMax = 0;
    uint16 RegionalValleyMin = MAX_uint16, RegionalValleyMax = 0;
    int32 RegionalReadyChunks = 0, RegionalPartialChunks = 0, RegionalIncompleteChunks = 0;
    int32 RegionalReachReferences = 0, RegionalLakeReferences = 0;
    int32 RegionalWaterComponents = 0;
    bool RegionalWaterPoliciesValid = true;
    bool RegionalDescriptorsConsistent = true;
    TMap<FString, FString> RegionalReachSignatures;
    TMap<FString, FString> RegionalLakeSignatures;
    TArray<TSharedPtr<FJsonValue>> Tiles, Trees, ActiveBatches, OuterBatches;
    const FName GrassTag(TEXT("AuthoredGrassMedium01"));
    const FName FernTag(TEXT("AuthoredFern02"));
    const FName FlowerTag(TEXT("DecorativeWildflower"));
    const TCHAR* GrassPaths[] = {
        TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_mid_b.SM_GrassMedium01_mid_b"),
        TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_small_b.SM_GrassMedium01_small_b"),
        TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tall_a.SM_GrassMedium01_tall_a"),
        TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_tiny_a.SM_GrassMedium01_tiny_a")
    };
    const TCHAR* FernPaths[] = {
        TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_a.SM_Fern02_a"),
        TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_b.SM_Fern02_b"),
        TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_c.SM_Fern02_c"),
        TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_d.SM_Fern02_d")
    };
    const TCHAR* FlowerPaths[] = {
        TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_a.SM_FlowerEmpodium_a"),
        TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_b.SM_FlowerEmpodium_b")
    };
    auto MaterialReady = [](UMaterialInterface* Interface)
    {
        auto* Material = Interface ? Interface->GetMaterial() : nullptr;
        auto* Resource = Interface ? Interface->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
        return Material && Material->GetPathName().StartsWith(TEXT("/Game/"))
            && Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete();
    };
    const auto TreeMaterialsReady = [MaterialReady](
        const UHierarchicalInstancedStaticMeshComponent* Batch)
    {
        const UStaticMesh* Mesh = Batch ? Batch->GetStaticMesh() : nullptr;
        if (!Mesh || !Batch->ComponentHasTag(TEXT("CameraSafeFoliage"))
            || Batch->GetCollisionResponseToChannel(ECC_Camera) != ECR_Ignore)
            return false;
        int32 FoliageSlot = 1;
        FString Wrapper = TEXT("MI_CameraSafe_TreeSmallLeaves");
        if (Mesh->GetPathName().Contains(TEXT("/MatureFir_20260922_02/")))
            Wrapper = TEXT("MI_CameraSafe_MatureFirTwig");
        else if (Mesh->GetPathName().Contains(TEXT("/TreePalette_20260921_01/Meshes/SM_Jacaranda")))
        {
            FoliageSlot = 2;
            Wrapper = TEXT("MI_CameraSafe_JacarandaLeaves");
        }
        for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
        {
            auto* Material = Batch->GetMaterial(Slot);
            if (!MaterialReady(Material)) return false;
            if (Slot == FoliageSlot)
            {
                if (!GetPathNameSafe(Material).Contains(Wrapper)) return false;
            }
            else if (Material != Mesh->GetMaterial(Slot)) return false;
        }
        return true;
    };
    for (const auto& Entry : Landscape->TerrainChunks)
    {
        auto* Terrain = Entry.Value.Terrain.Get();
        const auto* Section = Terrain ? Terrain->GetProcMeshSection(0) : nullptr;
        const bool ExpectedCollision = FMath::Abs(Entry.Key.X - State.activeChunk.x) <= 1
            && FMath::Abs(Entry.Key.Y - State.activeChunk.y) <= 1;
        Colliding += Terrain && Terrain->IsQueryCollisionEnabled() ? 1 : 0;
        bool TileValid = Section && Section->ProcVertexBuffer.Num() == Gen::TerrainVertexCount
            && Section->ProcIndexBuffer.Num() == Gen::TerrainCellsPerChunk * Gen::TerrainCellsPerChunk * 6
            && Terrain->IsQueryCollisionEnabled() == ExpectedCollision && MaterialReady(Terrain->GetMaterial(0));
        if (TileValid)
            for (int Index = 0; Index < Gen::TerrainVertexCount; ++Index)
            {
                const int64 X = static_cast<int64>(Entry.Key.X) * Gen::ChunkSizeCm
                    + (Index % Gen::TerrainVerticesPerSide) * Gen::TerrainSpacingCm;
                const int64 Y = static_cast<int64>(Entry.Key.Y) * Gen::ChunkSizeCm
                    + (Index / Gen::TerrainVerticesPerSide) * Gen::TerrainSpacingCm;
                Gen::TerrainSample Sample;
                TileValid &= Gen::SampleTerrain(State.world, X, Y, Sample) == Gen::Status::Ok;
                Gen::RegionalTerrainInfluence Influence;
                TileValid &= Gen::SampleRegionalTerrainInfluence(State.world, X, Y, Influence)
                    == Homestead::RegionalGeneration::Status::Ok;
                RegionalOffsetMin = FMath::Min(RegionalOffsetMin, Influence.heightOffsetCm);
                RegionalOffsetMax = FMath::Max(RegionalOffsetMax, Influence.heightOffsetCm);
                RegionalRidgeMin = FMath::Min(RegionalRidgeMin, Influence.ridge);
                RegionalRidgeMax = FMath::Max(RegionalRidgeMax, Influence.ridge);
                RegionalValleyMin = FMath::Min(RegionalValleyMin, Influence.valley);
                RegionalValleyMax = FMath::Max(RegionalValleyMax, Influence.valley);
                const auto& Vertex = Section->ProcVertexBuffer[Index];
                PositionError = FMath::Max(PositionError, FVector::Dist(FVector(Vertex.Position), FVector(X, Y, Sample.heightCm)));
                NormalError = FMath::Max(NormalError, FVector::Dist(FVector(Vertex.Normal),
                    FVector(Sample.normalX, Sample.normalY, Sample.normalZ)));
            }
        Valid &= TileValid;
        auto Row = MakeShared<FJsonObject>();
        Row->SetNumberField(TEXT("x"), Entry.Key.X); Row->SetNumberField(TEXT("y"), Entry.Key.Y);
        Row->SetBoolField(TEXT("valid"), TileValid); Row->SetBoolField(TEXT("collision"), ExpectedCollision);
        Tiles.Add(MakeShared<FJsonValueObject>(Row));
        TMap<FString, const UHierarchicalInstancedStaticMeshComponent*> GrassBatches;
        TMap<FString, const UHierarchicalInstancedStaticMeshComponent*> FernBatches;
        TMap<FString, const UHierarchicalInstancedStaticMeshComponent*> FlowerBatches;
        int32 ChunkGrassBatches = 0, ChunkFernBatches = 0, ChunkFlowerBatches = 0;
        for (const auto& Component : Entry.Value.Cover.Components)
        {
            if (const auto* Batch = Cast<UHierarchicalInstancedStaticMeshComponent>(Component))
            {
                const bool IsGrass = Batch->ComponentHasTag(GrassTag);
                const bool IsFern = Batch->ComponentHasTag(FernTag);
                const bool IsFlower = Batch->ComponentHasTag(FlowerTag);
                UStaticMesh* Mesh = Batch->GetStaticMesh();
                const FString MeshPath = Mesh ? Mesh->GetPathName() : FString();
                bool PolicyReady = static_cast<int32>(IsGrass) + static_cast<int32>(IsFern)
                    + static_cast<int32>(IsFlower) == 1 && Batch->IsRegistered() && Mesh
                    && Batch->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                    && !Batch->IsQueryCollisionEnabled() && !Batch->GetGenerateOverlapEvents()
                    && !Batch->CanEverAffectNavigation()
                    && Batch->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore
                    && Batch->ComponentHasTag(TEXT("CameraSafeFoliage"))
                    && Batch->GetCollisionProfileName() == UCollisionProfile::NoCollision_ProfileName
                    && Batch->GetVisibleFlag() && Batch->IsVisible() && !Batch->bHiddenInGame
                    && !Batch->CastShadow
                    && MaterialReady(Batch->GetMaterial(0));
                if (IsGrass)
                {
                    ++ChunkGrassBatches;
                    ++GrassBatchComponents;
                    Grass += Batch->GetInstanceCount();
                    PolicyReady &= Batch->InstanceStartCullDistance == 3500
                        && Batch->InstanceEndCullDistance == 5000 && !GrassBatches.Contains(MeshPath)
                        && GetPathNameSafe(Batch->GetMaterial(0)).Contains(TEXT("MI_CameraSafe_Grass"));
                    GrassBatches.Add(MeshPath, Batch);
                }
                else if (IsFern)
                {
                    ++ChunkFernBatches;
                    ++FernBatchComponents;
                    Ferns += Batch->GetInstanceCount();
                    PolicyReady &= Batch->InstanceStartCullDistance == 0
                        && Batch->InstanceEndCullDistance == 5000 && !FernBatches.Contains(MeshPath)
                        && GetPathNameSafe(Batch->GetMaterial(0)).Contains(TEXT("MI_CameraSafe_Fern"));
                    FernBatches.Add(MeshPath, Batch);
                }
                else if (IsFlower)
                {
                    ++ChunkFlowerBatches;
                    ++FlowerBatchComponents;
                    Flowers += Batch->GetInstanceCount();
                    PolicyReady &= Batch->InstanceStartCullDistance == 3000
                        && Batch->InstanceEndCullDistance == 4800
                        && GetPathNameSafe(Batch->GetMaterial(0)).Contains(TEXT("MI_CameraSafe_Flower"))
                        && MeshPath.StartsWith(
                            TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_FlowerEmpodium_"));
                    FlowerBatches.Add(MeshPath, Batch);
                }
                CoverPoliciesValid &= PolicyReady;
            }
            else CoverPoliciesValid = false;
        }
        CoverPoliciesValid &= ChunkGrassBatches == 4 && ChunkFernBatches == 4 && ChunkFlowerBatches == 2;
        Gen::LoadedChunkWaterDescriptors Water;
        const auto WaterStatus = Landscape->RegionalDescriptors.DescribeChunkWater(
            State.world, {Entry.Key.X, Entry.Key.Y}, Water);
        if (WaterStatus == Gen::RegionalChunkDescriptorStatus::Ready) ++RegionalReadyChunks;
        else if (WaterStatus == Gen::RegionalChunkDescriptorStatus::Partial) ++RegionalPartialChunks;
        else ++RegionalIncompleteChunks;
        for (const auto& Reach : Water.reaches)
        {
            ++RegionalReachReferences;
            const FString Key = FString::Printf(TEXT("%lld,%lld>%lld,%lld"),
                static_cast<long long>(Reach.key.upstream.x),
                static_cast<long long>(Reach.key.upstream.y),
                static_cast<long long>(Reach.key.downstream.x),
                static_cast<long long>(Reach.key.downstream.y));
            const FString Signature = FString::Printf(TEXT("%lld:%lld:%u:%u:%u"),
                static_cast<long long>(Reach.upstreamSurfaceMm),
                static_cast<long long>(Reach.downstreamSurfaceMm), Reach.accumulation,
                Reach.widthClass, Reach.depthClass);
            if (const FString* Existing = RegionalReachSignatures.Find(Key))
                RegionalDescriptorsConsistent &= *Existing == Signature;
            else
                RegionalReachSignatures.Add(Key, Signature);
            RegionalDescriptorsConsistent &= Reach.key.upstream != Reach.key.downstream
                && Reach.downstreamSurfaceMm < Reach.upstreamSurfaceMm;
        }
        for (const auto& Lake : Water.lakes)
        {
            ++RegionalLakeReferences;
            const FString Key = FString::Printf(TEXT("%lld,%lld"),
                static_cast<long long>(Lake.id.x), static_cast<long long>(Lake.id.y));
            const FString Signature = FString::Printf(TEXT("%lld,%lld>%lld,%lld:%lld:%u"),
                static_cast<long long>(Lake.outlet.upstream.x),
                static_cast<long long>(Lake.outlet.upstream.y),
                static_cast<long long>(Lake.outlet.downstream.x),
                static_cast<long long>(Lake.outlet.downstream.y),
                static_cast<long long>(Lake.surfaceMm), Lake.memberCount);
            if (const FString* Existing = RegionalLakeSignatures.Find(Key))
                RegionalDescriptorsConsistent &= *Existing == Signature;
            else
                RegionalLakeSignatures.Add(Key, Signature);
        }
        Homestead::State CoverState;
        CoverState.structures = State.structures;
        CoverState.plots = State.plots;
        bool ExpectedReady = true;
        for (int DY = -1; DY <= 1 && ExpectedReady; ++DY)
            for (int DX = -1; DX <= 1 && ExpectedReady; ++DX)
            {
                Gen::ChunkBaseline Baseline;
                ExpectedReady = Gen::GenerateChunk(State.world,
                    {Entry.Key.X + DX, Entry.Key.Y + DY}, Baseline) == Gen::Status::Ok;
                if (!ExpectedReady) break;
                for (const auto& Entity : Baseline.entities)
                {
                    Homestead::ResourceNode Node;
                    const auto Resolved = PC->Simulation().ResolveGeneratedResource(Entity.key, Node);
                    if (Resolved.code == Homestead::ResultCode::Unavailable) continue;
                    if (!Resolved)
                    {
                        ExpectedReady = false;
                        break;
                    }
                    CoverState.resources.push_back(Node);
                }
            }
        TMap<FString, int32> ExpectedGrassByMesh, ExpectedFernsByMesh, ExpectedFlowersByMesh;
        TMap<FString, FTransform> RepresentativeGrassByMesh, RepresentativeFernByMesh,
            RepresentativeFlowersByMesh;
        if (ExpectedReady)
        {
            const double OriginX = static_cast<int64>(Entry.Key.X) * Gen::ChunkSizeCm;
            const double OriginY = static_cast<int64>(Entry.Key.Y) * Gen::ChunkSizeCm;
            const uint32 Seed = GetTypeHash(State.world.seed) ^ GetTypeHash(Entry.Key);
            FRandomStream Random(static_cast<int32>(Seed));
            FRandomStream FlowerRandom(static_cast<int32>(Seed ^ 0x8DA6B343u));
            for (int32 Attempt = 0; Attempt < 1200; ++Attempt)
            {
                const double X = OriginX + Random.FRandRange(0, 2399.99f);
                const double Y = OriginY + Random.FRandRange(0, 2399.99f);
                const FRotator Rotation(0, Random.FRandRange(0, 360), 0);
                const double StreamDistance = FMath::Abs(X - Homestead::StreamX(Y));
                const bool bCreekBank = StreamDistance >= 100.0 && StreamDistance < 215.0
                    && Attempt % 5 == 0;
                if (Landscape->IsDecorationReserved(CoverState, X, Y, 20, 0, true)
                    || (StreamDistance < 215.0 && !bCreekBank))
                    continue;
                const int32 Variety = Attempt % 16;
                const int32 GrassIndex = Variety == 0 ? 0 : Variety < 3 ? 1 : Variety < 12 ? 2 : 3;
                const FString GrassPath(GrassPaths[GrassIndex]);
                const auto* GrassBatch = GrassBatches.FindRef(GrassPath);
                const int32 GrassIndexInBatch = ExpectedGrassByMesh.FindOrAdd(GrassPath)++;
                ++ExpectedGrass;
                if (GrassBatch && GrassIndexInBatch == 0)
                {
                    UStaticMesh* Mesh = GrassBatch->GetStaticMesh();
                    const FBox Bounds = Mesh->GetBoundingBox();
                    const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
                    RepresentativeGrassByMesh.Add(GrassPath, FTransform(Rotation,
                        Landscape->AtGround(X, Y) - Rotation.RotateVector(Anchor), FVector::OneVector));
                }
                if (Attempt % 32 == 0 && !Landscape->IsDecorationReserved(CoverState, X, Y, 75, 0, true))
                {
                    const FString FernPath(FernPaths[(Attempt / 32) % 4]);
                    const auto* FernBatch = FernBatches.FindRef(FernPath);
                    const int32 FernIndexInBatch = ExpectedFernsByMesh.FindOrAdd(FernPath)++;
                    ++ExpectedFerns;
                    if (FernBatch && FernIndexInBatch == 0)
                    {
                        UStaticMesh* Mesh = FernBatch->GetStaticMesh();
                        const FBox Bounds = Mesh->GetBoundingBox();
                        const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
                        RepresentativeFernByMesh.Add(FernPath, FTransform(Rotation,
                            Landscape->AtGround(X, Y) - Rotation.RotateVector(Anchor), FVector::OneVector));
                    }
                }
                if (Attempt % 64 == 17 && StreamDistance >= 260.0
                    && !Landscape->IsDecorationReserved(CoverState, X, Y, 55, 0, true))
                {
                    const int32 FlowerIndex = (Attempt / 64) % 2;
                    const FString FlowerPath(FlowerPaths[FlowerIndex]);
                    const auto* FlowerBatch = FlowerBatches.FindRef(FlowerPath);
                    const int32 FlowerIndexInBatch = ExpectedFlowersByMesh.FindOrAdd(FlowerPath)++;
                    ++ExpectedFlowers;
                    const float Scale = FlowerRandom.FRandRange(0.55f, 0.80f);
                    if (FlowerBatch && FlowerIndexInBatch == 0)
                    {
                        UStaticMesh* Mesh = FlowerBatch->GetStaticMesh();
                        const FBox Bounds = Mesh->GetBoundingBox();
                        const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
                        RepresentativeFlowersByMesh.Add(FlowerPath, FTransform(Rotation,
                            Landscape->AtGround(X, Y) - Rotation.RotateVector(Anchor * Scale),
                            FVector(Scale)));
                    }
                }
            }
        }
        CoverRepresentativeTransformsExact &= ExpectedReady;
        auto HasRepresentative = [](const UHierarchicalInstancedStaticMeshComponent* Batch,
            const FTransform* Expected)
        {
            if (!Batch || !Expected) return false;
            for (int32 InstanceIndex = 0; InstanceIndex < Batch->GetInstanceCount(); ++InstanceIndex)
            {
                FTransform Actual;
                if (Batch->GetInstanceTransform(InstanceIndex, Actual)
                    && Actual.Equals(*Expected, 0.001f))
                    return true;
            }
            return false;
        };
        for (int32 Index = 0; Index < 4; ++Index)
        {
            const auto* GrassBatch = GrassBatches.FindRef(GrassPaths[Index]);
            const auto* FernBatch = FernBatches.FindRef(FernPaths[Index]);
            const int32 ExpectedGrassInstances = ExpectedGrassByMesh.FindRef(GrassPaths[Index]);
            const int32 ExpectedFernInstances = ExpectedFernsByMesh.FindRef(FernPaths[Index]);
            CoverRepresentativeTransformsExact &= GrassBatch
                && GrassBatch->GetInstanceCount() == ExpectedGrassInstances
                && (ExpectedGrassInstances == 0
                    || HasRepresentative(GrassBatch, RepresentativeGrassByMesh.Find(GrassPaths[Index])));
            CoverRepresentativeTransformsExact &= FernBatch
                && FernBatch->GetInstanceCount() == ExpectedFernInstances
                && (ExpectedFernInstances == 0
                    || HasRepresentative(FernBatch, RepresentativeFernByMesh.Find(FernPaths[Index])));
        }
        for (int32 Index = 0; Index < 2; ++Index)
        {
            const auto* FlowerBatch = FlowerBatches.FindRef(FlowerPaths[Index]);
            const int32 ExpectedFlowerInstances = ExpectedFlowersByMesh.FindRef(FlowerPaths[Index]);
            CoverRepresentativeTransformsExact &= FlowerBatch
                && FlowerBatch->GetInstanceCount() == ExpectedFlowerInstances
                && (ExpectedFlowerInstances == 0
                    || HasRepresentative(FlowerBatch,
                        RepresentativeFlowersByMesh.Find(FlowerPaths[Index])));
        }
    }
    TSet<FString> Keys, TreeMeshes;
    TMap<FString, int32> ExpectedActiveByMesh;
    TMap<FString, FString> RepresentativeActiveKey;
    TMap<FString, int32> ExpectedOuterByMesh;
    TMap<FString, FString> RepresentativeOuterKey;
    auto ExpectedTreePath = [](Gen::TreePaletteRole PaletteRole)
    {
        return PaletteRole == Gen::TreePaletteRole::BroadleafMature
            ? FString(TEXT("/Game/Trials/WoodlandResources_20260921_01/Meshes/SM_TreeSmall02_Woodland.SM_TreeSmall02_Woodland"))
            : PaletteRole == Gen::TreePaletteRole::ConiferMature
                ? FString(TEXT("/Game/Trials/MatureFir_20260922_02/Meshes/SM_MatureFir.SM_MatureFir"))
                : PaletteRole == Gen::TreePaletteRole::WoodlandAccent
                    ? FString(TEXT("/Game/Trials/TreePalette_20260921_01/Meshes/SM_Jacaranda.SM_Jacaranda"))
                    : FString();
    };
    for (const auto& Node : State.resources)
        if (Node.kind == Homestead::ResourceKind::ForestTree)
        {
            const FString Key = FString::Printf(TEXT("%d,%d,%u"),
                Node.key.chunk.x, Node.key.chunk.y, Node.key.localId);
            const auto* Instance = Landscape->ActiveTreeInstances.Find(Key);
            const auto* Collision = Landscape->ActiveTreeCollisions.FindRef(Key).Get();
            if (Node.cleared)
            {
                Valid &= !Instance && !Collision && !Landscape->ResourceVisuals.Contains(Node.id);
                continue;
            }
            ++ActiveTrees;
            if (!Instance || !Collision || Landscape->ResourceVisuals.Contains(Node.id))
            { Valid = false; continue; }
            Gen::GeneratedEntity Entity{};
            const bool EntityReady = Gen::FindEntity(State.world, Node.key, Entity) == Gen::Status::Ok;
            const FString ExpectedPath = EntityReady ? ExpectedTreePath(Entity.paletteRole) : FString();
            auto* Batch = Landscape->ActiveTreeBatches.FindRef(Instance->Visual.MeshPath).Get();
            UStaticMesh* Mesh = Batch ? Batch->GetStaticMesh() : nullptr;
            auto* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
            const int32 ExpectedSlots = EntityReady
                && Entity.paletteRole == Gen::TreePaletteRole::ConiferMature ? 4 : 3;
            bool Ready = !Keys.Contains(Key) && EntityReady && !ExpectedPath.IsEmpty()
                && Instance->ResourceId == Node.id && Instance->Visual.MeshPath == ExpectedPath
                && Instance->Visual.PaletteRole == static_cast<int32>(Entity.paletteRole)
                && Instance->Visual.VariantIndex == Entity.variantIndex && Batch && Batch->IsRegistered()
                && !Batch->IsQueryCollisionEnabled() && Mesh && Mesh->GetPathName() == ExpectedPath
                && Mesh->GetStaticMaterials().Num() == ExpectedSlots
                && Body && Body->AggGeom.SphylElems.Num() == 1
                && Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() == 3;
            double AnchorError = -1;
            bool TransformFound = false;
            bool CollisionExact = false;
            if (Ready)
            {
                TreeMeshes.Add(Mesh->GetPathName());
                const auto& Capsule = Body->AggGeom.SphylElems[0];
                UStaticMesh* ExpectedMesh = nullptr;
                FHomesteadOuterTreeInstance ExpectedVisual;
                Ready &= Landscape->ResolveGeneratedTreeVisual(Node, ExpectedMesh, ExpectedVisual)
                    && ExpectedMesh == Mesh;
                const FTransform& ExpectedTransform = ExpectedVisual.Transform;
                AnchorError = FVector::Dist(
                    Instance->Visual.Transform.GetLocation(), ExpectedTransform.GetLocation());
                for (int32 Index = 0; Index < Batch->GetInstanceCount(); ++Index)
                {
                    FTransform Actual;
                    if (Batch->GetInstanceTransform(Index, Actual)
                        && Actual.Equals(ExpectedTransform, 0.001f))
                    {
                        TransformFound = true;
                        break;
                    }
                }
                const FTransform ExpectedCollision(
                    ExpectedTransform.TransformRotation(Capsule.Rotation.Quaternion()),
                    ExpectedTransform.TransformPosition(Capsule.Center));
                CollisionExact = Collision->GetRelativeTransform().Equals(ExpectedCollision, 0.001f)
                    && FMath::IsNearlyEqual(Collision->GetUnscaledCapsuleRadius(),
                        Capsule.GetScaledRadius(ExpectedTransform.GetScale3D()), 0.001f)
                    && FMath::IsNearlyEqual(Collision->GetUnscaledCapsuleHalfHeight(),
                        Capsule.GetScaledHalfLength(ExpectedTransform.GetScale3D()), 0.001f);
                Ready &= AnchorError < 0.15 && Instance->Visual.Transform.Equals(ExpectedTransform, 0.001f)
                    && TransformFound && CollisionExact && Collision->IsRegistered()
                    && Collision->IsQueryCollisionEnabled() && !Collision->GetGenerateOverlapEvents()
                    && !Collision->CanEverAffectNavigation()
                    && Collision->GetCollisionProfileName() == UCollisionProfile::BlockAll_ProfileName
                    && Collision->GetCollisionResponseToChannel(ECC_Pawn) == ECR_Block
                    && Collision->GetCollisionResponseToChannel(ECC_Camera) == ECR_Block
                    && Collision->GetOwner() == Landscape && Landscape->GetActorEnableCollision();
                Ready &= TreeMaterialsReady(Batch);
            }
            Valid &= Ready;
            Keys.Add(Key);
            ++ExpectedActiveByMesh.FindOrAdd(Instance->Visual.MeshPath);
            FString& Representative = RepresentativeActiveKey.FindOrAdd(Instance->Visual.MeshPath);
            if (Representative.IsEmpty() || Key < Representative) Representative = Key;
            auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("key"), Key); Row->SetBoolField(TEXT("active"), true);
            Row->SetNumberField(TEXT("paletteRole"), static_cast<int32>(Entity.paletteRole));
            Row->SetNumberField(TEXT("variantIndex"), Entity.variantIndex);
            Row->SetStringField(TEXT("mesh"), Instance->Visual.MeshPath);
            Row->SetBoolField(TEXT("renderTransformFound"), TransformFound);
            Row->SetBoolField(TEXT("collisionExact"), CollisionExact);
            Row->SetBoolField(TEXT("ready"), Ready); Row->SetNumberField(TEXT("anchorErrorCm"), AnchorError);
            Row->SetStringField(TEXT("collisionComponent"), GetPathNameSafe(Collision));
            Trees.Add(MakeShared<FJsonValueObject>(Row));
        }
    for (const auto& Tile : Landscape->TerrainChunks)
    {
        if (Tile.Value.bCollision) continue;
        Gen::ChunkBaseline Baseline;
        if (Gen::GenerateChunk(State.world, {Tile.Key.X, Tile.Key.Y}, Baseline) != Gen::Status::Ok)
        { Valid = false; continue; }
        for (const auto& Entity : Baseline.entities)
        {
            if (Entity.kind != Gen::EntityKind::ForestTree) continue;
            Homestead::ResourceNode Node;
            const auto Result = PC->Simulation().ResolveGeneratedResource(Entity.key, Node);
            if (Result.code == Homestead::ResultCode::Unavailable) continue;
            if (!Result) { Valid = false; continue; }
            const FString Key = FString::Printf(TEXT("%d,%d,%u"), Node.key.chunk.x, Node.key.chunk.y, Node.key.localId);
            const auto* Instance = Landscape->OuterTreeInstances.Find(Key);
            if (Node.cleared) { Valid &= !Instance; continue; }
            ++OuterTrees;
            if (!Instance) { Valid = false; continue; }
            const FString ExpectedPath = ExpectedTreePath(Entity.paletteRole);
            auto* Batch = Landscape->OuterTreeBatches.FindRef(Instance->MeshPath).Get();
            UStaticMesh* Mesh = Batch ? Batch->GetStaticMesh() : nullptr;
            auto* Body = Mesh ? Mesh->GetBodySetup() : nullptr;
            const int32 ExpectedSlots = Entity.paletteRole == Gen::TreePaletteRole::ConiferMature ? 4 : 3;
            bool Ready = !Keys.Contains(Key) && !ExpectedPath.IsEmpty()
                && Instance->MeshPath == ExpectedPath && Instance->PaletteRole == static_cast<int32>(Entity.paletteRole)
                && Instance->VariantIndex == Entity.variantIndex && Batch && Batch->IsRegistered()
                && !Batch->IsQueryCollisionEnabled() && !Batch->GetGenerateOverlapEvents()
                && !Batch->CanEverAffectNavigation() && Mesh && Mesh->GetPathName() == ExpectedPath
                && Mesh->GetStaticMaterials().Num() == ExpectedSlots && Body
                && Body->AggGeom.SphylElems.Num() == 1 && Mesh->GetRenderData()
                && Mesh->GetRenderData()->LODResources.Num() == 3;
            double AnchorError = -1;
            bool TransformExact = false;
            if (Ready)
            {
                UStaticMesh* ExpectedMesh = nullptr;
                FHomesteadOuterTreeInstance ExpectedVisual;
                Ready &= Landscape->ResolveGeneratedTreeVisual(Node, ExpectedMesh, ExpectedVisual)
                    && ExpectedMesh == Mesh;
                const FTransform& Expected = ExpectedVisual.Transform;
                AnchorError = FVector::Dist(Instance->Transform.GetLocation(), Expected.GetLocation());
                TransformExact = Instance->Transform.Equals(Expected, 0.001f);
                Ready &= TransformExact;
                Ready &= TreeMaterialsReady(Batch);
            }
            Valid &= Ready;
            Keys.Add(Key);
            TreeMeshes.Add(Instance->MeshPath);
            ++ExpectedOuterByMesh.FindOrAdd(Instance->MeshPath);
            FString& Representative = RepresentativeOuterKey.FindOrAdd(Instance->MeshPath);
            if (Representative.IsEmpty() || Key < Representative) Representative = Key;
            auto Row = MakeShared<FJsonObject>();
            Row->SetStringField(TEXT("key"), Key); Row->SetBoolField(TEXT("active"), false);
            Row->SetNumberField(TEXT("paletteRole"), static_cast<int32>(Entity.paletteRole));
            Row->SetNumberField(TEXT("variantIndex"), Entity.variantIndex);
            Row->SetStringField(TEXT("mesh"), Instance->MeshPath);
            Row->SetBoolField(TEXT("ready"), Ready); Row->SetBoolField(TEXT("transformExact"), TransformExact);
            Row->SetNumberField(TEXT("transformPositionErrorCm"), AnchorError);
            Trees.Add(MakeShared<FJsonValueObject>(Row));
        }
    }
    TArray<UHierarchicalInstancedStaticMeshComponent*> RegisteredActiveBatches;
    Landscape->GetComponents(RegisteredActiveBatches);
    RegisteredActiveBatches.RemoveAll([](const UHierarchicalInstancedStaticMeshComponent* Batch)
    {
        return !Batch || !Batch->ComponentHasTag(TEXT("GeneratedActiveTreeBatch"));
    });
    TArray<UCapsuleComponent*> RegisteredActiveCollisions;
    Landscape->GetComponents(RegisteredActiveCollisions);
    RegisteredActiveCollisions.RemoveAll([](const UCapsuleComponent* Collision)
    {
        return !Collision || !Collision->ComponentHasTag(TEXT("GeneratedForestTreeCollision"));
    });
    ActiveCollisions = RegisteredActiveCollisions.Num();
    for (const auto& Entry : Landscape->ActiveTreeBatches)
    {
        auto* Batch = Entry.Value.Get();
        const int32 Expected = ExpectedActiveByMesh.FindRef(Entry.Key);
        const int32 Actual = Batch ? Batch->GetInstanceCount() : -1;
        ActiveBatchInstances += FMath::Max(Actual, 0);
        const int32 ExpectedMinLOD = AHomesteadWorld::ActiveMatureTreeMinLOD;
        bool Ready = Batch && Batch->GetStaticMesh() && Batch->GetStaticMesh()->GetPathName() == Entry.Key
            && Expected > 0 && Actual == Expected && !Batch->IsQueryCollisionEnabled()
            && !Batch->GetGenerateOverlapEvents() && !Batch->CanEverAffectNavigation()
            && Batch->GetOverrideMinLOD() && Batch->GetMinLOD() == ExpectedMinLOD
            && Batch->GetForcedLodModel() == 0 && Batch->GetStaticMesh()->GetRenderData()
            && Batch->GetStaticMesh()->GetRenderData()->LODResources.Num() == 3;
        Ready &= TreeMaterialsReady(Batch);
        const FString RepresentativeKey = RepresentativeActiveKey.FindRef(Entry.Key);
        bool RepresentativeFound = false;
        if (const auto* Representative = Landscape->ActiveTreeInstances.Find(RepresentativeKey))
            for (int32 Index = 0; Batch && Index < Batch->GetInstanceCount(); ++Index)
            {
                FTransform ActualTransform;
                if (Batch->GetInstanceTransform(Index, ActualTransform)
                    && ActualTransform.Equals(Representative->Visual.Transform, 0.001f))
                {
                    RepresentativeFound = true;
                    break;
                }
            }
        Ready &= RepresentativeFound;
        Valid &= Ready;
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("mesh"), Entry.Key);
        Row->SetNumberField(TEXT("expectedInstances"), Expected);
        Row->SetNumberField(TEXT("actualInstances"), Actual);
        Row->SetBoolField(TEXT("overrideMinLOD"), Batch && Batch->GetOverrideMinLOD());
        Row->SetNumberField(TEXT("minLOD"), Batch ? Batch->GetMinLOD() : -1);
        Row->SetNumberField(TEXT("expectedMinLOD"), ExpectedMinLOD);
        Row->SetNumberField(TEXT("forcedLODModel"), Batch ? Batch->GetForcedLodModel() : -1);
        Row->SetNumberField(TEXT("lodCount"), Batch && Batch->GetStaticMesh()
            && Batch->GetStaticMesh()->GetRenderData()
            ? Batch->GetStaticMesh()->GetRenderData()->LODResources.Num() : 0);
        Row->SetStringField(TEXT("representativeKey"), RepresentativeKey);
        Row->SetBoolField(TEXT("representativeTransformFound"), RepresentativeFound);
        Row->SetBoolField(TEXT("ready"), Ready);
        ActiveBatches.Add(MakeShared<FJsonValueObject>(Row));
    }
    TArray<UHierarchicalInstancedStaticMeshComponent*> RegisteredOuterBatches;
    Landscape->GetComponents(RegisteredOuterBatches);
    RegisteredOuterBatches.RemoveAll([](const UHierarchicalInstancedStaticMeshComponent* Batch)
    {
        return !Batch || !Batch->ComponentHasTag(TEXT("GeneratedOuterTreeBatch"));
    });
    for (const auto& Entry : Landscape->OuterTreeBatches)
    {
        auto* Batch = Entry.Value.Get();
        const int32 Expected = ExpectedOuterByMesh.FindRef(Entry.Key);
        const int32 Actual = Batch ? Batch->GetInstanceCount() : -1;
        OuterBatchInstances += FMath::Max(Actual, 0);
        bool Ready = Batch && Batch->GetStaticMesh() && Batch->GetStaticMesh()->GetPathName() == Entry.Key
            && Expected > 0 && Actual == Expected && !Batch->IsQueryCollisionEnabled()
            && !Batch->GetGenerateOverlapEvents() && !Batch->CanEverAffectNavigation()
            && Batch->GetOverrideMinLOD() && Batch->GetMinLOD() == AHomesteadWorld::OuterMatureTreeMinLOD
            && Batch->GetForcedLodModel() == 0 && Batch->GetStaticMesh()->GetRenderData()
            && Batch->GetStaticMesh()->GetRenderData()->LODResources.Num() == 3;
        Ready &= TreeMaterialsReady(Batch);
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("mesh"), Entry.Key);
        Row->SetNumberField(TEXT("expectedInstances"), Expected);
        Row->SetNumberField(TEXT("actualInstances"), Actual);
        Row->SetBoolField(TEXT("overrideMinLOD"), Batch && Batch->GetOverrideMinLOD());
        Row->SetNumberField(TEXT("minLOD"), Batch ? Batch->GetMinLOD() : -1);
        Row->SetNumberField(TEXT("forcedLODModel"), Batch ? Batch->GetForcedLodModel() : -1);
        Row->SetNumberField(TEXT("lodCount"), Batch && Batch->GetStaticMesh()
            && Batch->GetStaticMesh()->GetRenderData()
            ? Batch->GetStaticMesh()->GetRenderData()->LODResources.Num() : 0);
        const FString RepresentativeKey = RepresentativeOuterKey.FindRef(Entry.Key);
        Row->SetStringField(TEXT("representativeKey"), RepresentativeKey);
        bool RepresentativeFound = false;
        if (const auto* Representative = Landscape->OuterTreeInstances.Find(RepresentativeKey))
        {
            Row->SetStringField(TEXT("representativeTransform"), Representative->Transform.ToString());
            for (int32 Index = 0; Batch && Index < Batch->GetInstanceCount(); ++Index)
            {
                FTransform ActualTransform;
                if (Batch->GetInstanceTransform(Index, ActualTransform)
                    && ActualTransform.Equals(Representative->Transform, 0.001f))
                {
                    RepresentativeFound = true;
                    break;
                }
            }
        }
        Ready &= RepresentativeFound;
        Valid &= Ready;
        Row->SetBoolField(TEXT("representativeTransformFound"), RepresentativeFound);
        Row->SetBoolField(TEXT("ready"), Ready);
        OuterBatches.Add(MakeShared<FJsonValueObject>(Row));
    }
    for (const auto& Entry : Landscape->RegionalWaterMeshes)
    {
        auto* Water = Entry.Value.Get();
        const auto* Section = Water ? Water->GetProcMeshSection(0) : nullptr;
        RegionalWaterPoliciesValid &= Water && Water->IsRegistered()
            && Water->ComponentHasTag(TEXT("GeneratedRegionalWater"))
            && !Water->IsQueryCollisionEnabled() && !Water->GetGenerateOverlapEvents()
            && !Water->CanEverAffectNavigation() && !Water->CastShadow
            && Section && Section->ProcVertexBuffer.Num() >= 4
            && Section->ProcIndexBuffer.Num() >= 6
            && MaterialReady(Water->GetMaterial(0));
        ++RegionalWaterComponents;
    }
    Valid &= Colliding == 9 && PositionError < 0.15 && NormalError < 0.0001
        && ActiveTrees > 0 && ActiveTrees == Landscape->ActiveTreeInstances.Num()
        && ActiveTrees == ActiveBatchInstances && ActiveTrees == ActiveCollisions
        && Landscape->ActiveTreeBatches.Num() == ExpectedActiveByMesh.Num()
        && RegisteredActiveBatches.Num() == Landscape->ActiveTreeBatches.Num()
        && OuterTrees == Landscape->OuterTreeInstances.Num()
        && OuterTrees == OuterBatchInstances && Landscape->OuterTreeBatches.Num() == ExpectedOuterByMesh.Num()
        && RegisteredOuterBatches.Num() == Landscape->OuterTreeBatches.Num()
        && TreeMeshes.Num() >= 3 && Grass > 0 && Ferns > 0 && Flowers > 0
        && Grass == ExpectedGrass && Ferns == ExpectedFerns && Flowers == ExpectedFlowers
        && GrassBatchComponents == Landscape->TerrainChunks.Num() * 4
        && FernBatchComponents == Landscape->TerrainChunks.Num() * 4
        && FlowerBatchComponents == Landscape->TerrainChunks.Num() * 2
        && CoverPoliciesValid && CoverRepresentativeTransformsExact
        && RegionalOffsetMin <= RegionalOffsetMax
        && RegionalRidgeMin <= RegionalRidgeMax && RegionalValleyMin <= RegionalValleyMax
        && Landscape->RegionalDescriptors.LoadedRegionCount() >= 1
        && Landscape->RegionalDescriptors.LoadedRegionCount() <= 4
        && Landscape->RegionalDescriptors.CachedRegionCount()
            <= Landscape->RegionalDescriptors.LoadedRegionCount()
        && RegionalDescriptorsConsistent
        && RegionalWaterComponents == Landscape->RenderedRegionalReachReferences
        && Landscape->RenderedRegionalReachReferences > 0
        && Landscape->RenderedRegionalReachReferences <= 5
        && Landscape->UnrenderedRegionalReachReferences >= 0
        && Landscape->UnrenderedRegionalLakeReferences >= 0
        && RegionalWaterPoliciesValid;
    auto Evidence = MakeShared<FJsonObject>();
    Evidence->SetStringField(TEXT("seed"), FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(State.world.seed)));
    Evidence->SetNumberField(TEXT("generationVersion"), State.world.generationVersion);
    Evidence->SetBoolField(TEXT("valid"), Valid);
    Evidence->SetNumberField(TEXT("collidingChunks"), Colliding);
    Evidence->SetNumberField(TEXT("terrainPositionErrorCm"), PositionError);
    Evidence->SetNumberField(TEXT("terrainNormalError"), NormalError);
    Evidence->SetNumberField(TEXT("regionalOffsetMinCm"), RegionalOffsetMin);
    Evidence->SetNumberField(TEXT("regionalOffsetMaxCm"), RegionalOffsetMax);
    Evidence->SetNumberField(TEXT("regionalRidgeMin"), RegionalRidgeMin);
    Evidence->SetNumberField(TEXT("regionalRidgeMax"), RegionalRidgeMax);
    Evidence->SetNumberField(TEXT("regionalValleyMin"), RegionalValleyMin);
    Evidence->SetNumberField(TEXT("regionalValleyMax"), RegionalValleyMax);
    Evidence->SetNumberField(TEXT("regionalLoadedRegionCount"),
        Landscape->RegionalDescriptors.LoadedRegionCount());
    Evidence->SetNumberField(TEXT("regionalCachedDescriptorCount"),
        Landscape->RegionalDescriptors.CachedRegionCount());
    Evidence->SetNumberField(TEXT("regionalCacheRefreshCount"),
        Landscape->RegionalDescriptors.RefreshCount());
    Evidence->SetNumberField(TEXT("regionalCacheStoreCount"),
        Landscape->RegionalDescriptors.StoreCount());
    Evidence->SetNumberField(TEXT("regionalCacheEvictionCount"),
        Landscape->RegionalDescriptors.EvictionCount());
    Evidence->SetNumberField(TEXT("regionalDescriptorBuildCount"),
        Landscape->RegionalDescriptorBuildCount);
    Evidence->SetNumberField(TEXT("regionalDescriptorFailureCount"),
        Landscape->RegionalDescriptorFailures.size());
    Evidence->SetBoolField(TEXT("regionalDescriptorBuildPending"),
        Landscape->RegionalDescriptorBuild != nullptr);
    Evidence->SetNumberField(TEXT("regionalReadyChunkCount"), RegionalReadyChunks);
    Evidence->SetNumberField(TEXT("regionalPartialChunkCount"), RegionalPartialChunks);
    Evidence->SetNumberField(TEXT("regionalIncompleteChunkCount"), RegionalIncompleteChunks);
    Evidence->SetNumberField(TEXT("regionalReachReferenceCount"), RegionalReachReferences);
    Evidence->SetNumberField(TEXT("regionalDistinctReachCount"), RegionalReachSignatures.Num());
    Evidence->SetNumberField(TEXT("regionalLakeReferenceCount"), RegionalLakeReferences);
    Evidence->SetNumberField(TEXT("regionalDistinctLakeCount"), RegionalLakeSignatures.Num());
    Evidence->SetBoolField(TEXT("regionalDescriptorsConsistent"), RegionalDescriptorsConsistent);
    Evidence->SetStringField(TEXT("renderedRegionalReachKey"), Landscape->RenderedRegionalReachKey);
    Evidence->SetNumberField(TEXT("regionalWaterComponents"), RegionalWaterComponents);
    Evidence->SetNumberField(TEXT("renderedRegionalReachReferences"),
        Landscape->RenderedRegionalReachReferences);
    Evidence->SetNumberField(TEXT("unrenderedRegionalReachReferences"),
        Landscape->UnrenderedRegionalReachReferences);
    Evidence->SetNumberField(TEXT("unrenderedRegionalLakeReferences"),
        Landscape->UnrenderedRegionalLakeReferences);
    Evidence->SetBoolField(TEXT("regionalWaterPoliciesValid"), RegionalWaterPoliciesValid);
    Evidence->SetBoolField(TEXT("regionalWaterRendered"), false);
    Evidence->SetNumberField(TEXT("activeTrees"), ActiveTrees); Evidence->SetNumberField(TEXT("outerTrees"), OuterTrees);
    Evidence->SetNumberField(TEXT("activeBatchComponents"), Landscape->ActiveTreeBatches.Num());
    Evidence->SetNumberField(TEXT("activeBatchInstances"), ActiveBatchInstances);
    Evidence->SetNumberField(TEXT("activeCollisionCapsules"), ActiveCollisions);
    Evidence->SetNumberField(TEXT("outerBatchComponents"), Landscape->OuterTreeBatches.Num());
    Evidence->SetNumberField(TEXT("outerBatchInstances"), OuterBatchInstances);
    Evidence->SetNumberField(TEXT("distinctMatureTreeMeshes"), TreeMeshes.Num());
    TArray<TSharedPtr<FJsonValue>> MeshPaths;
    for (const FString& Path : TreeMeshes) MeshPaths.Add(MakeShared<FJsonValueString>(Path));
    Evidence->SetArrayField(TEXT("matureTreeMeshes"), MeshPaths);
    Evidence->SetNumberField(TEXT("grass"), Grass); Evidence->SetNumberField(TEXT("ferns"), Ferns);
    Evidence->SetNumberField(TEXT("decorativeFlowers"), Flowers);
    Evidence->SetNumberField(TEXT("expectedGrass"), ExpectedGrass);
    Evidence->SetNumberField(TEXT("expectedFerns"), ExpectedFerns);
    Evidence->SetNumberField(TEXT("expectedDecorativeFlowers"), ExpectedFlowers);
    Evidence->SetNumberField(TEXT("grassBatchComponents"), GrassBatchComponents);
    Evidence->SetNumberField(TEXT("fernBatchComponents"), FernBatchComponents);
    Evidence->SetNumberField(TEXT("flowerBatchComponents"), FlowerBatchComponents);
    Evidence->SetBoolField(TEXT("coverPoliciesValid"), CoverPoliciesValid);
    Evidence->SetBoolField(TEXT("coverRepresentativeTransformsExact"), CoverRepresentativeTransformsExact);
    Evidence->SetNumberField(TEXT("processPhysicalBytes"), FPlatformMemory::GetStats().UsedPhysical);
    Evidence->SetArrayField(TEXT("tiles"), Tiles); Evidence->SetArrayField(TEXT("trees"), Trees);
    Evidence->SetArrayField(TEXT("activeBatches"), ActiveBatches);
    Evidence->SetArrayField(TEXT("outerBatches"), OuterBatches);
    Evidence->SetStringField(TEXT("limits"), TEXT("Loaded CPU geometry/material/physics inventory, not renderer-selected LOD, GPU cost, continuous process history or visual approval."));
    FString Text;
    const bool Written = FJsonSerializer::Serialize(Evidence, TJsonWriterFactory<>::Create(&Text))
        && FFileHelper::SaveStringToFile(Text, *FPaths::Combine(OutputDirectory, TEXT("generated-inventory.json")));
    if (!Valid || !Written) Observations.Add(TEXT("FAILED generated terrain/tree/cover inventory or persistence."));
    Observations.Add(FString::Printf(TEXT("Generated inventory valid=%d; chunks=%d collision=%d active_trees=%d active_batches=%d active_capsules=%d outer_trees=%d outer_batches=%d grass=%d/%d grass_batches=%d ferns=%d/%d fern_batches=%d flowers=%d/%d flower_batches=%d"),
        Valid && Written, Landscape->TerrainChunks.Num(), Colliding, ActiveTrees,
        Landscape->ActiveTreeBatches.Num(), ActiveCollisions, OuterTrees,
        Landscape->OuterTreeBatches.Num(), Grass, ExpectedGrass, GrassBatchComponents,
        Ferns, ExpectedFerns, FernBatchComponents, Flowers, ExpectedFlowers,
        FlowerBatchComponents));
}
