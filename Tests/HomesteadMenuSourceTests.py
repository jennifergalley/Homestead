"""Source-contract checks only; not a substitute for Unreal compile/playtesting."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source" / "SurvivalGame"
CONTROLLER = (SOURCE / "HomesteadController.cpp").read_text()
MENU = (SOURCE / "UI" / "SHomesteadMenu.cpp").read_text()


def function_body(source, signature):
    start = source.index("{", source.index(signature))
    depth = 0
    for index in range(start, len(source)):
        if source[index] == "{":
            depth += 1
        elif source[index] == "}":
            depth -= 1
            if depth == 0:
                return source[start:index + 1]
    raise AssertionError(f"Unclosed function {signature}")


class MenuSourceContracts(unittest.TestCase):
    def test_single_prompt_classifier(self):
        self.assertEqual(CONTROLLER.count("PromptIntent.Classify("), 1)
        self.assertNotIn("PromptIntent.Classify(", MENU)
        body = function_body(CONTROLLER, "bool AHomesteadController::InputKey(")
        self.assertLess(body.index("bAutomatedInputOnly"), body.index("PromptIntent.Classify("))
        self.assertLess(body.index("PromptIntent.Classify("), body.index("Menu->HandleKey("))

    def test_exit_gated_on_real_save(self):
        body = function_body(CONTROLLER, "void AHomesteadController::MenuSaveAndQuit(")
        self.assertIn('SaveSlot(TEXT("Homestead_Manual"))', body)
        self.assertIn("if (Saved)", body)
        self.assertLess(body.index("if (Saved)"), body.index("UKismetSystemLibrary::QuitGame"))
        self.assertIn("ShowSaveFailure(ToastText)", body)
        self.assertIn("if (IsFailed())", body)

    def test_no_save_in_explicit_discard(self):
        body = function_body(CONTROLLER, "void AHomesteadController::MenuQuitWithoutSaving(")
        self.assertNotIn("SaveSlot(", body)
        self.assertIn("QuitGame(", body)

    def test_recovery_and_pinned_exit_are_independent(self):
        self.assertIn("Retry checkpoint  [A / Enter]", MENU)
        self.assertIn("Settings / Quit  [Y / G]", MENU)
        self.assertIn("Rows[Index].Id == 9) continue", MENU)
        self.assertIn("Save and quit to desktop", MENU)

    def test_no_label_parsing_for_item_identity(self):
        body = function_body(MENU, "FString SHomesteadMenu::EntryName(")
        self.assertIn("Row.Id", body)
        self.assertNotRegex(MENU, r"(ParseIntoArray|Split|Find)\([^;\n]*Row.Label")

    def test_modal_cancel_is_default_and_failure_persistent(self):
        body = function_body(MENU, "void SHomesteadMenu::SetDialog(")
        self.assertIn("DialogSelection = 0", body)
        self.assertIn("Retry save and quit", MENU)
        self.assertNotIn("ToastRemaining", MENU)

    def test_shell_suppresses_canvas_menu(self):
        hud = (SOURCE / "HomesteadHUD.cpp").read_text()
        self.assertIn("if (PC->HasNativeMenu()) return;", hud)

    def test_icon_keys_cover_existing_items(self):
        declaration = re.search(r"const TCHAR\* ItemIcons\[\] = \{(.*?)\};", MENU, re.S)
        self.assertIsNotNone(declaration)
        self.assertEqual(len(re.findall(r'TEXT\("([^"]+)"\)', declaration.group(1))), 14)

    def test_current_schema_and_explicit_reset(self):
        body = function_body(CONTROLLER, "UHomesteadSave* AHomesteadController::ReadSave(")
        self.assertIn("Save->IsCurrentVersion()", body)
        self.assertIn("Homestead::ResultCode::UnsupportedVersion", body)
        save = function_body(CONTROLLER, "bool AHomesteadController::SaveSlot(")
        self.assertIn("if (bTestResetRequired)", save)
        load = function_body(CONTROLLER, "bool AHomesteadController::LoadLatest(")
        self.assertNotIn("starting a new session", load)
        self.assertIn("!bHasPlayableSession", load)

    def test_inventory_adapter_uses_authority_and_captured_revision(self):
        adapter = (SOURCE / "UI" / "HomesteadMenuInventory.cpp").read_text()
        self.assertIn("ExpectedRevision != Sim.GetRevision()", adapter)
        for api in ("TransferGroup", "SplitGroup", "MergeGroups", "ReorderEntry", "MoveWearable"):
            self.assertIn(f"Sim.{api}(", adapter)
        self.assertIn("PendingRevision = Controller->Simulation().GetRevision()", MENU)
        self.assertNotIn("const_cast", adapter)

    def test_pointer_bridge_is_scoped_and_does_not_add_classifier(self):
        self.assertIn("RegisterInputPreProcessor(MenuPointerInput)", CONTROLLER)
        self.assertIn("UnregisterInputPreProcessor(MenuPointerInput)", CONTROLLER)
        self.assertEqual(CONTROLLER.count("PromptIntent.Classify("), 1)

    def test_equipment_prepares_before_authority_commit(self):
        adapter = (SOURCE / "UI" / "HomesteadMenuInventory.cpp").read_text()
        self.assertLess(adapter.index("Avatar->PrepareEquipment("), adapter.index("Result = Transaction(Sim)"))
        self.assertLess(adapter.index("Result = Transaction(Sim)"), adapter.index("Avatar->ApplyPreparedEquipment("))
        self.assertIn("Sim.EatGroup(Row.SubjectId, ExpectedRevision)", adapter)

    def test_world_and_graphics_errors_are_distinct(self):
        body = function_body(CONTROLLER, "void AHomesteadController::MenuSaveAndQuit(")
        self.assertIn("PendingResolutionScale.IsSet()", body)
        self.assertIn("ShowGraphicsSaveFailure(GraphicsSaveError)", body)
        self.assertIn("World saved; video preference not confirmed", MENU)

    def test_saved_timestamp_is_bounded_before_date_formatting(self):
        body = function_body(CONTROLLER, "UHomesteadSave* AHomesteadController::ReadSave(")
        self.assertIn("Save->SavedAtUtc < 0", body)
        self.assertIn("Save->SavedAtUtc > 253402300799LL", body)

    def test_owned_dye_is_visible_without_hover(self):
        adapter = (SOURCE / "UI" / "HomesteadMenuInventory.cpp").read_text()
        self.assertIn("Row.IconTint", adapter)
        self.assertIn('Row.Name += TEXT(" - ")', adapter)
        self.assertIn(".Tint(Row.IconTint)", MENU)

    def test_equipment_slots_use_actual_authority(self):
        self.assertIn("Controller->State().equipment[", MENU)
        self.assertIn("Homestead::EquipmentSlot::Torso", MENU)
        self.assertIn("Controller->Simulation().GetWearable(Id)", MENU)
        self.assertIn("Empty - choose from pack", MENU)

    def test_context_actions_select_native_subjects(self):
        self.assertIn("case EFocus::Chest: MenuInventoryView(1); OpenBook(0);", CONTROLLER)
        self.assertIn("NativeMenu->FocusLegacySubject(Selection)", CONTROLLER)
        self.assertIn("Homestead::Recipe::RoastedRoots", CONTROLLER)

    def test_shipping_gate_preserves_native_routes(self):
        self.assertIn("HomesteadAutomatedActorsEnabled()", CONTROLLER)
        self.assertIn("SHIPPING_QA_REJECTED", CONTROLLER)
        smoke = (SOURCE / "HomesteadSmokeTest.cpp").read_text()
        self.assertIn("PrepareNativeMenuChecks();", smoke)
        self.assertIn("Controller->HasNativeMenu(), false", smoke)
        self.assertIn("Shipping QA cancelled", smoke)

    def test_native_menu_requires_real_modular_presentation(self):
        fixture = (SOURCE / "UI" / "HomesteadNativeMenuTest.cpp").read_text()
        verify = function_body(fixture, "bool AHomesteadSmokeTest::VerifyNativeMenuPresentation(")
        self.assertIn("if (!Presentation || !Presentation->Ready", verify)
        self.assertNotIn("Prototype", verify)
        self.assertIn("Owned->owner != Homestead::WearableOwner::Equipped", verify)
        self.assertIn("Surface.Dye != Owned->dye", verify)
        self.assertIn("VisibleComponents != Presentation->Garments.Num() + 1", verify)
        self.assertIn("HomesteadLook::TunicTint(Owned->dye)", verify)

    def test_native_wardrobe_checks_use_ui_and_real_save_keys(self):
        fixture = (SOURCE / "UI" / "HomesteadNativeMenuTest.cpp").read_text()
        route = function_body(fixture, "void AHomesteadSmokeTest::PrepareNativeWardrobeChecks(")
        for operation in ("Expected->UnequipWearable", "Expected->EquipWearable", "Expected->RecolorWearable"):
            self.assertIn(operation, route)
        self.assertIn("Tap(EKeys::F5)", route)
        self.assertIn("Tap(EKeys::F9)", route)
        self.assertIn("Controller->TestQuickLoads == *LoadCalls", route)
        self.assertNotIn("Controller->Sim.", route)
        self.assertIn("Controller->Simulation().Serialize().c_str())) == Saved->Simulation", route)

    def test_resume_is_distinct_pinned_nonreparse_and_uses_f9(self):
        fixture = (SOURCE / "UI" / "HomesteadNativeMenuTest.cpp").read_text()
        self.assertIn("FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE", fixture)
        self.assertIn("GetDriveTypeW(*Root) != DRIVE_FIXED", fixture)
        self.assertIn('const FString Segment(TEXT("/Saved/Automation/"))', fixture)
        self.assertIn("static_cast<uint32>(Process) == FPlatformProcess::GetCurrentProcessId()", fixture)
        route = function_body(fixture, "void AHomesteadSmokeTest::PrepareNativeResumeChecks(")
        self.assertIn('Controller->SaveRoute.Mode != TEXT("test-sandbox")', route)
        self.assertIn("Tap(EKeys::F9)", route)
        self.assertIn("FMD5::HashBytes(Copied.GetData(), Copied.Num()) != Expected->Fingerprint", route)
        self.assertIn("Controller->WorldId != Expected->World", route)
        self.assertIn("Controller->WorldId == Expected->World", route)
        self.assertNotIn("Controller->ApplySave(", route)
        self.assertNotIn("Controller->Sim.Deserialize(", route)

    def test_portrait_reuses_shared_presentation(self):
        portrait = (SOURCE / "UI" / "HomesteadMenuPortrait.cpp").read_text()
        self.assertIn("Character.GetEquipmentPresentation()", portrait)
        self.assertIn("HomesteadWardrobePresentation::ApplySurface(", portrait)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)", portrait)
        self.assertIn("bCaptureEveryFrame = false", portrait)

    def test_ui_delimiters_balance(self):
        # Lexical smoke check only; the shared Unreal compiler remains authoritative.
        strip = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'', re.S)
        pairs = {")": "(", "]": "[", "}": "{"}
        for path in (SOURCE / "UI").glob("*.cpp"):
            text = strip.sub("", path.read_text())
            stack = []
            for character in text:
                if character in "([{":
                    stack.append(character)
                elif character in pairs:
                    self.assertTrue(stack, str(path))
                    self.assertEqual(stack.pop(), pairs[character], str(path))
            self.assertFalse(stack, str(path))


if __name__ == "__main__":
    unittest.main()
