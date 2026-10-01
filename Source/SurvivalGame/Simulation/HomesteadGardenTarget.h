#pragma once

#include "HomesteadSimulation.h"

#include <string>
#include <vector>

// Which garden square a hoe stroke, a pail pour or a sowing would act on, and whether it would succeed, without
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
    Seed,   // sows the selected seed into the focused plot ([A]/[E], PlantFocusedPlot)
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

// Hoe: a focused withered crop (`focusPlotId`), else the square ahead: hoeing out a withered crop there,
// weeding a plot, tilling open ground. Pail: the plot `focusPlotId` (the controller's focus), or nothing.
// Seed: the chosen seed or berry sown into a focused plot, else a preview of the untended square ahead.
GardenTarget PreviewGarden(const Simulation& sim, GardenTool tool, Point player, double forwardX, double forwardY,
    int focusPlotId = -1, Item seed = Item::Count);

constexpr const char* UntilledSowText = "Till this square before sowing.";

// The sowing cue on a bare tilled plot's focus line (the [A]/[E] press, which sows only the seed selected
// on the hotbar). `keyed` cues take the press's key glyph and retire after a few uses; plain ones are
// guidance that always shows.
// - A seed selected, and the press would sow: "Plant <seed>" (keyed).
// - A seed selected, and it wouldn't: CheckSow's refusal (plain).
// - No seed selected, but one in the hotbar row she has and that's in season: "Select <seed> (<key>) to plant" (plain).
// - Otherwise: "Choose seeds on the hotbar to sow" (plain).
// `row` holds the hotbar's items by cell (Item::Count for an empty cell or a garment).
struct SowCue
{
    bool keyed = false;
    std::string text;
};
SowCue DescribeSow(const Simulation& sim, int plotId, Point player, Item selected, const std::vector<Item>& row);

// "Turnip seed", "Seed potato", ...; a berry sows "berry seeds".
std::string SeedLabel(Item seed);
}
