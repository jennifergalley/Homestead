#include "SHomesteadMenuPrivate.h"

namespace HomesteadMenus
{
TSharedRef<SWidget> SHomesteadMenu::BuildDetails()
{
    FocusTargets.RemoveAll([](const FFocusTarget& Target) { return Target.region == ERegion::Details || Target.region == ERegion::Actions; });
    TSharedPtr<SVerticalBox> DetailsContent;
    auto Result = SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 12)
        [
            FocusAnchor(SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 12, 0)
            [
                SNew(SBox).WidthOverride(48).HeightOverride(48)
                [
                    SNew(SHomesteadIcon).Kind_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        return Entries.IsValidIndex(Index) ? EntryIcon(Entries[Index]) : FName(TEXT("pack"));
                    })
                    .Tint_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        return Entries.IsValidIndex(Index) ? Entries[Index].IconTint : MenuGold;
                    })
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).WrapTextAt(DetailsColumnWidth() - 100).ColorAndOpacity(Ink)
                    .Font(FCoreStyle::GetDefaultFontStyle("Bold", 22))
                    .Text_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        return FText::FromString(Entries.IsValidIndex(Index) ? EntryName(Entries[Index]) : TEXT("Details"));
                    })
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 4, 0, 0)
                [
                    SNew(STextBlock).WrapTextAt(DetailsColumnWidth() - 100).ColorAndOpacity(Muted)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 17))
                    .Text_Lambda([this]()
                    {
                        const int32 Index = DetailIndex();
                        if (!Entries.IsValidIndex(Index)) return FText();
                        const auto& Row = Entries[Index];
                        return FText::FromString(Row.Quantity > 0 ? FString::Printf(TEXT("%s: %d"), *Row.Location, Row.Quantity) : Row.Location);
                    })
                ]
            ], ERegion::Details, 0)
        ]
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(DetailsScroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot()
            [
                SAssignNew(DetailsContent, SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 16)
                [
                    FocusAnchor(SNew(STextBlock).WrapTextAt(DetailsColumnWidth() - 56).ColorAndOpacity(Ink)
                    .Font(FCoreStyle::GetDefaultFontStyle("Regular", 18))
                    .Text_Lambda([this]() { return FText::FromString(DetailsBodyText()); }), ERegion::Details, 1)
                ]
            ]
        ];
    RequirementHints.Reset();
    if (Entries.IsValidIndex(ContentSelection)
        && Entries[ContentSelection].Subject == EHomesteadMenuSubject::Recipe
        && Entries[ContentSelection].HasRecipeState)
    {
        const auto& Assessment = Entries[ContentSelection].RecipeState;
        const auto AddRequirement = [this, &DetailsContent](FName Icon, const FString& Label,
            const FString& Status, const FString& Source, bool Met)
        {
            const int32 FocusIndex = RequirementHints.Num() + 2;
            RequirementHints.Add(Met ? FString() : Source);
            const FString FullDetail = FString::Printf(TEXT("%s: %s. %s. %s"),
                *Label, *Status, Met ? TEXT("Ready") : TEXT("Missing"),
                Met ? TEXT("Requirement met") : Source.IsEmpty()
                    ? TEXT("No other requirement") : *Source);
            auto Box = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(Met ? FLinearColor(0.075f, 0.16f, 0.10f, 0.75f)
                    : FLinearColor(0.24f, 0.075f, 0.055f, 0.8f))
                .Padding(FMargin(8, 5)).ToolTipText(FText::FromString(FullDetail))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 9, 0)
                        [ SNew(SBox).WidthOverride(28).HeightOverride(28)
                            [ SNew(SHomesteadIcon).Kind(Icon).Tint(Met ? MenuGold : Ink) ] ]
                        + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(Label)).ColorAndOpacity(Ink)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)) ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(5, 0)
                        [ SNew(STextBlock).Text(FText::FromString(Status)).ColorAndOpacity(Ink)
                            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16)) ]
                        + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [ SNew(STextBlock).Text(FText::FromString(Met ? TEXT("[+]") : TEXT("[-]")))
                            .ColorAndOpacity(Met ? MenuGold : FLinearColor(1.0f, 0.66f, 0.52f))
                            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 16)) ]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [
                        SNew(STextBlock).Text(FText::FromString(Source)).ColorAndOpacity(Muted)
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 14))
                        .Visibility_Lambda([this, FocusIndex, Met, Source]()
                        {
                            return !Met && !Source.IsEmpty() && Region == ERegion::Details
                                && DetailsSelection == FocusIndex
                                ? EVisibility::Visible : EVisibility::Collapsed;
                        })
                    ]
                ];
            DetailsContent->AddSlot().AutoHeight().Padding(0, 0, 0, 5)
            [FocusAnchor(Box, ERegion::Details, FocusIndex)];
        };
        for (const auto& Ingredient : Assessment.ingredients)
        {
            AddRequirement(RequirementIcon(Ingredient.item),
                UTF8_TO_TCHAR(Homestead::ItemName(Ingredient.item)),
                FString::Printf(TEXT("%d / %d"), Ingredient.have, Ingredient.need),
                UTF8_TO_TCHAR(Ingredient.source), Ingredient.met);
        }
        if (Assessment.retainedTool != Homestead::Item::Count)
        {
            const int32 Have = Controller.IsValid()
                ? Controller->Simulation().Count(Assessment.retainedTool) : 0;
            AddRequirement(RequirementIcon(Assessment.retainedTool),
                FString::Printf(TEXT("%s (kept)"),
                    UTF8_TO_TCHAR(Homestead::ItemName(Assessment.retainedTool))),
                FString::Printf(TEXT("%d / 1"), Have), TEXT("Required tool; not consumed"),
                Assessment.retainedToolMet);
        }
        if (Assessment.stationRequired)
            AddRequirement(FName(TEXT("fire")), TEXT("Cookfire"),
                Assessment.stationMet ? TEXT("Ready") : TEXT("Missing"),
                TEXT("Fueled cookfire nearby"), Assessment.stationMet);
        if (!Assessment.capacityMet)
            AddRequirement(FName(TEXT("pack")), TEXT("Pack space"), TEXT("Full"),
                TEXT("Make room for the crafted output"), false);
    }
    ComputeActions();
    if (Actions.IsEmpty() && Region == ERegion::Actions) Region = ERegion::Details;
    TSharedPtr<SUniformGridPanel> ActionGrid;
    DetailsContent->AddSlot().AutoHeight().Padding(0, 4, 0, 0)
    [ SAssignNew(ActionGrid, SUniformGridPanel).SlotPadding(FMargin(3)) ];
    for (int32 Index = 0; Index < Actions.Num(); ++Index)
    {
        const auto Action = Actions[Index];
        auto ActionButton = RegisterButton(SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(8)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? MenuGold : Selected; })
            .OnClicked_Lambda([this, Action]() { if (PointerAction()) RunAction(Action); return FReply::Handled(); })
            [
                SNew(STextBlock).WrapTextAt(120)
                .ColorAndOpacity_Lambda([this, Index]() { return Region == ERegion::Actions && ActionSelection == Index ? FSlateColor(PineInk) : FSlateColor(Ink); })
                .Font(FCoreStyle::GetDefaultFontStyle("Regular", 16))
                .Text_Lambda([this, Action]()
                {
                    return FText::FromString(ActionLabel(Action));
                })
            ], ERegion::Actions, Index);
        TSharedRef<SWidget> Button = SNew(SBox).WidthOverride(136).MinDesiredHeight(44)[ActionButton];
        ActionButtons.Add(Button);
        ActionGrid->AddSlot(Index % 2, Index / 2)[ Button ];
    }
    return Result;
}

void SHomesteadMenu::ComputeActions()
{
    Actions.Reset();
    ActionButtons.Reset();
    // Pack and chest items act through the pointer and the item menu instead of a details pane.
    if (Entries.IsValidIndex(ContentSelection) && SeenPage != 0)
    {
        const auto& Row = Entries[ContentSelection];
        if (Row.Subject != EHomesteadMenuSubject::Recipe && Row.Subject != EHomesteadMenuSubject::ItemGroup
            && Row.Subject != EHomesteadMenuSubject::Wearable && SeenPage != 3
            && SeenPage != 5 && !IsDirectCameraSetting(Row))
            Actions.Add(EHomesteadItemAction::Primary);
    }
    ActionSelection = FMath::Clamp(ActionSelection, 0, FMath::Max(0, Actions.Num() - 1));
}

FString SHomesteadMenu::EntryName(const FHomesteadRow& Row) const
{
    if (!Row.Name.IsEmpty()) return Row.Name;
    return SeenPage == 0 && Row.Id >= 0 && Row.Id < Homestead::ItemCount
        ? FString(UTF8_TO_TCHAR(Homestead::ItemName(static_cast<Homestead::Item>(Row.Id)))) : Row.Label;
}

FName SHomesteadMenu::EntryIcon(const FHomesteadRow& Row) const
{
    if (!Row.Icon.IsNone()) return Row.Icon;
    if (SeenPage == 0 && Row.Id >= 0 && Row.Id < Homestead::ItemCount)
        return FName(UTF8_TO_TCHAR(Homestead::ItemIcon(static_cast<Homestead::Item>(Row.Id))));
    if (SeenPage == 1 && Row.Id >= 0 && Row.Id < UE_ARRAY_COUNT(RecipeIcons)) return FName(RecipeIcons[Row.Id]);
    if (SeenPage == 2 && Row.Id >= 0 && Row.Id < UE_ARRAY_COUNT(PieceIcons)) return FName(PieceIcons[Row.Id]);
    return FName(TabIcons[FMath::Clamp(SeenPage, 0, 7)]);
}

bool SHomesteadMenu::IsDirectCameraSetting(const FHomesteadRow& Row) const
{
    return SeenPage == 4 && Row.Subject == EHomesteadMenuSubject::Legacy
        && (Row.Id == 3 || Row.Id == 4);
}

int32 SHomesteadMenu::DetailIndex() const { return Hover != INDEX_NONE ? Hover : ContentSelection; }

FString SHomesteadMenu::GetFocusedRequirementHint() const
{
    const int32 Index = DetailsSelection - 2;
    return Region == ERegion::Details && RequirementHints.IsValidIndex(Index)
        ? RequirementHints[Index] : FString();
}

FString SHomesteadMenu::DetailsText() const
{
    const int32 Index = DetailIndex();
    if (!Entries.IsValidIndex(Index)) return DetailsBodyText();
    const auto& Row = Entries[Index];
    if (Row.Subject == EHomesteadMenuSubject::Recipe && Row.HasRecipeState)
    {
        FString Result = Row.Label + TEXT("\n") + Row.Location + TEXT("\n") + Row.Detail;
        for (const auto& Ingredient : Row.RecipeState.ingredients)
            Result += FString::Printf(TEXT("\n%s: Have %d / Need %d (%s)"),
                UTF8_TO_TCHAR(Homestead::ItemName(Ingredient.item)), Ingredient.have,
                Ingredient.need, UTF8_TO_TCHAR(Ingredient.source));
        if (Row.RecipeState.retainedTool != Homestead::Item::Count)
            Result += FString::Printf(TEXT("\n%s (kept): %s"),
                UTF8_TO_TCHAR(Homestead::ItemName(Row.RecipeState.retainedTool)),
                Row.RecipeState.retainedToolMet ? TEXT("Ready") : TEXT("Missing"));
        if (Row.RecipeState.stationRequired)
            Result += FString::Printf(TEXT("\nFueled cookfire nearby: %s"),
                Row.RecipeState.stationMet ? TEXT("Ready") : TEXT("Not nearby"));
        Result += FString::Printf(TEXT("\nPack space: %s"),
            Row.RecipeState.capacityMet ? TEXT("Available") : TEXT("Full"));
        return Result;
    }
    return Row.Label + TEXT("\n") + Row.Location + TEXT("\n\n") + DetailsBodyText();
}

FString SHomesteadMenu::DetailsBodyText() const
{
    const int32 Index = DetailIndex();
    if (!Entries.IsValidIndex(Index)) return TEXT("Select an item to see its details.\n\nNothing here is a recipe output you already own.");
    const auto& Row = Entries[Index];
    if (Row.Subject == EHomesteadMenuSubject::Recipe) return FString();
    FString Detail = Row.Detail;
    if (SeenPage == 4 && Controller.IsValid() && Controller->IsFailed())
        Detail += TEXT("\n\nRecovery: saving a failed state is disabled. Retry a checkpoint or quit explicitly.");
    return Detail;
}

FString SHomesteadMenu::Footer() const
{
    if (!Controller.IsValid()) return {};
    if (bRecovery) return Controller->UsesGamepad() ? TEXT("D-pad / Left stick  Select    A  Activate    Y  Settings / Quit")
        : TEXT("Arrows  Select    Enter  Activate    G  Settings / Quit");
    const bool Pad = Controller->UsesGamepad();
    FString Hint = Pad ? TEXT("D-pad / Left stick  Move between sections    A  Activate    LB/RB  Tabs")
        : TEXT("Arrows  Move between sections    Enter  Activate    Ctrl+Tab  Tabs");
    if (Region == ERegion::Portrait) Hint += Pad ? TEXT("    Right stick  Turn    R3  Zoom") : TEXT("    Z  Zoom");
    if (Region == ERegion::Details) Hint += TEXT("    Up/Down  Scroll details");
    return Hint + (Pad ? TEXT("    B  Back") : TEXT("    Esc  Back"));
}

FString SHomesteadMenu::RowKey(const FHomesteadRow& Row) const
{
    return FString::Printf(TEXT("%d:%d:%d"), static_cast<int>(Row.Subject), Row.ContainerId,
        Row.Subject == EHomesteadMenuSubject::Legacy ? Row.Id : Row.SubjectId);
}

FString SHomesteadMenu::ActionLabel(EHomesteadItemAction Action) const
{
    if (!Entries.IsValidIndex(ContentSelection)) return TEXT("No item selected");
    const auto& Row = Entries[ContentSelection];
    switch (Action)
    {
    case EHomesteadItemAction::Transfer: return Row.ContainerId > 0 ? TEXT("Take to pack...")
        : Row.DestinationId < 0 ? TEXT("Store (no nearby chest)") : FString::Printf(TEXT("Store in chest %d..."), Row.DestinationId);
    case EHomesteadItemAction::Split: return TEXT("Split stack...");
    case EHomesteadItemAction::Merge: return TEXT("Merge with stack...");
    case EHomesteadItemAction::MoveEarlier: return TEXT("Move earlier in grid");
    case EHomesteadItemAction::MoveLater: return TEXT("Move later in grid");
    case EHomesteadItemAction::Equip: return TEXT("Equip");
    case EHomesteadItemAction::Unequip: return TEXT("Unequip to pack");
    case EHomesteadItemAction::Dye: return TEXT("Change dye...");
    case EHomesteadItemAction::Drop: return TEXT("Drop...");
    case EHomesteadItemAction::Pin:
        return Row.HotbarCell >= 0 ? TEXT("Move into the pack") : TEXT("Move to the hotbar");
    default: return Row.Action.IsEmpty() ? TEXT("Change / activate") : Row.Action;
    }
}

TSharedPtr<SWidget> SHomesteadMenu::GetAudioSliderWidget(int32 AudioId) const
{
    return SeenPage == 4 && IsAudioSetting(AudioId) && AudioSliders.IsValidIndex(AudioSliderSlot(AudioId))
        ? StaticCastSharedPtr<SWidget>(AudioSliders[AudioSliderSlot(AudioId)]) : nullptr;
}

FString SHomesteadMenu::EquipmentLabel(int32 Index) const
{
    if (!Controller.IsValid() || Index < 0 || Index >= VisibleEquipmentSlotCount) return {};
    const int32 Id = Controller->State().equipment[static_cast<int32>(VisibleEquipmentSlots[Index])];
    const auto* Item = Controller->Simulation().GetWearable(Id);
    return FString(EquipmentSlotNames[Index]) + TEXT("\n") + (Item
        ? FString(UTF8_TO_TCHAR(Homestead::WearableName(Item->definition))) : TEXT("Empty"));
}

FString SHomesteadMenu::PackHint() const
{
    if (!Controller.IsValid()) return {};
    const bool bHotbarLine = Region == ERegion::Hotbar || HeldHotbarRow.IsSet() || HeldHotbarSlot != INDEX_NONE;
    if (bHotbarLine && !HotbarCells.IsEmpty()) return HotbarHint();
    const int32 Index = DetailIndex();
    FString Subject;
    if (Entries.IsValidIndex(Index) && Entries[Index].Subject != EHomesteadMenuSubject::Legacy)
    {
        const auto& Row = Entries[Index];
        Subject = EntryName(Row) + (Row.Quantity > 1 ? FString::Printf(TEXT(" x%d"), Row.Quantity) : FString())
            + TEXT("  (") + Row.Location + TEXT(")") + (Row.Status.IsEmpty() ? FString() : TEXT("   ") + Row.Status)
            + TEXT("\n");
    }
    const bool Chest = Controller->ActiveStorageChest().IsSet();
    if (Controller->UsesGamepad())
        return Subject + TEXT("A  pick up / place     Y  item actions     X  split in half     LB / RB  pages");
    return Subject + (Chest
        ? TEXT("Shift+click  move across     Right-click  actions     Ctrl+click  amount     Drag  move")
        : TEXT("Right-click  actions     Shift+click  hotbar / wear     Ctrl+click  amount     Drag  rearrange"));
}
}
