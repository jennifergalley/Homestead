import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
CONTROLLER_H = (ROOT / "Source" / "SurvivalGame" / "HomesteadController.h").read_text()
CONTROLLER = (ROOT / "Source" / "SurvivalGame" / "HomesteadController.cpp").read_text()
INVENTORY = (ROOT / "Source" / "SurvivalGame" / "UI" / "HomesteadMenuInventory.cpp").read_text()
MENU = (ROOT / "Source" / "SurvivalGame" / "UI" / "SHomesteadMenu.cpp").read_text()
WORLD_H = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.h").read_text()
WORLD = (ROOT / "Source" / "SurvivalGame" / "HomesteadWorld.cpp").read_text()


def body(source, start, end):
    return source[source.index(start):source.index(end)]


class WorldDropSourceTests(unittest.TestCase):
    def test_contextual_drop_is_pack_only_and_modal(self):
        details = body(MENU, "TSharedRef<SWidget> SHomesteadMenu::BuildDetails(", "FString SHomesteadMenu::RowKey(")
        self.assertGreaterEqual(details.count("Row.ContainerId == 0"), 2)
        self.assertIn("Actions.Add(EHomesteadItemAction::Drop)", details)
        self.assertIn("SetDialog(EDialog::DropWearable)", MENU)
        self.assertIn("Action == EHomesteadItemAction::Drop", MENU)
        self.assertIn("MaximumAmount = Row.Quantity", MENU)

    def test_drop_transactions_capture_revision_and_authority(self):
        self.assertIn("PendingRevision = Controller->Simulation().GetRevision()", MENU)
        self.assertIn("ExpectedRevision != Sim.GetRevision()", INVENTORY)
        self.assertIn("Sim.DropGroup(Row.SubjectId, Amount, DropPoint", INVENTORY)
        self.assertIn("Sim.DropWearable(Row.SubjectId, DropPoint", INVENTORY)
        self.assertNotIn("worldDrops.push_back", INVENTORY)

    def test_safe_placement_is_bounded_and_collision_aware(self):
        placement = body(CONTROLLER, "bool AHomesteadController::ResolveDropPoint(", "bool AHomesteadController::PrepareWorldAt(")
        self.assertIn("static constexpr float Angles[]", placement)
        self.assertIn("static constexpr float Distances[]", placement)
        self.assertIn("Homestead::IsNearWater(Candidate)", placement)
        self.assertIn("OverlapAnyTestByChannel", placement)
        self.assertIn("ECC_WorldStatic", placement)
        self.assertIn("ResolveDropPoint", CONTROLLER_H)

    def test_drop_visual_lifecycle_and_cover_reservation(self):
        self.assertIn("TMap<int32, FHomesteadWorldVisual> DropVisuals", WORLD_H)
        refresh = body(WORLD, "bool AHomesteadWorld::Refresh(", "void AHomesteadWorld::SetPlacementPreview(")
        self.assertIn("RemoveMissing(DropVisuals, NearDrops)", refresh)
        self.assertIn("BuildDrop(Visual, Drop)", refresh)
        visual = body(WORLD, "void AHomesteadWorld::BuildDrop(", "void AHomesteadWorld::UpdateLighting(")
        self.assertIn("AddPart(Visual", visual)
        self.assertNotIn("SetCollision", visual)
        reserve = body(WORLD, "bool AHomesteadWorld::IsDecorationReserved(", "bool AHomesteadWorld::BuildDecorations(")
        self.assertIn("State.worldDrops", reserve)

    def test_focus_uses_the_stable_drop_id_and_exact_pickup(self):
        focus = body(CONTROLLER, "void AHomesteadController::UpdateFocus()", "FString AHomesteadController::FocusTitle()")
        self.assertIn("Consider(EFocus::Drop, Drop.id, Drop.position)", focus)
        interact = body(CONTROLLER, "void AHomesteadController::Interact()", "bool AHomesteadController::OpenChestStorage(")
        self.assertIn("Sim.PickUpDrop(FocusId, Position)", interact)
        self.assertIn('case EFocus::Drop: return A + TEXT(" Pick up")', CONTROLLER)


if __name__ == "__main__":
    unittest.main()
