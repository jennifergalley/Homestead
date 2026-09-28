#include "HomesteadEstateAuthoringLibrary.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Editor.h"
#include "EngineUtils.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Landscape.h"
#include "LandscapeInfo.h"
#include "LandscapeLayerInfoObject.h"
#include "LandscapeProxy.h"
#include "LandscapeSubsystem.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

namespace
{
constexpr int32 EstateAuthoringVerts = 4033;
constexpr int32 EstateAuthoringQuadsPerSection = 63;
constexpr int32 EstateAuthoringSectionsPerComponent = 2;

bool EstateLoadGreyPng(const FString& Path, int32 BitDepth, int32& OutWidth, int32& OutHeight, TArray64<uint8>& OutRaw)
{
    TArray64<uint8> File;
    if (!FFileHelper::LoadFileToArray(File, *Path))
        return false;
    IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>("ImageWrapper");
    const TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
    if (!Wrapper.IsValid() || !Wrapper->SetCompressed(File.GetData(), File.Num()))
        return false;
    OutWidth = static_cast<int32>(Wrapper->GetWidth());
    OutHeight = static_cast<int32>(Wrapper->GetHeight());
    return Wrapper->GetRaw(ERGBFormat::Gray, BitDepth, OutRaw);
}

ULandscapeLayerInfoObject* EstateFindOrCreateLayerInfo(const FString& PackagePath, FName LayerName)
{
    const FString AssetName = FString::Printf(TEXT("LI_%s"), *LayerName.ToString());
    const FString PackageName = PackagePath / AssetName;
    if (ULandscapeLayerInfoObject* Existing = LoadObject<ULandscapeLayerInfoObject>(nullptr,
            *(PackageName + TEXT(".") + AssetName), nullptr, LOAD_NoWarn | LOAD_Quiet))
        return Existing;
    UPackage* Package = CreatePackage(*PackageName);
    ULandscapeLayerInfoObject* Info = NewObject<ULandscapeLayerInfoObject>(Package, *AssetName,
        RF_Public | RF_Standalone | RF_Transactional);
    Info->SetLayerName(LayerName, false);
    Info->SetBlendMethod(ELandscapeTargetLayerBlendMethod::FinalWeightBlending, false);
    FAssetRegistryModule::AssetCreated(Info);
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    UPackage::SavePackage(Package, Info,
        *FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension()), Args);
    return Info;
}
}

FString UHomesteadEstateAuthoringLibrary::CreateEstateLandscape(const FString& HeightmapPng,
    const FString& WeightmapFolder, const TArray<FName>& LayerNames, const FString& LayerInfoPackagePath,
    UMaterialInterface* Material, int32 WorldPartitionGridSize)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
        return TEXT("error: no editor world");
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
        return FString::Printf(TEXT("error: the level already has landscape %s"), *It->GetActorLabel());

    int32 Width = 0, Height = 0;
    TArray64<uint8> Raw;
    if (!EstateLoadGreyPng(HeightmapPng, 16, Width, Height, Raw))
        return FString::Printf(TEXT("error: can't read 16-bit PNG %s"), *HeightmapPng);
    if (Width != EstateAuthoringVerts || Height != EstateAuthoringVerts)
        return FString::Printf(TEXT("error: heightmap is %dx%d, expected %d"), Width, Height, EstateAuthoringVerts);
    TArray<uint16> Heights;
    Heights.SetNumUninitialized(EstateAuthoringVerts * EstateAuthoringVerts);
    FMemory::Memcpy(Heights.GetData(), Raw.GetData(), Heights.Num() * sizeof(uint16));

    TArray<FLandscapeImportLayerInfo> Layers;
    FString LayerReport;
    for (const FName& Name : LayerNames)
    {
        FLandscapeImportLayerInfo& Layer = Layers.Emplace_GetRef(Name);
        Layer.LayerInfo = EstateFindOrCreateLayerInfo(LayerInfoPackagePath, Name);
        const FString WeightPath = WeightmapFolder / (Name.ToString() + TEXT(".png"));
        int32 WW = 0, WH = 0;
        TArray64<uint8> Weights;
        if (FPaths::FileExists(WeightPath) && EstateLoadGreyPng(WeightPath, 8, WW, WH, Weights)
            && WW == EstateAuthoringVerts && WH == EstateAuthoringVerts)
        {
            Layer.LayerData.SetNumUninitialized(EstateAuthoringVerts * EstateAuthoringVerts);
            FMemory::Memcpy(Layer.LayerData.GetData(), Weights.GetData(), Layer.LayerData.Num());
            Layer.SourceFilePath = WeightPath;
            LayerReport += FString::Printf(TEXT(" %s(w)"), *Name.ToString());
        }
        else
        {
            LayerReport += FString::Printf(TEXT(" %s"), *Name.ToString());
        }
    }

    const double Half = (EstateAuthoringVerts - 1) * 100.0 / 2.0;
    ALandscape* Landscape = World->SpawnActor<ALandscape>(FVector(-Half, -Half, 0.0), FRotator::ZeroRotator);
    Landscape->LandscapeMaterial = Material;
    Landscape->SetActorRelativeScale3D(FVector(100.0, 100.0, 100.0));
    Landscape->StaticLightingLOD = 2;

    TMap<FGuid, TArray<uint16>> HeightData;
    HeightData.Add(FGuid(), MoveTemp(Heights));
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> LayerData;
    LayerData.Add(FGuid(), Layers);
    Landscape->Import(FGuid::NewGuid(), 0, 0, EstateAuthoringVerts - 1, EstateAuthoringVerts - 1, EstateAuthoringSectionsPerComponent, EstateAuthoringQuadsPerSection,
        HeightData, *HeightmapPng, LayerData, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
    Landscape->SetActorLabel(TEXT("EstateLandscape"));

    ULandscapeInfo* Info = Landscape->GetLandscapeInfo();
    if (!Info)
        return TEXT("error: import produced no landscape info");
    Info->UpdateLayerInfoMap(Landscape);
    for (const FLandscapeImportLayerInfo& Layer : Layers)
    {
        Landscape->AddTargetLayer(Layer.LayerName, FLandscapeTargetLayerSettings(Layer.LayerInfo, Layer.SourceFilePath));
        const int32 Index = Info->GetLayerInfoIndex(Layer.LayerName);
        if (Index != INDEX_NONE)
            Info->Layers[Index].LayerInfoObj = Layer.LayerInfo;
    }

    FString Grid = TEXT("not world partition");
    if (ULandscapeSubsystem* Subsystem = World->GetSubsystem<ULandscapeSubsystem>(); Subsystem && Subsystem->IsGridBased())
    {
        Subsystem->ChangeGridSize(Info, static_cast<uint32>(FMath::Max(1, WorldPartitionGridSize)));
        Grid = FString::Printf(TEXT("grid %d components"), WorldPartitionGridSize);
    }
    int32 Proxies = 0;
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
        ++Proxies;
    return FString::Printf(TEXT("ok: %s, %d proxies, layers:%s"), *Grid, Proxies, *LayerReport);
}

double UHomesteadEstateAuthoringLibrary::EditorGroundHeight(double X, double Y)
{
    UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!World)
        return -1e9;
    for (TActorIterator<ALandscapeProxy> It(World); It; ++It)
    {
        if (const TOptional<float> Z = It->GetHeightAtLocation(FVector(X, Y, 0.0)); Z.IsSet())
            return Z.GetValue();
    }
    return -1e9;
}
