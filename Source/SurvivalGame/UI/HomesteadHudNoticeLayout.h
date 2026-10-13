#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

namespace HudNoticeLayout
{
// HUD logical units: below the compass, left of the calendar/vitals, clear of the hotbar.
inline constexpr float Top = 110, Gap = 10, SideMargin = 30, ColumnGap = 16;
inline constexpr float TextSize = 23, LineStep = 30, PadX = 22, PadTop = 13, PadBottom = 13;
inline constexpr float MinWidth = 240, MaxWidth = 900;
// The smallest the focus card's words (title and verbs, one size) are ever drawn on screen, in font
// size units (about pixels); above the toast's floor so the card stays easy to read at 720p.
inline constexpr float FocusCardMinScreenSize = 18;
}

namespace HudNoticeFont
{
// The smallest the serif is ever drawn on screen (font size units, about pixels): 720p scales the HUD to
// two thirds, which left the focus title near 9 and the toast near 12 (Orchestrator review, 2026-09-30).
// Text above it keeps its proportion at 1080p and 4K.
inline constexpr float DefaultMinScreenSize = 15.0f;
FSlateFontInfo At(float Size, float UiScale, float MinScreenSize = DefaultMinScreenSize);
FVector2D Measure(const FString& Text, float Size, float UiScale, float MinScreenSize = DefaultMinScreenSize);
}
