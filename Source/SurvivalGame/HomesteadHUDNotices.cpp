#include "HomesteadHUD.h"
#include "HomesteadController.h"
#include "Simulation/HomesteadItems.h"
#include "UI/HomesteadHudNoticeLayout.h"
#include "UI/HomesteadPickupTiming.h"
#include "UI/SHomesteadVitals.h"

namespace HudNoticeGeometry
{
constexpr float CalendarWidth = 460;

float Right(float ViewWidth, const AHomesteadController& PC)
{
    const float CalendarLeft = ViewWidth - HudNoticeLayout::SideMargin - CalendarWidth;
    const float VitalsLeft = HomesteadMenus::SHomesteadVitals::LogicalBox(ViewWidth, PC).Min.X;
    return FMath::Min(CalendarLeft, VitalsLeft) - HudNoticeLayout::ColumnGap;
}
}

float AHomesteadHUD::NoticeMaxWidth(const AHomesteadController& PC) const
{
    const float Right = HudNoticeGeometry::Right(ViewWidth, PC);
    return FMath::Min(HudNoticeLayout::MaxWidth, FMath::Max(80.0f, Right - HudNoticeLayout::SideMargin));
}

float AHomesteadHUD::NoticeLeft(float Width, const AHomesteadController& PC) const
{
    const float Right = HudNoticeGeometry::Right(ViewWidth, PC);
    return FMath::Max(HudNoticeLayout::SideMargin, FMath::Min((ViewWidth - Width) * 0.5f, Right - Width));
}

float AHomesteadHUD::DrawNoticeLines(const AHomesteadController& PC, const TArray<TPair<FString, float>>& Sources,
    float Top, bool bError, bool bToast)
{
    using namespace HudNoticeLayout;
    bNoticeText = true;
    float Width = NoticeMaxWidth(PC);
    TArray<TPair<FString, float>> Lines;
    float Longest = 0, Alpha = 0;
    for (const auto& Source : Sources)
    {
        for (const FString& Line : WrappedLines(Source.Key, Width - PadX * 2, TextSize))
        {
            Lines.Emplace(Line, Source.Value);
            Longest = FMath::Max(Longest, TextWidth(Line, TextSize));
        }
        Alpha = FMath::Max(Alpha, Source.Value);
    }
    if (Lines.IsEmpty())
    {
        bNoticeText = bThemeSerif;
        return Top;
    }
    Width = FMath::Clamp(Longest + PadX * 2 + 2, FMath::Min(MinWidth, Width), Width);
    const float X = PC.IsBookOpen() ? SideMargin : NoticeLeft(Width, PC);
    const float LineHeight = FMath::Max(TextSize, static_cast<float>(HudNoticeFont::Measure(TEXT("Ag"), TextSize, UiScale).Y));
    const float Step = FMath::Max(LineStep, LineHeight);
    const float Height = PadTop + PadBottom + LineHeight + (Lines.Num() - 1) * Step;
    bDrawingToast = bToast;
    if (bToast && bMeasureFeedback)
        ToastBounds = FBox2D(FVector2D(X, Top) * UiScale, FVector2D(X + Width, Top + Height) * UiScale);
    if (!bToast) ProtectFeedback(TEXT("pickup-gains"), X, Top, Width, Height);
    NoticeCard(X, Top, Width, Height, bError
        ? HomesteadNoticeStyle::ESurface::WorldNoticeError : HomesteadNoticeStyle::ESurface::WorldNotice, Alpha);
    const FLinearColor TextInk = bError ? HomesteadNoticeStyle::CardRust() : HomesteadNoticeStyle::CardInk();
    for (int32 Index = 0; Index < Lines.Num(); ++Index)
        Write(Lines[Index].Key, X + FMath::Max(PadX, (Width - TextWidth(Lines[Index].Key, TextSize)) * 0.5f),
            Top + PadTop + Index * Step, TextSize, TextInk.CopyWithNewOpacity(Lines[Index].Value));
    bDrawingToast = false;
    bNoticeText = bThemeSerif;
    return Top + Height;
}

void AHomesteadHUD::DrawWorldNotices(const AHomesteadController& PC, float FocusBottom)
{
    float Top = PC.IsBookOpen() ? HomesteadHudLayout::CalendarTop
        : FocusBottom > 0 ? FocusBottom + HudNoticeLayout::Gap : HudNoticeLayout::Top;
    const FString Toast = PC.Toast();
    if (!Toast.IsEmpty())
    {
        if (bMeasureFeedback) ToastSource = Toast;
        TArray<TPair<FString, float>> Lines;
        Lines.Emplace(Toast, 1.0f);
        Top = DrawNoticeLines(PC, Lines, Top, PC.ToastIsError(), true) + HudNoticeLayout::Gap;
    }
    if (!PC.PickupsVisible()) return;
    TArray<TPair<FString, float>> Gains;
    for (int32 Index = PC.RecentPickups().Num() - 1; Index >= 0; --Index)
    {
        const auto& Gain = PC.RecentPickups()[Index];
        const float In = FMath::Clamp(Gain.Shown / HomesteadPickupTiming::FadeIn, 0.0f, 1.0f);
        const float Out = FMath::Clamp((HomesteadPickupTiming::Seconds - Gain.Shown) / HomesteadPickupTiming::FadeOut, 0.0f, 1.0f);
        if (In * Out > 0.0f)
            Gains.Emplace(FString::Printf(TEXT("+%d %s"), Gain.Amount, UTF8_TO_TCHAR(Homestead::ItemName(Gain.Item))), In * Out);
    }
    if (!Gains.IsEmpty()) DrawNoticeLines(PC, Gains, Top, false, false);
}
