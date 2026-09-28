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
namespace HomesteadMenus { class SHomesteadMapView; }

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
    // Settings is split into Game (0), Sound (1) and Video (2) tabs below the always-visible session rows.
    int32 GetSettingsTab() const { return SettingsTab; }
    void SetSettingsTab(int32 Tab);
    static int32 SettingsTabOf(int32 SettingId);
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
    void OpenItemContextMenu(int32 Index, bool bPointer = true);
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
    TSharedPtr<SHomesteadMapView> GetMapView() const { return MapView; }
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
    int32 SettingsTab = 0;
    int32 DialogSelection = 0;
    int32 FocusedTab = 0;
    FString RememberedKeys[8];
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
    struct FPopupOption
    {
        TFunction<FString()> Label; TFunction<void()> Run; TFunction<bool()> Enabled;
        // The item action this option performs, so keyboard/controller routes can land on it.
        TOptional<EHomesteadItemAction> Action;
    };
    TArray<FPopupOption> PopupOptions;
    FString PopupTitle;
    FVector2D PopupAnchor = FVector2D::ZeroVector;
    bool bKeepPopupAnchor = false;
    void BuildPopup();
    void AdjustQuantity(int32 Delta);
    bool BuildItemOptions(const FHomesteadRow& Row);
    void OpenItemContextMenuFor(const FHomesteadRow& Row, FVector2D Anchor);
    // Where a popup opens: at the pointer for mouse input, beside the focused tile otherwise.
    FVector2D PopupAnchorFor(const TSharedPtr<SWidget>& Widget, bool bPointer) const;
    // Shift+click: the whole stack or garment to the other side of an open chest; with no chest
    // open, pins tools and food to the hotbar and puts on carried garments.
    void QuickMove(int32 Index);
    void ComputeActions();
    int32 StorageColumns() const;
    int32 SettingsTopCount() const;
    FString PackHint() const;
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
    // The recipe that just finished a cycle, for the square's completion flash.
    int32 CraftFlashRecipe = INDEX_NONE;
    double CraftFlashStart = 0;
    int32 CraftBeat = 0;
    ECraftInput CraftInput = ECraftInput::None;

    TSharedRef<SWidget> BuildBody();
    // The Map tab (page 7): one focusable map view that takes sticks, triggers and the D-pad.
    TSharedRef<SWidget> BuildMap();
    bool HandleMapKey(FKey Key, EInputEvent Event, float InputAmount);
    TSharedPtr<SHomesteadMapView> MapView;
    TSharedRef<SWidget> BuildDetails();
    TSharedRef<SButton> MakeButton(const FString& Label, TFunction<void()> Action,
        TAttribute<FSlateColor> Color = FSlateColor(FLinearColor(0.025f, 0.05f, 0.038f, 0.6f)),
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
    float CraftFlash(int32 Recipe) const;
    bool IsHoldingRecipe(int32 Recipe) const;
    void Activate();
    void RunAction(EHomesteadItemAction Action);
    FString ActionLabel(EHomesteadItemAction Action) const;
    FString RowKey(const FHomesteadRow& Row) const;
    void ChangeInventoryView(int32 View);
    void FocusEquipment(int32 Index, bool bPointer = false);
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
