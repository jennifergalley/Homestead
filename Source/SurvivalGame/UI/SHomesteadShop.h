#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../Simulation/HomesteadItems.h"

class AHomesteadController;
class SBox;
class SScrollBox;

namespace HomesteadMenus
{
// The shop screen: a greeting from the shopkeeper, then Sell and Buy panes in the field-book style.
// Buy lists the shop's own goods and, under "From {Estate}", the goods she has sold here. Choosing
// a row opens a quantity step with the total and the purse after it, then Confirm. Fully usable
// with the mouse alone, the keyboard alone, or a controller alone; the game is paused while open.
class SHomesteadShop : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadShop) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
        SLATE_ARGUMENT(int32, ShopId)
        SLATE_ARGUMENT(FString, Greeting)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override;
    virtual FReply OnKeyUp(const FGeometry&, const FKeyEvent&) override { return FReply::Handled(); }
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event) override;
    virtual FNavigationReply OnNavigation(const FGeometry&, const FNavigationEvent&) override
    { return FNavigationReply::Stop(); }
    bool HandleKey(FKey Key, EInputEvent Event, float Amount);
    void Refresh();

    // Inspection for playtests.
    bool IsGreeting() const { return bGreeting; }
    bool IsSellTab() const { return Tab == 0; }
    int32 GetSelection() const { return Selection; }
    bool IsChoosingQuantity() const { return bQuantity; }
    int32 GetQuantity() const { return Quantity; }
    int32 RowCount() const { return Rows.Num(); }
    FString RowLabel(int32 Index) const;
    // Whether the row is the one-time backpack upgrade (for playtests).
    bool IsUpgradeRow(int32 Index) const { return Rows.IsValidIndex(Index) && Rows[Index].bUpgrade; }
    FString GetStatus() const { return Status; }

    // Actions shared by every input route.
    void Continue();
    void SetTab(int32 NewTab);
    void Move(int32 Delta);
    void Choose(int32 Index);
    void AdjustQuantity(int32 Delta);
    void MaxQuantity();
    void Confirm();
    void Cancel();

private:
    struct FRow
    {
        Homestead::Item Item = Homestead::Item::Count;
        int32 Available = 0;
        int64 Unit = 0;
        bool bHeroine = false;
        // A one-time upgrade (the leather backpack) rather than goods: bought once, never a quantity.
        bool bUpgrade = false;
        FString Header; // Set for section headings, which can't be chosen.
    };
    TWeakObjectPtr<AHomesteadController> Controller;
    int32 ShopId = 0;
    FString Greeting;
    bool bGreeting = true;
    int32 Tab = 0;
    int32 Selection = 0;
    bool bQuantity = false;
    int32 Quantity = 1;
    FString Status;
    bool bStatusError = false;
    TArray<FRow> Rows;
    TSharedPtr<SBox> Host;
    TSharedPtr<SScrollBox> List;
    TArray<TSharedPtr<SWidget>> RowWidgets;
    float StickLatch = 0.0f;
    float StickX = 0.0f;

    void BuildRows();
    int32 FirstChoosable(int32 From, int32 Step) const;
    int32 Limit(const FRow& Row) const;
    const FRow* Chosen() const;
    TSharedRef<SWidget> BuildGreeting();
    TSharedRef<SWidget> BuildTrade();
    TSharedRef<SWidget> BuildRow(int32 Index);
    TSharedRef<SWidget> BuildFooter();
    TSharedRef<SWidget> Button(const FString& Label, TFunction<void()> Action, bool bPrimary = false, float MinWidth = 0.0f);
    // bWrap false for short single-line values (purse, prices, Energy): auto-wrap in an auto-width
    // slot wraps at the last frame's width and can break "100 coins each" or "+40 Energy" in two.
    TSharedRef<SWidget> Label(const FString& Value, int32 Size, const FLinearColor& Color, bool bWrap = true) const;
    FString Wallet() const;
    FString RowName(const FRow& Row) const;
    FString EstateName() const;
    void ScrollToSelection();
};
}
using SHomesteadShop = HomesteadMenus::SHomesteadShop;
