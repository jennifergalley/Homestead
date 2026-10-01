#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "HomesteadTextEdit.h"
#include "Widgets/SOverlay.h"
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
    virtual FReply OnMouseButtonDown(const FGeometry&, const FPointerEvent& Event) override;
    virtual FReply OnMouseButtonUp(const FGeometry&, const FPointerEvent& Event) override;
    virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override;
    virtual FReply OnMouseWheel(const FGeometry&, const FPointerEvent& Event) override;
    virtual FReply OnKeyChar(const FGeometry&, const FCharacterEvent& Event) override;
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
    // The field book's tabs (automation): how many there are, and whether one opens `Page`.
    int32 GetTabPageCount() const;
    // The pages the tab bar cycles through (MenuDetail::FieldBookPages), and how many tabs Settings has.
    static TConstArrayView<int32> TabPages();
    static constexpr int32 SettingsTabCount = 3;
    bool HasTabForPage(int32 Page) const;
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
    // The hotbar row: the first row of her pack (Simulation/HomesteadPackRow.h), shown as ten cells
    // above the pack grid (with a chest open, above the pack column's grid), and what she has picked up to place
    // in it (a pack or chest stack from "Move to a hotbar slot", or a cell being moved).
    int32 GetBookHotbarSlotCount() const { return HotbarCells.Num(); }
    TSharedPtr<SWidget> GetBookHotbarSlot(int32 Slot) const { return HotbarCells.IsValidIndex(Slot) ? HotbarCells[Slot] : nullptr; }
    int32 GetHeldHotbarSlot() const { return HeldHotbarSlot; }
    bool IsPlacingOnHotbar() const { return HeldHotbarRow.IsSet(); }
    int32 GetFocusedHotbarSlot() const { return Region == ERegion::Hotbar ? HotbarSelection : INDEX_NONE; }
    // Keyboard / controller: A (or Enter) on a hotbar slot, as a player would.
    void ActivateHotbarSlot(int32 Slot);
    // "Move to a hotbar slot": hold this stack and move focus to the row to choose a cell.
    void BeginPlacingOnHotbar(const FHomesteadRow& Row);
    float GetContentScrollOffset() const { return Scroll ? Scroll->GetScrollOffset() : 0.0f; }
    float GetContentScrollTop() const { return Scroll ? Scroll->GetCachedGeometry().GetAbsolutePosition().Y : 0.0f; }
    // Left edge of the pack's grid column with a chest open (0 without one).
    float GetPackColumnLeft() const { return PackDropArea ? PackDropArea->GetCachedGeometry().GetAbsolutePosition().X : 0.0f; }
    float GetContentScrollBottom() const
    {
        return Scroll ? Scroll->GetCachedGeometry().GetAbsolutePosition().Y
            + Scroll->GetCachedGeometry().GetAbsoluteSize().Y : 0.0f;
    }
    void PointerItemDragMove(FVector2D Position);
    float GetCraftProgress() const { return CraftHoldRecipe >= 0 ? CraftHoldElapsed / CraftCycleSeconds : 0.0f; }
    // The notice card that floats over the book for the latest toast (never takes layout space).
    bool IsNoticeShowing() const;
    bool IsNoticeAtTop() const { return bNoticeTop; }
    // A road sign's offer (AHomesteadController::InteractWithRoadSign): the Map tab's walk confirm, one
    // "Walk to ..." per way the sign points, centred over the book.
    void OpenSignTravelPrompt(const FString& SignWords, const TArray<Homestead::TravelDestination>& Destinations);
    bool IsTravelPromptOpen() const { return Dialog == EDialog::Context && bTravelPrompt; }
    FString GetNoticeText() const { return IsNoticeShowing() ? NoticeText : FString(); }
    bool IsDyeChooserOpen() const { return bDyeChooser && Dialog == EDialog::Context; }
    int32 GetDyePreview() const { return DyePreviewed; }
    int32 GetDyeChoice() const { return DyeChoice; }
    // Naming the open chest: typed on the keyboard (OnKeyChar), or one of the suggestions.
    void OpenRenameChest();
    // Sound sliders stepped with the d-pad or keyboard: saved once when she confirms, moves to another
    // row, page or region, or closes the book; Cancel (B/Esc) puts the level back unsaved.
    void CommitAudioStep();
    bool CancelAudioStep();
    bool HasPendingAudioStep() const { return bAudioStepEdit; }
    bool IsRenamingChest() const { return Dialog == EDialog::RenameChest; }
    FString GetChestNameDraft() const { return RenameDraft; }
    // Automation: types as the keyboard would.
    void TypeChestName(const FString& Characters);
    bool IsNoticeError() const { return bNoticeError; }
    // Automation (the Feedback suite): the notice card as laid out, in pixels from the menu's corner, with
    // the book's bounds and the regions it must keep clear of (the tabs and the focused control).
    struct FNoticeLayout
    {
        bool bShowing = false;
        FBox2D Card = FBox2D(ForceInit);
        FBox2D Book = FBox2D(ForceInit);
        FVector2D CardDesired = FVector2D::ZeroVector;
        TArray<TPair<FString, FBox2D>> Protected;
    };
    FNoticeLayout GetNoticeLayout() const;

private:
    enum class ERegion { Tabs, Session, Inventory, Portrait, Content, Equipment, Details, Actions, Recovery, Hotbar };
    enum class EDialog { None, Exit, SaveFailed, GraphicsFailed, Unsaved, Restart, TestReset, Amount, Merge, DropWearable, Context, Quantity, RenameChest };
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
        // A colour chip shown before the label (the dye chooser's swatches).
        TOptional<FLinearColor> Swatch;
    };
    TArray<FPopupOption> PopupOptions;
    FString PopupTitle;
    FVector2D PopupAnchor = FVector2D::ZeroVector;
    bool bKeepPopupAnchor = false;
    void BuildPopup();
    void AdjustQuantity(int32 Delta);
    bool BuildItemOptions(const FHomesteadRow& Row);
    // The dye chooser: a popup of the four dyes over the live book. Moving over a dye shows it on
    // her (a real wardrobe preview; nothing is saved), Apply commits it, Cancel puts hers back.
    bool bDyeChooser = false;
    FHomesteadRow DyeRow;
    uint64 DyeRevision = 0;
    int32 DyeOriginal = 0, DyeChoice = 0, DyePreviewed = INDEX_NONE;
    void OpenDyeChooser(int32 Choice);
    FString RenameDraft;
    HomesteadTextEdit::Typer RenameTyper;
    bool TypeChestNameCharacter(TCHAR Character);
    bool HandleRenameKey(FKey Key);
    void EndDyePreview();
    // Appearance camera input (page 6): held WASD, the right stick and a drag on the view orbit her.
    bool bOrbitLeft = false, bOrbitRight = false, bOrbitUp = false, bOrbitDown = false;
    float OrbitStickX = 0.0f, OrbitStickY = 0.0f;
    bool bOrbitDragging = false;
    FVector2D OrbitDragLast = FVector2D::ZeroVector;
    bool HandleAppearanceKey(FKey Key, EInputEvent Event, float InputAmount);
    void ClearAppearanceOrbit();
    // A second line under the popup title (the walk's distance, time and arrival), wrapped.
    FString PopupBody;
    // The Map tab's walk to the focused place: Town (or its store) and the manor.
    TOptional<Homestead::TravelDestination> MapTravelDestination() const;
    FString MapTravelLine() const;
    void OpenTravelPrompt(Homestead::TravelDestination Destination);
    // The travel prompt is open (Map tab or a road sign); a sign's is centred once the book has a size.
    bool bTravelPrompt = false;
    bool bCenterPopup = false;
    bool bPopupCentered = false;
    void OpenItemContextMenuFor(const FHomesteadRow& Row, FVector2D Anchor);
    // Where a popup opens: at the pointer for mouse input, beside the focused tile otherwise.
    FVector2D PopupAnchorFor(const TSharedPtr<SWidget>& Widget, bool bPointer) const;
    // Shift+click: the whole stack or garment to the other side of an open chest; with no chest
    // open, moves a pack stack between the hotbar row and the rest of the pack, and puts on carried garments.
    void QuickMove(int32 Index);
    void ComputeActions();
    int32 StorageColumns() const;
    int32 SettingsTopCount() const;
    FString PackHint() const;
    int32 AudioEditId = -1;
    // A d-pad/keyboard edit of a sound slider: steps preview live; CommitAudioStep saves it once.
    bool bAudioStepEdit = false;
    void StepAudio(int32 Id, int32 Direction);
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
    // The pack page's hotbar strip (see the public accessors). HotbarSelection is the focused slot;
    // PointerHotbarTarget the slot under a drag; HeldHotbarSlot a slot picked up to move (pointer
    // drag or A); HeldHotbarRow a pack stack waiting for a slot.
    TArray<TSharedPtr<SWidget>> HotbarCells;
    // Where a cell dragged out of the row can land: the rest of her pack, and the open chest.
    TSharedPtr<SWidget> PackDropArea;
    TSharedPtr<SWidget> ChestDropArea;
    int32 HotbarSelection = 0;
    int32 PointerHotbarTarget = INDEX_NONE;
    int32 HeldHotbarSlot = INDEX_NONE;
    FVector2D HotbarDragStart = FVector2D::ZeroVector;
    bool bHotbarPointerDown = false;
    bool bHotbarPointerDragging = false;
    bool bSuppressHotbarClick = false;
    TOptional<FHomesteadRow> HeldHotbarRow;
    // The row as one of the pack grid's rows (Pack page: cells as wide as the grid's, inside its scroll),
    // or as a strip of fixed cells (heading the pack column beside an open chest).
    TSharedRef<SWidget> BuildBookHotbar(bool bGridRow = false);
    bool bHotbarInScroll = false;
    // One hotbar snapshot per frame for the strip's many per-paint attributes.
    FHomesteadHotbarSlot BookHotbarSlot(int32 Slot) const;
    mutable TArray<FHomesteadHotbarSlot> HotbarSnapshotCache;
    mutable uint64 HotbarSnapshotFrame = MAX_uint64;
    int32 HotbarCellAt(FVector2D Position) const;
    FLinearColor HotbarCellColor(int32 Slot) const;
    bool IsHotbarDropTarget(int32 Slot) const;
    // The pack or chest stack a drag or pick-up would put in the hotbar row, if any.
    const FHomesteadRow* HotbarCandidateRow() const;
    void EndHotbarPointerDrag();
    // Right-click (or Y / F) on a used slot: "Clear this slot" / Cancel.
    void OpenHotbarSlotMenu(int32 Slot, bool bPointer);
    void CancelHotbarHolds();
    FString HotbarHint() const;
    // The drag ghost (SHomesteadMenuDragGhost.cpp): what she holds, drawn over the book under the cursor
    // or, on the pad, on the focused cell's corner, so she can't forget what she's carrying.
    struct FDragGhost { FName Icon; int32 Count = 0; FVector2D Position = FVector2D::ZeroVector; bool bOnCell = false; };
    TOptional<FDragGhost> CurrentDragGhost() const;
    TSharedRef<SWidget> BuildDragGhost();
    enum class ECraftInput { None, Pointer, Keyboard, Controller };
    static constexpr float CraftCycleSeconds = 1.2f;
    int32 CraftHoldRecipe = INDEX_NONE;
    float CraftHoldElapsed = 0;
    // The recipe that just finished a cycle, for the square's completion flash.
    int32 CraftFlashRecipe = INDEX_NONE;
    double CraftFlashStart = 0;
    int32 CraftBeat = 0;
    ECraftInput CraftInput = ECraftInput::None;
    // The notice card: a brief, non-focusable card over the book at the bottom centre, or under the
    // tabs when the focused control sits where the card would go.
    TSharedPtr<SOverlay> BookOverlay;
    TSharedPtr<SWidget> NoticeCard;
    SOverlay::FOverlaySlot* NoticeSlot = nullptr;
    FString NoticeText;
    bool bNoticeError = false;
    bool bNoticeTop = false;
    bool bNoticePrimed = false;
    uint32 NoticeSerialSeen = 0;
    double NoticeShownAt = -1000.0;
    void UpdateNotice();
    void PlaceNotice();

    TSharedRef<SWidget> BuildBody();
    // The Map tab (page 7): one focusable map view that takes sticks, triggers and the D-pad.
    TSharedRef<SWidget> BuildMap();
    bool HandleMapKey(FKey Key, EInputEvent Event, float InputAmount);
    TSharedPtr<SHomesteadMapView> MapView;
    TSharedRef<SWidget> BuildDetails();
    TSharedRef<SButton> MakeButton(const FString& Label, TFunction<void()> Action,
        TAttribute<FSlateColor> Color = FSlateColor(FLinearColor(0.025f, 0.05f, 0.038f, 0.6f)),
        const FString& AccessibleLabel = FString(), FMargin Padding = FMargin(14, 10), float FontSize = 17.0f);
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
