from pathlib import Path
import unittest


ROOT = Path(__file__).resolve().parents[1]
HEADER = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.h").read_text(encoding="utf-8")
WORLD = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.cpp").read_text(encoding="utf-8")
INVENTORY = (ROOT / "Source" / "SurvivalGame" / "HomesteadGeneratedVisual.cpp").read_text(encoding="utf-8")
TASKS = (
    ROOT / "openspec" / "changes" / "add-persistent-generated-woodland" / "tasks.md"
).read_text(encoding="utf-8")


def method(source, signature, next_signature):
    start = source.index(signature)
    end = source.index(next_signature, start)
    return source[start:end]


class GeneratedTreeLodPolicySourceTests(unittest.TestCase):
    def test_policy_constants_and_hism_creation_are_narrowly_scoped(self):
        self.assertIn("ActiveMatureTreeMinLOD = 1", HEADER)
        self.assertIn("OuterMatureTreeMinLOD = 2", HEADER)
        active = method(
            WORLD,
            "bool AHomesteadWorld::RebuildActiveTreeBatches",
            "void AHomesteadWorld::BuildResource",
        )
        outer = method(
            WORLD,
            "bool AHomesteadWorld::RebuildOuterTreeBatches",
            "void AHomesteadWorld::ClearActiveTreeBatches",
        )
        decorations = method(
            WORLD,
            "bool AHomesteadWorld::BuildDecorations",
            "void AHomesteadWorld::ClearVisual",
        )
        resources = method(
            WORLD,
            "void AHomesteadWorld::BuildResource",
            "void AHomesteadWorld::BuildStructure",
        )
        self.assertIn("Batch->bOverrideMinLOD = true;", active)
        self.assertIn("TreePaletteRole::BroadleafMature", active)
        self.assertIn("? 0 : ActiveMatureTreeMinLOD;", active)
        self.assertIn("Batch->bOverrideMinLOD = true;", outer)
        self.assertIn("Batch->MinLOD = OuterMatureTreeMinLOD;", outer)
        self.assertNotIn("MinLOD", decorations)
        self.assertNotIn("MinLOD", resources)
        self.assertNotIn("SetForcedLodModel", active)
        self.assertNotIn("SetForcedLodModel", outer)

    def test_active_collision_capsules_and_batch_lifecycle_are_unchanged(self):
        active = method(
            WORLD,
            "bool AHomesteadWorld::RebuildActiveTreeBatches",
            "void AHomesteadWorld::BuildResource",
        )
        clear_active = method(
            WORLD,
            "void AHomesteadWorld::ClearActiveTreeBatches",
            "bool AHomesteadWorld::RebuildActiveTreeBatches",
        )
        for contract in (
            "NewObject<UCapsuleComponent>(this)",
            "SetCapsuleSize(Entry.Instance.CapsuleRadius, Entry.Instance.CapsuleHalfHeight, false)",
            "SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName)",
            "SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics)",
            "PreparedCollisions.Add(Entry.Key, Collision)",
            "ActiveTreeCollisions.Add(Entry.Key, Entry.Value)",
        ):
            self.assertIn(contract, active)
        for contract in (
            "Entry.Value->DestroyComponent()",
            "ActiveTreeBatches.Reset()",
            "ActiveTreeCollisions.Reset()",
            "ActiveTreeInstances.Reset()",
            "ActiveTreeLayoutSignature.Reset()",
        ):
            self.assertIn(contract, clear_active)

    def test_inventory_asserts_policy_and_three_lods(self):
        for contract in (
            "Batch->GetOverrideMinLOD()",
            "Batch->GetMinLOD() == ExpectedMinLOD",
            'Entry.Key.Contains(TEXT("SM_TreeSmall02_Woodland"))',
            "Batch->GetMinLOD() == AHomesteadWorld::OuterMatureTreeMinLOD",
            "Batch->GetForcedLodModel() == 0",
            'TEXT("overrideMinLOD")',
            'TEXT("minLOD")',
            'TEXT("forcedLODModel")',
            'TEXT("lodCount")',
            "LODResources.Num() == 3",
            'TEXT("expectedInstances")',
            'TEXT("actualInstances")',
            'TEXT("representativeTransformFound")',
        ):
            self.assertIn(contract, INVENTORY)

    def test_openspec_tracks_completed_policy(self):
        self.assertIn(
            "- [x] 2.2d Apply component-local minimum authored LODs",
            TASKS,
        )


if __name__ == "__main__":
    unittest.main()
