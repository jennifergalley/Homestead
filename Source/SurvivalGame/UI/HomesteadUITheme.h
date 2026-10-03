#pragma once

// The game's UI theme: the parchment notice card and the field book's EB Garamond on every menu and
// HUD surface (trialled 2026-09-30 18:57, approved by Jenny 21:54). Parchment is the default; every new
// UI surface uses it (colours via FThemeColor/Themed, text via Font(), key glyphs via KeyFont()) and gets
// a UI gallery entry. `homestead.UITheme classic` (or -HomesteadUITheme=classic) keeps the old pine-and-
// cream look for comparison only. Widgets read the theme when they are built, so reopen a menu (or
// restart) after switching to the classic comparison font. Palette changes update immediately.
//
// Colours keep their classic value in code (FThemeColor) and turn into the parchment equivalent on use:
// dark panels become paper (deeper panels lighter, selections darker), light text becomes iron-gall
// ink, brass becomes a deep rust-brown accent, warnings rust ink. Fonts come from Font(): EB Garamond in
// parchment (one weight; a little larger, as the serif's x-height is small), the engine sans otherwise.
// Key and button glyphs keep the sans (KeyFont) so they stay crisp.
#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateColor.h"

namespace HomesteadUITheme
{
// Parchment is the light book; Dark is the same book by candlelight (umber vellum, cream ink, gilt), the
// pitched dark option (pitch-dark-parchment-theme). Both use EB Garamond; Classic is the old pine look.
enum class ETheme : uint8 { Classic, Parchment, Dark };
ETheme Current();
// The serif book family (parchment or dark): fonts and shapes; colours come from the palette.
inline bool IsParchment() { return Current() != ETheme::Classic; }
inline bool IsDark() { return Current() == ETheme::Dark; }
// Sets the theme for this session and remembers it (GameUserSettings) for the next.
void Set(ETheme Theme);
const TCHAR* Name(ETheme Theme);

// The parchment and dark counterparts of a classic UI colour (see above).
FLinearColor ParchmentOf(const FLinearColor& Classic);
FLinearColor DarkOf(const FLinearColor& Classic);

// A UI colour that follows the theme: it IS the FLinearColor in use (so code reads .R, returns and
// mixes it like any colour), holding its classic, parchment and dark values. Every one registers itself and
// Apply() rewrites them all in place when the theme is chosen (the controller's BeginPlay, and on
// `homestead.UITheme`). Static initialisation only links the list; nothing reads the theme until Apply.
// Declare them at namespace scope (`inline` in headers), never as locals.
struct FThemeColor : public FLinearColor
{
    FThemeColor(float R, float G, float B, float A = 1.0f) : FThemeColor(FLinearColor(R, G, B, A)) {}
    explicit FThemeColor(const FLinearColor& InClassic);
    FThemeColor(const FLinearColor& InClassic, const FLinearColor& InParchment);
    FThemeColor(const FLinearColor& InClassic, const FLinearColor& InParchment, const FLinearColor& InDark);
    // A copy (a lambda returning one, say) is a plain snapshot of the current colour: it isn't
    // registered, so it never changes afterwards.
    FThemeColor(const FThemeColor& Other) : FLinearColor(Other), Classic(Other.Classic), Parchment(Other.Parchment), Dark(Other.Dark) {}
    FThemeColor& operator=(const FThemeColor&) = delete;
    ~FThemeColor();
    void Apply(ETheme Theme);
private:
    friend void Apply();
    FLinearColor Classic;
    FLinearColor Parchment;
    FLinearColor Dark;
    FThemeColor* Next = nullptr;
};
// Sets every FThemeColor to the current theme. Widgets built afterwards use it.
void Apply();
// Broadcast by Apply when the theme actually changes, so widgets built in the old palette (the book,
// the hotbar, the vitals) can rebuild. Listeners may be mid-click: defer the rebuild a tick.
FSimpleMulticastDelegate& OnChanged();

// A colour written inline (not a named FThemeColor), for lambdas and widgets built after Apply.
inline FLinearColor Themed(const FLinearColor& Classic)
{
    const ETheme Theme = Current();
    return Theme == ETheme::Dark ? DarkOf(Classic) : Theme == ETheme::Parchment ? ParchmentOf(Classic) : Classic;
}

// The book's Slate fonts: EB Garamond in parchment, FCoreStyle's default (Typeface) in classic.
FSlateFontInfo Font(FName Typeface, float Size);
FSlateFontInfo Font(FName Typeface, float Size, const FFontOutlineSettings& Outline);
// Key and button glyphs ("1", "[E]", "LB"): the crisp sans in either theme.
FSlateFontInfo KeyFont(FName Typeface, float Size);
}
