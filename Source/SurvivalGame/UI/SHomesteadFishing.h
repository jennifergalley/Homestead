#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class AHomesteadController;

namespace HomesteadMenus
{
class SHomesteadFishing : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadFishing) {}
        SLATE_ARGUMENT(TWeakObjectPtr<AHomesteadController>, Controller)
    SLATE_END_ARGS()
    void Construct(const FArguments& Args);
    static FBox2D LogicalBox(float ViewWidth);
    static constexpr float Width = 520.0f;
    static constexpr float Height = 270.0f;
    static constexpr float Bottom = 180.0f;
private:
    TWeakObjectPtr<AHomesteadController> Controller;
};
}
