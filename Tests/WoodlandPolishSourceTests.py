from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.h").read_text()
WORLD = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.cpp").read_text()


class WoodlandPolishSourceTests(unittest.TestCase):
    def test_chunk_generation_is_cached_and_halo_prepared_off_thread(self):
        self.assertIn("ChunkBaselineCache", HEADER)
        self.assertIn("TFuture<FHomesteadChunkBaselineBuild>", HEADER)
        self.assertIn("EAsyncExecution::ThreadPool", WORLD)
        self.assertIn("QueueChunkBaselineBuild(State.world, State.activeChunk)", WORLD)
        self.assertIn("GetChunkBaseline(State.world", WORLD)
        terrain = WORLD[WORLD.index("bool AHomesteadWorld::BuildTerrain"):
                        WORLD.index("void AHomesteadWorld::QueueRegionalDescriptorBuild")]
        self.assertNotIn("RegionalGeneration::GenerateRegion", terrain)

    def test_cover_and_outer_trees_do_not_resolve_every_generated_entity(self):
        cover = WORLD[WORLD.index("bool AHomesteadWorld::BuildDecorations"):
                      WORLD.index("void AHomesteadWorld::ClearVisual")]
        outer = WORLD[WORLD.index("bool AHomesteadWorld::RebuildOuterTreeBatches"):
                      WORLD.index("void AHomesteadWorld::ClearActiveTreeBatches")]
        self.assertNotIn("Simulation.ResolveGeneratedResource", cover)
        self.assertNotIn("Simulation.ResolveGeneratedResource", outer)
        self.assertIn("State.resourceEdits", cover)
        self.assertIn("Simulation.GetState().resourceEdits", outer)

    def test_tree_grounding_samples_role_specific_root_footprints(self):
        resolve = WORLD[WORLD.index("bool AHomesteadWorld::ResolveGeneratedTreeVisual"):
                        WORLD.index("void AHomesteadWorld::ClearOuterTreeBatches")]
        self.assertIn("const float Radius = Capsule.Radius * Scale", resolve)
        self.assertIn("TreePaletteRole::ConiferMature ? 14.0f", resolve)
        self.assertIn("TreePaletteRole::WoodlandAccent ? 9.0f : 7.0f", resolve)
        self.assertGreaterEqual(resolve.count("GroundHeight("), 2)
        self.assertIn("RootGround - Embed", resolve)

    def test_tree_lod_policy_remains_bounded(self):
        active = WORLD[WORLD.index("bool AHomesteadWorld::RebuildActiveTreeBatches"):
                       WORLD.index("void AHomesteadWorld::BuildResource")]
        outer = WORLD[WORLD.index("bool AHomesteadWorld::RebuildOuterTreeBatches"):
                      WORLD.index("void AHomesteadWorld::ClearActiveTreeBatches")]
        self.assertIn("Batch->MinLOD = ActiveMatureTreeMinLOD", active)
        self.assertIn("Batch->MinLOD = OuterMatureTreeMinLOD", outer)

    def test_lighting_preserves_authored_role_colors_at_dawn_and_day(self):
        self.assertIn("AutoExposureBias = -0.15f", WORLD)
        self.assertIn("FLinearColor(1.0f, 0.76f, 0.56f)", WORLD)
        self.assertIn("FLinearColor(1.0f, 0.99f, 0.95f)", WORLD)
        self.assertIn("Sky->SetIntensity(FMath::Lerp(0.35f, 1.0f, Daylight))", WORLD)


if __name__ == "__main__":
    unittest.main()
