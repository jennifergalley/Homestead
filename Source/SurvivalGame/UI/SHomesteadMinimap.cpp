#include "SHomesteadMinimap.h"
#include "HomesteadUITheme.h"

#include "../HomesteadMapComponent.h"
#include "HomesteadMapPainter.h"

namespace HomesteadMenus
{
namespace
{
constexpr float MmLogicalRadius = 110.0f;
constexpr int32 MmRimSegments = 72;
HomesteadUITheme::FThemeColor MmBezel(0.035f, 0.055f, 0.046f, 0.96f);
FVector2D MmToLocal(HomesteadMap::Vec Value) { return FVector2D(Value.x, Value.y); }
HomesteadMap::Vec MmToVec(FVector2D Value) { return {Value.X, Value.Y}; }
}
// Glyph sizes in HUD units, each with a floor in physical pixels so they still read at 720p
// (0.667 px per unit there, where an 8.5-unit badge had shrunk to under 6 px).
namespace MmStyle
{
constexpr float NearBadge = 20.0f, NearBadgeMinPx = 16.0f;
constexpr float FarBadge = 15.0f, FarBadgeMinPx = 13.0f;
constexpr float BadgeGap = 3.0f;
constexpr float NorthLetter = 11.0f, NorthLetterMinPx = 10.0f;
constexpr float Arrow = 11.0f, ArrowMinPx = 10.0f;
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
    const FBox2D Box = UHomesteadMapComponent::MinimapBox(Physical.X / UiScale, Physical.Y / UiScale);
    const float LocalPerLogical = UiScale / Scale;
    Center = Box.GetCenter() * LocalPerLogical;
    Radius = MmLogicalRadius * LocalPerLogical;
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
    const float U = Radius / MmLogicalRadius;
    HomesteadMap::View View;
    View.center = Frame.Player;
    View.screenCenter = MmToVec(Center);
    View.pixelsPerCm = 2.0 * Radius / UHomesteadMapComponent::MinimapCropCm;
    View.headingDegrees = Frame.bRotateWithCamera ? Frame.CameraYaw : 0.0;

    HomesteadMapPaint::FPainter Paint(Geometry, Out, LayerId);
    Paint.Disc(Center + FVector2D(0, 3 * U), Radius + 7 * U, HomesteadMapPaint::RimShadow, MmRimSegments);
    Paint.Disc(Center, Radius + 5 * U, MmBezel, MmRimSegments);

    // The baked map, cropped to the disc: a fan whose rim samples the texture under each point.
    TArray<FVector2D> Points, UVs, Rim;
    Points.Add(Center);
    UVs.Add(MmToLocal(HomesteadMap::WorldToUV(Frame.Model->Transform, Frame.Player)));
    for (int32 Index = 0; Index <= MmRimSegments; ++Index)
    {
        const float Angle = Index * 2.0f * PI / MmRimSegments;
        const FVector2D Point = Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius;
        Points.Add(Point);
        if (Index < MmRimSegments) Rim.Add(Point);
        UVs.Add(MmToLocal(HomesteadMap::WorldToUV(Frame.Model->Transform, HomesteadMap::ScreenToWorld(View, MmToVec(Point)))));
    }
    if (Frame.MapBrush) Paint.TexturedFan(Frame.MapBrush, Points, UVs, FLinearColor::White);
    else Paint.Fill(Rim, HomesteadMapPaint::Parchment);

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
            Paint.Segment(MmToLocal(Dash.first), MmToLocal(Dash.second), HomesteadMapPaint::Halo, 4.5f * U);
            Paint.Segment(MmToLocal(Dash.first), MmToLocal(Dash.second), HomesteadMapPaint::BoundaryInk, 2.2f * U);
        }
    }

    // Landmarks: in view where they are, otherwise waiting on the rim in their direction. Near ones
    // come first, closest first, and a badge that would cover one already placed waits its turn.
    const double PhysicalPerLocal = FMath::Max(0.01f, Geometry.Scale);
    const auto Legible = [PhysicalPerLocal](float Local, float MinPhysical)
        { return static_cast<float>(HomesteadMap::AtLeastPhysical(Local, PhysicalPerLocal, MinPhysical)); };
    const float NearRadius = Legible(MmStyle::NearBadge * U, MmStyle::NearBadgeMinPx);
    const float FarRadius = Legible(MmStyle::FarBadge * U, MmStyle::FarBadgeMinPx);
    const float Inner = Radius - FMath::Max(13 * U, NearRadius + 2 * U);
    struct FPlaced { const FHomesteadMapLandmark* Place; FVector2D At; float Radius; bool bNear; double Order; };
    TArray<FPlaced> Candidates;
    for (const FHomesteadMapLandmark& Place : Frame.Model->Landmarks)
    {
        const FVector2D Offset = MmToLocal(HomesteadMap::WorldToScreen(View, Place.Position)) - Center;
        const bool bNear = Offset.Size() <= Inner;
        const FVector2D At = bNear ? Center + Offset : Center + MmToLocal(HomesteadMap::ClampToRadius(MmToVec(Offset), Inner));
        Candidates.Add({&Place, At, bNear ? NearRadius : FarRadius, bNear, (bNear ? 0.0 : 1e12) + Offset.Size()});
    }
    Candidates.Sort([](const FPlaced& A, const FPlaced& B) { return A.Order < B.Order; });
    std::vector<HomesteadMap::Vec> Centers;
    std::vector<double> Radii;
    for (const FPlaced& Candidate : Candidates) { Centers.push_back(MmToVec(Candidate.At)); Radii.push_back(Candidate.Radius); }
    const std::vector<int> Kept = HomesteadMap::SpacedCircles(Centers, Radii, MmStyle::BadgeGap * U);
    // Far ones draw under near ones, each centred on a whole physical pixel so its ink stays sharp.
    for (const bool bNearPass : {false, true})
        for (const int Index : Kept)
        {
            const FPlaced& Placed = Candidates[Index];
            if (Placed.bNear != bNearPass) continue;
            const FVector2D Snapped(HomesteadMap::SnapToPixel(Placed.At.X, PhysicalPerLocal),
                HomesteadMap::SnapToPixel(Placed.At.Y, PhysicalPerLocal));
            Paint.Badge(Placed.Place->Glyph, Snapped, Placed.Radius, Placed.bNear ? 1.0f : 0.82f);
        }

    Paint.Circle(Center, Radius, HomesteadMapPaint::Brass, 2.5f * U, MmRimSegments);
    Paint.Circle(Center, Radius + 4.5f * U, FLinearColor(0, 0, 0, 0.6f), 1.2f * U, MmRimSegments);

    // North sits on the rim; it moves only when the map turns with the camera.
    const float LetterSize = Legible(MmStyle::NorthLetter * U, MmStyle::NorthLetterMinPx);
    const float NorthRadius = FMath::Max(10.5f * U, LetterSize * 1.05f);
    const FVector2D NorthAt = Center + MmToLocal(HomesteadMap::ScreenDirection(View, 0.0)) * (Radius + 1.0f * U);
    const FVector2D North(HomesteadMap::SnapToPixel(NorthAt.X, PhysicalPerLocal), HomesteadMap::SnapToPixel(NorthAt.Y, PhysicalPerLocal));
    Paint.Disc(North, NorthRadius, MmBezel, 20);
    Paint.Circle(North, NorthRadius, HomesteadMapPaint::Brass, 1.6f * U, 20);
    const FVector2D Letter = HomesteadMapPaint::FPainter::MeasureText(TEXT("N"), LetterSize);
    Paint.Text(TEXT("N"), North - Letter * 0.5f, LetterSize, HomesteadMapPaint::Brass, true, false);

    // Her arrow last, always on top at the centre.
    Paint.PlayerArrow(Center, MmToLocal(HomesteadMap::ScreenDirection(View, Frame.FacingYaw)),
        Legible(MmStyle::Arrow * U, MmStyle::ArrowMinPx));
    return Paint.GetLayer();
}
}
