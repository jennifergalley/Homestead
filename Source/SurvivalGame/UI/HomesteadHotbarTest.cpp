#include "../HomesteadSmokeTest.h"

#include "../HomesteadCharacter.h"
#include "../HomesteadController.h"
#include "../HomesteadSave.h"
#include "SHomesteadHotbar.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/Events.h"

void AHomesteadSmokeTest::PrepareHotbarChecks()
{
    using Homestead::Item;
    const auto Capacity = MakeShared<int32>(0);
    const auto Revision = MakeShared<uint64>(0);
    const auto CameraDistance = MakeShared<float>(0);
    const auto TillX = MakeShared<int32>(0);
    const auto TillY = MakeShared<int32>(0);
    const auto ChestId = MakeShared<int32>(0);
    const auto LowId = MakeShared<int32>(0);
    const auto SaplingId = MakeShared<int32>(0);
    Homestead::ResourceNode LowResource;
    Homestead::ResourceNode Sapling;
    for (const auto& Node : Controller->State().resources)
    {
        if (!LowResource.id && Node.kind == Homestead::ResourceKind::Branches
            && !Node.cleared) LowResource = Node;
        if (!Sapling.id && Node.kind == Homestead::ResourceKind::Sapling
            && !Node.cleared) Sapling = Node;
    }
    if (!LowResource.id || !Sapling.id)
    {
        Finish(false, TEXT("Default hotbar resource fixtures are unavailable."));
        return;
    }
    const auto PointerClick = [this](int32 Index)
    {
        if (!Controller->HotbarWidget.IsValid()) return;
        const auto Widget = Controller->HotbarWidget->SlotWidget(Index);
        if (!Widget) return;
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        const FGeometry Geometry = Widget->GetCachedGeometry();
        const FVector2D Position = Geometry.GetAbsolutePosition()
            + Geometry.GetAbsoluteSize() * 0.5f;
        const FVector2D Previous = Slate.GetCursorPos();
        Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Previous, TSet<FKey>(),
            EKeys::Invalid, 0, FModifierKeysState()));
        TSet<FKey> Pressed;
        Pressed.Add(EKeys::LeftMouseButton);
        Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position,
            Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position,
            TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
    };

    Add(TEXT("Close Guidebook to expose the gameplay hotbar"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return Controller->ShouldShowHotbar()
            && Controller->HotbarWidget.IsValid(); });
    Add(TEXT("Default ten-slot references add no capacity and resolve only the carried Knife"),
        [this, Capacity]()
        {
            *Capacity = Controller->Simulation().UsedCapacity();
        },
        [this, Capacity]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            return Slots.Num() == 10 && Slots[0].Tool == Item::Knife
                && Slots[0].Available && Slots[0].Selected
                && Slots[1].Tool == Item::Hatchet && !Slots[1].Available
                && Slots[2].Tool == Item::DiggingStick && !Slots[2].Available
                && Slots[3].Tool == Item::WateringCan && !Slots[3].Available
                && !Slots[4].Assigned && !Slots[9].Assigned
                && Controller->Simulation().UsedCapacity() == *Capacity;
        });
    Add(TEXT("Capture the original ten-slot gameplay hotbar"),
        [this]() { Screenshot(TEXT("hotbar-gameplay")); },
        [this]()
        {
            const auto First = Controller->HotbarWidget->SlotWidget(0);
            const auto Last = Controller->HotbarWidget->SlotWidget(9);
            if (!First || !Last) return false;
            const FGeometry FirstGeometry = First->GetCachedGeometry();
            const FGeometry LastGeometry = Last->GetCachedGeometry();
            const FVector2D FirstPosition = FirstGeometry.GetAbsolutePosition();
            const FVector2D LastPosition = LastGeometry.GetAbsolutePosition();
            const FVector2D SlotSize = FirstGeometry.GetAbsoluteSize();
            const double WidthPixels = LastPosition.X + LastGeometry.GetAbsoluteSize().X
                - FirstPosition.X;
            int32 Width = 0, Height = 0;
            Controller->GetViewportSize(Width, Height);
            Results.Add(FString::Printf(
                TEXT("HOTBAR_BOUNDS x=%.2f y=%.2f width=%.2f slot=%.2f,%.2f viewport=%d,%d"),
                FirstPosition.X, FirstPosition.Y, WidthPixels, SlotSize.X, SlotSize.Y,
                Width, Height));
            const double Center = FirstPosition.X + WidthPixels * 0.5;
            return WidthPixels >= 300 && WidthPixels <= Width * 0.5
                && FMath::Abs(Center - Width * 0.5) <= Width * 0.03
                && FirstPosition.Y >= Height * 0.85
                && SlotSize.X >= 30 && SlotSize.Y >= 30
                && Width >= 1280 && Height >= 720;
        }, 0.8f);
    Add(TEXT("Number zero selects slot ten without world mutation"),
        [this]() { Tap(EKeys::Zero); },
        [this]() { return Controller->SelectedHotbarIndex() == 9; });
    Add(TEXT("Wheel down wraps slot ten to slot one exactly once"),
        [this]() { Axis(EKeys::MouseWheelAxis, -1); Axis(EKeys::MouseWheelAxis, 0); },
        [this]() { return Controller->SelectedHotbarIndex() == 0; });
    Add(TEXT("Number two selects the ghosted Hatchet"),
        [this]() { Tap(EKeys::Two); },
        [this]() { return Controller->SelectedHotbarIndex() == 1
            && !Controller->HotbarSnapshot()[1].Available; });
    Add(TEXT("Ghosted selected tool rejects left click without mutation"),
        [this, Revision]() { *Revision = Controller->Simulation().GetRevision(); Tap(EKeys::LeftMouseButton); },
        [this, Revision]() { return Controller->ToastIsError()
            && Controller->Simulation().GetRevision() == *Revision; });
    Add(TEXT("Duplicate invalid save references sanitize to Empty and clamp selection"),
        [this]()
        {
            Controller->SanitizeHotbar({
                static_cast<int32>(Item::Knife),
                static_cast<int32>(Item::Knife),
                999,
                static_cast<int32>(Item::WateringCan)}, 99);
        },
        [this]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            const bool Passed = Slots[0].Tool == Item::Knife && Slots[0].Assigned
                && !Slots[1].Assigned && !Slots[2].Assigned
                && Slots[3].Tool == Item::WateringCan && Slots[3].Assigned
                && Controller->SelectedHotbarIndex() == 9;
            Controller->ResetHotbar();
            return Passed;
        });
    Add(TEXT("CONTROLLED supply makes referenced crafted tools live without extra hotbar storage"),
        [this]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Supplied.GetState());
            for (const Item Tool : {Item::Hatchet, Item::DiggingStick, Item::WateringCan})
            {
                State.inventory[static_cast<int32>(Tool)] = 1;
                State.inventoryLayout.push_back({State.nextGroupId++, Tool, 1, 0});
            }
            const auto Result = Controller->Sim.Deserialize(Supplied.Serialize());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this, Capacity]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            return Slots[0].Available && Slots[1].Available && Slots[2].Available
                && Slots[3].Available
                && Controller->Simulation().UsedCapacity() == *Capacity + 3;
        });
    Add(TEXT("Pointer click selects Digging Stick and consumes the click"),
        [this, Revision, PointerClick]()
        {
            *Revision = Controller->Simulation().GetRevision();
            PointerClick(2);
        },
        [this, Revision]() { return Controller->SelectedHotbarIndex() == 2
            && Controller->Simulation().GetRevision() == *Revision; });
    Add(TEXT("CONTROLLED chest fixture uses ordinary transfer authority for live references"),
        [this, ChestId]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Supplied.GetState());
            for (const auto Pair : {TPair<Item, int32>(Item::Branch, 5),
                TPair<Item, int32>(Item::Fiber, 2)})
            {
                State.inventory[static_cast<int32>(Pair.Key)] += Pair.Value;
                State.inventoryLayout.push_back(
                    {State.nextGroupId++, Pair.Key, Pair.Value, 0});
            }
            bool Placed = false;
            for (int32 Y = -4; Y <= 3 && !Placed; ++Y)
                for (int32 X = -7; X <= 1 && !Placed; ++X)
                {
                    Homestead::Simulation Candidate = Supplied;
                    const auto Center = Homestead::CellCenter(X, Y);
                    const Homestead::Point Position{Center.x, Center.y};
                    if (!Candidate.Place(Homestead::Piece::Chest, X, Y, 0, Position))
                        continue;
                    const auto Result = Controller->Sim.Deserialize(Candidate.Serialize());
                    if (!Result)
                    {
                        Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
                        return;
                    }
                    *ChestId = Controller->State().structures.back().id;
                    Teleport(Position);
                    Placed = true;
                }
            if (!Placed) Finish(false, TEXT("No valid controlled chest location exists."));
        },
        [this, ChestId]() { return Controller->Simulation().FindNearestStructure(
                Controller->PlayerPoint(), Homestead::Piece::Chest, 280) == *ChestId; });
    Add(TEXT("Storing the referenced Hatchet ghosts its slot immediately"),
        [this, ChestId]()
        {
            const auto Result = Controller->Sim.Transfer(*ChestId, Item::Hatchet, 1,
                Controller->PlayerPoint());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this]() { return Controller->Simulation().Count(Item::Hatchet) == 0
            && !Controller->HotbarSnapshot()[1].Available; });
    Add(TEXT("Returning the Hatchet restores the same hotbar reference"),
        [this, ChestId]()
        {
            const auto Result = Controller->Sim.Transfer(*ChestId, Item::Hatchet, -1,
                Controller->PlayerPoint());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this]() { return Controller->Simulation().Count(Item::Hatchet) == 1
            && Controller->HotbarSnapshot()[1].Available; });
    Add(TEXT("Ctrl wheel changes camera distance without changing selected slot"),
        [this, CameraDistance]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            *CameraDistance = Avatar ? Avatar->CameraArm->TargetArmLength : 0;
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::LeftControl, IE_Pressed, 1));
            Axis(EKeys::MouseWheelAxis, 1);
            Axis(EKeys::MouseWheelAxis, 0);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::LeftControl, IE_Released, 0));
        },
        [this, CameraDistance]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 2
                && Avatar->CameraArm->TargetArmLength < *CameraDistance;
        });
    Add(TEXT("Gameplay RB cycles the selected tool"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->SelectedHotbarIndex() == 3; });
    Add(TEXT("Menu RB changes field-book page without changing selected tool"),
        [this]() { Tap(EKeys::I); Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 1
            && Controller->SelectedHotbarIndex() == 3; });
    Add(TEXT("Close menu and save selected hotbar state"),
        [this]() { Tap(EKeys::Escape); Tap(EKeys::F5); },
        [this]() { const auto* Save = Controller->ReadSave(
                Controller->SavePath(TEXT("Homestead_Manual")));
            return Save && Save->HotbarSlots.Num() == 10
                && Save->SelectedHotbarSlot == 3; });
    Add(TEXT("Current-version reload restores references and selected slot"),
        [this]() { Tap(EKeys::One); Tap(EKeys::F9); },
        [this]() { return Controller->SelectedHotbarIndex() == 3
            && Controller->HotbarSnapshot()[3].Tool == Item::WateringCan; });
    Add(TEXT("Settle the loaded pawn before ordinary tool approaches"),
        []() {}, [this]() { return !Controller->bPendingSpawn; }, 0.8f);
    Add(TEXT("Approach low growth for selected Knife use"),
        [this, LowResource]() { Teleport(LowResource.position); Tap(EKeys::One); },
        [this, LowId]()
        {
            if (Controller->Focus == AHomesteadController::EFocus::Resource)
                *LowId = Controller->FocusId;
            return *LowId > 0 && Controller->SelectedHotbarIndex() == 0;
        }, 0.7f);
    Add(TEXT("Left click with carried Knife clears exactly one authoritative low patch"),
        [this]() { Tap(EKeys::LeftMouseButton); },
        [this, LowId]() { const auto Node = std::find_if(
                Controller->State().resources.begin(), Controller->State().resources.end(),
                [LowId](const auto& Value) { return Value.id == *LowId; });
            return Node != Controller->State().resources.end() && Node->cleared; });
    Add(TEXT("Approach a sapling while Knife remains selected"),
        [this, Sapling]() { Teleport(Sapling.position); },
        [this, SaplingId]()
        {
            if (Controller->Focus == AHomesteadController::EFocus::Resource)
                *SaplingId = Controller->FocusId;
            const auto Node = std::find_if(Controller->State().resources.begin(),
                Controller->State().resources.end(), [SaplingId](const auto& Value)
                { return Value.id == *SaplingId; });
            return Node != Controller->State().resources.end()
                && Node->kind == Homestead::ResourceKind::Sapling
                && Controller->SelectedHotbarIndex() == 0;
        }, 0.7f);
    Add(TEXT("Wrong selected Knife cannot clear the sapling"),
        [this, Revision]() { *Revision = Controller->Simulation().GetRevision();
            Tap(EKeys::LeftMouseButton); },
        [this, Revision, SaplingId]() { const auto Node = std::find_if(
                Controller->State().resources.begin(), Controller->State().resources.end(),
                [SaplingId](const auto& Value) { return Value.id == *SaplingId; });
            return Controller->ToastIsError()
                && Controller->Simulation().GetRevision() == *Revision
                && Node != Controller->State().resources.end() && !Node->cleared; });
    Add(TEXT("Number two then left click clears the sapling with the carried Hatchet"),
        [this]() { Tap(EKeys::Two); Tap(EKeys::LeftMouseButton); },
        [this, SaplingId]() { const auto Node = std::find_if(
                Controller->State().resources.begin(), Controller->State().resources.end(),
                [SaplingId](const auto& Value) { return Value.id == *SaplingId; });
            return Controller->SelectedHotbarIndex() == 1
                && Node != Controller->State().resources.end() && Node->cleared; });
    Add(TEXT("Find a valid nearby till cell using an authority copy"),
        [this, TillX, TillY]()
        {
            bool Found = false;
            for (int32 Y = -5; Y <= 3 && !Found; ++Y)
                for (int32 X = -7; X <= 1 && !Found; ++X)
                {
                    Homestead::Simulation Candidate = Controller->Simulation();
                    const auto Center = Homestead::CellCenter(X, Y);
                    const Homestead::Point Position{Center.x - 190, Center.y};
                    if (Candidate.Till(X, Y, Position))
                    {
                        *TillX = X; *TillY = Y; Found = true;
                        Teleport(Position);
                        Controller->GetPawn()->SetActorRotation(FRotator::ZeroRotator);
                    }
                }
            if (!Found) Finish(false, TEXT("No valid nearby hotbar till cell exists."));
        },
        [this]() { return !Controller->IsBookOpen(); }, 0.7f);
    Add(TEXT("Wheel-selected Digging Stick tills the forward cell through left click"),
        [this]() { Tap(EKeys::Three); Tap(EKeys::LeftMouseButton); },
        [this, TillX, TillY]()
        {
            return Controller->SelectedHotbarIndex() == 2
                && std::any_of(Controller->State().plots.begin(),
                    Controller->State().plots.end(), [TillX, TillY](const auto& Plot)
                    { return Plot.cellX == *TillX && Plot.cellY == *TillY; });
        });
    Add(TEXT("CONTROLLED seeds and water prepare one existing plot for selected-tool proof"),
        [this, TillX, TillY]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Supplied.GetState());
            for (const auto Pair : {TPair<Item, int32>(Item::Seeds, 1),
                TPair<Item, int32>(Item::Water, 6)})
            {
                State.inventory[static_cast<int32>(Pair.Key)] += Pair.Value;
                State.inventoryLayout.push_back(
                    {State.nextGroupId++, Pair.Key, Pair.Value, 0});
            }
            const auto Result = Controller->Sim.Deserialize(Supplied.Serialize());
            if (!Result) { Finish(false, UTF8_TO_TCHAR(Result.message.c_str())); return; }
            Teleport(Homestead::CellCenter(*TillX, *TillY));
        },
        [this]() { return Controller->Simulation().Count(Item::Seeds) >= 1
            && Controller->Simulation().Count(Item::Water) >= 6; }, 0.7f);
    Add(TEXT("E remains independent and plants the focused plot"),
        [this]() { Tap(EKeys::E); },
        [this, TillX, TillY]() { return std::any_of(
                Controller->State().plots.begin(), Controller->State().plots.end(),
                [TillX, TillY](const auto& Plot)
                { return Plot.cellX == *TillX && Plot.cellY == *TillY && Plot.planted; }); });
    Add(TEXT("Controller right trigger with selected Watering Can waters the crop once"),
        [this]() { Tap(EKeys::Four); Tap(EKeys::Gamepad_RightTrigger); },
        [this, TillX, TillY]() { const auto Plot = std::find_if(
                Controller->State().plots.begin(), Controller->State().plots.end(),
                [TillX, TillY](const auto& Value)
                { return Value.cellX == *TillX && Value.cellY == *TillY; });
            return Controller->SelectedHotbarIndex() == 3
                && Plot != Controller->State().plots.end() && Plot->moisture > 0; });
}
