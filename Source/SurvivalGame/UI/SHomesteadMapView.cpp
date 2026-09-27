#include "SHomesteadMapView.h"

#include "../HomesteadMapComponent.h"
#include "HomesteadMapPainter.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"

namespace HomesteadMenus
{
namespace
{
constexpr double MvMaxPixelsPerCm = 0.06;  // 6 px per metre: close enough to read the lanes.
constexpr double MvPlacePixelsPerCm = 0.025;
constexpr float MvStickDeadZone = 0.2f;
constexpr double MvStaleAxisSeconds = 0.25;
const FLinearColor MvBackdrop(0.05f, 0.075f, 0.065f, 0.95f);
const FLinearColor MvPlate(0.025f, 0.05f, 0.038f, 0.82f);
const FLinearColor MvCream(0.95f, 0.92f, 0.82f, 1.0f);
const FLinearColor MvGold(0.92f, 0.74f, 0.43f, 1.0f);
FVector2D MvToLocal(HomesteadMap::Vec Value) { return FVector2D(Value.x, Value.y); }
HomesteadMap::Vec MvToVec(FVector2D Value) { return {Value.X, Value.Y}; }
HomesteadMap::MapTransform MvTransformOf(const UHomesteadMapComponent* Map)
{
    return Map && Map->Frame().Model.IsValid() ? Map->Frame().Model->Transform : HomesteadMap::MapTransform{};
}
float MvLive(const auto& Axis, double Now)
{
    return Axis.At >= 0 && Now - Axis.At <= MvStaleAxisSeconds ? Axis.Value : 0.0f;
}
}

void SHomesteadMapView::Construct(const FArguments& Args)
{
    Map = Args._Map;
    UsesGamepad = Args._UsesGamepad;
    SetClipping(EWidgetClipping::ClipToBounds);
}

FHomesteadMapViewState& SHomesteadMapView::State() const
{
    static FHomesteadMapViewState Fallback;
    UHomesteadMapComponent* Component = Map.Get();
    return Component ? Component->MapViewState() : Fallback;
}

double SHomesteadMapView::FitPixelsPerCm() const
{
    const HomesteadMap::MapTransform T = MvTransformOf(Map.Get());
    return FMath::Max(1e-6, FMath::Min(Size.X / T.sizeY, Size.Y / T.sizeX));
}

double SHomesteadMapView::PixelsPerCm() const { return State().PixelsPerCm; }
HomesteadMap::Vec SHomesteadMapView::Center() const { return State().Center; }

void SHomesteadMapView::EnsureState()
{
    FHomesteadMapViewState& S = State();
    if (!S.bValid)
    {
        const HomesteadMap::MapTransform T = MvTransformOf(Map.Get());
        S.PixelsPerCm = FitPixelsPerCm();
        S.Center = {T.minX + T.sizeX * 0.5, T.minY + T.sizeY * 0.5};
        S.Selected = INDEX_NONE;
        S.bValid = true;
    }
    Clamp();
}

void SHomesteadMapView::Clamp()
{
    FHomesteadMapViewState& S = State();
    const HomesteadMap::MapTransform T = MvTransformOf(Map.Get());
    const double Fit = FitPixelsPerCm();
    S.PixelsPerCm = FMath::Clamp(S.PixelsPerCm, Fit, FMath::Max(Fit, MvMaxPixelsPerCm));
    const auto Keep = [](double Value, double Low, double Extent, double Half)
    {
        return Extent > Half * 2.0 ? FMath::Clamp(Value, Low + Half, Low + Extent - Half) : Low + Extent * 0.5;
    };
    S.Center.x = Keep(S.Center.x, T.minX, T.sizeX, Size.Y * 0.5 / S.PixelsPerCm);
    S.Center.y = Keep(S.Center.y, T.minY, T.sizeY, Size.X * 0.5 / S.PixelsPerCm);
}

HomesteadMap::View SHomesteadMapView::MakeView() const
{
    HomesteadMap::View View;
    View.center = State().Center;
    View.screenCenter = {Size.X * 0.5, Size.Y * 0.5};
    View.pixelsPerCm = FMath::Max(1e-6, State().PixelsPerCm);
    return View;
}

void SHomesteadMapView::SetAnalog(const FKey& Key, float Value)
{
    const double Now = FPlatformTime::Seconds();
    FAxis* Axis = Key == EKeys::Gamepad_LeftX ? &LeftX : Key == EKeys::Gamepad_LeftY ? &LeftY
        : Key == EKeys::Gamepad_RightY ? &RightY : Key == EKeys::Gamepad_LeftTriggerAxis ? &LeftTrigger
        : Key == EKeys::Gamepad_RightTriggerAxis ? &RightTrigger : nullptr;
    if (Axis && FMath::IsFinite(Value)) { Axis->Value = FMath::Clamp(Value, -1.0f, 1.0f); Axis->At = Now; }
}

void SHomesteadMapView::ZoomBy(double Factor, FVector2D AnchorLocal)
{
    EnsureState();
    FHomesteadMapViewState& S = State();
    const HomesteadMap::Vec Anchor = MvToVec(AnchorLocal);
    const HomesteadMap::Vec Before = HomesteadMap::ScreenToWorld(MakeView(), Anchor);
    S.PixelsPerCm *= Factor;
    Clamp();
    const HomesteadMap::Vec After = HomesteadMap::ScreenToWorld(MakeView(), Anchor);
    S.Center.x += Before.x - After.x;
    S.Center.y += Before.y - After.y;
    Clamp();
}

void SHomesteadMapView::PanPixels(FVector2D Delta)
{
    EnsureState();
    const HomesteadMap::View View = MakeView();
    const HomesteadMap::Vec From = HomesteadMap::ScreenToWorld(View, View.screenCenter);
    const HomesteadMap::Vec To = HomesteadMap::ScreenToWorld(View, {View.screenCenter.x + Delta.X, View.screenCenter.y + Delta.Y});
    State().Center.x += To.x - From.x;
    State().Center.y += To.y - From.y;
    Clamp();
}

void SHomesteadMapView::Reveal(int32 Index)
{
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->Frame().Model.IsValid() || !Component->Frame().Model->Landmarks.IsValidIndex(Index)) return;
    const FVector2D At = MvToLocal(HomesteadMap::WorldToScreen(MakeView(), Component->Frame().Model->Landmarks[Index].Position));
    const FVector2D Margin = Size * 0.15f;
    if (At.X < Margin.X || At.Y < Margin.Y || At.X > Size.X - Margin.X || At.Y > Size.Y - Margin.Y)
    {
        State().Center = Component->Frame().Model->Landmarks[Index].Position;
        Clamp();
    }
}

bool SHomesteadMapView::Step(int32 Dx, int32 Dy)
{
    EnsureState();
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->Frame().Model.IsValid() || (!Dx && !Dy)) return false;
    const auto& Places = Component->Frame().Model->Landmarks;
    FHomesteadMapViewState& S = State();
    const HomesteadMap::View View = MakeView();
    const HomesteadMap::Vec Origin = HomesteadMap::WorldToScreen(View, Places.IsValidIndex(S.Selected) ? Places[S.Selected].Position
        : Component->Frame().Player);
    const FVector2D Direction = FVector2D(Dx, Dy).GetSafeNormal();
    int32 Best = INDEX_NONE;
    double BestScore = TNumericLimits<double>::Max();
    for (int32 Index = 0; Index < Places.Num(); ++Index)
    {
        if (Index == S.Selected) continue;
        const FVector2D Delta = MvToLocal(HomesteadMap::WorldToScreen(View, Places[Index].Position)) - MvToLocal(Origin);
        const double Along = FVector2D::DotProduct(Delta, Direction);
        const double Across = FMath::Abs(FVector2D::CrossProduct(Delta, Direction));
        // Within about 56 degrees of the direction pressed.
        if (Along <= 0.5 || Across > Along * 1.5) continue;
        const double Score = Along + Across * 2.0;
        if (Score < BestScore) { BestScore = Score; Best = Index; }
    }
    // Nothing has been chosen yet: any press starts with the nearest place in that direction or,
    // failing that, the manor.
    if (Best == INDEX_NONE && !Places.IsValidIndex(S.Selected) && Places.Num()) Best = 0;
    if (Best == INDEX_NONE) return false;
    S.Selected = Best;
    Reveal(Best);
    return true;
}

void SHomesteadMapView::ToggleZoomOnSelected()
{
    EnsureState();
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->Frame().Model.IsValid()) return;
    FHomesteadMapViewState& S = State();
    const auto& Places = Component->Frame().Model->Landmarks;
    if (S.PixelsPerCm >= MvPlacePixelsPerCm * 0.9) { S.PixelsPerCm = FitPixelsPerCm(); Clamp(); return; }
    S.Center = Places.IsValidIndex(S.Selected) ? Places[S.Selected].Position : Component->Frame().Player;
    S.PixelsPerCm = MvPlacePixelsPerCm;
    Clamp();
}

FString SHomesteadMapView::SelectedName() const
{
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->Frame().Model.IsValid()) return FString();
    const auto& Places = Component->Frame().Model->Landmarks;
    return Places.IsValidIndex(State().Selected) ? Places[State().Selected].Name : FString();
}

void SHomesteadMapView::Tick(const FGeometry& Geometry, const double, const float DeltaTime)
{
    Size = FVector2D(Geometry.GetLocalSize()).ComponentMax(FVector2D(1, 1));
    Time += DeltaTime;
    EnsureState();
    const double Now = FPlatformTime::Seconds();
    const float Dt = FMath::Min(DeltaTime, 0.1f);
    FVector2D Stick(MvLive(LeftX, Now), -MvLive(LeftY, Now));
    if (Stick.Size() > MvStickDeadZone)
        PanPixels(Stick * ((Stick.Size() - MvStickDeadZone) / (1 - MvStickDeadZone) / Stick.Size()) * Size.GetMin() * 0.9f * Dt);
    const float Zoom = MvLive(RightY, Now) + MvLive(RightTrigger, Now) - MvLive(LeftTrigger, Now);
    if (FMath::Abs(Zoom) > MvStickDeadZone) ZoomBy(FMath::Pow(2.0, Zoom * 1.6 * Dt), Size * 0.5f);
}

FReply SHomesteadMapView::OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
    bDragging = true;
    DragLast = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    DragStart = DragLast;
    return FReply::Handled().CaptureMouse(SharedThis(this));
}

FReply SHomesteadMapView::OnMouseButtonUp(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton || !bDragging) return FReply::Unhandled();
    bDragging = false;
    const FVector2D At = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    // A click without a drag picks the landmark under the pointer.
    const UHomesteadMapComponent* Component = Map.Get();
    if (FVector2D::Distance(At, DragStart) < 5.0f && Component && Component->Frame().Model.IsValid())
    {
        const auto& Places = Component->Frame().Model->Landmarks;
        int32 Hit = INDEX_NONE;
        double Nearest = 22.0;
        for (int32 Index = 0; Index < Places.Num(); ++Index)
        {
            const double Distance = FVector2D::Distance(At, MvToLocal(HomesteadMap::WorldToScreen(MakeView(), Places[Index].Position)));
            if (Distance < Nearest) { Nearest = Distance; Hit = Index; }
        }
        State().Selected = Hit;
    }
    return FReply::Handled().ReleaseMouseCapture();
}

FReply SHomesteadMapView::OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (!bDragging || !HasMouseCapture()) return FReply::Unhandled();
    const FVector2D At = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
    PanPixels(DragLast - At);
    DragLast = At;
    return FReply::Handled();
}

FReply SHomesteadMapView::OnMouseWheel(const FGeometry& Geometry, const FPointerEvent& Event)
{
    ZoomBy(FMath::Pow(1.3, Event.GetWheelDelta()), Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
    return FReply::Handled();
}

FReply SHomesteadMapView::OnMouseButtonDoubleClick(const FGeometry& Geometry, const FPointerEvent& Event)
{
    if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
    ZoomBy(2.0, Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition()));
    return FReply::Handled();
}

FCursorReply SHomesteadMapView::OnCursorQuery(const FGeometry&, const FPointerEvent&) const
{
    return FCursorReply::Cursor(bDragging ? EMouseCursor::GrabHandClosed : EMouseCursor::GrabHand);
}

int32 SHomesteadMapView::OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
    FSlateWindowElementList& Out, int32 LayerId, const FWidgetStyle&, bool) const
{
    HomesteadMapPaint::FPainter Paint(Geometry, Out, LayerId);
    const FVector2D Local = Geometry.GetLocalSize();
    Paint.Fill({FVector2D(0, 0), FVector2D(Local.X, 0), Local, FVector2D(0, Local.Y)}, MvBackdrop);
    const UHomesteadMapComponent* Component = Map.Get();
    if (!Component || !Component->Frame().Model.IsValid() || !State().bValid) return Paint.GetLayer();
    const FHomesteadMapFrame& Frame = Component->Frame();
    const FHomesteadMapModel& Model = *Frame.Model;
    const HomesteadMap::View View = MakeView();
    const HomesteadMap::MapTransform& T = Model.Transform;
    const HomesteadMap::Vec Min{0, 0}, Max{Local.X, Local.Y};

    // The sheet: the part of the baked texture inside the page.
    const FVector2D TopLeft = MvToLocal(HomesteadMap::WorldToScreen(View, {T.minX + T.sizeX, T.minY}));
    const FVector2D BottomRight = MvToLocal(HomesteadMap::WorldToScreen(View, {T.minX, T.minY + T.sizeY}));
    const FVector2D A(FMath::Max(0.0, TopLeft.X), FMath::Max(0.0, TopLeft.Y));
    const FVector2D B(FMath::Min(Local.X, BottomRight.X), FMath::Min(Local.Y, BottomRight.Y));
    if (B.X > A.X && B.Y > A.Y)
    {
        const TArray<FVector2D> Quad = {A, FVector2D(B.X, A.Y), B, FVector2D(A.X, B.Y)};
        TArray<FVector2D> UVs;
        for (const FVector2D& Corner : Quad) UVs.Add((Corner - TopLeft) / (BottomRight - TopLeft));
        if (Frame.MapBrush) Paint.TexturedFan(Frame.MapBrush, Quad, UVs, FLinearColor::White);
        else Paint.Fill(Quad, HomesteadMapPaint::Parchment);
    }
    const float Scale = FMath::Clamp(Local.GetMin() / 560.0f, 0.8f, 1.6f);

    // Parcels: for-sale land dimmed and hatched, the owned estate dashed in oxblood.
    for (const FHomesteadMapParcel& Parcel : Model.Parcels)
    {
        std::vector<HomesteadMap::Vec> Screen;
        TArray<FVector2D> Outline;
        for (const HomesteadMap::Vec& Point : Parcel.Ring)
        {
            Screen.push_back(HomesteadMap::WorldToScreen(View, Point));
            Outline.Add(MvToLocal(Screen.back()));
        }
        if (Parcel.bForSale)
        {
            Paint.Fill(Outline, FLinearColor(0.05f, 0.03f, 0.02f, 0.22f));
            for (auto Line : HomesteadMap::HatchPolygon(Screen, 9.0 * Scale, 45.0))
                if (HomesteadMap::ClipSegmentToRect(Line.first, Line.second, Min, Max))
                    Paint.Segment(MvToLocal(Line.first), MvToLocal(Line.second), FLinearColor(HomesteadMapPaint::Ink.R, HomesteadMapPaint::Ink.G, HomesteadMapPaint::Ink.B, 0.38f), 1.1f);
            for (auto Dash : HomesteadMap::DashRing(Screen, 3.0 * Scale, 4.0 * Scale))
                if (HomesteadMap::ClipSegmentToRect(Dash.first, Dash.second, Min, Max))
                    Paint.Segment(MvToLocal(Dash.first), MvToLocal(Dash.second), FLinearColor(HomesteadMapPaint::Ink.R, HomesteadMapPaint::Ink.G, HomesteadMapPaint::Ink.B, 0.7f), 1.4f);
        }
        else if (Parcel.bOwned)
            for (auto Dash : HomesteadMap::DashRing(Screen, 11.0 * Scale, 6.0 * Scale))
                if (HomesteadMap::ClipSegmentToRect(Dash.first, Dash.second, Min, Max))
                {
                    Paint.Segment(MvToLocal(Dash.first), MvToLocal(Dash.second), HomesteadMapPaint::Halo, 5.0f * Scale);
                    Paint.Segment(MvToLocal(Dash.first), MvToLocal(Dash.second), HomesteadMapPaint::BoundaryInk, 2.4f * Scale);
                }
    }
    // Landmarks: badges first, then as many names as fit without overlapping (the focused place
    // first, then the list order, which starts with the manor).
    const int32 Selected = State().Selected;
    const FVector2D Here = MvToLocal(HomesteadMap::WorldToScreen(View, Frame.Player));
    const auto OnPage = [&Local](FVector2D At) { return At.X >= -40 && At.Y >= -40 && At.X <= Local.X + 40 && At.Y <= Local.Y + 40; };
    TArray<FBox2D> Taken;
    {
        const FVector2D HereExtent = HomesteadMapPaint::FPainter::MeasureText(TEXT("You are here"), 12.0f * Scale);
        Taken.Add(FBox2D(Here + FVector2D(-HereExtent.X * 0.5f, 13 * Scale), Here + FVector2D(HereExtent.X * 0.5f, 15 * Scale + HereExtent.Y)));
    }
    TArray<int32> Order;
    if (Model.Landmarks.IsValidIndex(Selected)) Order.Add(Selected);
    for (int32 Index = 0; Index < Model.Landmarks.Num(); ++Index) if (Index != Selected) Order.Add(Index);
    TArray<FBox2D> Badges;
    Badges.SetNum(Model.Landmarks.Num());
    for (int32 Index = 0; Index < Model.Landmarks.Num(); ++Index)
    {
        const FVector2D At = MvToLocal(HomesteadMap::WorldToScreen(View, Model.Landmarks[Index].Position));
        const float Radius = (Index == Selected ? 15.0f : 12.0f) * Scale;
        Badges[Index] = FBox2D(At - FVector2D(Radius, Radius), At + FVector2D(Radius, Radius));
    }
    TArray<TPair<int32, FVector2D>> Labels;
    for (int32 Index : Order)
    {
        const FVector2D At = Badges[Index].GetCenter();
        if (!OnPage(At)) continue;
        const bool bSelected = Index == Selected;
        const float Radius = Badges[Index].GetExtent().X;
        const FVector2D Extent = HomesteadMapPaint::FPainter::MeasureText(Model.Landmarks[Index].Name, (bSelected ? 13.5f : 12.0f) * Scale) + FVector2D(8, 2);
        const FVector2D Candidates[] = {At + FVector2D(Radius + 3 * Scale, -Extent.Y * 0.5f),
            At - FVector2D(Radius + 3 * Scale + Extent.X, Extent.Y * 0.5f),
            At - FVector2D(Extent.X * 0.5f, Radius + 3 * Scale + Extent.Y), At + FVector2D(-Extent.X * 0.5f, Radius + 3 * Scale)};
        for (const FVector2D& Corner : Candidates)
        {
            const FBox2D Box(Corner, Corner + Extent);
            bool bClear = true;
            for (const FBox2D& Other : Taken) bClear &= !Box.Intersect(Other);
            for (int32 Other = 0; Other < Badges.Num() && bClear; ++Other)
                bClear &= Other == Index || !OnPage(Badges[Other].GetCenter()) || !Box.Intersect(Badges[Other]);
            if (!bClear && !bSelected) continue;
            Taken.Add(Box);
            Labels.Emplace(Index, Corner);
            break;
        }
    }
    for (int32 Index = 0; Index < Model.Landmarks.Num(); ++Index)
        if (Index != Selected && OnPage(Badges[Index].GetCenter()))
            Paint.Badge(Model.Landmarks[Index].Glyph, Badges[Index].GetCenter(), Badges[Index].GetExtent().X, 1.0f, false);
    if (Badges.IsValidIndex(Selected) && OnPage(Badges[Selected].GetCenter()))
        Paint.Badge(Model.Landmarks[Selected].Glyph, Badges[Selected].GetCenter(), Badges[Selected].GetExtent().X, 1.0f, true);
    for (const auto& Label : Labels)
    {
        const bool bSelected = Label.Key == Selected;
        const FString& Name = Model.Landmarks[Label.Key].Name;
        const float LabelSize = (bSelected ? 13.5f : 12.0f) * Scale;
        const FVector2D Extent = HomesteadMapPaint::FPainter::MeasureText(Name, LabelSize) + FVector2D(8, 2);
        const FVector2D& Corner = Label.Value;
        Paint.Fill({Corner, Corner + FVector2D(Extent.X, 0), Corner + Extent, Corner + FVector2D(0, Extent.Y)}, MvPlate);
        Paint.Text(Name, Corner + FVector2D(4, 1), LabelSize, bSelected ? MvGold : MvCream);
    }

    // For-sale names where they don't cover a place.
    for (const FHomesteadMapParcel& Parcel : Model.Parcels)
    {
        if (!Parcel.bForSale) continue;
        double Low = 1e300, High = -1e300;
        for (const HomesteadMap::Vec& Point : Parcel.Ring)
        {
            const HomesteadMap::Vec S = HomesteadMap::WorldToScreen(View, Point);
            Low = FMath::Min(Low, S.x); High = FMath::Max(High, S.x);
        }
        const FVector2D At = MvToLocal(HomesteadMap::WorldToScreen(View, Parcel.LabelAt));
        if (High - Low < 70 || At.X < 0 || At.Y < 0 || At.X > Local.X || At.Y > Local.Y) continue;
        const float LabelSize = 12.0f * Scale;
        const FVector2D Title = HomesteadMapPaint::FPainter::MeasureText(TEXT("FOR SALE"), LabelSize);
        const FVector2D Name = HomesteadMapPaint::FPainter::MeasureText(Parcel.Label, LabelSize * 0.9f, false);
        const FBox2D Box(At - FVector2D(FMath::Max(Title.X, Name.X) * 0.5f, Title.Y), At + FVector2D(FMath::Max(Title.X, Name.X) * 0.5f, Name.Y));
        bool bClear = true;
        for (const FBox2D& Other : Taken) bClear &= !Box.Intersect(Other);
        for (const FBox2D& Other : Badges) bClear &= !Box.Intersect(Other);
        if (!bClear) continue;
        Paint.Text(TEXT("FOR SALE"), At - FVector2D(Title.X * 0.5f, Title.Y), LabelSize, MvCream);
        Paint.Text(Parcel.Label, At - FVector2D(Name.X * 0.5f, 0), LabelSize * 0.9f, MvCream, false);
    }

    // You are here: a slow pulse under her arrow.
    const float Pulse = FMath::Frac(static_cast<float>(Time) * 0.7f);
    Paint.Circle(Here, (10 + 22 * Pulse) * Scale, FLinearColor(MvGold.R, MvGold.G, MvGold.B, 1.0f - Pulse), 2.5f * Scale, 32);
    Paint.PlayerArrow(Here, MvToLocal(HomesteadMap::ScreenDirection(View, Frame.FacingYaw)), 11.0f * Scale);
    {
        const float LabelSize = 12.0f * Scale;
        const FVector2D Extent = HomesteadMapPaint::FPainter::MeasureText(TEXT("You are here"), LabelSize);
        Paint.Text(TEXT("You are here"), Here + FVector2D(-Extent.X * 0.5f, 15 * Scale), LabelSize, MvCream);
    }

    // North and a scale bar, like a surveyor's sheet.
    const FVector2D NorthAt(Local.X - 34 * Scale, 38 * Scale);
    Paint.Disc(NorthAt, 20 * Scale, MvPlate, 24);
    Paint.Fill({NorthAt + FVector2D(0, -15) * Scale, NorthAt + FVector2D(6, 4) * Scale, NorthAt + FVector2D(-6, 4) * Scale}, MvGold);
    const FVector2D Letter = HomesteadMapPaint::FPainter::MeasureText(TEXT("N"), 11 * Scale);
    Paint.Text(TEXT("N"), NorthAt + FVector2D(-Letter.X * 0.5f, 3 * Scale), 11 * Scale, MvCream, true, false);
    {
        static const double Metres[] = {25, 50, 100, 200, 500, 1000};
        double Length = Metres[0];
        for (double Candidate : Metres) if (Candidate * 100.0 * View.pixelsPerCm <= 170 * Scale) Length = Candidate;
        const float Pixels = static_cast<float>(Length * 100.0 * View.pixelsPerCm);
        const FVector2D Left(18 * Scale, Local.Y - 26 * Scale);
        Paint.Fill({Left + FVector2D(-8, -22) * Scale, Left + FVector2D(Pixels + 60 * Scale, -22 * Scale),
            Left + FVector2D(Pixels + 60 * Scale, 12 * Scale), Left + FVector2D(-8, 12) * Scale}, MvPlate);
        Paint.Segment(Left, Left + FVector2D(Pixels, 0), MvCream, 3.0f * Scale);
        Paint.Segment(Left, Left + FVector2D(0, -7 * Scale), MvCream, 2.0f * Scale);
        Paint.Segment(Left + FVector2D(Pixels, 0), Left + FVector2D(Pixels, -7 * Scale), MvCream, 2.0f * Scale);
        const FString Label = Length >= 1000 ? FString::Printf(TEXT("%.0f km"), Length / 1000) : FString::Printf(TEXT("%.0f m"), Length);
        Paint.Text(Label, Left + FVector2D(Pixels + 8 * Scale, -15 * Scale), 11 * Scale, MvCream);
    }

    // The focused place, and how to drive the map.
    const float CardSize = 15.0f * Scale;
    FString Title = Model.Landmarks.IsValidIndex(Selected) ? Model.Landmarks[Selected].Name : FString();
    FString Detail = Model.Landmarks.IsValidIndex(Selected) ? Model.Landmarks[Selected].Description
        : FString(Frame.bInsideEstate ? TEXT("You are on your own land.") : TEXT("You are off your land just now."));
    if (Title.IsEmpty()) Title = FString::Printf(TEXT("The %s estate"), *Model.EstateName);
    const FVector2D TitleSize = HomesteadMapPaint::FPainter::MeasureText(Title, CardSize);
    const FVector2D DetailSize = HomesteadMapPaint::FPainter::MeasureText(Detail, CardSize * 0.8f, false);
    const FVector2D CardAt(14 * Scale, 14 * Scale);
    const float CardWidth = FMath::Max(TitleSize.X, DetailSize.X) + 24 * Scale;
    const float CardHeight = TitleSize.Y + DetailSize.Y + 20 * Scale;
    Paint.Fill({CardAt, CardAt + FVector2D(CardWidth, 0), CardAt + FVector2D(CardWidth, CardHeight), CardAt + FVector2D(0, CardHeight)}, MvPlate);
    Paint.Text(Title, CardAt + FVector2D(12, 8) * Scale, CardSize, MvGold);
    Paint.Text(Detail, CardAt + FVector2D(12 * Scale, 10 * Scale + TitleSize.Y), CardSize * 0.8f, MvCream, false);

    const bool bGamepad = UsesGamepad.Get(false);
    const FString Hints = bGamepad
        ? TEXT("Left stick: pan    Right stick / triggers: zoom    D-pad: places    A: zoom to place    LB / RB: pages")
        : TEXT("Drag: pan    Wheel: zoom    Click: choose a place    Arrows: places    Enter: zoom to place");
    const float HintSize = 11.0f * Scale;
    const FVector2D HintExtent = HomesteadMapPaint::FPainter::MeasureText(Hints, HintSize, false);
    const FVector2D HintAt(Local.X - HintExtent.X - 16 * Scale, Local.Y - HintExtent.Y - 12 * Scale);
    Paint.Fill({HintAt - FVector2D(8, 5) * Scale, HintAt + FVector2D(HintExtent.X + 8 * Scale, -5 * Scale),
        HintAt + HintExtent + FVector2D(8, 5) * Scale, HintAt + FVector2D(-8 * Scale, HintExtent.Y + 5 * Scale)}, MvPlate);
    Paint.Text(Hints, HintAt, HintSize, MvCream, false, false);
    // Legend, above the hints.
    const FVector2D LegendAt(HintAt.X, HintAt.Y - 26 * Scale);
    Paint.Fill({LegendAt - FVector2D(8, 5) * Scale, LegendAt + FVector2D(300, -5) * Scale, LegendAt + FVector2D(300, 18) * Scale,
        LegendAt + FVector2D(-8, 18) * Scale}, MvPlate);
    for (int32 Dash = 0; Dash < 3; ++Dash)
        Paint.Segment(LegendAt + FVector2D(Dash * 9, 7) * Scale, LegendAt + FVector2D(Dash * 9 + 6, 7) * Scale, HomesteadMapPaint::BoundaryInk, 2.4f * Scale);
    Paint.Text(TEXT("Your estate"), LegendAt + FVector2D(30, -1) * Scale, 11 * Scale, MvCream, false, false);
    const FVector2D Swatch = LegendAt + FVector2D(140, 0) * Scale;
    Paint.Fill({Swatch, Swatch + FVector2D(22, 0) * Scale, Swatch + FVector2D(22, 14) * Scale, Swatch + FVector2D(0, 14) * Scale},
        FLinearColor(0.6f, 0.5f, 0.35f, 0.9f));
    for (int32 Line = 0; Line < 4; ++Line)
        Paint.Segment(Swatch + FVector2D(Line * 6, 14) * Scale, Swatch + FVector2D(Line * 6 + 8, 0) * Scale, HomesteadMapPaint::Ink, 1.0f);
    Paint.Text(TEXT("For sale"), Swatch + FVector2D(30, -1) * Scale, 11 * Scale, MvCream, false, false);
    return Paint.GetLayer();
}
}
