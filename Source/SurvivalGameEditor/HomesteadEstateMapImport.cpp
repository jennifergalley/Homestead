#include "HomesteadEstateMapImport.h"

#include "AssetImportTask.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Editor.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "ImageUtils.h"
#include "RenderingThread.h"
#include "TextureResource.h"
#include "WorldPartition/LoaderAdapter/LoaderAdapterShape.h"
#include "WorldPartition/WorldPartition.h"
#include "WorldPartition/WorldPartitionEditorLoaderAdapter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Dom/JsonObject.h"
#include "Engine/Texture2D.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
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
FAutoConsoleCommand CaptureCommand(
    TEXT("Homestead.CaptureEstateMap"),
    TEXT("Captures the open Estate level straight down (base colour, north up) to Saved/EstateMap/EstateCapture.png for bake_estate_map.py --capture. Optional argument: resolution (default 8192)."),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        const int32 Resolution = Args.Num() ? FCString::Atoi(*Args[0]) : 8192;
        const FString Error = UHomesteadEstateMapImport::CaptureEstateMap(Resolution, FString());
        if (Error.IsEmpty()) { UE_LOG(LogTemp, Display, TEXT("ESTATE_MAP_CAPTURED")); }
        else { UE_LOG(LogTemp, Error, TEXT("ESTATE_MAP_CAPTURE_FAILED: %s"), *Error); }
    }));
}

FString UHomesteadEstateMapImport::CaptureEstateMap(int32 Resolution, const FString& OutputPng)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World) return TEXT("No editor world is open.");
    Resolution = FMath::Clamp(Resolution > 0 ? Resolution : 8192, 1024, 16384);
    const int32 Tiles = FMath::Max(1, Resolution / 2048);
    const int32 TileSize = Resolution / Tiles;
    Resolution = TileSize * Tiles;
    constexpr double Min = -201600.0, Extent = 403200.0;
    const double TileWorld = Extent / Tiles;
    const FString Output = OutputPng.IsEmpty()
        ? FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("EstateMap/EstateCapture.png"))) : OutputPng;

    // World Partition keeps most of the estate unloaded in the editor; load the whole square.
    if (UWorldPartition* Partition = World->GetWorldPartition())
    {
        UWorldPartitionEditorLoaderAdapter* Adapter = Partition->CreateEditorLoaderAdapter<FLoaderAdapterShape>(World,
            FBox(FVector(Min, Min, -100000.0), FVector(Min + Extent, Min + Extent, 100000.0)), TEXT("Estate map capture"));
        Adapter->GetLoaderAdapter()->Load();
    }
    FlushRenderingCommands();

    FActorSpawnParameters Spawn;
    Spawn.ObjectFlags = RF_Transient;
    ASceneCapture2D* Camera = World->SpawnActor<ASceneCapture2D>(Spawn);
    if (!Camera) return TEXT("Could not place the capture camera.");
    USceneCaptureComponent2D* Capture = Camera->GetCaptureComponent2D();
    Capture->ProjectionType = ECameraProjectionMode::Orthographic;
    Capture->OrthoWidth = TileWorld;
    Capture->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->ShowFlags.SetFog(false);
    Capture->ShowFlags.SetAtmosphere(false);
    Capture->ShowFlags.SetSkeletalMeshes(false);
    Capture->ShowFlags.SetParticles(false);
    Capture->ShowFlags.SetDecals(false);
    UTextureRenderTarget2D* Target = NewObject<UTextureRenderTarget2D>();
    Target->InitCustomFormat(TileSize, TileSize, PF_B8G8R8A8, false);
    Target->UpdateResourceImmediate(true);
    Capture->TextureTarget = Target;

    TArray64<FColor> Image;
    Image.SetNumZeroed(static_cast<int64>(Resolution) * Resolution);
    for (int32 Row = 0; Row < Tiles; ++Row)
        for (int32 Column = 0; Column < Tiles; ++Column)
        {
            // Rows run north to south, columns west to east; looking straight down with yaw 0 puts
            // north (+X) at the top of each tile and east (+Y) on the right.
            const FVector Centre(Min + Extent - (Row + 0.5) * TileWorld, Min + (Column + 0.5) * TileWorld, 150000.0);
            Camera->SetActorLocationAndRotation(Centre, FRotator(-90.0, 0.0, 0.0));
            Capture->CaptureScene();
            FlushRenderingCommands();
            TArray<FColor> Pixels;
            if (!Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels) || Pixels.Num() != TileSize * TileSize)
            {
                Camera->Destroy();
                return TEXT("Reading a capture tile failed.");
            }
            for (int32 Y = 0; Y < TileSize; ++Y)
                FMemory::Memcpy(&Image[(static_cast<int64>(Row) * TileSize + Y) * Resolution + static_cast<int64>(Column) * TileSize],
                    &Pixels[Y * TileSize], TileSize * sizeof(FColor));
        }
    Camera->Destroy();
    for (FColor& Pixel : Image) Pixel.A = 255;
    TArray64<uint8> Png;
    FImageUtils::PNGCompressImageArray(Resolution, Resolution, Image, Png);
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Output), true);
    if (!FFileHelper::SaveArrayToFile(Png, *Output)) return FString::Printf(TEXT("Could not write %s."), *Output);
    UE_LOG(LogTemp, Display, TEXT("Estate map capture written to %s (%dx%d, %d tiles)."), *Output, Resolution, Resolution, Tiles * Tiles);
    return FString();
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
