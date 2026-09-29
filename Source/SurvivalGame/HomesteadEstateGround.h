#pragma once

#include "CoreMinimal.h"

/**
 * The fixed estate's ground cover, baked by Scripts/Terrain/bake_ground.py into
 * Content/SurvivalGame/Estate/Runtime/EstateGround.bin: for each of Cells x Cells squares over the map,
 * how much meadow grass grows there (the most anywhere in the square) and what she walks on.
 */
namespace HomesteadEstateGround
{
// Surface bytes; bake_ground.py's SURFACES mirrors them.
enum class ESurface : uint8 { Soil, Grass, Road, Sand, Rock, Woodland, Moor, Water };

/** Loads EstateGround.bin once; true when ready. */
SURVIVALGAME_API bool Activate();
SURVIVALGAME_API void Deactivate();
SURVIVALGAME_API bool IsActive();
/** Meadow grass density 0-1 at a world XY (cm); 0 off the map or when inactive. */
SURVIVALGAME_API float GrassDensity(double X, double Y);
/** The ground under a world XY (cm); Soil off the map or when inactive. */
SURVIVALGAME_API ESurface SurfaceAt(double X, double Y);
/** Grass, moor and woodland floor: soft, muffled underfoot. */
SURVIVALGAME_API bool IsSoft(ESurface Surface);
}
