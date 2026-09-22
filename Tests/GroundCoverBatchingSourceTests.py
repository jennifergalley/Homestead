from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
WORLD = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.cpp").read_text(encoding="utf-8")
INVENTORY = (
    ROOT / "Source" / "SurvivalGame" / "HomesteadGeneratedVisual.cpp"
).read_text(encoding="utf-8")


def method(source, signature, next_signature):
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


class GroundCoverBatchingSourceTests(unittest.TestCase):
    def test_generated_ferns_are_chunk_owned_hism_batches(self):
        body = method(
            WORLD,
            "bool AHomesteadWorld::BuildDecorations",
            "void AHomesteadWorld::ClearVisual",
        )
        self.assertNotIn("NewObject<UStaticMeshComponent>", body)
        self.assertIn(
            "TMap<FString, UHierarchicalInstancedStaticMeshComponent*> FernBatches",
            body,
        )
        self.assertIn("FernBatches.Add(Mesh->GetPathName()", body)
        self.assertIn("FernBatches.FindChecked(Mesh->GetPathName())->AddInstance", body)
        self.assertIn("Chunk.Value.Cover.Components.Add(Batch)", body)
        self.assertIn(
            "for (const auto& Batch : FernBatches) Batch.Value->BuildTreeIfOutdated(false, true)",
            body,
        )

    def test_low_cover_keeps_nonblocking_policy_and_disables_only_its_shadows(self):
        body = method(
            WORLD,
            "bool AHomesteadWorld::BuildDecorations",
            "void AHomesteadWorld::ClearVisual",
        )
        for contract in (
            "SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName)",
            "SetCollisionEnabled(ECollisionEnabled::NoCollision)",
            "SetGenerateOverlapEvents(false)",
            "SetCanEverAffectNavigation(false)",
            "SetVisibility(true)",
            "SetHiddenInGame(false)",
            "SetCastShadow(false)",
            "CreateCoverBatch(Mesh, GrassTag, 3500, 5000)",
            "CreateCoverBatch(Mesh, FernTag, 0, 5000)",
        ):
            self.assertIn(contract, body)

    def test_streamed_chunk_lifecycle_destroys_owned_cover(self):
        terrain = method(
            WORLD,
            "bool AHomesteadWorld::BuildTerrain",
            "UProceduralMeshComponent* AHomesteadWorld::BuildTerrainChunk",
        )
        self.assertLess(
            terrain.index("ClearVisual(It.Value().Cover)"),
            terrain.index("It.RemoveCurrent()"),
        )
        clear = method(
            WORLD,
            "void AHomesteadWorld::ClearVisual",
            "bool AHomesteadWorld::ResolveGeneratedTreeVisual",
        )
        self.assertIn("Component->DestroyComponent()", clear)
        self.assertIn("Visual.Components.Reset()", clear)

    def test_inventory_recomputes_seeded_counts_and_checks_batch_contracts(self):
        for contract in (
            "ExpectedGrass",
            "ExpectedFerns",
            "GetTypeHash(State.world.seed) ^ GetTypeHash(Entry.Key)",
            "Grass == ExpectedGrass && Ferns == ExpectedFerns",
            "GrassBatchComponents == Landscape->TerrainChunks.Num() * 4",
            "FernBatchComponents == Landscape->TerrainChunks.Num() * 4",
            "CoverPoliciesValid && CoverRepresentativeTransformsExact",
            "Batch->InstanceStartCullDistance == 3500",
            "Batch->InstanceEndCullDistance == 5000",
            "Batch->GetCollisionEnabled() == ECollisionEnabled::NoCollision",
            "!Batch->bHiddenInGame",
            "!Batch->CastShadow",
        ):
            self.assertIn(contract, INVENTORY)


if __name__ == "__main__":
    unittest.main()
