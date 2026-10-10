#include "SHomesteadFishingRings.h"
#include "../HomesteadController.h"
#include "../HomesteadCharacter.h"
#include "../HomesteadFishingPresentation.h"
#include "../Simulation/HomesteadFishing.h"

#include "Rendering/DrawElements.h"

namespace HomesteadMenus
{
namespace FishingRingsStyle
{
// The water and the cue keep fixed colours in every theme: they're part of the scene, not paper.
const FLinearColor RippleColour(0.93f, 0.95f, 0.92f);
const FLinearColor Cue(1.0f, 0.8f, 0.42f);
const FLinearColor Shadow(0.01f, 0.025f, 0.03f);
constexpr int32 Segments = 48;
// World sizes (cm) and timings (s). One ring closes from ClosingOuter onto the float (ClosingInner) as the
// click window (Homestead::Fishing::HookWindowSeconds / StrikeWindowSeconds) runs out (Jenny, 2026-10-09:
// one ring, not a stack).
constexpr float ClosingInner = 10.0f, ClosingOuter = 85.0f;
constexpr float LineWidth = 2.5f, ShadowWidth = 5.0f;
// The only other ring: one faint ripple spreading from a false nibble (reach cm, life s, alpha).
constexpr float NibbleReach = 45.0f, NibbleLife = 0.8f, NibbleAlpha = 0.45f;
// Rings sit just above the water so they don't flicker into it.
constexpr float WaterLiftCm = 0.5f;
}

void SHomesteadFishingRings::Construct(const FArguments& Args)
{
    Controller = Args._Controller;
}

void SHomesteadFishingRings::Spawn(double Now, const FVector& At, float Reach, float Life, float Alpha) const
{
    FRipple Entry;
    Entry.Start = Now;
    Entry.At = At;
    Entry.Reach = Reach;
    Entry.Life = Life;
    Entry.Alpha = Alpha;
    Ripples.Add(Entry);
}

int32 SHomesteadFishingRings::OnPaint(const FPaintArgs& Args, const FGeometry& Geometry, const FSlateRect&,
    FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& Style, bool) const
{
    using namespace FishingRingsStyle;
    AHomesteadController* PC = Controller.Get();
    if (!PC) return Layer;
    const double Now = Args.GetCurrentTime();
    const auto& Session = PC->Simulation().FishingCast();
    const bool bFishing = Session.phase != Homestead::FishingPhase::Idle;
    const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn());
    FVector Bob;
    const bool bOnWater = Avatar && Avatar->FishingBobOnWater(Bob) && bFishing;
    if (bOnWater) LastBob = Bob + FVector(0, 0, WaterLiftCm);
    const bool bUnder = Homestead::Fishing::FloatUnder(Session);
    const float Nibble = static_cast<float>(Homestead::Fishing::Nibble(Session));

    if (bOnWater && Nibble > 0.01f && LastNibble <= 0.01f) Spawn(Now, LastBob, NibbleReach, NibbleLife, NibbleAlpha);
    LastNibble = Nibble;
    Ripples.RemoveAll([Now](const FRipple& Ripple) { return Now - Ripple.Start > Ripple.Life; });
    if (Ripples.IsEmpty() && !(bOnWater && bUnder)) return Layer;

    int32 ViewX = 0, ViewY = 0;
    PC->GetViewportSize(ViewX, ViewY);
    if (ViewX <= 0 || ViewY <= 0) return Layer;
    const FVector2D ToLocal = Geometry.GetLocalSize() / FVector2D(static_cast<double>(ViewX), static_cast<double>(ViewY));
    const FLinearColor Tint = Style.GetColorAndOpacityTint();
    TArray<FVector2D> Points;
    Points.Reserve(Segments + 1);
    const auto Ring = [&](const FVector& Centre, float Radius, const FLinearColor& Colour, float Alpha)
    {
        if (Alpha <= 0.01f || Radius <= 0.0f) return;
        Points.Reset();
        for (int32 Index = 0; Index <= Segments; ++Index)
        {
            const float Angle = UE_TWO_PI * Index / Segments;
            FVector2D Pixel;
            if (!PC->ProjectWorldLocationToScreen(Centre + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0) * Radius, Pixel)) return;
            Points.Add(Pixel * ToLocal);
        }
        FSlateDrawElement::MakeLines(Elements, ++Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
            Shadow.CopyWithNewOpacity(0.35f * Alpha) * Tint, true, ShadowWidth);
        FSlateDrawElement::MakeLines(Elements, ++Layer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None,
            Colour.CopyWithNewOpacity(Alpha) * Tint, true, LineWidth);
    };
    for (const FRipple& Ripple : Ripples)
    {
        const float Age = static_cast<float>((Now - Ripple.Start) / Ripple.Life);
        const float Spread = 1.0f - FMath::Square(1.0f - Age);
        Ring(Ripple.At, ClosingInner + (Ripple.Reach - ClosingInner) * Spread, RippleColour, Ripple.Alpha * (1.0f - Age));
    }
    if (bOnWater && bUnder)
    {
        // The click cue: one ring closes on the float's spot across the reaction window.
        const float Window = FMath::Clamp(static_cast<float>(Homestead::Fishing::Marker(Session)), 0.0f, 1.0f);
        Ring(LastBob, FMath::Lerp(ClosingOuter, ClosingInner, Window), Cue, 0.55f + 0.45f * Window);
    }
    return Layer;
}
}
