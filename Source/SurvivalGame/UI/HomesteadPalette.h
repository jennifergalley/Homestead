#pragma once

#include "Math/Color.h"

// The shared palette for the HUD, field book, shop, map and icons: pine panels, brass accents and
// cream text. Use these instead of redeclaring the literals in each widget (and renaming them to
// dodge unity-build clashes); widget-specific shades can still live in the widget's own named
// style namespace. Adjust opacity with CopyWithNewOpacity.
namespace HomesteadPalette
{
// Accent: titles, the selected slot, money, the default icon tint.
inline constexpr FLinearColor Brass(0.92f, 0.74f, 0.43f, 1.0f);
// Panel backing (the calendar and vitals).
inline constexpr FLinearColor Pine(0.055f, 0.09f, 0.075f, 1.0f);
// Darker plate behind lists and fields.
inline constexpr FLinearColor DeepPine(0.025f, 0.05f, 0.038f, 1.0f);
// Body text and light glyph strokes.
inline constexpr FLinearColor Cream(0.93f, 0.93f, 0.84f, 1.0f);
// Secondary text.
inline constexpr FLinearColor Sage(0.71f, 0.77f, 0.69f, 1.0f);
// Low vitals, failed actions and debts.
inline constexpr FLinearColor Warning(1.0f, 0.67f, 0.48f, 1.0f);
}
