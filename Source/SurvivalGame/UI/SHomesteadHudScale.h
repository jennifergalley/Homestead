#pragma once

#include "CoreMinimal.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"

namespace HomesteadMenus
{
// The Canvas HUD's rule (AHomesteadHUD::UiScale): 1080 logical lines, clamped for tiny and very
// large windows. Physical pixels per logical HUD unit.
inline float HudCanvasScale(float PhysicalHeight) { return FMath::Clamp(PhysicalHeight / 1080.0f, 0.4f, 1.5f); }

// Scales its content so one Slate unit inside is one Canvas HUD logical unit on screen, whatever the
// window size and Slate's own DPI scale. Slate HUD pieces (hotbar, vitals) wrap themselves in it so
// they stay in proportion with the Canvas calendar, hints and minimap at every resolution.
class SHomesteadHudScale : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SHomesteadHudScale) {}
        SLATE_DEFAULT_SLOT(FArguments, Content)
    SLATE_END_ARGS()

    void Construct(const FArguments& Args);
    virtual void Tick(const FGeometry& AllottedGeometry, const double CurrentTime, const float DeltaTime) override;

private:
    float Scale = 1.0f;
};
}
