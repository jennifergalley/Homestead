#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "../HomesteadController.h"
#include "HomesteadMenuNavigation.h"

class SScrollBox;
class SVerticalBox;
class SHorizontalBox;
class SBox;
class SButton;
class SSlider;

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
    virtual FReply OnPreviewKeyDown(const FGeometry&, const FKeyEvent& Event) override;
    virtual FNavigationReply OnNavigation(const FGeometry&, const FNavigationEvent&) override;
    virtual FReply OnKeyUp(const FGeometry&, const FKeyEvent& Event) override;
    virtual FReply OnAnalogValueChanged(const FGeometry&, const FAnalogInputEvent& Event) override;
    virtual FReply OnMouseMove(const FGeometry&, const FPointerEvent& Event) override;
    virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override;
    bool HandleKey(FKey Key, EInputEvent Event, float InputAmount);
    void ChangePage(int32 Page);
    bool FocusLegacySubject(int32 Id);
    bool FocusSubject(EHomesteadMenuSubject Subject, int32 SubjectId, int32 ContainerId);
    bool FocusItemAction(EHomesteadItemAction Action);
    void RequestExit();
    void ShowSaveFailure(const FString& Error);
    void ShowGraphicsSaveFailure(const FString& Error);
    void RequestTestResetPrompt() { bResetPromptShown = false; }
    void Back();
    void Refresh();
    bool PrepareQuickAction();
    bool HasActiveDialog() const { return Dialog != EDialog::None; }
    bool IsExitPrompt() const { return Dialog == EDialog::Exit; }
    bool IsSaveError() const { return Dialog == EDialog::SaveFailed; }
    bool IsUnsavedPrompt() const { return Dialog == EDialog::Unsaved; }
    bool IsTestResetPrompt() const { return Dialog == EDialog::TestReset; }
    // Right-click item menu and the Ctrl+click how-many popover (pointer-first inventory handling).
    bool IsItemContextMenu() const { return Dialog == EDialog::Context; }
    bool IsQuantityPrompt() const { return Dialog == EDialog::Quantity; }
    int32 DialogCountForTest() const { return DialogCount(); }
    FString GetPopupOptionLabel(int32 Index) const
    { return PopupOptions.IsValidIndex(Index) && PopupOptions[Index].Label ? PopupOptions[Index].Label() : FString(); }
    TSharedPtr<SWidget> GetDialogButton(int32 Index) const
    { return DialogButtons.IsValidIndex(Index) ? DialogButtons[Index] : nullptr; }
    void OpenItemContextMenu(int32 Index);
    void OpenQuantityPrompt(const FHomesteadRow& Row);
    const FHomesteadRow* GetSelectedSubject() const
    {
        const bool SubjectFocused = Region == ERegion::Content || Region == ERegion::Details || Region == ERegion::Actions;
        return Dialog == EDialog::None && SubjectFocused && Entries.IsValidIndex(ContentSelection) ? &Entries[ContentSelection] : nullptr;
    }
    FString GetDisplayedDetails() const { return DetailsText(); }
    FString GetFocusedRequirementHint() const;
    FString GetFocusedRegionName() const;
    bool HasSynchronizedFocus() const;
    bool IsFocusedControlVisible() const;
    int32 GetSelectedContentIndex() const { return ContentSelection; }
    int32 GetContentColumnCount() const { return Columns(); }
    int32 GetActionCount() const { return Actions.Num(); }
    TSharedPtr<SWidget> GetAudioSliderWidget(int32 AudioId) const;
    bool IsEditingQuantity() const { return bEditingAmount; }
    int32 GetDraftQuantity() const { return Amount; }
    bool IsPointerDraggingItem() const { return bPointerDraggingItem; }
    bool IsVirtualDraggingItem() const { return bVirtualDraggingItem; }
    float GetContentScrollOffset() const { return Scroll ? Scroll->GetScrollOffset() : 0.0f; }
    float GetContentScrollBottom() const
    {
        return Scroll ? Scroll->GetCachedGeometry().GetAbsolutePosition().Y
            + Scroll->GetCachedGeometry().GetAbsoluteSize().Y : 0.0f;
    }
    void PointerItemDragMove(FVector2D Position);
    float GetCraftProgress() const { return CraftHoldRecipe >= 0 ? CraftHoldElapsed / CraftCycleSeconds : 0.0f; }

private:
    enum class ERegion { Tabs, Session, Inventory, Portrait, Content, Equipment, Details, Actions, Recovery };
    enum class EDialog { None, Exit, SaveFailed, GraphicsFailed, Unsaved, Restart, TestReset, Amount, Merge, DropWearable, Context, Quantity };
    TWeakObjectPtr<AHomesteadController> Controller;
    TSharedPtr<SVerticalBox> Root;
    TSharedPtr<SHorizontalBox> TabBar;
    TSharedPtr<SBox> ContentHost;
    TSharedPtr<SBox> ModalHost;
    TSharedPtr<SBox> DetailsHost;
    TSharedPtr<SScrollBox> Scroll;
    TSharedPtr<SScrollBox> DetailsScroll;
    TSharedPtr<SScrollBox> DialogScroll;
    TArray<TSharedPtr<SWidget>> DialogButtons;
    TArray<TSharedPtr<SWidget>> Cells;
    TArray<TSharedPtr<SWidget>> ActionButtons;
    TArray<TSharedPtr<SSlider>> AudioSliders;
    TArray<FString> RequirementHints;
    TArray<FHomesteadRow> Entries;
    TArray<int32> RowIndices;
    TArray<EHomesteadItemAction> Actions;
    struct FFocusTarget
    {
        ERegion region;
        int32 index;
        TWeakPtr<SWidget> widget;
    };
    TArray<FFocusTarget> FocusTargets;
    TSharedPtr<SWidget> AmountControl;
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
    int32 EquipmentSelection = 0;
    int32 PortraitSelection = -1;
    int32 RecoverySelection = 0;
    int32 DetailsSelection = 0;
    ERegion Region = ERegion::Content;
    EDialog Dialog = EDialog::None;
    FString DialogError;
    FString Signature;
    bool bControl = false;
    bool bShift = false;
    bool bSaving = false;
    bool bRecovery = false;
    bool bResetPromptShown = false;
    HomesteadMenuNavigation::StickNavigation LeftStick;
    HomesteadMenuNavigation::Direction PendingDirection;
    bool bFocusPending = false;
    bool bSynchronizingFocus = false;
    bool bEditingAmount = false;
    FHomesteadRow PendingRow;
    EHomesteadItemAction PendingAction = EHomesteadItemAction::Primary;
    uint64 PendingRevision = 0;
    int32 Amount = 1, MaximumAmount = 1;
    TArray<int32> MergeTargets;
    struct FPopupOption { TFunction<FString()> Label; TFunction<void()> Run; TFunction<bool()> Enabled; };
    TArray<FPopupOption> PopupOptions;
    FString PopupTitle;
    FVector2D PopupAnchor = FVector2D::ZeroVector;
    void BuildPopup();
    void AdjustQuantity(int32 Delta);
    // Shift+click: the whole stack or garment to the other side of an open chest.
    void MoveWhole(int32 Index);
    int32 AudioEditId = -1;
    float AudioEditStart = 0;
    int32 PointerDragSource = INDEX_NONE;
    int32 PointerDragTarget = INDEX_NONE;
    FVector2D PointerDragStart = FVector2D::ZeroVector;
    uint64 PointerDragRevision = 0;
    bool bPointerItemDown = false;
    bool bPointerDraggingItem = false;
    bool bSuppressItemClick = false;
    int32 VirtualDragSource = INDEX_NONE;
    uint64 VirtualDragRevision = 0;
    bool bVirtualDraggingItem = false;
    enum class ECraftInput { None, Pointer, Keyboard, Controller };
    static constexpr float CraftCycleSeconds = 1.2f;
    int32 CraftHoldRecipe = INDEX_NONE;
    float CraftHoldElapsed = 0;
    int32 CraftBeat = 0;
    ECraftInput CraftInput = ECraftInput::None;

    TSharedRef<SWidget> BuildBody();
    TSharedRef<SWidget> BuildDetails();
    TSharedRef<SButton> MakeButton(const FString& Label, TFunction<void()> Action,
        TAttribute<FSlateColor> Color = FSlateColor(FLinearColor(0.025f, 0.05f, 0.038f, 0.97f)),
        const FString& AccessibleLabel = FString(), FMargin Padding = FMargin(14, 10));
    TSharedRef<SWidget> Text(const FString& Value, int32 Size = 18) const;
    FString EntryName(const FHomesteadRow& Row) const;
    FName EntryIcon(const FHomesteadRow& Row) const;
    bool IsDirectCameraSetting(const FHomesteadRow& Row) const;
    FString DetailsText() const;
    FString DetailsBodyText() const;
    void ScrollActionIntoView();
    FString Footer() const;
    FLinearColor CellColor(int32 Index) const;
    int32 Columns() const;
    int32 DetailIndex() const;
    void Select(int32 Index, bool KeepDesiredColumn = false);
    void SplitSelectedHalf();
    void BeginPointerItemDrag(int32 Index);
    void EndPointerItemDrag();
    void CancelPointerItemDrag();
    void BeginOrCommitVirtualItemDrag();
    void CancelVirtualItemDrag();
    bool StartCraftHold(ECraftInput Input);
    void StopCraftHold();
    bool IsHoldingRecipe(int32 Recipe) const;
    void Activate();
    void RunAction(EHomesteadItemAction Action);
    FString ActionLabel(EHomesteadItemAction Action) const;
    FString RowKey(const FHomesteadRow& Row) const;
    void ChangeInventoryView(int32 View);
    void FocusEquipment(int32 Index);
    FString EquipmentLabel(int32 Index) const;
    void CycleRegion(int32 Direction);
    void SetDialog(EDialog Value);
    void BuildDialog();
    void DialogAction(int32 Index);
    int32 DialogCount() const;
    bool PointerAction();
    TSharedRef<SButton> RegisterButton(TSharedRef<SButton> Button, ERegion TargetRegion, int32 Index);
    TSharedRef<SWidget> FocusAnchor(TSharedRef<SWidget> Content, ERegion TargetRegion, int32 Index);
    void AdoptFocus(ERegion TargetRegion, int32 Index);
    TSharedPtr<SWidget> FocusWidget() const;
    bool IsTargetAvailable(ERegion TargetRegion, int32 Index) const;
    void SynchronizeFocus();
    void NavigateDirection(HomesteadMenuNavigation::Direction Direction);
    void NavigateSpatial(HomesteadMenuNavigation::Direction Direction);
    void NavigateDialog(HomesteadMenuNavigation::Direction Direction);
    bool MoveWithin(int32& Index, int32 Count, int32 Columns, HomesteadMenuNavigation::Direction Direction, int32 Desired = -1);
};
}
