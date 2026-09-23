"""Source-contract checks only; not a substitute for Unreal compile/playtesting."""
from pathlib import Path
import re
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Source" / "SurvivalGame"
CONTROLLER = (SOURCE / "HomesteadController.cpp").read_text()
MENU = (SOURCE / "UI" / "SHomesteadMenu.cpp").read_text()
ICONS = (SOURCE / "UI" / "SHomesteadIcon.cpp").read_text()


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
        self.assertIn("++PromptDeviceChanges", body)

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
        self.assertIn('Title = TEXT("Quit game")', MENU)
        self.assertIn('Labels = {TEXT("Save & Quit"), TEXT("Quit without Saving")}', MENU)

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
        keys = re.findall(r'TEXT\("([^"]+)"\)', declaration.group(1))
        self.assertEqual(len(keys), 16)
        self.assertEqual(keys[-2:], ["timber", "firewood"])
        recipes = re.search(r"const TCHAR\* RecipeIcons\[\] = \{(.*?)\};", MENU, re.S)
        self.assertIsNotNone(recipes)
        self.assertEqual(re.findall(r'TEXT\("([^"]+)"\)', recipes.group(1))[-1], "firewood")
        self.assertIn('{FName(TEXT("timber")), EKind::Timber}', ICONS)
        self.assertIn('{FName(TEXT("firewood")), EKind::Firewood}', ICONS)
        for key, kind in (("slot-torso", "SlotTorso"), ("slot-apron", "SlotApron"), ("slot-feet", "SlotFeet")):
            self.assertIn(f'{{FName(TEXT("{key}")), EKind::{kind}}}', ICONS)
            self.assertIn(f"case EKind::{kind}:", ICONS)

    def test_timber_recipe_and_dual_fuel_are_player_visible(self):
        self.assertIn('TEXT(" Add firewood / branch")', CONTROLLER)
        self.assertIn("Split timber with a carried hatchet.", CONTROLLER)
        self.assertIn("Cookfires use prepared firewood first, then branches.", CONTROLLER)

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

    def test_item_grid_has_fixed_compact_cells_without_single_column_stretch(self):
        self.assertIn("constexpr float ItemCellWidth = 76", MENU)
        self.assertIn("HAlign(SeenPage <= 2 ? HAlign_Left : HAlign_Fill)", MENU)
        self.assertIn("WidthOverride(SeenPage <= 2 ? FOptionalSize(ItemCellWidth)", MENU)
        self.assertIn("FText::AsNumber(FMath::Max(1, Row.Quantity))", MENU)
        self.assertIn('Kind(FName(TEXT("sort")))', MENU)
        self.assertIn("Scroll->GetScrollOffsetOfEnd()", MENU)
        self.assertIn("PointerDragRevision != Controller->Simulation().GetRevision()", MENU)
        self.assertNotIn('TEXT("Nearby chest")', MENU)
        self.assertNotIn('TEXT("Carried"), TEXT("Nearby chest"), TEXT("Wearing")', MENU)
        details = function_body(MENU, "TSharedRef<SWidget> SHomesteadMenu::BuildDetails(")
        item_actions = details[details.index("if (Entries.IsValidIndex(ContentSelection))"):]
        self.assertNotIn("Actions.Add(EHomesteadItemAction::Transfer)", item_actions)
        self.assertNotIn("Actions.Add(EHomesteadItemAction::Split)", item_actions)
        self.assertNotIn("Actions.Add(EHomesteadItemAction::Merge)", item_actions)
        self.assertNotIn("Actions.Add(EHomesteadItemAction::MoveEarlier)", item_actions)

    def test_detail_actions_are_inside_the_clipped_scroll_region(self):
        details = function_body(MENU, "TSharedRef<SWidget> SHomesteadMenu::BuildDetails(")
        self.assertIn("SAssignNew(DetailsScroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)", details)
        self.assertIn("SAssignNew(DetailsContent, SVerticalBox)", details)
        self.assertIn("DetailsContent->AddSlot()", details)
        self.assertIn("EntryName(Entries[Index])", details)
        self.assertIn('TEXT("%s: %d"), *Row.Location, Row.Quantity', details)
        self.assertIn("DetailsScroll->ScrollDescendantIntoView(ActionButtons[ActionSelection]", MENU)

    def test_explicit_button_brush_and_compact_named_portrait_controls(self):
        self.assertIn(".SetNormal(FSlateColorBrush(FLinearColor::White))", MENU)
        self.assertIn("ButtonStyle(&MenuButtonStyle())", MENU)
        self.assertIn('TEXT("Turn character left")', MENU)
        self.assertIn('TEXT("Turn character right")', MENU)
        self.assertNotIn('TEXT("< Turn")', MENU)
        self.assertNotIn('TEXT("Turn >")', MENU)

    def test_portrait_framing_and_exposure_are_capture_local(self):
        portrait = (SOURCE / "UI" / "HomesteadMenuPortrait.cpp").read_text()
        self.assertIn("GetBounds().GetBox().TransformBy(ReferenceTransform)", portrait)
        self.assertIn("SubjectExtent.Z / VerticalTangent", portrait)
        self.assertIn("AutoExposureMethod = AEM_Manual", portrait)
        self.assertIn("Light->SetIntensityUnits(ELightUnits::Lumens)", portrait)
        self.assertIn("Capture->ShowFlags.SetSkyLighting(false)", portrait)
        self.assertNotIn("IConsoleManager", portrait)
        self.assertNotIn("SetViewMode", portrait)

    def test_context_actions_select_native_subjects(self):
        self.assertIn("case EFocus::Chest: OpenChestStorage(FocusId);", CONTROLLER)
        self.assertIn("ActiveChestId = ChestId", CONTROLLER)
        self.assertIn("NativeMenu->FocusLegacySubject(Selection)", CONTROLLER)
        self.assertIn("Homestead::Recipe::RoastedRoots", CONTROLLER)

    def test_shipping_gate_preserves_native_routes(self):
        self.assertIn("HomesteadAutomatedActorsEnabled()", CONTROLLER)
        self.assertIn("SHIPPING_QA_REJECTED", CONTROLLER)
        smoke = (SOURCE / "HomesteadSmokeTest.cpp").read_text()
        self.assertIn("PrepareNativeMenuChecks();", smoke)
        self.assertIn("const bool Native = Controller->HasNativeMenu();", smoke)
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

    def test_equipment_slots_use_original_explicit_icons(self):
        for key in ("slot-torso", "slot-apron", "slot-feet"):
            self.assertIn(f'TEXT("{key}")', MENU)
        self.assertIn("SNew(SHomesteadIcon).Kind(FName(SlotIcons[Index]))", MENU)

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
        self.assertIn("ProducerOutput.Mid(1, ProducerOutput.Len() - 2)", fixture)
        self.assertIn("ResumeRequested || !Token.StartsWith(ResumePrefix", fixture)
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

    def test_directional_edges_use_native_focus_navigation(self):
        self.assertIn("HomesteadMenuNavigation::Move(", MENU)
        self.assertIn("Slate.ProcessReply(Path, FReply::Handled().SetNavigation(", MENU)
        self.assertIn("FNavigationReply::Stop()", MENU)
        self.assertIn("SetKeyboardFocus(Target, EFocusCause::Navigation)", MENU)
        self.assertIn("Button->SetOnFocusReceived(", MENU)

    def test_left_stick_uses_shared_navigation_not_portrait_rotation(self):
        handler = function_body(MENU, "bool SHomesteadMenu::HandleKey(")
        self.assertIn("LeftStick.Sample(true, InputAmount", handler)
        self.assertIn("LeftStick.Sample(false, -InputAmount", handler)
        self.assertLess(handler.index("LeftStick.Sample("), handler.index("LeftStick.Poll("))
        self.assertIn("NavigateDirection(Direction)", handler)
        tick = function_body(MENU, "void SHomesteadMenu::Tick(")
        self.assertIn("LeftStick.Poll(", tick)
        self.assertIn("NavigateDirection(Direction)", tick)
        navigation = function_body(MENU, "void SHomesteadMenu::NavigateDirection(")
        self.assertNotIn("OrbitMenuPortrait", navigation)
        self.assertNotIn("MenuItemAction", navigation)

    def test_focused_controls_bubble_analog_through_shared_admission(self):
        controls = MENU[MENU.index("class SMenuButton"):MENU.index("const FLinearColor Ink")]
        self.assertEqual(controls.count("OnAnalogValueChanged("), 2)
        self.assertEqual(controls.count("return FReply::Unhandled();"), 2)
        self.assertNotIn("SNew(SButton)", MENU)
        analog = function_body(MENU, "FReply SHomesteadMenu::OnAnalogValueChanged(")
        self.assertIn("MenuPhysicalInput(", analog)

    def test_portrait_uses_live_native_content_destination(self):
        anchor = function_body(MENU, "TSharedRef<SWidget> SHomesteadMenu::FocusAnchor(")
        self.assertIn("TargetRegion == ERegion::Portrait && Index == -1", anchor)
        self.assertIn("EUINavigation::Right, EUINavigationRule::Custom", anchor)
        self.assertIn("Entries.IsEmpty() ? -1 : ContentSelection", anchor)
        self.assertIn("IsTargetAvailable(Target.region, Target.index)", anchor)
        self.assertIn("Widget->SupportsKeyboardFocus()", anchor)
        self.assertNotIn("Region = ", anchor)
        self.assertIn("Select(Index, Index == ContentSelection)", MENU)

    def test_quantity_has_explicit_edit_and_parent_focus_trap(self):
        self.assertIn("Root->SetEnabled(Value == EDialog::None)", MENU)
        self.assertIn("Dialog == EDialog::Amount && bEditingAmount", MENU)
        self.assertIn("DialogSelection < 0", MENU)
        self.assertIn("activate to edit", MENU)
        footer = function_body(MENU, "FString SHomesteadMenu::Footer(")
        self.assertIn("D-pad / Left stick", footer)
        self.assertNotIn("LT/RT  Regions", footer)

    def test_native_navigation_checks_send_real_directional_events(self):
        fixture = (SOURCE / "UI" / "HomesteadDirectionalNavigationTest.cpp").read_text()
        self.assertIn("Tap(EKeys::Gamepad_DPad_Down)", fixture)
        self.assertIn("SlateAxis(EKeys::Gamepad_LeftY, -0.9f)", fixture)
        self.assertIn("IsVirtualDraggingItem()", fixture)
        self.assertIn("Tap(EKeys::Gamepad_DPad_Right)", fixture)
        self.assertIn("ProcessMouseMoveEvent", fixture)
        self.assertIn("ProcessMouseButtonDownEvent", fixture)
        self.assertIn("PromptDeviceChangeCount()", fixture)
        self.assertIn("HasSynchronizedFocus()", fixture)
        self.assertIn("IsFocusedControlVisible()", fixture)
        self.assertNotIn("CycleRegion(", fixture)
        self.assertNotIn("AdoptFocus(", fixture)
        self.assertIn("ProcessKeyDownEvent(Event)", fixture)
        self.assertIn("ProcessAnalogInputEvent(", fixture)
        self.assertIn("TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true)", fixture)

    def test_visual_hair_review_uses_production_presentation(self):
        visual = (SOURCE / "HomesteadVisualPlaytest.cpp").read_text()
        script = (ROOT / "Scripts" / "Playtest-Visual.ps1").read_text()
        self.assertIn("HomesteadVisualBodyPreset=", visual)
        self.assertIn("Avatar->PrepareEquipment(PC->State(), Look, Error)", visual)
        self.assertIn("Avatar->ApplyPreparedEquipment(Error)", visual)
        self.assertIn("HomesteadVisualBodyPreset=$BodyPreset", script)
        native = (SOURCE / "UI" / "HomesteadNativeMenuTest.cpp").read_text()
        self.assertIn("for (int32 Id : {0, 1, 1, 1, 1, 2, 3, 6})", native)

    def test_chop_target_is_presentation_only(self):
        character = (SOURCE / "HomesteadCharacter.cpp").read_text()
        controller = CONTROLLER
        hatchet = (SOURCE / "HomesteadHatchet.cpp").read_text()
        self.assertIn("HomesteadWork_20260923_01/Animations/AN_Heroine_Chop", character)
        self.assertIn("ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X))", character)
        self.assertIn("Avatar->PlayClear(ActionTarget)", controller)
        self.assertIn("Avatar->ClearTargetYaw()", hatchet)
        self.assertNotIn("SetActorRotation", character[character.index("void AHomesteadCharacter::PlayClear(Homestead::Point"):])

    def test_till_target_and_prop_are_presentation_only(self):
        character = (SOURCE / "HomesteadCharacter.cpp").read_text()
        controller = CONTROLLER
        animation = (SOURCE / "HomesteadAnimInstance.cpp").read_text()
        prop = (SOURCE / "HomesteadDiggingStick.cpp").read_text()
        self.assertIn("HomesteadWork_20260923_01/Animations/AN_Heroine_Till", character)
        self.assertIn("TillYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X))", character)
        self.assertIn("const auto Result = Sim.Till(X, Y, Position)", controller)
        self.assertIn("if (Result.ok)", controller[controller.index("const auto Result = Sim.Till"):])
        self.assertIn("Avatar->PlayTill({(X + 0.5) * Homestead::CellSize", controller)
        self.assertIn("EHandAction { None, Gather, Water, Clear, Till }", animation)
        self.assertIn("SetCollisionEnabled(ECollisionEnabled::NoCollision)", prop)
        self.assertIn("SetGenerateOverlapEvents(false)", prop)
        self.assertIn("SetCanEverAffectNavigation(false)", prop)
        self.assertNotIn("SetActorRotation", character[character.index("void AHomesteadCharacter::PlayTill(Homestead::Point"):])

    def test_water_target_is_presentation_only(self):
        character = (SOURCE / "HomesteadCharacter.cpp").read_text()
        controller = CONTROLLER
        tool = (SOURCE / "HomesteadWateringTool.cpp").read_text()
        self.assertIn("HomesteadWork_20260923_01/Animations/AN_Heroine_WaterRefined", character)
        self.assertIn("WaterYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X))", character)
        self.assertIn("Avatar->PlayWater(Homestead::CellCenter(Plot.cellX, Plot.cellY))", controller)
        self.assertIn("Avatar->WaterTargetYaw()", tool)
        self.assertNotIn("SetActorRotation", character[character.index("void AHomesteadCharacter::PlayWater(Homestead::Point"):])

    def test_full_loop_resolves_generated_keys_after_region_prepare(self):
        fixture = (SOURCE / "HomesteadFullLoopTest.cpp").read_text()
        gather = fixture[fixture.index("void AHomesteadSmokeTest::QueueGatherTo"):
                         fixture.index("void AHomesteadSmokeTest::QueueCraft")]
        self.assertIn("const auto Key = Candidate.key", gather)
        self.assertLess(gather.index("Teleport(Position)"), gather.rindex("ResolveGeneratedResource(Key, Current)"))
        self.assertIn("Controller->IsResourceFocused(*CurrentId)", gather)
        self.assertIn("!Controller->Simulation().CanHarvest(*CurrentId)", gather)
        smoke = (SOURCE / "HomesteadSmokeTest.cpp").read_text()
        harvest = smoke[smoke.index("void AHomesteadSmokeTest::QueueHarvest"):
                        smoke.index("void AHomesteadSmokeTest::Screenshot")]
        self.assertIn("Key = Node.key", harvest)
        self.assertLess(harvest.index("Teleport(Position)"),
                        harvest.index("ResolveGeneratedResource(Key, Current)"))
        self.assertIn("Controller->IsResourceFocused(*CurrentId)", harvest)


if __name__ == "__main__":
    unittest.main()
