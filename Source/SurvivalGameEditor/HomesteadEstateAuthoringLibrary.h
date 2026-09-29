#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "HomesteadEstateAuthoringLibrary.generated.h"

class ALandscape;
class UMaterialInterface;

/**
 * Editor-only helpers for authoring the fixed Estate map from the committed terrain pipeline
 * (Scripts/Terrain). Exposed to Python and, through it, to the editor MCP server.
 */
UCLASS()
class UHomesteadEstateAuthoringLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Creates the estate Landscape in the current editor world from a 16-bit greyscale PNG
     * (column = +X, row = +Y, 32768 = 0), 1 m per quad, Z scale 100, centred on the origin.
     * Each name in LayerNames gets a layer info asset under LayerInfoPackagePath and, when
     * WeightmapFolder/<Layer>.png exists, its 8-bit weights. Replaces no existing landscape:
     * fails if one is already present. Returns a status string.
     */
    UFUNCTION(BlueprintCallable, Category = "Homestead Estate")
    static FString CreateEstateLandscape(const FString& HeightmapPng, const FString& WeightmapFolder,
        const TArray<FName>& LayerNames, const FString& LayerInfoPackagePath, UMaterialInterface* Material,
        int32 WorldPartitionGridSize = 2);

    /** Landscape height (cm) at a world XY in the editor world, or -1e9 when nothing is hit. */
    UFUNCTION(BlueprintCallable, Category = "Homestead Estate")
    static double EditorGroundHeight(double X, double Y);

    /**
     * Brings the Landscape's base edit layer in line with a 4033x4033 little-endian 16-bit
     * heightfield (row = +Y, column = +X, the EstateHeightfield.r16 layout), within the vertex
     * rectangle [MinX..MaxX] x [MinY..MaxY]. Works tile by tile (TileSize vertices square) and writes
     * only tiles that differ, so only the landscape proxies a terrain edit touched get dirtied.
     * Loads that part of the World Partition first. With bDryRun it only reports. Save afterwards,
     * once the editor has ticked the edit-layer merge. Returns a status string.
     */
    UFUNCTION(BlueprintCallable, Category = "Homestead Estate")
    static FString ApplyEstateHeightfield(const FString& HeightfieldR16, int32 MinX, int32 MinY, int32 MaxX,
        int32 MaxY, int32 TileSize = 63, bool bDryRun = true);
};
