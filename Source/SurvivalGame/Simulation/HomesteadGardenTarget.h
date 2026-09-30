#pragma once

#include "HomesteadSimulation.h"

#include <string>

// Which garden square a hoe stroke or a pail pour would act on, and whether it would succeed, without
// changing anything. The controller acts on exactly this square and the world outlines it beforehand, so
// the outline never shows a different square from the one the action takes.
namespace Homestead
{
namespace GardenReach
{
// The hoe's blade bites about 85 cm ahead of her; the pail's reach lands 60 cm ahead (a plot there takes
// the focus over nearer things; see AHomesteadController::UpdateFocus).
constexpr double HoeAheadCm = 85.0;
constexpr double PailAheadCm = 60.0;
}

enum class GardenTool
{
    None,
    Hoe,    // tills a fresh square, or weeds one already tilled
    Pail,   // waters the focused plot
};

struct GardenTarget
{
    bool shown = false;       // false: nothing to outline (no tool, no plot for the pail, ...)
    int cellX = 0, cellY = 0; // garden coordinates (GardenCell)
    int plotId = -1;          // the existing plot there, if any
    bool valid = false;       // the action would succeed now
    std::string reason;       // the refusal when it wouldn't (empty when valid)
};

// The square the hoe acts on from `player` facing (forwardX, forwardY) (unit vector).
void HoeCellAhead(Point player, double forwardX, double forwardY, int& cellX, int& cellY);

// Hoe: the square ahead, a till when it's untilled and a weeding when it's a plot.
// Pail: the plot `focusPlotId` (the controller's focus), or nothing when there is none.
GardenTarget PreviewGarden(const Simulation& sim, GardenTool tool, Point player, double forwardX, double forwardY,
    int focusPlotId = -1);
}
