#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadWorld.h"
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
    int32 Grass = 0, Ferns = 0;
    double PositionError = 0, NormalError = 0;
    TArray<TSharedPtr<FJsonValue>> Tiles, Trees, ActiveBatches, OuterBatches;
    auto MaterialReady = [](UMaterialInterface* Interface)
    {
        auto* Material = Interface ? Interface->GetMaterial() : nullptr;
        auto* Resource = Material ? Material->GetMaterialResource(GMaxRHIShaderPlatform) : nullptr;
        return Material && Material->GetPathName().StartsWith(TEXT("/Game/"))
            && Resource && Resource->GetGameThreadShaderMap() && Resource->IsGameThreadShaderMapComplete();
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
        for (const auto& Component : Entry.Value.Cover.Components)
        {
            if (const auto* Batch = Cast<UHierarchicalInstancedStaticMeshComponent>(Component))
            {
                Grass += Batch->GetInstanceCount();
                Valid &= !Batch->IsQueryCollisionEnabled() && MaterialReady(Batch->GetMaterial(0));
            }
            else if (const auto* Mesh = Cast<UStaticMeshComponent>(Component))
            {
                ++Ferns;
                Valid &= !Mesh->IsQueryCollisionEnabled() && MaterialReady(Mesh->GetMaterial(0));
            }
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
                const float Scale = Entity.scalePermille / 1000.0f;
                const FRotator Rotation(0, Entity.yawDegrees, 0);
                const FVector AnchorLocal(Capsule.Center.X, Capsule.Center.Y, Mesh->GetBoundingBox().Min.Z);
                const FTransform ExpectedTransform(Rotation,
                    FVector(Node.position.x, Node.position.y, PC->GroundHeight(Node.position.x, Node.position.y))
                        - Rotation.RotateVector(AnchorLocal * Scale),
                    FVector(Scale));
                const FVector Anchor = Instance->Visual.Transform.TransformPosition(
                    FVector(Capsule.Center.X, Capsule.Center.Y, Mesh->GetBoundingBox().Min.Z));
                AnchorError = FVector::Dist(Anchor, FVector(Node.position.x, Node.position.y,
                    PC->GroundHeight(Node.position.x, Node.position.y)));
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
                for (int Slot = 0; Slot < ExpectedSlots; ++Slot)
                    Ready &= MaterialReady(Batch->GetMaterial(Slot));
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
                const auto& Capsule = Body->AggGeom.SphylElems[0];
                const float Scale = Entity.scalePermille / 1000.0f;
                const FRotator Rotation(0, Entity.yawDegrees, 0);
                const FVector Anchor(Capsule.Center.X, Capsule.Center.Y, Mesh->GetBoundingBox().Min.Z);
                const FTransform Expected(Rotation,
                    FVector(Node.position.x, Node.position.y, PC->GroundHeight(Node.position.x, Node.position.y))
                        - Rotation.RotateVector(Anchor * Scale),
                    FVector(Scale));
                AnchorError = FVector::Dist(Instance->Transform.GetLocation(), Expected.GetLocation());
                TransformExact = Instance->Transform.Equals(Expected, 0.001f);
                Ready &= TransformExact;
                for (int Slot = 0; Slot < ExpectedSlots; ++Slot)
                    Ready &= MaterialReady(Batch->GetMaterial(Slot));
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
        bool Ready = Batch && Batch->GetStaticMesh() && Batch->GetStaticMesh()->GetPathName() == Entry.Key
            && Expected > 0 && Actual == Expected && !Batch->IsQueryCollisionEnabled()
            && !Batch->GetGenerateOverlapEvents() && !Batch->CanEverAffectNavigation();
        if (Batch && Batch->GetStaticMesh())
            for (int32 Slot = 0; Slot < Batch->GetStaticMesh()->GetStaticMaterials().Num(); ++Slot)
                Ready &= MaterialReady(Batch->GetMaterial(Slot));
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
            && !Batch->GetGenerateOverlapEvents() && !Batch->CanEverAffectNavigation();
        if (Batch && Batch->GetStaticMesh())
            for (int32 Slot = 0; Slot < Batch->GetStaticMesh()->GetStaticMaterials().Num(); ++Slot)
                Ready &= MaterialReady(Batch->GetMaterial(Slot));
        auto Row = MakeShared<FJsonObject>();
        Row->SetStringField(TEXT("mesh"), Entry.Key);
        Row->SetNumberField(TEXT("expectedInstances"), Expected);
        Row->SetNumberField(TEXT("actualInstances"), Actual);
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
    Valid &= Colliding == 9 && PositionError < 0.15 && NormalError < 0.0001
        && ActiveTrees > 0 && ActiveTrees == Landscape->ActiveTreeInstances.Num()
        && ActiveTrees == ActiveBatchInstances && ActiveTrees == ActiveCollisions
        && Landscape->ActiveTreeBatches.Num() == ExpectedActiveByMesh.Num()
        && RegisteredActiveBatches.Num() == Landscape->ActiveTreeBatches.Num()
        && OuterTrees == Landscape->OuterTreeInstances.Num()
        && OuterTrees == OuterBatchInstances && Landscape->OuterTreeBatches.Num() == ExpectedOuterByMesh.Num()
        && RegisteredOuterBatches.Num() == Landscape->OuterTreeBatches.Num()
        && TreeMeshes.Num() >= 3 && Grass > 0 && Ferns > 0;
    auto Evidence = MakeShared<FJsonObject>();
    Evidence->SetStringField(TEXT("seed"), FString::Printf(TEXT("%llu"), static_cast<unsigned long long>(State.world.seed)));
    Evidence->SetNumberField(TEXT("generationVersion"), State.world.generationVersion);
    Evidence->SetBoolField(TEXT("valid"), Valid);
    Evidence->SetNumberField(TEXT("collidingChunks"), Colliding);
    Evidence->SetNumberField(TEXT("terrainPositionErrorCm"), PositionError);
    Evidence->SetNumberField(TEXT("terrainNormalError"), NormalError);
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
    Evidence->SetNumberField(TEXT("processPhysicalBytes"), FPlatformMemory::GetStats().UsedPhysical);
    Evidence->SetArrayField(TEXT("tiles"), Tiles); Evidence->SetArrayField(TEXT("trees"), Trees);
    Evidence->SetArrayField(TEXT("activeBatches"), ActiveBatches);
    Evidence->SetArrayField(TEXT("outerBatches"), OuterBatches);
    Evidence->SetStringField(TEXT("limits"), TEXT("Loaded CPU geometry/material/physics inventory, not renderer-selected LOD, GPU cost, continuous process history or visual approval."));
    FString Text;
    const bool Written = FJsonSerializer::Serialize(Evidence, TJsonWriterFactory<>::Create(&Text))
        && FFileHelper::SaveStringToFile(Text, *FPaths::Combine(OutputDirectory, TEXT("generated-inventory.json")));
    if (!Valid || !Written) Observations.Add(TEXT("FAILED generated terrain/tree/cover inventory or persistence."));
    Observations.Add(FString::Printf(TEXT("Generated inventory valid=%d; chunks=%d collision=%d active_trees=%d active_batches=%d active_capsules=%d outer_trees=%d outer_batches=%d grass=%d ferns=%d"),
        Valid && Written, Landscape->TerrainChunks.Num(), Colliding, ActiveTrees,
        Landscape->ActiveTreeBatches.Num(), ActiveCollisions, OuterTrees,
        Landscape->OuterTreeBatches.Num(), Grass, Ferns));
}
