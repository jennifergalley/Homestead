#include "HomesteadGardenTarget.h"

#include "HomesteadCrops.h"
#include "HomesteadPackRow.h"
#include "../HomesteadActionHints.h"

namespace Homestead
{
void HoeCellAhead(Point player, double forwardX, double forwardY, int& cellX, int& cellY)
{
    cellX = GardenCell(player.x + forwardX * GardenReach::HoeAheadCm);
    cellY = GardenCell(player.y + forwardY * GardenReach::HoeAheadCm);
}

GardenTarget PreviewGarden(const Simulation& sim, GardenTool tool, Point player, double forwardX, double forwardY,
    int focusPlotId, Item seed)
{
    GardenTarget target;
    if (tool == GardenTool::Hoe)
    {
        // The same square the hoe takes (AHomesteadController::HoeSquareAhead): a focused withered crop
        // first, else the square ahead, where a withered crop is hoed out, a plot weeded, open ground tilled.
        for (const Plot& plot : sim.GetState().plots)
            if (plot.id == focusPlotId && plot.planted && plot.withered)
            {
                const Result check = sim.CheckClearWithered(plot.id, player);
                target.shown = true;
                target.cellX = plot.cellX;
                target.cellY = plot.cellY;
                target.plotId = plot.id;
                target.valid = check.ok;
                if (!check.ok) target.reason = check.message;
                return target;
            }
        HoeCellAhead(player, forwardX, forwardY, target.cellX, target.cellY);
        const Plot* ahead = nullptr;
        for (const Plot& plot : sim.GetState().plots)
            if (plot.cellX == target.cellX && plot.cellY == target.cellY) { target.plotId = plot.id; ahead = &plot; }
        const Result check = !ahead ? sim.CheckTill(target.cellX, target.cellY, player)
            : ahead->planted && ahead->withered ? sim.CheckClearWithered(ahead->id, player)
            : sim.CheckWeed(ahead->id, player);
        target.shown = true;
        target.valid = check.ok;
        if (!check.ok) target.reason = check.message;
        return target;
    }
    if (tool == GardenTool::Pail && focusPlotId >= 0)
        for (const Plot& plot : sim.GetState().plots)
            if (plot.id == focusPlotId)
            {
                const Result check = sim.CheckWater(plot.id, player);
                target.shown = true;
                target.cellX = plot.cellX;
                target.cellY = plot.cellY;
                target.plotId = plot.id;
                target.valid = check.ok;
                if (!check.ok) target.reason = check.message;
                return target;
            }
    const CropInfo* crop = tool == GardenTool::Seed ? CropForSeed(seed) : nullptr;
    if (!crop) return target;
    if (focusPlotId >= 0)
    {
        for (const Plot& plot : sim.GetState().plots)
            if (plot.id == focusPlotId)
            {
                const Result check = sim.CheckSow(plot.id, player, crop->kind);
                target.shown = true;
                target.cellX = plot.cellX;
                target.cellY = plot.cellY;
                target.plotId = plot.id;
                target.valid = check.ok;
                if (!check.ok) target.reason = check.message;
                return target;
            }
        return target;
    }
    if (crop->kind == CropKind::Berries) return target;
    // Out of its seasons, tilling first wouldn't help (Plant would still refuse): nothing to outline.
    if (!GrowsIn(crop->kind, sim.Today().season)) return target;
    // No plot in focus: the square the hoe would till next (HoeCellAhead) is untilled, so it's red until she
    // tills it, but only where it could be tilled (a missing hoe or tiredness aside); over a building,
    // resource, spoiling overgrowth or a plot, or out of reach, there's nothing to outline.
    HoeCellAhead(player, forwardX, forwardY, target.cellX, target.cellY);
    if (!sim.CheckTillGround(target.cellX, target.cellY, player)) return GardenTarget{};
    target.shown = true;
    target.reason = UntilledSowText;
    return target;
}

std::string SeedLabel(Item seed)
{
    return seed == Item::Berries ? std::string("berry seeds") : std::string(ItemName(seed));
}

std::string DescribeHarvest(const Plot& plot)
{
    if (!IsRipe(plot) || HasVisibleWeeds(plot)) return {};
    return std::string(HomesteadActionHints::Harvest) + " " + GetCropInfo(plot.kind).name;
}

SowCue DescribeSow(const Simulation& sim, int plotId, Point player, Item selected, const std::vector<Item>& row)
{
    if (const CropInfo* crop = selected == Item::Count ? nullptr : CropForSeed(selected))
    {
        const Result check = sim.CheckSow(plotId, player, crop->kind);
        return check.ok ? SowCue{true, HomesteadActionHints::PlantSeeds} : SowCue{false, check.message};
    }
    // Nothing is sown unselected, so name the seed to select and its number key: a real seed first, else a
    // berry (whose seeds also sow).
    int berryCell = -1;
    for (int cell = 0; cell < static_cast<int>(row.size()); ++cell)
    {
        const Item item = row[cell];
        if (item == Item::Count || !CropForSeed(item) || sim.Count(item) <= 0) continue;
        if (!GrowsIn(CropForSeed(item)->kind, sim.Today().season)) continue;  // out of season: not worth naming
        if (item == Item::Berries)
        {
            if (berryCell < 0) berryCell = cell;
            continue;
        }
        return {false, "Select " + SeedLabel(item) + " (" + std::to_string(PackRowRules::KeyNumber(cell)) + ") to plant"};
    }
    if (berryCell >= 0)
        return {false, "Select Berries (" + std::to_string(PackRowRules::KeyNumber(berryCell)) + ") to plant their seeds"};
    return {false, "Choose seeds on the hotbar to sow"};
}
}
