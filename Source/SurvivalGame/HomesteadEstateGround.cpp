#include "HomesteadEstateGround.h"

#include "HomesteadEstateTerrain.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
TArray<uint8> GroundCells; // Density, surface per cell; rows run +Y, columns +X.
int32 GroundSize = 0;
bool bGroundLoaded = false;

int32 CellIndex(double X, double Y)
{
    if (!bGroundLoaded || !FMath::IsFinite(X) || !FMath::IsFinite(Y) || !HomesteadEstateTerrain::Contains(X, Y))
        return INDEX_NONE;
    const double Span = 2.0 * HomesteadEstateTerrain::HalfExtentCm;
    const int32 Column = FMath::Clamp(FMath::FloorToInt32((X + HomesteadEstateTerrain::HalfExtentCm) / Span * GroundSize), 0, GroundSize - 1);
    const int32 Row = FMath::Clamp(FMath::FloorToInt32((Y + HomesteadEstateTerrain::HalfExtentCm) / Span * GroundSize), 0, GroundSize - 1);
    return (Row * GroundSize + Column) * 2;
}
}

namespace HomesteadEstateGround
{
bool Activate()
{
    if (bGroundLoaded)
        return true;
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("SurvivalGame/Estate/Runtime/EstateGround.bin"));
    TArray<uint8> Raw;
    if (!FFileHelper::LoadFileToArray(Raw, *Path) || Raw.Num() < 6 || FMemory::Memcmp(Raw.GetData(), "HGD1", 4) != 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("Estate ground cover is missing or unreadable: %s"), *Path);
        return false;
    }
    uint16 Size = 0;
    FMemory::Memcpy(&Size, Raw.GetData() + 4, 2);
    if (Size == 0 || Raw.Num() != 6 + static_cast<int32>(Size) * Size * 2)
    {
        UE_LOG(LogTemp, Warning, TEXT("Estate ground cover has %d bytes for %u cells a side."), Raw.Num(), Size);
        return false;
    }
    GroundSize = Size;
    GroundCells.SetNumUninitialized(Raw.Num() - 6);
    FMemory::Memcpy(GroundCells.GetData(), Raw.GetData() + 6, GroundCells.Num());
    bGroundLoaded = true;
    return true;
}

void Deactivate()
{
    bGroundLoaded = false;
    GroundSize = 0;
    GroundCells.Empty();
}

bool IsActive() { return bGroundLoaded; }

float GrassDensity(double X, double Y)
{
    const int32 Index = CellIndex(X, Y);
    return Index == INDEX_NONE ? 0.0f : GroundCells[Index] / 255.0f;
}

ESurface SurfaceAt(double X, double Y)
{
    const int32 Index = CellIndex(X, Y);
    if (Index == INDEX_NONE) return ESurface::Soil;
    const uint8 Value = GroundCells[Index + 1];
    return Value <= static_cast<uint8>(ESurface::Water) ? static_cast<ESurface>(Value) : ESurface::Soil;
}

bool IsSoft(ESurface Surface)
{
    return Surface == ESurface::Grass || Surface == ESurface::Moor || Surface == ESurface::Woodland;
}
}
