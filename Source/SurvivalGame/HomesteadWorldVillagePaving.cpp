// The village's paved square and lanes (village-path-and-dressing): one continuous cobble surface rather than
// laid slabs, so there are no seams, steps or lifted edges. Scripts/Terrain/village_dress.py writes the shapes
// (a rounded rectangle, round holes for the flower beds, lane segments) into HomesteadVillagePaving.inc; this
// builds them as a single mesh that lies on the terrain's own triangles a few cm above it, fading down to the
// ground at its edge. The texture is the cobble tile's, repeated in world space. Nothing here is game state.
#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadEstateTerrain.h"

#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

namespace VillagePavingStyle
{
struct FRect { float X, Y, HalfX, HalfY, Corner; };
struct FHole { float X, Y, Radius; };
struct FLane { float X0, Y0, X1, Y1, Half; };

#define PAVE_RECT(X, Y, HX, HY, C) {X, Y, HX, HY, C},
#define PAVE_HOLE(X, Y, R)
#define PAVE_LANE(X0, Y0, X1, Y1, H)
constexpr FRect Rects[] = {
#include "HomesteadVillagePaving.inc"
};
#undef PAVE_RECT
#undef PAVE_HOLE
#undef PAVE_LANE

#define PAVE_RECT(X, Y, HX, HY, C)
#define PAVE_HOLE(X, Y, R) {X, Y, R},
#define PAVE_LANE(X0, Y0, X1, Y1, H)
constexpr FHole Holes[] = {
#include "HomesteadVillagePaving.inc"
};
#undef PAVE_RECT
#undef PAVE_HOLE
#undef PAVE_LANE

#define PAVE_RECT(X, Y, HX, HY, C)
#define PAVE_HOLE(X, Y, R)
#define PAVE_LANE(X0, Y0, X1, Y1, H) {X0, Y0, X1, Y1, H},
constexpr FLane Lanes[] = {
#include "HomesteadVillagePaving.inc"
};
#undef PAVE_RECT
#undef PAVE_HOLE
#undef PAVE_LANE

// The cell size must divide the terrain's 100 cm spacing and the mesh's diagonals must match the terrain's
// (corner (x, y+1) to (x+1, y)): then every paving triangle lies exactly on a terrain triangle.
constexpr double TerrainSpacingCm = 100.0;
constexpr double CellCm = 50.0;
constexpr float RestLiftCm = 0.4f;       // at the edge: just clear of the turf so the two don't flicker
constexpr float FullLiftCm = 2.5f;       // inside the feather: the cobbles stand this far above the ground line
constexpr float FeatherCm = 60.0f;       // the edge rises from the ground to its full height over this distance
constexpr float EdgeWobbleCm = 14.0f;    // the outline drifts by up to this much so it reads as laid by hand
constexpr float TextureRepeatCm = 400.0f; // the cobble tile's texture spans one 4 m period (village_cobble_tile.py)
constexpr TCHAR MaterialPath[] = TEXT("/Game/SurvivalGame/Environment/Props/VillageCobbleTile/MI_VillageCobbleTile.MI_VillageCobbleTile");

float TerrainZ(double X, double Y)
{
    const float Height = HomesteadEstateTerrain::Height(X, Y);
    return FMath::IsFinite(Height) ? Height : 0.0f;
}

// The terrain mesh's own surface: its quads split along the (x, y+1) - (x+1, y) diagonal.
double RenderedZ(double X, double Y)
{
    const double LX = X / TerrainSpacingCm;
    const double LY = Y / TerrainSpacingCm;
    const double X0 = FMath::FloorToDouble(LX);
    const double Y0 = FMath::FloorToDouble(LY);
    const double FX = LX - X0;
    const double FY = LY - Y0;
    const auto H = [&](double Dx, double Dy) { return static_cast<double>(TerrainZ((X0 + Dx) * TerrainSpacingCm, (Y0 + Dy) * TerrainSpacingCm)); };
    return FX + FY <= 1.0
        ? H(0, 0) + FX * (H(1, 0) - H(0, 0)) + FY * (H(0, 1) - H(0, 0))
        : H(1, 1) + (1 - FX) * (H(0, 1) - H(1, 1)) + (1 - FY) * (H(1, 0) - H(1, 1));
}

// Signed distance in cm to the paving's edge: positive inside.
float PavedDepth(double X, double Y)
{
    const FVector2D P(X, Y);
    float Depth = -1.0e9f;
    for (const FRect& Rect : Rects)
    {
        const FVector2D Q(FMath::Abs(X - Rect.X) - (Rect.HalfX - Rect.Corner), FMath::Abs(Y - Rect.Y) - (Rect.HalfY - Rect.Corner));
        const double Outside = FVector2D(FMath::Max(Q.X, 0.0), FMath::Max(Q.Y, 0.0)).Size();
        Depth = FMath::Max(Depth, static_cast<float>(Rect.Corner - (Outside + FMath::Min(FMath::Max(Q.X, Q.Y), 0.0))));
    }
    for (const FLane& Lane : Lanes)
    {
        const FVector2D A(Lane.X0, Lane.Y0);
        const FVector2D B(Lane.X1, Lane.Y1);
        const FVector2D AB = B - A;
        const double T = FMath::Clamp(FVector2D::DotProduct(P - A, AB) / FMath::Max(AB.SizeSquared(), 1.0), 0.0, 1.0);
        Depth = FMath::Max(Depth, static_cast<float>(Lane.Half - (P - (A + AB * T)).Size()));
    }
    for (const FHole& Hole : Holes)
        Depth = FMath::Min(Depth, static_cast<float>((P - FVector2D(Hole.X, Hole.Y)).Size() - Hole.Radius));
    return Depth + EdgeWobbleCm * static_cast<float>(FMath::Sin(X * 0.021 + 1.3) * FMath::Sin(Y * 0.017 + 0.4)
        + 0.5 * FMath::Sin(X * 0.053 + Y * 0.041));
}
}

void AHomesteadWorld::BuildVillagePaving()
{
    using namespace VillagePavingStyle;
    if (!HomesteadEstateTerrain::IsActive()) return;

    double MinX = 1.0e18, MinY = 1.0e18, MaxX = -1.0e18, MaxY = -1.0e18;
    const auto Grow = [&](double X, double Y, double Reach)
    {
        MinX = FMath::Min(MinX, X - Reach); MaxX = FMath::Max(MaxX, X + Reach);
        MinY = FMath::Min(MinY, Y - Reach); MaxY = FMath::Max(MaxY, Y + Reach);
    };
    for (const FRect& Rect : Rects) Grow(Rect.X, Rect.Y, FMath::Max(Rect.HalfX, Rect.HalfY));
    for (const FLane& Lane : Lanes) { Grow(Lane.X0, Lane.Y0, Lane.Half); Grow(Lane.X1, Lane.Y1, Lane.Half); }
    if (MinX > MaxX) return;
    MinX = FMath::FloorToDouble((MinX - CellCm) / CellCm) * CellCm;
    MinY = FMath::FloorToDouble((MinY - CellCm) / CellCm) * CellCm;
    const int32 NX = FMath::CeilToInt((MaxX + CellCm - MinX) / CellCm);
    const int32 NY = FMath::CeilToInt((MaxY + CellCm - MinY) / CellCm);

    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UV;
    TArray<FProcMeshTangent> Tangents;
    TArray<float> Depths;
    Vertices.Reserve((NX + 1) * (NY + 1));
    Normals.Reserve((NX + 1) * (NY + 1));
    UV.Reserve((NX + 1) * (NY + 1));
    Tangents.Reserve((NX + 1) * (NY + 1));
    Depths.Reserve((NX + 1) * (NY + 1));
    for (int32 J = 0; J <= NY; ++J)
    {
        for (int32 I = 0; I <= NX; ++I)
        {
            const double X = MinX + I * CellCm;
            const double Y = MinY + J * CellCm;
            const float Depth = PavedDepth(X, Y);
            const float Lift = RestLiftCm + (FullLiftCm - RestLiftCm) * FMath::Clamp(Depth / FeatherCm, 0.0f, 1.0f);
            Vertices.Add(FVector(X, Y, RenderedZ(X, Y) + Lift));
            Normals.Add(FVector(RenderedZ(X - CellCm, Y) - RenderedZ(X + CellCm, Y),
                RenderedZ(X, Y - CellCm) - RenderedZ(X, Y + CellCm), 2.0 * CellCm).GetSafeNormal());
            UV.Add(FVector2D(X / TextureRepeatCm, Y / TextureRepeatCm));
            Tangents.Add(FProcMeshTangent(FVector(1.0, 0.0, 0.0), false));
            Depths.Add(Depth);
        }
    }

    TArray<int32> Triangles;
    for (int32 J = 0; J < NY; ++J)
    {
        for (int32 I = 0; I < NX; ++I)
        {
            const int32 A = J * (NX + 1) + I;
            if (FMath::Max(FMath::Max(Depths[A], Depths[A + 1]), FMath::Max(Depths[A + NX + 1], Depths[A + NX + 2])) <= 0.0f) continue;
            Triangles.Append({A, A + NX + 1, A + 1, A + 1, A + NX + 1, A + NX + 2});
        }
    }
    if (Triangles.IsEmpty()) return;

    auto* Mesh = NewObject<UProceduralMeshComponent>(this, NAME_None, RF_Transient);
    Mesh->SetupAttachment(GetRootComponent());
    Mesh->SetMobility(EComponentMobility::Static);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCanEverAffectNavigation(false);
    Mesh->SetGenerateOverlapEvents(false);
    Mesh->SetCastShadow(false);
    Mesh->bUseAsyncCooking = false;
    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UV, TArray<FColor>(), Tangents, false);
    if (UMaterialInterface* Cobbles = LoadObject<UMaterialInterface>(nullptr, MaterialPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
        Mesh->SetMaterial(0, Cobbles);
    else
        UE_LOG(LogHomesteadWorld, Log, TEXT("Village paving: the cobble material is not imported; the paving is left plain."));
    Mesh->RegisterComponent();
    VillageVisual.Components.Add(Mesh);
    UE_LOG(LogHomesteadWorld, Log, TEXT("Village paving: %d triangles."), Triangles.Num() / 3);
}
