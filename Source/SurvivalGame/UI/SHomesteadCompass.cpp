#include "SHomesteadCompass.h"

#include "../HomesteadMapComponent.h"
#include "HomesteadMapPainter.h"

#include <vector>

namespace HomesteadMenus
{
// The band's look, in Canvas HUD units unless a name says physical pixels. The half-field is how
// many degrees either side of the camera the band shows; the floors keep the letters and tokens
// legible at 720p (0.667 px per unit) while 1080p and 4K draw them at their HUD size.
namespace CompassStyle
{
constexpr double HalfFieldDegrees = 90.0;
constexpr float BandHeight = 40.0f, EndRadius = 20.0f;
constexpr float CardinalSize = 17.0f, CardinalMinPx = 12.0f;
constexpr float OrdinalSize = 11.5f, OrdinalMinPx = 9.0f;
constexpr float TickLength = 6.0f, TickWidth = 1.4f;
constexpr float TokenRadius = 10.5f, TokenMinPx = 9.0f, TokenDrop = 15.0f, TokenGap = 2.0f;
constexpr float CaretSize = 7.0f;
// Edge fade: marks start fading at this fraction of the half-width and are gone at the end.
constexpr float FadeStart = 0.72f;
// A landmark she's standing at has no useful bearing (cm).
constexpr double NearbyCm = 1500.0;
const FLinearColor Band(0.035f, 0.055f, 0.046f, 0.9f);
const FLinearColor Shadow(0.02f, 0.03f, 0.025f, 0.55f);
const FLinearColor Cream(0.95f, 0.91f, 0.78f, 1.0f);
const FLinearColor Ordinal(0.74f, 0.72f, 0.60f, 1.0f);
const FLinearColor North(0.93f, 0.44f, 0.30f, 1.0f);

FLinearColor Faded(FLinearColor Color, float Alpha) { Color.A *= Alpha; return Color; }
}

void SHomesteadCompass::Construct(const FArguments& Args)
{
    Map = Args._Map;
    SetVisibility(EVisibility::HitTestInvisible);
}

int32 SHomesteadCompass::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
    FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const
{
    using namespace CompassStyle;
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->IsCompassVisible()) return LayerId;
    const FHomesteadMapFrame& Frame = Component->Frame();
    if (!Frame.Model.IsValid()) return LayerId;

    const float Scale = FMath::Max(0.01f, Geometry.Scale);
    const FVector2D Physical = Geometry.GetLocalSize() * Scale;
    if (Physical.X < 1 || Physical.Y < 1) return LayerId;
    // The Canvas HUD's rule: 1080 logical lines, clamped for tiny and very large windows.
    const float UiScale = FMath::Clamp(static_cast<float>(Physical.Y) / 1080.0f, 0.4f, 1.5f);
    const FBox2D Box = UHomesteadMapComponent::CompassBox(Physical.X / UiScale, Physical.Y / UiScale);
    if (!Box.bIsValid) return LayerId;
    const float U = UiScale / Scale;
    const double PhysicalPerLocal = Scale;
    const auto Legible = [PhysicalPerLocal](float Local, float MinPhysical)
        { return static_cast<float>(HomesteadMap::AtLeastPhysical(Local, PhysicalPerLocal, MinPhysical)); };
    const auto Snap = [PhysicalPerLocal](FVector2D Point)
    {
        return FVector2D(HomesteadMap::SnapToPixel(Point.X, PhysicalPerLocal), HomesteadMap::SnapToPixel(Point.Y, PhysicalPerLocal));
    };

    const float Left = Box.Min.X * U, Right = Box.Max.X * U, Top = Box.Min.Y * U;
    const float Height = BandHeight * U, Radius = EndRadius * U;
    const float MidX = (Left + Right) * 0.5f;
    const float HalfWidth = (Right - Left) * 0.5f - Radius * 0.6f;
    const float Bottom = Top + Height;
    const auto EdgeAlpha = [HalfWidth](double Offset)
    {
        const float T = static_cast<float>(FMath::Abs(Offset) / FMath::Max(1.0f, HalfWidth));
        return 1.0f - FMath::SmoothStep(FadeStart, 1.0f, T);
    };

    HomesteadMapPaint::FPainter Paint(Geometry, Out, LayerId);
    // A pill of pine with a soft drop shadow and brass rules along its long edges.
    const auto Pill = [&](FVector2D Offset, float Grow)
    {
        TArray<FVector2D> Points;
        constexpr int32 Steps = 12;
        const float R = Radius + Grow;
        const float CY = Top + Height * 0.5f;
        for (int32 Step = 0; Step <= Steps; ++Step)
        {
            const float Angle = -0.5f * PI + Step * PI / Steps;
            Points.Add(Offset + FVector2D(Right - Radius + FMath::Cos(Angle) * R, CY + FMath::Sin(Angle) * (Height * 0.5f + Grow)));
        }
        for (int32 Step = 0; Step <= Steps; ++Step)
        {
            const float Angle = 0.5f * PI + Step * PI / Steps;
            Points.Add(Offset + FVector2D(Left + Radius + FMath::Cos(Angle) * R, CY + FMath::Sin(Angle) * (Height * 0.5f + Grow)));
        }
        return Points;
    };
    Paint.Fill(Pill(FVector2D(0, 3 * U), 2 * U), Shadow);
    Paint.Fill(Pill(FVector2D::ZeroVector, 0), Band);
    const float RuleInset = Radius * 0.55f;
    const float RuleWidth = FMath::Max(1.0f / Scale, 1.3f * U);
    Paint.Segment(Snap(FVector2D(Left + RuleInset, Top + 3 * U)), Snap(FVector2D(Right - RuleInset, Top + 3 * U)),
        Faded(HomesteadMapPaint::Brass, 0.7f), RuleWidth);
    Paint.Segment(Snap(FVector2D(Left + RuleInset, Bottom - 3 * U)), Snap(FVector2D(Right - RuleInset, Bottom - 3 * U)),
        Faded(HomesteadMapPaint::Brass, 0.7f), RuleWidth);

    // Ticks every 15 degrees; the eight winds are lettered instead.
    const double Heading = Frame.CameraYaw;
    static const TCHAR* const Winds[] = {TEXT("N"), TEXT("NE"), TEXT("E"), TEXT("SE"), TEXT("S"), TEXT("SW"), TEXT("W"), TEXT("NW")};
    const float CardinalFont = Legible(CardinalSize * U, CardinalMinPx);
    const float OrdinalFont = Legible(OrdinalSize * U, OrdinalMinPx);
    for (int32 Degrees = 0; Degrees < 360; Degrees += 15)
    {
        double Offset = 0.0;
        if (!HomesteadMap::CompassOffset(Degrees, Heading, HalfFieldDegrees, HalfWidth, Offset)) continue;
        const float Alpha = EdgeAlpha(Offset);
        if (Alpha <= 0.01f) continue;
        const float X = MidX + static_cast<float>(Offset);
        if (Degrees % 45 == 0)
        {
            const int32 Wind = Degrees / 45;
            const bool bCardinal = Wind % 2 == 0;
            const float Size = bCardinal ? CardinalFont : OrdinalFont;
            const FLinearColor Color = Wind == 0 ? North : bCardinal ? Cream : Ordinal;
            const FVector2D Extent = HomesteadMapPaint::FPainter::MeasureText(Winds[Wind], Size);
            Paint.Text(Winds[Wind], Snap(FVector2D(X, Top + Height * 0.5f) - Extent * 0.5f), Size, Faded(Color, Alpha), true, bCardinal);
            continue;
        }
        const float Length = TickLength * U;
        const FVector2D Foot = Snap(FVector2D(X, Bottom - 5 * U));
        Paint.Segment(Foot, Foot - FVector2D(0, Length), Faded(Ordinal, 0.75f * Alpha), FMath::Max(1.0f / Scale, TickWidth * U));
        const FVector2D Head = Snap(FVector2D(X, Top + 5 * U));
        Paint.Segment(Head, Head + FVector2D(0, Length * 0.6f), Faded(Ordinal, 0.45f * Alpha), FMath::Max(1.0f / Scale, TickWidth * U));
    }

    // Landmarks hang beneath the band at their bearing, nearest first; one that would cover a
    // nearer one waits its turn.
    struct FToken { const FHomesteadMapLandmark* Place; double Offset; double Distance; };
    TArray<FToken> Tokens;
    for (const FHomesteadMapLandmark& Place : Frame.Model->Landmarks)
    {
        const double Distance = FMath::Sqrt(FMath::Square(Place.Position.x - Frame.Player.x) + FMath::Square(Place.Position.y - Frame.Player.y));
        if (Distance < NearbyCm) continue;
        double Offset = 0.0;
        if (!HomesteadMap::CompassOffset(HomesteadMap::BearingDegrees(Frame.Player, Place.Position), Heading, HalfFieldDegrees, HalfWidth, Offset)) continue;
        Tokens.Add({&Place, Offset, Distance});
    }
    Tokens.Sort([](const FToken& A, const FToken& B) { return A.Distance < B.Distance; });
    const float TokenR = Legible(TokenRadius * U, TokenMinPx);
    const float TokenY = Bottom + TokenDrop * U;
    std::vector<HomesteadMap::Vec> Centers;
    std::vector<double> Radii;
    for (const FToken& Token : Tokens)
    {
        Centers.push_back({MidX + Token.Offset, TokenY});
        Radii.push_back(TokenR);
    }
    const std::vector<int> Kept = HomesteadMap::SpacedCircles(Centers, Radii, TokenGap * U);
    // Farther tokens draw first so the nearest sits on top.
    for (int32 Index = static_cast<int32>(Kept.size()) - 1; Index >= 0; --Index)
    {
        const FToken& Token = Tokens[Kept[static_cast<size_t>(Index)]];
        const float Alpha = EdgeAlpha(Token.Offset);
        if (Alpha <= 0.01f) continue;
        const FVector2D At = Snap(FVector2D(MidX + Token.Offset, TokenY));
        // A short brass stem ties the token to its bearing on the band.
        Paint.Segment(Snap(FVector2D(At.X, Bottom - 2 * U)), At - FVector2D(0, TokenR),
            Faded(HomesteadMapPaint::Brass, 0.8f * Alpha), FMath::Max(1.0f / Scale, 1.4f * U));
        Paint.Badge(Token.Place->Glyph, At, TokenR, Alpha);
    }

    // The lubber line: where the camera looks, a brass caret at each edge of the band.
    const float Caret = FMath::Max(CaretSize * U, 5.0f / Scale);
    const FVector2D TopMid = Snap(FVector2D(MidX, Top));
    const FVector2D BottomMid = Snap(FVector2D(MidX, Bottom));
    Paint.Fill({TopMid + FVector2D(-Caret, -1 * U), TopMid + FVector2D(Caret, -1 * U), TopMid + FVector2D(0, Caret)}, HomesteadMapPaint::Brass);
    Paint.Fill({BottomMid + FVector2D(-Caret, 1 * U), BottomMid + FVector2D(0, -Caret), BottomMid + FVector2D(Caret, 1 * U)}, HomesteadMapPaint::Brass);
    return Paint.GetLayer();
}
}
