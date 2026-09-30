// The field book's hotbar editor: she drags a pack stack onto one of the ten slots, or one slot onto
// another. The rules are plain C++ in Simulation/HomesteadHotbarLayout.h (native-tested); this only
// applies them to the controller's bindings and says why when something can't go there.
#include "HomesteadController.h"

#include "Simulation/HomesteadHotbarLayout.h"

namespace HotbarEditor
{
Homestead::HotbarLayout ToLayout(const TArray<int32>& Slots)
{
    Homestead::HotbarLayout Layout;
    Layout.fill(Homestead::HotbarEmpty);
    for (int32 Index = 0; Index < FMath::Min(Slots.Num(), Homestead::HotbarSize); ++Index) Layout[Index] = Slots[Index];
    return Layout;
}

void FromLayout(const Homestead::HotbarLayout& Layout, TArray<int32>& Slots)
{
    Slots.Init(-1, Homestead::HotbarSize);
    for (int32 Index = 0; Index < Homestead::HotbarSize; ++Index) Slots[Index] = Layout[Index];
}
}

bool AHomesteadController::MenuAssignHotbarSlot(const FHomesteadRow& Row, int32 Slot)
{
    if (Row.Subject != EHomesteadMenuSubject::ItemGroup || Row.Id < 0 || Row.Id >= Homestead::ItemCount)
    {
        Notify(TEXT("Only tools, food and seeds can go on the hotbar."), true);
        return false;
    }
    const auto Item = static_cast<Homestead::Item>(Row.Id);
    auto Layout = HotbarEditor::ToLayout(HotbarSlots);
    const auto Edit = Homestead::AssignHotbarSlot(Layout, Item, Slot, Row.ContainerId == 0 && Sim.Count(Item) > 0);
    if (Edit.Refused())
    {
        Notify(UTF8_TO_TCHAR(Edit.message.c_str()), true);
        return false;
    }
    if (Edit.code == Homestead::HotbarEditCode::Unchanged) return true;
    HotbarEditor::FromLayout(Layout, HotbarSlots);
    PlayEffect(UIClick, 0.05f);
    // A slot she filled over something else: say where that went, since it's no longer on show.
    if (Edit.unpinned >= 0 && Edit.unpinned < Homestead::ItemCount)
        Notify(FString::Printf(TEXT("%s is off the hotbar (still in your pack)."),
            UTF8_TO_TCHAR(Homestead::ItemName(static_cast<Homestead::Item>(Edit.unpinned)))));
    return true;
}

bool AHomesteadController::MenuMoveHotbarSlot(int32 From, int32 To)
{
    auto Layout = HotbarEditor::ToLayout(HotbarSlots);
    const auto Edit = Homestead::MoveHotbarSlot(Layout, From, To);
    if (Edit.Refused())
    {
        // Picking up an empty slot is just nothing to move; only a bad slot index is worth saying.
        if (Edit.code != Homestead::HotbarEditCode::EmptySource) Notify(UTF8_TO_TCHAR(Edit.message.c_str()), true);
        return false;
    }
    if (Edit.code == Homestead::HotbarEditCode::Changed)
    {
        HotbarEditor::FromLayout(Layout, HotbarSlots);
        PlayEffect(UIClick, 0.05f);
    }
    return true;
}
