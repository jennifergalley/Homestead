#pragma once

#include "HomesteadRegionalGeneration.h"
#include "HomesteadWorldGeneration.h"

#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

namespace Homestead::Generation
{
enum class RegionalCacheStoreResult
{
    Stored,
    Unchanged,
    Rejected
};

enum class RegionalChunkDescriptorStatus
{
    Ready,
    Partial,
    Incomplete,
    UnsupportedVersion,
    OutOfRange
};

struct LoadedChunkWaterDescriptors
{
    ChunkCoord chunk;
    std::vector<RegionalGeneration::RiverReach> reaches;
    std::vector<RegionalGeneration::LakeDescriptor> lakes;
};

class LoadedRegionalDescriptorCache
{
public:
    RegionalGeneration::Status RefreshLoadedRegions(
        WorldDescriptor world, const std::vector<RegionalGeneration::RegionCoord>& regions);
    RegionalCacheStoreResult Store(
        WorldDescriptor world, const RegionalGeneration::RegionalResult& result);
    RegionalCacheStoreResult StoreGenerated(WorldDescriptor world,
        RegionalGeneration::Status status,
        const RegionalGeneration::RegionalResult& result);
    const RegionalGeneration::RegionalResult* Find(
        WorldDescriptor world, RegionalGeneration::RegionCoord region) const;
    RegionalChunkDescriptorStatus DescribeChunkWater(
        WorldDescriptor world, ChunkCoord chunk, LoadedChunkWaterDescriptors& output) const;
    bool IsForWorld(WorldDescriptor world) const { return HasWorld(world); }

    std::size_t LoadedRegionCount() const { return loadedRegions_.size(); }
    std::size_t CachedRegionCount() const { return entries_.size(); }
    std::uint64_t RefreshCount() const { return refreshCount_; }
    std::uint64_t StoreCount() const { return storeCount_; }
    std::uint64_t EvictionCount() const { return evictionCount_; }

private:
    bool HasWorld(WorldDescriptor world) const;
    bool IsLoaded(RegionalGeneration::RegionCoord region) const;

    WorldDescriptor world_{};
    bool hasWorld_ = false;
    std::vector<RegionalGeneration::RegionCoord> loadedRegions_;
    std::map<RegionalGeneration::RegionCoord, RegionalGeneration::RegionalResult> entries_;
    std::uint64_t refreshCount_ = 0;
    std::uint64_t storeCount_ = 0;
    std::uint64_t evictionCount_ = 0;
};
}
