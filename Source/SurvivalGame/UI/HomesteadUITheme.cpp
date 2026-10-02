#include "HomesteadUITheme.h"

#include "HomesteadNoticeStyle.h"
#include "SHomesteadArrival.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
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

// Dark (the candlelit book, pitch-dark-parchment-theme): umber vellum panels, cream ink, a muted gilt
// accent and terracotta for warnings. Values are linear; the sRGB tokens and their contrast ratios
// (all 4.5:1 or better on the panels they sit on) are in openspec/changes/pitch-dark-parchment-theme.
const FLinearColor UmberDeep(0.0232f, 0.0137f, 0.0086f, 1.0f);   // #2A1F17 page and panels
const FLinearColor UmberRaised(0.0685f, 0.0423f, 0.0252f, 1.0f); // #4A3A2C slots and selected rows
const FLinearColor Gilt(0.6584f, 0.4020f, 0.1221f, 1.0f);        // #D4AA62 accent
const FLinearColor Cream(0.8632f, 0.7605f, 0.5647f, 1.0f);       // #EFE2C6 body text
const FLinearColor MutedCream(0.5776f, 0.4678f, 0.3050f, 1.0f);  // #C8B696 secondary text
const FLinearColor Terracotta(0.8070f, 0.2789f, 0.1590f, 1.0f);  // #E8906F warnings
const FLinearColor DarkOnAccent(0.0176f, 0.0097f, 0.0048f, 1.0f); // #24190F text on gilt

TAutoConsoleVariable<FString> CVarTheme(TEXT("homestead.UITheme"), TEXT(""),
    TEXT("UI theme: parchment (the light book), dark (the candlelit book) or classic (old pine and cream, for comparison). ")
    TEXT("Empty: -HomesteadUITheme=<name> from the command line, else Settings' choice, else parchment. Reopen menus after changing."),
    FConsoleVariableDelegate::CreateLambda([](IConsoleVariable*) { HomesteadUITheme::Apply(); }));

constexpr const TCHAR* ThemeSection = TEXT("/Script/SurvivalGame.HomesteadUITheme");
constexpr const TCHAR* ThemeKey = TEXT("Theme");

float Luminance(const FLinearColor& Colour) { return 0.2126f * Colour.R + 0.7152f * Colour.G + 0.0722f * Colour.B; }

HomesteadUITheme::ETheme Parse(const FString& Value)
{
    using HomesteadUITheme::ETheme;
    if (Value.Equals(TEXT("classic"), ESearchCase::IgnoreCase)) return ETheme::Classic;
    if (Value.Equals(TEXT("dark"), ESearchCase::IgnoreCase)) return ETheme::Dark;
    return ETheme::Parchment;
}

// Settings' choice, read once on first use (never during static initialisation).
FString& SavedTheme()
{
    static FString Saved = []()
    {
        FString Value;
        if (GConfig) GConfig->GetString(ThemeSection, ThemeKey, Value, GGameUserSettingsIni);
        return Value;
    }();
    return Saved;
}
}

namespace HomesteadUITheme
{
ETheme Current()
{
    FString Value = HomesteadUIThemeTuning::CVarTheme.GetValueOnGameThread();
    // Read once, on first use (read every frame by the HUD's cards).
    static const FString CommandLine = []() { FString Found; FParse::Value(FCommandLine::Get(), TEXT("HomesteadUITheme="), Found); return Found; }();
    if (Value.IsEmpty()) Value = CommandLine;
    if (Value.IsEmpty()) Value = HomesteadUIThemeTuning::SavedTheme();
    return HomesteadUIThemeTuning::Parse(Value);
}

void Set(ETheme Theme)
{
    HomesteadUIThemeTuning::SavedTheme() = Name(Theme);
    if (GConfig)
    {
        GConfig->SetString(HomesteadUIThemeTuning::ThemeSection, HomesteadUIThemeTuning::ThemeKey, Name(Theme), GGameUserSettingsIni);
        GConfig->Flush(false, GGameUserSettingsIni);
    }
    if (IConsoleVariable* Variable = IConsoleManager::Get().FindConsoleVariable(TEXT("homestead.UITheme")))
        Variable->Set(Name(Theme), ECVF_SetByConsole);
}

const TCHAR* Name(ETheme Theme)
{ return Theme == ETheme::Classic ? TEXT("classic") : Theme == ETheme::Dark ? TEXT("dark") : TEXT("parchment"); }

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

FLinearColor DarkOf(const FLinearColor& Classic)
{
    using namespace HomesteadUIThemeTuning;
    const float L = Luminance(Classic);
    const bool bWarm = Classic.R - Classic.B > 0.3f;
    if (bWarm && Classic.R > 0.95f && Classic.G < 0.72f) return Terracotta.CopyWithNewOpacity(Classic.A);
    if (L < SurfaceTop)
    {
        // The classic order is kept: deeper panels stay deepest, raised fills (selections, slots) lift.
        // The darkest classic tone (pine ink on brass) is the text on the gilt accent.
        if (L < 0.05f && Classic.A >= 0.99f) return DarkOnAccent;
        const float T = FMath::Clamp((L - SurfaceDark) / SurfaceSpan, 0.0f, 1.0f);
        FLinearColor Panel = FMath::Lerp(UmberDeep, UmberRaised, T);
        Panel.A = Classic.A < FaintAlpha ? Classic.A : FMath::Max(Classic.A, PaperMinAlpha);
        return Panel;
    }
    if (bWarm) return Gilt.CopyWithNewOpacity(Classic.A);
    // Saturated colours (a green gain, say) keep their meaning.
    if (FMath::Max3(Classic.R, Classic.G, Classic.B) - FMath::Min3(Classic.R, Classic.G, Classic.B) > 0.25f) return Classic;
    return (L < MutedBelow ? MutedCream : Cream).CopyWithNewOpacity(Classic.A);
}

namespace
{
FThemeColor*& Registry()
{
    static FThemeColor* Head = nullptr;
    return Head;
}
}

FThemeColor::FThemeColor(const FLinearColor& InClassic) : FThemeColor(InClassic, ParchmentOf(InClassic), DarkOf(InClassic)) {}

FThemeColor::FThemeColor(const FLinearColor& InClassic, const FLinearColor& InParchment)
    : FThemeColor(InClassic, InParchment, DarkOf(InClassic)) {}

FThemeColor::FThemeColor(const FLinearColor& InClassic, const FLinearColor& InParchment, const FLinearColor& InDark)
    : FLinearColor(InClassic), Classic(InClassic), Parchment(InParchment), Dark(InDark)
{
    Next = Registry();
    Registry() = this;
}

FThemeColor::~FThemeColor()
{
    for (FThemeColor** Link = &Registry(); *Link; Link = &(*Link)->Next)
        if (*Link == this) { *Link = Next; break; }
}

void FThemeColor::Apply(ETheme Theme)
{
    static_cast<FLinearColor&>(*this) = Theme == ETheme::Dark ? Dark : Theme == ETheme::Parchment ? Parchment : Classic;
}

void Apply()
{
    const ETheme Theme = Current();
    for (FThemeColor* Colour = Registry(); Colour; Colour = Colour->Next) Colour->Apply(Theme);
    static TOptional<ETheme> Applied;
    const bool bChanged = Applied.IsSet() && Applied.GetValue() != Theme;
    Applied = Theme;
    if (bChanged) OnChanged().Broadcast();
}

FSimpleMulticastDelegate& OnChanged()
{
    static FSimpleMulticastDelegate Changed;
    return Changed;
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
