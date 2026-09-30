#include "../HomesteadSmokeTest.h"

#include "../HomesteadCharacter.h"
#include "../HomesteadAnimInstance.h"
#include "../HomesteadController.h"
#include "../HomesteadSave.h"
#include "SHomesteadHotbar.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/Events.h"

// The estate kit in hotbar order: 0 Billhook, 1 Axe, 2 Scythe, 3 Pickaxe, 4 Hoe, 5 Pail, 6 Berries.
// The seeded woodland has no salvage, so a stand-in grant hands her the billhook the knife used to be.
void AHomesteadSmokeTest::PrepareHotbarChecks()
{
    using Homestead::Item;
    const auto Capacity = MakeShared<int32>(0);
    const auto Revision = MakeShared<uint64>(0);
    const auto CameraDistance = MakeShared<float>(0);
    const auto TillX = MakeShared<int32>(0);
    const auto TillY = MakeShared<int32>(0);
    const auto ChestId = MakeShared<int32>(0);
    const auto BillhookDropId = MakeShared<int32>(0);
    const auto SaplingId = MakeShared<int32>(0);
    const auto HackStarts = MakeShared<uint32>(0);
    const auto ClearStarts = MakeShared<uint32>(0);
    const auto BranchesBefore = MakeShared<int32>(0);
    const auto KindlingBefore = MakeShared<int32>(0);
    const auto OldHairStyle = MakeShared<int32>(-1);
    const auto OldBodyPreset = MakeShared<int32>(-1);
    const auto AirborneEnergy = MakeShared<double>(0);
    const auto ReserveEnergy = MakeShared<double>(0);
    const auto WorkEnergy = MakeShared<double>(0);
    const auto WorkStarts = MakeShared<uint32>(0);
    Homestead::ResourceNode Sapling;
    for (const auto& Node : Controller->State().resources)
        if (!Sapling.id && Node.kind == Homestead::ResourceKind::Sapling && !Node.cleared) Sapling = Node;
    if (!Sapling.id)
    {
        Finish(false, TEXT("Default hotbar resource fixtures are unavailable."));
        return;
    }
    const auto BillhookShown = [this]()
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        const auto* Prop = Avatar ? Avatar->GetHeldProp(Item::Billhook) : nullptr;
        return Prop && Prop->IsVisible();
    };
    const auto Anim = [this]() -> const UHomesteadAnimInstance*
    {
        const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        return Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    };
    const auto SaplingCleared = [this, SaplingId]()
    {
        const auto Node = std::find_if(Controller->State().resources.begin(), Controller->State().resources.end(),
            [SaplingId](const auto& Value) { return Value.id == *SaplingId; });
        return Node != Controller->State().resources.end() && Node->cleared;
    };
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
    QueueGrant(Item::Billhook, 1);
    Add(TEXT("Default ten-slot references add no capacity, resolve only the carried Billhook, pin Berries and place the lamp in slot 8"),
        [this, Capacity]()
        {
            *Capacity = Controller->Simulation().UsedCapacity();
        },
        [this, Capacity]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            return Slots.Num() == 10 && Slots[0].Tool == Item::Billhook
                && Slots[0].Available && Slots[0].Selected
                && Slots[1].Tool == Item::Hatchet && !Slots[1].Available
                && Slots[2].Tool == Item::Scythe && !Slots[2].Available
                && Slots[3].Tool == Item::Pickaxe && !Slots[3].Available
                && Slots[4].Tool == Item::DiggingStick && !Slots[4].Available
                && Slots[5].Tool == Item::WateringCan && !Slots[5].Available
                && Slots[6].Tool == Item::Berries && Slots[6].Assigned && Slots[6].Food
                && Slots[7].Tool == Item::OilLamp && !Slots[9].Assigned
                && Controller->Simulation().UsedCapacity() == *Capacity;
        });
    Add(TEXT("Carried selected Billhook is presented in the heroine hand"),
        []() {},
        [this, BillhookShown, Anim]()
        {
            if (BillhookShown()) return true;
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Prop = Avatar ? Avatar->GetHeldProp(Item::Billhook) : nullptr;
            const auto* Animation = Anim();
            Results.Add(FString::Printf(TEXT("BILLHOOK_PRESENT_DIAG prop=%d visible=%d presented=%d selected=%d count=%d action=%.3f eat=%.3f"),
                Prop != nullptr, Prop && Prop->IsVisible(), static_cast<int32>(Controller->PresentedTool()),
                static_cast<int32>(Controller->SelectedCarriedTool()), Controller->Simulation().Count(Item::Billhook),
                Animation ? Animation->ActionWeight() : -1.0f, Animation ? Animation->EatWeight() : -1.0f));
            return false;
        }, 0.25f);
    Add(TEXT("Held Billhook is hand-bound, noncolliding and plausibly sized"),
        []() {},
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Billhook = Avatar ? Avatar->GetHeldProp(Item::Billhook) : nullptr;
            const float Length = Billhook ? Billhook->Bounds.BoxExtent.GetMax() * 2 : 0;
            const bool Passed = Billhook && Billhook->GetAttachParent() == Avatar->GetMesh()
                && Billhook->GetAttachSocketName() == TEXT("hand_r")
                && Billhook->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                && Length > 30 && Length < 90;
            if (!Passed && Billhook)
                Results.Add(FString::Printf(TEXT("BILLHOOK_HELD_DIAG parent=%d socket=%s collision=%d length=%.1f"),
                    Billhook->GetAttachParent() == Avatar->GetMesh(), *Billhook->GetAttachSocketName().ToString(),
                    static_cast<int32>(Billhook->GetCollisionEnabled()), Length));
            return Passed;
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
    Add(TEXT("Duplicate invalid save references sanitize to Empty, gain the missing Scythe, Pickaxe and Berries and clamp selection"),
        [this]()
        {
            Controller->SanitizeHotbar({
                static_cast<int32>(Item::Billhook),
                static_cast<int32>(Item::Billhook),
                999,
                static_cast<int32>(Item::WateringCan)}, 99, 0);
        },
        [this]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            const bool Passed = Slots[0].Tool == Item::Billhook && Slots[0].Assigned
                && Slots[1].Tool == Item::Scythe && Slots[1].Assigned
                && Slots[2].Tool == Item::Pickaxe && Slots[2].Assigned
                && Slots[3].Tool == Item::WateringCan && Slots[3].Assigned
                && Slots[4].Tool == Item::Berries && Slots[4].Food && !Slots[5].Assigned
                && Controller->SelectedHotbarIndex() == 9;
            Controller->ResetHotbar();
            return Passed;
        });
    Add(TEXT("CONTROLLED supply makes referenced crafted tools live without extra hotbar storage"),
        [this]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Supplied.GetState());
            for (const Item Tool : {Item::Hatchet, Item::Scythe, Item::Pickaxe, Item::DiggingStick, Item::WateringCan})
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
                && Slots[3].Available && Slots[4].Available && Slots[5].Available
                && Controller->Simulation().UsedCapacity() == *Capacity + 5;
        });
    Add(TEXT("Pointer click selects the Scythe and consumes the click"),
        [this, Revision, PointerClick]()
        {
            *Revision = Controller->Simulation().GetRevision();
            PointerClick(2);
        },
        [this, Revision]() { return Controller->SelectedHotbarIndex() == 2
            && Controller->Simulation().GetRevision() == *Revision; });
    Add(TEXT("Pointer hover previews carried Billhook without selecting or using it"),
        [PointerHover]() { PointerHover(0); },
        [this, Revision, BillhookShown]()
        {
            return BillhookShown()
                && Controller->SelectedHotbarIndex() == 2
                && Controller->Simulation().GetRevision() == *Revision;
        }, 0.2f);
    Add(TEXT("Leaving Billhook hover restores the selected Scythe without a ghost prop"),
        [PointerHover]() { PointerHover(2); },
        [this, Revision, BillhookShown]()
        {
            return !BillhookShown()
                && Controller->SelectedHotbarIndex() == 2
                && Controller->Simulation().GetRevision() == *Revision;
        }, 0.2f);
    Add(TEXT("CONTROLLED chest fixture uses ordinary transfer authority for live references"),
        [this, ChestId]()
        {
            Homestead::Simulation Supplied = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Supplied.GetState());
            for (const auto Pair : {TPair<Item, int32>(Item::Branch, 5),
                TPair<Item, int32>(Item::BrambleCanes, 2)})
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
    Add(TEXT("Selected carried Billhook is visible before chest storage"),
        [this, PointerLeave]() { PointerLeave(); Tap(EKeys::One); },
        [this, BillhookShown]()
        {
            return Controller->SelectedHotbarIndex() == 0 && BillhookShown();
        }, 0.2f);
    Add(TEXT("Storing the only Billhook removes its live icon and held prop"),
        [this, ChestId]()
        {
            const auto Result = Controller->Sim.Transfer(*ChestId, Item::Billhook, 1,
                Controller->PlayerPoint());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this, BillhookShown]()
        {
            return Controller->SelectedHotbarIndex() == 0
                && Controller->Simulation().Count(Item::Billhook) == 0
                && !Controller->HotbarSnapshot()[0].Available
                && !BillhookShown();
        });
    Add(TEXT("Retrieving the Billhook restores its same numbered icon and held prop"),
        [this, ChestId]()
        {
            const auto Result = Controller->Sim.Transfer(*ChestId, Item::Billhook, -1,
                Controller->PlayerPoint());
            if (!Result) Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
        },
        [this, BillhookShown]()
        {
            return Controller->SelectedHotbarIndex() == 0
                && Controller->Simulation().Count(Item::Billhook) == 1
                && Controller->HotbarSnapshot()[0].Available
                && BillhookShown();
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
    Add(TEXT("Controller LB cycles toward the Billhook without inventory mutation"),
        [this, Revision]()
        {
            *Revision = Controller->Simulation().GetRevision();
            Tap(EKeys::Gamepad_LeftShoulder);
        },
        [this, Revision]() { return Controller->SelectedHotbarIndex() == 1
            && Controller->Simulation().GetRevision() == *Revision; });
    Add(TEXT("Controller LB equips the carried Billhook in hand"),
        [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this, BillhookShown]()
        {
            return Controller->SelectedHotbarIndex() == 0 && BillhookShown();
        }, 0.2f);
    Add(TEXT("Controller RB clears the held Billhook on tool switch"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this, BillhookShown]()
        {
            return Controller->SelectedHotbarIndex() == 1 && !BillhookShown();
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
            && Controller->HotbarSnapshot()[3].Tool == Item::Pickaxe; });
    // An old save written before the hotbar existed has an empty HotbarSlots array (no data pointer):
    // applying it must not crash, and it gains the layout-0 migration without touching her stock.
    const auto OldSaveWorld = MakeShared<std::string>();
    Add(TEXT("An old save with an empty hotbar array loads and gains the default migration"),
        [this, OldSaveWorld]()
        {
            UHomesteadSave* Save = Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual")));
            if (!Save) { Finish(false, TEXT("The manual save to age is unavailable.")); return; }
            Save->HotbarSlots.Empty();
            Save->HotbarLayout = 0;
            Save->SelectedHotbarSlot = 0;
            *OldSaveWorld = TCHAR_TO_UTF8(*Save->SimulationData);
            if (!Controller->ApplySave(*Save)) Finish(false, TEXT("The aged save with an empty hotbar did not apply."));
        },
        [this, OldSaveWorld]()
        {
            // Her stock (not the whole world: the clock keeps running) is what the save held (woodland map, no placements).
            Homestead::Simulation Saved;
            const bool StockSame = Saved.Deserialize(*OldSaveWorld).ok && Saved.GetState().inventory == Controller->State().inventory;
            const auto Slots = Controller->HotbarSnapshot();
            FString Line;
            for (const auto& Slot : Slots) Line += FString::Printf(TEXT("%d,"), Slot.Assigned ? static_cast<int32>(Slot.Tool) : -1);
            Results.Add(FString::Printf(TEXT("OLD_SAVE_HOTBAR slots=%s selected=%d stock_same=%d"), *Line, Controller->SelectedHotbarIndex(), StockSame));
            return StockSame && Slots.Num() == 10 && Slots[7].Tool == Item::OilLamp && Slots[7].Assigned
                && Slots[0].Tool == Item::Billhook && Slots[1].Tool == Item::Scythe
                && Slots[2].Tool == Item::Pickaxe && Slots[3].Tool == Item::Berries
                && !Slots[4].Assigned && !Slots[9].Assigned
                && Controller->SelectedHotbarIndex() == 0;
        });
    Add(TEXT("Reloading the current save restores her own hotbar"),
        [this]() { Tap(EKeys::F9); },
        [this]() { return !Controller->bPendingSpawn && Controller->SelectedHotbarIndex() == 3
            && Controller->HotbarSnapshot()[3].Tool == Item::Pickaxe; }, 0.8f);
    Add(TEXT("Settle the loaded pawn before ordinary tool approaches"),
        []() {}, [this]() { return !Controller->bPendingSpawn; }, 0.8f);
    Add(TEXT("Approach a sapling with the Billhook selected"),
        [this, Sapling]() { Teleport(Sapling.position); Tap(EKeys::One); },
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
    Add(TEXT("Number two selects the carried Axe, which cannot clear the sapling"),
        [this, Revision, Anim, HackStarts]()
        {
            *Revision = Controller->Simulation().GetRevision();
            const auto* Animation = Anim();
            *HackStarts = Animation ? Animation->MacheteStarts() : 0;
            Tap(EKeys::Two);
            Tap(EKeys::LeftMouseButton);
        },
        [this, Revision, Anim, HackStarts, SaplingCleared]()
        {
            const auto* Animation = Anim();
            return Controller->SelectedHotbarIndex() == 1 && Controller->ToastIsError()
                && Controller->Simulation().GetRevision() == *Revision && !SaplingCleared()
                && Animation && Animation->MacheteStarts() == *HackStarts;
        });
    Add(TEXT("Number one then one left click fells the sapling with a single two-blow Billhook hack"),
        [this, Anim, HackStarts, ClearStarts, BranchesBefore, KindlingBefore]()
        {
            const auto* Animation = Anim();
            if (!Animation) { Finish(false, TEXT("Billhook animation instance is missing.")); return; }
            *HackStarts = Animation->MacheteStarts();
            *ClearStarts = Animation->ClearStarts();
            *BranchesBefore = Controller->Simulation().Count(Item::Branch);
            *KindlingBefore = Controller->Simulation().Count(Item::Kindling);
            Tap(EKeys::One);
            Tap(EKeys::LeftMouseButton);
        },
        [this, Anim, HackStarts, ClearStarts, BillhookShown, SaplingCleared, BranchesBefore, KindlingBefore]()
        {
            // Worn billhook, one press: the clip's second blow commits the clear, with one yield.
            const auto* Animation = Anim();
            const int32 Branches = Controller->Simulation().Count(Item::Branch) - *BranchesBefore;
            return Controller->SelectedHotbarIndex() == 0 && Animation && BillhookShown()
                && Animation->MacheteStarts() == *HackStarts + 1
                && Animation->ClearStarts() == *ClearStarts
                && Animation->MacheteWeight() > 0.3f
                && SaplingCleared() && Branches >= 3 && Branches <= 4
                && Controller->Simulation().Count(Item::Kindling) == *KindlingBefore + 1
                && !Controller->ToastIsError();
        }, 2.5f);
    // The blow lands 1.25 s into the hack; move on while the follow-through is still playing.
    Steps.Last().bCompleteWhenReady = true;
    Add(TEXT("Opening the field book cancels the Billhook hack; she keeps it in hand"),
        [this, Anim]()
        {
            const auto* Animation = Anim();
            if (!Animation || Animation->MacheteWeight() <= 0.01f)
            {
                Finish(false, TEXT("Billhook hack ended before menu cancellation could be exercised."));
                return;
            }
            Tap(EKeys::I);
        },
        [this, Anim, HackStarts, BillhookShown]()
        {
            const auto* Animation = Anim();
            const bool Passed = Controller->IsBookOpen() && Animation && BillhookShown()
                && Animation->MacheteStarts() == *HackStarts + 1
                && Animation->MacheteWeight() < 0.01f;
            if (!Passed)
                Results.Add(FString::Printf(
                    TEXT("BILLHOOK_CANCEL_DIAG book=%d anim=%d prop_visible=%d hack_starts=%u expected=%u weight=%.3f"),
                    Controller->IsBookOpen(), Animation != nullptr, BillhookShown(),
                    Animation ? Animation->MacheteStarts() : 0, *HackStarts + 1,
                    Animation ? Animation->MacheteWeight() : -1.0f));
            return Passed;
        }, 0.3f);
    Add(TEXT("Closing the book never replays the canceled Billhook hack"),
        [this]() { Tap(EKeys::Escape); },
        [this, Anim, HackStarts]()
        {
            const auto* Animation = Anim();
            return !Controller->IsBookOpen() && Animation
                && Animation->MacheteStarts() == *HackStarts + 1
                && Animation->MacheteWeight() < 0.01f;
        }, 0.8f);
    Add(TEXT("Open Appearance without showing the Billhook through the field book"),
        [this, OldHairStyle, OldBodyPreset]()
        {
            *OldHairStyle = Controller->Appearance.MetaHair;
            *OldBodyPreset = Controller->Appearance.HairColor;
            Controller->MenuPage(6);
        },
        [this, BillhookShown]()
        {
            return Controller->IsBookOpen() && Controller->BookPage() == 6 && !BillhookShown();
        }, 0.25f);
    Add(TEXT("Appearance rebuild keeps the selected Billhook hidden in the book"),
        [this]()
        {
            Controller->MenuSelect(0);
            Controller->MenuActivate();
        },
        [this, OldHairStyle, BillhookShown]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const bool Passed = Controller->IsBookOpen() && Controller->BookPage() == 6
                && Controller->Appearance.MetaHair == (*OldHairStyle + 1) % HomesteadLook::MetaHairCount
                && Controller->SelectedHotbarIndex() == 0
                && Avatar && Avatar->IsEquipmentPresentationReady()
                && !BillhookShown();
            if (!Passed)
                Results.Add(FString::Printf(TEXT("APPEARANCE_REBIND_DIAG book=%d page=%d style=%d old=%d hotbar=%d ready=%d billhook=%d"),
                    Controller->IsBookOpen(), Controller->BookPage(), Controller->Appearance.HairStyle,
                    *OldHairStyle, Controller->SelectedHotbarIndex(),
                    Avatar && Avatar->IsEquipmentPresentationReady(), BillhookShown()));
            return Passed;
        }, 0.3f);
    Add(TEXT("Changing hair colour and fitted garments never ghosts the selected Billhook"),
        [this]()
        {
            Controller->MenuSelect(1);
            Controller->MenuActivate();
        },
        [this, OldBodyPreset, BillhookShown]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Controller->IsBookOpen() && Controller->BookPage() == 6
                && Controller->Appearance.HairColor == (*OldBodyPreset + 1) % HomesteadLook::HairColorCount
                && Avatar && Avatar->IsEquipmentPresentationReady()
                && !BillhookShown();
        }, 0.3f);
    Add(TEXT("Leaving Appearance rebinds exactly one held Billhook without replay"),
        [this]() { Tap(EKeys::Escape); },
        [this, Anim, HackStarts, BillhookShown]()
        {
            const auto* Animation = Anim();
            return !Controller->IsBookOpen() && Animation
                && Controller->SelectedHotbarIndex() == 0 && BillhookShown()
                && Animation->MacheteStarts() == *HackStarts + 1;
        }, 0.3f);
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
    Add(TEXT("Number five selects the Hoe, which tills the forward cell through left click"),
        [this]() { Tap(EKeys::Five); Tap(EKeys::LeftMouseButton); },
        [this, TillX, TillY]()
        {
            return Controller->SelectedHotbarIndex() == 4
                && std::any_of(Controller->State().plots.begin(),
                    Controller->State().plots.end(), [TillX, TillY](const auto& Plot)
                    { return Plot.cellX == *TillX && Plot.cellY == *TillY; });
        }, 4.0f);
    // The MetaHuman heroine tills when the hoe bites, partway through its swing.
    Steps.Last().bCompleteWhenReady = true;
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
    Add(TEXT("E remains independent and plants the focused plot (Seeds chosen on the hotbar)"),
        [this]() { Controller->ChooseOnHotbar(Item::Seeds); Tap(EKeys::E); },
        [this, TillX, TillY]() { return std::any_of(
                Controller->State().plots.begin(), Controller->State().plots.end(),
                [TillX, TillY](const auto& Plot)
                { return Plot.cellX == *TillX && Plot.cellY == *TillY && Plot.planted; }); }, 4.0f);
    Steps.Last().bCompleteWhenReady = true;
    Add(TEXT("Controller right trigger with selected Watering Can waters the crop once"),
        [this]() { Tap(EKeys::Six); Tap(EKeys::Gamepad_RightTrigger); },
        [this, TillX, TillY]() { const auto Plot = std::find_if(
                Controller->State().plots.begin(), Controller->State().plots.end(),
                [TillX, TillY](const auto& Value)
                { return Value.cellX == *TillX && Value.cellY == *TillY; });
            return Controller->SelectedHotbarIndex() == 5
                && Plot != Controller->State().plots.end() && Plot->moisture > 0; }, 4.0f);
    Steps.Last().bCompleteWhenReady = true;
    Add(TEXT("CONTROLLED dropping the only Billhook removes its icon and held prop"),
        [this, BillhookDropId]()
        {
            Tap(EKeys::One);
            int32 GroupId = 0;
            for (const auto& Entry : Controller->State().inventoryLayout)
                if (Entry.item == Item::Billhook && Entry.wearableId == 0)
                    GroupId = Entry.groupId;
            Homestead::Point DropPosition;
            if (!Controller->ResolveDropPoint(DropPosition))
            {
                Finish(false, TEXT("Controlled Billhook drop has no safe placement beside the heroine."));
                return;
            }
            const auto Result = Controller->Sim.DropGroup(GroupId, 1, DropPosition,
                Controller->PlayerPoint(),
                Controller->Sim.GetRevision());
            if (!Result || Controller->State().worldDrops.empty())
            {
                Finish(false, FString(TEXT("Controlled Billhook drop failed: "))
                    + UTF8_TO_TCHAR(Result.message.c_str()));
                return;
            }
            *BillhookDropId = Controller->State().worldDrops.back().id;
        },
        [this, BillhookShown]()
        {
            return Controller->SelectedHotbarIndex() == 0
                && Controller->Simulation().Count(Item::Billhook) == 0
                && !Controller->HotbarSnapshot()[0].Available
                && !BillhookShown();
        }, 0.2f);
    Add(TEXT("CONTROLLED pickup restores the same Billhook assignment and held prop"),
        [this, BillhookDropId]()
        {
            const auto Result = Controller->Sim.PickUpDrop(*BillhookDropId,
                Controller->PlayerPoint());
            if (!Result) Finish(false, TEXT("Controlled Billhook pickup failed."));
        },
        [this, BillhookShown]()
        {
            return Controller->Simulation().Count(Item::Billhook) == 1
                && Controller->HotbarSnapshot()[0].Available
                && BillhookShown();
        }, 0.2f);
    Add(TEXT("A new woodland resets world-specific hotbar references safely"),
        [this]() { Controller->NewGame(); },
        [this]()
        {
            const auto Slots = Controller->HotbarSnapshot();
            return Controller->IsBookOpen() && Controller->SelectedHotbarIndex() == 0
                && Slots.Num() == 10 && Slots[0].Tool == Item::Billhook
                && !Slots[0].Available && !Slots[1].Available
                && !Slots[2].Available && !Slots[3].Available && Slots[6].Food;
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
    Add(TEXT("CONTROLLED Simulation Energy set to exactly the 10-Energy sprint floor"),
        [this, ReserveEnergy]()
        {
            // Sprint costs nothing, so the fixture sets Energy directly instead of running it down.
            const auto Result = Controller->Sim.SetEnergy(10.0);
            if (!Result)
            {
                Finish(false, UTF8_TO_TCHAR(Result.message.c_str()));
                return;
            }
            *ReserveEnergy = Controller->State().energy;
        },
        [this, ReserveEnergy]() { return FMath::IsNearlyEqual(*ReserveEnergy, 10.0, 0.001)
            && Controller->State().energy <= 10.0 && Controller->State().energy > 9.9; });
    Add(TEXT("A Shift tap below the Energy reserve is refused and she walks"),
        [this, ReserveEnergy]()
        {
            *ReserveEnergy = Controller->State().energy;
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this, ReserveEnergy]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->GetVelocity().Size2D() > 60
                && !Avatar->IsSprinting()
                && FMath::IsNearlyEqual(Avatar->GetCharacterMovement()->MaxWalkSpeed, Avatar->WalkSpeed())
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
    Add(TEXT("A Shift tap turns sprint on and she reaches a grounded sprint"),
        [this]()
        {
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->IsSprinting()
                && Avatar->GetVelocity().Size2D() > Avatar->WalkSpeed() + 30;
        }, 1.3f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1); };
    Add(TEXT("CONTROLLED work presentation stops sprint before the Billhook hack"),
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
            *WorkStarts = Animation->MacheteStarts();
            const Homestead::Point Player = Controller->PlayerPoint();
            Avatar->PlayMacheteHack({Player.x + 100, Player.y}, Item::Billhook);
            Axis(EKeys::Gamepad_LeftY, 0);
        },
        [this, WorkEnergy, WorkStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            const bool Passed = Avatar && Animation && !Avatar->IsSprinting()
                && FMath::IsNearlyEqual(Avatar->GetCharacterMovement()->MaxWalkSpeed, Avatar->WalkSpeed())
                && Animation->MacheteStarts() == *WorkStarts + 1
                && Animation->MacheteWeight() > 0.1f
                && FMath::Abs(Controller->State().energy - *WorkEnergy) < 0.1;
            if (!Passed && Avatar && Animation)
                Results.Add(FString::Printf(TEXT("WORK_SPRINT_DIAG sprinting=%d max=%.0f walk=%.0f starts=%u expected=%u weight=%.3f energy=%.2f/%.2f"),
                    Avatar->IsSprinting(), Avatar->GetCharacterMovement()->MaxWalkSpeed, Avatar->WalkSpeed(),
                    Animation->MacheteStarts(), *WorkStarts + 1, Animation->MacheteWeight(),
                    Controller->State().energy, *WorkEnergy));
            return Passed;
        }, 0.6f);
    // She stops dead, but the hack only starts once she's settled (the anim refuses it while she
    // still has momentum), so re-ask until it has begun.
    Steps.Last().Repeat = [this, WorkStarts]()
    {
        auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
        if (!Avatar || !Animation || Animation->MacheteStarts() != *WorkStarts || Animation->MacheteWeight() > 0.01f) return;
        const Homestead::Point Player = Controller->PlayerPoint();
        Avatar->PlayMacheteHack({Player.x + 100, Player.y}, Item::Billhook);
    };
    Add(TEXT("Cancel the isolated work pose with no queued sprint replay"),
        [this]()
        {
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (Avatar) Avatar->CancelAction(true);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
        },
        [this, WorkStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Avatar && Animation && !Avatar->IsSprinting()
                && Animation->ActionWeight() < 0.01f
                && Animation->MacheteStarts() == *WorkStarts + 1;
        }, 0.3f);
    Add(TEXT("A fresh Shift tap turns sprint back on after completed work"),
        [this]()
        {
            // Back to the open ground the first sprint crossed, clear of the trees further on.
            Teleport({-1000, 0});
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->IsSprinting()
                && Avatar->GetVelocity().Size2D() > Avatar->WalkSpeed() + 30;
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
                && FMath::IsNearlyEqual(Avatar->GetCharacterMovement()->MaxWalkSpeed, Avatar->WalkSpeed())
                && FMath::Abs(Controller->State().energy - *AirborneEnergy) < 0.1;
        }, 0.45f);
    Add(TEXT("A Shift tap turns sprint off after the airborne check"),
        [this]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && !Avatar->IsSprinting();
        });
}
