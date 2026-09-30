#include "SHomesteadMenuPrivate.h"

namespace HomesteadMenus
{
bool SHomesteadMenu::IsNoticeShowing() const
{
    if (NoticeText.IsEmpty() || !Controller.IsValid() || Controller->Toast().IsEmpty()) return false;
    const double Life = bNoticeError ? MenuNoticeStyle::ErrorSeconds : MenuNoticeStyle::Seconds;
    return FSlateApplication::Get().GetCurrentTime() - NoticeShownAt < Life;
}

void SHomesteadMenu::UpdateNotice()
{
    const double Now = FSlateApplication::Get().GetCurrentTime();
    const FString Current = Controller->Toast();
    const uint32 Serial = Controller->NoticeCount();
    const bool bError = Controller->ToastIsError();
    if (!bNoticePrimed)
    {
        // A notice from the moment the book opened (a load, a save) shows; an older world toast doesn't.
        bNoticePrimed = true;
        const bool bFresh = !Current.IsEmpty() && Controller->ToastSecondsLeft() >= (bError ? 8.0f : 5.0f) - 0.5f;
        NoticeText = Current;
        bNoticeError = bError;
        if (bFresh) NoticeShownAt = Now;
    }
    else if (!Current.IsEmpty() && (Serial != NoticeSerialSeen || Current != NoticeText || bError != bNoticeError))
    {
        // A replacement while one is up starts part-faded, so it reads as new without blinking out.
        const bool bWasShowing = IsNoticeShowing();
        NoticeText = Current;
        bNoticeError = bError;
        NoticeShownAt = Now - (bWasShowing ? MenuNoticeStyle::FadeInSeconds * 0.5 : 0.0);
    }
    NoticeSerialSeen = Serial;
    if (!NoticeCard || !IsNoticeShowing()) return;
    PlaceNotice();
    const double Age = Now - NoticeShownAt;
    const double Life = bNoticeError ? MenuNoticeStyle::ErrorSeconds : MenuNoticeStyle::Seconds;
    const float In = FMath::Clamp(static_cast<float>(Age / MenuNoticeStyle::FadeInSeconds), 0.0f, 1.0f);
    const float Out = FMath::Clamp(static_cast<float>((Life - Age) / MenuNoticeStyle::FadeOutSeconds), 0.0f, 1.0f);
    NoticeCard->SetRenderOpacity(FMath::InterpEaseOut(0.0f, 1.0f, In, 2.0f) * Out);
    // It settles in from just off its edge: up from below, or down from the tabs.
    const float Rise = (1.0f - FMath::InterpEaseOut(0.0f, 1.0f, In, 2.0f)) * MenuNoticeStyle::RiseDistance;
    NoticeCard->SetRenderTransform(FSlateRenderTransform(FVector2f(0.0f, bNoticeTop ? -Rise : Rise)));
}

void SHomesteadMenu::PlaceNotice()
{
    if (!BookOverlay || !NoticeSlot || !NoticeCard) return;
    const FGeometry& Book = BookOverlay->GetCachedGeometry();
    const FVector2D Size = Book.GetLocalSize();
    if (Size.X <= 0 || Size.Y <= 0) return;
    const auto LocalRect = [&Book](const SWidget& Widget)
    {
        const FGeometry& Bounds = Widget.GetCachedGeometry();
        const FVector2D Position = Bounds.GetAbsolutePosition();
        const FVector2D Extent = Bounds.GetAbsoluteSize();
        const FVector2D A = Book.AbsoluteToLocal(Position), B = Book.AbsoluteToLocal(Position + Extent);
        return FSlateRect(A.X, A.Y, B.X, B.Y);
    };
    const FVector2D Card = NoticeCard->GetDesiredSize();
    const float Pad = MenuNoticeStyle::Clearance;
    const float Top = TabBar ? LocalRect(*TabBar).Bottom + MenuNoticeStyle::GapBelowTabs : 110.0f;
    const float Left = (Size.X - Card.X) * 0.5f - Pad, Right = (Size.X + Card.X) * 0.5f + Pad;
    const FSlateRect BottomZone(Left, Size.Y - MenuNoticeStyle::BottomInset - Card.Y - Pad, Right, Size.Y);
    const FSlateRect TopZone(Left, Top - Pad, Right, Top + Card.Y + Pad);
    bool bCoversBottom = false, bCoversTop = false;
    if (const auto Focused = FocusWidget())
    {
        const FSlateRect Target = LocalRect(*Focused);
        bCoversBottom = FSlateRect::DoRectanglesIntersect(Target, BottomZone);
        bCoversTop = FSlateRect::DoRectanglesIntersect(Target, TopZone);
    }
    const bool bTop = bCoversBottom && !bCoversTop;
    const FMargin Padding = bTop ? FMargin(0, Top, 0, 0) : FMargin(0, 0, 0, MenuNoticeStyle::BottomInset);
    if (bTop != bNoticeTop || NoticeSlot->GetPadding() != Padding)
    {
        bNoticeTop = bTop;
        NoticeSlot->SetVerticalAlignment(bTop ? VAlign_Top : VAlign_Bottom);
        NoticeSlot->SetPadding(Padding);
    }
}

bool SHomesteadMenu::StartCraftHold(ECraftInput Input)
{
    StopCraftHold();
    if (!Controller.IsValid() || Dialog != EDialog::None || bSaving || SeenPage != 1
        || Region != ERegion::Content || !Entries.IsValidIndex(ContentSelection))
        return false;
    const auto& Row = Entries[ContentSelection];
    if (Row.Subject != EHomesteadMenuSubject::Recipe || !Row.HasRecipeState
        || !Row.RecipeState.craftable)
        return false;
    CraftHoldRecipe = Row.SubjectId;
    CraftHoldElapsed = 0;
    CraftBeat = 0;
    CraftInput = Input;
    return true;
}

void SHomesteadMenu::StopCraftHold()
{
    CraftHoldRecipe = INDEX_NONE;
    CraftHoldElapsed = 0;
    CraftBeat = 0;
    CraftInput = ECraftInput::None;
}

bool SHomesteadMenu::IsHoldingRecipe(int32 Recipe) const
{
    return CraftInput != ECraftInput::None && CraftHoldRecipe == Recipe;
}

float SHomesteadMenu::CraftFlash(int32 Recipe) const
{
    if (Recipe != CraftFlashRecipe) return 0.0f;
    constexpr double FlashSeconds = 0.45;
    return FMath::Clamp(1.0f - static_cast<float>((FPlatformTime::Seconds() - CraftFlashStart) / FlashSeconds), 0.0f, 1.0f);
}

bool SHomesteadMenu::PointerAction()
{
    return Controller.IsValid() && Controller->MenuAcceptsPhysicalInput();
}

void SHomesteadMenu::RunAction(EHomesteadItemAction Action)
{
    if (!Controller.IsValid() || Dialog != EDialog::None || bSaving) return;
    if (!Entries.IsValidIndex(ContentSelection)) return;
    const auto Row = Entries[ContentSelection];
    if (Row.Subject != EHomesteadMenuSubject::Legacy)
    {
        PendingRow = Row; PendingAction = Action; PendingRevision = Controller->Simulation().GetRevision();
        if ((Action == EHomesteadItemAction::Transfer || Action == EHomesteadItemAction::Split
            || Action == EHomesteadItemAction::Drop)
            && Row.Subject == EHomesteadMenuSubject::ItemGroup)
        {
            Amount = 1; MaximumAmount = Row.Quantity - (Action == EHomesteadItemAction::Split ? 1 : 0);
            if (Action == EHomesteadItemAction::Transfer)
            {
                const int32 Used = Row.ContainerId > 0 ? Controller->Simulation().UsedCapacity()
                    : Controller->Simulation().ChestUsedCapacity(Row.DestinationId);
                MaximumAmount = Used < 0 ? 0 : FMath::Min(MaximumAmount,
                    Homestead::ContainerCapacity(Row.ContainerId > 0 ? 0 : Row.DestinationId) - Used);
            }
            if (MaximumAmount < 1)
            { Controller->MenuItemAction(Row, Action, 1, PendingRevision); return; }
            SetDialog(EDialog::Amount); return;
        }
        if (Action == EHomesteadItemAction::Drop && Row.Subject == EHomesteadMenuSubject::Wearable)
        { SetDialog(EDialog::DropWearable); return; }
        if (Action == EHomesteadItemAction::Merge)
        {
            MergeTargets.Reset();
            for (const auto& Target : Entries)
                if (Target.Subject == EHomesteadMenuSubject::ItemGroup && Target.Id == Row.Id && Target.ContainerId == Row.ContainerId
                    && Target.SubjectId != Row.SubjectId) MergeTargets.Add(Target.SubjectId);
            SetDialog(EDialog::Merge); return;
        }
        Controller->MenuItemAction(Row, Action, 1, PendingRevision);
        Refresh();
        return;
    }
    Controller->MenuSelect(RowIndices[ContentSelection]);
    if (SeenPage == 4 && Row.Id == 8) SetDialog(EDialog::Restart);
    else Controller->MenuActivate();
}

void SHomesteadMenu::Activate()
{
    if (Dialog == EDialog::Amount && DialogSelection < 0)
    { bEditingAmount = !bEditingAmount; BuildDialog(); return; }
    if (Dialog != EDialog::None) { DialogAction(DialogSelection); return; }
    if (Region == ERegion::Tabs) ChangePage(FocusedTab);
    else if (Region == ERegion::Inventory) ChangeInventoryView(InventorySelection);
    else if (Region == ERegion::Equipment) FocusEquipment(EquipmentSelection);
    else if (Region == ERegion::Session) { if (SessionSelection == 0) Back(); else SetSettingsTab(SessionSelection - 1); }
    else if (Region == ERegion::Recovery) { if (RecoverySelection == 0) Controller->MenuRetry(); else ChangePage(4); }
    else if (Region == ERegion::Portrait)
    {
        if (PortraitSelection < 0) { PortraitSelection = 0; bFocusPending = true; SynchronizeFocus(); }
        else if (PortraitSelection < 2) Controller->OrbitMenuPortrait(PortraitSelection == 0 ? -20 : 20);
        else Controller->ZoomMenuPortrait();
    }
    else if (Region == ERegion::Actions && Actions.IsValidIndex(ActionSelection)) RunAction(Actions[ActionSelection]);
    else if (Region == ERegion::Content && Entries.IsValidIndex(ContentSelection)
        && (SeenPage == 4 || SeenPage == 2 || SeenPage == 6))
        RunAction(EHomesteadItemAction::Primary);
    else if (SeenPage == 0)
    {
        if (Region == ERegion::Content) OpenItemContextMenu(ContentSelection, false);
    }
    else
    {
        Region = Actions.IsEmpty() ? ERegion::Details : ERegion::Actions;
        ActionSelection = 0; bFocusPending = true; SynchronizeFocus();
    }
}

void SHomesteadMenu::ChangePage(int32 Page)
{
    if (!Controller.IsValid() || Dialog != EDialog::None) return;
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    StopCraftHold();
    Controller->MenuPage(Page);
    Refresh();
}
}
