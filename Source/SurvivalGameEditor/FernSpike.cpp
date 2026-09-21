#include "FernSpike.h"
#include "AssetCompilingManager.h"
#include "AssetImportTask.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicRHI.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/TextureFactory.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "ImageUtils.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "MaterialShared.h"
#include "MeshDescription.h"
#include "Misc/CommandLine.h"
#include "Misc/App.h"
#include "Misc/FeedbackContext.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "PreviewScene.h"
#include "RenderingThread.h"
#include "SceneInterface.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "StaticMeshCompiler.h"
#include "StaticMeshResources.h"
#include "TextureResource.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectIterator.h"

DEFINE_LOG_CATEGORY_STATIC(LogFernSpike, Log, All);

namespace
{
const FString Trial = TEXT("/Game/Trials/Fern02_20260920_01");
const FString FernSourceRoot = FPaths::Combine(FPaths::ProjectDir(),
    TEXT("Assets/Source/woodland-preparation-20260920-182217-d1f84e39/fern_02"));
const TCHAR* MeshNames[] = { TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d") };
const int32 ExpectedTriangles[] = { 784, 2384, 2248, 816 };
const int64 ModelIds[] = { 902801568, 314288041, 297642122, 546770539 };
const int64 GeometryIds[] = { 812885519, 802914469, 387442062, 7844172 };
const FVector SourceSizesCm[] = {
    FVector(55.194062, 61.342108, 28.680355), FVector(98.974741, 89.368972, 42.769471),
    FVector(87.432581, 76.522568, 34.902037), FVector(57.278219, 59.327829, 21.308755)
};
struct FMap
{
    const TCHAR* File;
    const TCHAR* Name;
    EMaterialProperty Property;
    TextureCompressionSettings Compression;
    EMaterialSamplerType Sampler;
    bool Srgb;
};
const FMap Maps[] = {
    { TEXT("fern_02_diff_1k.png"), TEXT("T_Fern02_Diff"), MP_BaseColor, TC_Default, SAMPLERTYPE_Color, true },
    { TEXT("fern_02_nor_dx_1k.png"), TEXT("T_Fern02_NormalDX"), MP_Normal, TC_Normalmap, SAMPLERTYPE_Normal, false },
    { TEXT("fern_02_rough_1k.png"), TEXT("T_Fern02_Roughness"), MP_Roughness, TC_Masks, SAMPLERTYPE_Masks, false },
    { TEXT("fern_02_ao_1k.png"), TEXT("T_Fern02_AO"), MP_AmbientOcclusion, TC_Masks, SAMPLERTYPE_Masks, false },
    { TEXT("fern_02_alpha_1k.png"), TEXT("T_Fern02_Alpha"), MP_OpacityMask, TC_Masks, SAMPLERTYPE_Masks, false }
};

bool WriteJson(const FString& Path, const TSharedRef<FJsonObject>& Value)
{
    FString Text;
    return !IFileManager::Get().FileExists(*Path)
        && FJsonSerializer::Serialize(Value, TJsonWriterFactory<>::Create(&Text))
        && FFileHelper::SaveStringToFile(Text, *(Path + TEXT(".tmp")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && IFileManager::Get().Move(*Path, *(Path + TEXT(".tmp")), false, false);
}

bool WritePng(const FString& Path, const TArray<FColor>& Pixels)
{
    const FString Temp = Path + TEXT(".tmp");
    if (IFileManager::Get().FileExists(*Path) || IFileManager::Get().FileExists(*Temp)) return false;
    TArray64<uint8> Png;
    FImageUtils::PNGCompressImageArray(1280, 720, Pixels, Png);
    if (Png.IsEmpty()) return false;
    TUniquePtr<FArchive> Writer(IFileManager::Get().CreateFileWriter(*Temp, FILEWRITE_NoReplaceExisting));
    if (!Writer) return false;
    Writer->Serialize(Png.GetData(), Png.Num());
    if (!Writer->Close() || Writer->IsError()) return false;
    Writer.Reset();
    return IFileManager::Get().Move(*Path, *Temp, false, false);
}

class FStopFeedback : public FFeedbackContext
{
    FFeedbackContext* Previous;
    FString StopPath;
    FDateTime Deadline;
    bool bCompletionDriven;
public:
    FStopFeedback(const FString& Output, const FDateTime& InDeadline, bool bInCompletionDriven)
        : Previous(GWarn), StopPath(FPaths::Combine(Output, TEXT("stop-probe.txt"))), Deadline(InDeadline),
          bCompletionDriven(bInCompletionDriven) { GWarn = this; }
    ~FStopFeedback() override { GWarn = Previous; }
    bool ReceivedUserCancel() override
    {
        return (!bCompletionDriven && FDateTime::UtcNow() >= Deadline) || IFileManager::Get().FileExists(*StopPath);
    }
    void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
    {
        Previous->Serialize(Text, Verbosity, Category);
    }
    bool YesNof(const FText& Question) override
    {
        UE_LOG(LogFernSpike, Error, TEXT("Unexpected interactive question rejected: %s"), *Question.ToString());
        return false;
    }
};

TArray<TSharedPtr<FJsonValue>> Vector(const FVector& V)
{
    return { MakeShared<FJsonValueNumber>(V.X), MakeShared<FJsonValueNumber>(V.Y), MakeShared<FJsonValueNumber>(V.Z) };
}

bool Inventory(const TArray<UStaticMesh*>& Meshes, const TArray<UTexture2D*>& Textures,
    UMaterial* Material, const TSharedRef<FJsonObject>& Result)
{
    if (Meshes.Num() != 4 || Textures.Num() != 5 || !Material || Material->BlendMode != BLEND_Masked
        || !Material->TwoSided || Material->GetExpressions().Num() != 5
        || !FMath::IsNearlyEqual(Material->OpacityMaskClipValue, 0.333f)) return false;
    TArray<TSharedPtr<FJsonValue>> MeshRecords, TextureRecords;
    for (int32 Index = 0; Index < Meshes.Num(); ++Index)
    {
        UStaticMesh* Mesh = Meshes[Index];
        if (Mesh)
            UE_LOG(LogFernSpike, Display, TEXT("Measured %s: sourceLODs=%d triangles=%d slots=%d bounds=%s"),
                *Mesh->GetPathName(), Mesh->GetNumSourceModels(),
                Mesh->GetMeshDescription(0) ? Mesh->GetMeshDescription(0)->Triangles().Num() : -1,
                Mesh->GetStaticMaterials().Num(), *Mesh->GetBoundingBox().ToString());
        if (!Mesh || Mesh->GetNumSourceModels() != 1 || !Mesh->GetMeshDescription(0)
            || Mesh->GetMeshDescription(0)->Triangles().Num() != ExpectedTriangles[Index]
            || Mesh->GetStaticMaterials().Num() != 1
            || Mesh->GetStaticMaterials()[0].ImportedMaterialSlotName != TEXT("fern_02")
            || Mesh->GetMaterial(0) != Material || Mesh->GetNaniteSettings().bEnabled) return false;
        const FBox Bounds = Mesh->GetBoundingBox();
        const FVector Size = Bounds.GetSize();
        if (!Bounds.IsValid || Size.ContainsNaN() || Size.GetMin() < 1 || Size.GetMax() > 150) return false;
        TArray<double> ActualSize = { Size.X, Size.Y, Size.Z };
        TArray<double> ExpectedSize = { SourceSizesCm[Index].X, SourceSizesCm[Index].Y, SourceSizesCm[Index].Z };
        ActualSize.Sort();
        ExpectedSize.Sort();
        for (int32 Axis = 0; Axis < 3; ++Axis)
            if (!FMath::IsNearlyEqual(ActualSize[Axis], ExpectedSize[Axis], 0.1)) return false;
        auto Record = MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("object"), Mesh->GetPathName());
        Record->SetNumberField(TEXT("triangles"), Mesh->GetMeshDescription(0)->Triangles().Num());
        Record->SetNumberField(TEXT("sourceModelId"), ModelIds[Index]);
        Record->SetNumberField(TEXT("sourceGeometryId"), GeometryIds[Index]);
        Record->SetStringField(TEXT("sourceNode"), FString(TEXT("fern_02_")) + MeshNames[Index]);
        Record->SetStringField(TEXT("slot"), Mesh->GetStaticMaterials()[0].MaterialSlotName.ToString());
        Record->SetStringField(TEXT("importedSlot"), Mesh->GetStaticMaterials()[0].ImportedMaterialSlotName.ToString());
        Record->SetArrayField(TEXT("boundsMinCm"), Vector(Bounds.Min));
        Record->SetArrayField(TEXT("boundsMaxCm"), Vector(Bounds.Max));
        Record->SetArrayField(TEXT("sizeCm"), Vector(Size));
        Record->SetArrayField(TEXT("sourceDimensionsTimes100Cm"), Vector(SourceSizesCm[Index]));
        Record->SetNumberField(TEXT("axisIndependentSizeToleranceCm"), 0.1);
        Record->SetArrayField(TEXT("assetOrigin"), Vector(FVector::ZeroVector));
        Record->SetBoolField(TEXT("transformVertexToAbsolute"), true);
        Record->SetBoolField(TEXT("localPivotPreserved"), false);
        Record->SetNumberField(TEXT("importUniformScale"), 1);
        Record->SetBoolField(TEXT("nanite"), Mesh->GetNaniteSettings().bEnabled);
        MeshRecords.Add(MakeShared<FJsonValueObject>(Record));
    }
    for (int32 Index = 0; Index < Textures.Num(); ++Index)
    {
        UTexture2D* Texture = Textures[Index];
        if (!Texture || Texture->Source.GetSizeX() != 1024 || Texture->Source.GetSizeY() != 1024
            || Texture->SRGB != Maps[Index].Srgb || Texture->CompressionSettings != Maps[Index].Compression
            || Texture->bFlipGreenChannel) return false;
        const FExpressionInput* Input = Material->GetExpressionInputForProperty(Maps[Index].Property);
        const auto* Sample = Input ? Cast<UMaterialExpressionTextureSample>(Input->Expression) : nullptr;
        if (!Sample || Sample->Texture != Texture || Sample->SamplerType != Maps[Index].Sampler
            || Input->OutputIndex != (Index < 2 ? 0 : 1)) return false;
        auto Record = MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("object"), Texture->GetPathName());
        Record->SetStringField(TEXT("source"), Maps[Index].File);
        Record->SetNumberField(TEXT("width"), Texture->Source.GetSizeX());
        Record->SetNumberField(TEXT("height"), Texture->Source.GetSizeY());
        Record->SetNumberField(TEXT("sourceFormat"), Texture->Source.GetFormat());
        Record->SetNumberField(TEXT("compression"), Texture->CompressionSettings);
        Record->SetNumberField(TEXT("materialProperty"), Maps[Index].Property);
        Record->SetBoolField(TEXT("srgb"), Texture->SRGB);
        Record->SetBoolField(TEXT("flipGreen"), Texture->bFlipGreenChannel);
        Record->SetNumberField(TEXT("connectedOutputIndex"), Input->OutputIndex);
        TextureRecords.Add(MakeShared<FJsonValueObject>(Record));
    }
    Result->SetArrayField(TEXT("meshes"), MeshRecords);
    Result->SetArrayField(TEXT("textures"), TextureRecords);
    Result->SetStringField(TEXT("material"), Material->GetPathName());
    Result->SetStringField(TEXT("shadingModel"), TEXT("DefaultLit"));
    Result->SetBoolField(TEXT("twoSided"), Material->TwoSided);
    Result->SetNumberField(TEXT("opacityClip"), Material->OpacityMaskClipValue);
    Result->SetStringField(TEXT("transformPolicy"), TEXT("FBX scene/unit conversion once; absolute transform bake; no extra scale, rotation or vertex recentering."));
    return true;
}

bool Import(const FString& Output, FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
{
    Result->SetStringField(TEXT("stage"), TEXT("explicit-texture-import"));
    TArray<UObject*> Assets;
    TArray<UTexture2D*> Textures;
    for (const FMap& Map : Maps)
    {
        UE_LOG(LogFernSpike, Display, TEXT("Importing exact texture %s"), Map.File);
        if (Feedback.ReceivedUserCancel()) return false;
        auto* Factory = NewObject<UTextureFactory>();
        auto* Task = NewObject<UAssetImportTask>();
        Task->bAutomated = true;
        Task->bReplaceExisting = false;
        Factory->SetAssetImportTask(Task);
        Factory->CompressionSettings = Map.Compression;
        const FString PackageName = Trial + TEXT("/Textures/") + Map.Name;
        if (FindPackage(nullptr, *PackageName) || FPackageName::DoesPackageExist(PackageName)) return false;
        bool Cancelled = false;
        UTexture2D* Texture = Cast<UTexture2D>(Factory->ImportObject(UTexture2D::StaticClass(),
            CreatePackage(*PackageName), Map.Name, RF_Public | RF_Standalone, FPaths::Combine(FernSourceRoot, Map.File), nullptr, Cancelled));
        if (Cancelled || !Texture || Factory->GetAdditionalImportedObjects().Num()) return false;
        Texture->SRGB = Map.Srgb;
        Texture->CompressionSettings = Map.Compression;
        Texture->bFlipGreenChannel = false;
        Texture->PostEditChange();
        Textures.Add(Texture);
        Assets.Add(Texture);
    }
    Result->SetStringField(TEXT("stage"), TEXT("material-graph"));
    auto* Material = NewObject<UMaterial>(CreatePackage(*(Trial + TEXT("/Materials/M_Fern02"))),
        TEXT("M_Fern02"), RF_Public | RF_Standalone);
    Material->BlendMode = BLEND_Masked;
    Material->TwoSided = true;
    Material->OpacityMaskClipValue = 0.333f;
    Material->SetShadingModel(MSM_DefaultLit);
    for (int32 Index = 0; Index < Textures.Num(); ++Index)
    {
        auto* Sample = Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionTextureSample::StaticClass(), -400, Index * 200));
        if (!Sample) return false;
        Sample->Texture = Textures[Index];
        Sample->SamplerType = Maps[Index].Sampler;
        if (!UMaterialEditingLibrary::ConnectMaterialProperty(Sample, Index < 2 ? TEXT("") : TEXT("R"), Maps[Index].Property)) return false;
    }
    Material->PostEditChange();
    Assets.Add(Material);
    auto* Factory = NewObject<UFbxFactory>();
    Factory->SetDetectImportTypeOnImport(false);
    auto* Task = NewObject<UAssetImportTask>();
    Task->bAutomated = true;
    Task->bReplaceExisting = false;
    Factory->SetAssetImportTask(Task);
    UFbxImportUI* UI = Factory->ImportUI;
    UI->MeshTypeToImport = FBXIT_StaticMesh;
    UI->OriginalImportType = FBXIT_StaticMesh;
    UI->bAutomatedImportShouldDetectType = false;
    UI->bImportMesh = true;
    UI->bImportAsSkeletal = false;
    UI->bImportAnimations = false;
    UI->bImportMaterials = false;
    UI->bImportTextures = false;
    UI->bOverrideFullName = false;
    UFbxStaticMeshImportData* Data = UI->StaticMeshImportData;
    Data->bCombineMeshes = false;
    Data->bImportMeshLODs = false;
    Data->bAutoGenerateCollision = false;
    Data->bBuildNanite = false;
    Data->bGenerateLightmapUVs = false;
    Data->bRemoveDegenerates = false;
    Data->bTransformVertexToAbsolute = true;
    Data->bBakePivotInVertex = false;
    Data->bConvertScene = true;
    Data->bConvertSceneUnit = true;
    Data->bForceFrontXAxis = false;
    Data->ImportUniformScale = 1;
    Data->ImportTranslation = FVector::ZeroVector;
    Data->ImportRotation = FRotator::ZeroRotator;
    Data->NormalImportMethod = FBXNIM_ImportNormals;
    bool Cancelled = false;
    Result->SetStringField(TEXT("stage"), TEXT("explicit-fbx-import"));
    if (Feedback.ReceivedUserCancel()) return false;
    UObject* Imported = Factory->ImportObject(UStaticMesh::StaticClass(),
        CreatePackage(*(Trial + TEXT("/Unsaved/Fern02"))), TEXT("Fern02"), RF_Public | RF_Standalone,
        FPaths::Combine(FernSourceRoot, TEXT("fern_02_1k.fbx")), nullptr, Cancelled);
    if (!Imported || Cancelled) return false;
    TArray<UObject*> ImportedObjects = Factory->GetAdditionalImportedObjects();
    ImportedObjects.AddUnique(Imported);
    for (UObject* Object : ImportedObjects)
        UE_LOG(LogFernSpike, Display, TEXT("FBX returned object: %s (%s)"), *Object->GetPathName(), *Object->GetClass()->GetName());
    if (ImportedObjects.Num() != 4) return false;
    TArray<UStaticMesh*> Meshes;
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString Node = FString(TEXT("fern_02_")) + MeshNames[Index];
        UStaticMesh* Match = nullptr;
        for (UObject* Object : ImportedObjects)
        {
            if (Object->GetName().EndsWith(Node))
            {
                if (Match) return false;
                Match = Cast<UStaticMesh>(Object);
            }
        }
        if (!Match) return false;
        const FString Name = FString(TEXT("SM_Fern02_")) + MeshNames[Index];
        if (!Match->Rename(*Name, CreatePackage(*(Trial + TEXT("/Meshes/") + Name)),
            REN_DontCreateRedirectors | REN_NonTransactional)) return false;
        Match->SetMaterial(0, Material);
        Match->PostEditChange();
        Meshes.Add(Match);
        Assets.Add(Match);
    }
    if (Feedback.ReceivedUserCancel()) return false;
    FStaticMeshCompilingManager::Get().FinishCompilation(Meshes);
    Result->SetStringField(TEXT("stage"), TEXT("measured-inventory"));
    if (!Inventory(Meshes, Textures, Material, Result)) return false;
    for (TObjectIterator<UObject> It; It; ++It)
    {
        if (It->IsAsset() && It->GetOutermost()->GetName().StartsWith(Trial + TEXT("/")) && !Assets.Contains(*It)) return false;
    }
    TSet<FString> AllowedPackages;
    Result->SetStringField(TEXT("stage"), TEXT("persistent-reference-audit"));
    for (UObject* Asset : Assets) AllowedPackages.Add(Asset->GetOutermost()->GetName());
    TArray<TSharedPtr<FJsonValue>> References;
    TArray<UObject*> Pending = Assets;
    TSet<UObject*> Seen;
    while (Pending.Num())
    {
        UObject* Object = Pending.Pop(EAllowShrinking::No);
        if (Seen.Contains(Object)) continue;
        Seen.Add(Object);
        if (Seen.Num() > 4096) return false;
        TArray<UObject*> Found;
        FReferenceFinder Finder(Found, nullptr, false, true, false, true);
        Finder.FindReferences(Object);
        for (UObject* Reference : Found)
        {
            if (!Reference) continue;
            const FString PackageName = Reference->GetOutermost()->GetName();
            if (PackageName.StartsWith(TEXT("/Script/"))) continue;
            if (!AllowedPackages.Contains(PackageName))
            {
                UE_LOG(LogFernSpike, Error, TEXT("Unapproved persistent reference: %s -> %s"), *Object->GetPathName(), *Reference->GetPathName());
                return false;
            }
            References.Add(MakeShared<FJsonValueString>(Reference->GetPathName()));
            Pending.Add(Reference);
        }
    }
    Result->SetArrayField(TEXT("persistentObjectReferences"), References);
    // Save only validated final objects, never temporary factory packages or unrelated dirty assets.
    Result->SetStringField(TEXT("stage"), TEXT("ten-explicit-package-saves"));
    for (UObject* Asset : Assets)
    {
        if (Feedback.ReceivedUserCancel()) return false;
        UPackage* Package = Asset->GetOutermost();
        const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
        if (IFileManager::Get().FileExists(*File)) return false;
        FSavePackageArgs Args;
        Args.TopLevelFlags = RF_Public | RF_Standalone;
        Args.Error = GWarn;
        if (!UPackage::SavePackage(Package, Asset, *File, Args) || Feedback.ReceivedUserCancel()) return false;
    }
    Result->SetStringField(TEXT("stage"), TEXT("import-complete"));
    return true;
}

bool Render(const FString& Output, FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
{
    const double Started = FPlatformTime::Seconds();
    const auto Phase = [Started](const TCHAR* Name)
    {
        UE_LOG(LogFernSpike, Display, TEXT("Render phase at %.3fs: %s"), FPlatformTime::Seconds() - Started, Name);
    };
    Phase(TEXT("asset reload/inventory begin"));
    Result->SetStringField(TEXT("stage"), TEXT("trial-reload-and-inventory"));
    if (!FParse::Param(FCommandLine::Get(), TEXT("RenderOffScreen"))
        || !FParse::Param(FCommandLine::Get(), TEXT("AllowCommandletRendering"))
        || FParse::Param(FCommandLine::Get(), TEXT("nullrhi")) || !GDynamicRHI
        || !GShaderCompilingManager || GShaderCompilingManager->IsShaderCompilationSkipped()) return false;
    TArray<UStaticMesh*> Meshes;
    TArray<UTexture2D*> Textures;
    for (const TCHAR* Suffix : MeshNames)
    {
        const FString Name = FString(TEXT("SM_Fern02_")) + Suffix;
        Meshes.Add(LoadObject<UStaticMesh>(nullptr, *(Trial + TEXT("/Meshes/") + Name + TEXT(".") + Name)));
    }
    for (const FMap& Map : Maps)
        Textures.Add(LoadObject<UTexture2D>(nullptr, *(Trial + TEXT("/Textures/") + Map.Name + TEXT(".") + Map.Name)));
    auto* Material = LoadObject<UMaterial>(nullptr, *(Trial + TEXT("/Materials/M_Fern02.M_Fern02")));
    if (!Inventory(Meshes, Textures, Material, Result) || Feedback.ReceivedUserCancel()) return false;
    Phase(TEXT("asset inventory complete; transient preview scene begin"));
    FPreviewScene Scene(FPreviewScene::ConstructionValues().SetCreatePhysicsScene(false)
        .AllowAudioPlayback(false).SetTransactional(false).SetLightBrightness(3.0f).SetSkyBrightness(0));
    if (!Scene.IsInitialized()) return false;
    UWorld* World = Scene.GetWorld();
    auto SceneState = MakeShared<FJsonObject>();
    SceneState->SetBoolField(TEXT("isClient"), GIsClient);
    SceneState->SetBoolField(TEXT("canEverRender"), FApp::CanEverRender());
    SceneState->SetBoolField(TEXT("worldScenePresent"), World && World->Scene);
    const bool RealScene = World && World->Scene && World->Scene->GetRenderScene();
    SceneState->SetBoolField(TEXT("realRenderScene"), RealScene);
    Result->SetObjectField(TEXT("sceneState"), SceneState);
    if (!RealScene)
    {
        Result->SetStringField(TEXT("failure"), TEXT("Preview world has no real renderer scene; dummy commandlet scene is not accepted."));
        UE_LOG(LogFernSpike, Error, TEXT("Preview world has no real renderer scene (GIsClient=%d)."), GIsClient);
        return false;
    }
    Phase(TEXT("transient preview scene initialized; components/render target begin"));
    Scene.SkyLight->SetVisibility(false);
    Scene.SetLightDirection(FRotator(-45, -45, 0));
    auto* Fern = NewObject<UStaticMeshComponent>();
    Fern->SetStaticMesh(Meshes[0]);
    Fern->SetMaterial(0, Material);
    Scene.AddComponent(Fern, FTransform::Identity);
    const FBox Bounds = Meshes[0]->GetBoundingBox();
    const FVector Center = Bounds.GetCenter();
    const double Radius = Bounds.GetExtent().Size();
    auto* Floor = NewObject<UStaticMeshComponent>();
    auto* FloorMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
    auto* FloorMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!FloorMesh || !FloorMaterial) return false;
    Floor->SetStaticMesh(FloorMesh);
    Floor->SetMaterial(0, FloorMaterial);
    Scene.AddComponent(Floor, FTransform(FQuat::Identity, FVector(Center.X, Center.Y, Bounds.Min.Z - 0.5),
        FVector(Radius * 0.06, Radius * 0.06, 1)));
    auto* Fill = NewObject<UDirectionalLightComponent>();
    Fill->SetIntensity(0.5f);
    Scene.AddComponent(Fill, FTransform(FRotator(-25, 140, 0)));
    auto* Target = NewObject<UTextureRenderTarget2D>();
    Target->ClearColor = FLinearColor(0.035f, 0.045f, 0.055f);
    Target->RenderTargetFormat = RTF_RGBA8;
    Target->InitAutoFormat(1280, 720);
    Target->UpdateResourceImmediate(true);
    auto* Capture = NewObject<USceneCaptureComponent2D>();
    Capture->TextureTarget = Target;
    Capture->bCaptureEveryFrame = false;
    Capture->bCaptureOnMovement = false;
    Capture->CaptureSource = SCS_FinalColorLDR;
    Capture->FOVAngle = 45;
    Capture->PostProcessSettings.bOverride_AutoExposureMethod = true;
    Capture->PostProcessSettings.AutoExposureMethod = AEM_Manual;
    Capture->PostProcessSettings.bOverride_AutoExposureBias = true;
    Capture->PostProcessSettings.AutoExposureBias = 0;
    Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
    Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure = false;
    Scene.AddComponent(Capture, FTransform::Identity);
    Capture->ShowFlags.SetMotionBlur(false);
    Capture->ShowFlags.SetScreenPercentage(false);
    Phase(TEXT("scene/components complete; FinishAllCompilation begin"));
    FAssetCompilingManager::Get().FinishAllCompilation();
    Phase(TEXT("FinishAllCompilation returned; unchanged fern resource readiness begin"));
    Result->SetStringField(TEXT("stage"), TEXT("real-shader-readiness"));
    FMaterialResource* Resource = Material->GetMaterialResource(GMaxRHIShaderPlatform);
    const auto Readiness = [&]()
    {
        auto State = MakeShared<FJsonObject>();
        State->SetNumberField(TEXT("shaderPlatform"), static_cast<int32>(GMaxRHIShaderPlatform));
        State->SetNumberField(TEXT("featureLevel"), static_cast<int32>(GMaxRHIFeatureLevel));
        State->SetBoolField(TEXT("resourcePresent"), Resource != nullptr);
        State->SetBoolField(TEXT("shaderMapPresent"), Resource && Resource->GetGameThreadShaderMap());
        State->SetBoolField(TEXT("shaderMapComplete"), Resource && Resource->IsGameThreadShaderMapComplete());
        State->SetBoolField(TEXT("compilationFinished"), Resource && Resource->IsCompilationFinished());
        State->SetNumberField(TEXT("remainingGlobalJobs"), GShaderCompilingManager->GetNumRemainingJobs());
        TArray<TSharedPtr<FJsonValue>> Errors;
        if (Resource)
            for (const FString& Error : Resource->GetCompileErrors()) Errors.Add(MakeShared<FJsonValueString>(Error));
        State->SetArrayField(TEXT("compileErrors"), Errors);
        return State;
    };
    Result->SetObjectField(TEXT("fernReadinessBefore"), Readiness());
    if (!Resource)
    {
        Result->SetStringField(TEXT("failure"), TEXT("No fern material resource for the actual shader platform."));
        UE_LOG(LogFernSpike, Error, TEXT("No fern material resource for shader platform %d."), static_cast<int32>(GMaxRHIShaderPlatform));
        return false;
    }
    if (!Resource->IsGameThreadShaderMapComplete())
        Resource->SubmitCompileJobs_GameThread(EShaderCompileJobPriority::High);
    Phase(TEXT("fern resource FinishCompilation begin"));
    Resource->FinishCompilation();
    Phase(TEXT("fern resource FinishCompilation returned; final shader readiness begin"));
    while (Resource && (!Resource->IsCompilationFinished() || GShaderCompilingManager->GetNumRemainingJobs() > 0))
    {
        if (Feedback.ReceivedUserCancel()) { Material->CancelOutstandingCompilation(); return false; }
        GShaderCompilingManager->ProcessAsyncResults(0.01f, false);
        FPlatformProcess::SleepNoStats(0.01f);
    }
    Result->SetObjectField(TEXT("fernReadinessAfter"), Readiness());
    if (!Resource->IsGameThreadShaderMapComplete() || Resource->GetCompileErrors().Num() || Feedback.ReceivedUserCancel())
    {
        Result->SetStringField(TEXT("failure"), TEXT("Fern shader map is incomplete, has direct compile errors, or cancellation was requested."));
        UE_LOG(LogFernSpike, Error, TEXT("Fern readiness failed: mapComplete=%d errors=%d cancelled=%d."),
            Resource->IsGameThreadShaderMapComplete(), Resource->GetCompileErrors().Num(), Feedback.ReceivedUserCancel());
        for (const FString& Error : Resource->GetCompileErrors()) UE_LOG(LogFernSpike, Error, TEXT("Fern shader: %s"), *Error);
        return false;
    }
    Phase(TEXT("shader map complete; render-command flush begin"));
    FlushRenderingCommands();
    Phase(TEXT("readiness flush complete"));
    SceneState->SetBoolField(TEXT("fernRegistered"), Fern->IsRegistered());
    SceneState->SetBoolField(TEXT("fernRenderStateCreated"), Fern->IsRenderStateCreated());
    SceneState->SetBoolField(TEXT("fernSceneProxyPresent"), Fern->GetSceneProxy() != nullptr);
    SceneState->SetBoolField(TEXT("captureRegistered"), Capture->IsRegistered());
    SceneState->SetBoolField(TEXT("captureVisible"), Capture->IsVisible());
    SceneState->SetArrayField(TEXT("fernWorldBoundsOriginCm"), Vector(Fern->Bounds.Origin));
    SceneState->SetArrayField(TEXT("fernWorldBoundsExtentCm"), Vector(Fern->Bounds.BoxExtent));
    if (!Fern->IsRegistered() || !Fern->IsRenderStateCreated() || !Fern->GetSceneProxy()
        || !Capture->IsRegistered() || !Capture->IsVisible())
    {
        Result->SetStringField(TEXT("failure"), TEXT("Fern/capture registration or actual fern render state is missing."));
        UE_LOG(LogFernSpike, Error, TEXT("Fern/capture registration or actual fern render state is missing."));
        return false;
    }
    Result->SetStringField(TEXT("rhi"), GDynamicRHI->GetName());
    Result->SetStringField(TEXT("adapter"), GRHIAdapterName);
    Result->SetStringField(TEXT("driver"), GRHIAdapterInternalDriverVersion);
    Result->SetNumberField(TEXT("width"), 1280);
    Result->SetNumberField(TEXT("height"), 720);
    Result->SetNumberField(TEXT("targetFormat"), static_cast<int32>(Target->RenderTargetFormat));
    Result->SetNumberField(TEXT("pixelFormat"), static_cast<int32>(Target->GetFormat()));
    Result->SetBoolField(TEXT("shaderMapComplete"), Resource->IsGameThreadShaderMapComplete());
    Result->SetBoolField(TEXT("materialFallbackAllowed"), false);
    TArray<TSharedPtr<FJsonValue>> Views;
    Result->SetStringField(TEXT("stage"), TEXT("two-native-offscreen-views"));
    for (int32 Index = 0; Index < 2; ++Index)
    {
        if (Feedback.ReceivedUserCancel()) return false;
        const FVector Position = Center + FVector(0, (Index == 0 ? -1 : 1) * Radius * 3.5, Radius * 1.25);
        const FRotator Rotation = (Center - Position).Rotation();
        Capture->SetWorldLocationAndRotation(Position, Rotation);
        auto View = MakeShared<FJsonObject>();
        Views.Add(MakeShared<FJsonValueObject>(View));
        Result->SetArrayField(TEXT("views"), Views);
        View->SetArrayField(TEXT("cameraCm"), Vector(Capture->GetComponentLocation()));
        View->SetStringField(TEXT("rotation"), Capture->GetComponentRotation().ToString());
        View->SetNumberField(TEXT("fovDegrees"), Capture->FOVAngle);
        View->SetNumberField(TEXT("nearClipCm"), GNearClippingPlane);
        View->SetArrayField(TEXT("lookAtCm"), Vector(Center));
        View->SetBoolField(TEXT("screenPercentageShowFlag"), Capture->ShowFlags.ScreenPercentage);
        View->SetBoolField(TEXT("motionBlur"), Capture->ShowFlags.MotionBlur);
        View->SetStringField(TEXT("exposure"), TEXT("Manual, compensation 0, physical camera exposure disabled"));
        Phase(Index == 0 ? TEXT("front capture begin") : TEXT("back capture begin"));
        Capture->CaptureScene();
        View->SetBoolField(TEXT("captureCallReturned"), true);
        FlushRenderingCommands();
        View->SetBoolField(TEXT("renderCommandsFlushed"), true);
        Phase(Index == 0 ? TEXT("front capture flush complete; readback begin") : TEXT("back capture flush complete; readback begin"));
        const FString Name = Index == 0 ? TEXT("fern-a-front.png") : TEXT("fern-a-back.png");
        const FString Path = FPaths::Combine(Output, Name);
        TArray<FColor> Pixels;
        const bool ReadbackSucceeded = Target->GameThread_GetRenderTargetResource()->ReadPixels(Pixels);
        View->SetBoolField(TEXT("readbackSucceeded"), ReadbackSucceeded);
        View->SetNumberField(TEXT("pixelCount"), Pixels.Num());
        if (!ReadbackSucceeded || Pixels.Num() != 1280 * 720)
        {
            Result->SetStringField(TEXT("failure"), TEXT("Native render-target readback failed or returned an unexpected pixel count."));
            UE_LOG(LogFernSpike, Error, TEXT("Readback failed: success=%d pixels=%d."), ReadbackSucceeded, Pixels.Num());
            return false;
        }
        Phase(Index == 0 ? TEXT("front readback complete; pixel validation/PNG encoding begin") : TEXT("back readback complete; pixel validation/PNG encoding begin"));
        int32 GreenPixels = 0;
        uint8 Minimum[4] = {255, 255, 255, 255};
        uint8 Maximum[4] = {};
        for (const FColor& Pixel : Pixels)
        {
            if (int32(Pixel.G) > int32(Pixel.R) + 8 && int32(Pixel.G) > int32(Pixel.B) + 4) ++GreenPixels;
            const uint8 Channels[] = {Pixel.R, Pixel.G, Pixel.B, Pixel.A};
            for (int32 Channel = 0; Channel < 4; ++Channel)
            {
                Minimum[Channel] = FMath::Min(Minimum[Channel], Channels[Channel]);
                Maximum[Channel] = FMath::Max(Maximum[Channel], Channels[Channel]);
            }
        }
        TArray<TSharedPtr<FJsonValue>> Minima, Maxima;
        for (int32 Channel = 0; Channel < 4; ++Channel)
        {
            Minima.Add(MakeShared<FJsonValueNumber>(Minimum[Channel]));
            Maxima.Add(MakeShared<FJsonValueNumber>(Maximum[Channel]));
        }
        View->SetArrayField(TEXT("readbackRgbaMinimum"), Minima);
        View->SetArrayField(TEXT("readbackRgbaMaximum"), Maxima);
        View->SetNumberField(TEXT("greenPixelCount"), GreenPixels);
        if (GreenPixels < 50)
        {
            const FString Diagnostic = Index == 0 ? TEXT("diagnostic-front-failed.png") : TEXT("diagnostic-back-failed.png");
            const bool Written = WritePng(FPaths::Combine(Output, Diagnostic), Pixels);
            View->SetBoolField(TEXT("diagnosticWritten"), Written);
            if (Written) View->SetStringField(TEXT("diagnosticImage"), Diagnostic);
            View->SetStringField(TEXT("readback"), TEXT("Failed diagnostic preserves every readback RGBA byte; not an accepted fern capture"));
            Result->SetStringField(TEXT("failure"), Written ? TEXT("Fern content gate failed; unaltered diagnostic PNG retained.")
                : TEXT("Fern content gate failed and diagnostic PNG could not be written."));
            UE_LOG(LogFernSpike, Error, TEXT("Fern content gate failed: greenPixels=%d diagnosticWritten=%d."), GreenPixels, Written);
            return false;
        }
        for (FColor& Pixel : Pixels) Pixel.A = 255;
        Phase(Index == 0 ? TEXT("front PNG write begin") : TEXT("back PNG write begin"));
        if (!WritePng(Path, Pixels))
        {
            Result->SetStringField(TEXT("failure"), TEXT("Accepted-view PNG atomic write failed."));
            UE_LOG(LogFernSpike, Error, TEXT("PNG atomic write failed: %s"), *Path);
            return false;
        }
        Phase(Index == 0 ? TEXT("front PNG write complete") : TEXT("back PNG write complete"));
        View->SetStringField(TEXT("image"), Name);
        View->SetStringField(TEXT("readback"), TEXT("Native LDR RGB unchanged; PNG alpha explicitly opaque"));
    }
    Result->SetArrayField(TEXT("views"), Views);
    Result->SetStringField(TEXT("fernTransform"), TEXT("Identity; camera framing only"));
    Result->SetStringField(TEXT("scope"), TEXT("Transient offscreen Editor preview, not packaged, gameplay, 4K or performance proof"));
    Result->SetStringField(TEXT("stage"), TEXT("render-complete"));
    return !Feedback.ReceivedUserCancel();
}
}

bool RunFernSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven)
{
    FStopFeedback Feedback(Output, Deadline, bCompletionDriven);
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("mode"), Mode);
    Result->SetStringField(TEXT("namespace"), Trial);
    const bool Passed = Mode == TEXT("Import") ? Import(Output, Feedback, Result)
        : Mode == TEXT("Render") && Render(Output, Feedback, Result);
    Result->SetBoolField(TEXT("passed"), Passed);
    Result->SetBoolField(TEXT("cancelledAtPollingBoundary"), Feedback.ReceivedUserCancel());
    if (!Passed) UE_LOG(LogFernSpike, Error, TEXT("Fern %s failed; retain evidence and do not retry."), *Mode);
    return WriteJson(FPaths::Combine(Output, TEXT("fern-result.json")), Result) && Passed;
}
