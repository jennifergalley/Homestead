#include "FernSpike.h"
#include "AssetCompilingManager.h"
#include "AssetImportTask.h"
#include "Animation/Skeleton.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DynamicRHI.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "Factories/FbxFactory.h"
#include "Factories/FbxImportUI.h"
#include "Factories/FbxStaticMeshImportData.h"
#include "Factories/FbxSkeletalMeshImportData.h"
#include "Factories/TextureFactory.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "ImageUtils.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionVertexColor.h"
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
#include "PhysicsEngine/BodySetup.h"
#include "RenderingThread.h"
#include "Rendering/SkeletalMeshModel.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "SceneInterface.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "StaticMeshCompiler.h"
#include "SkinnedAssetCompiler.h"
#include "StaticMeshResources.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshOperations.h"
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
const FString HairTrial = TEXT("/Game/Trials/HeroineWave_20260921_01");
const FString IncumbentHeroine = TEXT("/Game/SurvivalGame/Characters/Heroine/");
struct FWaveMesh
{
    const TCHAR* Name;
    int32 Triangles;
    int32 Slots;
};
const FWaveMesh WaveMeshes[] = {
    { TEXT("SK_Heroine_LongWave"), 120440, 12 },
    { TEXT("SK_Heroine_LongWave_Apron"), 123976, 14 },
    { TEXT("SK_Heroine_Willow_LongWave"), 120588, 12 },
    { TEXT("SK_Heroine_Willow_LongWave_Apron"), 124124, 14 },
    { TEXT("SK_Heroine_Hazel_LongWave"), 120452, 12 },
    { TEXT("SK_Heroine_Hazel_LongWave_Apron"), 123988, 14 }
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

UTexture2D* ImportMappedTexture(const FMap& Map, const FString& SourceRoot,
    const FString& AssetRoot, FStopFeedback& Feedback)
{
    UE_LOG(LogFernSpike, Display, TEXT("Importing exact texture %s"), Map.File);
    if (Feedback.ReceivedUserCancel()) return nullptr;
    auto* Factory = NewObject<UTextureFactory>();
    auto* Task = NewObject<UAssetImportTask>();
    Task->bAutomated = true;
    Task->bReplaceExisting = false;
    Factory->SetAssetImportTask(Task);
    Factory->CompressionSettings = Map.Compression;
    const FString PackageName = AssetRoot + TEXT("/Textures/") + Map.Name;
    if (FindPackage(nullptr, *PackageName) || FPackageName::DoesPackageExist(PackageName)) return nullptr;
    bool Cancelled = false;
    auto* Texture = Cast<UTexture2D>(Factory->ImportObject(UTexture2D::StaticClass(),
        CreatePackage(*PackageName), Map.Name, RF_Public | RF_Standalone,
        FPaths::Combine(SourceRoot, Map.File), nullptr, Cancelled));
    if (Cancelled || !Texture || Factory->GetAdditionalImportedObjects().Num()) return nullptr;
    Texture->SRGB = Map.Srgb;
    Texture->CompressionSettings = Map.Compression;
    Texture->bFlipGreenChannel = false;
    Texture->PostEditChange();
    return Texture;
}

UFbxFactory* StaticMeshFactory(bool bCombine)
{
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
    UI->bOverrideFullName = bCombine;
    UFbxStaticMeshImportData* Data = UI->StaticMeshImportData;
    Data->bCombineMeshes = bCombine;
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
    return Factory;
}

bool AuditAssetReferences(const FString& Root, const TArray<UObject*>& Assets,
    const TSet<FString>& External, const TSharedRef<FJsonObject>& Result)
{
    TSet<FString> Local;
    for (auto* Asset : Assets) Local.Add(Asset->GetOutermost()->GetName());
    for (TObjectIterator<UObject> It; It; ++It)
        if (It->IsAsset() && It->GetOutermost()->GetName().StartsWith(Root + TEXT("/")) && !Assets.Contains(*It)) return false;
    TArray<UObject*> Pending = Assets;
    TSet<UObject*> Seen;
    TArray<TSharedPtr<FJsonValue>> References;
    while (Pending.Num())
    {
        UObject* Object = Pending.Pop(EAllowShrinking::No);
        if (Seen.Contains(Object)) continue;
        Seen.Add(Object);
        if (Seen.Num() > 16384) return false;
        TArray<UObject*> Found;
        FReferenceFinder Finder(Found, nullptr, false, true, false, true);
        Finder.FindReferences(Object);
        for (auto* Reference : Found)
        {
            if (!Reference) continue;
            const FString Package = Reference->GetOutermost()->GetName();
            if (Package.StartsWith(TEXT("/Script/"))) continue;
            if (!Local.Contains(Package) && !External.Contains(Package))
            {
                UE_LOG(LogFernSpike, Error, TEXT("Unapproved persistent reference %s -> %s"), *Object->GetPathName(), *Reference->GetPathName());
                return false;
            }
            References.Add(MakeShared<FJsonValueString>(Reference->GetPathName()));
            if (Local.Contains(Package)) Pending.Add(Reference);
        }
    }
    Result->SetArrayField(TEXT("persistentObjectReferences"), References);
    return true;
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

namespace Hair
{
    USkeletalMesh* ImportRiggedMesh(const FString& Source, const FString& PackageName,
        USkeleton* Skeleton, FStopFeedback& Feedback)
    {
        if (!Skeleton || Feedback.ReceivedUserCancel() || FindPackage(nullptr, *PackageName)
            || FPackageName::DoesPackageExist(PackageName)) return nullptr;
        auto* Factory = NewObject<UFbxFactory>();
        Factory->SetDetectImportTypeOnImport(false);
        auto* Task = NewObject<UAssetImportTask>();
        Task->bAutomated = true;
        Task->bReplaceExisting = false;
        Factory->SetAssetImportTask(Task);
        UFbxImportUI* UI = Factory->ImportUI;
        UI->MeshTypeToImport = FBXIT_SkeletalMesh;
        UI->OriginalImportType = FBXIT_SkeletalMesh;
        UI->bAutomatedImportShouldDetectType = false;
        UI->bImportMesh = true;
        UI->bImportAsSkeletal = true;
        UI->bImportAnimations = false;
        UI->bImportMaterials = false;
        UI->bImportTextures = false;
        UI->bCreatePhysicsAsset = false;
        UI->bOverrideFullName = true;
        UI->Skeleton = Skeleton;
        auto* Data = UI->SkeletalMeshImportData.Get();
        Data->bConvertScene = true;
        Data->bConvertSceneUnit = true;
        Data->bForceFrontXAxis = false;
        Data->ImportUniformScale = 1;
        Data->ImportTranslation = FVector::ZeroVector;
        Data->ImportRotation = FRotator::ZeroRotator;
        Data->bImportMeshLODs = false;
        Data->bImportMorphTargets = false;
        Data->bUpdateSkeletonReferencePose = false;
        Data->bUseT0AsRefPose = false;
        Data->NormalImportMethod = FBXNIM_ImportNormals;
        bool Cancelled = false;
        const FString Name = FPackageName::GetShortName(PackageName);
        auto* Mesh = Cast<USkeletalMesh>(Factory->ImportObject(USkeletalMesh::StaticClass(),
            CreatePackage(*PackageName), *Name, RF_Public | RF_Standalone, Source, nullptr, Cancelled));
        if (!Mesh || Cancelled || Factory->GetAdditionalImportedObjects().Num() || Feedback.ReceivedUserCancel())
        {
            UE_LOG(LogFernSpike, Error, TEXT("Explicit skeletal import rejected: %s -> %s"), *Source, *PackageName);
            return nullptr;
        }
        TArray<USkinnedAsset*> Pending = {Mesh};
        FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
        return Mesh;
    }

    bool WaveInventory(const TArray<USkeletalMesh*>& Meshes, UTexture2D* Texture, UMaterial* Material,
        const TSharedRef<FJsonObject>& Result)
    {
        if (Meshes.Num() != UE_ARRAY_COUNT(WaveMeshes) || !Texture || !Material
            || Texture->Source.GetSizeX() != 2048 || Texture->Source.GetSizeY() != 2048 || !Texture->SRGB
            || Texture->CompressionSettings != TC_Default || Material->BlendMode != BLEND_Masked
            || !Material->TwoSided || !Material->GetUsageByFlag(MATUSAGE_SkeletalMesh)
            || Material->GetExpressions().Num() != 4) return false;
        const auto* Mask = Material->GetExpressionInputForProperty(MP_OpacityMask);
        const auto* Sample = Mask ? Cast<UMaterialExpressionTextureSample>(Mask->Expression) : nullptr;
        if (!Sample || Sample->Texture != Texture || Mask->OutputIndex != 4 || Sample->SamplerType != SAMPLERTYPE_Color) return false;
        const auto* BaseColor = Material->GetExpressionInputForProperty(MP_BaseColor);
        const auto* Multiply = BaseColor ? Cast<UMaterialExpressionMultiply>(BaseColor->Expression) : nullptr;
        const auto* Tint = Multiply ? Cast<UMaterialExpressionVectorParameter>(Multiply->B.Expression) : nullptr;
        const auto* RoughnessInput = Material->GetExpressionInputForProperty(MP_Roughness);
        const auto* Roughness = RoughnessInput ? Cast<UMaterialExpressionConstant>(RoughnessInput->Expression) : nullptr;
        if (!Multiply || Multiply->A.Expression != Sample || !Tint || Tint->ParameterName != TEXT("ColorTint")
            || !Tint->DefaultValue.Equals(FLinearColor(0.055f, 0.011f, 0.003f, 1))
            || !Roughness || !FMath::IsNearlyEqual(Roughness->R, 0.7f)
            || !Material->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit)) return false;
        TArray<TSharedPtr<FJsonValue>> Records;
        for (int32 Index = 0; Index < Meshes.Num(); ++Index)
        {
            USkeletalMesh* Mesh = Meshes[Index];
            USkeletalMesh* Base = LoadObject<USkeletalMesh>(nullptr, *(IncumbentHeroine + WaveMeshes[Index].Name));
            if (!Mesh || !Base || !Mesh->GetSkeleton() || Mesh->GetSkeleton() != Base->GetSkeleton()
                || Mesh->GetMaterials().Num() != WaveMeshes[Index].Slots || Base->GetMaterials().Num() != WaveMeshes[Index].Slots
                || !Mesh->GetImportedModel() || Mesh->GetImportedModel()->LODModels.Num() != 1) return false;
            const auto& Ref = Mesh->GetRefSkeleton();
            const auto& Original = Base->GetRefSkeleton();
            if (Ref.GetNum() != 54 || Ref.GetNum() != Original.GetNum()) return false;
            for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
                if (Ref.GetBoneName(Bone) != Original.GetBoneName(Bone)
                    || Ref.GetParentIndex(Bone) != Original.GetParentIndex(Bone)
                    || !Ref.GetRefBonePose()[Bone].Equals(Original.GetRefBonePose()[Bone], 0.01f)) return false;
            int32 Triangles = 0;
            for (const auto& Section : Mesh->GetImportedModel()->LODModels[0].Sections) Triangles += Section.NumTriangles;
            const FBoxSphereBounds Bounds = Mesh->GetBounds();
            const double Height = Bounds.BoxExtent.Z * 2;
            UE_LOG(LogFernSpike, Display, TEXT("Wave inventory %s: triangles=%d slots=%d bones=%d height_cm=%.6f"),
                *Mesh->GetPathName(), Triangles, Mesh->GetMaterials().Num(), Ref.GetNum(), Height);
            if (Triangles != WaveMeshes[Index].Triangles || Height < 155 || Height > 175
                || Bounds.Origin.ContainsNaN() || Bounds.BoxExtent.ContainsNaN()) return false;
            TArray<TSharedPtr<FJsonValue>> Slots;
            for (int32 SlotIndex = 0; SlotIndex < Mesh->GetMaterials().Num(); ++SlotIndex)
            {
                const auto& Slot = Mesh->GetMaterials()[SlotIndex];
                const auto& Old = Base->GetMaterials()[SlotIndex];
                if (!Slot.MaterialInterface || (SlotIndex == 7
                    ? Slot.MaterialSlotName != TEXT("M_Heroine_Hair_long01_Neutral") || Slot.MaterialInterface != Material
                    : Slot.MaterialSlotName != Old.MaterialSlotName || Slot.MaterialInterface != Old.MaterialInterface)) return false;
                auto SlotRecord = MakeShared<FJsonObject>();
                SlotRecord->SetNumberField(TEXT("index"), SlotIndex);
                SlotRecord->SetStringField(TEXT("slot"), Slot.MaterialSlotName.ToString());
                SlotRecord->SetStringField(TEXT("importedSlot"), Slot.ImportedMaterialSlotName.ToString());
                SlotRecord->SetStringField(TEXT("material"), Slot.MaterialInterface->GetPathName());
                Slots.Add(MakeShared<FJsonValueObject>(SlotRecord));
            }
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("object"), Mesh->GetPathName());
            Record->SetStringField(TEXT("incumbent"), Base->GetPathName());
            Record->SetStringField(TEXT("skeleton"), Mesh->GetSkeleton()->GetPathName());
            Record->SetNumberField(TEXT("bones"), Ref.GetNum());
            Record->SetNumberField(TEXT("triangles"), Triangles);
            Record->SetNumberField(TEXT("heightCm"), Height);
            Record->SetArrayField(TEXT("boundsOriginCm"), Vector(Bounds.Origin));
            Record->SetArrayField(TEXT("boundsExtentCm"), Vector(Bounds.BoxExtent));
            Record->SetBoolField(TEXT("referencePoseMatchesIncumbent"), true);
            Record->SetArrayField(TEXT("slots"), Slots);
            Records.Add(MakeShared<FJsonValueObject>(Record));
        }
        Result->SetArrayField(TEXT("meshes"), Records);
        Result->SetStringField(TEXT("texture"), Texture->GetPathName());
        Result->SetStringField(TEXT("material"), Material->GetPathName());
        Result->SetStringField(TEXT("transformPolicy"), TEXT("Scene/unit conversion once, uniform scale 1, zero import rotation/translation, unchanged incumbent reference pose"));
        Result->SetStringField(TEXT("scope"), TEXT("Frozen joined-wave interchange, not visual approval; incumbent content remains unchanged"));
        return true;
    }

    bool ImportWaves(FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
    {
        const FString Source = FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Characters/HairstyleRefinement"));
        TArray<FString> Names = { TEXT("Textures/T_LongWave_Neutral"), TEXT("Materials/M_Heroine_Hair_long01_Neutral") };
        for (const FWaveMesh& Wave : WaveMeshes) Names.Add(FString(TEXT("Meshes/")) + Wave.Name);
        for (const FString& Name : Names)
            if (FindPackage(nullptr, *(HairTrial + TEXT("/") + Name)) || FPackageName::DoesPackageExist(HairTrial + TEXT("/") + Name)) return false;
        Result->SetStringField(TEXT("stage"), TEXT("neutral-texture"));
        auto* TextureFactory = NewObject<UTextureFactory>();
        auto* TextureTask = NewObject<UAssetImportTask>();
        TextureTask->bAutomated = true;
        TextureTask->bReplaceExisting = false;
        TextureFactory->SetAssetImportTask(TextureTask);
        TextureFactory->CompressionSettings = TC_Default;
        bool Cancelled = false;
        auto* Texture = Cast<UTexture2D>(TextureFactory->ImportObject(UTexture2D::StaticClass(),
            CreatePackage(*(HairTrial + TEXT("/Textures/T_LongWave_Neutral"))), TEXT("T_LongWave_Neutral"),
            RF_Public | RF_Standalone, FPaths::Combine(Source, TEXT("Textures/T_LongWave_Neutral.png")), nullptr, Cancelled));
        if (!Texture || Cancelled || TextureFactory->GetAdditionalImportedObjects().Num() || Feedback.ReceivedUserCancel()) return false;
        Texture->SRGB = true;
        Texture->CompressionSettings = TC_Default;
        Texture->PostEditChange();
        auto* Material = NewObject<UMaterial>(CreatePackage(*(HairTrial + TEXT("/Materials/M_Heroine_Hair_long01_Neutral"))),
            TEXT("M_Heroine_Hair_long01_Neutral"), RF_Public | RF_Standalone);
        Material->BlendMode = BLEND_Masked;
        Material->TwoSided = true;
        Material->OpacityMaskClipValue = 0.333f;
        Material->SetShadingModel(MSM_DefaultLit);
        if (!Material->SetMaterialUsage(MATUSAGE_SkeletalMesh)) return false;
        auto* Sample = Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionTextureSample::StaticClass(), -600, 0));
        auto* Tint = Cast<UMaterialExpressionVectorParameter>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionVectorParameter::StaticClass(), -600, 200));
        auto* Multiply = Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionMultiply::StaticClass(), -300, 0));
        auto* Roughness = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(
            Material, UMaterialExpressionConstant::StaticClass(), -300, 300));
        if (!Sample || !Tint || !Multiply || !Roughness) return false;
        Sample->Texture = Texture;
        Sample->SamplerType = SAMPLERTYPE_Color;
        Tint->ParameterName = TEXT("ColorTint");
        Tint->DefaultValue = FLinearColor(0.055f, 0.011f, 0.003f, 1);
        Roughness->R = 0.7f;
        if (!UMaterialEditingLibrary::ConnectMaterialExpressions(Sample, TEXT("RGB"), Multiply, TEXT("A"))
            || !UMaterialEditingLibrary::ConnectMaterialExpressions(Tint, TEXT(""), Multiply, TEXT("B"))
            || !UMaterialEditingLibrary::ConnectMaterialProperty(Multiply, TEXT(""), MP_BaseColor)
            || !UMaterialEditingLibrary::ConnectMaterialProperty(Sample, TEXT("A"), MP_OpacityMask)
            || !UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness)) return false;
        Material->PostEditChange();
        TArray<USkeletalMesh*> Meshes;
        TArray<UObject*> Assets = { Texture, Material };
        for (const FWaveMesh& Wave : WaveMeshes)
        {
            Result->SetStringField(TEXT("stage"), FString(TEXT("skeletal-import:")) + Wave.Name);
            if (Feedback.ReceivedUserCancel()) return false;
            USkeletalMesh* Base = LoadObject<USkeletalMesh>(nullptr, *(IncumbentHeroine + Wave.Name));
            if (!Base || !Base->GetSkeleton() || Base->GetMaterials().Num() != Wave.Slots) return false;
            auto* Mesh = ImportRiggedMesh(FPaths::Combine(Source, TEXT("Joined"), FString(Wave.Name) + TEXT(".fbx")),
                HairTrial + TEXT("/Meshes/") + Wave.Name, Base->GetSkeleton(), Feedback);
            if (!Mesh) return false;
            TArray<USkinnedAsset*> Pending = { Mesh };
            FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
            if (Mesh->GetMaterials().Num() != Wave.Slots) return false;
            for (int32 Index = 0; Index < Wave.Slots; ++Index)
            {
                auto& Slot = Mesh->GetMaterials()[Index];
                const auto& Old = Base->GetMaterials()[Index];
                if (Index != 7 && Slot.MaterialSlotName != Old.MaterialSlotName) return false;
                Slot.MaterialInterface = Index == 7 ? Material : Old.MaterialInterface.Get();
                if (Index == 7) Slot.MaterialSlotName = TEXT("M_Heroine_Hair_long01_Neutral");
            }
            Mesh->PostEditChange();
            FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
            Meshes.Add(Mesh);
            Assets.Add(Mesh);
        }
        Result->SetStringField(TEXT("stage"), TEXT("measured-wave-inventory"));
        if (!WaveInventory(Meshes, Texture, Material, Result) || Feedback.ReceivedUserCancel()) return false;
        for (TObjectIterator<UObject> It; It; ++It)
            if (It->IsAsset() && It->GetOutermost()->GetName().StartsWith(HairTrial + TEXT("/")) && !Assets.Contains(*It)) return false;
        TSet<FString> LocalPackages, IncumbentPackages;
        for (UObject* Asset : Assets) LocalPackages.Add(Asset->GetOutermost()->GetName());
        for (const FWaveMesh& Wave : WaveMeshes)
        {
            const auto* Base = LoadObject<USkeletalMesh>(nullptr, *(IncumbentHeroine + Wave.Name));
            IncumbentPackages.Add(Base->GetSkeleton()->GetOutermost()->GetName());
            for (int32 Index = 0; Index < Base->GetMaterials().Num(); ++Index)
                if (Index != 7) IncumbentPackages.Add(Base->GetMaterials()[Index].MaterialInterface->GetOutermost()->GetName());
        }
        TArray<UObject*> Pending = Assets;
        TSet<UObject*> Seen;
        TArray<TSharedPtr<FJsonValue>> References;
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
                const FString Package = Reference->GetOutermost()->GetName();
                if (Package.StartsWith(TEXT("/Script/"))) continue;
                if (!LocalPackages.Contains(Package) && !IncumbentPackages.Contains(Package))
                {
                    UE_LOG(LogFernSpike, Error, TEXT("Unapproved wave reference: %s -> %s"),
                        *Object->GetPathName(), *Reference->GetPathName());
                    return false;
                }
                References.Add(MakeShared<FJsonValueString>(Reference->GetPathName()));
                if (LocalPackages.Contains(Package)) Pending.Add(Reference);
            }
        }
        Result->SetArrayField(TEXT("persistentObjectReferences"), References);
        Result->SetStringField(TEXT("stage"), TEXT("saving-eight-wave-packages"));
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
}

namespace Wardrobe
{
    const FString Root = TEXT("/Game/SurvivalGame/Characters/ModularClothing");
    const TCHAR* Bodies[] = {TEXT("Preferred"), TEXT("Willow"), TEXT("Hazel")};
    const TCHAR* Parts[] = {TEXT("Base_LongWave"), TEXT("Base_Bob"), TEXT("Base_Ponytail"),
        TEXT("Tunic"), TEXT("Apron"), TEXT("Shoes"), TEXT("Footwraps")};
    const int32 Triangles[3][7] = {
        {133454, 151118, 143044, 15374, 3536, 3320, 11448},
        {133442, 151157, 143028, 15374, 3536, 3320, 11448},
        {133478, 151311, 143040, 15374, 3536, 3320, 11448}
    };
    const int32 BobTriangles[] = {138104, 141640, 138303, 141839, 138285, 141821};
    const TCHAR* ClothNames[] = {TEXT("M_Modular_BaseBra"), TEXT("M_Modular_BaseBriefs"),
        TEXT("M_Modular_FootwrapCloth"), TEXT("M_Modular_FootwrapBinding")};
    const FLinearColor ClothColors[] = {
        FLinearColor(0.16f, 0.175f, 0.15f), FLinearColor(0.16f, 0.175f, 0.15f),
        FLinearColor(0.49f, 0.4f, 0.26f), FLinearColor(0.25f, 0.19f, 0.11f)
    };
    const FString BobMaterial = TEXT("M_Heroine_Hair_bob01_Neutral");

    struct FFit
    {
        FString Name, Package, Source, Reference;
        int32 Triangles;
        bool FullBody;
        TArray<FName> Roles;
    };

    TArray<FFit> Fits()
    {
        TArray<FFit> Result;
        for (int32 Body = 0; Body < 3; ++Body)
        {
            const FString Prefix = Body == 0 ? TEXT("SK_Heroine_") : FString::Printf(TEXT("SK_Heroine_%s_"), Bodies[Body]);
            for (int32 Part = 0; Part < 7; ++Part)
            {
                FFit Fit;
                Fit.Name = FString::Printf(TEXT("SK_Modular_%s_%s"), Bodies[Body], Parts[Part]);
                Fit.Package = Root + TEXT("/") + Bodies[Body] + TEXT("/") + Fit.Name;
                Fit.Source = FPaths::Combine(FPaths::ProjectDir(), Part < 2
                    ? FString(TEXT("Assets/Characters/HairstyleRefinement/Modular/")) + Fit.Name + TEXT(".fbx")
                    : FString(TEXT("Assets/Characters/ModularClothing/")) + Bodies[Body] + TEXT("/") + Fit.Name + TEXT(".fbx"));
                Fit.Reference = IncumbentHeroine + Prefix + (Part == 2 ? TEXT("Ponytail") : TEXT("LongWave_Apron"));
                Fit.Triangles = Triangles[Body][Part];
                Fit.FullBody = Part < 3;
                if (Part < 3)
                    Fit.Roles = {TEXT("M_Heroine_Skin"), TEXT("M_Modular_BaseBra"), TEXT("M_Modular_BaseBriefs"),
                        Part == 0 ? FName(TEXT("M_Heroine_Hair_long01_Neutral"))
                            : Part == 1 ? FName(*BobMaterial) : FName(TEXT("M_Heroine_Hair_ponytail01")),
                        TEXT("M_Heroine_LightEyes"), TEXT("M_Heroine_Eyebrows"), TEXT("M_Heroine_Eyelashes"),
                        TEXT("M_Heroine_Teeth"), TEXT("M_Heroine_Tongue")};
                else if (Part == 3) Fit.Roles = {TEXT("M_Heroine_MossLinen"), TEXT("M_Heroine_LinenTrim"),
                    TEXT("M_Heroine_ChestnutLeather"), TEXT("M_Heroine_Brass")};
                else if (Part == 4) Fit.Roles = {TEXT("M_Heroine_ApronTrim"), TEXT("M_Heroine_ApronLinen")};
                else if (Part == 5) Fit.Roles = {TEXT("M_Heroine_LeatherShoes")};
                else Fit.Roles = {ClothNames[2], ClothNames[3]};
                Result.Add(MoveTemp(Fit));
            }
            for (int32 Outfit = 0; Outfit < 2; ++Outfit)
            {
                FFit Fit;
                Fit.Name = Prefix + TEXT("Bob") + (Outfit ? TEXT("_Apron") : TEXT(""));
                Fit.Package = Root + TEXT("/JoinedBob/") + Fit.Name;
                Fit.Source = FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Characters/HairstyleRefinement/Joined"), Fit.Name + TEXT(".fbx"));
                Fit.Reference = IncumbentHeroine + Fit.Name;
                Fit.Triangles = BobTriangles[Body * 2 + Outfit];
                Fit.FullBody = true;
                Fit.Roles = {TEXT("M_Heroine_Brass"), TEXT("M_Heroine_Skin"), TEXT("M_Heroine_LightEyes"),
                    TEXT("M_Heroine_Eyebrows"), TEXT("M_Heroine_Eyelashes"), TEXT("M_Heroine_Teeth"),
                    TEXT("M_Heroine_Tongue"), FName(*BobMaterial), TEXT("M_Heroine_LeatherShoes"),
                    TEXT("M_Heroine_MossLinen"), TEXT("M_Heroine_LinenTrim"), TEXT("M_Heroine_ChestnutLeather")};
                if (Outfit)
                {
                    Fit.Roles[0] = TEXT("M_Heroine_ApronTrim");
                    Fit.Roles.Append({TEXT("M_Heroine_Brass"), TEXT("M_Heroine_ApronLinen")});
                }
                Result.Add(MoveTemp(Fit));
            }
        }
        return Result;
    }

    bool Reject(const TSharedRef<FJsonObject>& Result, const FString& Reason)
    {
        Result->SetStringField(TEXT("failureReason"), Reason);
        UE_LOG(LogFernSpike, Error, TEXT("Wardrobe admission rejected: %s"), *Reason);
        return false;
    }

    UMaterialInterface* Surface(FName Role, USkeletalMesh& Reference)
    {
        if (Role == TEXT("M_Heroine_Hair_long01_Neutral"))
            return LoadObject<UMaterial>(nullptr, *(HairTrial + TEXT("/Materials/") + Role.ToString()));
        if (Role == FName(*BobMaterial) || Role.ToString().StartsWith(TEXT("M_Modular_")))
            return LoadObject<UMaterial>(nullptr, *(Root + TEXT("/Materials/") + Role.ToString()));
        for (const auto& Slot : Reference.GetMaterials())
            if (Slot.MaterialSlotName == Role) return Slot.MaterialInterface;
        UE_LOG(LogFernSpike, Error, TEXT("Missing admitted material role %s in %s"), *Role.ToString(), *Reference.GetPathName());
        return nullptr;
    }

    bool ResolveSlots(const FFit& Fit, USkeletalMesh& Mesh, USkeletalMesh& Reference,
        const TSharedRef<FJsonObject>& Result, bool Bind)
    {
        if (Mesh.GetMaterials().Num() != Fit.Roles.Num())
            return Reject(Result, FString::Printf(TEXT("%s material count: actual=%d expected=%d"),
                *Fit.Name, Mesh.GetMaterials().Num(), Fit.Roles.Num()));
        TSet<FName> Seen;
        for (int32 Index = 0; Index < Mesh.GetMaterials().Num(); ++Index)
        {
            auto& Slot = Mesh.GetMaterials()[Index];
            const FName Role = Slot.ImportedMaterialSlotName;
            if (!Fit.Roles.Contains(Role) || Slot.MaterialSlotName != Role || Seen.Contains(Role))
                return Reject(Result, FString::Printf(TEXT("%s slot %d: unknown/duplicate/renamed role=%s imported=%s"),
                    *Fit.Name, Index, *Slot.MaterialSlotName.ToString(), *Role.ToString()));
            Seen.Add(Role);
            auto* Expected = Surface(Role, Reference);
            if (!Expected || (!Bind && Slot.MaterialInterface != Expected))
                return Reject(Result, FString::Printf(TEXT("%s slot %d role=%s material=%s expected=%s"),
                    *Fit.Name, Index, *Role.ToString(), *GetPathNameSafe(Slot.MaterialInterface), *GetPathNameSafe(Expected)));
            if (Fit.Package.StartsWith(Root + TEXT("/JoinedBob/")) && Role == FName(*BobMaterial) && Index != 7)
                return Reject(Result, TEXT("Canonical joined Bob hair slot7 differs: ") + Fit.Name);
            if (Bind) Slot.MaterialInterface = Expected;
        }
        return true;
    }

    UTexture2D* Texture(const TCHAR* Name, const FString& Source, FStopFeedback& Feedback)
    {
        if (Feedback.ReceivedUserCancel()) return nullptr;
        auto* Factory = NewObject<UTextureFactory>();
        auto* Task = NewObject<UAssetImportTask>();
        Task->bAutomated = true;
        Task->bReplaceExisting = false;
        Factory->SetAssetImportTask(Task);
        Factory->CompressionSettings = TC_Default;
        bool Cancelled = false;
        auto* Value = Cast<UTexture2D>(Factory->ImportObject(UTexture2D::StaticClass(),
            CreatePackage(*(Root + TEXT("/Textures/") + Name)), Name, RF_Public | RF_Standalone, Source, nullptr, Cancelled));
        if (!Value || Cancelled || Factory->GetAdditionalImportedObjects().Num() || Feedback.ReceivedUserCancel()) return nullptr;
        Value->SRGB = true;
        Value->CompressionSettings = TC_Default;
        Value->PostEditChange();
        return Value;
    }

    UMaterial* Material(const FString& Name, UTexture2D* Map, const FLinearColor& Color, bool Hair)
    {
        auto* Value = NewObject<UMaterial>(CreatePackage(*(Root + TEXT("/Materials/") + Name)),
            *Name, RF_Public | RF_Standalone);
        Value->BlendMode = Hair ? BLEND_Masked : BLEND_Opaque;
        Value->TwoSided = true;
        Value->OpacityMaskClipValue = 0.333f;
        Value->SetShadingModel(MSM_DefaultLit);
        if (!Value->SetMaterialUsage(MATUSAGE_SkeletalMesh)) return nullptr;
        auto* Sample = Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(Value,
            UMaterialExpressionTextureSample::StaticClass()));
        auto* Multiply = Cast<UMaterialExpressionMultiply>(UMaterialEditingLibrary::CreateMaterialExpression(Value,
            UMaterialExpressionMultiply::StaticClass()));
        auto* Rough = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(Value,
            UMaterialExpressionConstant::StaticClass()));
        if (!Sample || !Multiply || !Rough) return nullptr;
        Sample->Texture = Map;
        Sample->SamplerType = SAMPLERTYPE_Color;
        Multiply->A.Expression = Sample;
        Rough->R = Hair ? 0.7f : 0.92f;
        if (Hair)
        {
            auto* Tint = Cast<UMaterialExpressionVectorParameter>(UMaterialEditingLibrary::CreateMaterialExpression(Value,
                UMaterialExpressionVectorParameter::StaticClass()));
            if (!Tint) return nullptr;
            Tint->ParameterName = TEXT("ColorTint");
            Tint->DefaultValue = Color;
            Multiply->B.Expression = Tint;
            if (!UMaterialEditingLibrary::ConnectMaterialProperty(Sample, TEXT("A"), MP_OpacityMask)) return nullptr;
        }
        else
        {
            auto* Tint = Cast<UMaterialExpressionConstant3Vector>(UMaterialEditingLibrary::CreateMaterialExpression(Value,
                UMaterialExpressionConstant3Vector::StaticClass()));
            auto* UV = Cast<UMaterialExpressionTextureCoordinate>(UMaterialEditingLibrary::CreateMaterialExpression(Value,
                UMaterialExpressionTextureCoordinate::StaticClass()));
            if (!Tint || !UV) return nullptr;
            Tint->Constant = Color;
            UV->UTiling = UV->VTiling = 8;
            Sample->Coordinates.Expression = UV;
            Multiply->B.Expression = Tint;
        }
        if (!UMaterialEditingLibrary::ConnectMaterialProperty(Multiply, TEXT(""), MP_BaseColor)
            || !UMaterialEditingLibrary::ConnectMaterialProperty(Rough, TEXT(""), MP_Roughness)) return nullptr;
        Value->PostEditChange();
        return Value;
    }

    bool CheckMaterial(UMaterial* Value, UTexture2D* Map, const FLinearColor& Color, bool Hair)
    {
        if (!Value || !Map || Value->BlendMode != (Hair ? BLEND_Masked : BLEND_Opaque) || !Value->TwoSided
            || !Value->GetUsageByFlag(MATUSAGE_SkeletalMesh) || !Value->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit)
            || Value->GetExpressions().Num() != (Hair ? 4 : 5)) return false;
        const auto* Base = Value->GetExpressionInputForProperty(MP_BaseColor);
        const auto* Mul = Base ? Cast<UMaterialExpressionMultiply>(Base->Expression) : nullptr;
        const auto* Sample = Mul ? Cast<UMaterialExpressionTextureSample>(Mul->A.Expression) : nullptr;
        const auto* RoughInput = Value->GetExpressionInputForProperty(MP_Roughness);
        const auto* Rough = RoughInput ? Cast<UMaterialExpressionConstant>(RoughInput->Expression) : nullptr;
        if (!Sample || Sample->Texture != Map || Sample->SamplerType != SAMPLERTYPE_Color
            || !Rough || !FMath::IsNearlyEqual(Rough->R, Hair ? 0.7f : 0.92f)) return false;
        if (Hair)
        {
            const auto* Tint = Cast<UMaterialExpressionVectorParameter>(Mul->B.Expression);
            const auto* Mask = Value->GetExpressionInputForProperty(MP_OpacityMask);
            return Tint && Tint->ParameterName == TEXT("ColorTint") && Tint->DefaultValue.Equals(Color)
                && Mask && Mask->Expression == Sample && Mask->OutputIndex == 4
                && FMath::IsNearlyEqual(Value->OpacityMaskClipValue, 0.333f);
        }
        const auto* Tint = Cast<UMaterialExpressionConstant3Vector>(Mul->B.Expression);
        const auto* UV = Cast<UMaterialExpressionTextureCoordinate>(Sample->Coordinates.Expression);
        return Tint && Tint->Constant.Equals(Color) && UV && UV->CoordinateIndex == 0
            && FMath::IsNearlyEqual(UV->UTiling, 8.0f) && FMath::IsNearlyEqual(UV->VTiling, 8.0f);
    }

    bool Inventory(const TSharedRef<FJsonObject>& Result, TArray<UObject*>& Assets, TSet<FString>& External)
    {
        auto* Bob = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("/Textures/T_BobReuse_Neutral")));
        auto* Weave = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("/Textures/T_ModularWeave")));
        if (!Bob || !Weave || Bob->Source.GetSizeX() != 2048 || Bob->Source.GetSizeY() != 2048
            || Weave->Source.GetSizeX() != 256 || Weave->Source.GetSizeY() != 256 || !Bob->SRGB || !Weave->SRGB
            || Bob->CompressionSettings != TC_Default || Weave->CompressionSettings != TC_Default)
            return Reject(Result, TEXT("Texture identity, dimensions, sRGB or compression differs."));
        Assets = {Bob, Weave};
        auto* BobSurface = LoadObject<UMaterial>(nullptr, *(Root + TEXT("/Materials/") + BobMaterial));
        if (!CheckMaterial(BobSurface, Bob, FLinearColor(0.055f, 0.011f, 0.003f, 1), true))
            return Reject(Result, TEXT("Bob material graph or rendering properties differ."));
        Assets.Add(BobSurface);
        for (int32 Index = 0; Index < 4; ++Index)
        {
            auto* Cloth = LoadObject<UMaterial>(nullptr, *(Root + TEXT("/Materials/") + ClothNames[Index]));
            if (!CheckMaterial(Cloth, Weave, ClothColors[Index], false))
                return Reject(Result, FString::Printf(TEXT("Cloth material graph/properties differ: %s"), ClothNames[Index]));
            Assets.Add(Cloth);
        }
        TArray<TSharedPtr<FJsonValue>> Records;
        for (const FFit& Fit : Fits())
        {
            auto* Mesh = LoadObject<USkeletalMesh>(nullptr, *Fit.Package);
            auto* Reference = LoadObject<USkeletalMesh>(nullptr, *Fit.Reference);
            if (!Mesh || !Reference || !Reference->GetSkeleton() || Mesh->GetSkeleton() != Reference->GetSkeleton()
                || !Mesh->GetImportedModel() || Mesh->GetImportedModel()->LODModels.Num() != 1
                || Mesh->GetMaterials().Num() != Fit.Roles.Num())
                return Reject(Result, TEXT("Mesh/reference/skeleton/LOD/material-count differs: ") + Fit.Name);
            TArray<USkinnedAsset*> Pending = {Mesh};
            FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
            const auto& Ref = Mesh->GetRefSkeleton();
            const auto& Original = Reference->GetRefSkeleton();
            if (Ref.GetNum() != 54 || Ref.GetNum() != Original.GetNum())
                return Reject(Result, FString::Printf(TEXT("%s bone counts: actual=%d reference=%d expected=54"),
                    *Fit.Name, Ref.GetNum(), Original.GetNum()));
            for (int32 Bone = 0; Bone < Ref.GetNum(); ++Bone)
                if (Ref.GetBoneName(Bone) != Original.GetBoneName(Bone)
                    || Ref.GetParentIndex(Bone) != Original.GetParentIndex(Bone)
                    || !Ref.GetRefBonePose()[Bone].Equals(Original.GetRefBonePose()[Bone], 0.01f))
                    return Reject(Result, FString::Printf(TEXT("%s bone %d name/parent/bind differs: actual=%s expected=%s"),
                        *Fit.Name, Bone, *Ref.GetBoneName(Bone).ToString(), *Original.GetBoneName(Bone).ToString()));
            int32 ActualTriangles = 0;
            for (const auto& Section : Mesh->GetImportedModel()->LODModels[0].Sections) ActualTriangles += Section.NumTriangles;
            const auto Bounds = Mesh->GetBounds();
            const double Height = Bounds.BoxExtent.Z * 2;
            if (ActualTriangles != Fit.Triangles || Bounds.Origin.ContainsNaN() || Bounds.BoxExtent.ContainsNaN()
                || Bounds.BoxExtent.GetMin() <= 0 || !FMath::IsFinite(Height)
                || (Fit.FullBody ? Height < 155 || Height > 175 : Height <= 0 || Height > 140))
                return Reject(Result, FString::Printf(TEXT("%s geometry: triangles=%d expected=%d height_cm=%.6f extent=%s"),
                    *Fit.Name, ActualTriangles, Fit.Triangles, Height, *Bounds.BoxExtent.ToString()));
            TArray<TSharedPtr<FJsonValue>> Slots;
            External.Add(Reference->GetSkeleton()->GetOutermost()->GetName());
            if (!ResolveSlots(Fit, *Mesh, *Reference, Result, false)) return false;
            for (int32 Index = 0; Index < Mesh->GetMaterials().Num(); ++Index)
            {
                const auto& Slot = Mesh->GetMaterials()[Index];
                auto* Expected = Slot.MaterialInterface.Get();
                if (!Expected->GetOutermost()->GetName().StartsWith(Root + TEXT("/")))
                    External.Add(Expected->GetOutermost()->GetName());
                auto Record = MakeShared<FJsonObject>();
                Record->SetNumberField(TEXT("index"), Index);
                Record->SetNumberField(TEXT("sourceRoleIndex"), Fit.Roles.IndexOfByKey(Slot.ImportedMaterialSlotName));
                Record->SetStringField(TEXT("role"), Slot.MaterialSlotName.ToString());
                Record->SetStringField(TEXT("material"), Expected->GetPathName());
                Slots.Add(MakeShared<FJsonValueObject>(Record));
            }
            TArray<TSharedPtr<FJsonValue>> Sections;
            TSet<int32> UsedSlots;
            for (const auto& Section : Mesh->GetImportedModel()->LODModels[0].Sections)
            {
                if (!Mesh->GetMaterials().IsValidIndex(Section.MaterialIndex) || Section.NumTriangles <= 0)
                    return Reject(Result, TEXT("Invalid section material index/triangles: ") + Fit.Name);
                UsedSlots.Add(Section.MaterialIndex);
                auto SectionRecord = MakeShared<FJsonObject>();
                SectionRecord->SetNumberField(TEXT("materialIndex"), Section.MaterialIndex);
                SectionRecord->SetStringField(TEXT("role"), Mesh->GetMaterials()[Section.MaterialIndex].ImportedMaterialSlotName.ToString());
                SectionRecord->SetNumberField(TEXT("triangles"), Section.NumTriangles);
                Sections.Add(MakeShared<FJsonValueObject>(SectionRecord));
            }
            if (UsedSlots.Num() != Fit.Roles.Num())
                return Reject(Result, TEXT("Canonical material role has no mesh section: ") + Fit.Name);
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("object"), Mesh->GetPathName());
            Record->SetStringField(TEXT("source"), Fit.Source);
            Record->SetStringField(TEXT("reference"), Reference->GetPathName());
            Record->SetStringField(TEXT("skeleton"), Mesh->GetSkeleton()->GetPathName());
            Record->SetNumberField(TEXT("bones"), Ref.GetNum());
            Record->SetNumberField(TEXT("triangles"), ActualTriangles);
            Record->SetNumberField(TEXT("heightCm"), Height);
            Record->SetArrayField(TEXT("boundsOriginCm"), Vector(Bounds.Origin));
            Record->SetArrayField(TEXT("boundsExtentCm"), Vector(Bounds.BoxExtent));
            Record->SetBoolField(TEXT("referencePoseMatchesIncumbent"), true);
            Record->SetArrayField(TEXT("slots"), Slots);
            Record->SetArrayField(TEXT("sections"), Sections);
            Records.Add(MakeShared<FJsonValueObject>(Record));
            Assets.Add(Mesh);
            UE_LOG(LogFernSpike, Display, TEXT("Wardrobe inventory %s: triangles=%d height_cm=%.6f"), *Fit.Name, ActualTriangles, Height);
        }
        Result->SetArrayField(TEXT("meshes"), Records);
        Result->SetNumberField(TEXT("packages"), Assets.Num());
        Result->SetStringField(TEXT("transformPolicy"), TEXT("Scene/unit conversion once; scale1; zero import offsets; original reference bind/pivot"));
        Result->SetStringField(TEXT("materialPolicy"), TEXT("Original roles retained; admitted neutral hair; opaque fixed-color base/footwraps with UV8 weave"));
        return Assets.Num() == 34;
    }

    bool Audit(const TArray<UObject*>& Assets, const TSet<FString>& External, const TSharedRef<FJsonObject>& Result)
    {
        return AuditAssetReferences(Root, Assets, External, Result);
    }

    bool Import(FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
    {
        TArray<FString> Packages = {Root + TEXT("/Textures/T_BobReuse_Neutral"), Root + TEXT("/Textures/T_ModularWeave"),
            Root + TEXT("/Materials/") + BobMaterial};
        for (const auto* Name : ClothNames) Packages.Add(Root + TEXT("/Materials/") + Name);
        for (const auto& Fit : Fits()) Packages.Add(Fit.Package);
        for (const auto& Package : Packages)
            if (FindPackage(nullptr, *Package) || FPackageName::DoesPackageExist(Package))
                return Reject(Result, TEXT("Reserved package already exists: ") + Package);
        Result->SetStringField(TEXT("stage"), TEXT("two-textures-five-materials"));
        auto* Bob = Texture(TEXT("T_BobReuse_Neutral"),
            FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Characters/HairstyleRefinement/Textures/T_BobReuse_Neutral.png")), Feedback);
        auto* Weave = Texture(TEXT("T_ModularWeave"),
            FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Characters/ModularClothing/Textures/T_ModularWeave.png")), Feedback);
        if (!Bob || !Weave || !Material(BobMaterial, Bob, FLinearColor(0.055f, 0.011f, 0.003f, 1), true)) return false;
        for (int32 Index = 0; Index < 4; ++Index)
            if (Feedback.ReceivedUserCancel() || !Material(ClothNames[Index], Weave, ClothColors[Index], false)) return false;
        for (const auto& Fit : Fits())
        {
            Result->SetStringField(TEXT("stage"), TEXT("skeletal-import:") + Fit.Name);
            UE_LOG(LogFernSpike, Display, TEXT("Importing canonical wardrobe fit %s"), *Fit.Name);
            auto* Reference = LoadObject<USkeletalMesh>(nullptr, *Fit.Reference);
            if (!Reference) return Reject(Result, TEXT("Missing incumbent reference: ") + Fit.Reference);
            if (Feedback.ReceivedUserCancel()) return false;
            auto* Mesh = Hair::ImportRiggedMesh(Fit.Source, Fit.Package, Reference->GetSkeleton(), Feedback);
            if (!Mesh || Mesh->GetMaterials().Num() != Fit.Roles.Num())
                return Reject(Result, FString::Printf(TEXT("%s imported mesh/material count: actual=%d expected=%d"),
                    *Fit.Name, Mesh ? Mesh->GetMaterials().Num() : -1, Fit.Roles.Num()));
            if (!ResolveSlots(Fit, *Mesh, *Reference, Result, true)) return false;
            Mesh->PostEditChange();
            TArray<USkinnedAsset*> Pending = {Mesh};
            FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
        }
        Result->SetStringField(TEXT("stage"), TEXT("inventory-reference-audit"));
        TArray<UObject*> Assets;
        TSet<FString> External;
        if (!Inventory(Result, Assets, External) || !Audit(Assets, External, Result) || Feedback.ReceivedUserCancel()) return false;
        Result->SetStringField(TEXT("stage"), TEXT("saving-exactly34-packages"));
        for (auto* Asset : Assets)
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
}

namespace Tree
{
    const FString Root = TEXT("/Game/Trials/TreeSmall02_20260921_01");
    const FString Source = FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Environment/TreeSmall02Prepared/v3"));
    const TCHAR* MeshName = TEXT("SM_TreeSmall02_LOD2");
    const TCHAR* Roles[] = {TEXT("tree_small_02_branches"), TEXT("tree_small_02_leaves"), TEXT("tree_small_02_trunk")};
    const TCHAR* Materials[] = {TEXT("M_TreeSmall02_Branches"), TEXT("M_TreeSmall02_Leaves"), TEXT("M_TreeSmall02_Trunk")};
    const int32 Triangles[] = {23702, 193938, 14145};
    const FMap TextureMaps[] = {
        {TEXT("tree_small_02_branch_diff_2k.png"), TEXT("T_TreeSmall02_Branch_Diff"), MP_BaseColor, TC_Default, SAMPLERTYPE_Color, true},
        {TEXT("tree_small_02_branch_nor_dx_2k.png"), TEXT("T_TreeSmall02_Branch_NormalDX"), MP_Normal, TC_Normalmap, SAMPLERTYPE_Normal, false},
        {TEXT("tree_small_02_branch_rough_2k.png"), TEXT("T_TreeSmall02_Branch_Roughness"), MP_Roughness, TC_Masks, SAMPLERTYPE_Masks, false},
        {TEXT("tree_small_02_branch_ao_2k.png"), TEXT("T_TreeSmall02_Branch_AO"), MP_AmbientOcclusion, TC_Masks, SAMPLERTYPE_Masks, false},
        {TEXT("tree_small_02_leaves_diff_2k.png"), TEXT("T_TreeSmall02_Leaves_Diff"), MP_BaseColor, TC_Default, SAMPLERTYPE_Color, true},
        {TEXT("tree_small_02_leaves_nor_dx_2k.png"), TEXT("T_TreeSmall02_Leaves_NormalDX"), MP_Normal, TC_Normalmap, SAMPLERTYPE_Normal, false},
        {TEXT("tree_small_02_leaves_rough_2k.png"), TEXT("T_TreeSmall02_Leaves_Roughness"), MP_Roughness, TC_Masks, SAMPLERTYPE_Masks, false},
        {TEXT("tree_small_02_leaves_ao_2k.png"), TEXT("T_TreeSmall02_Leaves_AO"), MP_AmbientOcclusion, TC_Masks, SAMPLERTYPE_Masks, false},
        {TEXT("tree_small_02_leaves_alpha_2k.png"), TEXT("T_TreeSmall02_Leaves_Alpha"), MP_OpacityMask, TC_Masks, SAMPLERTYPE_Masks, false},
        {TEXT("tree_small_02_diff_2k.jpg"), TEXT("T_TreeSmall02_Trunk_Diff"), MP_BaseColor, TC_Default, SAMPLERTYPE_Color, true},
        {TEXT("tree_small_02_nor_dx_2k.png"), TEXT("T_TreeSmall02_Trunk_NormalDX"), MP_Normal, TC_Normalmap, SAMPLERTYPE_Normal, false},
        {TEXT("tree_small_02_rough_2k.png"), TEXT("T_TreeSmall02_Trunk_Roughness"), MP_Roughness, TC_Masks, SAMPLERTYPE_Masks, false},
        {TEXT("tree_small_02_ao_2k.png"), TEXT("T_TreeSmall02_Trunk_AO"), MP_AmbientOcclusion, TC_Masks, SAMPLERTYPE_Masks, false}
    };

    int32 Role(FName Name)
    {
        for (int32 Index = 0; Index < 3; ++Index)
            if (Name == Roles[Index]) return Index;
        return INDEX_NONE;
    }

    int32 MapRole(int32 Index) { return Index < 4 ? 0 : Index < 9 ? 1 : 2; }

    bool Reject(const TSharedRef<FJsonObject>& Result, const FString& Reason)
    {
        Result->SetStringField(TEXT("failure"), Reason);
        UE_LOG(LogFernSpike, Error, TEXT("Tree admission failed: %s"), *Reason);
        return false;
    }

    TArray<FString> Packages()
    {
        TArray<FString> Names = {Root + TEXT("/Meshes/") + MeshName};
        for (const auto* Name : Materials) Names.Add(Root + TEXT("/Materials/") + Name);
        for (const FMap& Map : TextureMaps) Names.Add(Root + TEXT("/Textures/") + Map.Name);
        return Names;
    }

    FBox SourceBoundsInUnreal(bool ReferencedOnly)
    {
        const FVector Min(-1.310724139213562, ReferencedOnly ? -2.907623291015625 : -2.9118497371673584, -0.024194231256842613);
        const FVector Max(1.6107107400894165, ReferencedOnly ? 1.3830232620239258 : 1.383678913116455, 4.539843559265137);
        const FQuat ModelRotation = FQuat(FVector::UpVector, FMath::DegreesToRadians(180.00000500895632))
            * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(-9.334666828389418e-6));
        FBox Converted(ForceInit);
        for (int32 Corner = 0; Corner < 8; ++Corner)
        {
            const FVector Local(Corner & 1 ? Max.X : Min.X, Corner & 2 ? Max.Y : Min.Y, Corner & 4 ? Max.Z : Min.Z);
            const FVector Model = ModelRotation.RotateVector(Local);
            // FBX +Y-front/-X-right to -Y-front/+X-right, then UE's Y handedness flip; metres to cm once.
            Converted += FVector(-Model.X, Model.Y, Model.Z) * 100;
        }
        return Converted;
    }

    double UvDeterminant(const FVector2f& A, const FVector2f& B, const FVector2f& C)
    {
        return (static_cast<double>(B.X) - A.X) * (static_cast<double>(C.Y) - A.Y)
            - (static_cast<double>(B.Y) - A.Y) * (static_cast<double>(C.X) - A.X);
    }

    void RecordTreeBasis(const FMeshDescription& Description, const TCHAR* Field, const TSharedRef<FJsonObject>& Result)
    {
        const FStaticMeshConstAttributes Attributes(Description);
        const auto Slots = Attributes.GetPolygonGroupMaterialSlotNames();
        const auto Normals = Attributes.GetVertexInstanceNormals();
        const auto Tangents = Attributes.GetVertexInstanceTangents();
        const auto Signs = Attributes.GetVertexInstanceBinormalSigns();
        int32 InvalidNormals[3] = {}, InvalidTangents[3] = {}, InvalidBinormals[3] = {};
        for (const FTriangleID Triangle : Description.Triangles().GetElementIDs())
        {
            const int32 Index = Role(Slots[Description.GetTrianglePolygonGroup(Triangle)]);
            if (Index == INDEX_NONE) continue;
            for (const FVertexInstanceID Corner : Description.GetTriangleVertexInstances(Triangle))
            {
                const FVector3f Normal = Normals[Corner], Tangent = Tangents[Corner];
                const FVector3f Binormal = FVector3f::CrossProduct(Normal, Tangent) * Signs[Corner];
                if (Normal.ContainsNaN() || Normal.IsNearlyZero()) ++InvalidNormals[Index];
                if (Tangent.ContainsNaN() || Tangent.IsNearlyZero()) ++InvalidTangents[Index];
                if (Binormal.ContainsNaN() || Binormal.IsNearlyZero()) ++InvalidBinormals[Index];
            }
        }
        TArray<TSharedPtr<FJsonValue>> Records;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("role"), Roles[Index]);
            Record->SetNumberField(TEXT("invalidNormalCorners"), InvalidNormals[Index]);
            Record->SetNumberField(TEXT("invalidTangentCorners"), InvalidTangents[Index]);
            Record->SetNumberField(TEXT("invalidBinormalCorners"), InvalidBinormals[Index]);
            Records.Add(MakeShared<FJsonValueObject>(Record));
        }
        Result->SetArrayField(Field, Records);
        int32 Orphans = 0, OrphanZeroNormals = 0;
        for (const FVertexInstanceID Corner : Description.VertexInstances().GetElementIDs())
        {
            if (Description.GetVertexInstanceConnectedTriangleIDs(Corner).Num() != 0) continue;
            ++Orphans;
            if (Normals[Corner].IsNearlyZero()) ++OrphanZeroNormals;
        }
        Result->SetNumberField(FString(Field) + TEXT("OrphanVertexInstances"), Orphans);
        Result->SetNumberField(FString(Field) + TEXT("OrphanZeroNormals"), OrphanZeroNormals);
    }

    bool RouteBranchUvs(UStaticMesh& Mesh, const TSharedRef<FJsonObject>& Result)
    {
        auto* Description = Mesh.GetMeshDescription(0);
        if (!Description) return Reject(Result, TEXT("No editable imported mesh description."));
        FStaticMeshAttributes Attributes(*Description);
        auto UVs = Attributes.GetVertexInstanceUVs();
        auto Normals = Attributes.GetVertexInstanceNormals();
        const auto Slots = Attributes.GetPolygonGroupMaterialSlotNames();
        if (UVs.GetNumChannels() != 2) return Reject(Result, TEXT("Expected both frozen source UV channels."));
        RecordTreeBasis(*Description, TEXT("meshDescriptionBasisBefore"), Result);
        TSet<FVertexInstanceID> BranchCorners;
        TSet<FVertexInstanceID> OtherCorners;
        int32 ZeroNormalCorners[3] = {};
        int32 BranchTriangles = 0;
        for (const FTriangleID Triangle : Description->Triangles().GetElementIDs())
        {
            const int32 Index = Role(Slots[Description->GetTrianglePolygonGroup(Triangle)]);
            if (Index == INDEX_NONE) return Reject(Result, TEXT("Unexpected UV material role."));
            const auto Corners = Description->GetTriangleVertexInstances(Triangle);
            for (const FVertexInstanceID Corner : Corners)
            {
                if (Normals[Corner].ContainsNaN()) return Reject(Result, TEXT("Nonfinite source normal."));
                if (Normals[Corner] == FVector3f::ZeroVector) ++ZeroNormalCorners[Index];
                if (Index != 0) OtherCorners.Add(Corner);
            }
            const int32 Active = Index == 0 ? 1 : 0;
            const double ActiveDet = UvDeterminant(UVs.Get(Corners[0], Active), UVs.Get(Corners[1], Active), UVs.Get(Corners[2], Active));
            const double InactiveDet = UvDeterminant(UVs.Get(Corners[0], 1 - Active), UVs.Get(Corners[1], 1 - Active), UVs.Get(Corners[2], 1 - Active));
            if (!FMath::IsFinite(ActiveDet) || ActiveDet == 0 || InactiveDet != 0)
                return Reject(Result, FString::Printf(TEXT("Native source UV occupancy differs in role%d."), Index));
            if (Index == 0)
            {
                ++BranchTriangles;
                for (const FVertexInstanceID Corner : Corners) BranchCorners.Add(Corner);
            }
        }
        if (BranchTriangles != 23702) return Reject(Result, TEXT("Branch-only UV routing scope differs."));
        for (const FVertexInstanceID Corner : BranchCorners)
            if (OtherCorners.Contains(Corner)) return Reject(Result, TEXT("UV vertex instance is shared across incompatible roles."));
        Result->SetArrayField(TEXT("nativeZeroNormalCornersBefore"), {
            MakeShared<FJsonValueNumber>(ZeroNormalCorners[0]), MakeShared<FJsonValueNumber>(ZeroNormalCorners[1]),
            MakeShared<FJsonValueNumber>(ZeroNormalCorners[2])});
        for (const FVertexInstanceID Corner : BranchCorners)
        {
            const FVector2f Original0 = UVs.Get(Corner, 0);
            const FVector2f Original1 = UVs.Get(Corner, 1);
            UVs.Set(Corner, 0, Original1);
            UVs.Set(Corner, 1, Original0);
        }
        TArray<FVector3f> OriginalNormals;
        OriginalNormals.SetNum(Description->VertexInstances().GetArraySize());
        for (const FVertexInstanceID Corner : Description->VertexInstances().GetElementIDs())
            OriginalNormals[Corner.GetValue()] = Normals[Corner];
        FStaticMeshOperations::ComputeTriangleTangentsAndNormals(*Description);
        // Tangents-only leaves valid custom normals intact; the engine fills only invalid normals.
        FStaticMeshOperations::ComputeTangentsAndNormals(*Description, EComputeNTBsFlags::Tangents | EComputeNTBsFlags::UseMikkTSpace);
        RecordTreeBasis(*Description, TEXT("meshDescriptionBasisAfterGroupedRepair"), Result);
        const auto TriangleNormals = Attributes.GetTriangleNormals();
        int32 FaceFallbacks[3] = {};
        for (const FVertexInstanceID Corner : Description->VertexInstances().GetElementIDs())
        {
            if (!Normals[Corner].IsNearlyZero()) continue;
            const auto Connected = Description->GetVertexInstanceConnectedTriangleIDs(Corner);
            if (Connected.Num() == 0) continue;
            Result->SetNumberField(TEXT("lastInvalidNormalVertexInstance"), Corner.GetValue());
            Result->SetNumberField(TEXT("lastInvalidNormalConnectedTriangles"), Connected.Num());
            if (OriginalNormals[Corner.GetValue()] != FVector3f::ZeroVector || Connected.Num() != 1)
                return Reject(Result, TEXT("Remaining invalid normal is not an exact-zero single-face source corner."));
            const FTriangleID Triangle = Connected[0];
            const int32 Index = Role(Slots[Description->GetTrianglePolygonGroup(Triangle)]);
            const FVector3f FaceNormal = TriangleNormals[Triangle];
            if (Index == INDEX_NONE || FaceNormal.ContainsNaN() || FaceNormal.IsNearlyZero())
                return Reject(Result, TEXT("No valid engine-computed face normal for the invalid source corner."));
            Normals[Corner] = FaceNormal;
            ++FaceFallbacks[Index];
        }
        Result->SetArrayField(TEXT("singleFaceZeroNormalFallbacks"), {
            MakeShared<FJsonValueNumber>(FaceFallbacks[0]), MakeShared<FJsonValueNumber>(FaceFallbacks[1]),
            MakeShared<FJsonValueNumber>(FaceFallbacks[2])});
        FStaticMeshOperations::ComputeMikktTangents(*Description, false);
        int32 RepairedNormals = 0;
        for (const FVertexInstanceID Corner : Description->VertexInstances().GetElementIDs())
        {
            const FVector3f Original = OriginalNormals[Corner.GetValue()];
            if (Description->GetVertexInstanceConnectedTriangleIDs(Corner).Num() == 0)
            {
                if (Original != Normals[Corner]) return Reject(Result, TEXT("Unreferenced normal unexpectedly changed."));
                continue;
            }
            if (Normals[Corner].ContainsNaN() || Normals[Corner].IsNearlyZero())
                return Reject(Result, TEXT("Engine tangent construction left an invalid normal."));
            if (Original != Normals[Corner])
            {
                if (Original != FVector3f::ZeroVector)
                    return Reject(Result, TEXT("Engine tangent construction changed a nonzero custom normal."));
                ++RepairedNormals;
            }
        }
        Result->SetNumberField(TEXT("repairedZeroNormalVertexInstances"), RepairedNormals);
        Result->SetStringField(TEXT("normalAdaptation"), TEXT("Correction02: UE triangle/grouped repair, remaining exact-zero single-face corners use their engine-computed face normal, then MikkTSpace. Nonzero custom normals compared exactly; ambiguous/invalid face fallback rejected."));
        Mesh.CommitMeshDescription(0);
        auto& Build = Mesh.GetSourceModel(0).BuildSettings;
        Build.bRecomputeTangents = true;
        Build.bRecomputeNormals = false;
        Build.bUseMikkTSpace = true;
        Result->SetNumberField(TEXT("routedBranchTriangles"), BranchTriangles);
        Result->SetNumberField(TEXT("swappedBranchVertexInstances"), BranchCorners.Num());
        Result->SetStringField(TEXT("uvAdaptation"), TEXT("Branch-only invertible UV0/UV1 swap; both source channels preserved, active authored UVs feed runtimeUV0/MikkTSpace. Leaf/trunk channels, positions, triangles and nonzero custom normals unchanged."));
        Mesh.PostEditChange();
        FStaticMeshCompilingManager::Get().FinishCompilation({&Mesh});
        RecordTreeBasis(*Mesh.GetMeshDescription(0), TEXT("meshDescriptionBasisAfter"), Result);
        return true;
    }

    bool Measure(UStaticMesh* Mesh, FKSphylElem& Collision, const TSharedRef<FJsonObject>& Result)
    {
        if (!Mesh || Mesh->GetNumSourceModels() != 1 || !Mesh->GetMeshDescription(0)
            || Mesh->GetStaticMaterials().Num() != 3 || Mesh->GetNaniteSettings().bEnabled
            || Mesh->GetMeshDescription(0)->Triangles().Num() != 231785 || Mesh->GetNumUVChannels(0) != 2)
            return Reject(Result, TEXT("Mesh/LOD/triangle/UV/material counts differ."));
        const auto* Data = Cast<UFbxStaticMeshImportData>(Mesh->GetAssetImportData());
        if (!Data || !Data->bConvertScene || !Data->bConvertSceneUnit || !Data->bTransformVertexToAbsolute
            || Data->bBakePivotInVertex || Data->bForceFrontXAxis || Data->ImportUniformScale != 1
            || !Data->ImportTranslation.IsZero() || !Data->ImportRotation.IsZero()
            || Data->NormalImportMethod != FBXNIM_ImportNormals || Data->bGenerateLightmapUVs
            || Data->bAutoGenerateCollision || Data->bBuildNanite)
            return Reject(Result, TEXT("Actual persisted source-transform settings differ."));
        const FBox Bounds = Mesh->GetBoundingBox();
        const FBox Expected = SourceBoundsInUnreal(true);
        const FBox AllControlPoints = SourceBoundsInUnreal(false);
        Result->SetArrayField(TEXT("boundsMinCm"), Vector(Bounds.Min));
        Result->SetArrayField(TEXT("boundsMaxCm"), Vector(Bounds.Max));
        Result->SetArrayField(TEXT("boundsExtentCm"), Vector(Bounds.GetExtent()));
        Result->SetArrayField(TEXT("importTranslationCm"), Vector(Data->ImportTranslation));
        Result->SetArrayField(TEXT("expectedReferencedMinCm"), Vector(Expected.Min));
        Result->SetArrayField(TEXT("expectedReferencedMaxCm"), Vector(Expected.Max));
        Result->SetArrayField(TEXT("expectedAllControlPointsMinCm"), Vector(AllControlPoints.Min));
        Result->SetArrayField(TEXT("expectedAllControlPointsMaxCm"), Vector(AllControlPoints.Max));
        Result->SetNumberField(TEXT("sourceControlPoints"), 424817);
        Result->SetNumberField(TEXT("sourceReferencedControlPoints"), 384193);
        Result->SetStringField(TEXT("sourceBoundsPolicy"), TEXT("Selected LOD2 polygon-referenced extrema; transform all8 corners through raw model rotation, FBX axis basis, UE handedness and100cm/metre. Unused FBX control points are not rendered bounds."));
        UE_LOG(LogFernSpike, Display, TEXT("Tree bounds actual=%s extent=%s expected-referenced=%s expected-all-controls=%s import-translation=%s"),
            *Bounds.ToString(), *Bounds.GetExtent().ToString(), *Expected.ToString(), *AllControlPoints.ToString(), *Data->ImportTranslation.ToString());
        if (!Bounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN()
            || !Bounds.Min.Equals(Expected.Min, 0.2) || !Bounds.Max.Equals(Expected.Max, 0.2))
            return Reject(Result, TEXT("Measured source units/bounds differ."));
        const FMeshDescription& Description = *Mesh->GetMeshDescription(0);
        RecordTreeBasis(Description, TEXT("meshDescriptionBasisAfter"), Result);
        const FStaticMeshConstAttributes Attributes(Description);
        const auto Positions = Attributes.GetVertexPositions();
        const auto SlotNames = Attributes.GetPolygonGroupMaterialSlotNames();
        const auto UVs = Attributes.GetVertexInstanceUVs();
        const auto Normals = Attributes.GetVertexInstanceNormals();
        if (UVs.GetNumChannels() != 2) return Reject(Result, TEXT("Source UV channels differ."));
        for (const FVertexInstanceID Vertex : Description.VertexInstances().GetElementIDs())
            for (int32 Channel = 0; Channel < 2; ++Channel)
                if (UVs.Get(Vertex, Channel).ContainsNaN()) return Reject(Result, TEXT("Nonfinite source UV."));
        int32 Counts[3] = {};
        int32 ShortNormalCorners[3] = {};
        int32 ActiveUvDegenerates[3] = {};
        int32 InactiveUvDegenerates[3] = {};
        FBox LowerTrunk(ForceInit);
        TArray<FVector> LowerVertices;
        for (const FTriangleID Triangle : Description.Triangles().GetElementIDs())
        {
            const int32 Index = Role(SlotNames[Description.GetTrianglePolygonGroup(Triangle)]);
            if (Index == INDEX_NONE) return Reject(Result, TEXT("Unexpected polygon material identity."));
            ++Counts[Index];
            const auto Corners = Description.GetTriangleVertexInstances(Triangle);
            for (const FVertexInstanceID Corner : Corners)
            {
                if (Normals[Corner].ContainsNaN()) return Reject(Result, TEXT("Nonfinite imported custom normal."));
                if (Normals[Corner].SizeSquared() <= 1.e-8f) ++ShortNormalCorners[Index];
            }
            if (UvDeterminant(UVs.Get(Corners[0], 0), UVs.Get(Corners[1], 0), UVs.Get(Corners[2], 0)) == 0)
                ++ActiveUvDegenerates[Index];
            if (UvDeterminant(UVs.Get(Corners[0], 1), UVs.Get(Corners[1], 1), UVs.Get(Corners[2], 1)) == 0)
                ++InactiveUvDegenerates[Index];
            if (Index != 2) continue;
            for (const FVertexID Vertex : Description.GetTriangleVertices(Triangle))
            {
                const FVector Position(Positions[Vertex]);
                if (Position.ContainsNaN()) return Reject(Result, TEXT("Nonfinite trunk vertex."));
                if (Position.Z <= 200)
                {
                    LowerTrunk += Position;
                    LowerVertices.Add(Position);
                }
            }
        }
        TSet<int32> Seen;
        TArray<TSharedPtr<FJsonValue>> Slots;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const auto& Slot = Mesh->GetStaticMaterials()[Index];
            const int32 SourceRole = Role(Slot.ImportedMaterialSlotName);
            if (SourceRole == INDEX_NONE || Seen.Contains(SourceRole) || Counts[SourceRole] != Triangles[SourceRole]
                || ActiveUvDegenerates[SourceRole] != 0 || InactiveUvDegenerates[SourceRole] != Counts[SourceRole])
                return Reject(Result, TEXT("Imported material roles/section totals differ."));
            if (ShortNormalCorners[SourceRole] != 0)
                return Reject(Result, TEXT("Invalid custom normals remain after narrow native adaptation."));
            Seen.Add(SourceRole);
            auto Record = MakeShared<FJsonObject>();
            Record->SetNumberField(TEXT("index"), Index);
            Record->SetNumberField(TEXT("sourceRoleIndex"), SourceRole);
            Record->SetStringField(TEXT("role"), Roles[SourceRole]);
            Record->SetStringField(TEXT("slot"), Slot.MaterialSlotName.ToString());
            Record->SetStringField(TEXT("material"), Mesh->GetMaterial(Index) ? Mesh->GetMaterial(Index)->GetPathName() : TEXT(""));
            Record->SetNumberField(TEXT("triangles"), Counts[SourceRole]);
            Record->SetNumberField(TEXT("sourceActiveUvChannel"), SourceRole == 0 ? 1 : 0);
            Record->SetNumberField(TEXT("runtimeActiveUvChannel"), 0);
            Record->SetNumberField(TEXT("runtimeActiveUvDegenerateTriangles"), ActiveUvDegenerates[SourceRole]);
            Record->SetNumberField(TEXT("runtimeInactiveUvDegenerateTriangles"), InactiveUvDegenerates[SourceRole]);
            Record->SetNumberField(TEXT("nearZeroImportedNormalCorners"), ShortNormalCorners[SourceRole]);
            Slots.Add(MakeShared<FJsonValueObject>(Record));
        }
        if (!LowerTrunk.IsValid || LowerTrunk.GetSize().Z < 150 || LowerTrunk.GetSize().Z > 205)
            return Reject(Result, TEXT("Lower-trunk measurement is missing or implausible."));
        Collision.Center = LowerTrunk.GetCenter();
        double Radius = 0;
        for (const FVector& Vertex : LowerVertices)
            Radius = FMath::Max(Radius, FVector2D(Vertex - Collision.Center).Size());
        if (!FMath::IsFinite(Radius) || Radius < 2 || Radius > 60)
            return Reject(Result, FString::Printf(TEXT("Measured trunk collision radius is implausible: %.6fcm."), Radius));
        Collision.Radius = Radius;
        Collision.Length = LowerTrunk.GetSize().Z - 2 * Radius;
        Collision.Rotation = FRotator::ZeroRotator;
        Result->SetStringField(TEXT("mesh"), Mesh->GetPathName());
        Result->SetNumberField(TEXT("triangles"), 231785);
        Result->SetNumberField(TEXT("uvChannels"), UVs.GetNumChannels());
        Result->SetArrayField(TEXT("slots"), Slots);
        Result->SetArrayField(TEXT("boundsMinCm"), Vector(Bounds.Min));
        Result->SetArrayField(TEXT("boundsMaxCm"), Vector(Bounds.Max));
        Result->SetArrayField(TEXT("lowerTrunkMinCm"), Vector(LowerTrunk.Min));
        Result->SetArrayField(TEXT("lowerTrunkMaxCm"), Vector(LowerTrunk.Max));
        Result->SetNumberField(TEXT("measuredTrunkVertexSamples"), LowerVertices.Num());
        Result->SetArrayField(TEXT("collisionCenterCm"), Vector(Collision.Center));
        Result->SetNumberField(TEXT("collisionRadiusCm"), Collision.Radius);
        Result->SetNumberField(TEXT("collisionCylinderLengthCm"), Collision.Length);
        Result->SetNumberField(TEXT("importUniformScale"), Data->ImportUniformScale);
        Result->SetBoolField(TEXT("convertScene"), Data->bConvertScene);
        Result->SetBoolField(TEXT("convertSceneUnit"), Data->bConvertSceneUnit);
        Result->SetBoolField(TEXT("transformVertexToAbsolute"), Data->bTransformVertexToAbsolute);
        Result->SetBoolField(TEXT("nanite"), Mesh->GetNaniteSettings().bEnabled);
        return true;
    }

    bool Inventory(TArray<UObject*>& Assets, const TSharedRef<FJsonObject>& Result)
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *(Root + TEXT("/Meshes/") + MeshName));
        if (!Mesh) return Reject(Result, TEXT("Missing exact tree mesh."));
        FStaticMeshCompilingManager::Get().FinishCompilation({Mesh});
        FKSphylElem Measured;
        if (!Measure(Mesh, Measured, Result)) return false;
        const auto* Body = Mesh->GetBodySetup();
        if (!Body || Body->CollisionTraceFlag != CTF_UseSimpleAsComplex || Body->AggGeom.GetElementCount() != 1
            || Body->AggGeom.SphylElems.Num() != 1)
            return Reject(Result, TEXT("Expected only one simple trunk capsule."));
        const auto& Actual = Body->AggGeom.SphylElems[0];
        if (!Actual.Center.Equals(Measured.Center, 0.01) || !Actual.Rotation.IsZero()
            || !FMath::IsNearlyEqual(Actual.Radius, Measured.Radius, 0.01f)
            || !FMath::IsNearlyEqual(Actual.Length, Measured.Length, 0.01f))
            return Reject(Result, TEXT("Persisted collision differs from measured trunk."));
        Assets.Add(Mesh);
        TArray<UMaterial*> Bound;
        TArray<TSharedPtr<FJsonValue>> MaterialRecords, TextureRecords;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            auto* Material = LoadObject<UMaterial>(nullptr, *(Root + TEXT("/Materials/") + Materials[Index]));
            if (!Material || Material->BlendMode != (Index == 1 ? BLEND_Masked : BLEND_Opaque)
                || Material->TwoSided != (Index == 1) || !Material->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit)
                || Material->GetExpressions().Num() != (Index == 1 ? 5 : 4)
                || (Index == 1 && Material->OpacityMaskClipValue != 0.5f))
                return Reject(Result, TEXT("PBR material state differs."));
            Bound.Add(Material);
            Assets.Add(Material);
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("object"), Material->GetPathName());
            Record->SetBoolField(TEXT("twoSided"), Material->TwoSided);
            Record->SetBoolField(TEXT("masked"), Material->BlendMode == BLEND_Masked);
            Record->SetNumberField(TEXT("opacityClip"), Material->OpacityMaskClipValue);
            Record->SetStringField(TEXT("shadingModel"), TEXT("DefaultLit"));
            MaterialRecords.Add(MakeShared<FJsonValueObject>(Record));
        }
        for (int32 Index = 0; Index < 3; ++Index)
            if (Mesh->GetMaterial(Index) != Bound[Role(Mesh->GetStaticMaterials()[Index].ImportedMaterialSlotName)])
                return Reject(Result, TEXT("Mesh material role binding differs."));
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(TextureMaps); ++Index)
        {
            const FMap& Map = TextureMaps[Index];
            auto* Texture = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("/Textures/") + Map.Name));
            const FExpressionInput* Input = Bound[MapRole(Index)]->GetExpressionInputForProperty(Map.Property);
            const auto* Sample = Input ? Cast<UMaterialExpressionTextureSample>(Input->Expression) : nullptr;
            const int32 OutputIndex = Map.Property == MP_BaseColor || Map.Property == MP_Normal ? 0 : 1;
            if (!Texture || Texture->Source.GetSizeX() != 2048 || Texture->Source.GetSizeY() != 2048
                || Texture->SRGB != Map.Srgb || Texture->CompressionSettings != Map.Compression || Texture->bFlipGreenChannel
                || !Sample || Sample->Texture != Texture || Sample->SamplerType != Map.Sampler || Input->OutputIndex != OutputIndex)
                return Reject(Result, FString(TEXT("Texture/graph differs: ")) + Map.Name);
            Assets.Add(Texture);
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("object"), Texture->GetPathName());
            Record->SetStringField(TEXT("source"), Map.File);
            Record->SetNumberField(TEXT("width"), Texture->Source.GetSizeX());
            Record->SetNumberField(TEXT("height"), Texture->Source.GetSizeY());
            Record->SetNumberField(TEXT("sourceFormat"), Texture->Source.GetFormat());
            Record->SetNumberField(TEXT("compression"), Texture->CompressionSettings);
            Record->SetNumberField(TEXT("materialRole"), MapRole(Index));
            Record->SetNumberField(TEXT("connectedOutputIndex"), Input->OutputIndex);
            Record->SetBoolField(TEXT("srgb"), Texture->SRGB);
            Record->SetBoolField(TEXT("flipGreen"), Texture->bFlipGreenChannel);
            TextureRecords.Add(MakeShared<FJsonValueObject>(Record));
        }
        Result->SetArrayField(TEXT("materials"), MaterialRecords);
        Result->SetArrayField(TEXT("textures"), TextureRecords);
        Result->SetNumberField(TEXT("packages"), Assets.Num());
        return Assets.Num() == 17 && AuditAssetReferences(Root, Assets, {}, Result);
    }

    bool Import(FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
    {
        for (const FString& Package : Packages())
            if (FindPackage(nullptr, *Package) || FPackageName::DoesPackageExist(Package))
                return Reject(Result, TEXT("Reserved package exists: ") + Package);
        Result->SetStringField(TEXT("stage"), TEXT("thirteen-textures-three-materials"));
        TArray<UTexture2D*> Textures;
        for (const FMap& Map : TextureMaps)
        {
            auto* Texture = ImportMappedTexture(Map, FPaths::Combine(Source, TEXT("Textures")), Root, Feedback);
            if (!Texture) return Reject(Result, FString(TEXT("Texture import failed: ")) + Map.File);
            Textures.Add(Texture);
        }
        TArray<UMaterial*> Bound;
        for (int32 RoleIndex = 0; RoleIndex < 3; ++RoleIndex)
        {
            if (Feedback.ReceivedUserCancel()) return false;
            auto* Material = NewObject<UMaterial>(CreatePackage(*(Root + TEXT("/Materials/") + Materials[RoleIndex])),
                Materials[RoleIndex], RF_Public | RF_Standalone);
            Material->BlendMode = RoleIndex == 1 ? BLEND_Masked : BLEND_Opaque;
            Material->TwoSided = RoleIndex == 1;
            Material->OpacityMaskClipValue = 0.5f;
            Material->SetShadingModel(MSM_DefaultLit);
            for (int32 Index = 0; Index < Textures.Num(); ++Index)
            {
                if (MapRole(Index) != RoleIndex) continue;
                const FMap& Map = TextureMaps[Index];
                auto* Sample = Cast<UMaterialExpressionTextureSample>(UMaterialEditingLibrary::CreateMaterialExpression(
                    Material, UMaterialExpressionTextureSample::StaticClass(), -400, Index * 200));
                if (!Sample) return Reject(Result, TEXT("Cannot create tree texture sample."));
                Sample->Texture = Textures[Index];
                Sample->SamplerType = Map.Sampler;
                const bool Color = Map.Property == MP_BaseColor || Map.Property == MP_Normal;
                if (!UMaterialEditingLibrary::ConnectMaterialProperty(Sample, Color ? TEXT("") : TEXT("R"), Map.Property))
                    return Reject(Result, TEXT("Cannot connect tree material property."));
            }
            Material->PostEditChange();
            Bound.Add(Material);
        }
        Result->SetStringField(TEXT("stage"), TEXT("one-lod2-fbx"));
        if (Feedback.ReceivedUserCancel()) return false;
        auto* Factory = StaticMeshFactory(true);
        bool Cancelled = false;
        auto* Mesh = Cast<UStaticMesh>(Factory->ImportObject(UStaticMesh::StaticClass(),
            CreatePackage(*(Root + TEXT("/Meshes/") + MeshName)), MeshName, RF_Public | RF_Standalone,
            FPaths::Combine(Source, TEXT("TreeSmall02_LOD2.fbx")), nullptr, Cancelled));
        if (!Mesh || Cancelled || Factory->GetAdditionalImportedObjects().Num())
            return Reject(Result, TEXT("FBX factory did not return exactly one static mesh."));
        FStaticMeshCompilingManager::Get().FinishCompilation({Mesh});
        Result->SetArrayField(TEXT("importedBoundsMinBeforeAdaptationCm"), Vector(Mesh->GetBoundingBox().Min));
        Result->SetArrayField(TEXT("importedBoundsMaxBeforeAdaptationCm"), Vector(Mesh->GetBoundingBox().Max));
        Result->SetStringField(TEXT("stage"), TEXT("branch-source-uv-routing"));
        if (!RouteBranchUvs(*Mesh, Result)) return false;
        FKSphylElem Collision;
        if (!Measure(Mesh, Collision, Result)) return false;
        for (int32 Index = 0; Index < 3; ++Index)
            Mesh->SetMaterial(Index, Bound[Role(Mesh->GetStaticMaterials()[Index].ImportedMaterialSlotName)]);
        Mesh->CreateBodySetup();
        auto* Body = Mesh->GetBodySetup();
        if (!Body || Body->AggGeom.GetElementCount()) return Reject(Result, TEXT("Unexpected factory collision."));
        Body->CollisionTraceFlag = CTF_UseSimpleAsComplex;
        Body->AggGeom.SphylElems.Add(Collision);
        Body->InvalidatePhysicsData();
        Body->CreatePhysicsMeshes();
        Mesh->PostEditChange();
        Result->SetStringField(TEXT("stage"), TEXT("inventory-and-reference-audit"));
        TArray<UObject*> Assets;
        if (!Inventory(Assets, Result) || Feedback.ReceivedUserCancel()) return false;
        Result->SetStringField(TEXT("stage"), TEXT("seventeen-explicit-package-saves"));
        for (UObject* Asset : Assets)
        {
            if (Feedback.ReceivedUserCancel()) return false;
            auto* Package = Asset->GetOutermost();
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            if (IFileManager::Get().FileExists(*File)) return Reject(Result, TEXT("Output already exists: ") + File);
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone;
            Args.Error = GWarn;
            if (!UPackage::SavePackage(Package, Asset, *File, Args) || Feedback.ReceivedUserCancel()) return false;
        }
        Result->SetStringField(TEXT("stage"), TEXT("import-complete"));
        return true;
    }
}

namespace Grass
{
    const FString Root = TEXT("/Game/Trials/GrassGround_20260921_01");
    const TCHAR* Names[] = { TEXT("mid_b"), TEXT("small_b"), TEXT("tall_a"), TEXT("tiny_a") };
    const int32 Triangles[] = { 1257, 653, 290, 79 };
    const FVector Sizes[] = {
        FVector(18.778882, 20.102860, 17.772157), FVector(16.595670, 18.989212, 9.989528),
        FVector(16.095641, 15.853148, 32.254639), FVector(6.734776, 7.376876, 11.245334)
    };
    const int64 Models[] = { 9907750, 854938463, 515132540, 99195606 };
    const int64 Geometries[] = { 34972056, 556848324, 425513347, 697680708 };
    const FMap TextureMaps[] = {
        { TEXT("grass_medium_01_diff_1k.png"), TEXT("T_GrassMedium01_Diff"), MP_BaseColor, TC_Default, SAMPLERTYPE_Color, true },
        { TEXT("grass_medium_01_nor_dx_1k.png"), TEXT("T_GrassMedium01_NormalDX"), MP_Normal, TC_Normalmap, SAMPLERTYPE_Normal, false },
        { TEXT("grass_medium_01_rough_1k.png"), TEXT("T_GrassMedium01_Roughness"), MP_Roughness, TC_Masks, SAMPLERTYPE_Masks, false },
        { TEXT("grass_medium_01_ao_1k.png"), TEXT("T_GrassMedium01_AO"), MP_AmbientOcclusion, TC_Masks, SAMPLERTYPE_Masks, false },
        { TEXT("grass_medium_01_alpha_1k.png"), TEXT("T_GrassMedium01_Alpha"), MP_OpacityMask, TC_Masks, SAMPLERTYPE_Masks, false },
        { TEXT("grass_ground_diff_2k.png"), TEXT("T_GrassGround_Diff"), MP_BaseColor, TC_Default, SAMPLERTYPE_Color, true },
        { TEXT("grass_ground_nor_dx_2k.png"), TEXT("T_GrassGround_NormalDX"), MP_Normal, TC_Normalmap, SAMPLERTYPE_Normal, false },
        { TEXT("grass_ground_rough_2k.png"), TEXT("T_GrassGround_Roughness"), MP_Roughness, TC_Masks, SAMPLERTYPE_Masks, false }
    };
    const TCHAR* ExistingMaps[] = {
        TEXT("/Game/SurvivalGame/Textures/T_GroundColor"),
        TEXT("/Game/SurvivalGame/Textures/T_GroundNormal"),
        TEXT("/Game/SurvivalGame/Textures/T_GroundRoughness")
    };

    bool Reject(const TSharedRef<FJsonObject>& Result, const FString& Reason)
    {
        Result->SetStringField(TEXT("failure"), Reason);
        UE_LOG(LogFernSpike, Error, TEXT("Grass/ground: %s"), *Reason);
        return false;
    }

    FString MeshName(int32 Index) { return FString(TEXT("SM_GrassMedium01_")) + Names[Index]; }

    TArray<FString> Packages()
    {
        TArray<FString> Value;
        for (int32 Index = 0; Index < 4; ++Index) Value.Add(Root + TEXT("/Meshes/") + MeshName(Index));
        for (const FMap& Map : TextureMaps) Value.Add(Root + TEXT("/Textures/") + Map.Name);
        Value.Add(Root + TEXT("/Materials/M_GrassMedium01"));
        Value.Add(Root + TEXT("/Materials/M_GrassGroundBlend"));
        return Value;
    }

    template <typename T> T* Expression(UMaterial* Material)
    {
        return Cast<T>(UMaterialEditingLibrary::CreateMaterialExpression(Material, T::StaticClass()));
    }

    bool SampleMatches(const FExpressionInput* Input, UTexture2D* Texture, const FMap& Map)
    {
        const auto* Sample = Input ? Cast<UMaterialExpressionTextureSample>(Input->Expression) : nullptr;
        return Sample && Sample->Texture == Texture && Sample->SamplerType == Map.Sampler
            && Input->OutputIndex == (Map.Property == MP_BaseColor || Map.Property == MP_Normal ? 0 : 1)
            && !Sample->Coordinates.Expression && Sample->ConstCoordinate == 0;
    }

    bool Inventory(TArray<UObject*>& Assets, const TSharedRef<FJsonObject>& Result)
    {
        auto* Material = LoadObject<UMaterial>(nullptr, *(Root + TEXT("/Materials/M_GrassMedium01")));
        auto* Ground = LoadObject<UMaterial>(nullptr, *(Root + TEXT("/Materials/M_GrassGroundBlend")));
        if (!Material || Material->BlendMode != BLEND_Masked || !Material->TwoSided
            || Material->OpacityMaskClipValue != 0.333f || Material->GetExpressions().Num() != 5
            || !Material->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit)
            || !Material->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes)
            || !Ground || Ground->BlendMode != BLEND_Opaque || Ground->TwoSided
            || !Ground->GetShadingModels().HasOnlyShadingModel(MSM_DefaultLit) || Ground->GetExpressions().Num() != 11)
            return Reject(Result, TEXT("Authored grass/ground material properties or instancing usage differ."));
        Assets.Append({ Material, Ground });
        TArray<TSharedPtr<FJsonValue>> TextureRecords, MeshRecords;
        TArray<UTexture2D*> Textures;
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(TextureMaps); ++Index)
        {
            const FMap& Map = TextureMaps[Index];
            auto* Texture = LoadObject<UTexture2D>(nullptr, *(Root + TEXT("/Textures/") + Map.Name));
            const int32 Pixels = Index < 5 ? 1024 : 2048;
            if (!Texture || Texture->Source.GetSizeX() != Pixels || Texture->Source.GetSizeY() != Pixels
                || Texture->SRGB != Map.Srgb || Texture->CompressionSettings != Map.Compression || Texture->bFlipGreenChannel)
                return Reject(Result, FString(TEXT("Texture identity/format differs: ")) + Map.Name);
            if (Index < 5 && !SampleMatches(Material->GetExpressionInputForProperty(Map.Property), Texture, Map))
                return Reject(Result, FString(TEXT("Grass material graph differs: ")) + Map.Name);
            Assets.Add(Texture);
            Textures.Add(Texture);
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("object"), Texture->GetPathName());
            Record->SetStringField(TEXT("source"), Map.File);
            Record->SetNumberField(TEXT("width"), Pixels);
            Record->SetNumberField(TEXT("height"), Pixels);
            Record->SetNumberField(TEXT("compression"), Texture->CompressionSettings);
            Record->SetBoolField(TEXT("srgb"), Texture->SRGB);
            Record->SetBoolField(TEXT("flipGreen"), Texture->bFlipGreenChannel);
            TextureRecords.Add(MakeShared<FJsonValueObject>(Record));
        }
        TSet<FString> External;
        const UMaterialExpressionVertexColor* SharedWeight = nullptr;
        for (int32 Index = 0; Index < 3; ++Index)
        {
            const FMap& Map = TextureMaps[Index + 5];
            auto* Existing = LoadObject<UTexture2D>(nullptr, ExistingMaps[Index]);
            if (!Existing || Existing->SRGB != (Index == 0)
                || Existing->CompressionSettings != (Index == 1 ? TC_Normalmap : TC_Default))
                return Reject(Result, TEXT("Missing/different admitted read-only old-ground texture."));
            FMap ExistingMap = Map;
            ExistingMap.Sampler = Index == 2 ? SAMPLERTYPE_LinearColor : Map.Sampler;
            External.Add(ExistingMaps[Index]);
            const FExpressionInput* Input = Ground->GetExpressionInputForProperty(Map.Property);
            if (Index == 1)
            {
                const auto* Normalized = Input ? Cast<UMaterialExpressionNormalize>(Input->Expression) : nullptr;
                if (!Normalized || Input->OutputIndex != 0) return Reject(Result, TEXT("Ground normal must be normalized after blending."));
                Input = &Normalized->VectorInput;
            }
            const auto* Blend = Input ? Cast<UMaterialExpressionLinearInterpolate>(Input->Expression) : nullptr;
            const auto* Weight = Blend ? Cast<UMaterialExpressionVertexColor>(Blend->Alpha.Expression) : nullptr;
            if (!Blend || Input->OutputIndex != 0 || !Weight || Blend->Alpha.OutputIndex != 1
                || (SharedWeight && SharedWeight != Weight)
                || !SampleMatches(&Blend->A, Existing, ExistingMap) || !SampleMatches(&Blend->B, Textures[Index + 5], Map))
                return Reject(Result, TEXT("Ground graph must share vertex-red weight across exact old/new maps."));
            SharedWeight = Weight;
        }
        for (int32 Index = 0; Index < 4; ++Index)
        {
            auto* Mesh = LoadObject<UStaticMesh>(nullptr, *(Root + TEXT("/Meshes/") + MeshName(Index)));
            if (!Mesh) return Reject(Result, TEXT("Missing selected clump: ") + MeshName(Index));
            FStaticMeshCompilingManager::Get().FinishCompilation({ Mesh });
            const auto* Data = Cast<UFbxStaticMeshImportData>(Mesh->AssetImportData);
            const auto* Description = Mesh->GetMeshDescription(0);
            const auto* Render = Mesh->GetRenderData();
            if (Mesh->GetNumSourceModels() != 1 || !Description || Description->Triangles().Num() != Triangles[Index]
                || !Render || Render->LODResources.Num() != 1 || Render->LODResources[0].GetNumTriangles() != Triangles[Index]
                || Mesh->GetStaticMaterials().Num() != 1 || Mesh->GetStaticMaterials()[0].ImportedMaterialSlotName != TEXT("grass_medium_01")
                || Mesh->GetMaterial(0) != Material || Mesh->GetNaniteSettings().bEnabled
                || !Data || Data->ImportUniformScale != 1 || !Data->ImportTranslation.IsZero() || !Data->ImportRotation.IsZero()
                || !Data->bConvertScene || !Data->bConvertSceneUnit || !Data->bTransformVertexToAbsolute
                || Data->bBakePivotInVertex || Data->bAutoGenerateCollision
                || (Mesh->GetBodySetup() && Mesh->GetBodySetup()->AggGeom.GetElementCount()))
                return Reject(Result, TEXT("Clump topology/slot/material/unit/import/collision policy differs: ") + MeshName(Index));
            const FBox Bounds = Mesh->GetBoundingBox();
            const FVector Size = Bounds.GetSize();
            if (!Bounds.IsValid || Bounds.Min.ContainsNaN() || Bounds.Max.ContainsNaN())
                return Reject(Result, TEXT("Nonfinite clump bounds."));
            TArray<double> Actual = { Size.X, Size.Y, Size.Z };
            TArray<double> Expected = { Sizes[Index].X, Sizes[Index].Y, Sizes[Index].Z };
            Actual.Sort(); Expected.Sort();
            for (int32 Axis = 0; Axis < 3; ++Axis)
                if (!FMath::IsNearlyEqual(Actual[Axis], Expected[Axis], 0.1))
                    return Reject(Result, TEXT("Source units/baked clump dimensions differ: ") + MeshName(Index));
            const FStaticMeshConstAttributes Attributes(*Description);
            const auto Normals = Attributes.GetVertexInstanceNormals();
            const auto UVs = Attributes.GetVertexInstanceUVs();
            if (UVs.GetNumChannels() != 1) return Reject(Result, TEXT("Expected one retained source UV channel."));
            for (FVertexInstanceID Corner : Description->VertexInstances().GetElementIDs())
                if (Normals[Corner].ContainsNaN() || Normals[Corner].SizeSquared() < 0.5f || UVs.Get(Corner, 0).ContainsNaN())
                    return Reject(Result, TEXT("Invalid referenced clump normal/UV."));
            Assets.Add(Mesh);
            auto Record = MakeShared<FJsonObject>();
            Record->SetStringField(TEXT("object"), Mesh->GetPathName());
            Record->SetStringField(TEXT("sourceNode"), FString(TEXT("grass_medium_01_")) + Names[Index] + TEXT("_LOD0"));
            Record->SetNumberField(TEXT("sourceModelId"), Models[Index]);
            Record->SetNumberField(TEXT("sourceGeometryId"), Geometries[Index]);
            Record->SetNumberField(TEXT("sourceTriangles"), Description->Triangles().Num());
            Record->SetNumberField(TEXT("renderTriangles"), Render->LODResources[0].GetNumTriangles());
            Record->SetStringField(TEXT("importedSlot"), Mesh->GetStaticMaterials()[0].ImportedMaterialSlotName.ToString());
            Record->SetStringField(TEXT("material"), Material->GetPathName());
            Record->SetArrayField(TEXT("boundsMinCm"), Vector(Bounds.Min));
            Record->SetArrayField(TEXT("boundsMaxCm"), Vector(Bounds.Max));
            Record->SetArrayField(TEXT("sourceDimensionsTimes100Cm"), Vector(Sizes[Index]));
            Record->SetArrayField(TEXT("placementGroundAnchorCm"), Vector(FVector(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z)));
            Record->SetNumberField(TEXT("importUniformScale"), Data->ImportUniformScale);
            Record->SetBoolField(TEXT("nanite"), Mesh->GetNaniteSettings().bEnabled);
            Record->SetBoolField(TEXT("simpleCollision"), false);
            MeshRecords.Add(MakeShared<FJsonValueObject>(Record));
        }
        Result->SetArrayField(TEXT("meshes"), MeshRecords);
        Result->SetArrayField(TEXT("textures"), TextureRecords);
        Result->SetNumberField(TEXT("totalMeshTriangles"), 2279);
        Result->SetNumberField(TEXT("packages"), Assets.Num());
        Result->SetStringField(TEXT("grassMaterial"), Material->GetPathName());
        Result->SetStringField(TEXT("groundMaterial"), Ground->GetPathName());
        Result->SetStringField(TEXT("groundBlend"), TEXT("Vertex red lerps original ground to grass-ground color/roughness/normal; normalize final normal; unchanged UV0 three-meter tiling."));
        Result->SetStringField(TEXT("transformPolicy"), TEXT("Scene/unit conversion once and absolute source transform bake; authored scale1; runtime placement offsets measured ground anchor only."));
        return Assets.Num() == 14 && AuditAssetReferences(Root, Assets, External, Result);
    }

    bool Import(FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
    {
        for (const FString& Package : Packages())
            if (FindPackage(nullptr, *Package) || FPackageName::DoesPackageExist(Package))
                return Reject(Result, TEXT("Fresh grass/ground package required: ") + Package);
        Result->SetStringField(TEXT("stage"), TEXT("eight-explicit-textures"));
        TArray<UTexture2D*> Textures;
        const FString Source = FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Source/woodland-preparation-20260920-182217-d1f84e39"));
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(TextureMaps); ++Index)
        {
            auto* Texture = ImportMappedTexture(TextureMaps[Index],
                FPaths::Combine(Source, Index < 5 ? TEXT("grass_medium_01") : TEXT("grass_ground")), Root, Feedback);
            if (!Texture) return Reject(Result, FString(TEXT("Explicit texture import failed: ")) + TextureMaps[Index].File);
            Textures.Add(Texture);
        }
        Result->SetStringField(TEXT("stage"), TEXT("two-described-materials"));
        auto* Material = NewObject<UMaterial>(CreatePackage(*(Root + TEXT("/Materials/M_GrassMedium01"))),
            TEXT("M_GrassMedium01"), RF_Public | RF_Standalone);
        Material->BlendMode = BLEND_Masked;
        Material->TwoSided = true;
        Material->OpacityMaskClipValue = 0.333f;
        Material->SetShadingModel(MSM_DefaultLit);
        for (int32 Index = 0; Index < 5; ++Index)
        {
            auto* Sample = Expression<UMaterialExpressionTextureSample>(Material);
            if (!Sample) return Reject(Result, TEXT("Cannot allocate grass texture sample."));
            Sample->Texture = Textures[Index];
            Sample->SamplerType = TextureMaps[Index].Sampler;
            Material->GetExpressionInputForProperty(TextureMaps[Index].Property)->Connect(Index < 2 ? 0 : 1, Sample);
        }
        if (!Material->SetMaterialUsage(MATUSAGE_InstancedStaticMeshes)) return Reject(Result, TEXT("Grass instancing usage failed."));
        Material->PostEditChange();
        auto* Ground = NewObject<UMaterial>(CreatePackage(*(Root + TEXT("/Materials/M_GrassGroundBlend"))),
            TEXT("M_GrassGroundBlend"), RF_Public | RF_Standalone);
        Ground->BlendMode = BLEND_Opaque;
        Ground->TwoSided = false;
        Ground->SetShadingModel(MSM_DefaultLit);
        auto* Weight = Expression<UMaterialExpressionVertexColor>(Ground);
        if (!Weight) return Reject(Result, TEXT("Cannot allocate ground blend weight."));
        for (int32 Index = 0; Index < 3; ++Index)
        {
            auto* Existing = LoadObject<UTexture2D>(nullptr, ExistingMaps[Index]);
            auto* Old = Expression<UMaterialExpressionTextureSample>(Ground);
            auto* Fresh = Expression<UMaterialExpressionTextureSample>(Ground);
            auto* Blend = Expression<UMaterialExpressionLinearInterpolate>(Ground);
            if (!Existing || !Old || !Fresh || !Blend) return Reject(Result, TEXT("Cannot build admitted ground blend."));
            const FMap& Map = TextureMaps[Index + 5];
            Old->Texture = Existing;
            Old->SamplerType = Index == 2 ? SAMPLERTYPE_LinearColor : Map.Sampler;
            Fresh->Texture = Textures[Index + 5]; Fresh->SamplerType = Map.Sampler;
            Blend->A.Connect(Index == 2 ? 1 : 0, Old);
            Blend->B.Connect(Index == 2 ? 1 : 0, Fresh);
            Blend->Alpha.Connect(1, Weight);
            if (Index == 1)
            {
                auto* Normalized = Expression<UMaterialExpressionNormalize>(Ground);
                if (!Normalized) return Reject(Result, TEXT("Cannot normalize ground normal."));
                Normalized->VectorInput.Connect(0, Blend);
                Ground->GetExpressionInputForProperty(Map.Property)->Connect(0, Normalized);
            }
            else Ground->GetExpressionInputForProperty(Map.Property)->Connect(0, Blend);
        }
        Ground->PostEditChange();
        Result->SetStringField(TEXT("stage"), TEXT("four-selected-clumps"));
        if (Feedback.ReceivedUserCancel()) return false;
        auto* Factory = StaticMeshFactory(false);
        bool Cancelled = false;
        UObject* Imported = Factory->ImportObject(UStaticMesh::StaticClass(),
            CreatePackage(*(Root + TEXT("/Unsaved/SelectedGrass"))), TEXT("SelectedGrass"), RF_Public | RF_Standalone,
            FPaths::Combine(FPaths::ProjectDir(), TEXT("Assets/Environment/GrassMedium01Prepared/v1/GrassMedium01_Selected.fbx")),
            nullptr, Cancelled);
        if (!Imported || Cancelled) return Reject(Result, TEXT("Selected FBX factory failed."));
        TArray<UObject*> Objects = Factory->GetAdditionalImportedObjects();
        Objects.AddUnique(Imported);
        if (Objects.Num() != 4) return Reject(Result, TEXT("Expected exactly four imported clump objects."));
        for (int32 Index = 0; Index < 4; ++Index)
        {
            UStaticMesh* Match = nullptr;
            const FString Node = FString(TEXT("grass_medium_01_")) + Names[Index] + TEXT("_LOD0");
            for (UObject* Object : Objects)
                if (Object->GetName().EndsWith(Node))
                {
                    if (Match) return Reject(Result, TEXT("Ambiguous selected FBX clump."));
                    Match = Cast<UStaticMesh>(Object);
                    if (!Match) return Reject(Result, TEXT("Non-static selected FBX object."));
                }
            if (!Match || !Match->Rename(*MeshName(Index), CreatePackage(*(Root + TEXT("/Meshes/") + MeshName(Index))),
                REN_DontCreateRedirectors | REN_NonTransactional))
                return Reject(Result, TEXT("Cannot identify/name selected clump: ") + Node);
            Match->SetMaterial(0, Material);
            Match->PostEditChange();
        }
        Result->SetStringField(TEXT("stage"), TEXT("inventory-and-reference-audit"));
        TArray<UObject*> Assets;
        if (!Inventory(Assets, Result) || Feedback.ReceivedUserCancel()) return false;
        Result->SetStringField(TEXT("stage"), TEXT("fourteen-explicit-package-saves"));
        for (UObject* Asset : Assets)
        {
            if (Feedback.ReceivedUserCancel()) return false;
            auto* Package = Asset->GetOutermost();
            const FString File = FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension());
            if (IFileManager::Get().FileExists(*File)) return Reject(Result, TEXT("Package output already exists: ") + File);
            FSavePackageArgs Args;
            Args.TopLevelFlags = RF_Public | RF_Standalone;
            Args.Error = GWarn;
            if (!UPackage::SavePackage(Package, Asset, *File, Args) || Feedback.ReceivedUserCancel())
                return Reject(Result, TEXT("Grass/ground package save failed/cancelled: ") + File);
        }
        Result->SetStringField(TEXT("stage"), TEXT("import-complete"));
        return true;
    }
}

bool Import(const FString& Output, FStopFeedback& Feedback, const TSharedRef<FJsonObject>& Result)
{
    Result->SetStringField(TEXT("stage"), TEXT("explicit-texture-import"));
    TArray<UObject*> Assets;
    TArray<UTexture2D*> Textures;
    for (const FMap& Map : Maps)
    {
        auto* Texture = ImportMappedTexture(Map, FernSourceRoot, Trial, Feedback);
        if (!Texture) return false;
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
    auto* Factory = StaticMeshFactory(false);
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

bool RunHairWaveSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven)
{
    FStopFeedback Feedback(Output, Deadline, bCompletionDriven);
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("mode"), Mode);
    Result->SetStringField(TEXT("namespace"), HairTrial);
    Result->SetStringField(TEXT("stage"), TEXT("persisted-inventory"));
    bool Passed = false;
    if (Mode == TEXT("HairImport")) Passed = Hair::ImportWaves(Feedback, Result);
    else if (Mode == TEXT("HairVerify"))
    {
        TArray<USkeletalMesh*> Meshes;
        TArray<USkinnedAsset*> Pending;
        for (const FWaveMesh& Wave : WaveMeshes)
        {
            auto* Mesh = LoadObject<USkeletalMesh>(nullptr, *(HairTrial + TEXT("/Meshes/") + Wave.Name));
            Meshes.Add(Mesh);
            if (Mesh) Pending.Add(Mesh);
        }
        FSkinnedAssetCompilingManager::Get().FinishCompilation(Pending);
        Passed = Hair::WaveInventory(Meshes,
            LoadObject<UTexture2D>(nullptr, *(HairTrial + TEXT("/Textures/T_LongWave_Neutral"))),
            LoadObject<UMaterial>(nullptr, *(HairTrial + TEXT("/Materials/M_Heroine_Hair_long01_Neutral"))), Result);
    }
    Result->SetBoolField(TEXT("passed"), Passed);
    Result->SetBoolField(TEXT("cancelledAtPollingBoundary"), Feedback.ReceivedUserCancel());
    if (!Passed) UE_LOG(LogFernSpike, Error, TEXT("Frozen wave %s failed; preserve its evidence and partial outputs."), *Mode);
    return WriteJson(FPaths::Combine(Output, TEXT("hair-wave-result.json")), Result) && Passed && !Feedback.ReceivedUserCancel();
}

bool RunWardrobeSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven)
{
    FStopFeedback Feedback(Output, Deadline, bCompletionDriven);
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("mode"), Mode);
    Result->SetStringField(TEXT("namespace"), Wardrobe::Root);
    Result->SetStringField(TEXT("stage"), TEXT("persisted-inventory"));
    bool Passed = false;
    if (Mode == TEXT("WardrobeImport")) Passed = Wardrobe::Import(Feedback, Result);
    else if (Mode == TEXT("WardrobeVerify"))
    {
        TArray<UObject*> Assets;
        TSet<FString> External;
        Passed = Wardrobe::Inventory(Result, Assets, External) && Wardrobe::Audit(Assets, External, Result);
    }
    Result->SetBoolField(TEXT("passed"), Passed);
    Result->SetBoolField(TEXT("cancelledAtPollingBoundary"), Feedback.ReceivedUserCancel());
    if (!Passed) UE_LOG(LogFernSpike, Error, TEXT("Wardrobe %s failed at %s; preserve evidence and partial outputs."),
        *Mode, *Result->GetStringField(TEXT("stage")));
    return WriteJson(FPaths::Combine(Output, TEXT("wardrobe-result.json")), Result) && Passed && !Feedback.ReceivedUserCancel();
}

bool RunTreeSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven)
{
    FStopFeedback Feedback(Output, Deadline, bCompletionDriven);
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("mode"), Mode);
    Result->SetStringField(TEXT("namespace"), Tree::Root);
    Result->SetStringField(TEXT("stage"), TEXT("persisted-inventory"));
    bool Passed = false;
    if (Mode == TEXT("TreeImport")) Passed = Tree::Import(Feedback, Result);
    else if (Mode == TEXT("TreeVerify"))
    {
        TArray<UObject*> Assets;
        Passed = Tree::Inventory(Assets, Result);
    }
    Result->SetBoolField(TEXT("passed"), Passed);
    Result->SetBoolField(TEXT("cancelledAtPollingBoundary"), Feedback.ReceivedUserCancel());
    if (!Passed) UE_LOG(LogFernSpike, Error, TEXT("Tree %s failed at %s; retain evidence and partial outputs."),
        *Mode, *Result->GetStringField(TEXT("stage")));
    return WriteJson(FPaths::Combine(Output, TEXT("tree-result.json")), Result) && Passed && !Feedback.ReceivedUserCancel();
}

bool RunGrassSpike(const FString& Mode, const FString& Output, const FDateTime& Deadline, bool bCompletionDriven)
{
    FStopFeedback Feedback(Output, Deadline, bCompletionDriven);
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("mode"), Mode);
    Result->SetStringField(TEXT("namespace"), Grass::Root);
    Result->SetStringField(TEXT("stage"), TEXT("persisted-inventory"));
    bool Passed = false;
    if (Mode == TEXT("GrassImport")) Passed = Grass::Import(Feedback, Result);
    else if (Mode == TEXT("GrassVerify"))
    {
        TArray<UObject*> Assets;
        Passed = Grass::Inventory(Assets, Result);
    }
    Result->SetBoolField(TEXT("passed"), Passed);
    Result->SetBoolField(TEXT("cancelledAtPollingBoundary"), Feedback.ReceivedUserCancel());
    if (!Passed) UE_LOG(LogFernSpike, Error, TEXT("Grass %s failed at %s; retain partial outputs."),
        *Mode, *Result->GetStringField(TEXT("stage")));
    return WriteJson(FPaths::Combine(Output, TEXT("grass-result.json")), Result) && Passed && !Feedback.ReceivedUserCancel();
}
