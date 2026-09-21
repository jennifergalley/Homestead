#include "../HomesteadSmokeTest.h"
#include "../HomesteadController.h"
#include "SHomesteadMenu.h"
#include "HomesteadMenuPortrait.h"
#include "Components/SkeletalMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"

void AHomesteadSmokeTest::PrepareDirectionalNavigationChecks()
{
    const auto Before = MakeShared<std::string>();
    const auto Location = MakeShared<FVector>();
    const auto PortraitRotation = MakeShared<FRotator>();
    const auto ColumnCount = MakeShared<int32>(4);
    const auto SelectedId = MakeShared<int32>(0);
    const auto SlateTap = [this](FKey Key)
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        const FKeyEvent Event(Key, FModifierKeysState(), static_cast<uint32>(0), false, 0, 0);
        FSlateApplication::Get().ProcessKeyDownEvent(Event);
        FSlateApplication::Get().ProcessKeyUpEvent(Event);
    };
    const auto SlateAxis = [this](FKey Key, float Value)
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        FSlateApplication::Get().ProcessAnalogInputEvent(
            FAnalogInputEvent(Key, FModifierKeysState(), static_cast<uint32>(0), false, 0, 0, Value));
    };
    const auto PortraitPose = [this]()
    {
        TArray<USkeletalMeshComponent*> Parts;
        if (Controller->MenuPortrait) Controller->MenuPortrait->GetComponents(Parts);
        for (const auto* Part : Parts)
            if (Part->GetFName() == TEXT("PortraitBody")) return Part->GetRelativeRotation();
        return FRotator::ZeroRotator;
    };
    const auto Focused = [this](const TCHAR* Region)
    {
        return Controller->NativeMenu && Controller->NativeMenu->GetFocusedRegionName() == Region
            && Controller->NativeMenu->HasSynchronizedFocus() && Controller->NativeMenu->IsFocusedControlVisible();
    };
    const auto Open = [this](int32 View)
    {
        Controller->CloseBook(); Controller->MenuInventoryView(View); Controller->OpenBook(0);
    };
    const auto Fixture = [this, Open](int32 Groups)
    {
        // Disclosed authority-only fixture setup, not an ordinary gathering playthrough.
        const auto Resources = Controller->State().resources;
        for (const auto& Node : Resources)
        {
            if (Controller->Sim.UsedCapacity() >= Groups + 3) break;
            if (Node.kind == Homestead::ResourceKind::Branches && Controller->Sim.CanHarvest(Node.id))
                if (!Controller->Sim.Harvest(Node.id, Node.position))
                { Finish(false, TEXT("Navigation fixture could not gather its real branch stock.")); return; }
        }
        while (static_cast<int32>(Controller->Sim.GetLayout(0)->size()) < Groups)
        {
            int32 Group = 0;
            for (const auto& Entry : *Controller->Sim.GetLayout(0))
                if (Entry.groupId && Entry.quantity > 1) { Group = Entry.groupId; break; }
            if (!Group || !Controller->Sim.SplitGroup(0, Group, 1, Controller->PlayerPoint(), Controller->Sim.GetRevision()))
            { Finish(false, TEXT("Navigation fixture could not partition its existing stock.")); return; }
        }
        Open(0);
    };
    Add(TEXT("Directional navigation starts with actual native item focus"),
        [this, Open, Before, Location]() { Open(0); *Before = Controller->Sim.Serialize(); *Location = Controller->GetPawn()->GetActorLocation(); },
        [this, Focused, ColumnCount, PortraitRotation, PortraitPose]()
        {
            *ColumnCount = Controller->NativeMenu->GetContentColumnCount();
            *PortraitRotation = PortraitPose();
            return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedSubject() != nullptr;
        });
    Add(TEXT("Real D-pad Down crosses final carried row into equipment without trigger"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]() { return Focused(TEXT("Equipment")) && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Real D-pad Up reverses from equipment into carried items"),
        [this]() { Tap(EKeys::Gamepad_DPad_Up); },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Focused-widget Slate D-pad routing reaches the same equipment boundary exactly once"),
        [SlateTap]() { SlateTap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]() { return Focused(TEXT("Equipment")) && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Focused-widget Slate keyboard routing returns to content"),
        [SlateTap]() { SlateTap(EKeys::Up); },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Real left-stick Down crosses the same boundary"),
        [SlateAxis]() { SlateAxis(EKeys::Gamepad_LeftY, -0.9f); },
        [SlateAxis, Focused]() { SlateAxis(EKeys::Gamepad_LeftY, 0); return Focused(TEXT("Equipment")); }, 0.15f);
    Add(TEXT("Real left-stick Up returns without moving pawn or rotating portrait"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0.9f); },
        [this, Focused, Before, Location, PortraitRotation, PortraitPose]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            return Focused(TEXT("Content")) && Controller->Sim.Serialize() == *Before
                && Controller->GetPawn()->GetActorLocation().Equals(*Location, 0.01)
                && PortraitPose().Equals(*PortraitRotation, 0.01);
        }, 0.15f);
    Add(TEXT("Left boundary reaches visible portrait without rotation"),
        [this]() { Tap(EKeys::Gamepad_DPad_Left); },
        [Focused, PortraitRotation, PortraitPose]() { return Focused(TEXT("Portrait")) && PortraitPose().Equals(*PortraitRotation, 0.01); });
    Add(TEXT("Right reverses from portrait into the carried content"),
        [this]() { Tap(EKeys::Gamepad_DPad_Right); },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Right boundary reaches actual details or action controls"),
        [this]() { Tap(EKeys::Right); },
        [Focused]() { return Focused(TEXT("Details")) || Focused(TEXT("Actions")); });

    Add(TEXT("Prepare disclosed short-row fixture from real stock and splits"),
        [Fixture, ColumnCount]() { Fixture(*ColumnCount + 2); },
        [this, Focused, ColumnCount]() { return Focused(TEXT("Content")) && Controller->MenuRows().Num() == *ColumnCount + 2; });
    Add(TEXT("Navigate to the desired last column"),
        [this, ColumnCount]() { for (int32 Index = 1; Index < *ColumnCount; ++Index) Tap(EKeys::Right); },
        [this, ColumnCount]() { return Controller->NativeMenu->GetSelectedContentIndex() == *ColumnCount - 1; });
    Add(TEXT("Short final row retains desired column memory"),
        [this]() { Tap(EKeys::Down); },
        [this, ColumnCount, Focused]() { return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedContentIndex() == *ColumnCount + 1; });
    Add(TEXT("Up from short row restores original column"),
        [this]() { Tap(EKeys::Up); },
        [this, ColumnCount, Focused]() { return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedContentIndex() == *ColumnCount - 1; });
    Add(TEXT("Prepare disclosed full-row scrolled fixture"),
        [Fixture, ColumnCount, Before, this]() { Fixture(*ColumnCount * 6); *Before = Controller->Sim.Serialize(); },
        [this, ColumnCount, Focused]() { return Focused(TEXT("Content")) && Controller->MenuRows().Num() == *ColumnCount * 6; });
    for (int32 Row = 1; Row < 6; ++Row)
        Add(TEXT("Directional input traverses a real logical row and scrolls its widget into view"),
            [this]() { Tap(EKeys::Gamepad_DPad_Down); },
            [this, Row, ColumnCount, Focused]() { return Focused(TEXT("Content"))
                && Controller->NativeMenu->GetSelectedContentIndex() == Row * *ColumnCount; });
    Add(TEXT("Only the actual final scrolled row exits downward"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]() { return Focused(TEXT("Equipment")) && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Empty nearby-storage view has real explanatory focus"),
        [Open]() { Open(1); },
        [this, Focused]() { return Controller->MenuRows().IsEmpty() && Focused(TEXT("Content")); });
    Add(TEXT("Empty content can move down to equipment and back up"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [Focused]() { return Focused(TEXT("Equipment")); });
    Add(TEXT("Reverse returns to the empty-content focus anchor"),
        [this]() { Tap(EKeys::Gamepad_DPad_Up); },
        [this, Focused]() { return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedSubject() == nullptr; });

    Add(TEXT("Select a real splittable stack for modal navigation"),
        [this, Open, Before, SelectedId]()
        {
            Open(0); Tap(EKeys::Right);
            *Before = Controller->Sim.Serialize();
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            *SelectedId = Subject ? Subject->SubjectId : 0;
        },
        [this, Focused]() { const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Focused(TEXT("Content")) && Subject && Subject->Quantity > 2; });
    Add(TEXT("Mapped split opens a safe focused modal"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Top); },
        [this]() { return Controller->NativeMenu->HasActiveDialog() && Controller->NativeMenu->HasSynchronizedFocus()
            && !Controller->NativeMenu->IsEditingQuantity() && Controller->NativeMenu->GetDraftQuantity() == 1; });
    Add(TEXT("Focus browsing does not edit or activate quantity"),
        [this]() { Tap(EKeys::Up); Tap(EKeys::Right); },
        [this, Before]() { return !Controller->NativeMenu->IsEditingQuantity() && Controller->NativeMenu->GetDraftQuantity() == 1
            && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Scroll modal choices and return to a visible amount editor"),
        [this]()
        {
            for (int32 Index = 0; Index < 5; ++Index) Tap(EKeys::Down);
            for (int32 Index = 0; Index < 4; ++Index) Tap(EKeys::Up);
        },
        [this]() { return Controller->NativeMenu->HasActiveDialog() && Controller->NativeMenu->HasSynchronizedFocus()
            && Controller->NativeMenu->IsFocusedControlVisible() && !Controller->NativeMenu->IsEditingQuantity(); });
    Add(TEXT("Explicit amount activation enters editing"),
        [this]() { Tap(EKeys::Enter); },
        [this]() { return Controller->NativeMenu->IsEditingQuantity() && Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Actual left-stick input edits only the draft in edit mode"),
        [this]() { Axis(EKeys::Gamepad_LeftX, 0.9f); },
        [this, Before]() { Axis(EKeys::Gamepad_LeftX, 0); return Controller->NativeMenu->GetDraftQuantity() == 2
            && Controller->Sim.Serialize() == *Before; }, 0.15f);
    Add(TEXT("Back first leaves editing without closing modal"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return Controller->NativeMenu->HasActiveDialog() && !Controller->NativeMenu->IsEditingQuantity(); });
    Add(TEXT("Modal cancel returns real focus to the original stable subject"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, Before, SelectedId, Focused]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return !Controller->NativeMenu->HasActiveDialog() && Focused(TEXT("Content"))
                && Subject && Subject->SubjectId == *SelectedId && Controller->Sim.Serialize() == *Before;
        });
    Add(TEXT("Settings still begins on safe Resume control"),
        [this]() { Controller->CloseBook(); Tap(EKeys::Escape); },
        [Focused]() { return Focused(TEXT("Session")); });
    Add(TEXT("Existing Right then Activate still reaches quit confirmation"),
        [this]() { Tap(EKeys::Gamepad_DPad_Right); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->NativeMenu->IsExitPrompt() && Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Quit modal remains trapped and defaults to staying"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Focused]() { return Controller->IsBookOpen() && !Controller->NativeMenu->HasActiveDialog() && Focused(TEXT("Session")); });
}
