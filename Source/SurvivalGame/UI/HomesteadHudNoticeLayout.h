#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

namespace HudNoticeLayout
{
// HUD logical units: below the compass, left of the calendar/vitals, clear of the hotbar.
inline constexpr float Top = 110, Gap = 10, SideMargin = 30, ColumnGap = 16;
inline constexpr float TextSize = 23, LineStep = 30, PadX = 22, PadTop = 13, PadBottom = 13;
inline constexpr float MinWidth = 240, MaxWidth = 900;
inline constexpr float ChestTitleSize = 32, ChestMinScreenSize = 20;
}

namespace HudNoticeFont
{
FSlateFontInfo At(float Size, float UiScale);
FVector2D Measure(const FString& Text, float Size, float UiScale);
}
