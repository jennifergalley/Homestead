#include "HomesteadRegionalGeneration.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <set>
#include <vector>

using namespace Homestead::RegionalGeneration;

namespace
{
int checks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

bool Same(const ReliefSample& a, const ReliefSample& b)
{
    return a.node == b.node && a.elevationMm == b.elevationMm &&
        a.mountain == b.mountain && a.ridge == b.ridge &&
        a.valley == b.valley && a.wetness == b.wetness;
}
bool Same(const DrainageNode& a, const DrainageNode& b)
{
    return Same(a.relief, b.relief) && a.downstream == b.downstream &&
        a.lakeId == b.lakeId && a.waterSurfaceMm == b.waterSurfaceMm &&
        a.accumulation == b.accumulation && a.widthClass == b.widthClass &&
        a.depthClass == b.depthClass && a.inLake == b.inLake &&
        a.lakeOutlet == b.lakeOutlet;
}
bool Same(const RegionalResult& a, const RegionalResult& b)
{
    if (a.region != b.region || a.lakes.size() != b.lakes.size() ||
        a.reaches.size() != b.reaches.size() ||
        a.maximumBasinSearch != b.maximumBasinSearch)
        return false;
    for (int index = 0; index < NodesPerRegion; ++index)
        if (!Same(a.nodes[index], b.nodes[index])) return false;
    for (std::size_t index = 0; index < a.lakes.size(); ++index)
    {
        const auto& x = a.lakes[index];
        const auto& y = b.lakes[index];
        if (x.id != y.id || !(x.outlet == y.outlet) ||
            x.surfaceMm != y.surfaceMm || x.memberCount != y.memberCount)
            return false;
    }
    for (std::size_t index = 0; index < a.reaches.size(); ++index)
    {
        const auto& x = a.reaches[index];
        const auto& y = b.reaches[index];
        if (!(x.key == y.key) || x.upstreamSurfaceMm != y.upstreamSurfaceMm ||
            x.downstreamSurfaceMm != y.downstreamSurfaceMm ||
            x.accumulation != y.accumulation || x.widthClass != y.widthClass ||
            x.depthClass != y.depthClass)
            return false;
    }
    return true;
}

void OwnershipAndFailures()
{
    RegionCoord region{7, 9};
    CHECK(RegionAtCm(-1, -RegionSizeCm - 1, region) == Status::Ok);
    CHECK((region == RegionCoord{-1, -2}));
    CHECK(RegionAtCm(-RegionSizeCm, RegionSizeCm - 1, region) == Status::Ok);
    CHECK((region == RegionCoord{-1, 0}));
    CHECK(RegionAtCm(RegionSizeCm, 0, region) == Status::Ok);
    CHECK((region == RegionCoord{1, 0}));
    const auto before = region;
    CHECK(RegionAtCm(std::numeric_limits<std::int64_t>::min(), 0, region) ==
        Status::OutOfRange);
    CHECK(region == before);

    ReliefSample relief;
    relief.elevationMm = 12345;
    CHECK(SampleRelief({1, 0}, {}, relief) == Status::UnsupportedVersion);
    CHECK(relief.elevationMm == 12345);
    constexpr std::int64_t highestNode =
        (static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max()) + 1) *
        NodesPerRegionSide - 1;
    CHECK(SampleRelief({}, {highestNode, highestNode}, relief) == Status::Ok);
    CHECK(SampleRelief({}, {highestNode + 1, 0}, relief) == Status::OutOfRange);

    DrainageNode node;
    node.waterSurfaceMm = 6789;
    CHECK(ResolveDrainage({1, 2}, {}, node) == Status::UnsupportedVersion);
    CHECK(node.waterSurfaceMm == 6789);
    RegionalResult result;
    result.region = {12, 34};
    CHECK(GenerateRegion({1, 2}, {}, result) == Status::UnsupportedVersion);
    CHECK((result.region == RegionCoord{12, 34}));
}

void ReliefAndOrder()
{
    for (const auto seed : {UINT64_C(0), UINT64_C(1), UINT64_C(817391),
        UINT64_C(0x100000000), UINT64_C(0xffffffffffffffff)})
    {
        const RegionalDescriptor descriptor{seed, RegionalGenerationVersion};
        for (const NodeCoord node : {NodeCoord{-33, -32}, NodeCoord{-32, -1},
            NodeCoord{-1, 0}, NodeCoord{0, 0}, NodeCoord{31, 32},
            NodeCoord{32768, -32769}})
        {
            ReliefSample first, second;
            CHECK(SampleRelief(descriptor, node, first) == Status::Ok);
            CHECK(SampleRelief(descriptor, node, second) == Status::Ok);
            CHECK(Same(first, second));
        }
    }
    ReliefSample a, b;
    CHECK(SampleRelief({817391, 1}, {-1, 0}, a) == Status::Ok);
    CHECK(SampleRelief({817392, 1}, {-1, 0}, b) == Status::Ok);
    CHECK(!Same(a, b));

}

void DrainageContinuity()
{
    const RegionalDescriptor descriptor{817391, 1};
    int crossings = 0;
    int tributaries = 0;
    bool belowReachThreshold = false;
    std::array<bool, 5> widthClasses{};
    std::array<bool, 4> depthClasses{};
    RegionalResult firstRight;
    CHECK(GenerateRegion(descriptor, {3, -6}, firstRight) == Status::Ok);
    RegionalResult left;
    CHECK(GenerateRegion(descriptor, {2, -6}, left) == Status::Ok);
    const RegionalResult* results[] = {&left, &firstRight};
    for (const auto* resultPointer : results)
    {
        const auto& result = *resultPointer;
        const auto region = result.region;
        CHECK(result.maximumBasinSearch == MaximumBasinNodes);
        CHECK(result.nodes.size() == NodesPerRegion);
        CHECK(result.reaches.size() <= NodesPerRegion);
        CHECK(result.lakes.size() <= NodesPerRegion);
        const auto lowX = static_cast<std::int64_t>(region.x) * NodesPerRegionSide;
        const auto lowY = static_cast<std::int64_t>(region.y) * NodesPerRegionSide;
        const auto highX = lowX + NodesPerRegionSide;
        const auto highY = lowY + NodesPerRegionSide;

        std::set<NodeCoord> reachKeys;
        for (const auto& reach : result.reaches)
        {
            CHECK(reachKeys.insert(reach.key.upstream).second);
            CHECK(reach.key.upstream != reach.key.downstream);
            CHECK(reach.downstreamSurfaceMm < reach.upstreamSurfaceMm);
            CHECK(reach.widthClass <= 4);
            CHECK(reach.depthClass <= 3);
            DrainageNode reconstructed;
            CHECK(ResolveDrainage(descriptor, reach.key.upstream, reconstructed) == Status::Ok);
            CHECK(reconstructed.downstream == reach.key.downstream);
            CHECK(reconstructed.accumulation == reach.accumulation);
            if (reach.key.downstream.x < lowX || reach.key.downstream.x >= highX ||
                reach.key.downstream.y < lowY || reach.key.downstream.y >= highY)
                ++crossings;
        }
        for (const auto& lake : result.lakes)
        {
            CHECK(lake.memberCount > 0 && lake.memberCount <= MaximumBasinNodes);
            CHECK(lake.outlet.upstream != lake.outlet.downstream);
            ReliefSample downstream;
            CHECK(SampleRelief(descriptor, lake.outlet.downstream, downstream) == Status::Ok);
            CHECK(downstream.elevationMm < lake.surfaceMm);
        }
        for (int index = 0; index < NodesPerRegion; ++index)
        {
            const auto& node = result.nodes[index];
            if (node.inLake)
            {
                CHECK(node.waterSurfaceMm >= node.relief.elevationMm);
                const auto lake = std::find_if(result.lakes.begin(), result.lakes.end(),
                    [&](const LakeDescriptor& candidate) { return candidate.id == node.lakeId; });
                CHECK(lake != result.lakes.end());
                CHECK(node.waterSurfaceMm == lake->surfaceMm);
                if (node.lakeOutlet) CHECK(node.downstream == lake->outlet.downstream);
            }
            if (node.accumulation >= 8) ++tributaries;
            belowReachThreshold = belowReachThreshold || node.accumulation < 4;
            CHECK(node.widthClass == (node.accumulation >= 32 ? 4 :
                (node.accumulation >= 16 ? 3 : (node.accumulation >= 8 ? 2 :
                    (node.accumulation >= 4 ? 1 : 0)))));
            CHECK(node.depthClass == (node.accumulation >= 24 ? 3 :
                (node.accumulation >= 12 ? 2 : (node.accumulation >= 5 ? 1 : 0))));
            widthClasses[node.widthClass] = true;
            depthClasses[node.depthClass] = true;
            if (index == (region.x & 31) + (region.y & 31) * NodesPerRegionSide)
            {
                DrainageNode reconstructed;
                CHECK(ResolveDrainage(descriptor, node.relief.node, reconstructed) == Status::Ok);
                CHECK(Same(node, reconstructed));
            }
        }
    }
    CHECK(crossings > 0);
    CHECK(tributaries > 0);
    CHECK(belowReachThreshold);
    CHECK(std::all_of(widthClasses.begin(), widthClasses.end(), [](bool seen) { return seen; }));
    CHECK(std::all_of(depthClasses.begin(), depthClasses.end(), [](bool seen) { return seen; }));
    RegionalResult repeatedRight;
    CHECK(GenerateRegion(descriptor, {3, -6}, repeatedRight) == Status::Ok);
    CHECK(Same(firstRight, repeatedRight));
}

void SuccessfulLakeScenario()
{
    const RegionalDescriptor descriptor{817391, 1};
    RegionalResult result, repeated;
    CHECK(GenerateRegion(descriptor, {1, -8}, result) == Status::Ok);
    CHECK(GenerateRegion(descriptor, {1, -8}, repeated) == Status::Ok);
    CHECK(Same(result, repeated));
    CHECK(!result.lakes.empty());
    std::set<NodeCoord> represented;
    for (const auto& node : result.nodes)
        if (node.inLake)
        {
            represented.insert(node.lakeId);
            const auto lake = std::find_if(result.lakes.begin(), result.lakes.end(),
                [&](const LakeDescriptor& candidate) { return candidate.id == node.lakeId; });
            CHECK(lake != result.lakes.end());
            CHECK(node.waterSurfaceMm == lake->surfaceMm);
            CHECK(node.waterSurfaceMm >= node.relief.elevationMm);
            if (node.lakeOutlet)
            {
                CHECK(node.relief.node == lake->outlet.upstream);
                CHECK(node.downstream == lake->outlet.downstream);
            }
        }
    CHECK(represented.size() == result.lakes.size());
    for (const auto& lake : result.lakes)
    {
        CHECK(lake.memberCount > 0 && lake.memberCount <= MaximumBasinNodes);
        ReliefSample downstream;
        CHECK(SampleRelief(descriptor, lake.outlet.downstream, downstream) == Status::Ok);
        CHECK(downstream.elevationMm < lake.surfaceMm);
    }
}

void MultipleSeedBounds()
{
    DrainageNode unchanged;
    unchanged.waterSurfaceMm = 314159;
    CHECK(ResolveDrainage({817391, 1}, {32, 69}, unchanged) == Status::BasinTooLarge);
    CHECK(unchanged.waterSurfaceMm == 314159);
    RegionalResult oversized;
    oversized.region = {17, 23};
    CHECK(GenerateRegion({817391, 1}, {-2, -1}, oversized) == Status::BasinTooLarge);
    CHECK((oversized.region == RegionCoord{17, 23}));

    struct SeedRegion { std::uint64_t seed; RegionCoord region; };
    constexpr SeedRegion fixtures[] = {{0, {-6, 3}}};
    for (const auto fixture : fixtures)
    {
        const RegionalDescriptor descriptor{fixture.seed, 1};
        RegionalResult result;
        CHECK(GenerateRegion(descriptor, fixture.region, result) == Status::Ok);
        CHECK(result.nodes.size() == NodesPerRegion);
        CHECK(result.lakes.size() <= NodesPerRegion);
        CHECK(result.reaches.size() <= NodesPerRegion);
    }
}
}

int main()
{
    std::cout << "Regional phase: ownership\n" << std::flush;
    OwnershipAndFailures();
    std::cout << "Regional phase: relief\n" << std::flush;
    ReliefAndOrder();
    std::cout << "Regional phase: continuity\n" << std::flush;
    DrainageContinuity();
    std::cout << "Regional phase: lakes\n" << std::flush;
    SuccessfulLakeScenario();
    std::cout << "Regional phase: seeds\n" << std::flush;
    MultipleSeedBounds();
    std::cout << "Regional generation: " << checks << " checks passed.\n";
    return 0;
}
