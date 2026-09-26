#include "../HomesteadSmokeTest.h"

#include "../HomesteadCharacter.h"
#include "../HomesteadAnimInstance.h"
#include "../HomesteadController.h"
#include "../HomesteadKnife.h"
#include "../HomesteadSave.h"
#include "SHomesteadHotbar.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
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
    const auto KnifeDropId = MakeShared<int32>(0);
    const auto LowId = MakeShared<int32>(0);
    const auto SaplingId = MakeShared<int32>(0);
    const auto KnifeStarts = MakeShared<uint32>(0);
    const auto HatchetStarts = MakeShared<uint32>(0);
    const auto OldHairStyle = MakeShared<int32>(-1);
    const auto OldBodyPreset = MakeShared<int32>(-1);
    const auto AirborneEnergy = MakeShared<double>(0);
    const auto ReserveEnergy = MakeShared<double>(0);
    const auto WorkEnergy = MakeShared<double>(0);
    const auto WorkStarts = MakeShared<uint32>(0);
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
    const auto PointerHover = [this](int32 Index)
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
    };
    const auto PointerLeave = [this]()
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        const FVector2D Position(20, 20);
        const FVector2D Previous = Slate.GetCursorPos();
        Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Previous, TSet<FKey>(),
            EKeys::Invalid, 0, FModifierKeysState()));
    };

    Add(TEXT("Close Guidebook to expose the gameplay hotbar"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return Controller->ShouldShowHotbar()
            && Controller->HotbarWidget.IsValid(); });
    Add(TEXT("Default ten-slot references add no capacity, resolve only the carried Knife and pin Berries"),
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
                && Slots[4].Tool == Item::Machete && !Slots[4].Available
                && Slots[5].Tool == Item::Berries && Slots[5].Assigned && Slots[5].Food
                && !Slots[6].Assigned && !Slots[9].Assigned
                && Controller->Simulation().UsedCapacity() == *Capacity;
        });
    Add(TEXT("Carried selected Knife is presented in the heroine hand"),
        []() {},
        [this]() { return Controller->GetPawn()
            && Cast<AHomesteadCharacter>(Controller->GetPawn())->GetKnife()->IsPresented(); }, 0.25f);
    Add(TEXT("Held Knife is hand-bound, noncolliding and plausibly sized"),
        []() {},
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Knife = Avatar ? Avatar->GetKnife() : nullptr;
            const float Height = Knife ? Knife->Bounds.BoxExtent.Z * 2 : 0;
            return Knife && Knife->GetAttachParent() == Avatar->GetMesh()
                && Knife->GetAttachSocketName() == TEXT("hand_r")
                && Knife->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                && !Knife->GetGenerateOverlapEvents()
                && Knife->GetNumSections() == 3
                && Height > 12 && Height < 30;
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
    Add(TEXT("Number two selects the assigned but uncarried Hatchet"),
        [this]() { Tap(EKeys::Two); },
        [this]() { return Controller->SelectedHotbarIndex() == 1
            && !Controller->HotbarSnapshot()[1].Available; });
    Add(TEXT("Uncarried selected tool rejects left click without mutation"),
        [this, Revision]() { *Revision = Controller->Simulation().GetRevision(); Tap(EKeys::LeftMouseButton); },
        [this, Revision]() { return Controller->ToastIsError()
            && Controller->Simulation().GetRevision() == *Revision; });
    Add(TEXT("Duplicate invalid save references sanitize to Empty, gain the missing Machete and Berries and clamp selection"),
        [this]()
        {
            Controller->SanitizeHotbar({
                static_cast<int32>(Item::Knife),
                static_cast<int32>(Item::Knife),
                999,
                static_cast<int32>(Item::WateringCan)}, 99, 0);
        },
        [this]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            const bool Passed = Slots[0].Tool == Item::Knife && Slots[0].Assigned
                && Slots[1].Tool == Item::Machete && Slots[1].Assigned
                && Slots[2].Tool == Item::Berries && Slots[2].Food && !Slots[4].Assigned
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
    Add(TEXT("Pointer hover previews carried Knife without selecting or using it"),
        [PointerHover]() { PointerHover(0); },
        [this, Revision]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->GetKnife()->IsPresented()
                && Controller->SelectedHotbarIndex() == 2
                && Controller->Simulation().GetRevision() == *Revision;
        }, 0.2f);
    Add(TEXT("Leaving Knife hover restores selected Digging Stick without a ghost prop"),
        [PointerHover]() { PointerHover(2); },
        [this, Revision]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && !Avatar->GetKnife()->IsPresented()
                && Controller->SelectedHotbarIndex() == 2
                && Controller->Simulation().GetRevision() == *Revision;
        }, 0.2f);
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
    Add(TEXT("Selected carried Knife is visible before chest storage"),
        [this, PointerLeave]() { PointerLeave(); Tap(EKeys::One); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 0
                && Avatar->GetKnife()->IsPresented();
        }, 0.2f);
    Add(TEXT("Storing the only Knife removes its live icon and held prop"),
        [this, ChestId]()
        {
            const auto Result = Controller->Sim.Transfer(*ChestId, Item::Knife, 1,
                Controller->PlayerPoint());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 0
                && Controller->Simulation().Count(Item::Knife) == 0
                && !Controller->HotbarSnapshot()[0].Available
                && !Avatar->GetKnife()->IsPresented();
        });
    Add(TEXT("Retrieving Knife restores its same numbered icon and held prop"),
        [this, ChestId]()
        {
            const auto Result = Controller->Sim.Transfer(*ChestId, Item::Knife, -1,
                Controller->PlayerPoint());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 0
                && Controller->Simulation().Count(Item::Knife) == 1
                && Controller->HotbarSnapshot()[0].Available
                && Avatar->GetKnife()->IsPresented();
        }, 0.2f);
    Add(TEXT("Return to the original selected tool after chest proof"),
        [this]() { Tap(EKeys::Three); },
        [this]() { return Controller->SelectedHotbarIndex() == 2; });
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
    Add(TEXT("R3 still toggles camera distance without changing selected slot"),
        [this, CameraDistance]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            *CameraDistance = Avatar ? Avatar->CameraArm->TargetArmLength : 0;
            Tap(EKeys::Gamepad_RightThumbstick);
        },
        [this, CameraDistance]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 2
                && !FMath::IsNearlyEqual(
                    Avatar->CameraArm->TargetArmLength, *CameraDistance);
        });
    Add(TEXT("Controller LB cycles toward Knife without inventory mutation"),
        [this, Revision]()
        {
            *Revision = Controller->Simulation().GetRevision();
            Tap(EKeys::Gamepad_LeftShoulder);
        },
        [this, Revision]() { return Controller->SelectedHotbarIndex() == 1
            && Controller->Simulation().GetRevision() == *Revision; });
    Add(TEXT("Controller LB equips the carried Knife in hand"),
        [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 0
                && Avatar->GetKnife()->IsPresented();
        }, 0.2f);
    Add(TEXT("Controller RB clears held Knife on tool switch"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 1
                && !Avatar->GetKnife()->IsPresented();
        });
    Add(TEXT("Controller RB returns to the previous selected tool"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->SelectedHotbarIndex() == 2; });
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
        [this, KnifeStarts, HatchetStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Animation) { Finish(false, TEXT("Knife animation instance is missing.")); return; }
            *KnifeStarts = Animation->KnifeCutStarts();
            *HatchetStarts = Animation->ClearStarts();
            Tap(EKeys::LeftMouseButton);
        },
        [this, LowId]() { const auto Node = std::find_if(
                Controller->State().resources.begin(), Controller->State().resources.end(),
                [LowId](const auto& Value) { return Value.id == *LowId; });
            return Node != Controller->State().resources.end() && Node->cleared; });
    Add(TEXT("Successful Knife clear presents one distinct short cut with the held prop"),
        []() {},
        [this, KnifeStarts, HatchetStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Avatar || !Animation || !Avatar->GetKnife()->IsPresented()
                || Animation->KnifeCutStarts() != *KnifeStarts + 1
                || Animation->ClearStarts() != *HatchetStarts
                || Animation->KnifeCutWeight() <= 0.3f)
                return false;
            return true;
        }, 0.2f);
    Add(TEXT("Opening the field book cancels a Knife cut and hides the prop"),
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Animation || Animation->KnifeCutWeight() <= 0.01f)
            {
                Finish(false, TEXT("Knife gesture ended before menu cancellation could be exercised."));
                return;
            }
            Tap(EKeys::I);
        },
        [this, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            const bool Passed = Controller->IsBookOpen() && Avatar && Animation
                && !Avatar->GetKnife()->IsPresented()
                && Animation->KnifeCutStarts() == *KnifeStarts + 1;
            if (!Passed)
                Results.Add(FString::Printf(
                    TEXT("KNIFE_CANCEL_DIAG book=%d avatar=%d anim=%d prop_visible=%d cut_starts=%u expected=%u weight=%.3f"),
                    Controller->IsBookOpen(), Avatar != nullptr, Animation != nullptr,
                    Avatar && Avatar->GetKnife()->IsPresented(),
                    Animation ? Animation->KnifeCutStarts() : 0, *KnifeStarts + 1,
                    Animation ? Animation->KnifeCutWeight() : -1.0f));
            return Passed;
        }, 0.3f);
    Add(TEXT("Closing the book never replays canceled Knife work"),
        [this]() { Tap(EKeys::Escape); },
        [this, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return !Controller->IsBookOpen() && Avatar && Animation
                && Animation->KnifeCutStarts() == *KnifeStarts + 1
                && Animation->KnifeCutWeight() < 0.01f;
        }, 0.8f);
    Add(TEXT("Open Appearance without showing a Knife through the field book"),
        [this, OldHairStyle, OldBodyPreset]()
        {
            *OldHairStyle = Controller->Appearance.HairStyle;
            *OldBodyPreset = Controller->Appearance.BodyPreset;
            Controller->MenuPage(6);
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Controller->IsBookOpen() && Controller->BookPage() == 6
                && Avatar && !Avatar->GetKnife()->IsPresented();
        }, 0.25f);
    Add(TEXT("Appearance rebuild keeps the selected Knife hidden in the book"),
        [this]()
        {
            Controller->MenuSelect(0);
            Controller->MenuActivate();
        },
        [this, OldHairStyle]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const bool Passed = Controller->IsBookOpen() && Controller->BookPage() == 6
                && Controller->Appearance.HairStyle == (*OldHairStyle + 1) % 3
                && Controller->SelectedHotbarIndex() == 0
                && Avatar && Avatar->IsEquipmentPresentationReady()
                && !Avatar->GetKnife()->IsPresented();
            if (!Passed)
                Results.Add(FString::Printf(TEXT("APPEARANCE_REBIND_DIAG book=%d page=%d style=%d old=%d hotbar=%d ready=%d knife=%d"),
                    Controller->IsBookOpen(), Controller->BookPage(), Controller->Appearance.HairStyle,
                    *OldHairStyle, Controller->SelectedHotbarIndex(),
                    Avatar && Avatar->IsEquipmentPresentationReady(),
                    Avatar && Avatar->GetKnife()->IsPresented()));
            return Passed;
        }, 0.3f);
    Add(TEXT("Changing body and fitted garments never ghosts the selected Knife"),
        [this]()
        {
            Controller->MenuSelect(6);
            Controller->MenuActivate();
        },
        [this, OldBodyPreset]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Controller->IsBookOpen() && Controller->BookPage() == 6
                && Controller->Appearance.BodyPreset == (*OldBodyPreset + 1) % 3
                && Avatar && Avatar->IsEquipmentPresentationReady()
                && !Avatar->GetKnife()->IsPresented();
        }, 0.3f);
    Add(TEXT("Leaving Appearance rebinds exactly one held Knife without replay"),
        [this]() { Tap(EKeys::Escape); },
        [this, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return !Controller->IsBookOpen() && Avatar && Animation
                && Controller->SelectedHotbarIndex() == 0
                && Avatar->GetKnife()->IsPresented()
                && Animation->KnifeCutStarts() == *KnifeStarts + 1;
        }, 0.3f);
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
        [this, Revision, SaplingId, KnifeStarts]() { const auto Node = std::find_if(
                Controller->State().resources.begin(), Controller->State().resources.end(),
                [SaplingId](const auto& Value) { return Value.id == *SaplingId; });
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Controller->ToastIsError()
                && Controller->Simulation().GetRevision() == *Revision
                && Node != Controller->State().resources.end() && !Node->cleared
                && Animation && Animation->KnifeCutStarts() == *KnifeStarts + 1; });
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
                    const int32 GardenX = Homestead::CellToGarden(X), GardenY = Homestead::CellToGarden(Y);
                    const auto Center = Homestead::GardenCellCenter(GardenX, GardenY);
                    const Homestead::Point Position{Center.x - Homestead::GardenCellSize, Center.y};
                    if (Candidate.Till(GardenX, GardenY, Position))
                    {
                        *TillX = GardenX; *TillY = GardenY; Found = true;
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
            Teleport(Homestead::GardenCellCenter(*TillX, *TillY));
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
    Add(TEXT("CONTROLLED dropping the only Knife removes its icon and held prop"),
        [this, KnifeDropId]()
        {
            Tap(EKeys::One);
            int32 GroupId = 0;
            for (const auto& Entry : Controller->State().inventoryLayout)
                if (Entry.item == Item::Knife && Entry.wearableId == 0)
                    GroupId = Entry.groupId;
            Homestead::Point DropPosition;
            if (!Controller->ResolveDropPoint(DropPosition))
            {
                Finish(false, TEXT("Controlled Knife drop has no safe placement beside the heroine."));
                return;
            }
            const auto Result = Controller->Sim.DropGroup(GroupId, 1, DropPosition,
                Controller->PlayerPoint(),
                Controller->Sim.GetRevision());
            if (!Result || Controller->State().worldDrops.empty())
            {
                Finish(false, FString(TEXT("Controlled Knife drop failed: "))
                    + UTF8_TO_TCHAR(Result.message.c_str()));
                return;
            }
            *KnifeDropId = Controller->State().worldDrops.back().id;
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->SelectedHotbarIndex() == 0
                && Controller->Simulation().Count(Item::Knife) == 0
                && !Controller->HotbarSnapshot()[0].Available
                && !Avatar->GetKnife()->IsPresented();
        }, 0.2f);
    Add(TEXT("CONTROLLED pickup restores the same Knife assignment and held prop"),
        [this, KnifeDropId]()
        {
            const auto Result = Controller->Sim.PickUpDrop(*KnifeDropId,
                Controller->PlayerPoint());
            if (!Result) Finish(false, TEXT("Controlled Knife pickup failed."));
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Controller->Simulation().Count(Item::Knife) == 1
                && Controller->HotbarSnapshot()[0].Available
                && Avatar->GetKnife()->IsPresented();
        }, 0.2f);
    Add(TEXT("A new woodland resets world-specific hotbar references safely"),
        [this]() { Controller->NewGame(); },
        [this]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            return Controller->IsBookOpen() && Controller->SelectedHotbarIndex() == 0
                && Slots.Num() == 10 && Slots[0].Tool == Item::Knife
                && Slots[0].Available && !Slots[1].Available
                && !Slots[2].Available && !Slots[3].Available;
        }, 0.8f);
    Add(TEXT("Close fresh notes and settle before sprint eligibility fixtures"),
        [this]() { Tap(EKeys::Escape); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return !Controller->IsBookOpen() && Avatar
                && Avatar->GetCharacterMovement()->IsMovingOnGround()
                && !Controller->bPendingSpawn;
        }, 0.8f);
    Add(TEXT("CONTROLLED Simulation exertion reaches exactly the 10-Energy reserve"),
        [this, ReserveEnergy]()
        {
            for (int32 Index = 0; Index < 26; ++Index)
            {
                const auto Result = Controller->Sim.SpendSprintEnergy(10);
                if (!Result)
                {
                    Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
                    return;
                }
            }
            *ReserveEnergy = Controller->State().energy;
            if (!FMath::IsNearlyEqual(*ReserveEnergy, 10.0, 0.001))
                Finish(false, FString::Printf(
                    TEXT("Sprint authority did not clamp to 10 Energy: %.5f"), *ReserveEnergy));
        },
        [this, ReserveEnergy]() { return FMath::IsNearlyEqual(*ReserveEnergy, 10.0, 0.001)
            && Controller->State().energy <= 10.0 && Controller->State().energy > 9.9; });
    Add(TEXT("Held mapped Shift walks instead of sprinting below Energy reserve"),
        [this, ReserveEnergy]()
        {
            *ReserveEnergy = Controller->State().energy;
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this, ReserveEnergy]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->GetVelocity().Size2D() > 60
                && !Avatar->IsSprinting()
                && FMath::IsNearlyEqual(Avatar->GetCharacterMovement()->MaxWalkSpeed, 180.0f)
                && Controller->State().energy <= *ReserveEnergy
                && Controller->State().energy > *ReserveEnergy - 0.1;
        }, 0.9f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1); };
    Add(TEXT("CONTROLLED fresh-world reset restores sprint eligibility safely"),
        [this]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
            Controller->NewGame();
        },
        [this]() { return Controller->IsBookOpen()
            && Controller->State().energy > 90; }, 0.8f);
    Add(TEXT("Close reset notes before new mapped sprint input"),
        [this]() { Tap(EKeys::Escape); },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && !Controller->IsBookOpen() && !Controller->bPendingSpawn
                && Avatar->GetCharacterMovement()->IsMovingOnGround();
        }, 0.6f);
    Add(TEXT("Held mapped Shift again reaches active grounded sprint"),
        [this]()
        {
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->IsSprinting()
                && Avatar->GetVelocity().Size2D() > 200;
        }, 1.3f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1); };
    Add(TEXT("CONTROLLED work presentation stops sprint before the Knife gesture"),
        [this, WorkEnergy, WorkStarts]()
        {
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Avatar || !Animation || !Avatar->IsSprinting())
            {
                Finish(false, TEXT("The controlled work interruption lacked an active sprint."));
                return;
            }
            *WorkEnergy = Controller->State().energy;
            *WorkStarts = Animation->KnifeCutStarts();
            const Homestead::Point Player = Controller->PlayerPoint();
            Avatar->PlayKnifeCut({Player.x + 100, Player.y});
            Axis(EKeys::Gamepad_LeftY, 0);
        },
        [this, WorkEnergy, WorkStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Avatar && Animation && !Avatar->IsSprinting()
                && FMath::IsNearlyEqual(Avatar->GetCharacterMovement()->MaxWalkSpeed, 180.0f)
                && Animation->KnifeCutStarts() == *WorkStarts + 1
                && Animation->KnifeCutWeight() > 0.1f
                && FMath::Abs(Controller->State().energy - *WorkEnergy) < 0.1;
        }, 0.4f);
    Add(TEXT("Cancel the isolated work pose with no queued sprint replay"),
        [this]()
        {
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (Avatar) Avatar->CancelAction(true);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
        },
        [this, WorkStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Avatar && Animation && !Avatar->IsSprinting()
                && Animation->ActionWeight() < 0.01f
                && Animation->KnifeCutStarts() == *WorkStarts + 1;
        }, 0.3f);
    Add(TEXT("Fresh mapped hold re-enters sprint after completed work"),
        [this]()
        {
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->IsSprinting()
                && Avatar->GetVelocity().Size2D() > 200;
        }, 1.3f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1); };
    Add(TEXT("CONTROLLED airborne movement cancels sprint and its Energy cost"),
        [this, AirborneEnergy]()
        {
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (!Avatar || !Avatar->IsSprinting())
            {
                Finish(false, TEXT("Airborne sprint fixture lacked a grounded sprint."));
                return;
            }
            *AirborneEnergy = Controller->State().energy;
            Avatar->SetActorLocation(Avatar->GetActorLocation() + FVector(0, 0, 140),
                false, nullptr, ETeleportType::TeleportPhysics);
            Avatar->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        },
        [this, AirborneEnergy]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && !Avatar->GetCharacterMovement()->IsMovingOnGround()
                && !Avatar->IsSprinting()
                && FMath::IsNearlyEqual(Avatar->GetCharacterMovement()->MaxWalkSpeed, 180.0f)
                && FMath::Abs(Controller->State().energy - *AirborneEnergy) < 0.1;
        }, 0.45f);
    Add(TEXT("Release airborne sprint input without queued restart"),
        [this]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && !Avatar->IsSprinting();
        });
}
