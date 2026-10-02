#include "SHomesteadMenuPrivate.h"

namespace HomesteadMenus
{
FNavigationReply SHomesteadMenu::OnNavigation(const FGeometry&, const FNavigationEvent&)
{
    return FNavigationReply::Stop();
}

bool SHomesteadMenu::HandleKey(FKey Key, EInputEvent Event, float InputAmount)
{
    if (SeenPage == 7 && MapView && Dialog == EDialog::None && !bRecovery && !bSaving
        && HandleMapKey(Key, Event, InputAmount)) return true;
    if (SeenPage == 6 && Dialog == EDialog::None && !bRecovery && !bSaving
        && HandleAppearanceKey(Key, Event, InputAmount)) return true;
    if (Key == EKeys::LeftControl || Key == EKeys::RightControl) bControl = Event != IE_Released;
    if (Key == EKeys::LeftShift || Key == EKeys::RightShift) bShift = Event != IE_Released;
    if (Event == IE_Axis)
    {
        if (Region == ERegion::Portrait && Dialog == EDialog::None && Key == EKeys::Gamepad_RightX)
        {
            if (FMath::Abs(InputAmount) > 0.3f) Controller->OrbitMenuPortrait(InputAmount * 1.5f);
            return true;
        }
        if (Key == EKeys::Gamepad_LeftX || Key == EKeys::Gamepad_LeftY)
        {
            const double Now = FPlatformTime::Seconds();
            if (Key == EKeys::Gamepad_LeftX) LeftStick.Sample(true, InputAmount, Now);
            else LeftStick.Sample(false, -InputAmount, Now);
            const auto Direction = LeftStick.Poll(Now);
            if (Direction.Any() && Controller->UsesGamepad() && !bSaving) NavigateDirection(Direction);
        }
        return true;
    }
    if (Event == IE_Repeat && (Key == EKeys::Left || Key == EKeys::Right || Key == EKeys::Up || Key == EKeys::Down
        || Key == EKeys::Gamepad_DPad_Left || Key == EKeys::Gamepad_DPad_Right || Key == EKeys::Gamepad_DPad_Up || Key == EKeys::Gamepad_DPad_Down))
        Event = IE_Pressed;
    const bool ActivateKey = Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::E
        || Key == EKeys::Gamepad_FaceButton_Bottom || Key == EKeys::LeftMouseButton;
    if (Event == IE_Released && ActivateKey)
    {
        const bool MatchingInput = (CraftInput == ECraftInput::Pointer && Key == EKeys::LeftMouseButton)
            || (CraftInput == ECraftInput::Controller && Key == EKeys::Gamepad_FaceButton_Bottom)
            || (CraftInput == ECraftInput::Keyboard
                && (Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::E));
        if (MatchingInput) StopCraftHold();
        return true;
    }
    if (Event != IE_Pressed || bSaving) return true;
    if (!Key.IsMouseButton()) Hover = INDEX_NONE;
    if (HandleRenameKey(Key)) return true;
    if (Region == ERegion::Portrait && Dialog == EDialog::None
        && (Key == EKeys::Z || Key == EKeys::Gamepad_RightThumbstick))
    { Controller->ZoomMenuPortrait(); return true; }
    if (bRecovery && Dialog == EDialog::None)
    {
        if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Escape
            || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right) ChangePage(4);
        if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::Escape
            || Key == EKeys::Gamepad_Special_Right || Key == EKeys::Gamepad_FaceButton_Right) return true;
    }
    // A stepped sound level: Back puts it back unsaved; confirm saves it (and doesn't also step it).
    if (bAudioStepEdit && Dialog == EDialog::None)
    {
        if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right) { CancelAudioStep(); return true; }
        if (ActivateKey && !Key.IsMouseButton()) { CommitAudioStep(); Refresh(); return true; }
    }
    if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right || Key == EKeys::I || Key == EKeys::Gamepad_Special_Right) { Back(); return true; }
    if (ActivateKey)
    {
        if (Dialog != EDialog::None && !Key.IsMouseButton()) { Activate(); return true; }
        if (SeenPage == 0 && bShift && !Key.IsMouseButton() && Region == ERegion::Content
            && Entries.IsValidIndex(ContentSelection))
        {
            QuickMove(ContentSelection);
            return true;
        }
        if (SeenPage == 0 && bControl && !Key.IsMouseButton() && Region == ERegion::Content
            && Entries.IsValidIndex(ContentSelection)
            && Entries[ContentSelection].Subject == EHomesteadMenuSubject::ItemGroup)
        {
            SplitSelectedHalf();
            return true;
        }
        const ECraftInput Input = Key == EKeys::LeftMouseButton ? ECraftInput::Pointer
            : Key == EKeys::Gamepad_FaceButton_Bottom ? ECraftInput::Controller : ECraftInput::Keyboard;
        if (SeenPage == 1 && Region == ERegion::Content && Entries.IsValidIndex(ContentSelection)
            && Entries[ContentSelection].Subject == EHomesteadMenuSubject::Recipe)
            StartCraftHold(Input);
        else if (SeenPage == 0 && Region == ERegion::Content
            && Entries.IsValidIndex(ContentSelection))
            BeginOrCommitVirtualItemDrag();
        else
            Activate();
        return true;
    }
    int32 Dx = Key == EKeys::Right || Key == EKeys::Gamepad_DPad_Right ? 1 : Key == EKeys::Left || Key == EKeys::Gamepad_DPad_Left ? -1 : 0;
    int32 Dy = Key == EKeys::Down || Key == EKeys::Gamepad_DPad_Down ? 1 : Key == EKeys::Up || Key == EKeys::Gamepad_DPad_Up ? -1 : 0;
    if (Dialog != EDialog::None)
    {
        if (Dialog == EDialog::Quantity && (Dx || Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_LeftShoulder))
        {
            AdjustQuantity(Dx ? Dx : Key == EKeys::Gamepad_RightShoulder ? 10 : -10);
            return true;
        }
        if (Dialog == EDialog::Amount)
        {
            const int32 Delta = Key == EKeys::Gamepad_RightShoulder ? 10 : Key == EKeys::Gamepad_LeftShoulder ? -10 : 0;
            if (Delta && bEditingAmount) { Amount = FMath::Clamp(Amount + Delta, 1, MaximumAmount); BuildDialog(); return true; }
        }
        if (Dx || Dy) NavigateDialog({Dx, Dy});
        if (Key == EKeys::Tab) NavigateDialog({0, bShift ? -1 : 1});
        return true;
    }
    if (Key == EKeys::Gamepad_LeftShoulder) { ChangePage(ShiftFieldBookPage(SeenPage, -1)); return true; }
    if (Key == EKeys::Gamepad_RightShoulder) { ChangePage(ShiftFieldBookPage(SeenPage, 1)); return true; }
    // The world's book shortcuts work inside it too, as the controls strip says: C crafting, B building.
    if (Key == EKeys::C && !bControl) { ChangePage(1); return true; }
    if (Key == EKeys::B && !bControl) { ChangePage(2); return true; }
    if (Key == EKeys::Tab)
    {
        if (bControl) ChangePage(ShiftFieldBookPage(SeenPage, bShift ? -1 : 1));
        else CycleRegion(bShift ? -1 : 1);
        return true;
    }
    if (Key == EKeys::Gamepad_LeftTrigger) { CycleRegion(-1); return true; }
    if (Key == EKeys::Gamepad_RightTrigger) { CycleRegion(1); return true; }
    if (SeenPage == 0 && Key == EKeys::S)
    {
        if (Controller->MenuSortPack()) Refresh();
        return true;
    }
    // Auto-store onto the open chest's matching stacks (gamepad: the item menu's option, since Y is taken).
    if (SeenPage == 0 && Key == EKeys::T && Controller->ActiveStorageChest().IsSet())
    {
        if (Controller->MenuStoreMatching()) Refresh();
        return true;
    }
    if (Key == EKeys::Gamepad_FaceButton_Left)
    {
        if (SeenPage == 0 && GetSelectedSubject())
            SplitSelectedHalf();
        return true;
    }
    if (SeenPage == 0 && (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top)
        && Region == ERegion::Content && GetSelectedSubject())
    {
        OpenItemContextMenu(ContentSelection, false);
        return true;
    }
    if (SeenPage == 0 && (Key == EKeys::F || Key == EKeys::Gamepad_FaceButton_Top) && Region == ERegion::Hotbar)
    {
        OpenHotbarSlotMenu(HotbarSelection, false);
        return true;
    }
    if (Key == EKeys::G || Key == EKeys::Gamepad_FaceButton_Top)
    {
        if (SeenPage == 0 && Key == EKeys::Gamepad_FaceButton_Top)
        {
            if (Controller->MenuSortPack()) Refresh();
        }
        return true;
    }
    if (Dx || Dy) { LeftStick.Reset(); NavigateDirection({Dx, Dy}); }
    return true;
}

FReply SHomesteadMenu::OnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
    return FReply::Handled();
}

FReply SHomesteadMenu::OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event)
{
    bControl = Event.IsControlDown(); bShift = Event.IsShiftDown();
    if (Controller.IsValid()) Controller->MenuPhysicalInput(Event.GetKey(), Event.IsRepeat() ? IE_Repeat : IE_Pressed);
    return FReply::Handled();
}

FReply SHomesteadMenu::OnKeyUp(const FGeometry&, const FKeyEvent& Event)
{
    bControl = Event.IsControlDown(); bShift = Event.IsShiftDown();
    if (Controller.IsValid()) Controller->MenuPhysicalInput(Event.GetKey(), IE_Released, 0);
    return FReply::Handled();
}

FReply SHomesteadMenu::OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event)
{
    if (Controller.IsValid()) Controller->MenuPhysicalInput(Event.GetKey(), IE_Axis, Event.GetAnalogValue());
    return FReply::Handled();
}

FReply SHomesteadMenu::OnMouseMove(const FGeometry&, const FPointerEvent& Event)
{
    if (bOrbitDragging && HasMouseCapture())
    {
        const FVector2D Position = Event.GetScreenSpacePosition();
        const FVector2D Delta = Position - OrbitDragLast;
        OrbitDragLast = Position;
        if (Controller.IsValid())
            Controller->MenuOrbitAppearance(Delta.X * MenuAppearanceInput::DragYawPerPixel, -Delta.Y * MenuAppearanceInput::DragPitchPerPixel);
        return FReply::Handled();
    }
    PointerItemDragMove(Event.GetScreenSpacePosition());
    return FReply::Handled();
}

FReply SHomesteadMenu::OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event)
{
    // A left drag anywhere the Appearance page has no control of its own turns her around.
    if (SeenPage == 6 && Dialog == EDialog::None && Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bOrbitDragging = true;
        OrbitDragLast = Event.GetScreenSpacePosition();
        return FReply::Handled().CaptureMouse(SharedThis(this));
    }
    return FReply::Unhandled();
}

FReply SHomesteadMenu::OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event)
{
    if (bOrbitDragging && Event.GetEffectingButton() == EKeys::LeftMouseButton)
    {
        bOrbitDragging = false;
        return FReply::Handled().ReleaseMouseCapture();
    }
    return FReply::Unhandled();
}

void SHomesteadMenu::OnMouseCaptureLost(const FCaptureLostEvent& Event)
{
    SCompoundWidget::OnMouseCaptureLost(Event);
    bOrbitDragging = false;
}

bool SHomesteadMenu::HandleAppearanceKey(FKey Key, EInputEvent Event, float InputAmount)
{
    if (Key == EKeys::MouseWheelAxis)
    {
        // The wheel zooms here, wherever the pointer is, instead of scrolling or picking a tool.
        if (Event == IE_Axis && InputAmount != 0.0f && Controller.IsValid()) Controller->MenuZoomAppearance(InputAmount > 0 ? 1.0f : -1.0f);
        return true;
    }
    if (Event == IE_Axis && (Key == EKeys::Gamepad_RightX || Key == EKeys::Gamepad_RightY))
    {
        const float Value = FMath::Abs(InputAmount) > MenuAppearanceInput::StickDeadZone ? InputAmount : 0.0f;
        (Key == EKeys::Gamepad_RightX ? OrbitStickX : OrbitStickY) = Value;
        return true;
    }
    bool* Held = Key == EKeys::A ? &bOrbitLeft : Key == EKeys::D ? &bOrbitRight
        : Key == EKeys::W ? &bOrbitUp : Key == EKeys::S ? &bOrbitDown : nullptr;
    if (!Held) return false;
    if (Event == IE_Pressed || Event == IE_Repeat) *Held = true;
    else if (Event == IE_Released) *Held = false;
    return true;
}

void SHomesteadMenu::ClearAppearanceOrbit()
{
    bOrbitLeft = bOrbitRight = bOrbitUp = bOrbitDown = false;
    OrbitStickX = OrbitStickY = 0.0f;
}

FReply SHomesteadMenu::OnMouseWheel(const FGeometry&, const FPointerEvent& Event)
{
    // On Appearance the wheel zooms through one route only: the menu's input preprocessor sends every
    // notch to HandleAppearanceKey (as the Map tab does), so here it's only kept from scrolling.
    if (SeenPage == 6 && Dialog == EDialog::None) return FReply::Handled();
    if (Dialog == EDialog::Quantity)
    {
        if (Controller.IsValid() && Controller->MenuAcceptsPhysicalInput())
            AdjustQuantity((Event.GetWheelDelta() > 0 ? 1 : -1) * (Event.IsShiftDown() ? 10 : 1));
        return FReply::Handled();
    }
    if (Controller.IsValid() && Controller->MenuAcceptsPhysicalInput() && Scroll)
        Scroll->SetScrollOffset(FMath::Max(0.0f, Scroll->GetScrollOffset() - Event.GetWheelDelta() * 80));
    return FReply::Handled();
}
}
