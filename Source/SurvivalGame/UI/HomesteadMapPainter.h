#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"
#include "../HomesteadMapComponent.h"
#include "Layout/Geometry.h"

class FSlateWindowElementList;
struct FSlateBrush;

// Drawing helpers shared by the minimap and the Map tab, all in the widget's local space.
namespace HomesteadMapPaint
{
// Map colours: a parchment-and-ink palette that reads over the baked map.
extern const FLinearColor Parchment;
extern const FLinearColor Ink;
extern const FLinearColor BoundaryInk;
extern const FLinearColor Halo;
extern const FLinearColor Brass;
extern const FLinearColor RimShadow;

class FPainter
{
public:
    FPainter(const FGeometry& InGeometry, FSlateWindowElementList& InElements, int32 InLayer)
        : Geometry(InGeometry), Elements(InElements), Layer(InLayer) {}

    // A simple polygon, filled (ear-clipped).
    void Fill(const TArray<FVector2D>& Points, const FLinearColor& Color);
    // Textured triangles: `Points` with map UVs (0..1 across the texture), fanned from the first.
    void TexturedFan(const FSlateBrush* Brush, const TArray<FVector2D>& Points, const TArray<FVector2D>& UVs, const FLinearColor& Tint);
    void Lines(const TArray<FVector2D>& Points, const FLinearColor& Color, float Thickness, bool bClosed = false);
    void Segment(FVector2D A, FVector2D B, const FLinearColor& Color, float Thickness);
    void Disc(FVector2D Center, float Radius, const FLinearColor& Color, int32 Segments = 32);
    void Circle(FVector2D Center, float Radius, const FLinearColor& Color, float Thickness, int32 Segments = 48);
    // A landmark badge: a round paper token with the glyph inked on it.
    void Badge(EHomesteadMapGlyph Glyph, FVector2D Center, float Radius, float Opacity = 1.0f, bool bHighlight = false);
    void Glyph(EHomesteadMapGlyph Glyph, FVector2D Center, float Size, const FLinearColor& Color);
    // Text whose top-left is at `TopLeft`; returns its size.
    FVector2D Text(const FString& Value, FVector2D TopLeft, float Size, const FLinearColor& Color, bool bBold = true, bool bShadow = true);
    static FVector2D MeasureText(const FString& Value, float Size, bool bBold = true);
    // Her position and facing: a cream arrowhead with an ink edge.
    void PlayerArrow(FVector2D Center, FVector2D Direction, float Size);
    int32 NextLayer() { return ++Layer; }
    int32 GetLayer() const { return Layer; }

private:
    const FGeometry& Geometry;
    FSlateWindowElementList& Elements;
    int32 Layer;
};
}
