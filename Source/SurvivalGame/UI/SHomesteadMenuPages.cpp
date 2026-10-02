#include "SHomesteadMenuPrivate.h"
#include "HomesteadUITheme.h"

namespace HomesteadMenus
{
// Settings (Jenny 2026-09-30 21:59: "too large, compact it"): one size step below the book's pages, with
// tight padding, so a tab's rows fit on screen at 1080p. Details stay at 13, the book's smallest size,
// so they read at 720p.
namespace MenuSettingsStyle
{
constexpr float ResumeSize = 15.0f;
constexpr float SessionSize = 14.0f;   // Save / Load / Quit
constexpr float TabSize = 16.0f;       // Game / Sound / Video
constexpr float TitleSize = 15.0f;     // "Game speed: Balanced"
constexpr float DetailSize = 13.0f;
constexpr float OptionSize = 13.0f;    // Leisurely / Balanced / Fast
constexpr float SliderHeight = 26.0f;
constexpr float RowGap = 4.0f;
const FMargin RowPadding(12, 7);
const FMargin OptionPadding(8, 3);
const FMargin TabPadding(8, 5);
const FMargin SessionPadding(4, 6);
const FMargin ResumePadding(14, 6);
}

int32 SHomesteadMenu::StorageColumns() const { return LogicalBookWidth() >= 1800 ? 8 : 6; }
// Beside a chest the pack grid is as wide as the hotbar row heading it.
int32 SHomesteadMenu::StoragePackColumns() const { return Homestead::PackRowSize; }

int32 SHomesteadMenu::Columns() const
{
    // The Pack page's grid is as wide as the hotbar row heading it (Homestead::PackRowSize), so the row
    // reads as the grid's first row.
    const bool Expanded = LogicalBookWidth() >= 1800;
    return SeenPage == 0 ? Homestead::PackRowSize
        : SeenPage <= 2 ? (Expanded ? 10 : 6) : 1;
}

TSharedRef<SWidget> SHomesteadMenu::BuildBody()
{
    if (bRecovery)
    {
        return SNew(SVerticalBox)
            + SVerticalBox::Slot().AutoHeight().Padding(24)[ Text(TEXT("Time to try again"), 32) ]
            + SVerticalBox::Slot().FillHeight(1).Padding(24)
            [ Text(TEXT("You ran out of food."), 23) ]
            + SVerticalBox::Slot().AutoHeight().Padding(24, 8)
            [ RegisterButton(MakeButton(TEXT("Return to latest save  [A / Enter]"), [this]() { Controller->MenuRetry(); }), ERegion::Recovery, 0) ]
            + SVerticalBox::Slot().AutoHeight().Padding(24, 8)
            [ RegisterButton(MakeButton(TEXT("Settings / Quit  [Y / G]"), [this]() { ChangePage(4); }), ERegion::Recovery, 1) ];
    }
    TSharedPtr<SVerticalBox> Body;
    TSharedPtr<SUniformGridPanel> Grid;
    TSharedPtr<SVerticalBox> AppearanceList;
    TSharedPtr<SUniformGridPanel> ChestGrid;
    TSharedPtr<SUniformGridPanel> PackGrid;
    auto Result = SNew(SVerticalBox)
        + SVerticalBox::Slot().FillHeight(1)
        [
            SAssignNew(Body, SVerticalBox)
        ];
    const auto BuildSettings = [this]() -> TSharedRef<SWidget>
    {
        AudioSliders.Init(nullptr, 4);
        TSharedPtr<SVerticalBox> RowsBox;
        TSharedPtr<SHorizontalBox> TopRow;
        TSharedPtr<SHorizontalBox> TabStrip;
        const int32 TopCount = SettingsTopCount();
        // Settings sits in a centred column a little under a third of the screen wide.
        auto Result = SNew(SBox).HAlign(HAlign_Center)
        [
            SNew(SBox).WidthOverride_Lambda([]()
                { return FOptionalSize(FMath::Clamp(LogicalBookWidth() * 0.30f, 440.0f, 720.0f)); })
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)
                [
                    RegisterButton(MakeButton(Controller->IsFailed() ? TEXT("Return to recovery") : TEXT("Resume"),
                        [this]() { Back(); }, TAttribute<FSlateColor>::CreateLambda([this]()
                            { return Region == ERegion::Session && SessionSelection == 0 ? MenuGold : MenuPine; }),
                        FString(), MenuSettingsStyle::ResumePadding, MenuSettingsStyle::ResumeSize),
                        ERegion::Session, 0)
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 8)[ SAssignNew(TopRow, SHorizontalBox) ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 6)[ SAssignNew(TabStrip, SHorizontalBox) ]
                + SVerticalBox::Slot().FillHeight(1)
                [
                    SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
                    + SScrollBox::Slot()
                    [
                        SAssignNew(RowsBox, SVerticalBox)
                    ]
                ]
            ]
        ];
        static const TCHAR* TabNames[] = {TEXT("Game"), TEXT("Sound"), TEXT("Video")};
        for (int32 Tab = 0; Tab < 3; ++Tab)
        {
            const int32 Session = Tab + 1;
            TabStrip->AddSlot().FillWidth(1).Padding(Tab ? 4 : 0, 0, 0, 0)
            [
                FocusAnchor(
                    SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false).ContentPadding(MenuSettingsStyle::TabPadding)
                    .HAlign(HAlign_Center)
                    .ButtonColorAndOpacity_Lambda([this, Tab, Session]()
                    {
                        return SettingsTab == Tab ? MenuGold
                            : Region == ERegion::Session && SessionSelection == Session ? Selected : MenuPine;
                    })
                    .OnClicked_Lambda([this, Tab, Session]()
                    {
                        if (PointerAction()) { Region = ERegion::Session; SessionSelection = Session; SetSettingsTab(Tab); }
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock).Text(FText::FromString(TabNames[Tab]))
                        .Font(HomesteadUITheme::Font("Bold", MenuSettingsStyle::TabSize))
                        .ColorAndOpacity_Lambda([this, Tab]() { return SettingsTab == Tab ? FSlateColor(PineInk) : FSlateColor(Ink); })
                    ],
                    ERegion::Session, Session)
            ];
        }

        const auto OptionButton = [this](const FString& Label, bool SelectedOption, TFunction<void()> Action)
        {
            return SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false).ContentPadding(MenuSettingsStyle::OptionPadding)
                .ButtonColorAndOpacity(SelectedOption ? MenuGold : Selected)
                .OnClicked_Lambda([this, Action]() { if (PointerAction()) Action(); return FReply::Handled(); })
                [
                    SNew(STextBlock).Text(FText::FromString(Label))
                    .Font(HomesteadUITheme::Font("Regular", MenuSettingsStyle::OptionSize))
                    .ColorAndOpacity(SelectedOption ? FSlateColor(PineInk) : FSlateColor(Ink))
                ];
        };

        for (int32 Index = 0; Index < TopCount; ++Index)
        {
            const FHomesteadRow& Row = Entries[Index];
            const TSharedRef<SWidget> Cell = FocusAnchor(
                SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false).ContentPadding(MenuSettingsStyle::SessionPadding)
                .HAlign(HAlign_Center)
                .ToolTipText(FText::FromString(Row.Detail))
                .ButtonColorAndOpacity_Lambda([this, Index]() { return ContentSelection == Index && Region == ERegion::Content ? Selected : MenuPine; })
                .OnClicked_Lambda([this, Index]()
                {
                    if (PointerAction()) { Region = ERegion::Content; Select(Index); RunAction(EHomesteadItemAction::Primary); }
                    return FReply::Handled();
                })
                [
                    SNew(STextBlock).Text(FText::FromString(Row.Label)).Justification(ETextJustify::Center)
                    .ColorAndOpacity(Ink).Font(HomesteadUITheme::Font("Bold", MenuSettingsStyle::SessionSize))
                ],
                ERegion::Content, Index);
            Cells.Add(Cell);
            TopRow->AddSlot().FillWidth(Row.Label.Len() > 10 ? 1.4f : 1.0f).Padding(Index ? 4 : 0, 0, 0, 0)[Cell];
        }

        for (int32 Index = TopCount; Index < Entries.Num(); ++Index)
        {
            const FHomesteadRow& Row = Entries[Index];
            TAttribute<FText> RowLabel = FText::FromString(Row.Label);
            if (IsAudioSetting(Row.Id))
            {
                const int32 AudioId = Row.Id;
                const FString Prefix = Row.Id == 16 ? TEXT("Overall volume") : Row.Id == 5 ? TEXT("Music volume")
                    : Row.Id == 6 ? TEXT("Ambience volume") : TEXT("Effects volume");
                RowLabel = TAttribute<FText>::CreateLambda([this, AudioId, Prefix]()
                {
                    return FText::FromString(FString::Printf(TEXT("%s: %d%%"), *Prefix,
                        FMath::RoundToInt(Controller->MenuAudioVolume(AudioId) * 100)));
                });
            }
            TSharedPtr<SVerticalBox> RowContent;
            auto Content = SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(RowLabel)
                    .ColorAndOpacity(Row.Id == 13 && !Controller->IsAutosaveEnabled() ? Muted : Ink)
                    .Font(HomesteadUITheme::Font("Bold", MenuSettingsStyle::TitleSize))
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 1, 0, 0)
                [
                    // A label and its value; a short line only where a setting isn't obvious.
                    SNew(STextBlock).Text(FText::FromString(Row.Detail)).ColorAndOpacity(Muted)
                    .Visibility(Row.Detail.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible)
                    .Font(HomesteadUITheme::Font("Regular", MenuSettingsStyle::DetailSize)).AutoWrapText(true)
                ];
            RowContent = Content;

            if (Row.Id == 2)
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 5, 0, 0)[SAssignNew(Choices, SHorizontalBox)];
                const double Current = Controller->State().dayMinutes;
                const TPair<const TCHAR*, double> Options[] = {
                    {TEXT("Leisurely"), 120}, {TEXT("Balanced"), 60}, {TEXT("Fast"), 30}};
                for (const auto& Option : Options)
                    Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                    [OptionButton(Option.Key, FMath::IsNearlyEqual(Current, Option.Value),
                        [this, Value = Option.Value]() { Controller->MenuSetGameSpeed(Value); Refresh(); })];
            }
            else if (IsAudioSetting(Row.Id))
            {
                const int32 AudioId = Row.Id;
                RowContent->AddSlot().AutoHeight().Padding(0, 6, 0, 0)
                [
                    SNew(SBox).HeightOverride(MenuSettingsStyle::SliderHeight)
                    [
                        SAssignNew(AudioSliders[AudioSliderSlot(AudioId)], SSlider)
                        .Style(&MenuSliderStyle())
                        .SliderBarColor(FLinearColor::White)
                        .SliderHandleColor(FLinearColor::White)
                        .Value_Lambda([this, AudioId]() { return Controller->MenuAudioVolume(AudioId); })
                        .OnMouseCaptureBegin_Lambda([this, AudioId, Index]()
                        {
                            CommitAudioStep();
                            AudioEditId = AudioId;
                            AudioEditStart = Controller->MenuAudioVolume(AudioId);
                            Region = ERegion::Content;
                            Select(Index);
                        })
                        .OnValueChanged_Lambda([this, AudioId](float Value)
                        {
                            Controller->MenuPreviewAudioVolume(AudioId, Value);
                        })
                        .OnMouseCaptureEnd_Lambda([this, AudioId]()
                        {
                            const float Current = Controller->MenuAudioVolume(AudioId);
                            Controller->MenuCommitAudioVolume(AudioId, Current, AudioEditStart);
                            AudioEditId = -1;
                            Refresh();
                        })
                    ]
                ];
            }
            else if (Row.Id == 12)
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 5, 0, 0)[SAssignNew(Choices, SHorizontalBox)];
                Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                [OptionButton(TEXT("On"), Controller->IsAutosaveEnabled(),
                    [this]() { Controller->MenuSetAutosaveEnabled(true); Refresh(); })];
                Choices->AddSlot().AutoWidth()
                [OptionButton(TEXT("Off"), !Controller->IsAutosaveEnabled(),
                    [this]() { Controller->MenuSetAutosaveEnabled(false); Refresh(); })];
            }
            else if (Row.Id == 17 && Controller->MapPresenter())
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 5, 0, 0)[SAssignNew(Choices, SHorizontalBox)];
                Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                [OptionButton(TEXT("North up"), !Controller->MapPresenter()->RotatesWithCamera(),
                    [this]() { Controller->MapPresenter()->SetRotatesWithCamera(false); Refresh(); })];
                Choices->AddSlot().AutoWidth()
                [OptionButton(TEXT("Turns with view"), Controller->MapPresenter()->RotatesWithCamera(),
                    [this]() { Controller->MapPresenter()->SetRotatesWithCamera(true); Refresh(); })];
            }
            else if (Row.Id == 13)
            {
                TSharedPtr<SHorizontalBox> Choices;
                RowContent->AddSlot().AutoHeight().Padding(0, 5, 0, 0)
                [
                    SAssignNew(Choices, SHorizontalBox)
                    .IsEnabled(Controller->IsAutosaveEnabled())
                ];
                for (const int32 Minutes : {5, 10, 20, 30})
                    Choices->AddSlot().AutoWidth().Padding(0, 0, 6, 0)
                    [OptionButton(FString::Printf(TEXT("%d min"), Minutes), Controller->AutosaveIntervalMinutes() == Minutes,
                        [this, Minutes]() { Controller->MenuSetAutosaveInterval(Minutes); Refresh(); })];
            }

            TSharedRef<SWidget> RowWidget = Row.Id == 14
                ? StaticCastSharedRef<SWidget>(SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(MenuPine).Padding(MenuSettingsStyle::RowPadding)[RowContent.ToSharedRef()])
                : FocusAnchor(
                    SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false).ContentPadding(MenuSettingsStyle::RowPadding)
                    .ButtonColorAndOpacity_Lambda([this, Index]() { return ContentSelection == Index ? Selected : MenuPine; })
                    .OnClicked_Lambda([this, Index, Id = Row.Id]()
                    {
                        if (PointerAction())
                        {
                            Region = ERegion::Content;
                            Select(Index);
                            if (Id != 2 && !IsAudioSetting(Id) && Id != 12 && Id != 13)
                                RunAction(EHomesteadItemAction::Primary);
                        }
                        return FReply::Handled();
                    })[RowContent.ToSharedRef()],
                    ERegion::Content, Index);
            Cells.Add(RowWidget);
            RowsBox->AddSlot().AutoHeight().Padding(0, 0, 0, MenuSettingsStyle::RowGap)[RowWidget];
        }
        return Result;
    };
    if (SeenPage == 4) return BuildSettings();
    if (SeenPage == 7) return BuildMap();
    const bool Storage = SeenPage == 0 && Controller->ActiveStorageChest().IsSet();
    const bool PackOnly = SeenPage == 0 && !Storage;
    TSharedPtr<SHorizontalBox> ColumnsBox;
    // Every page spans the tab row's full width, so the tabs sit square above it (Jenny, 2026-09-30).
    Body->AddSlot().FillHeight(1).HAlign(HAlign_Fill)[ SAssignNew(ColumnsBox, SHorizontalBox) ];
    if (SeenPage == 0 && !Storage && Controller->MenuPortraitBrush())
    {
        ColumnsBox->AddSlot().AutoWidth().Padding(0, 0, 12, 0)
        [
            SNew(SBox).WidthOverride(PortraitColumnWidth()).Clipping(EWidgetClipping::ClipToBounds)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Portrait ? MenuGold : MenuPine; })
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().FillHeight(1)
                    [
                        FocusAnchor(SNew(SScaleBox).Stretch(EStretch::ScaleToFit).HAlign(HAlign_Center).VAlign(VAlign_Center)
                        [ SNew(SImage).Image_Lambda([this]() { return Controller.IsValid() ? Controller->MenuPortraitBrush() : nullptr; }) ], ERegion::Portrait, -1)
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(8)
                    [
                        SNew(STextBlock).AutoWrapText(true).ColorAndOpacity(Ink)
                        .Font(HomesteadUITheme::Font("Regular", 16))
                        .Text_Lambda([this]() { return FText::FromString(Controller->MenuPortraitStatus()); })
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(8, 0, 8, 8)
                    [
                        SNew(SBox).HeightOverride(44)
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
                            [ RegisterButton(MakeButton(TEXT("<"), [this]() { Controller->OrbitMenuPortrait(-20); },
                                TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Portrait && PortraitSelection == 0 ? MenuGold : MenuPine; }),
                                TEXT("Turn character left"), FMargin(8)), ERegion::Portrait, 0) ]
                            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 4, 0)
                            [ RegisterButton(MakeButton(TEXT(">"), [this]() { Controller->OrbitMenuPortrait(20); },
                                TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Portrait && PortraitSelection == 1 ? MenuGold : MenuPine; }),
                                TEXT("Turn character right"), FMargin(8)), ERegion::Portrait, 1) ]
                            + SHorizontalBox::Slot().FillWidth(1)
                            [ RegisterButton(MakeButton(TEXT("Zoom"), [this]() { Controller->ZoomMenuPortrait(); },
                                TAttribute<FSlateColor>::CreateLambda([this]() { return Region == ERegion::Portrait && PortraitSelection == 2 ? MenuGold : MenuPine; }),
                                TEXT("Toggle close-up and full-body view"), FMargin(8)), ERegion::Portrait, 2) ]
                        ]
                    ]
                ]
            ]
        ];
    }
    TSharedPtr<SVerticalBox> InventoryColumn;
    const auto InventoryPanel = SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(MenuPine).Padding(12)
        .Clipping(EWidgetClipping::ClipToBounds)
        [
            SAssignNew(InventoryColumn, SVerticalBox)
        ];
    if (PackOnly) ColumnsBox->AddSlot().FillWidth(1)[ InventoryPanel ];
    // Appearance is a narrow column at the left; she stands in the world to its right.
    else if (SeenPage == 6) ColumnsBox->AddSlot().AutoWidth()[ SNew(SBox).WidthOverride(AppearancePanelWidth)[ InventoryPanel ] ];
    else ColumnsBox->AddSlot().FillWidth(1).Padding(0, 0, SeenPage == 0 ? 0 : 16, 0)[ InventoryPanel ];
    if (SeenPage == 0 && !Storage)
    {
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 0, 4, 6)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
            [
                SNew(SBox).WidthOverride(42).HeightOverride(42)
                [
                    SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                    .ContentPadding(5).ButtonColorAndOpacity(Selected)
                    .ToolTipText(FText::FromString(TEXT("Sort pack")))
                    .OnClicked_Lambda([this]()
                    {
                        if (PointerAction() && Controller->MenuSortPack()) Refresh();
                        return FReply::Handled();
                    })
                    [ SNew(SHomesteadIcon).Kind(FName(TEXT("sort"))).Tint(MenuGold) ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [ Text(Controller->BookTitle(), 26) ]
        ];
    }
    else
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 0, 4, 6)
        [ Text(SeenPage <= 2 ? Controller->BookTitle() : Tabs[SeenPage], 26) ];
    const FString Summary = SeenPage == 0 ? Controller->MenuInventorySummary() : Controller->BookSummary();
    if (!Summary.IsEmpty())
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 0, 4, 12)[ Text(Summary, 17) ];
    PackDropArea.Reset();
    ChestDropArea.Reset();
    if (Storage)
    {
        TSharedPtr<SVerticalBox> ChestColumn;
        TSharedPtr<SVerticalBox> PackColumn;
        // The headings stay put above the scrolling grids; the hotbar, her pack's first row
        // (Simulation/HomesteadPackRow.h), heads the pack column inside them (below).
        InventoryColumn->AddSlot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
            [
                // The chest's own name, with naming and auto-store beside it.
                SNew(SBox).Padding(4, 0, 4, 6)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                    [
                        SNew(STextBlock).Font(HomesteadUITheme::Font("Regular", 20)).ColorAndOpacity(Ink)
                        .Text_Lambda([this]()
                        {
                            return FText::FromString(Controller.IsValid() && Controller->ActiveStorageChest().IsSet()
                                ? Controller->ChestDisplayName(Controller->ActiveStorageChest().GetValue()) : FString());
                        })
                    ]
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 6, 0)
                    [
                        SNew(SBox).HeightOverride(34)
                        [
                            SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                            .ContentPadding(FMargin(10, 4)).ButtonColorAndOpacity(Selected)
                            .ToolTipText(FText::FromString(TEXT("Name this chest")))
                            .OnClicked_Lambda([this]() { if (PointerAction()) OpenRenameChest(); return FReply::Handled(); })
                            [ SNew(STextBlock).Font(HomesteadUITheme::Font("Regular", 15)).ColorAndOpacity(MenuGold)
                                .Text(FText::FromString(TEXT("Name..."))) ]
                        ]
                    ]
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(SBox).WidthOverride(34).HeightOverride(34)
                        [
                            SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                            .ContentPadding(4).ButtonColorAndOpacity(Selected)
                            .ToolTipText(FText::FromString(TEXT("Store matching stacks (T): put what you carry onto the same items already in this chest")))
                            .OnClicked_Lambda([this]()
                            {
                                if (PointerAction() && Controller->MenuStoreMatching()) Refresh();
                                return FReply::Handled();
                            })
                            [ SNew(SHomesteadIcon).Kind(FName(TEXT("chest"))).Tint(MenuGold) ]
                        ]
                    ]
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1).Padding(8, 0, 0, 0)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 6)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                    [ Text(TEXT("Pack"), 20) ]
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        SNew(SBox).WidthOverride(34).HeightOverride(34)
                        [
                            SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                            .ContentPadding(4).ButtonColorAndOpacity(Selected)
                            .ToolTipText(FText::FromString(TEXT("Sort pack")))
                            .OnClicked_Lambda([this]()
                            {
                                if (PointerAction() && Controller->MenuSortPack()) Refresh();
                                return FReply::Handled();
                            })
                            [ SNew(SHomesteadIcon).Kind(FName(TEXT("sort"))).Tint(MenuGold) ]
                        ]
                    ]
                ]
            ]
        ];
        InventoryColumn->AddSlot().FillHeight(1)
        [
            SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot()
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1).Padding(0, 0, 8, 0)
                [
                    SAssignNew(ChestColumn, SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight()
                    [ SAssignNew(ChestGrid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
                ]
                + SHorizontalBox::Slot().FillWidth(1).Padding(8, 0, 0, 0)
                [
                    // The hotbar heads the pack column as its first row (Jenny 2026-09-30: "it should look and
                    // act like any other row"): the pack grid is as wide as it, ten cells, and the row's
                    // cells are the grid's own size; stacks drag and Shift+click between it and the chest.
                    SAssignNew(PackColumn, SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(4, 0, 4, 0)
                    [
                        SNew(STextBlock).Text(FText::FromString(TEXT("Hotbar"))).ColorAndOpacity(Muted)
                        .Font(HomesteadUITheme::Font("Regular", 13))
                    ]
                    + SVerticalBox::Slot().AutoHeight()[ BuildBookHotbar(true) ]
                    + SVerticalBox::Slot().AutoHeight().Padding(4, 2, 4, 4)
                    [
                        SNew(SBox).HeightOverride(1.5f)
                        [
                            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                            .BorderBackgroundColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.35f))
                        ]
                    ]
                    + SVerticalBox::Slot().AutoHeight()
                    [ SAssignNew(PackGrid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
                ]
            ]
        ];
        PackDropArea = PackColumn;
        ChestDropArea = ChestColumn;
    }
    else if (SeenPage == 6)
    {
        // Rows differ a lot in height (style chips vs. one swatch row), so stack them instead of a uniform grid.
        InventoryColumn->AddSlot().FillHeight(1)
        [
            SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot().HAlign(HAlign_Fill)[ SAssignNew(AppearanceList, SVerticalBox) ]
        ];
    }
    else if (SeenPage == 0)
    {
        // The hotbar is the pack's first row (Simulation/HomesteadPackRow.h): one of the grid's rows,
        // the same width and height as the rest, set apart only by a small label and a rule beneath it.
        InventoryColumn->AddSlot().FillHeight(1)
        [
            SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            + SScrollBox::Slot().HAlign(HAlign_Fill)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight().Padding(6, 0, 6, 0)
                [
                    SNew(STextBlock).Text(FText::FromString(TEXT("Hotbar"))).ColorAndOpacity(Muted)
                    .Font(HomesteadUITheme::Font("Regular", 13))
                ]
                + SVerticalBox::Slot().AutoHeight()[ BuildBookHotbar(true) ]
                + SVerticalBox::Slot().AutoHeight().Padding(6, 2, 6, 6)
                [
                    SNew(SBox).HeightOverride(1.5f)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.35f))
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight()
                [ SAssignNew(Grid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
            ]
        ];
        PackDropArea = Scroll;
    }
    else
    {
        InventoryColumn->AddSlot().FillHeight(1)
        [
            SAssignNew(Scroll, SScrollBox).Clipping(EWidgetClipping::ClipToBounds)
            // The pack's grid spreads across the full-width page; recipe and plan grids stay left.
            + SScrollBox::Slot().HAlign(SeenPage == 1 || SeenPage == 2 ? HAlign_Left : HAlign_Fill)
            [ SAssignNew(Grid, SUniformGridPanel).SlotPadding(FMargin(4)) ]
        ];
        if (SeenPage == 0) PackDropArea = Scroll;
    }

    if (SeenPage == 0 && !Storage)
    {
        TSharedPtr<SHorizontalBox> EquipmentBar;
        InventoryColumn->AddSlot().AutoHeight().Padding(0, 8, 0, 4)[ Text(TEXT("Equipped slots"), 16) ];
        InventoryColumn->AddSlot().AutoHeight()[ SAssignNew(EquipmentBar, SHorizontalBox) ];
        for (int32 Index = 0; Index < VisibleEquipmentSlotCount; ++Index)
        {
            TSharedRef<SMenuButton> SlotButton = SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(8)
                .ButtonColorAndOpacity_Lambda([this, Index]()
                    { return Region == ERegion::Equipment && EquipmentSelection == Index ? MenuGold : Selected; })
                .ToolTipText(FText::FromString(TEXT("Click or right-click for what you can wear here")))
                .OnClicked_Lambda([this, Index]()
                    { if (PointerAction() && Dialog == EDialog::None) FocusEquipment(Index, true); return FReply::Handled(); })
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                    [ SNew(SBox).WidthOverride(36).HeightOverride(36)
                        [ SNew(SHomesteadIcon).Kind(FName(EquipmentSlotIcons[Index])) ] ]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                    [ Text(EquipmentLabel(Index), 15) ]
                ];
            SlotButton->RightClick = [this, Index]()
                { if (PointerAction() && Dialog == EDialog::None) FocusEquipment(Index, true); };
            EquipmentBar->AddSlot().FillWidth(1).Padding(3, 0)
            [
                RegisterButton(SlotButton, ERegion::Equipment, Index)
            ];
        }
    }
    if (SeenPage == 0)
    {
        DetailsHost.Reset();
        DetailsScroll.Reset();
        InventoryColumn->AddSlot().AutoHeight().Padding(4, 10, 4, 0)
        [
            SNew(STextBlock).ColorAndOpacity(Muted).Font(HomesteadUITheme::Font("Regular", 14))
            .WrapTextAt(Storage ? 0.0f : Columns() * (ItemCellWidth + 12.0f))
            .AutoWrapText(Storage)
            .Text_Lambda([this]() { return FText::FromString(PackHint()); })
        ];
    }
    else if (SeenPage == 6) { DetailsHost.Reset(); DetailsScroll.Reset(); }
    else
    ColumnsBox->AddSlot().AutoWidth()
        [
            SNew(SBox).WidthOverride(DetailsColumnWidth()).Clipping(EWidgetClipping::ClipToBounds)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this]() { return Region == ERegion::Details ? MenuGold : MenuPine; })
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(MenuPine).Padding(16)
                    [ SAssignNew(DetailsHost, SBox)[ BuildDetails() ] ]
                ]
            ]
        ];
    // Empty slots draw as pale outlines on the page, so the pack and chests read as grids of room.
    auto EmptyCell = []() -> TSharedRef<SWidget>
    {
        return SNew(SBox).WidthOverride(ItemCellWidth + 4).HeightOverride(80).Padding(2)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(1.5f)
                .BorderBackgroundColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.2f))
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(HomesteadUITheme::Themed(FLinearColor(0.2f, 0.3f, 0.24f, 0.25f)))
                ]
            ];
    };
    const bool bPackGrid = SeenPage == 0 && !(Controller->InventoryView() == 2);
    if (Entries.IsEmpty() && Grid && bPackGrid)
        Grid->AddSlot(0, 0)[ FocusAnchor(EmptyCell(), ERegion::Content, -1) ];
    else if (Entries.IsEmpty() && Grid)
        Grid->AddSlot(0, 0)[ FocusAnchor(SNew(SBox).WidthOverride_Lambda([this]()
            { return FMath::Max(ItemCellWidth, Scroll->GetCachedGeometry().GetLocalSize().X > 0
                ? static_cast<float>(Scroll->GetCachedGeometry().GetLocalSize().X) - 24.0f : 480.0f); })
            [Text(SeenPage == 0 && Controller->InventoryView() == 1
            ? TEXT("This chest is empty.")
            : SeenPage == 0 && Controller->InventoryView() == 2 ? TEXT("No removable clothing is equipped.")
            : TEXT("Your pack is empty."))], ERegion::Content, -1) ];
    int32 ChestCell = 0;
    int32 PackCell = 0;
    for (int32 Index = 0; Index < Entries.Num(); ++Index)
    {
        const auto& Row = Entries[Index];
        const FString Name = EntryName(Row);
        TSharedPtr<SWidget> Cell;
        TSharedPtr<SVerticalBox> Contents;
        auto Button = RegisterButton(SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(true).ContentPadding(7)
            .ButtonColorAndOpacity_Lambda([this, Index]() { return CellColor(Index); })
            .ToolTipText(FText::FromString(Name + TEXT("\n") + Row.Detail))
            .OnHovered_Lambda([this, Index]() { if (Controller.IsValid() && !Controller->UsesGamepad()) Hover = Index; })
            .OnUnhovered_Lambda([this, Index]() { if (Hover == Index) Hover = INDEX_NONE; })
            .OnPressed_Lambda([this, Index]()
            {
                if (PointerAction() && Entries.IsValidIndex(Index)
                    && Entries[Index].Subject == EHomesteadMenuSubject::Recipe)
                {
                    StopCraftHold();
                    Region = ERegion::Content;
                    Select(Index);
                    StartCraftHold(ECraftInput::Pointer);
                }
                else if (PointerAction() && SeenPage == 0 && Entries.IsValidIndex(Index)
                    && (Entries[Index].Subject == EHomesteadMenuSubject::ItemGroup
                        || Entries[Index].Subject == EHomesteadMenuSubject::Wearable))
                {
                    CancelVirtualItemDrag();
                    Region = ERegion::Content;
                    Select(Index);
                    BeginPointerItemDrag(Index);
                }
            })
            .OnReleased_Lambda([this]()
            {
                if (CraftInput == ECraftInput::Pointer) StopCraftHold();
                if (bPointerItemDown) EndPointerItemDrag();
            })
            .OnClicked_Lambda([this, Index]()
            {
                if (PointerAction() && Dialog == EDialog::None)
                {
                    if (bSuppressItemClick) { bSuppressItemClick = false; return FReply::Handled(); }
                    Region = ERegion::Content;
                    Select(Index);
                    const FModifierKeysState Modifiers = FSlateApplication::Get().GetModifierKeys();
                    const bool Item = SeenPage == 0 && Entries.IsValidIndex(Index)
                        && (Entries[Index].Subject == EHomesteadMenuSubject::ItemGroup
                            || Entries[Index].Subject == EHomesteadMenuSubject::Wearable);
                    if (Item && (bShift || Modifiers.IsShiftDown())) QuickMove(Index);
                    else if (Item && (bControl || Modifiers.IsControlDown())
                        && Entries[Index].Subject == EHomesteadMenuSubject::ItemGroup)
                        OpenQuantityPrompt(Entries[Index]);
                    if (Entries.IsValidIndex(ContentSelection)
                        && (IsDirectCameraSetting(Entries[ContentSelection]) || SeenPage == 2))
                        RunAction(EHomesteadItemAction::Primary);
                }
                return FReply::Handled();
            })
            [ SAssignNew(Contents, SVerticalBox) ], ERegion::Content, Index);
        if (SeenPage == 0 && (Row.Subject == EHomesteadMenuSubject::ItemGroup || Row.Subject == EHomesteadMenuSubject::Wearable))
            StaticCastSharedRef<SMenuButton>(Button)->RightClick = [this, Index]()
            {
                if (PointerAction() && Dialog == EDialog::None) OpenItemContextMenu(Index);
            };
        if (SeenPage == 0)
        {
            Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
            [
                SNew(SBox).WidthOverride(58).HeightOverride(58)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)
                    [ SNew(SBox).WidthOverride(48).HeightOverride(48)
                        [ SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(Row.IconTint) ] ]
                    + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom)
                    [
                        SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                        .BorderBackgroundColor(MenuPine).Padding(FMargin(4, 1))
                        [
                            SNew(STextBlock).Text(FText::AsNumber(FMath::Max(1, Row.Quantity)))
                            .ColorAndOpacity(Ink)
                            .Font(HomesteadUITheme::Font("Bold", 15))
                        ]
                    ]
                ]
            ];
        }
        else if (SeenPage <= 2)
        {
            if (Row.Subject == EHomesteadMenuSubject::Recipe)
            {
                Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
                [
                    SNew(SBox).WidthOverride(58).HeightOverride(58)
                    [
                        SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(MenuGold)
                        .Desaturation(Row.RecipeState.craftable ? 0.0f : 1.0f)
                    ]
                ];
            }
            else
            {
                Contents->AddSlot().AutoHeight().HAlign(HAlign_Center)
                [ SNew(SBox).WidthOverride(48).HeightOverride(48)[ SNew(SHomesteadIcon).Kind(EntryIcon(Row)).Tint(Row.IconTint) ] ];
            }
        }
        if (SeenPage > 1)
            Contents->AddSlot().AutoHeight()[ Text(Name, SeenPage <= 2 ? 16 : 18) ];
        if (SeenPage == 6)
        {
            const int32 Id = Row.Id;
            const int32 Count = AHomesteadController::AppearanceChoiceCount(Id);
            const int32 Current = Controller->AppearanceChoice(Id);
            const bool Swatches = Id >= 1 && Id <= 4;
            TSharedPtr<SWrapBox> Choices;
            Contents->AddSlot().AutoHeight().Padding(0, 8, 0, 2)
            [ SAssignNew(Choices, SWrapBox).UseAllottedSize(true).InnerSlotPadding(FVector2D(6, 6)) ];
            for (int32 Value = 0; Value < Count; ++Value)
            {
                const bool Chosen = Value == Current;
                const FString ChoiceName = Id == 0 ? HomesteadLook::MetaHairName(Value)
                    : Id == 1 ? HomesteadLook::HairColorName(Value) : Id == 2 ? HomesteadLook::SkinToneName(Value)
                    : Id == 3 ? HomesteadLook::EyeColorName(Value) : Id == 4 ? HomesteadLook::TunicColorName(Value)
                    : Id == 6 ? (Value == 0 ? TEXT("Shown") : TEXT("Hidden"))
                    : HomesteadLook::OutfitName(Value);
                const auto Choose = [this, Index, Id, Value]()
                {
                    if (!PointerAction() || Dialog != EDialog::None) return FReply::Handled();
                    Region = ERegion::Content;
                    Select(Index);
                    Controller->MenuSetAppearance(Id, Value);
                    Refresh();
                    return FReply::Handled();
                };
                TSharedRef<SWidget> Face = Swatches
                    ? StaticCastSharedRef<SWidget>(SNew(SBox).WidthOverride(38).HeightOverride(38)
                        [
                            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                            .BorderBackgroundColor(Chosen ? FLinearColor(MenuGold) : HomesteadUITheme::Themed(FLinearColor(0.02f, 0.04f, 0.03f, 0.85f))).Padding(Chosen ? 4 : 2)
                            [
                                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                                .BorderBackgroundColor(AppearanceSwatchColor(Id, Value))
                            ]
                        ])
                    : StaticCastSharedRef<SWidget>(SNew(STextBlock).Text(FText::FromString(ChoiceName))
                        .Font(HomesteadUITheme::Font("Regular", 14))
                        .ColorAndOpacity(Chosen ? FSlateColor(PineInk) : FSlateColor(Ink)));
                Choices->AddSlot()
                [
                    SNew(SMenuButton).ButtonStyle(&MenuButtonStyle()).IsFocusable(false)
                    .ContentPadding(Swatches ? FMargin(0) : FMargin(9, 5))
                    .ButtonColorAndOpacity(Swatches ? FLinearColor::Transparent : Chosen ? MenuGold : Selected)
                    .ToolTipText(FText::FromString(ChoiceName))
                    .OnClicked_Lambda(Choose)
                    [ Face ]
                ];
            }
        }
        Cell = SNew(SBox).WidthOverride(SeenPage <= 2 ? FOptionalSize(ItemCellWidth) : FOptionalSize())
            .MinDesiredWidth(SeenPage <= 2 ? ItemCellWidth : SeenPage == 4 ? 330 : SeenPage == 6 ? AppearancePanelWidth - 60 : 670)
            .MinDesiredHeight(SeenPage == 0 ? 76 : SeenPage == 1 ? 96 : SeenPage <= 2 ? 144 : 72)
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(2)
                .BorderBackgroundColor_Lambda([this, Index]() { return Index == ContentSelection ? MenuGold : FLinearColor::Transparent; })
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()[ Button ]
                    // While she crafts this recipe its square fills with white from the bottom, then flashes.
                    + SOverlay::Slot()
                    [
                        SNew(SHomesteadCraftFill)
                        .Visibility(SeenPage == 1 && Row.Subject == EHomesteadMenuSubject::Recipe
                            ? EVisibility::HitTestInvisible : EVisibility::Collapsed)
                        .Progress_Lambda([this, Recipe = Row.SubjectId]()
                        {
                            return IsHoldingRecipe(Recipe) ? FMath::Clamp(GetCraftProgress(), 0.0f, 1.0f) : 0.0f;
                        })
                        .Flash_Lambda([this, Recipe = Row.SubjectId]() { return CraftFlash(Recipe); })
                    ]
                ]
            ];
        Cells.Add(Cell);
        if (Storage)
        {
            const bool InChest = Row.ContainerId > 0;
            const int32 CellIndex = InChest ? ChestCell++ : PackCell++;
            auto TargetGrid = InChest ? ChestGrid : PackGrid;
            const int32 Width = InChest ? StorageColumns() : StoragePackColumns();
            TargetGrid->AddSlot(CellIndex % Width, CellIndex / Width)[ Cell.ToSharedRef() ];
        }
        else if (AppearanceList) AppearanceList->AddSlot().AutoHeight().Padding(4)[ Cell.ToSharedRef() ];
        else Grid->AddSlot(Index % Columns(), Index / Columns())[ Cell.ToSharedRef() ];
    }
    // Pad the grids with empty slots to at least four full rows, and always complete the last row.
    const auto Pad = [&EmptyCell](const TSharedPtr<SUniformGridPanel>& Target, int32 Used, int32 Width)
    {
        if (!Target || Width <= 0) return;
        const int32 Total = FMath::Max(Width * 4, (Used + Width - 1) / Width * Width);
        for (int32 Cell = FMath::Max(Used, 0); Cell < Total; ++Cell)
            Target->AddSlot(Cell % Width, Cell / Width)[ EmptyCell() ];
    };
    if (Storage)
    {
        Pad(ChestGrid, ChestCell, StorageColumns());
        Pad(PackGrid, PackCell, StoragePackColumns());
    }
    else if (bPackGrid) Pad(Grid, FMath::Max(Entries.Num(), 1), Columns());
    return Result;
}
}
