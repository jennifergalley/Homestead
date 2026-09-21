#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../HomesteadController.h"

class SScrollBox;
class SVerticalBox;
class SHorizontalBox;
class SBox;

namespace HomesteadMenus
{
class SHomesteadMenu : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadMenu) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual void Tick(const FGeometry& Geometry, double Time, float Delta) override;
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& Event) override;
    virtual FReply OnKeyUp(const FGeometry&, const FKeyEvent& Event) override;
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override;
    virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override;
    bool HandleKey(FKey Key, EInputEvent Event, float Amount);
    void ChangePage(int32 Page);
    void RequestExit();
    void ShowSaveFailure(const FString& Error);
    void ShowGraphicsSaveFailure(const FString& Error);
    void Back();
    void Refresh();
    bool PrepareQuickAction();

private:
    enum class ERegion { Tabs, Session, Inventory, Portrait, Content, Details, Actions };
    enum class EDialog { None, Exit, SaveFailed, GraphicsFailed, Unsaved, Restart, TestReset, Amount, Merge };
    TWeakObjectPtr<AHomesteadController> Controller;
    TSharedPtr<SVerticalBox> Root;
    TSharedPtr<SBox> ContentHost;
    TSharedPtr<SBox> ModalHost;
    TSharedPtr<SBox> DetailsHost;
    TSharedPtr<SScrollBox> Scroll;
    TSharedPtr<SScrollBox> DetailsScroll;
    TSharedPtr<SScrollBox> DialogScroll;
    TArray<TSharedPtr<SWidget>> DialogButtons;
    TArray<TSharedPtr<SWidget>> Cells;
    TArray<FHomesteadRow> Entries;
    TArray<int32> RowIndices;
    TArray<EHomesteadItemAction> Actions;
    int32 SeenPage = -1;
    int32 Hover = INDEX_NONE;
    int32 ContentSelection = 0;
    int32 DesiredColumn = 0;
    int32 ActionSelection = 0;
    int32 SessionSelection = 0;
    int32 DialogSelection = 0;
    int32 FocusedTab = 0;
    FString RememberedKeys[7];
    int32 InventorySelection = 0;
    ERegion Region = ERegion::Content;
    EDialog Dialog = EDialog::None;
    FString DialogError;
    FString Signature;
    bool bControl = false;
    bool bShift = false;
    bool bSaving = false;
    bool bRecovery = false;
    bool bResetPromptShown = false;
    double NextAxisMove = 0;
    FHomesteadRow PendingRow;
    EHomesteadItemAction PendingAction = EHomesteadItemAction::Primary;
    uint64 PendingRevision = 0;
    int32 Amount = 1, MaximumAmount = 1;
    TArray<int32> MergeTargets;

    TSharedRef<SWidget> BuildBody();
    TSharedRef<SWidget> BuildDetails();
    TSharedRef<SWidget> MakeButton(const FString& Label, TFunction<void()> Action,
        TAttribute<FSlateColor> Color = FSlateColor(FLinearColor(0.025f, 0.05f, 0.038f, 0.97f)));
    TSharedRef<SWidget> Text(const FString& Value, int32 Size = 18) const;
    FString EntryName(const FHomesteadRow& Row) const;
    FName EntryIcon(const FHomesteadRow& Row) const;
    FString DetailsText() const;
    FString Footer() const;
    FLinearColor CellColor(int32 Index) const;
    int32 Columns() const;
    int32 DetailIndex() const;
    void Select(int32 Index, bool KeepDesiredColumn = false);
    void Activate();
    void RunAction(EHomesteadItemAction Action);
    FString ActionLabel(EHomesteadItemAction Action) const;
    FString RowKey(const FHomesteadRow& Row) const;
    void ChangeInventoryView(int32 View);
    void CycleRegion(int32 Direction);
    void SetDialog(EDialog Value);
    void BuildDialog();
    void DialogAction(int32 Index);
    int32 DialogCount() const;
    bool PointerAction();
};
}
