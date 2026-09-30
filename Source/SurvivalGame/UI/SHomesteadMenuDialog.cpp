#include "SHomesteadMenuPrivate.h"

namespace HomesteadMenus
{
void SHomesteadMenu::BuildPopup()
{
    DialogButtons.Reset();
    AmountControl.Reset();
    DialogScroll.Reset();
    ModalHost->SetVisibility(EVisibility::Visible);
    const bool Quantity = Dialog == EDialog::Quantity;
    const bool Body = !PopupBody.IsEmpty();
    const float Width = Quantity ? 340.0f : Body ? 420.0f : 280.0f;
    const float Height = 60.0f + PopupOptions.Num() * 48.0f + (Quantity ? 110.0f : 0.0f) + (Body ? 110.0f : 0.0f);
    // Open beside the pointer, kept inside the book.
    const FGeometry Frame = Root->GetCachedGeometry();
    const FVector2D Size = Frame.GetLocalSize();
    FVector2D Local = Frame.AbsoluteToLocal(PopupAnchor) + FVector2D(8, 8);
    if (Size.X <= 0 || Size.Y <= 0 || !FSlateApplication::Get().IsInitialized()) Local = FVector2D(200, 150);
    // A road sign's walk prompt sits in the middle of the book (rebuilt once the book has a size).
    bPopupCentered = bCenterPopup && Size.X > 0 && Size.Y > 0;
    if (bPopupCentered) Local = (FVector2D(Size) - FVector2D(Width, Height)) * 0.5f;
    Local.X = FMath::Clamp(Local.X, 8.0f, FMath::Max(8.0f, static_cast<float>(Size.X) - Width - 8.0f));
    Local.Y = FMath::Clamp(Local.Y, 8.0f, FMath::Max(8.0f, static_cast<float>(Size.Y) - Height - 8.0f));
    TSharedPtr<SVerticalBox> List;
    ModalHost->SetContent(
        SNew(SOverlay)
        + SOverlay::Slot()
        [
            SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
            .ButtonColorAndOpacity(FLinearColor(0, 0, 0, 0.2f))
            .OnClicked_Lambda([this]() { if (PointerAction()) SetDialog(EDialog::None); return FReply::Handled(); })
        ]
        + SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(FMargin(Local.X, Local.Y, 0, 0))
        [
            SNew(SBox).WidthOverride(Width)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(MenuGold).Padding(2)
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(PopupPine).Padding(10)
                    [ SAssignNew(List, SVerticalBox) ]
                ]
            ]
        ]);
    List->AddSlot().AutoHeight().Padding(4, 0, 4, 8)
    [
        SNew(STextBlock).Text(FText::FromString(PopupTitle)).ColorAndOpacity(MenuGold)
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16))
    ];
    if (Body)
        List->AddSlot().AutoHeight().Padding(4, 0, 4, 10)
        [
            SNew(STextBlock).Text(FText::FromString(PopupBody)).ColorAndOpacity(Ink).AutoWrapText(true)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14)).LineHeightPercentage(1.1f)
        ];
    if (Quantity)
    {
        List->AddSlot().AutoHeight().HAlign(HAlign_Center).Padding(0, 2)
        [
            SNew(STextBlock).ColorAndOpacity(Ink).Font(FCoreStyle::GetDefaultFontStyle("Bold", 28))
            .Text_Lambda([this]() { return FText::FromString(FString::Printf(TEXT("%d  of  %d"), Amount, MaximumAmount)); })
        ];
        List->AddSlot().AutoHeight().Padding(4, 6)
        [
            SNew(SSlider).MinValue(1.0f).MaxValue(static_cast<float>(MaximumAmount)).StepSize(1.0f).MouseUsesStep(true)
            .IsFocusable(false)
            .Value_Lambda([this]() { return static_cast<float>(Amount); })
            .OnValueChanged_Lambda([this](float Value)
                { Amount = FMath::Clamp(FMath::RoundToInt(Value), 1, MaximumAmount); })
        ];
        List->AddSlot().AutoHeight().Padding(4, 0, 4, 8)
        [
            SNew(STextBlock).Text(FText::FromString(TEXT("Drag, scroll the wheel, or press Left / Right.")))
            .ColorAndOpacity(Muted).Font(FCoreStyle::GetDefaultFontStyle("Regular", 13)).AutoWrapText(true)
        ];
    }
    for (int32 Index = 0; Index < PopupOptions.Num(); ++Index)
    {
        const TFunction<FString()> Label = PopupOptions[Index].Label;
        const TOptional<FLinearColor> Swatch = PopupOptions[Index].Swatch;
        const TFunction<bool()> Enabled = PopupOptions[Index].Enabled;
        TSharedRef<SButton> Button = SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(FMargin(12, 8))
            .ButtonColorAndOpacity_Lambda([this, Index]() { return DialogSelection == Index ? MenuGold : Selected; })
            .IsEnabled_Lambda([Enabled]() { return !Enabled || Enabled(); })
            .OnHovered_Lambda([this, Index]() { DialogSelection = Index; })
            .OnClicked_Lambda([this, Index]() { if (PointerAction()) DialogAction(Index); return FReply::Handled(); })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
                [
                    SNew(SBox).WidthOverride(22).HeightOverride(22)
                    .Visibility(Swatch.IsSet() ? EVisibility::Visible : EVisibility::Collapsed)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                        .BorderBackgroundColor(PineInk)
                        [ SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Swatch.Get(FLinearColor::White)) ]
                    ]
                ]
                + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                [
                    SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                    .ColorAndOpacity_Lambda([this, Index]() { return DialogSelection == Index ? FSlateColor(PineInk) : FSlateColor(Ink); })
                    .Text_Lambda([Label]() { return FText::FromString(Label ? Label() : FString()); })
                ]
            ];
        Button->SetOnFocusReceived(FSimpleDelegate::CreateLambda([this, Index]()
            { if (!bSynchronizingFocus) DialogSelection = Index; }));
        DialogButtons.Add(Button);
        List->AddSlot().AutoHeight().Padding(0, 3)[ Button ];
    }
    bFocusPending = true;
}

void SHomesteadMenu::NavigateDialog(HomesteadMenuNavigation::Direction Direction)
{
    if (Dialog == EDialog::Amount && bEditingAmount)
    {
        if (Direction.x) Amount = FMath::Clamp(Amount + Direction.x, 1, MaximumAmount);
        else { bEditingAmount = false; DialogSelection = 0; }
        BuildDialog();
        return;
    }
    const int32 Delta = Direction.y ? Direction.y : Direction.x;
    if (!Delta) return;
    if (Dialog == EDialog::Amount && DialogSelection == 0 && Direction.y < 0) DialogSelection = -1;
    else if (DialogSelection < 0) DialogSelection = 0;
    else DialogSelection = FMath::Clamp(DialogSelection + Delta, 0, DialogCount() - 1);
    if (DialogScroll && DialogButtons.IsValidIndex(DialogSelection))
        DialogScroll->ScrollDescendantIntoView(DialogButtons[DialogSelection], false);
    bFocusPending = true;
    SynchronizeFocus();
}

void SHomesteadMenu::Back()
{
    if (bSaving) return;
    if (bVirtualDraggingItem) { CancelVirtualItemDrag(); return; }
    if (HeldHotbarRow.IsSet() || HeldHotbarSlot != INDEX_NONE) { CancelHotbarHolds(); return; }
    CancelPointerItemDrag();
    StopCraftHold();
    if (Dialog == EDialog::Amount && bEditingAmount) { bEditingAmount = false; BuildDialog(); return; }
    if (Dialog != EDialog::None) { SetDialog(EDialog::None); return; }
    if (Controller.IsValid()) Controller->MenuBack();
}

bool SHomesteadMenu::PrepareQuickAction()
{
    if (bSaving) return false;
    if (Dialog == EDialog::Amount || Dialog == EDialog::Merge || Dialog == EDialog::Context || Dialog == EDialog::Quantity)
        SetDialog(EDialog::None);
    return Dialog == EDialog::None;
}

void SHomesteadMenu::RequestExit()
{
    if (Controller.IsValid() && !bSaving) { DialogError.Reset(); SetDialog(EDialog::Exit); }
}

void SHomesteadMenu::ShowSaveFailure(const FString& Error)
{
    bSaving = false;
    DialogError = Error;
    if (Dialog == EDialog::Exit) { BuildDialog(); bFocusPending = true; }
    else SetDialog(EDialog::SaveFailed);
}

void SHomesteadMenu::ShowGraphicsSaveFailure(const FString& Error)
{
    bSaving = false; DialogError = Error; SetDialog(EDialog::GraphicsFailed);
}

void SHomesteadMenu::SetDialog(EDialog Value)
{
    CancelPointerItemDrag();
    CancelVirtualItemDrag();
    StopCraftHold();
    Dialog = Value; DialogSelection = 0;
    if (Value == EDialog::None) PopupBody.Reset();
    if (Value != EDialog::Context) { bTravelPrompt = false; bCenterPopup = false; }
    bEditingAmount = false;
    LeftStick.Reset();
    PendingDirection = {};
    // Item popups float over a live, readable book; the click-away scrim already blocks it.
    if (Root) Root->SetEnabled(Value == EDialog::None || Value == EDialog::Context || Value == EDialog::Quantity);
    BuildDialog();
    bFocusPending = true;
}

int32 SHomesteadMenu::DialogCount() const
{
    if (Dialog == EDialog::Amount) return 4;
    if (Dialog == EDialog::Context || Dialog == EDialog::Quantity) return PopupOptions.Num();
    if (Dialog == EDialog::Merge) return MergeTargets.Num() + 1;
    if (Dialog == EDialog::Exit) return 2;
    if (Dialog == EDialog::RenameChest) return 3 + UE_ARRAY_COUNT(MenuChestNames::Suggestions);
    return Dialog == EDialog::SaveFailed || Dialog == EDialog::GraphicsFailed ? 3 : 2;
}

void SHomesteadMenu::BuildDialog()
{
    if (Dialog == EDialog::None) { ModalHost->SetVisibility(EVisibility::Collapsed); AmountControl.Reset(); return; }
    if (Dialog == EDialog::Context || Dialog == EDialog::Quantity) { BuildPopup(); return; }
    FString Title, Description;
    TArray<FString> Labels;
    if (Dialog == EDialog::Amount)
    {
        Title = PendingAction == EHomesteadItemAction::Split ? TEXT("Split stack")
            : PendingAction == EHomesteadItemAction::Drop ? TEXT("Drop items") : TEXT("Transfer items");
        const FString Destination = PendingAction == EHomesteadItemAction::Split ? TEXT("A new stack in the same container")
            : PendingAction == EHomesteadItemAction::Drop ? TEXT("Nearby ground")
            : PendingRow.ContainerId > 0 ? FString(TEXT("Your pack")) : Controller->ChestDisplayName(PendingRow.DestinationId);
        Description = FString::Printf(TEXT("%s\nFrom: %s\nTo: %s\n\nAmount: %d / %d\nChoose Amount and activate to edit. While editing: Left/Right one, LB/RB ten; Back finishes editing."),
            *PendingRow.Name, *PendingRow.Location, *Destination, Amount, MaximumAmount);
        Labels = {TEXT("Cancel"), TEXT("Confirm"), TEXT("One"), TEXT("All available")};
    }
    else if (Dialog == EDialog::RenameChest)
    {
        Title = TEXT("Name this chest");
        Description = FString::Printf(TEXT("Type a name of up to %d letters and press Enter, or choose one below. ")
            TEXT("You'll see it when you walk up to the chest.\n\nName:  %s_"),
            Homestead::Chests::MaxNameLength, *RenameDraft);
        Labels = {TEXT("Cancel"), TEXT("Save name"), FString::Printf(TEXT("Clear name (%s)"), UTF8_TO_TCHAR(Homestead::Chests::DefaultName))};
        for (const TCHAR* Suggestion : MenuChestNames::Suggestions) Labels.Add(FString::Printf(TEXT("Call it %s"), Suggestion));
    }
    else if (Dialog == EDialog::DropWearable)
    {
        Title = TEXT("Drop garment");
        Description = FString::Printf(TEXT("%s\n\nPlace this garment on clear ground nearby?"),
            *PendingRow.Name);
        Labels = {TEXT("Cancel"), TEXT("Drop")};
    }
    else if (Dialog == EDialog::Merge)
    {
        Title = TEXT("Merge stacks");
        Description = MergeTargets.IsEmpty() ? TEXT("No other matching stack exists in this container.")
            : TEXT("Choose the destination stack. Quantities are combined; nothing changes until you confirm a destination.");
        Labels.Add(TEXT("Cancel"));
        for (int32 Target : MergeTargets)
        {
            const auto* Row = Entries.FindByPredicate([Target](const FHomesteadRow& Entry)
                { return Entry.Subject == EHomesteadMenuSubject::ItemGroup && Entry.SubjectId == Target; });
            Labels.Add(FString::Printf(TEXT("Stack #%d  (%d)"), Target, Row ? Row->Quantity : 0));
        }
    }
    else if (Dialog == EDialog::Exit)
    {
        Title = TEXT("Quit game");
        Description = (DialogError.IsEmpty() ? FString() : DialogError + TEXT("\n\n"))
            + Controller->MenuSaveStatus();
        Labels = {TEXT("Save & Quit"), TEXT("Quit without Saving")};
    }
    else if (Dialog == EDialog::SaveFailed)
    {
        Title = TEXT("Your progress was not saved");
        Description = DialogError + TEXT("\n\nThe game is still paused. Retry or choose what to do next.");
        Labels = {TEXT("Return to Settings"), TEXT("Retry save and quit"), TEXT("Quit without saving...")};
    }
    else if (Dialog == EDialog::GraphicsFailed)
    {
        Title = TEXT("World saved; video preference not confirmed");
        Description = DialogError + TEXT("\n\nYour homestead was saved successfully. You can retry the preference or explicitly leave it unsaved.");
        Labels = {TEXT("Return to Settings"), TEXT("Retry and quit"), TEXT("Quit with video preference unverified")};
    }
    else if (Dialog == EDialog::Unsaved)
    {
        Title = TEXT("Quit without saving?");
        Description = TEXT("Progress since the last successful save will be lost.\n\n") + Controller->MenuSaveStatus();
        Labels = {TEXT("Cancel"), TEXT("Quit without saving")};
    }
    else if (Dialog == EDialog::TestReset)
    {
        Title = TEXT("This test save could not be loaded");
        Description = Controller->MenuLoadProblem() + TEXT("\n\nA reset starts a fresh test world. It is not a migration of the old progress.");
        Labels = {TEXT("Stay in Settings"), TEXT("Start a new test woodland")};
    }
    else
    {
        Title = TEXT("Start a new woodland?");
        Description = TEXT("This replaces the current test session with a new woodland seed. Unsaved progress will be lost.");
        Labels = {TEXT("Cancel"), TEXT("Start a new woodland")};
    }
    TSharedPtr<SVerticalBox> Choices;
    DialogButtons.Reset();
    AmountControl.Reset();
    ModalHost->SetVisibility(EVisibility::Visible);
    ModalHost->SetContent(
        // A compact card centred over a dimmed screen, not a full-width sheet.
        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.45f)).Padding(0)
        .HAlign(HAlign_Center).VAlign(VAlign_Center)
        [
            SNew(SBox).WidthOverride(Dialog == EDialog::Exit || Dialog == EDialog::Unsaved ? 620.0f : 760.0f)
            .MaxDesiredHeight(820)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0.01f, 0.025f, 0.015f, 0.92f)).Padding(FMargin(44, 34))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 14)[ Text(Title, 30) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 18)
                    [ SNew(SBox).MaxDesiredHeight(360)
                        [ SNew(SScrollBox) + SScrollBox::Slot()[ Text(Description, 20) ] ] ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SBox).MaxDesiredHeight(230)
                        [ SAssignNew(DialogScroll, SScrollBox) + SScrollBox::Slot()[ SAssignNew(Choices, SVerticalBox) ] ]
                    ]
                ]
            ]
        ]);
    if (Dialog == EDialog::Amount)
    {
        auto Editor = MakeButton(FString::Printf(TEXT("Amount: %d%s"), Amount,
            bEditingAmount ? TEXT("  (editing)") : TEXT("  - activate to edit")),
            [this]() { DialogSelection = -1; bEditingAmount = true; BuildDialog(); },
            TAttribute<FSlateColor>::CreateLambda([this]() { return DialogSelection < 0 ? MenuGold : Selected; }));
        Editor->SetOnFocusReceived(FSimpleDelegate::CreateLambda([this]()
            { if (!bSynchronizingFocus) DialogSelection = -1; }));
        AmountControl = Editor;
        Choices->AddSlot().AutoHeight().Padding(0, 6)[Editor];
    }
    for (int32 Index = 0; Index < Labels.Num(); ++Index)
    {
        auto Button = MakeButton(Labels[Index], [this, Index]() { DialogAction(Index); },
            TAttribute<FSlateColor>::CreateLambda([this, Index]() { return DialogSelection == Index ? MenuGold : Selected; }));
        Button->SetOnFocusReceived(FSimpleDelegate::CreateLambda([this, Index]()
            { if (!bSynchronizingFocus) { DialogSelection = Index; bEditingAmount = false; } }));
        DialogButtons.Add(Button);
        Choices->AddSlot().AutoHeight().Padding(0, 6)[ Button ];
    }
    bFocusPending = true;
}

void SHomesteadMenu::DialogAction(int32 Index)
{
    if (!Controller.IsValid() || bSaving) return;
    if (Dialog == EDialog::Exit)
    {
        if (Index == 0) { bSaving = true; Controller->MenuSaveAndQuit(); }
        else if (Index == 1) Controller->MenuQuitWithoutSaving();
        return;
    }
    if (Dialog == EDialog::Context || Dialog == EDialog::Quantity)
    {
        if (!PopupOptions.IsValidIndex(Index) || (PopupOptions[Index].Enabled && !PopupOptions[Index].Enabled())) return;
        const TFunction<void()> Run = PopupOptions[Index].Run;
        SetDialog(EDialog::None);
        {
            TGuardValue<bool> KeepAnchor(bKeepPopupAnchor, true);
            if (Run) Run();
        }
        if (Dialog == EDialog::None) Refresh();
        return;
    }
    if (Index == 0)
    {
        SetDialog(EDialog::None);
        return;
    }
    if (Dialog == EDialog::Amount)
    {
        if (Index == 2) { Amount = 1; BuildDialog(); return; }
        if (Index == 3) { Amount = MaximumAmount; BuildDialog(); return; }
        Controller->MenuItemAction(PendingRow, PendingAction, Amount, PendingRevision);
        SetDialog(EDialog::None); Refresh(); return;
    }
    if (Dialog == EDialog::Merge)
    {
        if (MergeTargets.IsValidIndex(Index - 1))
            Controller->MenuItemAction(PendingRow, PendingAction, MergeTargets[Index - 1], PendingRevision);
        SetDialog(EDialog::None); Refresh(); return;
    }
    if (Dialog == EDialog::DropWearable)
    {
        Controller->MenuItemAction(PendingRow, PendingAction, 1, PendingRevision);
        SetDialog(EDialog::None); Refresh(); return;
    }
    if (Dialog == EDialog::RenameChest)
    {
        const FString Name = Index == 1 ? RenameDraft : Index == 2 ? FString()
            : FString(MenuChestNames::Suggestions[FMath::Clamp(Index - 3, 0, static_cast<int32>(UE_ARRAY_COUNT(MenuChestNames::Suggestions)) - 1)]);
        // A refused name (too long, unchanged) keeps the dialog open with the reason in the notice.
        if (Controller->MenuRenameChest(Name)) { SetDialog(EDialog::None); Refresh(); }
        return;
    }
    if (Dialog == EDialog::SaveFailed && Index == 2) { Controller->MenuQuitWithoutSaving(); return; }
    if (Dialog == EDialog::GraphicsFailed && Index == 2) { Controller->MenuQuitWithoutSaving(); return; }
    if (Dialog == EDialog::Unsaved) Controller->MenuQuitWithoutSaving();
    else if (Dialog == EDialog::Restart || Dialog == EDialog::TestReset) { SetDialog(EDialog::None); Controller->MenuRestart(); }
    else if (Dialog == EDialog::SaveFailed || Dialog == EDialog::GraphicsFailed)
    {
        bSaving = true;
        Controller->MenuSaveAndQuit();
    }
}

void SHomesteadMenu::OpenRenameChest()
{
    if (!Controller.IsValid() || bSaving || SeenPage != 0 || !Controller->ActiveStorageChest().IsSet()) return;
    const FString Current = Controller->ChestDisplayName(Controller->ActiveStorageChest().GetValue());
    RenameDraft = Current == UTF8_TO_TCHAR(Homestead::Chests::DefaultName) ? FString() : Current;
    RenameTyper.Reset();
    SetDialog(EDialog::RenameChest);
    // Enter saves what she types.
    DialogSelection = 1;
}

bool SHomesteadMenu::TypeChestNameCharacter(TCHAR Character)
{
    if (Dialog != EDialog::RenameChest) return false;
    // UTF-16 units arrive one per event: a surrogate pair is appended whole or not at all, and the
    // limit counts characters (code points), like Homestead::Manor::NameLength.
    std::uint32_t Units[2] = {};
    const int32 Count = RenameTyper.Type(static_cast<std::uint32_t>(Character),
        HomesteadTextEdit::CodePoints(*RenameDraft, RenameDraft.Len()), Homestead::Chests::MaxNameLength, Units);
    if (Count == 0) return true;
    for (int32 Index = 0; Index < Count; ++Index) RenameDraft.AppendChar(static_cast<TCHAR>(Units[Index]));
    const int32 Selection = DialogSelection;
    BuildDialog();
    DialogSelection = Selection;
    return true;
}

void SHomesteadMenu::TypeChestName(const FString& Characters)
{
    for (const TCHAR Character : Characters) TypeChestNameCharacter(Character);
}

bool SHomesteadMenu::HandleRenameKey(FKey Key)
{
    // Keyboard keys belong to the name while it's being typed; the pad drives the choices as usual.
    if (Dialog != EDialog::RenameChest || Key.IsGamepadKey() || Key.IsMouseButton()) return false;
    if (Key == EKeys::Escape) { SetDialog(EDialog::None); return true; }
    if (Key == EKeys::BackSpace)
    {
        if (!RenameDraft.IsEmpty())
        {
            RenameDraft.LeftChopInline(HomesteadTextEdit::BackspaceUnits(*RenameDraft, RenameDraft.Len()));
            RenameTyper.Reset();
            const int32 Selection = DialogSelection;
            BuildDialog();
            DialogSelection = Selection;
        }
        return true;
    }
    if (Key == EKeys::Enter) { DialogAction(DialogSelection); return true; }
    if (Key == EKeys::Up || Key == EKeys::Down || Key == EKeys::Tab)
    {
        NavigateDialog({0, Key == EKeys::Up || (Key == EKeys::Tab && bShift) ? -1 : 1});
        return true;
    }
    // Letters arrive as characters (OnKeyChar); nothing else acts while she types.
    return true;
}

FReply SHomesteadMenu::OnKeyChar(const FGeometry&, const FCharacterEvent& Event)
{
    if (Dialog != EDialog::RenameChest || !Controller.IsValid() || !Controller->MenuAcceptsPhysicalInput()) return FReply::Unhandled();
    TypeChestNameCharacter(Event.GetCharacter());
    return FReply::Handled();
}
}