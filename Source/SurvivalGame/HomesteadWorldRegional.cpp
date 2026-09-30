#include "HomesteadWorld.h"
#include "HomesteadWorldCommon.h"
#include "HomesteadWorldLog.h"
#include "HomesteadEstateTerrain.h"

#include "Async/Async.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "ProceduralMeshComponent.h"

namespace HomesteadWorldRegional
{
bool RegionalReachLess(const Homestead::RegionalGeneration::RiverReach& A,
    const Homestead::RegionalGeneration::RiverReach& B)
{
    return A.key.upstream != B.key.upstream ? A.key.upstream < B.key.upstream
        : A.key.downstream < B.key.downstream;
}

bool ClipRegionalSegment(FVector2D& A, FVector2D& B,
    double LowX, double LowY, double HighX, double HighY)
{
    double Minimum = 0.0;
    double Maximum = 1.0;
    const FVector2D Delta = B - A;
    const auto Clip = [&](double Direction, double Distance)
    {
        if (FMath::IsNearlyZero(Direction)) return Distance >= 0.0;
        const double Ratio = Distance / Direction;
        if (Direction < 0.0) Minimum = FMath::Max(Minimum, Ratio);
        else Maximum = FMath::Min(Maximum, Ratio);
        return Minimum <= Maximum;
    };
    if (!Clip(-Delta.X, A.X - LowX) || !Clip(Delta.X, HighX - A.X)
        || !Clip(-Delta.Y, A.Y - LowY) || !Clip(Delta.Y, HighY - A.Y))
        return false;
    const FVector2D Start = A;
    A = Start + Delta * Minimum;
    B = Start + Delta * Maximum;
    return FVector2D::Distance(A, B) > 1.0;
}
}

using HomesteadWorldCommon::ProfileChunkPublishing;
using HomesteadWorldRegional::ClipRegionalSegment;
using HomesteadWorldRegional::RegionalReachLess;

void AHomesteadWorld::ClearRegionalWater()
{
    for (auto& Entry : RegionalWaterMeshes)
        if (IsValid(Entry.Value.Get())) Entry.Value->DestroyComponent();
    RegionalWaterMeshes.Reset();
    RegionalWaterSignature.Reset();
    RenderedRegionalReachKey.Reset();
    RenderedRegionalReachReferences = 0;
    UnrenderedRegionalReachReferences = 0;
    UnrenderedRegionalLakeReferences = 0;
}

bool AHomesteadWorld::RebuildRegionalWater()
{
    using namespace Homestead::Generation;
    using namespace Homestead::RegionalGeneration;
    TArray<RiverReach> Reaches;
    int32 LakeReferences = 0;
    for (const auto& Terrain : TerrainChunks)
    {
        LoadedChunkWaterDescriptors Water;
        const auto Status = RegionalDescriptors.DescribeChunkWater(
            Descriptor, {Terrain.Key.X, Terrain.Key.Y}, Water);
        if (Status != RegionalChunkDescriptorStatus::Ready
            && Status != RegionalChunkDescriptorStatus::Partial)
            continue;
        LakeReferences += static_cast<int32>(Water.lakes.size());
        for (const auto& Reach : Water.reaches)
            if (!Reaches.ContainsByPredicate([&](const RiverReach& Existing)
                { return Existing.key == Reach.key; }))
                Reaches.Add(Reach);
    }
    const FVector2D ActiveCenter(
        (static_cast<double>(PreparedChunk.x) + 0.5) * ChunkSizeCm,
        (static_cast<double>(PreparedChunk.y) + 0.5) * ChunkSizeCm);
    Reaches.Sort([ActiveCenter](const RiverReach& A, const RiverReach& B)
    {
        const FVector2D MidA(
            (A.key.upstream.x + A.key.downstream.x) * DrainageSpacingCm * 0.5,
            (A.key.upstream.y + A.key.downstream.y) * DrainageSpacingCm * 0.5);
        const FVector2D MidB(
            (B.key.upstream.x + B.key.downstream.x) * DrainageSpacingCm * 0.5,
            (B.key.upstream.y + B.key.downstream.y) * DrainageSpacingCm * 0.5);
        const double DistanceA = FVector2D::DistSquared(MidA, ActiveCenter);
        const double DistanceB = FVector2D::DistSquared(MidB, ActiveCenter);
        return !FMath::IsNearlyEqual(DistanceA, DistanceB)
            ? DistanceA < DistanceB : RegionalReachLess(A, B);
    });
    FString Signature = FString::Printf(TEXT("%llu:%u;"),
        static_cast<unsigned long long>(Descriptor.seed), Descriptor.generationVersion);
    for (const auto& Terrain : TerrainChunks)
        Signature += FString::Printf(TEXT("%d,%d;"), Terrain.Key.X, Terrain.Key.Y);
    if (!Reaches.IsEmpty())
        Signature += FString::Printf(TEXT("%lld,%lld>%lld,%lld;"),
            Reaches[0].key.upstream.x, Reaches[0].key.upstream.y,
            Reaches[0].key.downstream.x, Reaches[0].key.downstream.y);
    if (RegionalWaterSignature == Signature) return true;

    ClearRegionalWater();
    RegionalWaterSignature = Signature;
    UnrenderedRegionalReachReferences = Reaches.Num();
    UnrenderedRegionalLakeReferences = LakeReferences;
    if (Reaches.IsEmpty()) return true;

    const RiverReach& Reach = Reaches[0];
    RenderedRegionalReachKey = FString::Printf(TEXT("%lld,%lld>%lld,%lld"),
        Reach.key.upstream.x, Reach.key.upstream.y,
        Reach.key.downstream.x, Reach.key.downstream.y);
    const double Width = 70.0 + Reach.widthClass * 25.0;
    for (const auto& Terrain : TerrainChunks)
    {
        const double LowX = static_cast<double>(Terrain.Key.X) * ChunkSizeCm;
        const double LowY = static_cast<double>(Terrain.Key.Y) * ChunkSizeCm;
        const double HighX = LowX + ChunkSizeCm;
        const double HighY = LowY + ChunkSizeCm;
        FVector2D A(Reach.key.upstream.x * DrainageSpacingCm,
            Reach.key.upstream.y * DrainageSpacingCm);
        FVector2D B(Reach.key.downstream.x * DrainageSpacingCm,
            Reach.key.downstream.y * DrainageSpacingCm);
        if (!ClipRegionalSegment(A, B, LowX, LowY, HighX, HighY)) continue;
        auto* Mesh = NewObject<UProceduralMeshComponent>(this);
        if (!Mesh) return false;
        Mesh->SetupAttachment(GetRootComponent());
        Mesh->SetMobility(EComponentMobility::Static);
        Mesh->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCanEverAffectNavigation(false);
        Mesh->SetCastShadow(false);
        Mesh->ComponentTags.Add(TEXT("GeneratedRegionalWater"));
        Mesh->ComponentTags.Add(FName(*RenderedRegionalReachKey));

        const FVector2D Direction = (B - A).GetSafeNormal();
        const FVector2D Perpendicular(-Direction.Y, Direction.X);
        const int32 Steps = FMath::Clamp(
            FMath::CeilToInt(FVector2D::Distance(A, B) / 100.0), 1, 32);
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FLinearColor> Colors;
        TArray<FProcMeshTangent> Tangents;
        for (int32 Step = 0; Step <= Steps; ++Step)
        {
            const double Alpha = static_cast<double>(Step) / Steps;
            const FVector2D Center = FMath::Lerp(A, B, Alpha);
            const double Z = GroundHeight(Center.X, Center.Y) + 3.0;
            Vertices.Add(FVector(Center - Perpendicular * Width, Z));
            Vertices.Add(FVector(Center + Perpendicular * Width, Z));
            Normals.Add(FVector::UpVector); Normals.Add(FVector::UpVector);
            UVs.Add(FVector2D(0, Alpha)); UVs.Add(FVector2D(1, Alpha));
            Colors.Add(FLinearColor::White); Colors.Add(FLinearColor::White);
            Tangents.Add(FProcMeshTangent(Direction.X, Direction.Y, 0));
            Tangents.Add(FProcMeshTangent(Direction.X, Direction.Y, 0));
            if (Step < Steps)
            {
                const int32 Index = Step * 2;
                Triangles.Append({Index, Index + 2, Index + 1,
                    Index + 1, Index + 2, Index + 3});
            }
        }
        Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals,
            UVs, Colors, Tangents, false);
        Mesh->SetMaterial(0, Material(FLinearColor(0.075f, 0.26f, 0.29f), 0.16f));
        Mesh->RegisterComponent();
        RegionalWaterMeshes.Add(Terrain.Key, Mesh);
        ++RenderedRegionalReachReferences;
    }
    UnrenderedRegionalReachReferences = FMath::Max(
        0, UnrenderedRegionalReachReferences - (RenderedRegionalReachReferences > 0 ? 1 : 0));
    return true;
}

bool AHomesteadWorld::RefreshRegionalDescriptors(
    Homestead::Generation::WorldDescriptor World,
    const std::vector<Homestead::RegionalGeneration::RegionCoord>& Regions)
{
    const bool SameWorld = RegionalDescriptors.IsForWorld(World);
    const auto Status = RegionalDescriptors.RefreshLoadedRegions(World, Regions);
    if (Status != Homestead::RegionalGeneration::Status::Ok)
    {
        UE_LOG(LogHomesteadWorld, Error, TEXT("Loaded regional cache refresh failed: %s"),
            UTF8_TO_TCHAR(Homestead::RegionalGeneration::StatusMessage(Status)));
        return false;
    }

    DesiredRegionalDescriptors = Regions;
    std::sort(DesiredRegionalDescriptors.begin(), DesiredRegionalDescriptors.end());
    DesiredRegionalDescriptors.erase(
        std::unique(DesiredRegionalDescriptors.begin(), DesiredRegionalDescriptors.end()),
        DesiredRegionalDescriptors.end());
    if (!SameWorld)
    {
        RegionalDescriptorFailures.clear();
    }
    else
    {
        for (auto Iterator = RegionalDescriptorFailures.begin();
            Iterator != RegionalDescriptorFailures.end();)
        {
            if (!std::binary_search(DesiredRegionalDescriptors.begin(),
                DesiredRegionalDescriptors.end(), Iterator->first))
                Iterator = RegionalDescriptorFailures.erase(Iterator);
            else
                ++Iterator;
        }
    }
    QueueRegionalDescriptorBuild();
    return true;
}

void AHomesteadWorld::QueueRegionalDescriptorBuild()
{
    if (RegionalDescriptorBuild) return;

    std::vector<Homestead::RegionalGeneration::RegionCoord> Missing;
    for (const auto Region : DesiredRegionalDescriptors)
        if (!RegionalDescriptors.Find(Descriptor, Region)
            && RegionalDescriptorFailures.find(Region) == RegionalDescriptorFailures.end())
            Missing.push_back(Region);
    if (Missing.empty()) return;

    const auto World = Descriptor;
    RegionalDescriptorBuildCount += Missing.size();
    RegionalDescriptorBuild = MakeUnique<TFuture<FHomesteadRegionalDescriptorBuild>>(
        Async(EAsyncExecution::ThreadPool, [World, Missing = std::move(Missing)]()
        {
            FHomesteadRegionalDescriptorBuild Build;
            Build.World = World;
            const Homestead::RegionalGeneration::RegionalDescriptor RegionalWorld{
                World.seed, Homestead::RegionalGeneration::RegionalGenerationVersion};
            Build.Entries.resize(Missing.size());
            ParallelFor(static_cast<int32>(Missing.size()), [&](int32 Index)
            {
                auto& Entry = Build.Entries[Index];
                const auto Region = Missing[Index];
                Entry.Region = Region;
                Entry.ResultStatus = Homestead::RegionalGeneration::GenerateRegion(
                    RegionalWorld, Region, Entry.Result);
            });
            return Build;
        }));
}
