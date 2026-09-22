#include "HomesteadRegionalGeneration.h"

#include <algorithm>
#include <cstdlib>
#include <limits>
#include <map>
#include <queue>
#include <set>
#include <utility>

namespace Homestead::RegionalGeneration
{
namespace
{
constexpr std::int64_t FixedOne = 65536;
constexpr std::int64_t MinRegion = std::numeric_limits<std::int32_t>::min();
constexpr std::int64_t MaxRegion = std::numeric_limits<std::int32_t>::max();
constexpr NodeCoord NeighborOffsets[] = {
    {-1, -1}, {0, -1}, {1, -1}, {-1, 0},
    {1, 0}, {-1, 1}, {0, 1}, {1, 1}};

std::int64_t FloorDivide(std::int64_t value, std::int64_t divisor)
{
    return value / divisor - (value % divisor < 0 ? 1 : 0);
}

std::uint64_t Mix(std::uint64_t value)
{
    value = (value ^ (value >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
    value = (value ^ (value >> 27)) * UINT64_C(0x94d049bb133111eb);
    return value ^ (value >> 31);
}

std::uint64_t Hash(RegionalDescriptor descriptor, std::int64_t x, std::int64_t y,
    std::uint64_t channel)
{
    const auto hy = Mix(static_cast<std::uint64_t>(y) + UINT64_C(0x632be59bd9b4e019));
    return Mix(Mix(descriptor.seed ^ channel) ^
        Mix(static_cast<std::uint64_t>(x) + UINT64_C(0x9e3779b97f4a7c15)) ^
        ((hy << 32) | (hy >> 32)));
}

std::int64_t SmoothFixed(std::int64_t value)
{
    return value * value * (3 * FixedOne - 2 * value) / (FixedOne * FixedOne);
}

std::int64_t LerpFixed(std::int64_t a, std::int64_t b, std::int64_t weight)
{
    return a + (b - a) * weight / FixedOne;
}

std::int64_t Noise(RegionalDescriptor descriptor, std::int64_t x, std::int64_t y,
    std::int64_t period, std::uint64_t channel)
{
    const auto cellX = FloorDivide(x, period);
    const auto cellY = FloorDivide(y, period);
    const auto tx = SmoothFixed((x - cellX * period) * FixedOne / period);
    const auto ty = SmoothFixed((y - cellY * period) * FixedOne / period);
    const auto value = [&](std::int64_t cx, std::int64_t cy) {
        return static_cast<std::int64_t>(Hash(descriptor, cx, cy, channel) & 65535U) - 32768;
    };
    return LerpFixed(LerpFixed(value(cellX, cellY), value(cellX + 1, cellY), tx),
        LerpFixed(value(cellX, cellY + 1), value(cellX + 1, cellY + 1), tx), ty);
}

std::uint16_t UnitPotential(std::int64_t value)
{
    const auto adjusted = std::max<std::int64_t>(0, std::min<std::int64_t>(65535, value + 32768));
    return static_cast<std::uint16_t>(adjusted);
}

bool ValidDescriptor(RegionalDescriptor descriptor)
{
    return descriptor.generationVersion == RegionalGenerationVersion;
}

bool ValidNode(NodeCoord node)
{
    constexpr std::int64_t low = MinRegion * NodesPerRegionSide;
    constexpr std::int64_t high = (MaxRegion + 1) * NodesPerRegionSide;
    return node.x >= low && node.x < high && node.y >= low && node.y < high;
}

std::int64_t Elevation(RegionalDescriptor descriptor, NodeCoord node)
{
    const auto continental = Noise(descriptor, node.x, node.y, 256, 11);
    const auto regional = Noise(descriptor, node.x, node.y, 96, 12);
    const auto valley = Noise(descriptor, node.x, node.y, 32, 13);
    const auto ridges = std::abs(Noise(descriptor, node.x, node.y, 128, 14)) - 16384;
    return continental * 8 + regional * 4 + valley * 2 + ridges * 3;
}

ReliefSample Relief(RegionalDescriptor descriptor, NodeCoord node)
{
    const auto broad = Noise(descriptor, node.x, node.y, 256, 11);
    const auto ridgeNoise = std::abs(Noise(descriptor, node.x, node.y, 128, 14));
    const auto valleyNoise = Noise(descriptor, node.x, node.y, 32, 13);
    ReliefSample sample;
    sample.node = node;
    sample.elevationMm = Elevation(descriptor, node);
    sample.mountain = UnitPotential(broad);
    sample.ridge = static_cast<std::uint16_t>(
        std::min<std::int64_t>(65535, ridgeNoise * 2));
    sample.valley = UnitPotential(-valleyNoise);
    sample.wetness = UnitPotential(
        (Noise(descriptor, node.x, node.y, 64, 15) - broad) / 2);
    return sample;
}

bool LowestNeighbor(RegionalDescriptor descriptor, NodeCoord node, NodeCoord& output)
{
    const auto elevation = Elevation(descriptor, node);
    auto bestElevation = elevation;
    NodeCoord best = node;
    for (const auto offset : NeighborOffsets)
    {
        const NodeCoord candidate{node.x + offset.x, node.y + offset.y};
        if (!ValidNode(candidate)) continue;
        const auto candidateElevation = Elevation(descriptor, candidate);
        if (candidateElevation < bestElevation ||
            (candidateElevation == bestElevation && candidate < best))
        {
            best = candidate;
            bestElevation = candidateElevation;
        }
    }
    if (bestElevation >= elevation) return false;
    output = best;
    return true;
}

bool Contains(const std::vector<NodeCoord>& values, NodeCoord value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}

struct Frontier
{
    std::int64_t cost = 0;
    NodeCoord parent;
    NodeCoord node;
};

struct FrontierLater
{
    bool operator()(const Frontier& a, const Frontier& b) const
    {
        if (a.cost != b.cost) return a.cost > b.cost;
        if (a.node != b.node) return b.node < a.node;
        return b.parent < a.parent;
    }
};

struct Basin
{
    LakeDescriptor lake;
    std::vector<NodeCoord> members;
};

Status ResolveBasin(RegionalDescriptor descriptor, NodeCoord sink, Basin& output)
{
    Basin basin;
    basin.members.push_back(sink);
    std::set<NodeCoord> settled{sink};
    std::map<NodeCoord, Frontier> best;
    std::priority_queue<Frontier, std::vector<Frontier>, FrontierLater> frontier;
    const auto sinkElevation = Elevation(descriptor, sink);
    const auto addNeighbors = [&](NodeCoord parent, std::int64_t pathCost, auto& queue,
        const auto& completed, auto& candidates) {
        for (const auto offset : NeighborOffsets)
        {
            const NodeCoord node{parent.x + offset.x, parent.y + offset.y};
            if (!ValidNode(node) || completed.count(node) != 0) continue;
            const Frontier candidate{std::max(pathCost, Elevation(descriptor, node)), parent, node};
            const auto existing = candidates.find(node);
            if (existing != candidates.end() &&
                (existing->second.cost < candidate.cost ||
                    (existing->second.cost == candidate.cost &&
                        !(candidate.parent < existing->second.parent))))
                continue;
            candidates[node] = candidate;
            queue.push(candidate);
        }
    };
    addNeighbors(sink, sinkElevation, frontier, settled, best);
    while (!frontier.empty())
    {
        const auto next = frontier.top();
        frontier.pop();
        const auto current = best.find(next.node);
        if (settled.count(next.node) != 0 || current == best.end() ||
            current->second.cost != next.cost || current->second.parent != next.parent)
            continue;
        if (Elevation(descriptor, next.node) < sinkElevation)
        {
            basin.lake.id = *std::min_element(basin.members.begin(), basin.members.end());
            basin.lake.outlet = {next.parent, next.node};
            basin.lake.surfaceMm = next.cost;
            basin.lake.memberCount = static_cast<std::uint16_t>(basin.members.size());
            output = std::move(basin);
            return Status::Ok;
        }
        if (basin.members.size() >= MaximumBasinNodes) return Status::BasinTooLarge;
        settled.insert(next.node);
        basin.members.push_back(next.node);
        addNeighbors(next.node, next.cost, frontier, settled, best);
    }
    return Status::BasinTooLarge;
}

Status FindSink(RegionalDescriptor descriptor, NodeCoord start, NodeCoord& sink)
{
    auto current = start;
    for (int step = 0; step < MaximumBasinNodes; ++step)
    {
        NodeCoord next;
        if (!LowestNeighbor(descriptor, current, next))
        {
            sink = current;
            return Status::Ok;
        }
        current = next;
    }
    return Status::BasinTooLarge;
}

std::uint16_t Accumulation(RegionalDescriptor descriptor, NodeCoord target)
{
    std::uint32_t count = 1;
    for (int y = -UpstreamRadiusNodes; y <= UpstreamRadiusNodes; ++y)
        for (int x = -UpstreamRadiusNodes; x <= UpstreamRadiusNodes; ++x)
        {
            if (x == 0 && y == 0) continue;
            NodeCoord current{target.x + x, target.y + y};
            for (int step = 0; step < UpstreamRadiusNodes * 2; ++step)
            {
                NodeCoord next;
                if (!LowestNeighbor(descriptor, current, next)) break;
                if (next == target)
                {
                    ++count;
                    break;
                }
                current = next;
            }
        }
    return static_cast<std::uint16_t>(std::min<std::uint32_t>(count, 65535));
}

void SetFlowClasses(DrainageNode& node)
{
    node.widthClass = node.accumulation >= 32 ? 4 :
        (node.accumulation >= 16 ? 3 : (node.accumulation >= 8 ? 2 :
            (node.accumulation >= 4 ? 1 : 0)));
    node.depthClass = node.accumulation >= 24 ? 3 :
        (node.accumulation >= 12 ? 2 : (node.accumulation >= 5 ? 1 : 0));
}
}

bool operator==(RegionCoord a, RegionCoord b) { return a.x == b.x && a.y == b.y; }
bool operator!=(RegionCoord a, RegionCoord b) { return !(a == b); }
bool operator<(RegionCoord a, RegionCoord b)
{
    return a.x < b.x || (a.x == b.x && a.y < b.y);
}
bool operator==(NodeCoord a, NodeCoord b) { return a.x == b.x && a.y == b.y; }
bool operator!=(NodeCoord a, NodeCoord b) { return !(a == b); }
bool operator<(NodeCoord a, NodeCoord b)
{
    return a.x < b.x || (a.x == b.x && a.y < b.y);
}
bool operator==(const OutletKey& a, const OutletKey& b)
{
    return a.upstream == b.upstream && a.downstream == b.downstream;
}

const char* StatusMessage(Status status)
{
    switch (status)
    {
    case Status::Ok: return "Regional generation result is available.";
    case Status::UnsupportedVersion: return "Regional generation version is unsupported.";
    case Status::OutOfRange: return "Regional generation coordinates are out of range.";
    case Status::BasinTooLarge: return "Regional basin exceeds the bounded prototype search.";
    }
    return "Unknown regional generation status.";
}

Status RegionAtCm(std::int64_t xCm, std::int64_t yCm, RegionCoord& output)
{
    const auto x = FloorDivide(xCm, RegionSizeCm);
    const auto y = FloorDivide(yCm, RegionSizeCm);
    if (x < MinRegion || x > MaxRegion || y < MinRegion || y > MaxRegion)
        return Status::OutOfRange;
    output = {static_cast<std::int32_t>(x), static_cast<std::int32_t>(y)};
    return Status::Ok;
}

Status SampleRelief(RegionalDescriptor descriptor, NodeCoord node, ReliefSample& output)
{
    if (!ValidDescriptor(descriptor)) return Status::UnsupportedVersion;
    if (!ValidNode(node)) return Status::OutOfRange;
    output = Relief(descriptor, node);
    return Status::Ok;
}

Status ResolveDrainage(RegionalDescriptor descriptor, NodeCoord node, DrainageNode& output)
{
    if (!ValidDescriptor(descriptor)) return Status::UnsupportedVersion;
    if (!ValidNode(node)) return Status::OutOfRange;
    NodeCoord sink;
    const auto sinkStatus = FindSink(descriptor, node, sink);
    if (sinkStatus != Status::Ok) return sinkStatus;
    Basin basin;
    const auto basinStatus = ResolveBasin(descriptor, sink, basin);
    if (basinStatus != Status::Ok) return basinStatus;

    DrainageNode result;
    result.relief = Relief(descriptor, node);
    result.waterSurfaceMm = result.relief.elevationMm;
    NodeCoord direct;
    const bool hasDirect = LowestNeighbor(descriptor, node, direct);
    if (Contains(basin.members, node))
    {
        result.inLake = true;
        result.lakeId = basin.lake.id;
        result.waterSurfaceMm = basin.lake.surfaceMm;
        result.lakeOutlet = node == basin.lake.outlet.upstream;
        result.downstream = result.lakeOutlet ? basin.lake.outlet.downstream :
            (hasDirect ? direct : sink);
    }
    else
    {
        result.downstream = hasDirect ? direct : basin.lake.outlet.downstream;
    }
    result.accumulation = Accumulation(descriptor, node);
    SetFlowClasses(result);
    output = result;
    return Status::Ok;
}

Status GenerateRegion(RegionalDescriptor descriptor, RegionCoord region, RegionalResult& output)
{
    if (!ValidDescriptor(descriptor)) return Status::UnsupportedVersion;
    RegionalResult result;
    result.region = region;
    const auto originX = static_cast<std::int64_t>(region.x) * NodesPerRegionSide;
    const auto originY = static_cast<std::int64_t>(region.y) * NodesPerRegionSide;
    std::map<NodeCoord, NodeCoord> sinks;
    std::map<NodeCoord, Basin> basins;
    const auto findSink = [&](NodeCoord start, NodeCoord& sink) {
        std::vector<NodeCoord> path;
        auto current = start;
        for (int step = 0; step < MaximumBasinNodes; ++step)
        {
            const auto cached = sinks.find(current);
            if (cached != sinks.end())
            {
                sink = cached->second;
                for (const auto node : path) sinks[node] = sink;
                return Status::Ok;
            }
            path.push_back(current);
            NodeCoord next;
            if (!LowestNeighbor(descriptor, current, next))
            {
                sink = current;
                for (const auto node : path) sinks[node] = sink;
                return Status::Ok;
            }
            current = next;
        }
        return Status::BasinTooLarge;
    };
    for (int y = 0; y < NodesPerRegionSide; ++y)
        for (int x = 0; x < NodesPerRegionSide; ++x)
        {
            auto& node = result.nodes[y * NodesPerRegionSide + x];
            const NodeCoord coordinate{originX + x, originY + y};
            NodeCoord sink;
            const auto sinkStatus = findSink(coordinate, sink);
            if (sinkStatus != Status::Ok) return sinkStatus;
            auto basin = basins.find(sink);
            if (basin == basins.end())
            {
                Basin resolved;
                const auto basinStatus = ResolveBasin(descriptor, sink, resolved);
                if (basinStatus != Status::Ok) return basinStatus;
                basin = basins.emplace(sink, std::move(resolved)).first;
            }

            node.relief = Relief(descriptor, coordinate);
            node.waterSurfaceMm = node.relief.elevationMm;
            NodeCoord direct;
            const bool hasDirect = LowestNeighbor(descriptor, coordinate, direct);
            if (Contains(basin->second.members, coordinate))
            {
                node.inLake = true;
                node.lakeId = basin->second.lake.id;
                node.waterSurfaceMm = basin->second.lake.surfaceMm;
                node.lakeOutlet = coordinate == basin->second.lake.outlet.upstream;
                node.downstream = node.lakeOutlet ? basin->second.lake.outlet.downstream :
                    (hasDirect ? direct : sink);
            }
            else
            {
                node.downstream = hasDirect ? direct : basin->second.lake.outlet.downstream;
            }
            if (node.inLake)
            {
                const auto found = std::find_if(result.lakes.begin(), result.lakes.end(),
                    [&](const LakeDescriptor& lake) { return lake.id == node.lakeId; });
                if (found == result.lakes.end())
                    result.lakes.push_back(basin->second.lake);
            }
        }

    // Accumulate every bounded upstream path once instead of retracing the same
    // candidates independently for all 1,024 owned nodes.
    for (std::int64_t sourceY = originY - UpstreamRadiusNodes;
        sourceY < originY + NodesPerRegionSide + UpstreamRadiusNodes; ++sourceY)
        for (std::int64_t sourceX = originX - UpstreamRadiusNodes;
            sourceX < originX + NodesPerRegionSide + UpstreamRadiusNodes; ++sourceX)
        {
            const NodeCoord source{sourceX, sourceY};
            auto current = source;
            for (int step = 0; step < UpstreamRadiusNodes * 2; ++step)
            {
                NodeCoord next;
                if (!LowestNeighbor(descriptor, current, next)) break;
                if (next.x >= originX && next.x < originX + NodesPerRegionSide &&
                    next.y >= originY && next.y < originY + NodesPerRegionSide &&
                    std::abs(source.x - next.x) <= UpstreamRadiusNodes &&
                    std::abs(source.y - next.y) <= UpstreamRadiusNodes)
                {
                    auto& target = result.nodes[
                        static_cast<int>(next.y - originY) * NodesPerRegionSide +
                        static_cast<int>(next.x - originX)];
                    if (target.accumulation < 65535) ++target.accumulation;
                }
                current = next;
            }
        }
    for (auto& node : result.nodes)
    {
        SetFlowClasses(node);
        if (node.accumulation >= 4 && (!node.inLake || node.lakeOutlet))
        {
            const auto downstreamElevation = Elevation(descriptor, node.downstream);
            result.reaches.push_back({{node.relief.node, node.downstream},
                node.waterSurfaceMm, downstreamElevation, node.accumulation,
                node.widthClass, node.depthClass});
        }
    }
    output = std::move(result);
    return Status::Ok;
}
}
