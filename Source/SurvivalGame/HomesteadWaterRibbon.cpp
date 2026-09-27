#include "HomesteadWaterRibbon.h"

#include "Components/SplineComponent.h"
#include "Engine/CollisionProfile.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

AHomesteadWaterRibbon::AHomesteadWaterRibbon()
{
    PrimaryActorTick.bCanEverTick = false;
    Spline = CreateDefaultSubobject<USplineComponent>(TEXT("Course"));
    SetRootComponent(Spline);
    Spline->SetMobility(EComponentMobility::Static);
    Surface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Surface"));
    Surface->SetupAttachment(Spline);
    Surface->SetMobility(EComponentMobility::Static);
    Surface->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
    Surface->SetCastShadow(false);
    Surface->SetCanEverAffectNavigation(false);
    Surface->bUseAsyncCooking = true;
    Tags.Add(TEXT("HomesteadWater"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Creek(TEXT("/Game/SurvivalGame/Materials/M_CreekWater.M_CreekWater"));
    if (Creek.Succeeded()) Material = Creek.Object;
}

void AHomesteadWaterRibbon::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    RebuildSurface();
}

void AHomesteadWaterRibbon::SetCourse(const TArray<FVector>& WorldPoints, const TArray<float>& HalfWidthsCm)
{
    Spline->ClearSplinePoints(false);
    for (int32 Index = 0; Index < WorldPoints.Num(); ++Index)
    {
        Spline->AddSplinePoint(WorldPoints[Index], ESplineCoordinateSpace::World, false);
        const float Half = HalfWidthsCm.IsValidIndex(Index) ? HalfWidthsCm[Index] : 200.0f;
        Spline->SetScaleAtSplinePoint(Index, FVector(1.0f, Half / 100.0f, 1.0f), false);
    }
    Spline->UpdateSpline();
    RebuildSurface();
}

void AHomesteadWaterRibbon::RebuildSurface()
{
    Surface->ClearAllMeshSections();
    const float Length = Spline->GetSplineLength();
    if (Length < 1.0f || Spline->GetNumberOfSplinePoints() < 2)
        return;
    const int32 Rows = FMath::Max(1, FMath::CeilToInt32(Length / SegmentLength));
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    const FTransform ToLocal = GetActorTransform().Inverse();
    for (int32 Row = 0; Row <= Rows; ++Row)
    {
        const float Distance = Length * Row / Rows;
        const FVector Centre = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector Right = Spline->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        Right.Z = 0;
        Right = Right.GetSafeNormal();
        FVector Forward = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        Forward.Z = 0;
        Forward = Forward.GetSafeNormal();
        const float Half = 100.0f * Spline->GetScaleAtDistanceAlongSpline(Distance).Y;
        for (int32 Column = 0; Column <= Columns; ++Column)
        {
            const float Across = (static_cast<float>(Column) / Columns - 0.5f) * 2.0f;
            const FVector World = Centre + Right * (Across * Half);
            Vertices.Add(ToLocal.TransformPosition(World));
            Normals.Add(FVector::UpVector);
            // Same convention as the woodland creek: U across in metres, V along the flow in metres.
            UV.Add(FVector2D(Across * Half / 100.0, Distance / 100.0));
            // R: depth proxy for the shallows tint (shallow at the banks), G: foam near the banks.
            const float Edge = 1.0f - FMath::Abs(Across);
            Colors.Add(FLinearColor(FMath::Clamp(Edge * 1.4f, 0.0f, 1.0f), FMath::Clamp(Edge * 3.0f, 0.0f, 1.0f), 0, 1));
            Tangents.Add(FProcMeshTangent(ToLocal.TransformVectorNoScale(Forward), false));
        }
    }
    for (int32 Row = 0; Row < Rows; ++Row)
        for (int32 Column = 0; Column < Columns; ++Column)
        {
            const int32 A = Row * (Columns + 1) + Column;
            const int32 C = A + Columns + 1;
            // Columns run toward the right bank, so this winding faces up.
            Triangles.Append({A, A + 1, C, A + 1, C + 1, C});
        }
    Surface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, false);
    if (Material) Surface->SetMaterial(0, Material);
}
