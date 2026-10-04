#pragma once

#include "Math/Color.h"
#include "HomesteadUITheme.h"

// The shared palette for the HUD, field book, shop, map and icons: pine panels, brass accents and
// cream text. Use these instead of redeclaring the literals in each widget (and renaming them to
// dodge unity-build clashes); widget-specific shades can still live in the widget's own named
// style namespace. Adjust opacity with CopyWithNewOpacity.
namespace HomesteadPalette
{
// Accent: titles, the selected slot, money, the default icon tint.
inline HomesteadUITheme::FThemeColor Brass(0.92f, 0.74f, 0.43f, 1.0f);
// Panel backing (the calendar and vitals).
inline HomesteadUITheme::FThemeColor Pine(0.055f, 0.09f, 0.075f, 1.0f);
// Darker plate behind lists and fields.
inline HomesteadUITheme::FThemeColor DeepPine(0.025f, 0.05f, 0.038f, 1.0f);
// Body text and light glyph strokes.
inline HomesteadUITheme::FThemeColor Cream(0.93f, 0.93f, 0.84f, 1.0f);
// Secondary text.
inline HomesteadUITheme::FThemeColor Sage(0.71f, 0.77f, 0.69f, 1.0f);
// Low vitals, failed actions and debts.
inline HomesteadUITheme::FThemeColor Warning(1.0f, 0.67f, 0.48f, 1.0f);

// The shared ornate frame (UI/HomesteadFrameStyle.h, SHomesteadFrame): classic brass on pine, iron-gall
// rules on parchment, dim gilt in the dark book (values match HomesteadNoticeStyle's notice card).
inline HomesteadUITheme::FThemeColor FrameOuter(FLinearColor(0.92f, 0.74f, 0.43f, 0.8f),
    FLinearColor(0.2f, 0.12f, 0.05f, 1.0f), FLinearColor(0.3325f, 0.1946f, 0.0595f, 1.0f));
inline HomesteadUITheme::FThemeColor FrameInner(FLinearColor(0.92f, 0.74f, 0.43f, 0.4f),
    FLinearColor(0.03f, 0.022f, 0.014f, 0.45f), FLinearColor(0.6584f, 0.4020f, 0.1221f, 0.45f));
inline HomesteadUITheme::FThemeColor FrameFlourish(FLinearColor(0.92f, 0.74f, 0.43f, 0.9f),
    FLinearColor(0.2f, 0.12f, 0.05f, 1.0f), FLinearColor(0.6584f, 0.4020f, 0.1221f, 1.0f));
inline HomesteadUITheme::FThemeColor FrameGrain(FLinearColor(0.93f, 0.93f, 0.84f, 1.0f),
    FLinearColor(0.03f, 0.022f, 0.014f, 1.0f), FLinearColor(0.8632f, 0.7605f, 0.5647f, 1.0f));

// The book's cell borders (HomesteadFrameStyle::ForEachCellRect): quieter than the frame, so a screen of
// cells reads as inked boxes, with the corner brackets a shade stronger than the rule between them.
inline HomesteadUITheme::FThemeColor CellRule(FLinearColor(0.92f, 0.74f, 0.43f, 0.35f),
    FLinearColor(0.2f, 0.12f, 0.05f, 0.55f), FLinearColor(0.6584f, 0.4020f, 0.1221f, 0.4f));
inline HomesteadUITheme::FThemeColor CellInner(FLinearColor(0.92f, 0.74f, 0.43f, 0.15f),
    FLinearColor(0.2f, 0.12f, 0.05f, 0.22f), FLinearColor(0.6584f, 0.4020f, 0.1221f, 0.2f));
inline HomesteadUITheme::FThemeColor CellBracket(FLinearColor(0.92f, 0.74f, 0.43f, 0.8f),
    FLinearColor(0.2f, 0.12f, 0.05f, 0.9f), FLinearColor(0.6584f, 0.4020f, 0.1221f, 0.85f));

// An ink keycap (SHomesteadKeycap): a paper cap with an ink rim, its lower edge a shade deeper for a
// slight bevel, in every theme; the dark book's cap is umber vellum with cream ink.
inline HomesteadUITheme::FThemeColor KeycapPaper(FLinearColor(0.62f, 0.54f, 0.38f, 1.0f),
    FLinearColor(0.62f, 0.54f, 0.38f, 1.0f), FLinearColor(0.0423f, 0.0252f, 0.0152f, 1.0f));
inline HomesteadUITheme::FThemeColor KeycapInk(FLinearColor(0.03f, 0.022f, 0.014f, 1.0f),
    FLinearColor(0.03f, 0.022f, 0.014f, 1.0f), FLinearColor(0.8632f, 0.7605f, 0.5647f, 1.0f));
inline HomesteadUITheme::FThemeColor KeycapBevel(FLinearColor(0.2f, 0.12f, 0.05f, 1.0f),
    FLinearColor(0.2f, 0.12f, 0.05f, 1.0f), FLinearColor(0.3325f, 0.1946f, 0.0595f, 1.0f));
}
