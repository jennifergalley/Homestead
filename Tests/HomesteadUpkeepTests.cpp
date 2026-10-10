#include "HomesteadEstate.h"
#include "HomesteadSimulation.h"
#include "HomesteadUpkeep.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace
{
int UpkeepChecks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++UpkeepChecks;
    if (condition) return;
    std::cerr << "FAILED: " << expression << " (line " << line << ")\n";
    std::exit(1);
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

using namespace Homestead;

constexpr int YardWeeds = 40;
constexpr int FarWeeds = 20;
constexpr int Windfalls = 30;

EstatePlacements Table(Point yard)
{
    EstatePlacements placements;
    placements.bakeVersion = 9;
    int next = EstatePlacementIdBase + 30000;
    for (int i = 0; i < YardWeeds; ++i)
        placements.placements.push_back({next++, i % 3 ? ResourceKind::Weeds : ResourceKind::TallGrass,
            {yard.x + (i % 8) * 90.0, yard.y + (i / 8) * 90.0}, 0, 0, 1, 0});
    for (int i = 0; i < FarWeeds; ++i)
        placements.placements.push_back({next++, ResourceKind::Weeds, {yard.x + 24000 + i * 90.0, yard.y}, 0, 0, 1, 0});
    for (int i = 0; i < Windfalls; ++i)
        placements.placements.push_back({WindfallPlacementIdFirst + i, ResourceKind::FallenBranch,
            {yard.x + 6000 + (i % 6) * 250.0, yard.y - 6000 - (i / 6) * 250.0}, 0, 0, 1, 0});
    return placements;
}

int Standing(const Simulation& sim, bool windfall, bool far, Point yard)
{
    int count = 0;
    for (const auto& node : sim.GetState().resources)
    {
        if (node.cleared || IsWindfallPlacement(node.id) != windfall) continue;
        if (!windfall && (node.position.x > yard.x + 12000) != far) continue;
        ++count;
    }
    return count;
}

void UpkeepLoop()
{
    const Point spawn = ProvisionalEstateLayout().PointOr(Anchor::StandingRoomSpawn, {});
    const Point yard{spawn.x - 1500, spawn.y + 1500};
    const EstatePlacements placements = Table(yard);
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), placements));
    CHECK(sim.GrantItems(Item::Scythe, 1));
    CHECK(sim.GrantItems(Item::Billhook, 1));
    // The windfall spots start empty: no branch lies there until the wind drops one.
    CHECK(Standing(sim, true, false, yard) == 0);
    for (const auto& node : std::vector<ResourceNode>(sim.GetState().resources))
    {
        if (IsWindfallPlacement(node.id)) continue;
        CHECK(sim.SetEnergy(100.0));
        CHECK(sim.ClearOvergrowth(node.id, Item::Scythe, node.position));
    }
    CHECK(Standing(sim, false, false, yard) == 0 && Standing(sim, false, true, yard) == 0);

    int lying = 0, grownYard = 0;
    for (int day = 0; day < 40; ++day)
    {
        sim.AdvanceGameHours(24, yard);
        CHECK(!sim.GetState().failed);
        const int nowLying = Standing(sim, true, false, yard);
        CHECK(nowLying - lying <= Upkeep::MaxWindfallsPerDay && nowLying <= Upkeep::MaxWindfallsLying);
        lying = nowLying;
        const int nowYard = Standing(sim, false, false, yard);
        if (day == 0) CHECK(nowYard <= Upkeep::MaxRegrowthsPerDay);
        grownYard = nowYard;
        CHECK(Standing(sim, false, true, yard) == 0);
    }
    CHECK(grownYard >= 3);
    CHECK(lying >= 3 && lying <= Upkeep::MaxWindfallsLying);

    // A woken windfall is saved and loads back the same; picking the branch up leaves the spot to blow again.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(placements);
    CHECK(loaded.Deserialize(saved));
    CHECK(loaded.Serialize() == saved);
    CHECK(Standing(loaded, true, false, yard) == lying);
    int branchId = -1;
    for (const auto& node : loaded.GetState().resources)
        if (IsWindfallPlacement(node.id) && !node.cleared) { branchId = node.id; break; }
    CHECK(branchId != -1);
    const Point at{yard.x + 6000, yard.y - 6000};
    CHECK(loaded.SetEnergy(100.0));
    const int branches = loaded.Count(Item::Branch), kindling = loaded.Count(Item::Kindling);
    const double energy = loaded.GetState().energy;
    for (const auto& node : loaded.GetState().resources)
        if (node.id == branchId)
        {
            CHECK(loaded.ClearOvergrowth(branchId, Item::Count, node.position));
            break;
        }
    CHECK(Standing(loaded, true, false, yard) == lying - 1);
    CHECK(loaded.Count(Item::Branch) == branches + 3 && loaded.Count(Item::Kindling) == kindling + 1);
    CHECK(std::abs((energy - loaded.GetState().energy) - 0.8) < 1e-6);
    (void)at;
}

// Balance 2026-10-04: +40 berry bushes (thickets of 3-5), +30 root spots, +30 branch piles, +20 stone piles, and
// dormant windfall spots near home that start empty in a new game.
void WoodlandForageData()
{
    int bushes = 0, roots = 0, branches = 0, stones = 0, spots = 0;
    for (const auto& p : ProvisionalEstatePlacements().placements)
    {
        if (p.id >= 583000 && p.id < 583200)
        {
            bushes += p.kind == ResourceKind::BerryBush;
            roots += p.kind == ResourceKind::Roots;
            branches += p.kind == ResourceKind::Branches;
            stones += p.kind == ResourceKind::Stones;
        }
        if (IsWindfallPlacement(p.id))
        {
            CHECK(p.kind == ResourceKind::FallenBranch);
            ++spots;
        }
    }
    CHECK(bushes == 40 && roots == 30 && branches == 30 && stones == 20);
    CHECK(spots >= 100);
    Simulation sim;
    CHECK(sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    for (const auto& node : sim.GetState().resources)
        if (IsWindfallPlacement(node.id)) CHECK(node.cleared);
}
}

int main()
{
    UpkeepLoop();
    WoodlandForageData();
    std::cout << "HomesteadUpkeepTests passed (" << UpkeepChecks << " checks)\n";
    return 0;
}
