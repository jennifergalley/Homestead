#pragma once

// The UI theme trial (Jenny, 2026-09-30 18:57): the parchment notice card and the field book's EB
// Garamond as the look of every menu and HUD surface, against the classic pine-and-cream look.
// `homestead.UITheme parchment|classic` (or -HomesteadUITheme=classic on the command line) picks it;
// parchment is this branch's default until Jenny approves it. Widgets read the theme when they are
// built, so reopen a menu (or restart) after switching.
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
enum class ETheme : uint8 { Classic, Parchment };
ETheme Current();
inline bool IsParchment() { return Current() == ETheme::Parchment; }
void Set(ETheme Theme);
const TCHAR* Name(ETheme Theme);

// The parchment counterpart of a classic UI colour (see above).
FLinearColor ParchmentOf(const FLinearColor& Classic);

// A UI colour that follows the theme: it IS the FLinearColor in use (so code reads .R, returns and
// mixes it like any colour), holding its classic and parchment values. Every one registers itself and
// Apply() rewrites them all in place when the theme is chosen (the controller's BeginPlay, and on
// `homestead.UITheme`). Static initialisation only links the list; nothing reads the theme until Apply.
// Declare them at namespace scope (`inline` in headers), never as locals.
struct FThemeColor : public FLinearColor
{
    FThemeColor(float R, float G, float B, float A = 1.0f) : FThemeColor(FLinearColor(R, G, B, A)) {}
    explicit FThemeColor(const FLinearColor& InClassic);
    FThemeColor(const FLinearColor& InClassic, const FLinearColor& InParchment);
    // A copy (a lambda returning one, say) is a plain snapshot of the current colour: it isn't
    // registered, so it never changes afterwards.
    FThemeColor(const FThemeColor& Other) : FLinearColor(Other), Classic(Other.Classic), Parchment(Other.Parchment) {}
    FThemeColor& operator=(const FThemeColor&) = delete;
    ~FThemeColor();
    void Apply(bool bParchment);
private:
    friend void Apply();
    FLinearColor Classic;
    FLinearColor Parchment;
    FThemeColor* Next = nullptr;
};
// Sets every FThemeColor to the current theme. Widgets built afterwards use it.
void Apply();

// A colour written inline (not a named FThemeColor), for lambdas and widgets built after Apply.
inline FLinearColor Themed(const FLinearColor& Classic) { return IsParchment() ? ParchmentOf(Classic) : Classic; }

// The book's Slate fonts: EB Garamond in parchment, FCoreStyle's default (Typeface) in classic.
FSlateFontInfo Font(FName Typeface, float Size);
FSlateFontInfo Font(FName Typeface, float Size, const FFontOutlineSettings& Outline);
// Key and button glyphs ("1", "[E]", "LB"): the crisp sans in either theme.
FSlateFontInfo KeyFont(FName Typeface, float Size);
}
