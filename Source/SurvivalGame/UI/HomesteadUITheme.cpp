#include "HomesteadUITheme.h"

#include "HomesteadNoticeStyle.h"
#include "SHomesteadArrival.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Styling/CoreStyle.h"

namespace HomesteadUIThemeTuning
{
// Paper for the lightest and the darkest classic surfaces (list plates read lighter, selections darker).
// Contrast (WCAG relative luminance): PaperLight ~0.64, PaperDark ~0.48; ink ~0.02, MutedInk ~0.07 and
// Accent ~0.06 keep body text and labels at 4.5:1 or better on both (Orchestrator review, 2026-09-30).
const FLinearColor PaperLight(0.70f, 0.62f, 0.45f, 1.0f);
const FLinearColor PaperDark(0.56f, 0.47f, 0.31f, 1.0f);
// Classic surface luminance mapped across that range (pine panels sit near 0.08).
constexpr float SurfaceDark = 0.03f, SurfaceSpan = 0.12f, SurfaceTop = 0.3f;
// A translucent pine panel becomes near-opaque paper; nearly transparent fills stay faint.
constexpr float PaperMinAlpha = 0.98f, FaintAlpha = 0.3f;
// Text: the brightest becomes full ink, secondary text muted ink.
const FLinearColor MutedInk(0.10f, 0.07f, 0.04f, 1.0f);
const FLinearColor Accent(0.16f, 0.035f, 0.01f, 1.0f);
constexpr float MutedBelow = 0.86f;
// The serif's x-height is small: the same point size reads about this much smaller than the sans.
constexpr float SerifScale = 1.12f;

TAutoConsoleVariable<FString> CVarTheme(TEXT("homestead.UITheme"), TEXT(""),
    TEXT("UI theme: parchment (the game's look: parchment card and EB Garamond everywhere) or classic (old pine and cream, for comparison). ")
    TEXT("Empty: -HomesteadUITheme=<name> from the command line, else parchment. Reopen menus after changing."),
    FConsoleVariableDelegate::CreateLambda([](IConsoleVariable*) { HomesteadUITheme::Apply(); }));

float Luminance(const FLinearColor& Colour) { return 0.2126f * Colour.R + 0.7152f * Colour.G + 0.0722f * Colour.B; }
}

namespace HomesteadUITheme
{
ETheme Current()
{
    FString Value = HomesteadUIThemeTuning::CVarTheme.GetValueOnGameThread();
    if (Value.IsEmpty()) FParse::Value(FCommandLine::Get(), TEXT("HomesteadUITheme="), Value);
    return Value.Equals(TEXT("classic"), ESearchCase::IgnoreCase) ? ETheme::Classic : ETheme::Parchment;
}

void Set(ETheme Theme)
{
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("homestead.UITheme")))
        Variable->Set(Name(Theme), ECVF_SetByConsole);
}

const TCHAR* Name(ETheme Theme) { return Theme == ETheme::Classic ? TEXT("classic") : TEXT("parchment"); }

FLinearColor ParchmentOf(const FLinearColor& Classic)
{
    using namespace HomesteadUIThemeTuning;
    const float L = Luminance(Classic);
    const bool bWarm = Classic.R - Classic.B > 0.3f;
    // Warm, bright warnings (salmon/orange with a high red and middling green) become rust ink.
    if (bWarm && Classic.R > 0.95f && Classic.G < 0.72f) return HomesteadNoticeStyle::RustInk.CopyWithNewOpacity(Classic.A);
    if (L < SurfaceTop)
    {
        const float T = FMath::Clamp((L - SurfaceDark) / SurfaceSpan, 0.0f, 1.0f);
        FLinearColor Paper = FMath::Lerp(PaperLight, PaperDark, T);
        Paper.A = Classic.A < FaintAlpha ? Classic.A : FMath::Max(Classic.A, PaperMinAlpha);
        return Paper;
    }
    if (bWarm) return Accent.CopyWithNewOpacity(Classic.A);
    return (L < MutedBelow ? MutedInk : HomesteadNoticeStyle::InkBrown).CopyWithNewOpacity(Classic.A);
}

namespace
{
FThemeColor*& Registry()
{
    static FThemeColor* Head = nullptr;
    return Head;
}
}

FThemeColor::FThemeColor(const FLinearColor& InClassic) : FThemeColor(InClassic, ParchmentOf(InClassic)) {}

FThemeColor::FThemeColor(const FLinearColor& InClassic, const FLinearColor& InParchment)
    : FLinearColor(InClassic), Classic(InClassic), Parchment(InParchment)
{
    Next = Registry();
    Registry() = this;
}

FThemeColor::~FThemeColor()
{
    for (FThemeColor** Link = &Registry(); *Link; Link = &(*Link)->Next)
        if (*Link == this) { *Link = Next; break; }
}

void FThemeColor::Apply(bool bParchment)
{
    static_cast<FLinearColor&>(*this) = bParchment ? Parchment : Classic;
}

void Apply()
{
    const bool bParchment = IsParchment();
    for (FThemeColor* Colour = Registry(); Colour; Colour = Colour->Next) Colour->Apply(bParchment);
}

FSlateFontInfo Font(FName Typeface, float Size)
{
    if (!IsParchment()) return FCoreStyle::GetDefaultFontStyle(Typeface, FMath::RoundToInt(Size));
    return HomesteadMenus::DisplayFont(Size * HomesteadUIThemeTuning::SerifScale);
}

FSlateFontInfo Font(FName Typeface, float Size, const FFontOutlineSettings& Outline)
{
    FSlateFontInfo Info = Font(Typeface, Size);
    Info.OutlineSettings = Outline;
    return Info;
}

FSlateFontInfo KeyFont(FName Typeface, float Size) { return FCoreStyle::GetDefaultFontStyle(Typeface, FMath::RoundToInt(Size)); }
}
