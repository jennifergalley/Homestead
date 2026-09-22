#include "HomesteadRegionalDescriptorCache.h"

#include <algorithm>
#include <cmath>

namespace Homestead::Generation
{
namespace
{
std::int64_t FloorDivide(std::int64_t value, std::int64_t divisor)
{
    return value / divisor - (value % divisor < 0 ? 1 : 0);
}

bool SegmentIntersectsRectangle(double ax, double ay, double bx, double by,
    double lowX, double lowY, double highX, double highY)
{
    double minimum = 0.0;
    double maximum = 1.0;
    const double dx = bx - ax;
    const double dy = by - ay;
    const auto Clip = [&](double direction, double distance)
    {
        if (direction == 0.0) return distance >= 0.0;
        const double ratio = distance / direction;
        if (direction < 0.0)
            minimum = std::max(minimum, ratio);
        else
            maximum = std::min(maximum, ratio);
        return minimum <= maximum;
    };
    return Clip(-dx, ax - lowX) && Clip(dx, highX - ax)
        && Clip(-dy, ay - lowY) && Clip(dy, highY - ay);
}

bool SameReachKey(const RegionalGeneration::RiverReach& a,
    const RegionalGeneration::RiverReach& b)
{
    return a.key == b.key;
}

bool SameLakeId(const RegionalGeneration::LakeDescriptor& a,
    const RegionalGeneration::LakeDescriptor& b)
{
    return a.id == b.id;
}
}

bool LoadedRegionalDescriptorCache::HasWorld(WorldDescriptor world) const
{
    return hasWorld_ && world_.seed == world.seed
        && world_.generationVersion == world.generationVersion;
}

bool LoadedRegionalDescriptorCache::IsLoaded(RegionalGeneration::RegionCoord region) const
{
    return std::binary_search(loadedRegions_.begin(), loadedRegions_.end(), region);
}

RegionalGeneration::Status LoadedRegionalDescriptorCache::RefreshLoadedRegions(
    WorldDescriptor world, const std::vector<RegionalGeneration::RegionCoord>& regions)
{
    if (world.generationVersion != WorldGenerationVersion)
        return RegionalGeneration::Status::UnsupportedVersion;

    auto nextRegions = regions;
    std::sort(nextRegions.begin(), nextRegions.end());
    nextRegions.erase(std::unique(nextRegions.begin(), nextRegions.end()), nextRegions.end());

    if (!HasWorld(world))
    {
        evictionCount_ += entries_.size();
        entries_.clear();
        world_ = world;
        hasWorld_ = true;
    }
    else
    {
        for (auto iterator = entries_.begin(); iterator != entries_.end();)
        {
            if (!std::binary_search(nextRegions.begin(), nextRegions.end(), iterator->first))
            {
                iterator = entries_.erase(iterator);
                ++evictionCount_;
            }
            else
            {
                ++iterator;
            }
        }
    }

    loadedRegions_ = std::move(nextRegions);
    ++refreshCount_;
    return RegionalGeneration::Status::Ok;
}

RegionalCacheStoreResult LoadedRegionalDescriptorCache::Store(
    WorldDescriptor world, const RegionalGeneration::RegionalResult& result)
{
    if (!HasWorld(world) || !IsLoaded(result.region))
        return RegionalCacheStoreResult::Rejected;

    if (entries_.find(result.region) != entries_.end())
        return RegionalCacheStoreResult::Unchanged;

    entries_.emplace(result.region, result);
    ++storeCount_;
    return RegionalCacheStoreResult::Stored;
}

RegionalCacheStoreResult LoadedRegionalDescriptorCache::StoreGenerated(
    WorldDescriptor world, RegionalGeneration::Status status,
    const RegionalGeneration::RegionalResult& result)
{
    return status == RegionalGeneration::Status::Ok
        ? Store(world, result) : RegionalCacheStoreResult::Rejected;
}

const RegionalGeneration::RegionalResult* LoadedRegionalDescriptorCache::Find(
    WorldDescriptor world, RegionalGeneration::RegionCoord region) const
{
    if (!HasWorld(world) || !IsLoaded(region))
        return nullptr;
    const auto found = entries_.find(region);
    return found == entries_.end() ? nullptr : &found->second;
}

RegionalChunkDescriptorStatus LoadedRegionalDescriptorCache::DescribeChunkWater(
    WorldDescriptor world, ChunkCoord chunk, LoadedChunkWaterDescriptors& output) const
{
    using namespace RegionalGeneration;
    if (world.generationVersion != WorldGenerationVersion || !HasWorld(world))
        return RegionalChunkDescriptorStatus::UnsupportedVersion;

    const std::int64_t lowX = static_cast<std::int64_t>(chunk.x) * ChunkSizeCm;
    const std::int64_t lowY = static_cast<std::int64_t>(chunk.y) * ChunkSizeCm;
    const std::int64_t highX = lowX + ChunkSizeCm;
    const std::int64_t highY = lowY + ChunkSizeCm;
    const std::int64_t expandedLowX = lowX - DrainageSpacingCm;
    const std::int64_t expandedLowY = lowY - DrainageSpacingCm;
    const std::int64_t expandedHighX = highX + DrainageSpacingCm - 1;
    const std::int64_t expandedHighY = highY + DrainageSpacingCm - 1;

    const RegionCoord requiredLow{
        static_cast<std::int32_t>(FloorDivide(expandedLowX, RegionSizeCm)),
        static_cast<std::int32_t>(FloorDivide(expandedLowY, RegionSizeCm))};
    const RegionCoord requiredHigh{
        static_cast<std::int32_t>(FloorDivide(expandedHighX, RegionSizeCm)),
        static_cast<std::int32_t>(FloorDivide(expandedHighY, RegionSizeCm))};
    std::vector<RegionCoord> requiredRegions;
    bool complete = true;
    for (std::int64_t regionY = requiredLow.y; regionY <= requiredHigh.y; ++regionY)
        for (std::int64_t regionX = requiredLow.x; regionX <= requiredHigh.x; ++regionX)
        {
            const RegionCoord region{static_cast<std::int32_t>(regionX),
                static_cast<std::int32_t>(regionY)};
            requiredRegions.push_back(region);
            complete &= Find(world, region) != nullptr;
        }
    std::sort(requiredRegions.begin(), requiredRegions.end());
    const bool anyAvailable = std::any_of(requiredRegions.begin(), requiredRegions.end(),
        [&](RegionCoord region) { return Find(world, region) != nullptr; });
    if (!anyAvailable) return RegionalChunkDescriptorStatus::Incomplete;

    LoadedChunkWaterDescriptors result;
    result.chunk = chunk;
    for (const auto& Entry : entries_)
    {
        if (!std::binary_search(requiredRegions.begin(), requiredRegions.end(), Entry.first))
            continue;
        const auto& regional = Entry.second;
        for (const auto& reach : regional.reaches)
        {
            const double ax = static_cast<double>(reach.key.upstream.x * DrainageSpacingCm);
            const double ay = static_cast<double>(reach.key.upstream.y * DrainageSpacingCm);
            const double bx = static_cast<double>(reach.key.downstream.x * DrainageSpacingCm);
            const double by = static_cast<double>(reach.key.downstream.y * DrainageSpacingCm);
            if (SegmentIntersectsRectangle(ax, ay, bx, by, static_cast<double>(lowX),
                static_cast<double>(lowY), static_cast<double>(highX), static_cast<double>(highY))
                && std::find_if(result.reaches.begin(), result.reaches.end(),
                    [&](const RiverReach& existing) { return SameReachKey(existing, reach); })
                    == result.reaches.end())
                result.reaches.push_back(reach);
        }
        for (const auto& lake : regional.lakes)
        {
            bool intersects = SegmentIntersectsRectangle(
                static_cast<double>(lake.outlet.upstream.x * DrainageSpacingCm),
                static_cast<double>(lake.outlet.upstream.y * DrainageSpacingCm),
                static_cast<double>(lake.outlet.downstream.x * DrainageSpacingCm),
                static_cast<double>(lake.outlet.downstream.y * DrainageSpacingCm),
                static_cast<double>(lowX), static_cast<double>(lowY),
                static_cast<double>(highX), static_cast<double>(highY));
            if (!intersects)
                for (const auto& node : regional.nodes)
                    if (node.inLake && node.lakeId == lake.id)
                    {
                        const auto x = node.relief.node.x * DrainageSpacingCm;
                        const auto y = node.relief.node.y * DrainageSpacingCm;
                        if (x >= lowX && x <= highX && y >= lowY && y <= highY)
                        {
                            intersects = true;
                            break;
                        }
                    }
            if (intersects && std::find_if(result.lakes.begin(), result.lakes.end(),
                [&](const LakeDescriptor& existing) { return SameLakeId(existing, lake); })
                == result.lakes.end())
                result.lakes.push_back(lake);
        }
    }
    output = std::move(result);
    return complete ? RegionalChunkDescriptorStatus::Ready
        : RegionalChunkDescriptorStatus::Partial;
}
}
