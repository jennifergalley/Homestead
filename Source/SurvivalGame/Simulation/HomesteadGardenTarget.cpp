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
    return target;
}
}
