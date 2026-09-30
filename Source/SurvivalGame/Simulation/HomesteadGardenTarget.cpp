#include "HomesteadGardenTarget.h"

namespace Homestead
{
void HoeCellAhead(Point player, double forwardX, double forwardY, int& cellX, int& cellY)
{
    cellX = GardenCell(player.x + forwardX * GardenReach::HoeAheadCm);
    cellY = GardenCell(player.y + forwardY * GardenReach::HoeAheadCm);
}

GardenTarget PreviewGarden(const Simulation& sim, GardenTool tool, Point player, double forwardX, double forwardY,
    int focusPlotId)
{
    GardenTarget target;
    if (tool == GardenTool::Hoe)
    {
        HoeCellAhead(player, forwardX, forwardY, target.cellX, target.cellY);
        for (const Plot& plot : sim.GetState().plots)
            if (plot.cellX == target.cellX && plot.cellY == target.cellY) target.plotId = plot.id;
        const Result check = target.plotId >= 0 ? sim.CheckWeed(target.plotId, player)
                                                : sim.CheckTill(target.cellX, target.cellY, player);
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
    return target;
}
}
