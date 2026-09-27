#include "HomesteadEstateMapImport.h"

#include "AssetImportTask.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "HAL/IConsoleManager.h"
#include "HomesteadEstateMap.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/Package.h"

namespace
{
constexpr const TCHAR* MapFolder = TEXT("/Game/SurvivalGame/UI/Map");

FString DefaultSource()
{
    return FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Map/T_EstateMap.png")));
}

FAutoConsoleCommand ImportCommand(
    TEXT("Homestead.ImportEstateMap"),
    TEXT("Imports Assets/Map/T_EstateMap.png (bake it first with Scripts/Map/bake_estate_map.py) as T_EstateMap and DA_EstateMap. Optional argument: another PNG."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        const FString Error = UHomesteadEstateMapImport::ImportEstateMap(Args.Num() ? Args[0] : FString());
        if (Error.IsEmpty()) { UE_LOG(LogTemp, Display, TEXT("ESTATE_MAP_IMPORTED")); }
        else { UE_LOG(LogTemp, Error, TEXT("ESTATE_MAP_IMPORT_FAILED: %s"), *Error); }
    }));
}

FString UHomesteadEstateMapImport::ImportEstateMap(const FString& SourcePng)
{
    const FString Png = SourcePng.IsEmpty() ? DefaultSource() : FPaths::ConvertRelativePathToFull(SourcePng);
    if (!FPaths::FileExists(Png)) return FString::Printf(TEXT("%s is missing. Run Scripts/Map/bake_estate_map.py first."), *Png);

    FVector2D WorldMin(-201600.0, -201600.0), WorldSize(403200.0, 403200.0);
    FString Source = TEXT("Estate heightmap"), BakedAt;
    FString Json;
    if (FFileHelper::LoadFileToString(Json, *FPaths::ChangeExtension(Png, TEXT("json"))))
    {
        TSharedPtr<FJsonObject> Info;
        if (!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json), Info) || !Info.IsValid())
            return TEXT("The map's transform JSON is unreadable.");
        WorldMin = FVector2D(Info->GetNumberField(TEXT("minX")), Info->GetNumberField(TEXT("minY")));
        WorldSize = FVector2D(Info->GetNumberField(TEXT("sizeX")), Info->GetNumberField(TEXT("sizeY")));
        Info->TryGetStringField(TEXT("source"), Source);
        Info->TryGetStringField(TEXT("bakedAt"), BakedAt);
        if (WorldSize.X <= 0 || WorldSize.Y <= 0) return TEXT("The map's transform JSON has no extent.");
    }

    UAssetImportTask* Task = NewObject<UAssetImportTask>();
    Task->Filename = Png;
    Task->DestinationPath = MapFolder;
    Task->DestinationName = TEXT("T_EstateMap");
    Task->bReplaceExisting = true;
    Task->bReplaceExistingSettings = false;
    Task->bAutomated = true;
    Task->bSave = false;
    FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
    AssetTools.Get().ImportAssetTasks({Task});
    UTexture2D* Texture = nullptr;
    for (UObject* Object : Task->GetObjects())
        if (UTexture2D* Imported = Cast<UTexture2D>(Object)) Texture = Imported;
    if (!Texture) return TEXT("The PNG did not import as a texture.");
    Texture->Modify();
    // Drawn by Slate, never on a mesh: keep every mip resident so zoomed-out views stay sharp.
    Texture->LODGroup = TEXTUREGROUP_World;
    Texture->NeverStream = true;
    Texture->MipGenSettings = TMGS_SimpleAverage;
    Texture->CompressionSettings = TC_Default;
    Texture->SRGB = true;
    Texture->AddressX = TA_Clamp;
    Texture->AddressY = TA_Clamp;
    Texture->PostEditChange();

    const FString DataPackageName = FString(MapFolder) + TEXT("/DA_EstateMap");
    UPackage* Package = CreatePackage(*DataPackageName);
    Package->FullyLoad();
    UHomesteadEstateMap* Map = FindObject<UHomesteadEstateMap>(Package, TEXT("DA_EstateMap"));
    if (!Map)
    {
        Map = NewObject<UHomesteadEstateMap>(Package, TEXT("DA_EstateMap"), RF_Public | RF_Standalone | RF_Transactional);
        FAssetRegistryModule::AssetCreated(Map);
    }
    Map->Modify();
    Map->Texture = Texture;
    Map->WorldMin = WorldMin;
    Map->WorldSize = WorldSize;
    Map->Source = Source;
    Map->BakedAt = BakedAt;
    Map->MarkPackageDirty();
    Texture->MarkPackageDirty();
    if (!UEditorLoadingAndSavingUtils::SavePackages({Texture->GetOutermost(), Package}, false))
        return TEXT("Saving T_EstateMap or DA_EstateMap failed.");
    UE_LOG(LogTemp, Display, TEXT("Estate map imported from %s (%s, baked %s): %dx%d over %.0f x %.0f cm."),
        *Png, *Source, *BakedAt, Texture->GetSizeX(), Texture->GetSizeY(), WorldSize.X, WorldSize.Y);
    return FString();
}

UHomesteadImportEstateMapCommandlet::UHomesteadImportEstateMapCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 UHomesteadImportEstateMapCommandlet::Main(const FString& Params)
{
    FString Source;
    FParse::Value(*Params, TEXT("Source="), Source);
    const FString Error = UHomesteadEstateMapImport::ImportEstateMap(Source);
    if (!Error.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("ESTATE_MAP_IMPORT_FAILED: %s"), *Error);
        return 1;
    }
    UE_LOG(LogTemp, Display, TEXT("ESTATE_MAP_IMPORTED"));
    return 0;
}
