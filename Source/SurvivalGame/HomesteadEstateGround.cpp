#include "HomesteadEstateGround.h"

#include "HomesteadEstateTerrain.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadGround, Log, All);

namespace
{
TArray<uint8> EstateGroundCells; // Density, surface per cell; rows run +Y, columns +X.
int32 EstateGroundSize = 0;
bool bEstateGroundLoaded = false;

int32 EstateGroundCellIndex(double X, double Y)
{
    if (!bEstateGroundLoaded || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !HomesteadEstateTerrain::Contains(X, Y))
        return INDEX_NONE;
    const double Span = 2.0 * HomesteadEstateTerrain::HalfExtentCm;
    const int32 Column = FMath::Clamp(FMath::FloorToInt32((X + HomesteadEstateTerrain::HalfExtentCm) / Span * EstateGroundSize), 0, EstateGroundSize - 1);
    const int32 Row = FMath::Clamp(FMath::FloorToInt32((Y + HomesteadEstateTerrain::HalfExtentCm) / Span * EstateGroundSize), 0, EstateGroundSize - 1);
    return (Row * EstateGroundSize + Column) * 2;
}
}

namespace HomesteadEstateGround
{
bool Activate()
{
    if (bEstateGroundLoaded)
        return true;
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("SurvivalGame/Estate/Runtime/EstateGround.bin"));
    TArray<uint8> Raw;
    if (!FFileHelper::LoadFileToArray(Raw, *Path) || Raw.Num() < 6 || FMemory::Memcmp(Raw.GetData(), "HGD1", 4) != 0)
    {
        UE_LOG(LogHomesteadGround, Warning, TEXT("Estate ground cover is missing or unreadable: %s"), *Path);
        return false;
    }
    uint16 Size = 0;
    FMemory::Memcpy(&Size, Raw.GetData() + 4, 2);
    if (Size == 0 || Raw.Num() != 6 + static_cast<int32>(Size) * Size * 2)
    {
        UE_LOG(LogHomesteadGround, Warning, TEXT("Estate ground cover has %d bytes for %u cells a side."), Raw.Num(), Size);
        return false;
    }
    EstateGroundSize = Size;
    EstateGroundCells.SetNumUninitialized(Raw.Num() - 6);
    FMemory::Memcpy(EstateGroundCells.GetData(), Raw.GetData() + 6, EstateGroundCells.Num());
    bEstateGroundLoaded = true;
    return true;
}

void Deactivate()
{
    bEstateGroundLoaded = false;
    EstateGroundSize = 0;
    EstateGroundCells.Empty();
}

bool IsActive() { return bEstateGroundLoaded; }

float GrassDensity(double X, double Y)
{
    const int32 Index = EstateGroundCellIndex(X, Y);
    return Index == INDEX_NONE ? 0.0f : EstateGroundCells[Index] / 255.0f;
}

ESurface SurfaceAt(double X, double Y)
{
    const int32 Index = EstateGroundCellIndex(X, Y);
    if (Index == INDEX_NONE) return ESurface::Soil;
    const uint8 Value = EstateGroundCells[Index + 1];
    return Value <= static_cast<uint8>(ESurface::Water) ? static_cast<ESurface>(Value) : ESurface::Soil;
}

bool IsSoft(ESurface Surface)
{
    return Surface == ESurface::Grass || Surface == ESurface::Moor || Surface == ESurface::Woodland;
}
}
