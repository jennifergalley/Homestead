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
    const auto SlateMouseMove = [this](FVector2D Delta)
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        const FVector2D From = Slate.GetCursorPos();
        const FVector2D To = From + Delta;
        Slate.SetCursorPos(To);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, To, From, TSet<FKey>(),
            EKeys::Invalid, 0, FModifierKeysState()));
    };
    const auto SlateMouseClick = [this]()
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        const FVector2D Position = Slate.GetCursorPos();
        TSet<FKey> Pressed;
        Pressed.Add(EKeys::LeftMouseButton);
        Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position, Pressed,
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
    };
    const auto PortraitPose = [this]()
    {
        return Controller->MenuPortrait ? FRotator(0, Controller->MenuPortrait->OrbitYaw(), 0) : FRotator::ZeroRotator;
    };
    const auto Focused = [this](const TCHAR* Region)
    {
        const bool Passed = Controller->NativeMenu && Controller->NativeMenu->GetFocusedRegionName() == Region
            && Controller->NativeMenu->HasSynchronizedFocus() && Controller->NativeMenu->IsFocusedControlVisible();
        if (!Passed && Controller->NativeMenu)
        {
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            const FVector2D Position = Widget ? FVector2D(Widget->GetCachedGeometry().GetAbsolutePosition()) : FVector2D::ZeroVector;
            const FVector2D Size = Widget ? FVector2D(Widget->GetCachedGeometry().GetAbsoluteSize()) : FVector2D::ZeroVector;
            Results.Add(FString::Printf(TEXT("FOCUS_CHECK wanted=%s actual=%s index=%d sync=%d visible=%d widget=%s position=(%.2f,%.2f) size=(%.2f,%.2f)"),
                Region, *Controller->NativeMenu->GetFocusedRegionName(), Controller->NativeMenu->GetSelectedContentIndex(),
                Controller->NativeMenu->HasSynchronizedFocus(), Controller->NativeMenu->IsFocusedControlVisible(),
                Widget ? *Widget->GetTypeAsString() : TEXT("none"), Position.X, Position.Y, Size.X, Size.Y));
        }
        return Passed;
    };
    const auto Open = [this](int32 View)
    {
        Controller->CloseBook(); Controller->MenuInventoryView(View); Controller->OpenBook(0);
    };
    const auto Capture = [this](const FString& Name)
    {
        Add(TEXT("Capture actual directional focus: ") + Name,
            [this, Name]() { Screenshot(Name); },
            [this]() { return Controller->NativeMenu && Controller->NativeMenu->HasSynchronizedFocus()
                && Controller->NativeMenu->IsFocusedControlVisible(); }, 0.8f);
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
    const auto DeviceChanges = MakeShared<uint32>(0);
    const auto PointerSubject = MakeShared<int32>(0);
    Add(TEXT("Controller focus establishes one accepted device transition baseline"),
        [this, DeviceChanges, PointerSubject]()
        {
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (Widget)
            {
                const auto Geometry = Widget->GetCachedGeometry();
                FSlateApplication::Get().SetCursorPos(
                    Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f);
            }
            *DeviceChanges = Controller->PromptDeviceChangeCount();
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            *PointerSubject = Subject ? Subject->SubjectId : 0;
        },
        [this]() { return Controller->UsesGamepad(); });
    Add(TEXT("Subthreshold real Slate mouse noise does not steal controller intent"),
        [SlateMouseMove]() { SlateMouseMove(FVector2D(0.1f, 0.1f)); },
        [this, DeviceChanges]() { return Controller->UsesGamepad()
            && Controller->PromptDeviceChangeCount() == *DeviceChanges; });
    Add(TEXT("Deliberate real Slate mouse movement switches intent exactly once without activation"),
        [SlateMouseMove]() { SlateMouseMove(FVector2D(2.0f, 0.0f)); },
        [this, DeviceChanges, Before, Location]() { return !Controller->UsesGamepad()
            && Controller->PromptDeviceChangeCount() == *DeviceChanges + 1
            && Controller->Sim.Serialize() == *Before
            && Controller->GetPawn()->GetActorLocation().Equals(*Location, 0.01); });
    Add(TEXT("Real mouse click selects the intended cell without action or click-through"),
        [SlateMouseClick]() { SlateMouseClick(); },
        [this, DeviceChanges, PointerSubject, Before, Location]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return !Controller->UsesGamepad() && Controller->PromptDeviceChangeCount() == *DeviceChanges + 1
                && Subject && Subject->SubjectId == *PointerSubject && Controller->BookPage() == 0
                && Controller->Sim.Serialize() == *Before
                && Controller->GetPawn()->GetActorLocation().Equals(*Location, 0.01);
        });
    Add(TEXT("Keyboard navigation retains keyboard-mouse intent without duplicate transition"),
        [SlateTap]() { SlateTap(EKeys::Right); },
        [this, DeviceChanges]() { return !Controller->UsesGamepad()
            && Controller->PromptDeviceChangeCount() == *DeviceChanges + 1; });
    Add(TEXT("Deliberate controller navigation switches intent exactly once"),
        [SlateTap]() { SlateTap(EKeys::Gamepad_DPad_Left); },
        [this, DeviceChanges]() { return Controller->UsesGamepad()
            && Controller->PromptDeviceChangeCount() == *DeviceChanges + 2; });
    Add(TEXT("Device switching leaves a fresh stable native content focus"),
        [Open]() { Open(0); },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Real D-pad Down crosses final carried row into equipment without trigger"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]() { return Focused(TEXT("Equipment")) && Controller->Sim.Serialize() == *Before; });
    Capture(TEXT("native-navigation-equipment"));
    Add(TEXT("Real D-pad Up reverses from equipment into carried items"),
        [this]() { Tap(EKeys::Gamepad_DPad_Up); },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Focused-widget Slate D-pad routing reaches the same equipment boundary exactly once"),
        [SlateTap]() { SlateTap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]() { return Focused(TEXT("Equipment")) && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Focused-widget Slate keyboard routing returns to content"),
        [SlateTap]() { SlateTap(EKeys::Up); },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Unadmitted focused-widget analog input cannot bypass automation isolation"),
        []()
        {
            for (const float Value : {-0.9f, 0.0f})
                FSlateApplication::Get().ProcessAnalogInputEvent(
                    FAnalogInputEvent(EKeys::Gamepad_LeftY, FModifierKeysState(), static_cast<uint32>(0), false, 0, 0, Value));
        },
        [Focused]() { return Focused(TEXT("Content")); });
    Add(TEXT("Real left-stick Down crosses the same boundary"),
        [SlateAxis]() { SlateAxis(EKeys::Gamepad_LeftY, -0.9f); },
        [SlateAxis, Focused]() { SlateAxis(EKeys::Gamepad_LeftY, 0); return Focused(TEXT("Equipment")); }, 0.15f);
    Add(TEXT("Real left-stick Up returns without moving pawn or rotating portrait"),
        [this]()
        {
            const double At = FPlatformTime::Seconds();
            const FString BeforeRegion = Controller->NativeMenu->GetFocusedRegionName();
            Axis(EKeys::Gamepad_LeftY, 0.9f);
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            Results.Add(FString::Printf(TEXT("INPUT stick-up value=0.9 monotonic=%.9f before=%s after_dispatch=%s widget=%s"),
                At, *BeforeRegion, *Controller->NativeMenu->GetFocusedRegionName(),
                Widget ? *Widget->GetTypeAsString() : TEXT("none")));
        },
        [this, Focused, Before, Location, PortraitRotation, PortraitPose]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            const bool Passed = Focused(TEXT("Content")) && Controller->Sim.Serialize() == *Before
                && Controller->GetPawn()->GetActorLocation().Equals(*Location, 0.01)
                && PortraitPose().Equals(*PortraitRotation, 0.01);
            if (!Passed)
                Results.Add(FString::Printf(TEXT("DIAGNOSTIC stick-up region=%s sync=%d visible=%d gamepad=%d state_equal=%d actor_before=%s actor_after=%s portrait_before=%s portrait_after=%s"),
                    *Controller->NativeMenu->GetFocusedRegionName(), Controller->NativeMenu->HasSynchronizedFocus(),
                    Controller->NativeMenu->IsFocusedControlVisible(), Controller->UsesGamepad(),
                    Controller->Sim.Serialize() == *Before, *Location->ToString(),
                    *Controller->GetPawn()->GetActorLocation().ToString(), *PortraitRotation->ToString(), *PortraitPose().ToString()));
            return Passed;
        }, 0.15f);
    Add(TEXT("Left boundary reaches visible portrait without rotation"),
        [this]() { Tap(EKeys::Gamepad_DPad_Left); },
        [Focused, PortraitRotation, PortraitPose]() { return Focused(TEXT("Portrait")) && PortraitPose().Equals(*PortraitRotation, 0.01); });
    Add(TEXT("Right reverses from portrait into the carried content"),
        [this]()
        {
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            const auto Position = Widget->GetCachedGeometry().GetAbsolutePosition();
            const auto Size = Widget->GetCachedGeometry().GetAbsoluteSize();
            Results.Add(FString::Printf(TEXT("INPUT portrait-right before=%s widget=%s position=(%.2f,%.2f) size=(%.2f,%.2f)"),
                *Controller->NativeMenu->GetFocusedRegionName(), *Widget->GetTypeAsString(), Position.X, Position.Y, Size.X, Size.Y));
            Tap(EKeys::Gamepad_DPad_Right);
        },
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
    Add(TEXT("Held focused-widget stick repeats into the next logical row"),
        [SlateAxis]() { SlateAxis(EKeys::Gamepad_LeftY, -0.9f); },
        [this, SlateAxis, ColumnCount, Focused]()
        {
            SlateAxis(EKeys::Gamepad_LeftY, 0);
            return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedContentIndex() == 2 * *ColumnCount;
        }, 0.36f);
    Steps.Last().Repeat = [SlateAxis]() { SlateAxis(EKeys::Gamepad_LeftY, -0.9f); };
    Add(TEXT("Released stick stops repeating while focus remains visible"),
        []() {},
        [this, ColumnCount, Focused]() { return Focused(TEXT("Content"))
            && Controller->NativeMenu->GetSelectedContentIndex() == 2 * *ColumnCount; }, 0.4f);
    Add(TEXT("Reverse stick gesture returns one row without waiting for held repeat"),
        [SlateAxis]() { SlateAxis(EKeys::Gamepad_LeftY, 0.9f); },
        [this, SlateAxis, ColumnCount, Focused]()
        {
            SlateAxis(EKeys::Gamepad_LeftY, 0);
            return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedContentIndex() == *ColumnCount;
        }, 0.15f);
    Add(TEXT("D-pad returns to the first row after stick navigation"),
        [this]() { Tap(EKeys::Gamepad_DPad_Up); },
        [this, Focused]() { return Focused(TEXT("Content")) && Controller->NativeMenu->GetSelectedContentIndex() == 0; });
    for (int32 Row = 1; Row < 6; ++Row)
        Add(TEXT("Directional input traverses a real logical row and scrolls its widget into view"),
            [this]() { Tap(EKeys::Gamepad_DPad_Down); },
            [this, Row, ColumnCount, Focused]() { return Focused(TEXT("Content"))
                && Controller->NativeMenu->GetSelectedContentIndex() == Row * *ColumnCount; });
    Capture(TEXT("native-navigation-scrolled"));
    Add(TEXT("Only the actual final scrolled row exits downward"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]() { return Focused(TEXT("Equipment")) && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Select a real stack for controller virtual drag"),
        [this, Open, Before, SelectedId]()
        {
            Open(0); Tap(EKeys::Right);
            *Before = Controller->Sim.Serialize();
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            *SelectedId = Subject ? Subject->SubjectId : 0;
        },
        [this, Focused]() { const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Focused(TEXT("Content")) && Subject && Subject->Quantity > 2; });
    Add(TEXT("Controller A picks up the selected tile without mutation"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->NativeMenu->IsVirtualDraggingItem()
            && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Directional focus moves while virtual drag remains transient"),
        [this]() { Tap(EKeys::Gamepad_DPad_Right); },
        [this, Before, SelectedId, Focused]() { const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Focused(TEXT("Content")) && Controller->NativeMenu->IsVirtualDraggingItem()
                && Subject && Subject->SubjectId != *SelectedId
                && Controller->Sim.Serialize() == *Before; });
    Capture(TEXT("native-navigation-drag"));
    Add(TEXT("Back cancels virtual drag without closing Inventory"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, Before, Focused]() { return Controller->IsBookOpen()
            && !Controller->NativeMenu->IsVirtualDraggingItem() && Focused(TEXT("Content"))
            && Controller->Sim.Serialize() == *Before; });
    Add(TEXT("Settings still begins on safe Resume control"),
        [this]() { Controller->CloseBook(); Tap(EKeys::Escape); },
        [Focused]() { return Focused(TEXT("Session")); });
    Add(TEXT("Down enters the first row of the vertical Settings list"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this, Focused, Before]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Focused(TEXT("Content")) && Subject && Subject->Id == 0
                && Controller->Sim.Serialize() == *Before;
        });
    Add(TEXT("Directional input reaches the direct Quit game row"),
        [this]()
        {
            for (int32 Index = 0; Index < 32; ++Index)
            {
                const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
                if (Subject && Subject->Id == 9) break;
                Tap(EKeys::Gamepad_DPad_Down);
            }
        },
        [this, Focused]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Focused(TEXT("Content")) && Subject && Subject->Id == 9;
        });
    Add(TEXT("Activating Quit game reaches the one two-choice confirmation"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->NativeMenu->IsExitPrompt() && Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Back cancels Quit and returns focus to the direct Quit game row"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, Focused]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Controller->IsBookOpen() && !Controller->NativeMenu->HasActiveDialog()
                && Focused(TEXT("Content")) && Subject && Subject->Id == 9;
        });
}
