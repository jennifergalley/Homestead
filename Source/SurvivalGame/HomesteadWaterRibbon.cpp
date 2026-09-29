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

namespace HomesteadRibbonStyle
{
// M_CreekWater's vertex colour: R reaches 1 at 35 cm deep (less froth), G at 8 cm (full highlight).
constexpr float DepthForClearCm = 35.0f;
constexpr float DepthForShineCm = 8.0f;
// Depth gained per centimetre in from the waterline: the graded banks rise 0.6 m per metre
// (Scripts/Terrain/river_channel.py BANK_SLOPE).
constexpr float BankRise = 0.6f;
// Riffles: froth builds from a 3.5 % grade and is full at 11.5 %.
constexpr float RiffleGrade = 0.035f;
constexpr float RiffleGradeRange = 0.08f;
// Rows are this much closer together along the rounded ends, so the caps stay round.
constexpr float CapRowDivisor = 4.0f;
// M_CreekWater tiles its ripples 0.8 and 2.1 times and its foam 1.3 times per metre, so all three
// repeat whole every 10 m: V can restart there without a seam.
constexpr float UVRepeatCm = 1000.0f;

// Circular rounding: 0 at the very end, 1 once a full cap length in.
float CapWidth(float Along, float Cap)
{
    if (Cap <= 1.0f || Along >= Cap) return 1.0f;
    const float T = FMath::Clamp(Along / Cap, 0.0f, 1.0f);
    return FMath::Sqrt(FMath::Max(0.0f, 1.0f - FMath::Square(1.0f - T)));
}
}

void AHomesteadWaterRibbon::SetCourse(const TArray<FVector>& WorldPoints, const TArray<float>& HalfWidthsCm)
{
    Spline->ClearSplinePoints(false);
    for (int32 Index = 0; Index < WorldPoints.Num(); ++Index)
    {
        Spline->AddSplinePoint(WorldPoints[Index], ESplineCoordinateSpace::World, false);
        // Clamped tangents keep the surface from bulging above the banks where the bed steps down.
        Spline->SetSplinePointType(Index, ESplinePointType::CurveClamped, false);
        const float Half = HalfWidthsCm.IsValidIndex(Index) ? HalfWidthsCm[Index] : 200.0f;
        Spline->SetScaleAtSplinePoint(Index, FVector(1.0f, Half / 100.0f, 1.0f), false);
    }
    Spline->UpdateSpline();
    RebuildSurface();
}

void AHomesteadWaterRibbon::RebuildSurface()
{
    using namespace HomesteadRibbonStyle;
    Surface->ClearAllMeshSections();
    const float Length = Spline->GetSplineLength();
    if (Length < 1.0f || Spline->GetNumberOfSplinePoints() < 2)
        return;
    const float SourceCap = StartCap > 0.0f ? StartCap
        : 100.0f * Spline->GetScaleAtDistanceAlongSpline(0.0f).Y + BankOverlap;
    const float MouthCap = FMath::Min(EndCap, Length * 0.5f);
    TArray<float> Stations;
    for (float Distance = 0.0f; Distance < Length;)
    {
        Stations.Add(Distance);
        const bool bNearEnd = Distance < SourceCap * 1.2f || Length - Distance < MouthCap * 1.2f;
        Distance += bNearEnd ? SegmentLength / CapRowDivisor : SegmentLength;
    }
    Stations.Add(Length);
    // V restarts every UVRepeatCm (the mesh's UVs are half precision, so metres along a 1.3 km river
    // snap to whole numbers and smear the ripples into stripes). Each restart is a doubled row: one
    // ending the block at V = UVRepeatCm / 100, one starting the next at 0, with no quads between.
    for (float Seam = UVRepeatCm; Seam < Length - 1.0f; Seam += UVRepeatCm)
        Stations.Add(Seam);
    Stations.Sort();
    struct FRibbonRow { float Distance; float V; bool bJoinPrevious; };
    TArray<FRibbonRow> RibbonRows;
    for (const float Distance : Stations)
    {
        if (!RibbonRows.IsEmpty() && Distance - RibbonRows.Last().Distance < 1.0f)
            continue;
        const float Nearest = FMath::RoundToFloat(Distance / UVRepeatCm);
        if (Nearest > 0.0f && FMath::Abs(Distance - Nearest * UVRepeatCm) < 0.5f && Distance < Length - 1.0f)
        {
            RibbonRows.Add({Distance, UVRepeatCm / 100.0f, true});
            RibbonRows.Add({Distance, 0.0f, false});
        }
        else
        {
            const float Local = Distance - FMath::FloorToFloat(Distance / UVRepeatCm) * UVRepeatCm;
            RibbonRows.Add({Distance, Local / 100.0f, !RibbonRows.IsEmpty()});
        }
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    const FTransform ToLocal = GetActorTransform().Inverse();
    for (const FRibbonRow& Row : RibbonRows)
    {
        const float Distance = Row.Distance;
        const FVector Centre = Spline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector Right = Spline->GetRightVectorAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        Right.Z = 0;
        Right = Right.GetSafeNormal();
        const FVector Heading = Spline->GetDirectionAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::World);
        FVector Forward = Heading;
        Forward.Z = 0;
        Forward = Forward.GetSafeNormal();
        const float Grade = FMath::Abs(Heading.Z) / FMath::Max(Heading.Size2D(), 0.05f);
        const float Rounding = FMath::Min(CapWidth(Distance, SourceCap), CapWidth(Length - Distance, MouthCap));
        const float Wet = 100.0f * Spline->GetScaleAtDistanceAlongSpline(Distance).Y * Rounding;
        const float Outer = Wet + BankOverlap * Rounding;
        const float Riffle = FMath::Clamp((Grade - RiffleGrade) / RiffleGradeRange, 0.0f, 1.0f);
        const float Spring = SpringFroth > 1.0f ? 1.0f - FMath::SmoothStep(0.0f, SpringFroth, Distance) : 0.0f;
        const float Churn = FMath::Max(Riffle, 0.8f * Spring);
        for (int32 Column = 0; Column <= Columns; ++Column)
        {
            const float Across = (static_cast<float>(Column) / Columns - 0.5f) * 2.0f;
            const FVector World = Centre + Right * (Across * Outer);
            Vertices.Add(ToLocal.TransformPosition(World));
            Normals.Add(FVector::UpVector);
            // Same convention as the woodland creek: U across in metres, V along the flow in metres.
            UV.Add(FVector2D(Across * Outer / 100.0, Row.V));
            // R: depth over the bed (froth thins with depth); G: the shoreline highlight, both zero
            // under the banks; B: white water, on riffles and where the spring wells up.
            const float Depth = FMath::Max(0.0f, Wet - FMath::Abs(Across) * Outer) * BankRise;
            const float Clear = FMath::Clamp(Depth / DepthForClearCm, 0.0f, 1.0f);
            Colors.Add(FLinearColor(Clear, FMath::Clamp(Depth / DepthForShineCm, 0.0f, 1.0f), Churn, 1));
            Tangents.Add(FProcMeshTangent(ToLocal.TransformVectorNoScale(Forward), false));
        }
    }
    for (int32 Row = 0; Row + 1 < RibbonRows.Num(); ++Row)
        for (int32 Column = 0; Column < Columns && RibbonRows[Row + 1].bJoinPrevious; ++Column)
        {
            const int32 A = Row * (Columns + 1) + Column;
            const int32 C = A + Columns + 1;
            // Columns run toward the right bank, so this winding faces up.
            Triangles.Append({A, A + 1, C, A + 1, C + 1, C});
        }
    Surface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, false);
    if (Material) Surface->SetMaterial(0, Material);
}
