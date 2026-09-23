#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
class SHomesteadHotbar : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadHotbar) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    TSharedPtr<SWidget> SlotWidget(int32 Index) const
    {
        return SlotButtons.IsValidIndex(Index) ? SlotButtons[Index] : nullptr;
    }
    FGeometry HotbarGeometry() const { return GetCachedGeometry(); }

private:
    TWeakObjectPtr<AHomesteadController> Controller;
    TArray<TSharedPtr<SWidget>> SlotButtons;
};
}
