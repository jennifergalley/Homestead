#include "SHomesteadShop.h"
#include "HomesteadUITheme.h"
#include "SHomesteadFrame.h"
#include "SHomesteadCellBorder.h"
#include "HomesteadPalette.h"

#include "SHomesteadIcon.h"
#include "SHomesteadHudScale.h"
#include "../HomesteadController.h"
#include "../HomesteadShopkeeper.h"
#include "../Simulation/HomesteadBackpack.h"
#include "../Simulation/HomesteadFood.h"
#include "../Simulation/HomesteadItems.h"
#include <algorithm>
#include "Brushes/SlateColorBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

namespace HomesteadMenus
{
namespace
{
const HomesteadUITheme::FThemeColor& ShopInk = HomesteadPalette::Cream;
const HomesteadUITheme::FThemeColor& ShopMuted = HomesteadPalette::Sage;
const HomesteadUITheme::FThemeColor& ShopGold = HomesteadPalette::Brass;
const HomesteadUITheme::FThemeColor& ShopWarning = HomesteadPalette::Warning;
HomesteadUITheme::FThemeColor ShopPanel(0.025f, 0.05f, 0.038f, 0.93f);
HomesteadUITheme::FThemeColor ShopRow(0.05f, 0.085f, 0.065f, 0.85f);
HomesteadUITheme::FThemeColor ShopSelected(0.13f, 0.20f, 0.15f, 0.95f);
const HomesteadUITheme::FThemeColor& ShopPineInk = HomesteadPalette::DeepPine;

const FButtonStyle& ShopButtonStyle()
{
    static const FButtonStyle Style = FButtonStyle()
        .SetNormal(FSlateColorBrush(FLinearColor::White))
        .SetHovered(FSlateColorBrush(FLinearColor(1.12f, 1.12f, 1.12f, 1)))
        .SetPressed(FSlateColorBrush(FLinearColor(0.85f, 0.85f, 0.85f, 1)))
        .SetDisabled(FSlateColorBrush(FLinearColor(0.65f, 0.65f, 0.65f, 1)))
        .SetNormalPadding(FMargin(0)).SetPressedPadding(FMargin(0));
    return Style;
}
FString Utf8(const char* Text) { return UTF8_TO_TCHAR(Text); }
// The backpack and the fishing pole are one-off tools: a price, not "each".
template <typename RowType> bool IsOneOff(const RowType& Row)
{
    return Row.bUpgrade || (!Row.bHeroine && Row.Item == Homestead::Item::FishingPole);
}
FString Money(int64 Amount) { return Utf8(Homestead::FormatMoney(Amount).c_str()); }
}

void SHomesteadShop::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
    ShopId = Args._ShopId;
    Greeting = Args._Greeting;
    bGreeting = !Greeting.IsEmpty();
    ChildSlot
    [
        SNew(SScaleBox).Stretch(EStretch::ScaleToFit)
        [
            SNew(SBox)
            .WidthOverride_Lambda([]() { return FOptionalSize(static_cast<float>(HomesteadMenus::FullScreenLogicalSize().X)); })
            .HeightOverride_Lambda([]() { return FOptionalSize(static_cast<float>(HomesteadMenus::FullScreenLogicalSize().Y)); })
            [
                SAssignNew(Host, SBox)
            ]
        ]
    ];
    Refresh();
}

TSharedRef<SWidget> SHomesteadShop::Label(const FString& Value, int32 Size, const FLinearColor& Color, bool bWrap) const
{
    return SNew(STextBlock).Text(FText::FromString(Value)).ColorAndOpacity(Color)
        .Font(HomesteadUITheme::Font("Regular", Size)).AutoWrapText(bWrap);
}

TSharedRef<SWidget> SHomesteadShop::Button(const FString& Text, TFunction<void()> Action, bool bPrimary, float MinWidth)
{
    return SNew(SBox).MinDesiredWidth(MinWidth)
    [
        SNew(SOverlay)
        + SOverlay::Slot()
        [
            SNew(SButton).ButtonStyle(&ShopButtonStyle()).IsFocusable(false).ContentPadding(FMargin(14, 8))
            .HAlign(HAlign_Center)
            .ButtonColorAndOpacity(bPrimary ? FLinearColor(ShopGold) : HomesteadUITheme::Themed(FLinearColor(0.10f, 0.16f, 0.12f, 1)))
            .OnClicked_Lambda([Action]() { Action(); return FReply::Handled(); })
            [
                SNew(STextBlock).Text(FText::FromString(Text)).Font(HomesteadUITheme::Font("Regular", 15))
                .ColorAndOpacity(bPrimary ? ShopPineInk : ShopInk)
            ]
        ]
        + SOverlay::Slot()[SNew(SHomesteadCellBorder)]
    ];
}

FString SHomesteadShop::Wallet() const
{
    return Controller.IsValid() ? Money(Controller->Simulation().GetState().money) : FString();
}

FString SHomesteadShop::EstateName() const
{
    return Controller.IsValid() ? Controller->EstateName() : FString(TEXT("the estate"));
}

void SHomesteadShop::BuildRows()
{
    Rows.Reset();
    if (!Controller.IsValid()) return;
    const auto& Sim = Controller->Simulation();
    const Homestead::Shop* Shop = Sim.FindShop(ShopId);
    if (!Shop) return;
    if (Tab == 0)
    {
        for (const Homestead::Item Item : Homestead::ShopSellableItems(Sim, Shop->kind))
        {
            FRow Row;
            Row.Item = Item;
            Row.Available = Sim.Count(Item);
            Row.Unit = Homestead::SellPrice(Item);
            Rows.Add(Row);
        }
        return;
    }
    // Upgrades head the Buy list: the leather backpack until she owns it (one only, not a repeating good), then the
    // fishing pole, an ordinary purchase listed with them.
    const auto GoodsRow = [](Homestead::Item Item)
    {
        FRow Row;
        Row.Item = Item;
        Row.Available = -1;
        Row.Unit = Homestead::BuyPrice(Item);
        return Row;
    };
    const auto IsUpgradeGood = [](Homestead::Item Item) { return Item == Homestead::Item::FishingPole; };
    const bool bBackpack = Homestead::Backpack::Offered(Sim.GetState(), Shop->kind);
    const auto& ShopItems = Homestead::ShopGoods(Shop->kind);
    if (bBackpack || std::any_of(ShopItems.begin(), ShopItems.end(), IsUpgradeGood))
    {
        FRow Upgrades;
        Upgrades.Header = TEXT("Upgrades");
        Rows.Add(Upgrades);
    }
    if (bBackpack)
    {
        FRow Backpack;
        Backpack.bUpgrade = true;
        Backpack.Available = 1;
        Backpack.Unit = Homestead::Backpack::Price;
        Rows.Add(Backpack);
    }
    for (const Homestead::Item Item : ShopItems)
        if (IsUpgradeGood(Item)) Rows.Add(GoodsRow(Item));
    FRow Goods;
    Goods.Header = TEXT("Shop goods");
    Rows.Add(Goods);
    for (const Homestead::Item Item : ShopItems)
        if (!IsUpgradeGood(Item)) Rows.Add(GoodsRow(Item));
    FRow Hers;
    Hers.Header = TEXT("From ") + EstateName();
    Rows.Add(Hers);
    const int32 HeaderIndex = Rows.Num() - 1;
    for (int32 Index = 0; Index < Homestead::ItemCount; ++Index)
    {
        if (Shop->heroineStock[Index] <= 0) continue;
        FRow Row;
        Row.Item = static_cast<Homestead::Item>(Index);
        Row.Available = Shop->heroineStock[Index];
        Row.Unit = Homestead::BuyBackPrice(Row.Item);
        Row.bHeroine = true;
        Rows.Add(Row);
    }
    if (Rows.Num() == HeaderIndex + 1)
    {
        FRow Empty;
        Empty.Header = TEXT("Nothing yet.");
        Empty.Available = -2;
        Rows.Add(Empty);
    }
}

int32 SHomesteadShop::FirstChoosable(int32 From, int32 Step) const
{
    for (int32 Index = From; Rows.IsValidIndex(Index); Index += Step)
        if (Rows[Index].Header.IsEmpty()) return Index;
    return INDEX_NONE;
}

const SHomesteadShop::FRow* SHomesteadShop::Chosen() const
{
    return Rows.IsValidIndex(Selection) && Rows[Selection].Header.IsEmpty() ? &Rows[Selection] : nullptr;
}

int32 SHomesteadShop::Limit(const FRow& Row) const
{
    if (!Controller.IsValid()) return 0;
    const auto& Sim = Controller->Simulation();
    if (Tab == 0) return Row.Available;
    if (Row.bUpgrade) return Sim.GetState().money >= Row.Unit ? 1 : 0;
    const int64 Affordable = Row.Unit > 0 ? Sim.GetState().money / Row.Unit : Sim.PackCapacity();
    int32 Room = Sim.PackCapacity() - Sim.UsedCapacity();
    Room = FMath::Min(Room, Sim.PackCapacity() - Sim.Count(Row.Item));
    int64 Most = FMath::Min<int64>(Affordable, Room);
    if (Row.bHeroine) Most = FMath::Min<int64>(Most, Row.Available);
    return static_cast<int32>(FMath::Max<int64>(0, Most));
}

FString SHomesteadShop::RowLabel(int32 Index) const
{
    if (!Rows.IsValidIndex(Index)) return FString();
    const FRow& Row = Rows[Index];
    if (!Row.Header.IsEmpty()) return Row.Header;
    return RowName(Row);
}

FString SHomesteadShop::RowName(const FRow& Row) const
{
    return Utf8(Row.bUpgrade ? Homestead::Backpack::Name : Homestead::ItemName(Row.Item));
}

void SHomesteadShop::Refresh()
{
    BuildRows();
    if (!Chosen())
    {
        const int32 First = FirstChoosable(FMath::Clamp(Selection, 0, FMath::Max(0, Rows.Num() - 1)), 1);
        Selection = First != INDEX_NONE ? First : FirstChoosable(0, 1);
        if (Selection == INDEX_NONE) Selection = 0;
    }
    if (bQuantity)
    {
        const FRow* Row = Chosen();
        const int32 Most = Row ? Limit(*Row) : 0;
        if (!Row || Most <= 0) bQuantity = false;
        else Quantity = FMath::Clamp(Quantity, 1, Most);
    }
    if (Host.IsValid())
    {
        Host->SetContent(
            SNew(SOverlay)
            + SOverlay::Slot()
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                .BorderBackgroundColor(FLinearColor(0, 0, 0, bGreeting ? 0.15f : 0.4f))
            ]
            + SOverlay::Slot()
            [
                bGreeting ? BuildGreeting() : BuildTrade()
            ]);
    }
    ScrollToSelection();
}

TSharedRef<SWidget> SHomesteadShop::BuildGreeting()
{
    const bool bPad = Controller.IsValid() && Controller->UsesGamepad();
    return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(FMargin(0, 0, 0, 56))
    [
        SNew(SBox).WidthOverride(820)
        [
            SNew(SHomesteadFrame)
            [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(ShopPanel)
            .Padding(FMargin(26, 18))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
                    [
                        SNew(SBox).WidthOverride(30).HeightOverride(30)[SNew(SHomesteadIcon).Kind(FName(TEXT("shop")))]
                    ]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                    [
                        Label(AHomesteadShopkeeper::FullName(), 19, ShopGold)
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 14)
                [
                    Label(Greeting, 18, ShopInk)
                ]
                + SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Right)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
                    [
                        Button(bPad ? TEXT("Leave  [B]") : TEXT("Leave  [Esc]"), [this]() { Cancel(); }, false, 120)
                    ]
                    + SHorizontalBox::Slot().AutoWidth()
                    [
                        Button(bPad ? TEXT("Continue  [A]") : TEXT("Continue  [E]"), [this]() { Continue(); }, true, 160)
                    ]
                ]
            ]
            ]
        ]
    ];
}

TSharedRef<SWidget> SHomesteadShop::BuildRow(int32 Index)
{
    const FRow& Row = Rows[Index];
    if (!Row.Header.IsEmpty())
    {
        const bool bNote = Row.Available == -2;
        return SNew(SBox).Padding(FMargin(4, bNote ? 2 : 12, 4, 4))
        [
            Label(Row.Header, bNote ? 14 : 16, bNote ? ShopMuted : ShopGold)
        ];
    }
    const bool bSelected = Index == Selection;
    FString Stock;
    if (Tab == 0) Stock = FString::Printf(TEXT("%d carried"), Row.Available);
    else if (Row.bUpgrade) Stock = FString::Printf(TEXT("Carry %d"), Homestead::MaxPackCapacity);
    else if (Row.bHeroine) Stock = FString::Printf(TEXT("%d on the shelf"), Row.Available);
    else Stock = TEXT("In stock");
    auto Widget = SNew(SButton).ButtonStyle(&ShopButtonStyle()).IsFocusable(false).ContentPadding(FMargin(10, 5))
        .ButtonColorAndOpacity(bSelected ? ShopSelected : ShopRow)
        .OnClicked_Lambda([this, Index]() { Choose(Index); return FReply::Handled(); })
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 12, 0)
            [
                SNew(SBox).WidthOverride(38).HeightOverride(38)
                [
                    SNew(SHomesteadIcon).Kind(Row.bUpgrade ? FName(TEXT("pack")) : FName(UTF8_TO_TCHAR(Homestead::ItemIcon(Row.Item))))
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    // Food shows what eating one now would do before she buys it: its Energy, and for a
                    // Meal on the estate until when she'd be Well fed (Homestead::Food::EffectLabel; the
                    // clock is paused while the shop is open).
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom)
                    [ Label(RowName(Row), 17, bSelected ? ShopGold : ShopInk, false) ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(10, 0, 0, 1)
                    [ Label(Row.bUpgrade || !Controller.IsValid() ? FString() : Utf8(Homestead::Food::EffectLabel(Controller->Simulation().GetState(), Row.Item).c_str()), 14, ShopGold, false) ]
                ]
                + SVerticalBox::Slot().AutoHeight()
                [ Label(Utf8(Row.bUpgrade ? Homestead::Backpack::Description : Homestead::ItemDescription(Row.Item)), 12, ShopMuted) ]
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(16, 0)
            [
                SNew(SBox).WidthOverride(130)[Label(Stock, 14, ShopMuted)]
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
            [
                // Wide enough for "1,000 coins each" on one line.
                SNew(SBox).WidthOverride(170).HAlign(HAlign_Right)
                [ Label(Money(Row.Unit) + (IsOneOff(Row) ? TEXT("") : TEXT(" each")), 16, ShopInk, false) ]
            ]
        ];
    RowWidgets[Index] = Widget;
    return SNew(SBox).Padding(FMargin(0, 2))
    [
        SNew(SOverlay)
        + SOverlay::Slot()[Widget]
        + SOverlay::Slot()[SNew(SHomesteadCellBorder).Brackets(false)]
    ];
}

TSharedRef<SWidget> SHomesteadShop::BuildFooter()
{
    const bool bPad = Controller.IsValid() && Controller->UsesGamepad();
    const FRow* Row = Chosen();
    auto Footer = SNew(SVerticalBox);
    if (!Status.IsEmpty())
        Footer->AddSlot().AutoHeight().Padding(0, 0, 0, 8)[Label(Status, 15, bStatusError ? ShopWarning : ShopInk)];
    if (bQuantity && Row)
    {
        const int64 Total = Row->Unit * Quantity;
        const int64 Purse = Controller->Simulation().GetState().money;
        const int64 After = Tab == 0 ? Purse + Total : Purse - Total;
        const FString Verb = Tab == 0 ? TEXT("Sell") : TEXT("Buy");
        Footer->AddSlot().AutoHeight()
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [
                Label(FString::Printf(TEXT("%s %s"), *Verb, *RowName(*Row)), 18, ShopGold)
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Button(TEXT("-"), [this]() { AdjustQuantity(-1); }, false, 44)]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(10, 0)
            [
                SNew(SBox).WidthOverride(56).HAlign(HAlign_Center)[Label(FString::FromInt(Quantity), 22, ShopInk)]
            ]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[Button(TEXT("+"), [this]() { AdjustQuantity(1); }, false, 44)]
            + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
            [
                Button(bPad ? TEXT("Max [Y]") : TEXT("Max"), [this]() { MaxQuantity(); }, false, 80)
            ]
        ];
        Footer->AddSlot().AutoHeight().Padding(0, 10, 0, 0)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
            [
                Label(FString::Printf(TEXT("Total %s      Purse after %s"),
                    *(Tab == 0 ? TEXT("+") + Money(Total) : TEXT("-") + Money(Total)), *Money(After)), 17, ShopInk)
            ]
            + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 10, 0)
            [
                Button(bPad ? TEXT("Back  [B]") : TEXT("Back  [Esc]"), [this]() { Cancel(); }, false, 110)
            ]
            + SHorizontalBox::Slot().AutoWidth()
            [
                Button(FString::Printf(TEXT("%s  %s"), bPad ? TEXT("[A]") : TEXT("[Enter]"), Tab == 0 ? TEXT("Confirm sale") : TEXT("Confirm purchase")),
                    [this]() { Confirm(); }, true, 190)
            ]
        ];
        return Footer;
    }
    FString Hint = bPad ? TEXT("[A] Choose   [LB / RB] Sell / Buy   [B] Leave")
        : TEXT("Click or [Enter] to choose   [Tab] Sell / Buy   [Esc] Leave");
    Footer->AddSlot().AutoHeight()[Label(Hint, 14, ShopMuted)];
    return Footer;
}

TSharedRef<SWidget> SHomesteadShop::BuildTrade()
{
    const Homestead::Shop* Shop = Controller.IsValid() ? Controller->Simulation().FindShop(ShopId) : nullptr;
    const FString Hours = Shop ? FString::Printf(TEXT("Open %s\u2013%s"),
        UTF8_TO_TCHAR(Homestead::FormatHour(Shop->openHour).c_str()), UTF8_TO_TCHAR(Homestead::FormatHour(Shop->closeHour).c_str()))
        : FString();
    const auto TabButton = [this](int32 Index, const FString& Text)
    {
        return SNew(SBox).MinDesiredWidth(150)
        [
            SNew(SOverlay)
            + SOverlay::Slot()
            [
                SNew(SButton).ButtonStyle(&ShopButtonStyle()).IsFocusable(false).ContentPadding(FMargin(16, 7)).HAlign(HAlign_Center)
                .ButtonColorAndOpacity(Tab == Index ? FLinearColor(ShopGold) : HomesteadUITheme::Themed(FLinearColor(0.10f, 0.16f, 0.12f, 1)))
                .OnClicked_Lambda([this, Index]() { SetTab(Index); return FReply::Handled(); })
                [
                    SNew(STextBlock).Text(FText::FromString(Text)).Font(HomesteadUITheme::Font("Regular", 17))
                    .ColorAndOpacity(Tab == Index ? ShopPineInk : ShopInk)
                ]
            ]
            + SOverlay::Slot()[SNew(SHomesteadCellBorder)]
        ];
    };
    RowWidgets.SetNum(Rows.Num());
    auto ListBox = SAssignNew(List, SScrollBox).ScrollBarThickness(FVector2D(6, 6));
    for (int32 Index = 0; Index < Rows.Num(); ++Index) ListBox->AddSlot()[BuildRow(Index)];
    if (Tab == 0 && Rows.IsEmpty())
        ListBox->AddSlot().Padding(4, 12)[Label(TEXT("You aren't carrying anything this shop buys."), 16, ShopMuted)];
    return SNew(SBox).HAlign(HAlign_Center).VAlign(VAlign_Center)
    [
        SNew(SBox).WidthOverride(920).HeightOverride(600)
        [
            SNew(SHomesteadFrame)
            [
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(ShopPanel)
            .Padding(FMargin(26, 18))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 10, 0)
                    [
                        SNew(SBox).WidthOverride(34).HeightOverride(34)[SNew(SHomesteadIcon).Kind(FName(TEXT("shop")))]
                    ]
                    + SHorizontalBox::Slot().FillWidth(1).VAlign(VAlign_Center)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot().AutoHeight()[Label(TEXT("Trethewey's General Store"), 22, ShopInk)]
                        + SVerticalBox::Slot().AutoHeight()[Label(Hours, 13, ShopMuted)]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 8, 0)
                    [
                        SNew(SBox).WidthOverride(30).HeightOverride(30)[SNew(SHomesteadIcon).Kind(FName(TEXT("coin")))]
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(0, 0, 18, 0)
                    [
                        Label(Wallet(), 22, ShopGold, false)
                    ]
                    + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                    [
                        Button(Controller.IsValid() && Controller->UsesGamepad() ? TEXT("Leave [B]") : TEXT("Leave"),
                            [this]() { if (Controller.IsValid()) Controller->CloseShopScreen(); }, false, 90)
                    ]
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 14, 0, 10)
                [
                    SNew(SHorizontalBox)
                    + SHorizontalBox::Slot().AutoWidth().Padding(0, 0, 8, 0)[TabButton(0, TEXT("Sell"))]
                    + SHorizontalBox::Slot().AutoWidth()[TabButton(1, TEXT("Buy"))]
                ]
                + SVerticalBox::Slot().FillHeight(1)
                [
                    ListBox
                ]
                + SVerticalBox::Slot().AutoHeight().Padding(0, 12, 0, 0)
                [
                    SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
                    .BorderBackgroundColor(HomesteadUITheme::Themed(FLinearColor(0.04f, 0.07f, 0.055f, 0.95f))).Padding(FMargin(14, 10))
                    [
                        BuildFooter()
                    ]
                ]
            ]
            ]
        ]
    ];
}

void SHomesteadShop::ScrollToSelection()
{
    if (List.IsValid() && RowWidgets.IsValidIndex(Selection) && RowWidgets[Selection].IsValid())
        List->ScrollDescendantIntoView(RowWidgets[Selection], true, EDescendantScrollDestination::IntoView);
}

void SHomesteadShop::Continue()
{
    if (!bGreeting) return;
    bGreeting = false;
    if (Controller.IsValid()) Controller->ShopClick();
    Refresh();
}

void SHomesteadShop::SetTab(int32 NewTab)
{
    if (bGreeting) return;
    NewTab = (NewTab % 2 + 2) % 2;
    if (NewTab == Tab && !bQuantity) return;
    Tab = NewTab;
    Selection = 0;
    bQuantity = false;
    Status.Reset();
    if (Controller.IsValid()) Controller->ShopClick();
    Refresh();
}

void SHomesteadShop::Move(int32 Delta)
{
    if (bGreeting || bQuantity || Rows.IsEmpty()) return;
    const int32 Next = FirstChoosable(Selection + Delta, Delta > 0 ? 1 : -1);
    if (Next == INDEX_NONE) return;
    Selection = Next;
    if (Controller.IsValid()) Controller->ShopClick();
    Refresh();
}

void SHomesteadShop::Choose(int32 Index)
{
    if (bGreeting || !Rows.IsValidIndex(Index) || !Rows[Index].Header.IsEmpty()) return;
    Selection = Index;
    const int32 Most = Limit(Rows[Index]);
    if (Most <= 0)
    {
        bQuantity = false;
        bStatusError = true;
        const auto& Sim = Controller->Simulation();
        Status = Tab == 0 ? TEXT("You have none to sell.")
            : Sim.GetState().money < Rows[Index].Unit ? FString::Printf(TEXT("That's %s%s; you have %s."), *Money(Rows[Index].Unit),
                IsOneOff(Rows[Index]) ? TEXT("") : TEXT(" each"), *Wallet())
            : TEXT("Your pack is full.");
        Refresh();
        return;
    }
    bQuantity = true;
    Quantity = 1;
    Status.Reset();
    if (Controller.IsValid()) Controller->ShopClick();
    Refresh();
}

void SHomesteadShop::AdjustQuantity(int32 Delta)
{
    const FRow* Row = Chosen();
    if (!bQuantity || !Row) return;
    const int32 Next = FMath::Clamp(Quantity + Delta, 1, FMath::Max(1, Limit(*Row)));
    if (Next == Quantity) return;
    Quantity = Next;
    if (Controller.IsValid()) Controller->ShopClick();
    Refresh();
}

void SHomesteadShop::MaxQuantity()
{
    const FRow* Row = Chosen();
    if (!bQuantity || !Row) return;
    Quantity = FMath::Max(1, Limit(*Row));
    Refresh();
}

void SHomesteadShop::Confirm()
{
    const FRow* Row = Chosen();
    if (!bQuantity || !Row || !Controller.IsValid()) return;
    const auto Result = Row->bUpgrade ? Controller->ShopBuyBackpack(ShopId)
        : Controller->ShopTrade(ShopId, Row->Item, Quantity, Tab == 0, Row->bHeroine);
    Status = UTF8_TO_TCHAR(Result.message.c_str());
    bStatusError = !Result.ok;
    if (Result.ok) bQuantity = false;
    Refresh();
}

void SHomesteadShop::Cancel()
{
    if (bQuantity)
    {
        bQuantity = false;
        if (Controller.IsValid()) Controller->ShopClick();
        Refresh();
        return;
    }
    if (Controller.IsValid()) Controller->CloseShopScreen();
}

bool SHomesteadShop::HandleKey(FKey Key, EInputEvent Event, float Amount)
{
    if (Key == EKeys::Gamepad_LeftY || Key == EKeys::Gamepad_LeftX)
    {
        // A tilted stick steps once, then again only after it returns toward centre.
        float& Latch = Key == EKeys::Gamepad_LeftY ? StickLatch : StickX;
        if (FMath::Abs(Amount) < 0.35f) { Latch = 0; return true; }
        if (Latch != 0) return true;
        Latch = FMath::Sign(Amount);
        if (Key == EKeys::Gamepad_LeftY) { if (bQuantity) AdjustQuantity(Amount > 0 ? 10 : -10); else Move(Amount > 0 ? -1 : 1); }
        else if (bQuantity) AdjustQuantity(Amount > 0 ? 1 : -1);
        return true;
    }
    if (Event != IE_Pressed && Event != IE_Repeat) return true;
    if (Key == EKeys::Escape || Key == EKeys::BackSpace || Key == EKeys::Gamepad_FaceButton_Right) { Cancel(); return true; }
    if (bGreeting)
    {
        if (Key == EKeys::E || Key == EKeys::Enter || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom
            || Key == EKeys::LeftMouseButton) Continue();
        return true;
    }
    const bool Up = Key == EKeys::Up || Key == EKeys::W || Key == EKeys::Gamepad_DPad_Up;
    const bool Down = Key == EKeys::Down || Key == EKeys::S || Key == EKeys::Gamepad_DPad_Down;
    const bool Left = Key == EKeys::Left || Key == EKeys::A || Key == EKeys::Gamepad_DPad_Left;
    const bool Right = Key == EKeys::Right || Key == EKeys::D || Key == EKeys::Gamepad_DPad_Right;
    if (Key == EKeys::Tab || Key == EKeys::Gamepad_RightShoulder || Key == EKeys::Gamepad_LeftShoulder || Key == EKeys::Q)
    {
        SetTab(Tab + 1);
        return true;
    }
    if (Key == EKeys::One) { SetTab(0); return true; }
    if (Key == EKeys::Two) { SetTab(1); return true; }
    const bool Accept = Key == EKeys::Enter || Key == EKeys::E || Key == EKeys::SpaceBar || Key == EKeys::Gamepad_FaceButton_Bottom;
    if (bQuantity)
    {
        if (Left) AdjustQuantity(-1);
        else if (Right) AdjustQuantity(1);
        else if (Up) AdjustQuantity(10);
        else if (Down) AdjustQuantity(-10);
        else if (Key == EKeys::Gamepad_FaceButton_Top || Key == EKeys::M) MaxQuantity();
        else if (Accept) Confirm();
        return true;
    }
    if (Up) Move(-1);
    else if (Down) Move(1);
    else if (Left) SetTab(0);
    else if (Right) SetTab(1);
    else if (Accept) Choose(Selection);
    return true;
}

FReply SHomesteadShop::OnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
    if (Controller.IsValid()) Controller->NoteShopDevice(Event.GetKey().IsGamepadKey());
    HandleKey(Event.GetKey(), Event.IsRepeat() ? IE_Repeat : IE_Pressed, 1.0f);
    return FReply::Handled();
}

FReply SHomesteadShop::OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event)
{
    const FKey Key = Event.GetKey();
    if (Key != EKeys::Gamepad_LeftY && Key != EKeys::Gamepad_LeftX) return FReply::Unhandled();
    if (FMath::Abs(Event.GetAnalogValue()) > 0.35f && Controller.IsValid()) Controller->NoteShopDevice(true);
    HandleKey(Key, IE_Axis, Event.GetAnalogValue());
    return FReply::Handled();
}
}
