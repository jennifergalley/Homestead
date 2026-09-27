#pragma once

#include "CoreMinimal.h"

/**
 * The fixed estate's ground: a runtime copy of the Estate Landscape's heights, sampled with the
 * landscape's own triangulation. The game uses it wherever the generated woodland used its terrain
 * function (placement, feet, props), so it works away from streamed-in landscape collision too.
 */
namespace HomesteadEstateTerrain
{
constexpr int32 Vertices = 4033;
constexpr double QuadCm = 100.0;
constexpr double HalfExtentCm = (Vertices - 1) * QuadCm / 2.0;

/** Loads Content/SurvivalGame/Estate/Runtime/EstateHeightfield.r16 once; true when ready. */
SURVIVALGAME_API bool Activate();
SURVIVALGAME_API void Deactivate();
SURVIVALGAME_API bool IsActive();
/** Height in cm at a world XY; clamps to the map edge. NaN when inactive or not finite. */
SURVIVALGAME_API float Height(double X, double Y);
SURVIVALGAME_API bool Contains(double X, double Y);
}
