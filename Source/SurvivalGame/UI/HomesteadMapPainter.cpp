#include "HomesteadMapPainter.h"
#include "HomesteadUITheme.h"
#include "HomesteadPalette.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

namespace HomesteadMapPaint
{
const FLinearColor Parchment(0.80f, 0.70f, 0.50f, 1.0f);
const FLinearColor Ink(0.12f, 0.075f, 0.04f, 1.0f);
const FLinearColor BoundaryInk(0.46f, 0.07f, 0.05f, 1.0f);
const FLinearColor Halo(0.95f, 0.90f, 0.76f, 0.85f);
const FLinearColor Brass(0.92f, 0.74f, 0.43f, 1.0f); // the map is its own parchment in either theme
const FLinearColor RimShadow(0.02f, 0.03f, 0.025f, 0.75f);

namespace
{
const FSlateBrush* White() { return FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")); }

FVector2f ProxyUV(const FSlateResourceHandle& Handle, FVector2D UV)
{
    const FSlateShaderResourceProxy* Proxy = Handle.GetResourceProxy();
    if (!Proxy) return FVector2f(UV);
    return Proxy->StartUV + FVector2f(UV) * Proxy->SizeUV;
}

FColor Vertex(const FLinearColor& Color) { return Color.ToFColor(true); }

FSlateFontInfo Font(float Size, bool bBold)
{
    FSlateFontInfo Info = HomesteadUITheme::Font(bBold ? TEXT("Bold") : TEXT("Regular"), 10);
    Info.Size = FMath::Max(1.0f, Size);
    return Info;
}
}

void FPainter::Fill(const TArray<FVector2D>& Points, const FLinearColor& Color)
{
    if (Points.Num() < 3 || Color.A <= 0.0f) return;
    std::vector<HomesteadMap::Vec> Ring;
    Ring.reserve(Points.Num());
    for (const FVector2D& Point : Points) Ring.push_back({Point.X, Point.Y});
    const std::vector<int> Triangles = HomesteadMap::Triangulate(Ring);
    if (Triangles.empty()) return;
    const FSlateResourceHandle Handle = White()->GetRenderingResource();
    const FVector2f UV = ProxyUV(Handle, FVector2D(0.5, 0.5));
    const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
    TArray<FSlateVertex> Vertices;
    Vertices.Reserve(Points.Num());
    for (const FVector2D& Point : Points)
        Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(Point), UV, Vertex(Color)));
    TArray<SlateIndex> Indices;
    Indices.Reserve(static_cast<int32>(Triangles.size()));
    for (int Index : Triangles) Indices.Add(static_cast<SlateIndex>(Index));
    FSlateDrawElement::MakeCustomVerts(Elements, ++Layer, Handle, Vertices, Indices, nullptr, 0, 0);
}

void FPainter::TexturedFan(const FSlateBrush* Brush, const TArray<FVector2D>& Points, const TArray<FVector2D>& UVs, const FLinearColor& Tint)
{
    if (!Brush || Points.Num() < 3 || Points.Num() != UVs.Num()) return;
    const FSlateResourceHandle Handle = Brush->GetRenderingResource();
    if (!Handle.IsValid()) return;
    const FSlateRenderTransform& Transform = Geometry.GetAccumulatedRenderTransform();
    TArray<FSlateVertex> Vertices;
    Vertices.Reserve(Points.Num());
    for (int32 Index = 0; Index < Points.Num(); ++Index)
        Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, FVector2f(Points[Index]),
            ProxyUV(Handle, UVs[Index]), Vertex(Tint)));
    TArray<SlateIndex> Indices;
    for (int32 Index = 1; Index + 1 < Points.Num(); ++Index)
        Indices.Append({0, static_cast<SlateIndex>(Index), static_cast<SlateIndex>(Index + 1)});
    FSlateDrawElement::MakeCustomVerts(Elements, ++Layer, Handle, Vertices, Indices, nullptr, 0, 0);
}

void FPainter::Lines(const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness, bool bClosed)
{
    if (Points.Num() < 2 || Color.A <= 0.0f) return;
    TArray<FVector2D> Path = Points;
    if (bClosed) Path.Add(Points[0]);
    FSlateDrawElement::MakeLines(Elements, ++Layer, Geometry.ToPaintGeometry(), Path,
        ESlateDrawEffect::NoPixelSnapping, Color, true, Thickness);
}

void FPainter::Segment(FVector2D A, FVector2D B, const FLinearColor& Color, float Thickness)
{
    Lines({A, B}, Color, Thickness);
}

void FPainter::Disc(FVector2D Center, float Radius, const FLinearColor& Color, int32 Segments)
{
    TArray<FVector2D> Points;
    Points.Reserve(Segments);
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float Angle = Index * 2.0f * PI / Segments;
        Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
    Fill(Points, Color);
}

void FPainter::Circle(FVector2D Center, float Radius, const FLinearColor& Color, float Thickness, int32 Segments)
{
    TArray<FVector2D> Points;
    Points.Reserve(Segments);
    for (int32 Index = 0; Index < Segments; ++Index)
    {
        const float Angle = Index * 2.0f * PI / Segments;
        Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
    }
    Lines(Points, Color, Thickness, true);
}

void FPainter::Badge(EHomesteadMapGlyph Kind, FVector2D Center, float Radius, float Opacity, bool bHighlight)
{
    const auto Fade = [Opacity](FLinearColor Color) { Color.A *= Opacity; return Color; };
    Disc(Center + FVector2D(0, Radius * 0.12f), Radius * 1.08f, Fade(FLinearColor(0, 0, 0, 0.35f)), 24);
    Disc(Center, Radius, Fade(FLinearColor(0.95f, 0.89f, 0.72f, 1.0f)), 24);
    Circle(Center, Radius, Fade(bHighlight ? Brass : Ink), FMath::Max(1.0f, Radius * (bHighlight ? 0.2f : 0.12f)), 24);
    Glyph(Kind, Center, Radius * 1.25f, Fade(Ink));
}

void FPainter::Glyph(EHomesteadMapGlyph Kind, FVector2D Center, float Size, const FLinearColor& Color)
{
    // Shapes are authored in a unit box (-1..1, y down) and scaled to half of `Size`.
    const float S = Size * 0.5f;
    const auto P = [&](float X, float Y) { return Center + FVector2D(X, Y) * S; };
    const float Line = FMath::Max(1.0f, Size * 0.09f);
    switch (Kind)
    {
    case EHomesteadMapGlyph::Manor:
        Fill({P(-0.75f, -0.05f), P(0.0f, -0.7f), P(0.75f, -0.05f)}, Color);
        Fill({P(-0.55f, -0.05f), P(0.55f, -0.05f), P(0.55f, 0.6f), P(-0.55f, 0.6f)}, Color);
        Fill({P(0.25f, -0.62f), P(0.42f, -0.62f), P(0.42f, -0.25f), P(0.25f, -0.4f)}, Color);
        break;
    case EHomesteadMapGlyph::Mine:
        // A rural engine house: a gabled block and its tall chimney stack.
        Fill({P(-0.7f, -0.1f), P(-0.2f, -0.45f), P(0.3f, -0.1f), P(0.3f, 0.6f), P(-0.7f, 0.6f)}, Color);
        Fill({P(0.4f, -0.8f), P(0.62f, -0.8f), P(0.68f, 0.6f), P(0.36f, 0.6f)}, Color);
        break;
    case EHomesteadMapGlyph::Cove:
        for (float Y : {-0.25f, 0.2f})
        {
            TArray<FVector2D> Wave;
            for (int32 Step = 0; Step <= 12; ++Step)
            {
                const float X = -0.75f + Step * (1.5f / 12.0f);
                Wave.Add(P(X, Y + FMath::Sin(Step * PI / 3.0f) * 0.14f));
            }
            Lines(Wave, Color, Line);
        }
        break;
    case EHomesteadMapGlyph::Mill:
        Circle(Center, S * 0.62f, Color, Line, 20);
        for (int32 Spoke = 0; Spoke < 4; ++Spoke)
        {
            const float Angle = Spoke * PI / 4.0f;
            const FVector2D D(FMath::Cos(Angle), FMath::Sin(Angle));
            Segment(Center - D * S * 0.62f, Center + D * S * 0.62f, Color, Line * 0.8f);
        }
        break;
    case EHomesteadMapGlyph::Gateway:
        Fill({P(-0.62f, -0.55f), P(-0.36f, -0.55f), P(-0.36f, 0.6f), P(-0.62f, 0.6f)}, Color);
        Fill({P(0.36f, -0.55f), P(0.62f, -0.55f), P(0.62f, 0.6f), P(0.36f, 0.6f)}, Color);
        for (float Y : {-0.2f, 0.15f}) Segment(P(-0.36f, Y), P(0.36f, Y), Color, Line);
        break;
    case EHomesteadMapGlyph::Road:
        Lines({P(-0.7f, 0.6f), P(-0.2f, 0.1f), P(0.2f, -0.05f), P(0.7f, -0.6f)}, Color, Line * 1.4f);
        Lines({P(-0.45f, 0.75f), P(0.05f, 0.3f), P(0.4f, 0.15f), P(0.8f, -0.25f)}, Color, Line * 0.7f);
        break;
    case EHomesteadMapGlyph::Town:
        Fill({P(-0.8f, 0.1f), P(-0.5f, -0.2f), P(-0.2f, 0.1f), P(-0.2f, 0.6f), P(-0.8f, 0.6f)}, Color);
        Fill({P(-0.3f, -0.15f), P(0.05f, -0.55f), P(0.4f, -0.15f), P(0.4f, 0.6f), P(-0.3f, 0.6f)}, Color);
        Fill({P(0.3f, 0.15f), P(0.55f, -0.1f), P(0.8f, 0.15f), P(0.8f, 0.6f), P(0.3f, 0.6f)}, Color);
        break;
    case EHomesteadMapGlyph::Store:
        Fill({P(-0.7f, -0.1f), P(0.7f, -0.1f), P(0.55f, 0.6f), P(-0.55f, 0.6f)}, Color);
        // The striped awning over its door.
        Fill({P(-0.8f, -0.45f), P(0.8f, -0.45f), P(0.7f, -0.15f), P(-0.7f, -0.15f)}, Color);
        Disc(P(0.0f, 0.25f), S * 0.18f, FLinearColor(0.95f, 0.89f, 0.72f, Color.A), 12);
        break;
    }
}

FVector2D FPainter::MeasureText(const FString& Value, float Size, bool bBold)
{
    if (!FSlateApplication::IsInitialized()) return FVector2D(Value.Len() * Size * 0.6f, Size * 1.4f);
    return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value, Font(Size, bBold));
}

FVector2D FPainter::Text(const FString& Value, FVector2D TopLeft, float Size, const FLinearColor& Color, bool bBold, bool bShadow)
{
    const FVector2D Extent = MeasureText(Value, Size, bBold);
    const FSlateFontInfo Info = Font(Size, bBold);
    if (bShadow)
        FSlateDrawElement::MakeText(Elements, ++Layer,
            Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(TopLeft + FVector2D(1, 1) * FMath::Max(1.0f, Size * 0.08f))),
            Value, Info, ESlateDrawEffect::None, FLinearColor(0, 0, 0, 0.7f * Color.A));
    FSlateDrawElement::MakeText(Elements, ++Layer, Geometry.ToPaintGeometry(Extent, FSlateLayoutTransform(TopLeft)),
        Value, Info, ESlateDrawEffect::None, Color);
    return Extent;
}

void FPainter::PlayerArrow(FVector2D Center, FVector2D Direction, float Size)
{
    const FVector2D D = Direction.GetSafeNormal();
    const FVector2D N(-D.Y, D.X);
    const TArray<FVector2D> Shape = {Center + D * Size, Center - D * Size * 0.65f + N * Size * 0.7f,
        Center - D * Size * 0.3f, Center - D * Size * 0.65f - N * Size * 0.7f};
    TArray<FVector2D> Shadow;
    for (const FVector2D& Point : Shape) Shadow.Add(Point + FVector2D(0, Size * 0.12f));
    Fill(Shadow, FLinearColor(0, 0, 0, 0.4f));
    Fill(Shape, FLinearColor(0.99f, 0.96f, 0.86f, 1.0f));
    Lines(Shape, Ink, FMath::Max(1.0f, Size * 0.14f), true);
}
}
