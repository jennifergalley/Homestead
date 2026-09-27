#include "HomesteadEstateTerrain.h"

#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
TArray<uint16> Heights;
bool bActive = false;

float Sample(int32 X, int32 Y)
{
    X = FMath::Clamp(X, 0, HomesteadEstateTerrain::Vertices - 1);
    Y = FMath::Clamp(Y, 0, HomesteadEstateTerrain::Vertices - 1);
    // Landscape Z scale 100: one height unit is 100/128 cm and 32768 is zero.
    return (static_cast<int32>(Heights[Y * HomesteadEstateTerrain::Vertices + X]) - 32768) * (100.0f / 128.0f);
}
}

namespace HomesteadEstateTerrain
{
bool Activate()
{
    if (bActive)
        return true;
    const FString Path = FPaths::Combine(FPaths::ProjectContentDir(),
        TEXT("SurvivalGame/Estate/Runtime/EstateHeightfield.r16"));
    TArray<uint8> Raw;
    if (!FFileHelper::LoadFileToArray(Raw, *Path) || Raw.Num() != Vertices * Vertices * 2)
    {
        UE_LOG(LogTemp, Error, TEXT("Estate heightfield is missing or the wrong size: %s (%d bytes)."), *Path, Raw.Num());
        return false;
    }
    Heights.SetNumUninitialized(Vertices * Vertices);
    FMemory::Memcpy(Heights.GetData(), Raw.GetData(), Raw.Num());
    bActive = true;
    return true;
}

void Deactivate()
{
    bActive = false;
    Heights.Empty();
}

bool IsActive() { return bActive; }

bool Contains(double X, double Y)
{
    return FMath::Abs(X) <= HalfExtentCm && FMath::Abs(Y) <= HalfExtentCm;
}

float Height(double X, double Y)
{
    if (!bActive || !FMath::IsFinite(X) || !FMath::IsFinite(Y))
        return std::numeric_limits<float>::quiet_NaN();
    const double FX = FMath::Clamp((X + HalfExtentCm) / QuadCm, 0.0, Vertices - 1.0);
    const double FY = FMath::Clamp((Y + HalfExtentCm) / QuadCm, 0.0, Vertices - 1.0);
    const int32 X0 = FMath::Min(FMath::FloorToInt32(FX), Vertices - 2);
    const int32 Y0 = FMath::Min(FMath::FloorToInt32(FY), Vertices - 2);
    const float U = static_cast<float>(FX - X0), V = static_cast<float>(FY - Y0);
    const float H00 = Sample(X0, Y0), H10 = Sample(X0 + 1, Y0), H01 = Sample(X0, Y0 + 1), H11 = Sample(X0 + 1, Y0 + 1);
    // Each landscape quad is split along its (0,0)-(1,1) diagonal.
    return U >= V ? H00 + U * (H10 - H00) + V * (H11 - H10)
                  : H00 + V * (H01 - H00) + U * (H11 - H01);
}
}
