#include "SHomesteadMinimap.h"

#include "../HomesteadMapComponent.h"
#include "HomesteadMapPainter.h"

namespace HomesteadMenus
{
namespace
{
using namespace HomesteadMapPaint;
constexpr float LogicalRadius = 110.0f;
constexpr int32 RimSegments = 72;
const FLinearColor Bezel(0.035f, 0.055f, 0.046f, 0.96f);
FVector2D ToLocal(HomesteadMap::Vec Value) { return FVector2D(Value.x, Value.y); }
HomesteadMap::Vec ToVec(FVector2D Value) { return {Value.X, Value.Y}; }
}

void SHomesteadMinimap::Construct(const FArguments& Args)
{
    Map = Args._Map;
    SetVisibility(EVisibility::HitTestInvisible);
}

bool SHomesteadMinimap::CircleLocal(const FGeometry& Geometry, FVector2D& Center, float& Radius) const
{
    const float Scale = FMath::Max(0.01f, Geometry.Scale);
    const FVector2D Physical = Geometry.GetLocalSize() * Scale;
    if (Physical.X < 1 || Physical.Y < 1) return false;
    // The Canvas HUD's rule: 1080 logical lines, clamped for tiny and very large windows.
    const float UiScale = FMath::Clamp(static_cast<float>(Physical.Y) / 1080.0f, 0.4f, 1.5f);
    const FBox2D Box = UHomesteadMapComponent::MinimapBox(Physical.X / UiScale);
    const float LocalPerLogical = UiScale / Scale;
    Center = Box.GetCenter() * LocalPerLogical;
    Radius = LogicalRadius * LocalPerLogical;
    return true;
}

int32 SHomesteadMinimap::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
    FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const
{
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->IsMinimapVisible()) return LayerId;
    const FHomesteadMapFrame& Frame = Component->Frame();
    if (!Frame.Model.IsValid()) return LayerId;
    FVector2D Center;
    float Radius = 0;
    if (!CircleLocal(Geometry, Center, Radius)) return LayerId;
    const float U = Radius / LogicalRadius;
    HomesteadMap::View View;
    View.center = Frame.Player;
    View.screenCenter = ToVec(Center);
    View.pixelsPerCm = 2.0 * Radius / UHomesteadMapComponent::MinimapCropCm;
    View.headingDegrees = Frame.bRotateWithCamera ? Frame.CameraYaw : 0.0;

    FPainter Paint(Geometry, Out, LayerId);
    Paint.Disc(Center + FVector2D(0, 3 * U), Radius + 7 * U, RimShadow, RimSegments);
    Paint.Disc(Center, Radius + 5 * U, Bezel, RimSegments);

    // The baked map, cropped to the disc: a fan whose rim samples the texture under each point.
    TArray<FVector2D> Points, UVs, Rim;
    Points.Add(Center);
    UVs.Add(ToLocal(HomesteadMap::WorldToUV(Frame.Model->Transform, Frame.Player)));
    for (int32 Index = 0; Index <= RimSegments; ++Index)
    {
        const float Angle = Index * 2.0f * PI / RimSegments;
        const FVector2D Point = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
        Points.Add(Point);
        if (Index < RimSegments) Rim.Add(Point);
        UVs.Add(ToLocal(HomesteadMap::WorldToUV(Frame.Model->Transform, HomesteadMap::ScreenToWorld(View, ToVec(Point)))));
    }
    if (Frame.MapBrush) Paint.TexturedFan(Frame.MapBrush, Points, UVs, FLinearColor::White);
    else Paint.Fill(Rim, Parchment);

    // The owned boundary, dashed in oxblood ink over a pale halo.
    for (const FHomesteadMapParcel& Parcel : Frame.Model->Parcels)
    {
        if (!Parcel.bOwned) continue;
        std::vector<HomesteadMap::Vec> Screen;
        Screen.reserve(Parcel.Ring.size());
        for (const auto& Point : Parcel.Ring) Screen.push_back(HomesteadMap::WorldToScreen(View, Point));
        for (auto Dash : HomesteadMap::DashRing(Screen, 8.0 * U, 5.0 * U))
        {
            if (!HomesteadMap::ClipSegmentToCircle(Dash.first, Dash.second, View.screenCenter, Radius - 1.5 * U)) continue;
            Paint.Segment(ToLocal(Dash.first), ToLocal(Dash.second), Halo, 4.5f * U);
            Paint.Segment(ToLocal(Dash.first), ToLocal(Dash.second), BoundaryInk, 2.2f * U);
        }
    }

    // Landmarks: in view where they are, otherwise waiting on the rim in their direction.
    const float Inner = Radius - 13 * U;
    TArray<TPair<const FHomesteadMapLandmark*, FVector2D>> Near, Far;
    for (const FHomesteadMapLandmark& Place : Frame.Model->Landmarks)
    {
        const FVector2D Offset = ToLocal(HomesteadMap::WorldToScreen(View, Place.Position)) - Center;
        if (Offset.Size() <= Inner) Near.Emplace(&Place, Center + Offset);
        else Far.Emplace(&Place, Center + ToLocal(HomesteadMap::ClampToRadius(ToVec(Offset), Inner)));
    }
    // Off-crop places share the rim; one that would cover another there waits its turn.
    TArray<FVector2D> RimTaken;
    for (const auto& Place : Far)
    {
        bool bClear = true;
        for (const FVector2D& Other : RimTaken) bClear &= FVector2D::Distance(Other, Place.Value) > 17.0f * U;
        if (!bClear) continue;
        RimTaken.Add(Place.Value);
        Paint.Badge(Place.Key->Glyph, Place.Value, 8.5f * U, 0.78f);
    }
    for (const auto& Place : Near) Paint.Badge(Place.Key->Glyph, Place.Value, 11.0f * U);

    Paint.Circle(Center, Radius, Brass, 2.5f * U, RimSegments);
    Paint.Circle(Center, Radius + 4.5f * U, FLinearColor(0, 0, 0, 0.6f), 1.2f * U, RimSegments);

    // North sits on the rim; it moves only when the map turns with the camera.
    const FVector2D North = Center + ToLocal(HomesteadMap::ScreenDirection(View, 0.0)) * (Radius + 1.0f * U);
    Paint.Disc(North, 10.5f * U, Bezel, 20);
    Paint.Circle(North, 10.5f * U, Brass, 1.6f * U, 20);
    const float LetterSize = 10.0f * U;
    const FVector2D Letter = FPainter::MeasureText(TEXT("N"), LetterSize);
    Paint.Text(TEXT("N"), North - Letter * 0.5f, LetterSize, Brass, true, false);

    // Her arrow last, always on top at the centre.
    Paint.PlayerArrow(Center, ToLocal(HomesteadMap::ScreenDirection(View, Frame.FacingYaw)), 10.0f * U);
    return Paint.GetLayer();
}
}
