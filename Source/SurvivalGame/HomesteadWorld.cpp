#include "HomesteadWorld.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadWorld, Log, All);

namespace
{
// Original provisional shapes, not the final realistic environment asset set.
const FLinearColor Meadow(0.22f, 0.31f, 0.095f);
const FLinearColor Leaf(0.12f, 0.26f, 0.065f);
const FLinearColor LightLeaf(0.27f, 0.37f, 0.095f);
const FLinearColor Bark(0.19f, 0.105f, 0.052f);
const FLinearColor Wood(0.37f, 0.23f, 0.115f);
const FLinearColor Stone(0.32f, 0.36f, 0.34f);
const FLinearColor Soil(0.16f, 0.085f, 0.039f);
const FLinearColor Cloth(0.55f, 0.43f, 0.25f);
const FLinearColor PreviewColor(0.65f, 0.79f, 0.77f);

FName MaterialKey(const FLinearColor& Color, float Roughness, float Glow)
{
    return FName(*FString::Printf(TEXT("Tint_%d_%d_%d_R%d_G%d"),
        FMath::RoundToInt(Color.R * 1000), FMath::RoundToInt(Color.G * 1000),
        FMath::RoundToInt(Color.B * 1000), FMath::RoundToInt(Roughness * 100),
        FMath::RoundToInt(Glow * 100)));
}

FVector AtGround(float X, float Y, float Offset = 0.0f)
{
    return FVector(X, Y, AHomesteadWorld::GroundHeight(X, Y) + Offset);
}

int Stage(double Value, int Steps)
{
    return FMath::Clamp(FMath::FloorToInt(Value * Steps), 0, Steps);
}

template<typename T>
void RemoveMissing(TMap<int32, FHomesteadWorldVisual>& Visuals, const T& Entries)
{
    TSet<int32> Existing;
    for (const auto& Entry : Entries)
    {
        Existing.Add(Entry.id);
    }
    for (auto It = Visuals.CreateIterator(); It; ++It)
    {
        if (!Existing.Contains(It.Key()))
        {
            for (USceneComponent* Component : It.Value().Components)
            {
                if (IsValid(Component))
                {
                    Component->DestroyComponent();
                }
            }
            It.RemoveCurrent();
        }
    }
}
}

AHomesteadWorld::AHomesteadWorld()
{
    PrimaryActorTick.bCanEverTick = false;
    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WorldRoot"));
    SceneRoot->SetMobility(EComponentMobility::Static);
    SetRootComponent(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeAsset(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereAsset(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderAsset(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeAsset(TEXT("/Engine/BasicShapes/Cone.Cone"));
    Cube = CubeAsset.Object;
    Sphere = SphereAsset.Object;
    Cylinder = CylinderAsset.Object;
    Cone = ConeAsset.Object;
}

float AHomesteadWorld::GroundHeight(float X, float Y)
{
    const float DistanceHome = FVector2D(X + 1000.0f, Y).Size();
    const float HomeBlend = FMath::SmoothStep(350.0f, 1050.0f, DistanceHome);
    const float Land = HomeBlend * (24.0f * FMath::Sin(X / 1550.0f)
        + 18.0f * FMath::Sin(Y / 1250.0f) + 9.0f * FMath::Sin((X + Y) / 700.0f));
    const float StreamDistance = FMath::Abs(X - static_cast<float>(Homestead::StreamX(Y)));
    const float Channel = 1.0f - FMath::SmoothStep(45.0f, 175.0f, StreamDistance);
    const float Bank = FMath::Exp(-FMath::Square((StreamDistance - 190.0f) / 85.0f));
    return Land - 38.0f * Channel + 7.0f * Bank;
}

float AHomesteadWorld::CellBase(int CellX, int CellY)
{
    const Homestead::Point Center = Homestead::CellCenter(CellX, CellY);
    float Height = GroundHeight(Center.x, Center.y);
    for (int X : {-1, 1})
    {
        for (int Y : {-1, 1})
        {
            Height = FMath::Max(Height, GroundHeight(Center.x + X * 150.0, Center.y + Y * 150.0));
        }
    }
    return Height + 16.0f;
}

UMaterialInterface* AHomesteadWorld::Material(FLinearColor Color, float Roughness, float Glow)
{
    const FName Key = MaterialKey(Color, Roughness, Glow);
    if (TObjectPtr<UMaterialInstanceDynamic>* Existing = Materials.Find(Key))
    {
        return Existing->Get();
    }
    const bool bTexturedRock = RockMaterial && Color.Equals(Stone);
    UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(
        bTexturedRock ? RockMaterial.Get() : FieldMaterial.Get(), this);
    if (!Instance)
    {
        return UMaterial::GetDefaultMaterial(MD_Surface);
    }
    Instance->SetVectorParameterValue(TEXT("Tint"), bTexturedRock ? FLinearColor::White : Color);
    Instance->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
    Materials.Add(Key, Instance);
    return Instance;
}

UStaticMeshComponent* AHomesteadWorld::AddPart(FHomesteadWorldVisual& Visual, UStaticMesh* Mesh,
    const FVector& Position, const FVector& Size, FLinearColor Color, bool bCollision,
    const FRotator& Rotation, float Roughness, float Glow)
{
    if (!Mesh)
    {
        return nullptr;
    }
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(GetRootComponent());
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetStaticMesh(Mesh);
    Part->SetMaterial(0, Material(Color, Roughness, Glow));
    Part->SetRelativeTransform(FTransform(Rotation, Position, Size / 100.0f));
    Part->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(bCollision);
    Part->SetCastShadow(Glow <= 0.0f);
    Part->RegisterComponent();
    Visual.Components.Add(Part);
    return Part;
}

void AHomesteadWorld::AddDecoration(UStaticMesh* Mesh, const FVector& Position, const FVector& Size,
    FLinearColor Color, bool bCollision, const FRotator& Rotation, bool bHideMesh)
{
    if (!Mesh)
    {
        return;
    }
    const FName Key(*FString::Printf(TEXT("%s_%s_%d_%d"), *Mesh->GetName(),
        *MaterialKey(Color, 0.85f, 0.0f).ToString(), bCollision, bHideMesh));
    UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
    if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Existing = DecorationBatches.Find(Key))
    {
        Batch = Existing->Get();
    }
    else
    {
        Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        Batch->SetupAttachment(GetRootComponent());
        Batch->SetMobility(EComponentMobility::Static);
        Batch->SetStaticMesh(Mesh);
        for (int Slot = 0; Slot < FMath::Max(1, Mesh->GetStaticMaterials().Num()); ++Slot)
        {
            Batch->SetMaterial(Slot, Material(Color));
        }
        Batch->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
        Batch->SetGenerateOverlapEvents(false);
        Batch->SetCanEverAffectNavigation(bCollision);
        Batch->SetCullDistances(bCollision ? 0 : 4500, bCollision ? 0 : 7000);
        Batch->bAutoRebuildTreeOnInstanceChanges = false;
        Batch->SetVisibility(!bHideMesh);
        Batch->SetCastShadow(!bHideMesh);
        Batch->RegisterComponent();
        DecorationBatches.Add(Key, Batch);
    }
    // Imported FBX assets need not share the engine primitives' centered 100 cm bounds.
    const FBox Bounds = Mesh->GetBoundingBox();
    const FVector Dimensions = Bounds.GetSize().ComponentMax(FVector(0.01f));
    const FVector Scale = Size / Dimensions;
    const FVector Origin = Position - Rotation.RotateVector(Bounds.GetCenter() * Scale);
    Batch->AddInstance(FTransform(Rotation, Origin, Scale));
}

float AHomesteadWorld::GrassGroundWeight(float X, float Y)
{
    const float Home = FMath::SmoothStep(650.0, 1000.0, FVector2D(X + 1000, Y).Size());
    const float Bank = FMath::SmoothStep(195.0, 350.0, FMath::Abs(X - Homestead::StreamX(Y)));
    const float Patch = 0.5f + 0.5f * FMath::Sin(X * 0.0021f) * FMath::Cos(Y * 0.0017f);
    return Home * Bank * FMath::Lerp(0.45f, 0.88f, Patch);
}

void AHomesteadWorld::BuildTerrain()
{
    Ground = NewObject<UProceduralMeshComponent>(this, TEXT("OriginalMeadowTerrain"));
    Ground->SetupAttachment(GetRootComponent());
    Ground->SetMobility(EComponentMobility::Static);
    Ground->bUseAsyncCooking = false;
    Ground->bUseComplexAsSimpleCollision = true;
    Ground->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Ground->SetGenerateOverlapEvents(false);
    Ground->RegisterComponent();

    constexpr int Cells = 320;
    constexpr float Spacing = 25.0f;
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UV;
    TArray<FLinearColor> Colors;
    TArray<FProcMeshTangent> Tangents;
    Vertices.Reserve((Cells + 1) * (Cells + 1));
    for (int Y = 0; Y <= Cells; ++Y)
    {
        for (int X = 0; X <= Cells; ++X)
        {
            const float PX = -4000.0f + X * Spacing;
            const float PY = -4000.0f + Y * Spacing;
            Vertices.Add(AtGround(PX, PY));
            const float DX = (GroundHeight(PX + 1, PY) - GroundHeight(PX - 1, PY)) * 0.5f;
            const float DY = (GroundHeight(PX, PY + 1) - GroundHeight(PX, PY - 1)) * 0.5f;
            Normals.Add(FVector(-DX, -DY, 1.0f).GetSafeNormal());
            UV.Add(FVector2D(PX / 300.0f, PY / 300.0f));
            Colors.Add(FLinearColor(GrassGroundWeight(PX, PY), 0, 0, 1));
            Tangents.Add(FProcMeshTangent(FVector(1, 0, DX).GetSafeNormal(), false));
            if (X < Cells && Y < Cells)
            {
                const int A = Y * (Cells + 1) + X;
                Triangles.Append({A, A + Cells + 1, A + 1, A + 1, A + Cells + 1, A + Cells + 2});
            }
        }
    }
    Ground->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV, Colors, Tangents, true);
    Ground->SetMaterial(0, GroundMaterial ? GroundMaterial.Get() : Material(Meadow));

    // Separate non-colliding ribbons expose the shallow channel and its muddy banks.
    auto Ribbon = [&](int Section, float Left, float Right, bool bWater)
    {
        Vertices.Reset();
        Triangles.Reset();
        Normals.Reset();
        UV.Reset();
        Colors.Reset();
        Tangents.Reset();
        const int Columns = bWater ? 1 : 8;
        for (int Y = 0; Y <= Cells; ++Y)
        {
            const float PY = -4000.0f + Y * Spacing;
            const float Center = Homestead::StreamX(PY);
            for (int X = 0; X <= Columns; ++X)
            {
                const float Offset = FMath::Lerp(Left, Right, static_cast<float>(X) / Columns);
                const float Z = bWater ? GroundHeight(Center, PY) + 19.0f : GroundHeight(Center + Offset, PY) + 2.0f;
                Vertices.Add(FVector(Center + Offset, PY, Z));
                Normals.Add(FVector::UpVector);
                UV.Add(FVector2D(Offset / 100.0f, PY / 300.0f));
                Tangents.Add(FProcMeshTangent(1, 0, 0));
                if (Y < Cells && X < Columns)
                {
                    const int A = Y * (Columns + 1) + X;
                    Triangles.Append({A, A + Columns + 1, A + 1,
                        A + 1, A + Columns + 1, A + Columns + 2});
                }
            }
        }
        Ground->CreateMeshSection_LinearColor(Section, Vertices, Triangles, Normals, UV, Colors, Tangents, false);
        Ground->SetMaterial(Section, bWater ? Material(FLinearColor(0.075f, 0.26f, 0.29f), 0.16f)
            : Material(FLinearColor(0.27f, 0.235f, 0.14f)));
    };
    Ribbon(1, -125.0f, 125.0f, true);
    Ribbon(2, -180.0f, -105.0f, false);
    Ribbon(3, 105.0f, 180.0f, false);
}

void AHomesteadWorld::BuildLighting()
{
    Exposure = NewObject<UPostProcessComponent>(this, TEXT("MeadowExposure"));
    Exposure->SetupAttachment(GetRootComponent());
    Exposure->bUnbound = true;
    Exposure->BlendWeight = 1.0f;
    FPostProcessSettings& Settings = Exposure->Settings;
    Settings.bOverride_AutoExposureMethod = true;
    Settings.AutoExposureMethod = AEM_Histogram;
    // Extended luminance range makes these EV100: accommodate the physical sun,
    // but cap dark adaptation so moonlit ground is not exposed like daylight.
    Settings.bOverride_AutoExposureMinBrightness = true;
    Settings.AutoExposureMinBrightness = 0.0f;
    Settings.bOverride_AutoExposureMaxBrightness = true;
    Settings.AutoExposureMaxBrightness = 16.0f;
    Settings.bOverride_AutoExposureBias = true;
    Settings.AutoExposureBias = 0.5f;
    Settings.bOverride_AutoExposureSpeedUp = true;
    Settings.AutoExposureSpeedUp = 3.0f;
    Settings.bOverride_AutoExposureSpeedDown = true;
    Settings.AutoExposureSpeedDown = 1.0f;
    Exposure->RegisterComponent();

    Sun = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowSun"));
    Sun->SetupAttachment(GetRootComponent());
    Sun->SetMobility(EComponentMobility::Movable);
    Sun->bAtmosphereSunLight = true;
    Sun->SetIntensity(65000.0f);
    Sun->RegisterComponent();

    Moon = NewObject<UDirectionalLightComponent>(this, TEXT("MeadowMoonlight"));
    Moon->SetupAttachment(GetRootComponent());
    Moon->SetMobility(EComponentMobility::Movable);
    Moon->bAtmosphereSunLight = true;
    Moon->AtmosphereSunLightIndex = 1;
    Moon->SetLightColor(FLinearColor(0.53f, 0.66f, 1.0f));
    Moon->SetIntensity(0.5f);
    Moon->RegisterComponent();

    Sky = NewObject<USkyLightComponent>(this, TEXT("MeadowSkyLight"));
    Sky->SetupAttachment(GetRootComponent());
    Sky->SetMobility(EComponentMobility::Movable);
    Sky->bRealTimeCapture = true;
    Sky->SetIntensity(0.85f);
    Sky->RegisterComponent();

    USkyAtmosphereComponent* Atmosphere = NewObject<USkyAtmosphereComponent>(this, TEXT("MeadowAtmosphere"));
    Atmosphere->SetupAttachment(GetRootComponent());
    Atmosphere->RegisterComponent();

    Fog = NewObject<UExponentialHeightFogComponent>(this, TEXT("MeadowDistanceHaze"));
    Fog->SetupAttachment(GetRootComponent());
    Fog->SetMobility(EComponentMobility::Movable);
    Fog->SetFogDensity(0.007f);
    Fog->SetFogHeightFalloff(0.3f);
    Fog->SetStartDistance(1100.0f);
    Fog->RegisterComponent();
}

void AHomesteadWorld::BuildDecorations(const Homestead::State& State)
{
    const FName FernTag(TEXT("AuthoredFern02"));
    const FName TreeTag(TEXT("AuthoredTreeSmall02"));
    const FName GroveTag(TEXT("AuthoredTreeSmall02Grove"));
    TArray<UStaticMeshComponent*> PreviousParts;
    GetComponents(PreviousParts);
    for (UStaticMeshComponent* Part : PreviousParts)
    {
        if (Part->ComponentHasTag(FernTag) || Part->ComponentHasTag(TreeTag) || Part->ComponentHasTag(GroveTag))
        {
            RemoveInstanceComponent(Part);
            Part->DestroyComponent();
        }
    }
    for (auto& Entry : DecorationBatches)
    {
        Entry.Value->ClearInstances();
    }
    FRandomStream Random(817391);
    auto Reserved = [&](float X, float Y, float Radius)
    {
        if (FVector2D(X + 1000, Y).Size() < 650.0f + Radius)
        {
            return true;
        }
        for (const auto& Node : State.resources)
        {
            if (FVector2D(X - Node.position.x, Y - Node.position.y).Size() < Radius + 130.0f)
            {
                return true;
            }
        }
        for (const auto& Structure : State.structures)
        {
            const Homestead::Point Center = Homestead::CellCenter(Structure.cellX, Structure.cellY);
            if (FVector2D(X - Center.x, Y - Center.y).Size() < Radius + 225.0f)
            {
                return true;
            }
        }
        for (const auto& Plot : State.plots)
        {
            const Homestead::Point Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
            if (FVector2D(X - Center.x, Y - Center.y).Size() < Radius + 175.0f)
            {
                return true;
            }
        }
        return false;
    };

    TArray<UStaticMesh*> FernMeshes;
    for (const TCHAR* Suffix : {TEXT("a"), TEXT("b"), TEXT("c"), TEXT("d")})
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Trials/Fern02_20260920_01/Meshes/SM_Fern02_%s.SM_Fern02_%s"), Suffix, Suffix);
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh || Mesh->GetStaticMaterials().Num() != 1 || !Mesh->GetStaticMaterials()[0].MaterialInterface)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Authored clearing fern is unavailable or has no material: %s"), *Path);
            FernMeshes.Reset();
            break;
        }
        FernMeshes.Add(Mesh);
    }
    int32 FernCount = 0;
    if (FernMeshes.Num() == 4)
    {
        for (int32 Index = 0; Index < 32; ++Index)
        {
            FRandomStream FernRandom(63017 + Index * 211);
            const float Angle = Index * 2.39996323f;
            const float Radius = FernRandom.FRandRange(820.0f, 1250.0f);
            const float X = -1000.0f + FMath::Cos(Angle) * Radius;
            const float Y = FMath::Sin(Angle) * Radius;
            if (Reserved(X, Y, 75.0f) || FMath::Abs(X - Homestead::StreamX(Y)) < 270.0f)
            {
                continue;
            }
            UStaticMesh* Mesh = FernMeshes[Index % FernMeshes.Num()];
            const FBox Bounds = Mesh->GetBoundingBox();
            const FVector GroundAnchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
            const FRotator Rotation(0, FernRandom.FRandRange(0, 360), 0);
            UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
            AddInstanceComponent(Part);
            Part->ComponentTags.Add(FernTag);
            Part->SetupAttachment(GetRootComponent());
            Part->SetMobility(EComponentMobility::Static);
            Part->SetStaticMesh(Mesh);
            // Keep authored vertices/slots/scale; undo the baked layout offset at placement only.
            Part->SetRelativeTransform(FTransform(Rotation,
                AtGround(X, Y) - Rotation.RotateVector(GroundAnchor), FVector::OneVector));
            Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Part->SetGenerateOverlapEvents(false);
            Part->SetCanEverAffectNavigation(false);
            Part->SetCullDistance(5000.0f);
            Part->RegisterComponent();
            ++FernCount;
        }
    }
    UE_LOG(LogHomesteadWorld, Display, TEXT("Authored clearing fern patch: %d noncolliding plants; native materials and scale."), FernCount);

    auto* AuthoredTree = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/Trials/TreeSmall02_20260921_01/Meshes/SM_TreeSmall02_LOD2.SM_TreeSmall02_LOD2"));
    int32 TreeIndex = INDEX_NONE;
    TSet<int32> GroveIndices;
    if (AuthoredTree && AuthoredTree->GetStaticMaterials().Num() == 3 && AuthoredTree->GetBodySetup()
        && AuthoredTree->GetBodySetup()->AggGeom.SphylElems.Num() == 1
        && AuthoredTree->GetBodySetup()->AggGeom.GetElementCount() == 1
        && AuthoredTree->GetRenderData() && AuthoredTree->GetRenderData()->LODResources.Num() > 0
        && AuthoredTree->GetRenderData()->LODResources[0].GetNumTriangles() == 231785)
    {
        const FBox Bounds = AuthoredTree->GetBoundingBox();
        const float Radius = FVector2D(FMath::Max(FMath::Abs(Bounds.Min.X), FMath::Abs(Bounds.Max.X)),
            FMath::Max(FMath::Abs(Bounds.Min.Y), FMath::Abs(Bounds.Max.Y))).Size();
        float Nearest = TNumericLimits<float>::Max();
        TArray<TPair<int32, FVector2D>> Sites;
        for (int32 Index = 0; Index < 430; ++Index)
        {
            FRandomStream Candidate(817391 + Index * 179);
            const float X = Candidate.FRandRange(-3850, 3850);
            const float Y = Candidate.FRandRange(-3850, 3850);
            const float Edge = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
            const float Cluster = FMath::Sin(X / 480) * FMath::Cos(Y / 620);
            if (Reserved(X, Y, 90) || FMath::Abs(X - Homestead::StreamX(Y)) < 280
                || (Edge < 2650 && (Cluster < 0.45f || Candidate.FRand() < 0.68f)))
                continue;
            if (Reserved(X, Y, Radius) || FMath::Abs(X - Homestead::StreamX(Y)) < Radius + 120
                || FVector2D(X + 1000, Y).Size() > 3000)
                continue;
            Sites.Emplace(Index, FVector2D(X, Y));
            const float Distance = FVector2D(X - 250, Y + 650).SizeSquared();
            if (FVector2D(X + 1000, Y).Size() <= 1900 && Distance < Nearest)
            {
                TreeIndex = Index;
                Nearest = Distance;
            }
        }
        TArray<FVector2D> SelectedSites;
        for (const auto& Site : Sites)
            if (Site.Key == TreeIndex)
            {
                GroveIndices.Add(Site.Key);
                SelectedSites.Add(Site.Value);
            }
        Sites.Sort([](const auto& A, const auto& B)
        {
            const double ADistance = (A.Value + FVector2D(1000, 0)).SizeSquared();
            const double BDistance = (B.Value + FVector2D(1000, 0)).SizeSquared();
            return ADistance == BDistance ? A.Key < B.Key : ADistance < BDistance;
        });
        for (const auto& Site : Sites)
        {
            if (GroveIndices.Num() >= 16) break;
            if (SelectedSites.ContainsByPredicate([&](const FVector2D& Other)
                { return FVector2D::Distance(Other, Site.Value) < 2 * Radius; }))
                continue;
            GroveIndices.Add(Site.Key);
            SelectedSites.Add(Site.Value);
        }
    }
    else
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Authored reduced tree or its measured simple trunk collision is unavailable."));
    }
    if (AuthoredTree && TreeIndex == INDEX_NONE)
        UE_LOG(LogHomesteadWorld, Warning, TEXT("No safe seeded clearing tree site in this world state; keeping existing primitive decorations."));
    for (int Index = 0; Index < 430; ++Index)
    {
        Random.Initialize(817391 + Index * 179);
        const float X = Random.FRandRange(-3850, 3850);
        const float Y = Random.FRandRange(-3850, 3850);
        const float Edge = FMath::Max(FMath::Abs(X), FMath::Abs(Y));
        const float Cluster = FMath::Sin(X / 480) * FMath::Cos(Y / 620);
        if (Reserved(X, Y, 90) || FMath::Abs(X - Homestead::StreamX(Y)) < 280
            || (Edge < 2650 && (Cluster < 0.45f || Random.FRand() < 0.68f)))
        {
            continue;
        }
        const float Height = Random.FRandRange(440, 800);
        const float Width = Random.FRandRange(34, 58);
        const FVector Base = AtGround(X, Y);
        const float Yaw = Random.FRandRange(0, 360);
        if (GroveIndices.Contains(Index))
        {
            auto* Part = NewObject<UStaticMeshComponent>(this);
            AddInstanceComponent(Part);
            Part->ComponentTags.Add(GroveTag);
            if (Index == TreeIndex) Part->ComponentTags.Add(TreeTag);
            Part->SetupAttachment(GetRootComponent());
            Part->SetMobility(EComponentMobility::Static);
            Part->SetStaticMesh(AuthoredTree);
            Part->SetRelativeTransform(FTransform(FRotator(0, Yaw, 0), Base, FVector::OneVector));
            Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Part->SetGenerateOverlapEvents(false);
            Part->SetCullDistance(5000.0f);
            Part->RegisterComponent();
            UE_LOG(LogHomesteadWorld, Display,
                TEXT("Authored reduced tree: seedIndex=%d root=%s yaw=%.6f scale=1; native three-role materials/simple trunk capsule."),
                Index, *Base.ToString(), Yaw);
            continue;
        }
        // The upgraded clearing is a sparse real grove, not real trees hidden among taller proxies.
        if (GroveIndices.Num() > 0)
            continue;
        AddDecoration(Cylinder, Base + FVector(0, 0, Height * 0.47f),
            FVector(Width, Width, Height * 0.94f), Bark, true);
        if (Index % 3 == 0)
        {
            for (int Tier = 0; Tier < 3; ++Tier)
            {
                const float Span = (340.0f - Tier * 65.0f) * Height / 620.0f;
                AddDecoration(Cone, Base + FVector(0, 0, Height * (0.52f + Tier * 0.2f)),
                    FVector(Span, Span, Height * 0.6f), Tier == 1 ? Leaf : LightLeaf, false, FRotator(0, Yaw, 0));
            }
        }
        else
        {
            for (int Crown = 0; Crown < 4; ++Crown)
            {
                const float Angle = Crown * 2.399f + Yaw;
                AddDecoration(Sphere, Base + FVector(FMath::Cos(Angle) * 90, FMath::Sin(Angle) * 90,
                    Height * 0.77f + Crown * 32),
                    FVector(340, 290, 290) * Height / 620.0f,
                    Crown % 2 ? Leaf : LightLeaf, false, FRotator(0, Yaw, 0));
            }
        }
    }

    for (int Index = 0; Index < 140; ++Index)
    {
        Random.Initialize(37231 + Index * 193);
        const float X = Random.FRandRange(-3800, 3800);
        const float Y = Random.FRandRange(-3800, 3800);
        const float Width = Random.FRandRange(50, 135);
        if (Reserved(X, Y, Width * 0.5f) || FMath::Abs(X - Homestead::StreamX(Y)) < 210)
        {
            continue;
        }
        const FVector Position = AtGround(X, Y, Width * 0.22f);
        const FVector Size(Width, Width * 0.82f, Width * 0.75f);
        const FRotator RockRotation(Random.FRandRange(-15, 15), Random.FRandRange(0, 360), 12);
        if (ImportedRock)
        {
            AddDecoration(ImportedRock, Position, Size, Stone, false, RockRotation);
            // A simple invisible collider remains reliable even when FBX has no collision data.
            AddDecoration(Sphere, Position, Size, Stone, true, RockRotation, true);
        }
        else
        {
            AddDecoration(Sphere, Position, Size, Stone, true, RockRotation);
        }
    }

    for (int Index = 0; Index < 220; ++Index)
    {
        Random.Initialize(92731 + Index * 191);
        const float Y = Random.FRandRange(-3920, 3920);
        const float X = Homestead::StreamX(Y) + (Index % 2 ? -1 : 1) * Random.FRandRange(95, 230);
        const float Width = Random.FRandRange(14, 47);
        if (!Reserved(X, Y, 35))
        {
            AddDecoration(ImportedRock ? ImportedRock.Get() : Sphere.Get(), AtGround(X, Y, Width * 0.17f),
                FVector(Width, Width * 0.8f, Width * 0.48f),
                Stone, false, FRotator(0, Random.FRandRange(0, 360), 0));
        }
    }

    TArray<UStaticMesh*> GrassMeshes;
    const TCHAR* GrassNames[] = { TEXT("mid_b"), TEXT("small_b"), TEXT("tall_a"), TEXT("tiny_a") };
    const int32 GrassTriangles[] = { 1257, 653, 290, 79 };
    for (int32 Index = 0; Index < 4; ++Index)
    {
        const FString Path = FString::Printf(
            TEXT("/Game/Trials/GrassGround_20260921_01/Meshes/SM_GrassMedium01_%s.SM_GrassMedium01_%s"),
            GrassNames[Index], GrassNames[Index]);
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!Mesh || Mesh->GetStaticMaterials().Num() != 1 || !Mesh->GetMaterial(0)
            || Mesh->GetMaterial(0)->GetPathName() != TEXT("/Game/Trials/GrassGround_20260921_01/Materials/M_GrassMedium01.M_GrassMedium01")
            || !Mesh->GetRenderData() || Mesh->GetRenderData()->LODResources.Num() != 1
            || Mesh->GetRenderData()->LODResources[0].GetNumTriangles() != GrassTriangles[Index])
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Admitted authored grass is missing or differs: %s"), *Path);
            GrassMeshes.Reset();
            break;
        }
        GrassMeshes.Add(Mesh);
    }
    int32 GrassCount = 0;
    int32 GrassTriangleCount = 0;
    if (GrassMeshes.Num() == 4)
    {
        for (int32 Attempt = 0; Attempt < 2048 && GrassCount < 512; ++Attempt)
        {
            FRandomStream GrassRandom(71039 + Attempt * 233);
            const float Radius = FMath::Sqrt((Attempt + 0.5f) / 2048.0f) * 2600.0f;
            const float Angle = Attempt * 2.39996323f;
            const float X = -1000 + FMath::Cos(Angle) * Radius;
            const float Y = FMath::Sin(Angle) * Radius;
            if (Reserved(X, Y, 20) || GrassGroundWeight(X, Y) < 0.4f)
            {
                continue;
            }
            const int32 Index = Attempt % 4;
            UStaticMesh* Mesh = GrassMeshes[Index];
            const FName Key(*FString::Printf(TEXT("AuthoredGrass_%s"), GrassNames[Index]));
            UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
            if (auto* Existing = DecorationBatches.Find(Key))
            {
                Batch = Existing->Get();
            }
            else
            {
                Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
                Batch->SetupAttachment(GetRootComponent());
                Batch->SetMobility(EComponentMobility::Static);
                Batch->ComponentTags.Add(TEXT("AuthoredGrassMedium01"));
                Batch->SetStaticMesh(Mesh);
                Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
                Batch->SetGenerateOverlapEvents(false);
                Batch->SetCanEverAffectNavigation(false);
                Batch->SetCullDistances(2500, 3500);
                Batch->bAutoRebuildTreeOnInstanceChanges = false;
                Batch->RegisterComponent();
                DecorationBatches.Add(Key, Batch);
            }
            const FBox Bounds = Mesh->GetBoundingBox();
            const FVector Anchor(Bounds.GetCenter().X, Bounds.GetCenter().Y, Bounds.Min.Z);
            const FRotator Rotation(0, GrassRandom.FRandRange(0, 360), 0);
            Batch->AddInstance(FTransform(Rotation, AtGround(X, Y) - Rotation.RotateVector(Anchor), FVector::OneVector));
            ++GrassCount;
            GrassTriangleCount += GrassTriangles[Index];
        }
    }
    UE_LOG(LogHomesteadWorld, Display, TEXT("Authored clearing grass: %d nonblocking clumps, %d triangles, native scale/material."),
        GrassCount, GrassTriangleCount);

    // Retain outer meadow and flower-stem proxies outside the replaced clearing grass.
    for (int Patch = 0; Patch < 1150; ++Patch)
    {
        Random.Initialize(19271 + Patch * 199);
        const float X = Random.FRandRange(-3900, 3900);
        const float Y = Random.FRandRange(-3900, 3900);
        if (Reserved(X, Y, 0) || FMath::Abs(X - Homestead::StreamX(Y)) < 195)
        {
            continue;
        }
        const bool bFlowers = Patch % 7 == 0;
        if (!bFlowers && GrassCount > 0 && FVector2D(X + 1000, Y).Size() < 2600.0f)
        {
            continue;
        }
        const FLinearColor FlowerColor = Patch % 3 == 0 ? FLinearColor(0.7f, 0.56f, 0.1f)
            : Patch % 3 == 1 ? FLinearColor(0.48f, 0.23f, 0.54f) : FLinearColor(0.83f, 0.79f, 0.59f);
        for (int Blade = 0; Blade < 3; ++Blade)
        {
            const float PX = X + Random.FRandRange(-25, 25);
            const float PY = Y + Random.FRandRange(-25, 25);
            const float Height = Random.FRandRange(18, 43);
            AddDecoration(Cone, AtGround(PX, PY, Height * 0.5f), FVector(9, 7, Height),
                Patch % 2 ? Leaf : LightLeaf, false, FRotator(0, Random.FRandRange(0, 360), 8));
            if (bFlowers)
            {
                AddDecoration(Sphere, AtGround(PX, PY, Height), FVector(10, 10, 5), FlowerColor);
            }
        }
    }

    // Invisible edge rails prevent falling off the finite collision mesh.
    for (int Side = 0; Side < 4; ++Side)
    {
        const bool bAlongX = Side < 2;
        const float Sign = Side % 2 ? 1.0f : -1.0f;
        AddDecoration(Cube, FVector(bAlongX ? 0 : Sign * 4050, bAlongX ? Sign * 4050 : 0, 500),
            FVector(bAlongX ? 8200 : 100, bAlongX ? 100 : 8200, 1200),
            FLinearColor(0, 0, 0), true, FRotator::ZeroRotator, true);
    }
    for (auto& Entry : DecorationBatches)
    {
        Entry.Value->BuildTreeIfOutdated(false, true);
    }
}

void AHomesteadWorld::ClearVisual(FHomesteadWorldVisual& Visual)
{
    for (USceneComponent* Component : Visual.Components)
    {
        if (IsValid(Component))
        {
            Component->DestroyComponent();
        }
    }
    Visual.Components.Reset();
    Visual.Signature.Reset();
}

void AHomesteadWorld::BuildResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly)
{
    if (Node.cleared)
    {
        return;
    }
    const FVector Base = AtGround(Node.position.x, Node.position.y);
    FRandomStream Random(Node.id * 127 + 19);
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        FRotator Rotation = FRotator::ZeroRotator, bool bProduce = false)
    {
        if (bProduce == bProduceOnly)
        {
            AddPart(Visual, Mesh, Base + Offset, Size, Color, false, Rotation);
        }
    };

    switch (Node.kind)
    {
    case Homestead::ResourceKind::Branches:
        if (bProduceOnly)
        {
            for (int I = 0; I < 3; ++I)
            {
                Part(Cylinder, FVector(I * 9 - 9, 0, 6 + I * 3), FVector(7, 7, 65),
                    Bark, FRotator(82, I * 35 + 20, 0), true);
            }
        }
        break;
    case Homestead::ResourceKind::Stones:
        if (bProduceOnly)
        {
            for (int I = 0; I < 3; ++I)
            {
                Part(Sphere, FVector(I * 17 - 17, I % 2 * 14, 10), FVector(28, 24, 20), Stone,
                    FRotator::ZeroRotator, true);
            }
        }
        break;
    case Homestead::ResourceKind::BerryBush:
        Part(Cylinder, FVector(0, 0, 24), FVector(9, 9, 48), Bark);
        for (int I = 0; I < 3; ++I)
        {
            Part(Sphere, FVector((I - 1) * 26, I % 2 * 18, 46 + I % 2 * 10),
                FVector(60, 52, 52), I % 2 ? Leaf : LightLeaf);
        }
        if (bProduceOnly)
        {
            for (int I = 0; I < 8; ++I)
            {
                const float Angle = I * 2.399f;
                Part(Sphere, FVector(FMath::Cos(Angle) * 42, FMath::Sin(Angle) * 33, 54 + I % 3 * 9),
                    FVector(10), FLinearColor(0.42f, 0.025f, 0.055f), FRotator::ZeroRotator, true);
            }
        }
        break;
    case Homestead::ResourceKind::Roots:
        Part(Sphere, FVector(0, 0, 1), FVector(66, 56, 5), Soil);
        for (int I = 0; I < 5; ++I)
        {
            const float Angle = I * 72.0f;
            Part(Sphere, FVector(FMath::Cos(FMath::DegreesToRadians(Angle)) * 14,
                FMath::Sin(FMath::DegreesToRadians(Angle)) * 14, 7),
                FVector(28, 12, 5), Leaf, FRotator(20, Angle, 0));
        }
        if (bProduceOnly)
        {
            Part(Sphere, FVector(0, 0, 5), FVector(18, 18, 12), FLinearColor(0.65f, 0.43f, 0.19f),
                FRotator::ZeroRotator, true);
        }
        break;
    case Homestead::ResourceKind::Flowers:
        for (int I = 0; I < 5; ++I)
        {
            const float X = Random.FRandRange(-23, 23);
            const float Y = Random.FRandRange(-23, 23);
            const float Height = Random.FRandRange(25, 45);
            Part(Cone, FVector(X, Y, Height * 0.4f), FVector(16, 12, Height * 0.8f), Leaf);
            if (bProduceOnly)
            {
                Part(Cylinder, FVector(X, Y, Height * 0.5f), FVector(2, 2, Height), Leaf,
                    FRotator::ZeroRotator, true);
                Part(Sphere, FVector(X, Y, Height), FVector(19, 19, 6),
                    Node.id % 2 ? FLinearColor(0.65f, 0.32f, 0.61f) : FLinearColor(0.88f, 0.72f, 0.2f),
                    FRotator::ZeroRotator, true);
                Part(Sphere, FVector(X, Y, Height + 3), FVector(6, 6, 4), FLinearColor(0.48f, 0.25f, 0.02f),
                    FRotator::ZeroRotator, true);
            }
        }
        break;
    case Homestead::ResourceKind::Reeds:
        for (int I = 0; I < 7; ++I)
        {
            const float X = Random.FRandRange(-24, 24);
            const float Y = Random.FRandRange(-24, 24);
            const float Height = Random.FRandRange(65, 110);
            Part(Cone, FVector(X, Y, 12), FVector(8, 8, 24), Leaf);
            if (bProduceOnly)
            {
                Part(Cylinder, FVector(X, Y, Height * 0.5f), FVector(3, 3, Height), LightLeaf,
                    FRotator(0, I * 49, 7), true);
                Part(Sphere, FVector(X + 5, Y, Height), FVector(7, 7, 21), Bark,
                    FRotator::ZeroRotator, true);
            }
        }
        break;
    case Homestead::ResourceKind::Sapling:
        Part(Cylinder, FVector(0, 0, 18), FVector(12, 12, 36), Bark);
        if (bProduceOnly)
        {
            Part(Cylinder, FVector(0, 0, 87), FVector(12, 12, 104), Bark, FRotator::ZeroRotator, true);
            Part(Cone, FVector(0, 0, 108), FVector(105, 105, 150), Leaf, FRotator::ZeroRotator, true);
            Part(Cone, FVector(0, 0, 158), FVector(72, 72, 115), LightLeaf, FRotator::ZeroRotator, true);
        }
        break;
    default:
        break;
    }
}

void AHomesteadWorld::BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure, bool bPreview)
{
    const Homestead::Point Center = Homestead::CellCenter(Structure.cellX, Structure.cellY);
    const FVector Base(Center.x, Center.y, CellBase(Structure.cellX, Structure.cellY));
    // UE positive yaw rotates +X toward +Y; negative yaw maps the north edge to east.
    const FRotator Rotation(0, -90.0f * (Structure.rotation % 4), 0);
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        bool bSolid = false, FRotator LocalRotation = FRotator::ZeroRotator, float Glow = 0.0f)
    {
        const FRotator Combined = (Rotation.Quaternion() * LocalRotation.Quaternion()).Rotator();
        return AddPart(Visual, Mesh, Base + Rotation.RotateVector(Offset), Size,
            bPreview ? PreviewColor : Color, bSolid && !bPreview, Combined, 0.85f, bPreview ? 0.0f : Glow);
    };
    switch (Structure.kind)
    {
    case Homestead::Piece::Foundation:
        Part(Cube, FVector(0, 0, -10), FVector(298, 298, 20), Wood, true);
        for (int I = 0; I < 6; ++I)
        {
            Part(Cube, FVector(-125 + I * 50, 0, 0.8f), FVector(2, 292, 1), Bark);
        }
        break;
    case Homestead::Piece::Wall:
        Part(Cube, FVector(0, 144, 130), FVector(300, 12, 260), Wood, true);
        for (int I = 0; I < 4; ++I)
        {
            Part(Cube, FVector(-142 + I * 94.67f, 142, 130), FVector(12, 20, 260), Bark);
        }
        Part(Cube, FVector(0, 142, 252), FVector(300, 22, 16), Bark);
        break;
    case Homestead::Piece::Doorway:
        Part(Cube, FVector(-107.5f, 144, 130), FVector(85, 12, 260), Wood, true);
        Part(Cube, FVector(107.5f, 144, 130), FVector(85, 12, 260), Wood, true);
        Part(Cube, FVector(0, 144, 247.5f), FVector(130, 16, 25), Bark, true);
        // Tied-back cloth suggests a simple shelter entrance without a hidden collider.
        Part(Cube, FVector(-61, 133, 123), FVector(8, 8, 214), Cloth);
        Part(Cube, FVector(61, 133, 123), FVector(8, 8, 214), Cloth);
        Part(Cube, FVector(0, 137, 235), FVector(124, 5, 8), Cloth);
        break;
    case Homestead::Piece::Roof:
        Part(Cube, FVector(0, 0, 273), FVector(310, 310, 22), FLinearColor(0.27f, 0.25f, 0.105f), true);
        for (int I = -1; I <= 1; ++I)
        {
            Part(Cube, FVector(I * 110, 0, 256), FVector(12, 300, 12), Bark);
        }
        break;
    case Homestead::Piece::Fire:
    {
        const FVector Hearth(-100, 95, 0);
        for (int I = 0; I < 8; ++I)
        {
            const float Angle = I * PI / 4;
            Part(Sphere, Hearth + FVector(FMath::Cos(Angle) * 28, FMath::Sin(Angle) * 28, 7),
                FVector(19, 16, 14), Stone);
        }
        Part(Cylinder, Hearth + FVector(0, 0, 8), FVector(10, 10, 42), Bark, false, FRotator(85, 30, 0));
        Part(Cylinder, Hearth + FVector(0, 0, 10), FVector(10, 10, 42), Bark, false, FRotator(85, -30, 0));
        if (Structure.fuelHours > 0 && !bPreview)
        {
            Part(Cone, Hearth + FVector(0, 0, 28), FVector(24, 24, 45),
                FLinearColor(0.95f, 0.2f, 0.015f), false, FRotator::ZeroRotator, 3.0f);
            Part(Cone, Hearth + FVector(0, 0, 24), FVector(13, 13, 28),
                FLinearColor(1.0f, 0.63f, 0.08f), false, FRotator::ZeroRotator, 4.0f);
            UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
            Light->SetupAttachment(GetRootComponent());
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetRelativeLocation(Base + Rotation.RotateVector(Hearth + FVector(0, 0, 65)));
            Light->SetLightColor(FLinearColor(1.0f, 0.49f, 0.19f));
            Light->SetIntensity(1800.0f);
            Light->SetAttenuationRadius(550.0f);
            Light->SetSourceRadius(18.0f);
            Light->SetCastShadows(false);
            Light->RegisterComponent();
            Visual.Components.Add(Light);
        }
        break;
    }
    case Homestead::Piece::Bed:
        Part(Cube, FVector(95, -10, 6), FVector(70, 155, 12), Cloth, true);
        Part(Cube, FVector(95, 46, 14), FVector(62, 32, 12), FLinearColor(0.7f, 0.65f, 0.46f));
        Part(Cube, FVector(95, -28, 14), FVector(68, 106, 8), FLinearColor(0.23f, 0.31f, 0.21f));
        break;
    case Homestead::Piece::Chest:
        Part(Cube, FVector(-100, -100, 27), FVector(70, 55, 54), Wood, true);
        Part(Cube, FVector(-100, -100, 55), FVector(74, 59, 6), Bark);
        Part(Cube, FVector(-100, -71, 35), FVector(12, 4, 12), Stone);
        break;
    default:
        break;
    }
    if (bPreview)
    {
        for (USceneComponent* Component : Visual.Components)
        {
            if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
            {
                Mesh->SetCastShadow(false);
            }
        }
    }
}

void AHomesteadWorld::BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot)
{
    const Homestead::Point Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
    const float Moisture = Stage(Plot.moisture, 5) / 5.0f;
    const FLinearColor WetSoil = FMath::Lerp(Soil, FLinearColor(0.075f, 0.044f, 0.025f), Moisture);
    // Small separate soil tiles follow the terrain rather than floating above a slope.
    for (int X = -1; X <= 1; ++X)
    {
        for (int Y = -1; Y <= 1; ++Y)
        {
            const float PX = Center.x + X * 76;
            const float PY = Center.y + Y * 76;
            AddPart(Visual, Cube, AtGround(PX, PY, 1.5f), FVector(74, 74, 3), WetSoil,
                false, FRotator::ZeroRotator, 0.9f - Moisture * 0.35f);
            if (Plot.planted)
            {
                const float Growth = Stage(Plot.growth, 12) / 12.0f;
                const bool BerryCrop = Plot.kind == Homestead::CropKind::Berries;
                const float Height = 7 + Growth * (BerryCrop ? 65 : 48);
                AddPart(Visual, Cone, AtGround(PX, PY, Height * 0.5f + 3),
                    FVector(12 + Growth * 23, 12 + Growth * 23, Height), Leaf);
                AddPart(Visual, Sphere, AtGround(PX, PY, Height * 0.55f + 3),
                    FVector(20 + Growth * 35, 12 + Growth * 20, 7 + Growth * 8), LightLeaf,
                    false, FRotator(0, (X + Y) * 52, 0));
                if (Plot.growth >= 1.0)
                {
                    if (BerryCrop)
                    {
                        for (int Berry = 0; Berry < 3; ++Berry)
                            AddPart(Visual, Sphere, AtGround(PX + (Berry - 1) * 10, PY + 5, Height * 0.6f),
                                FVector(9, 9, 9), FLinearColor(0.42f, 0.035f, 0.09f));
                    }
                    else
                        AddPart(Visual, Sphere, AtGround(PX, PY, 7), FVector(24, 24, 15),
                            FLinearColor(0.66f, 0.43f, 0.21f));
                }
            }
        }
    }
    FRandomStream Random(Plot.id * 193 + 51);
    const int WeedCount = Stage(Plot.weeds, 8);
    for (int I = 0; I < WeedCount; ++I)
    {
        const float X = Center.x + Random.FRandRange(-100, 100);
        const float Y = Center.y + Random.FRandRange(-100, 100);
        AddPart(Visual, Cone, AtGround(X, Y, 19), FVector(22, 22, 38),
            FLinearColor(0.34f, 0.31f, 0.07f), false, FRotator(0, I * 47, 16));
    }
}

void AHomesteadWorld::UpdateLighting(const Homestead::State& State)
{
    const float Hour = static_cast<float>(FMath::Fmod(State.hour, 24.0));
    const float SolarAngle = (Hour - 6.0f) / 24.0f * 2.0f * PI;
    const float Elevation = FMath::Sin(SolarAngle);
    const float Daylight = FMath::SmoothStep(-0.1f, 0.25f, Elevation);
    // Matches the simulation's deterministic three-day spring weather cycle.
    const bool bRaining = static_cast<int64>(State.hour / 24.0) % 3 == 1 && Hour >= 9.0f && Hour < 15.0f;
    Sun->SetRelativeRotation(FRotator(-Elevation * 65.0f, (Hour - 6) * 15.0f - 70.0f, 0));
    Sun->SetIntensity(FMath::Lerp(0.0f, bRaining ? 19000.0f : 65000.0f, Daylight));
    Sun->SetLightColor(FMath::Lerp(FLinearColor(1.0f, 0.49f, 0.24f),
        FLinearColor(1.0f, 0.94f, 0.81f), FMath::Clamp(Elevation * 2, 0.0f, 1.0f)));
    Moon->SetRelativeRotation(FRotator(Elevation * 65.0f, (Hour - 6) * 15.0f + 110.0f, 0));
    Moon->SetIntensity(0.5f * (1.0f - Daylight));
    Sky->SetIntensity(FMath::Lerp(0.3f, 0.85f, Daylight));
    Fog->SetFogDensity(bRaining ? 0.035f : FMath::Lerp(0.016f, 0.007f, Daylight));
    Fog->SetFogInscatteringColor(bRaining ? FLinearColor(0.43f, 0.49f, 0.52f)
        : FMath::Lerp(FLinearColor(0.055f, 0.085f, 0.14f), FLinearColor(0.64f, 0.72f, 0.68f), Daylight));
}

void AHomesteadWorld::Initialize(const Homestead::State& State)
{
    ClearVisual(Preview);
    if (!bInitialized)
    {
        FieldMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
        GroundMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Trials/GrassGround_20260921_01/Materials/M_GrassGroundBlend.M_GrassGroundBlend"));
        if (!GroundMaterial)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Admitted grass-ground blend is missing; retaining original ground material."));
            GroundMaterial = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Game/SurvivalGame/Materials/M_Ground.M_Ground"));
        }
        RockMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Rock.M_Rock"));
        ImportedRock = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/MossRocks.MossRocks"));
        if (!GroundMaterial || !RockMaterial || !ImportedRock)
        {
            UE_LOG(LogHomesteadWorld, Warning,
                TEXT("Optional CC0 assets: ground material=%s, rock material=%s, MossRocks mesh=%s. Missing assets use original provisional shapes/M_Field. Bootstrap must import or rename the rock mesh to /Game/SurvivalGame/Environment/MossRocks.MossRocks."),
                GroundMaterial ? TEXT("loaded") : TEXT("missing"),
                RockMaterial ? TEXT("loaded") : TEXT("missing"),
                ImportedRock ? TEXT("loaded") : TEXT("missing"));
        }
        if (!FieldMaterial)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("PROTOTYPE FALLBACK: M_Field is missing. Run the editor bootstrap before judging visuals; engine default material will be used."));
            FieldMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
            if (GEngine)
            {
                GEngine->AddOnScreenDebugMessage(INDEX_NONE, 30.0f, FColor::Yellow,
                    TEXT("PROTOTYPE FALLBACK: M_Field missing. Run the editor bootstrap; these are not final visuals."));
            }
        }
        if (!Cube || !Sphere || !Cylinder || !Cone)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("PROTOTYPE FALLBACK: required Engine/BasicShapes assets are missing."));
        }
        BuildTerrain();
        BuildLighting();
        bInitialized = true;
        UE_LOG(LogHomesteadWorld, Display,
            TEXT("Loaded original provisional meadow geometry. This is a technical scene, not approved final realistic art."));
    }
    Refresh(State);
}

void AHomesteadWorld::Refresh(const Homestead::State& State)
{
    if (!bInitialized)
    {
        Initialize(State);
        return;
    }

    FString Layout;
    for (const auto& Node : State.resources)
    {
        Layout += FString::Printf(TEXT("%d:%.3f:%.3f;"), Node.id, Node.position.x, Node.position.y);
    }
    for (const auto& Structure : State.structures)
    {
        Layout += FString::Printf(TEXT("S:%d:%d;"), Structure.cellX, Structure.cellY);
    }
    for (const auto& Plot : State.plots)
    {
        Layout += FString::Printf(TEXT("P:%d:%d;"), Plot.cellX, Plot.cellY);
    }
    if (ResourceLayoutSignature != Layout || DecorationBatches.IsEmpty())
    {
        BuildDecorations(State);
        ResourceLayoutSignature = MoveTemp(Layout);
    }

    RemoveMissing(ResourceVisuals, State.resources);
    RemoveMissing(ResourceProduceVisuals, State.resources);
    for (const auto& Node : State.resources)
    {
        const bool bReady = Node.readyAtHour <= State.hour;
        const FString Signature = FString::Printf(TEXT("%d:%.3f:%.3f:%d"),
            static_cast<int>(Node.kind), Node.position.x, Node.position.y, Node.cleared);
        FHomesteadWorldVisual& Visual = ResourceVisuals.FindOrAdd(Node.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildResource(Visual, Node, false);
            Visual.Signature = Signature;
        }
        const FString ProduceSignature = Signature + (bReady ? TEXT(":ready") : TEXT(":harvested"));
        FHomesteadWorldVisual& Produce = ResourceProduceVisuals.FindOrAdd(Node.id);
        if (Produce.Signature != ProduceSignature)
        {
            ClearVisual(Produce);
            if (bReady)
            {
                BuildResource(Produce, Node, true);
            }
            Produce.Signature = ProduceSignature;
        }
    }

    RemoveMissing(StructureVisuals, State.structures);
    for (const auto& Structure : State.structures)
    {
        const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d"),
            static_cast<int>(Structure.kind), Structure.cellX, Structure.cellY, Structure.rotation,
            Structure.fuelHours > 0);
        FHomesteadWorldVisual& Visual = StructureVisuals.FindOrAdd(Structure.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildStructure(Visual, Structure, false);
            Visual.Signature = Signature;
        }
    }

    RemoveMissing(PlotVisuals, State.plots);
    for (const auto& Plot : State.plots)
    {
        const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d:%d:%d"),
            Plot.cellX, Plot.cellY, Plot.planted, Stage(Plot.growth, 12),
            Stage(Plot.moisture, 5), Stage(Plot.weeds, 8), static_cast<int>(Plot.kind));
        FHomesteadWorldVisual& Visual = PlotVisuals.FindOrAdd(Plot.id);
        if (Visual.Signature != Signature)
        {
            ClearVisual(Visual);
            BuildPlot(Visual, Plot);
            Visual.Signature = Signature;
        }
    }
    UpdateLighting(State);
}

void AHomesteadWorld::SetPlacementPreview(bool Visible, Homestead::Piece Kind, int CellX, int CellY, int Rotation)
{
    if (!Visible || !bInitialized || Kind == Homestead::Piece::Count)
    {
        ClearVisual(Preview);
        return;
    }
    const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d"), static_cast<int>(Kind), CellX, CellY, Rotation);
    if (Preview.Signature == Signature)
    {
        return;
    }
    ClearVisual(Preview);
    Homestead::Structure Structure;
    Structure.kind = Kind;
    Structure.cellX = CellX;
    Structure.cellY = CellY;
    Structure.rotation = Rotation;
    BuildStructure(Preview, Structure, true);
    Preview.Signature = Signature;
}
