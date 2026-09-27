#pragma once

#include "Commandlets/Commandlet.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HomesteadEstateMapImport.generated.h"

// Imports the baked estate map (Assets/Map/T_EstateMap.png from Scripts/Map/bake_estate_map.py) as
// /Game/SurvivalGame/UI/Map/T_EstateMap and writes DA_EstateMap with the world rectangle it covers.
// In the editor: the console command Homestead.ImportEstateMap. Headless: -run=HomesteadImportEstateMap.
UCLASS()
class UHomesteadEstateMapImport : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    // Returns an empty string on success, otherwise what went wrong.
    UFUNCTION(BlueprintCallable, Category = "Homestead|Map")
    static FString ImportEstateMap(const FString& SourcePng);
    // Captures the open level straight down (base colour, north up) over the whole estate square
    // in tiles, writing a Resolution-square PNG for bake_estate_map.py --capture.
    UFUNCTION(BlueprintCallable, Category = "Homestead|Map")
    static FString CaptureEstateMap(int32 Resolution, const FString& OutputPng);
};

UCLASS()
class UHomesteadImportEstateMapCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    UHomesteadImportEstateMapCommandlet();
    virtual int32 Main(const FString& Params) override;
};
