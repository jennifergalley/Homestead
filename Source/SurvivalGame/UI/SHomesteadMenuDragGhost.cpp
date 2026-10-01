// The drag ghost (Jenny, 2026-09-30 22:26: "I'd like to see its icon dragging around where my mouse is
// pointing, so I don't forget what I'm holding onto"). While she drags or holds a stack in the pack,
// the chest or the hotbar row, its icon and count ride just below and right of the pointer, so the
// cell she's aiming at stays in view; on the pad it sits on the focused cell's corner instead. It
// draws above everything in the book, takes no input, and goes when the stack is put down or the
// hold is cancelled (it reads the same drag and hold state the cells do).
#include "SHomesteadMenuPrivate.h"
#include "HomesteadUITheme.h"

namespace HomesteadMenus
{
namespace MenuDragGhostStyle
{
constexpr float Size = 56;              // in hand: a little smaller than a 76-unit pack tile
constexpr float CornerSize = 42;        // on the pad, a badge on the focused cell's corner
constexpr float IconInset = 7;
constexpr float Opacity = 0.85f;
const FVector2D CursorOffset(14, 16);   // below-right of the pointer's tip
const FVector2D CornerOffset(-26, -14); // overhanging the focused cell's top-right corner
}

TOptional<SHomesteadMenu::FDragGhost> SHomesteadMenu::CurrentDragGhost() const
{
    if (!Controller.IsValid() || !BookOverlay || Dialog != EDialog::None) return {};
    FDragGhost Ghost;
    bool bPointerDrag = false;
    const FHomesteadRow* Row = nullptr;
    if (bPointerDraggingItem && Entries.IsValidIndex(PointerDragSource)) { Row = &Entries[PointerDragSource]; bPointerDrag = true; }
    else if (bVirtualDraggingItem && Entries.IsValidIndex(VirtualDragSource)) Row = &Entries[VirtualDragSource];
    else if (HeldHotbarRow.IsSet()) Row = &HeldHotbarRow.GetValue();
    if (Row)
    {
        Ghost.Icon = EntryIcon(*Row);
        Ghost.Count = Row->Quantity;
    }
    // A hotbar cell picked up with A / Enter, or being dragged (a press alone is not yet a drag).
    else if (HeldHotbarSlot != INDEX_NONE && (bHotbarPointerDragging || !bHotbarPointerDown))
    {
        const FHomesteadHotbarSlot Slot = BookHotbarSlot(HeldHotbarSlot);
        if (!Slot.Assigned) return {};
        Ghost.Icon = Slot.Icon;
        Ghost.Count = Slot.Food || Slot.Seed || Slot.Material ? Slot.Count : 1;
        bPointerDrag = bHotbarPointerDragging;
    }
    else return {};

    const FGeometry& Book = BookOverlay->GetCachedGeometry();
    if (bPointerDrag || !Controller->UsesGamepad())
    {
        if (!FSlateApplication::IsInitialized()) return {};
        Ghost.Position = Book.AbsoluteToLocal(FSlateApplication::Get().GetCursorPos()) + MenuDragGhostStyle::CursorOffset;
        return Ghost;
    }
    const TSharedPtr<SWidget> Cell = Region == ERegion::Hotbar
        ? (HotbarCells.IsValidIndex(HotbarSelection) ? HotbarCells[HotbarSelection] : nullptr)
        : (Cells.IsValidIndex(ContentSelection) ? Cells[ContentSelection] : nullptr);
    if (!Cell) return {};
    const FGeometry& At = Cell->GetCachedGeometry();
    Ghost.Position = Book.AbsoluteToLocal(At.LocalToAbsolute(FVector2D(At.GetLocalSize().X, 0))) + MenuDragGhostStyle::CornerOffset;
    Ghost.bOnCell = true;
    return Ghost;
}

TSharedRef<SWidget> SHomesteadMenu::BuildDragGhost()
{
    const auto Edge = [this]()
    {
        const auto Ghost = CurrentDragGhost();
        return FOptionalSize(Ghost && Ghost->bOnCell ? MenuDragGhostStyle::CornerSize : MenuDragGhostStyle::Size);
    };
    return SNew(SBox)
        .Visibility_Lambda([this]() { return CurrentDragGhost().IsSet() ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
        .RenderTransform_Lambda([this]() -> TOptional<FSlateRenderTransform>
        {
            const auto Ghost = CurrentDragGhost();
            return FSlateRenderTransform(Ghost ? Ghost->Position : FVector2D::ZeroVector);
        })
        .RenderOpacity(MenuDragGhostStyle::Opacity)
        .WidthOverride_Lambda(Edge)
        .HeightOverride_Lambda(Edge)
        [
            // A pack tile lifted off the page: its thin ink edge and paper fill, the icon and the count.
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(1.5f)
            .BorderBackgroundColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.6f))
            [
                SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).Padding(0)
                .BorderBackgroundColor(Selected)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot().Padding(MenuDragGhostStyle::IconInset)
                    [
                        SNew(SHomesteadIcon).Tint(MenuGold)
                        .Kind_Lambda([this]() { const auto Ghost = CurrentDragGhost(); return Ghost ? Ghost->Icon : FName(); })
                    ]
                    + SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0, 0, 4, 1)
                    [
                        SNew(STextBlock).Font(HomesteadUITheme::Font("Bold", 13)).ColorAndOpacity(Ink)
                        .Text_Lambda([this]()
                        {
                            const auto Ghost = CurrentDragGhost();
                            return Ghost && Ghost->Count > 1 ? FText::AsNumber(Ghost->Count) : FText::GetEmpty();
                        })
                    ]
                ]
            ]
        ];
}
}
