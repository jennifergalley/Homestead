#pragma once

#include <array>
#include <cstdint>
#include <vector>

namespace Homestead::RegionalGeneration
{
constexpr std::uint32_t RegionalGenerationVersion = 1;
constexpr std::int64_t DrainageSpacingCm = 3000;
constexpr std::int64_t RegionSizeCm = 96000;
constexpr int NodesPerRegionSide = 32;
constexpr int NodesPerRegion = NodesPerRegionSide * NodesPerRegionSide;
constexpr int MaximumBasinNodes = 4096;
constexpr int UpstreamRadiusNodes = 8;

struct RegionalDescriptor
{
    std::uint64_t seed = 0;
    std::uint32_t generationVersion = RegionalGenerationVersion;
};

struct RegionCoord { std::int32_t x = 0; std::int32_t y = 0; };
struct NodeCoord { std::int64_t x = 0; std::int64_t y = 0; };

bool operator==(RegionCoord a, RegionCoord b);
bool operator!=(RegionCoord a, RegionCoord b);
bool operator<(RegionCoord a, RegionCoord b);
bool operator==(NodeCoord a, NodeCoord b);
bool operator!=(NodeCoord a, NodeCoord b);
bool operator<(NodeCoord a, NodeCoord b);

enum class Status { Ok, UnsupportedVersion, OutOfRange, BasinTooLarge };
const char* StatusMessage(Status status);

struct ReliefSample
{
    NodeCoord node;
    std::int64_t elevationMm = 0;
    std::uint16_t mountain = 0;
    std::uint16_t ridge = 0;
    std::uint16_t valley = 0;
    std::uint16_t wetness = 0;
};

struct OutletKey
{
    NodeCoord upstream;
    NodeCoord downstream;
};
bool operator==(const OutletKey& a, const OutletKey& b);

struct LakeDescriptor
{
    NodeCoord id;
    OutletKey outlet;
    std::int64_t surfaceMm = 0;
    std::uint16_t memberCount = 0;
};

struct DrainageNode
{
    ReliefSample relief;
    NodeCoord downstream;
    NodeCoord lakeId;
    std::int64_t waterSurfaceMm = 0;
    std::uint16_t accumulation = 1;
    std::uint8_t widthClass = 0;
    std::uint8_t depthClass = 0;
    bool inLake = false;
    bool lakeOutlet = false;
};

struct RiverReach
{
    OutletKey key;
    std::int64_t upstreamSurfaceMm = 0;
    std::int64_t downstreamSurfaceMm = 0;
    std::uint16_t accumulation = 1;
    std::uint8_t widthClass = 0;
    std::uint8_t depthClass = 0;
};

struct RegionalResult
{
    RegionCoord region;
    std::array<DrainageNode, NodesPerRegion> nodes{};
    std::vector<LakeDescriptor> lakes;
    std::vector<RiverReach> reaches;
    std::uint32_t maximumBasinSearch = MaximumBasinNodes;
};

Status RegionAtCm(std::int64_t xCm, std::int64_t yCm, RegionCoord& output);
Status SampleRelief(RegionalDescriptor descriptor, NodeCoord node, ReliefSample& output);
Status ResolveDrainage(RegionalDescriptor descriptor, NodeCoord node, DrainageNode& output);
Status GenerateRegion(RegionalDescriptor descriptor, RegionCoord region, RegionalResult& output);
}
