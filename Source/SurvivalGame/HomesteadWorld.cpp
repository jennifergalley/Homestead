#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadLampLook.h"
#include "HomesteadWeather.h"

#include "HAL/IConsoleManager.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

DEFINE_LOG_CATEGORY(LogHomesteadWorld);

AHomesteadWorld::AHomesteadWorld()
{
    PrimaryActorTick.bCanEverTick = true;
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

void AHomesteadWorld::EndPlay(const EEndPlayReason::Type Reason)
{
    // The hair sky-lighting switch is a global render setting; never leave it off behind us.
    if (bHairSkyLightingOff)
        if (IConsoleVariable* HairSky = IConsoleManager::Get().FindConsoleVariable(TEXT("r.HairStrands.SkyLighting")))
            HairSky->Set(1, ECVF_SetByCode);
    bHairSkyLightingOff = false;
    Super::EndPlay(Reason);
}

void AHomesteadWorld::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdateFallingTree(DeltaSeconds);
    UpdateHearthFlicker(DeltaSeconds);
    LampDropFlickerTime += DeltaSeconds;
    HomesteadLampLook::SetLit(LampDropFlame.Get(), LampDropLight.Get(), bLampDropLit, LampDropFlickerTime, LampDropGlass.Get());
    UpdateHearthSound(DeltaSeconds);
    UpdateDoors(DeltaSeconds);
    UpdateClearPops(DeltaSeconds);
    UpdateSoilGrounding(DeltaSeconds);
    if (Weather) Weather->TickWeather(DeltaSeconds);
    if (ChunkBaselineBuild && ChunkBaselineBuild->IsReady())
    {
        FHomesteadChunkBaselineBuild Completed = ChunkBaselineBuild->Get();
        ChunkBaselineBuild.Reset();
        if (Descriptor.seed == Completed.World.seed
            && Descriptor.generationVersion == Completed.World.generationVersion)
            for (auto& Chunk : Completed.Chunks)
                ChunkBaselineCache.emplace(Chunk.chunk, MoveTemp(Chunk));
    }
    if (!RegionalDescriptorBuild || !RegionalDescriptorBuild->IsReady()) return;

    FHomesteadRegionalDescriptorBuild Completed = RegionalDescriptorBuild->Get();
    RegionalDescriptorBuild.Reset();
    if (Descriptor.seed == Completed.World.seed
        && Descriptor.generationVersion == Completed.World.generationVersion)
    {
        for (const auto& Entry : Completed.Entries)
        {
            if (RegionalDescriptors.StoreGenerated(
                Completed.World, Entry.ResultStatus, Entry.Result)
                != Homestead::Generation::RegionalCacheStoreResult::Rejected)
            {
                continue;
            }
            if (Entry.ResultStatus != Homestead::RegionalGeneration::Status::Ok)
            {
                RegionalDescriptorFailures[Entry.Region] = Entry.ResultStatus;
                UE_LOG(LogHomesteadWorld, Warning,
                    TEXT("Regional descriptor %d,%d unavailable: %s"),
                    Entry.Region.x, Entry.Region.y,
                    UTF8_TO_TCHAR(Homestead::RegionalGeneration::StatusMessage(Entry.ResultStatus)));
            }
        }
        int32 ReadyChunks = 0;
        int32 PartialChunks = 0;
        int32 IncompleteChunks = 0;
        int32 ReachDescriptors = 0;
        int32 LakeDescriptors = 0;
        for (const auto& Terrain : TerrainChunks)
        {
            Homestead::Generation::LoadedChunkWaterDescriptors Water;
            const auto Status = RegionalDescriptors.DescribeChunkWater(Descriptor,
                {Terrain.Key.X, Terrain.Key.Y}, Water);
            if (Status == Homestead::Generation::RegionalChunkDescriptorStatus::Ready)
            {
                ++ReadyChunks;
                ReachDescriptors += static_cast<int32>(Water.reaches.size());
                LakeDescriptors += static_cast<int32>(Water.lakes.size());
            }
            else if (Status == Homestead::Generation::RegionalChunkDescriptorStatus::Partial)
            {
                ++PartialChunks;
                ReachDescriptors += static_cast<int32>(Water.reaches.size());
                LakeDescriptors += static_cast<int32>(Water.lakes.size());
            }
            else
            {
                ++IncompleteChunks;
            }
        }
        UE_LOG(LogHomesteadWorld, Display,
            TEXT("Regional descriptor cache: loaded=%llu cached=%llu failures=%llu ready_chunks=%d partial_chunks=%d incomplete_chunks=%d reaches=%d lakes=%d builds=%llu"),
            static_cast<unsigned long long>(RegionalDescriptors.LoadedRegionCount()),
            static_cast<unsigned long long>(RegionalDescriptors.CachedRegionCount()),
            static_cast<unsigned long long>(RegionalDescriptorFailures.size()),
            ReadyChunks, PartialChunks, IncompleteChunks, ReachDescriptors, LakeDescriptors,
            static_cast<unsigned long long>(RegionalDescriptorBuildCount));
        if (!RebuildRegionalWater())
            UE_LOG(LogHomesteadWorld, Error, TEXT("Regional water rebuild failed."));
    }
    QueueRegionalDescriptorBuild();
}

bool AHomesteadWorld::IsPreparedFor(const Homestead::State& State) const
{
    return bTerrainReady && Descriptor.seed == State.world.seed
        && Descriptor.generationVersion == State.world.generationVersion && PreparedChunk == State.activeChunk;
}

int32 AHomesteadWorld::StartingViewObstructions(FVector Focus, FVector Camera) const
{
    int32 Count = 0;
    for (const auto& Entry : ActiveTreeInstances)
    {
        const auto* Batch = ActiveTreeBatches.FindRef(Entry.Value.Visual.MeshPath).Get();
        const UStaticMesh* Mesh = Batch ? Batch->GetStaticMesh().Get() : nullptr;
        if (!Mesh) continue;
        const FTransform& Transform = Entry.Value.Visual.Transform;
        const FVector Start = Transform.InverseTransformPosition(Focus);
        const FVector End = Transform.InverseTransformPosition(Camera);
        const FBox Bounds = Mesh->GetBoundingBox().ExpandBy(20);
        Count += FMath::LineBoxIntersection(Bounds, Start, End, End - Start) ? 1 : 0;
    }
    return Count;
}

bool AHomesteadWorld::Initialize(const Homestead::Simulation& Simulation)
{
    ClearVisual(Preview);
    if (!bInitialized)
    {
        FieldMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
        CreekWaterMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_CreekWater.M_CreekWater"));
        if (!CreekWaterMaterial)
            UE_LOG(LogHomesteadWorld, Warning, TEXT("M_CreekWater is missing; the creek falls back to a flat tint. Run Scripts/bootstrap_unreal.py."));
        GroundMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Trials/GrassGround_20260921_01/Materials/M_GrassGroundBlend.M_GrassGroundBlend"));
        if (!GroundMaterial)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("Admitted grass-ground blend is missing; world preparation stopped."));
            return false;
        }
        RockMaterial = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/SurvivalGame/Materials/M_Rock.M_Rock"));
        ImportedRock = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/MossRocks.MossRocks"));
        if (!GroundMaterial || !RockMaterial || !ImportedRock)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("Required woodland assets: ground material=%s, rock material=%s, MossRocks mesh=%s. World preparation stopped."),
                GroundMaterial ? TEXT("loaded") : TEXT("missing"),
                RockMaterial ? TEXT("loaded") : TEXT("missing"),
                ImportedRock ? TEXT("loaded") : TEXT("missing"));
            return false;
        }
        if (!FieldMaterial)
        {
            UE_LOG(LogHomesteadWorld, Error,
                TEXT("M_Field is missing; world preparation stopped without a default-material substitute."));
            return false;
        }
        if (!Cube || !Sphere || !Cylinder || !Cone)
        {
            UE_LOG(LogHomesteadWorld, Error, TEXT("PROTOTYPE FALLBACK: required Engine/BasicShapes assets are missing."));
        }
        if (!FieldMaterial || !GroundMaterial || !RockMaterial || !ImportedRock || !Cube || !Sphere || !Cylinder || !Cone)
            return false;
        if (!LoadCameraSafeFoliageMaterials())
            return false;
        BuildLighting();
        bInitialized = true;
        UE_LOG(LogHomesteadWorld, Display,
            TEXT("Preparing persistent generated woodland. Runtime evidence does not establish final art acceptance."));
    }
    return Refresh(Simulation);
}
