#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "Simulation/HomesteadPail.h"

#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

// Defined in HomesteadController.cpp; declared here so this file doesn't rely on unity-build order.
namespace HomesteadWaterProbe { bool ShoreContains(const USplineComponent& Spline, const FVector2D& Point); }

Homestead::Point AHomesteadController::FreshWaterDipPoint(Homestead::Point Position) const
{
    if (!bEstateMap)
        return {Homestead::StreamX(Position.y), Position.y};

    // Refresh the tagged HomesteadWater spline cache and aim the pail at the nearest fresh-water edge,
    // not the retired procedural creek curve.
    WaterEdgeDistance(Position, false);
    const FVector Here(Position.x, Position.y, GroundHeight(Position.x, Position.y));
    double Best = TNumericLimits<double>::Max();
    FVector BestPoint(Homestead::StreamX(Position.y), Position.y, Here.Z);
    for (const auto& Weak : EstateWaterSplines)
        if (const USplineComponent* Spline = Weak.Get())
        {
            const FVector Center = Spline->FindLocationClosestToWorldLocation(Here, ESplineCoordinateSpace::World);
            const float Key = Spline->FindInputKeyClosestToWorldLocation(Here);
            // Aim a hand's breadth inside the waterline (spline scale Y is the waterline half width).
            const double HalfWidth = FMath::Max(0.0, 100.0 * Spline->GetScaleAtSplineInputKey(Key).Y - PailDipInsideCm);
            const FVector2D Out(Position.x - Center.X, Position.y - Center.Y);
            FVector Edge = Out.SizeSquared() > 1.0
                ? Center + FVector(Out.GetSafeNormal().X * HalfWidth, Out.GetSafeNormal().Y * HalfWidth, 0.0)
                : Center;
            // A lake's shoreline (a closed spline) is the waterline itself: step in from it, away from her.
            if (Spline->IsClosedLoop() && Out.SizeSquared() > 1.0)
            {
                const double Toward = HomesteadWaterProbe::ShoreContains(*Spline, FVector2D(Position.x, Position.y)) ? 1.0 : -1.0;
                Edge = Center + FVector(Out.GetSafeNormal() * (Toward * PailDipInsideCm), 0.0);
            }
            const double Distance = FVector::Dist2D(Edge, Here);
            if (Distance < Best)
            {
                Best = Distance;
                BestPoint = Edge;
            }
        }
    return {BestPoint.X, BestPoint.Y};
}

void AHomesteadController::FillPailAtStream(Homestead::Point Position)
{
    const auto Result = Sim.FillWater(Position);
    // Jenny's concise rule: the gauge and the dip show a full pail, so only a refusal toasts.
    NotifyResourceAction(Result);
    // She kneels at the bank and dips the pail into the nearest authored fresh-water ribbon; standing in
    // the shallows, she dips where she stands instead (Jenny, 2026-09-30: fill in the water, not from the bank).
    if (Result.ok)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
            Avatar->PlayFillPail(WaterEdgeDistance(Position, false) < 0.0
                ? InWaterDipPoint(Position, Avatar->GetActorRotation().Yaw) : FreshWaterDipPoint(Position));
}

Homestead::Point AHomesteadController::InWaterDipPoint(Homestead::Point Position, double Yaw) const
{
    // Anywhere in the water (review: a threshold left her turning back to the bank from 0-30 cm in).
    return Homestead::InWaterDipPoint(Position, Yaw, AHomesteadCharacter::FillForward, AHomesteadCharacter::FillRight,
        PailDipInsideCm, [this](Homestead::Point Point) { return WaterEdgeDistance(Point, false); });
}

double AHomesteadController::WaterEdgeDistance(Homestead::Point Position, bool bIncludeSea) const
{
    if (!bEstateMap)
        return FMath::Abs(Position.x - Homestead::StreamX(Position.y)) - Homestead::Generation::StreamWaterHalfWidthCm;
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (Now - EstateWaterScanTime > 5.0 || Now < EstateWaterScanTime)
    {
        // Rivers and ponds are splines tagged HomesteadWater in the Estate level.
        EstateWaterScanTime = Now;
        EstateWaterSplines.Reset();
        if (UWorld* World = GetWorld())
            for (TActorIterator<AActor> It(World); It; ++It)
                if (It->ActorHasTag(TEXT("HomesteadWater")))
                    for (USplineComponent* Spline : TInlineComponentArray<USplineComponent*>(*It))
                        EstateWaterSplines.Add(Spline);
    }
    double Best = TNumericLimits<double>::Max();
    const FVector Here(Position.x, Position.y, GroundHeight(Position.x, Position.y));
    for (const auto& Weak : EstateWaterSplines)
        if (const USplineComponent* Spline = Weak.Get())
        {
            const FVector Point = Spline->FindLocationClosestToWorldLocation(Here, ESplineCoordinateSpace::World);
            const float Key = Spline->FindInputKeyClosestToWorldLocation(Here);
            const double HalfWidth = 100.0 * Spline->GetScaleAtSplineInputKey(Key).Y;
            double Distance = FVector::Dist2D(Point, Here) - HalfWidth;
            // A lake (closed shoreline): inside it is in the water.
            if (Spline->IsClosedLoop() && HomesteadWaterProbe::ShoreContains(*Spline, FVector2D(Position.x, Position.y)))
                Distance = -Distance;
            Best = FMath::Min(Best, Distance);
        }
    if (!bIncludeSea)
        return Best;
    // The sea and the estuary: ground below sea level within a couple of metres.
    if (HomesteadEstateTerrain::Height(Position.x, Position.y) < -15.0f)
        return 0.0;
    for (const double Radius : {60.0, 120.0, 180.0, 240.0})
        for (int32 Step = 0; Step < 8; ++Step)
        {
            const double Angle = Step * UE_PI / 4.0;
            if (HomesteadEstateTerrain::Height(Position.x + Radius * FMath::Cos(Angle), Position.y + Radius * FMath::Sin(Angle)) < -15.0f)
                return FMath::Min(Best, Radius);
        }
    return Best;
}
