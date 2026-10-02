#include "HomesteadWaterPool.h"

#include "Components/SplineComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

namespace HomesteadPoolStyle
{
// M_CreekWater-family vertex colour: R reaches 1 at 35 cm deep (less shore froth), G at 8 cm (full
// highlight); B (white water) stays 0 on still water.
constexpr float PoolDepthForClearCm = 35.0f;
constexpr float PoolDepthForShineCm = 8.0f;
// UVs are metres from the pool's centre (the creek graph tiles per metre), so half-precision UVs stay
// fine: a 100 m pool spans +/-50, where half floats still resolve 3 cm.
constexpr float PoolCmPerUV = 100.0f;
// The wading wall runs from below the deepest bed (lake_basin.py MAX_DEPTH 1.8 m) to above her head.
constexpr float WadeWallBelowCm = 250.0f;
constexpr float WadeWallAboveCm = 300.0f;
}

AHomesteadWaterPool::AHomesteadWaterPool()
{
    PrimaryActorTick.bCanEverTick = false;
    Shore = CreateDefaultSubobject<USplineComponent>(TEXT("Shore"));
    SetRootComponent(Shore);
    Shore->SetMobility(EComponentMobility::Static);
    Shore->SetClosedLoop(true);
    Surface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Surface"));
    Surface->SetupAttachment(Shore);
    Surface->SetMobility(EComponentMobility::Static);
    Surface->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    Surface->SetCastShadow(false);
    Surface->SetCanEverAffectNavigation(false);
    Surface->bUseAsyncCooking = true;
    WadeLimit = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WadeLimit"));
    WadeLimit->SetupAttachment(Shore);
    WadeLimit->SetMobility(EComponentMobility::Static);
    WadeLimit->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    WadeLimit->SetCollisionObjectType(ECC_WorldStatic);
    WadeLimit->SetCollisionResponseToAllChannels(ECR_Ignore);
    WadeLimit->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    WadeLimit->SetVisibility(false);
    WadeLimit->SetHiddenInGame(true);
    WadeLimit->SetCastShadow(false);
    WadeLimit->SetCanEverAffectNavigation(false);
    WadeLimit->bUseAsyncCooking = true;
    Tags.Add(TEXT("HomesteadWater"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Creek(TEXT("/Game/SurvivalGame/Materials/M_CreekWater.M_CreekWater"));
    if (Creek.Succeeded()) Material = Creek.Object;
}

void AHomesteadWaterPool::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildSurface();
}

void AHomesteadWaterPool::SetShore(const TArray<FVector2D>& WorldPoints, float WaterZ)
{
    Shore->ClearSplinePoints(false);
    for (int32 Index = 0; Index < WorldPoints.Num(); ++Index)
    {
        Shore->AddSplinePoint(FVector(WorldPoints[Index], WaterZ), ESplineCoordinateSpace::World, false);
        Shore->SetSplinePointType(Index, ESplinePointType::CurveClamped, false);
        // Scale Y 0: the shoreline itself is the waterline (the probe reads scale Y as a half width).
        Shore->SetScaleAtSplinePoint(Index, FVector(1.0f, 0.0f, 1.0f), false);
    }
    Shore->SetClosedLoop(true, false);
    Shore->UpdateSpline();
    RebuildSurface();
}

void AHomesteadWaterPool::RebuildSurface()
{
    using namespace HomesteadPoolStyle;
    Surface->ClearAllMeshSections();
    WadeLimit->ClearAllMeshSections();
    const float Length = Shore->GetSplineLength();
    if (Length < 100.0f || Shore->GetNumberOfSplinePoints() < 3)
        return;

    // The shoreline, and the centre it's star-shaped about (the estate lake's outline is a wobbly
    // ellipse, so every shore point sees the centroid).
    TArray<FVector> Edge;
    FVector Centre = FVector::ZeroVector;
    for (int32 Sample = 0; Sample < ShoreSamples; ++Sample)
    {
        Edge.Add(Shore->GetLocationAtDistanceAlongSpline(Length * Sample / ShoreSamples, ESplineCoordinateSpace::World));
        Centre += Edge.Last();
    }
    Centre /= Edge.Num();
    const float WaterZ = Centre.Z;
    // Wind the triangles to face up whichever way the shoreline runs (positive area = anticlockwise
    // in X, Y).
    double TwiceArea = 0.0;
    for (int32 Index = 0; Index < Edge.Num(); ++Index)
    {
        const FVector& P = Edge[Index];
        const FVector& Q = Edge[(Index + 1) % Edge.Num()];
        TwiceArea += P.X * Q.Y - Q.X * P.Y;
    }
    const bool bAnticlockwise = TwiceArea > 0.0;

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    const FTransform ToLocal = GetActorTransform().Inverse();
    // Ring 0 lies BankOverlap outside the shore, ring 1 on it, then rings step in to the centre.
    const int32 RingCount = Rings + 1;
    for (int32 Ring = 0; Ring <= RingCount; ++Ring)
        for (int32 Sample = 0; Sample < Edge.Num(); ++Sample)
        {
            const FVector2D FromCentre(Edge[Sample].X - Centre.X, Edge[Sample].Y - Centre.Y);
            const float Radius = FromCentre.Size();
            const FVector2D Out = Radius > 1.0f ? FromCentre / Radius : FVector2D(1, 0);
            // Distance in from the shore (negative under the bank).
            const float Inward = Ring == 0 ? -BankOverlap
                : Radius * FMath::Pow(static_cast<float>(Ring - 1) / Rings, 0.7f);
            const FVector2D Point = FVector2D(Centre.X, Centre.Y) + Out * (Radius - Inward);
            Vertices.Add(ToLocal.TransformPosition(FVector(Point, WaterZ)));
            Normals.Add(FVector::UpVector);
            UV.Add(FVector2D((Point.X - Centre.X) / PoolCmPerUV, (Point.Y - Centre.Y) / PoolCmPerUV));
            const float Depth = FMath::Max(0.0f, Inward) * ShelfSlope;
            Colors.Add(FLinearColor(FMath::Clamp(Depth / PoolDepthForClearCm, 0.0f, 1.0f),
                FMath::Clamp(Depth / PoolDepthForShineCm, 0.0f, 1.0f), 0.0f, 1.0f));
            Tangents.Add(FProcMeshTangent(ToLocal.TransformVectorNoScale(FVector(0, 1, 0)), false));
        }
    const int32 Count = Edge.Num();
    for (int32 Ring = 0; Ring < RingCount; ++Ring)
        for (int32 Sample = 0; Sample < Count; ++Sample)
        {
            const int32 A = Ring * Count + Sample;
            const int32 B = Ring * Count + (Sample + 1) % Count;
            const int32 C = A + Count;
            const int32 D = B + Count;
            if (bAnticlockwise)
                Triangles.Append({A, C, B, B, C, D});
            else
                Triangles.Append({A, B, C, B, D, C});
        }
    Surface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, false);
    if (Material) Surface->SetMaterial(0, Material);

    // The wading limit: a ring of wall WadeInset in from the shore, from below the bed to above her head,
    // wound both ways so it blocks from either side.
    TArray<FVector> WallVertices;
    TArray<int32> WallTriangles;
    for (int32 Sample = 0; Sample < Count; ++Sample)
    {
        const FVector2D FromCentre(Edge[Sample].X - Centre.X, Edge[Sample].Y - Centre.Y);
        const float Radius = FromCentre.Size();
        const FVector2D Out = Radius > 1.0f ? FromCentre / Radius : FVector2D(1, 0);
        const FVector2D Point = FVector2D(Centre.X, Centre.Y) + Out * FMath::Max(Radius - WadeInset, Radius * 0.2f);
        WallVertices.Add(ToLocal.TransformPosition(FVector(Point, WaterZ - WadeWallBelowCm)));
        WallVertices.Add(ToLocal.TransformPosition(FVector(Point, WaterZ + WadeWallAboveCm)));
    }
    for (int32 Sample = 0; Sample < Count; ++Sample)
    {
        const int32 A = Sample * 2, B = ((Sample + 1) % Count) * 2;
        WallTriangles.Append({A, A + 1, B, B, A + 1, B + 1, A, B, A + 1, B, B + 1, A + 1});
    }
    WadeLimit->CreateMeshSection_LinearColor(0, WallVertices, WallTriangles, {}, {}, {}, {}, true);
}
