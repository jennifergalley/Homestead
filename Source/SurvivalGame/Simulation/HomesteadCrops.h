#pragma once

#include "HomesteadItems.h"
#include "HomesteadSimulation.h"

#include <string>

// Garden crops (improve-crops-and-harvest): what each crop is sown from, how many days it takes,
// what it yields and how she harvests it. The Simulation transactions (Plant, HarvestCrop, the
// growth tick) read this table; the HUD and the plot visuals read the status helpers.
namespace Homestead
{
enum class HarvestStyle : int
{
    Pull, // kneel and pull the root up (roots, turnips, carrots, potatoes)
    Cut,  // kneel and cut the head (cabbage)
    Pick, // pick from the standing plant, which stays planted (berries, beans, strawberries)
};

struct CropInfo
{
    CropKind kind = CropKind::Count;
    const char* name = "";     // "Turnips": the plot label and the focus title
    const char* lower = "";    // "turnips": used mid-sentence
    Item seed = Item::Count;   // what she sows (consumed one per plot)
    Item produce = Item::Count;
    int produceCount = 0;
    Item bonus = Item::Count;  // an extra yield, e.g. seed saved from wild roots
    int bonusCount = 0;
    // Hours of steady growth from sowing to ripe when the soil is kept watered and weeded.
    double growHours = 0.0;
    // Hours for a picked plant to ripen again; 0 when harvesting clears the plot.
    double regrowHours = 0.0;
    HarvestStyle style = HarvestStyle::Pull;
    // Stem of the plant meshes: /Game/SurvivalGame/Environment/Props/<visual>/SM_<visual>_<Stage>.
    const char* visual = "";
};

const CropInfo& GetCropInfo(CropKind kind);
// The crop sown from `seed`, or null when it isn't a seed.
const CropInfo* CropForSeed(Item seed);
// Whole days to ripen when watered (growHours rounded up to days).
int CropDays(CropKind kind);
int CropRegrowDays(CropKind kind);

// Growth-rate modifiers. Soil watered within the last day grows at full speed; drier soil and a
// plot more than half weedy grow slowly, never kill the crop.
namespace CropCare
{
constexpr double WellWatered = 0.4; // a full watering stays above this for a whole day
constexpr double DryFloor = 0.2;   // growth speed in bone-dry soil
constexpr double WeedyFrom = 0.5;  // weeds below this don't slow the crop (about two days' creep)
constexpr double WeedPenalty = 0.7;
}
double MoistureGrowthFactor(double moisture);
double WeedGrowthFactor(double weeds);
inline bool NeedsWater(const Plot& plot) { return plot.moisture < CropCare::WellWatered; }
inline bool IsWeedy(const Plot& plot) { return plot.weeds > CropCare::WeedyFrom; }
inline bool IsRipe(const Plot& plot) { return plot.planted && plot.growth >= 1.0; }

// Visible growth stages. Sown is the soil mound; the rest map to the plant meshes.
enum class CropStage : int { Bare, Sown, Sprout, Young, Growing, Mature, Ripe, Count };
CropStage StageOf(const Plot& plot);
const char* StageName(CropStage stage); // "Sprout", ... (the mesh suffix)

// Day of growth she's on (1-based, capped at CropDays) for a growing crop.
int CropDay(const Plot& plot);
// The focus line for a plot: "Turnips: day 2 of 4  |  needs water, growing slowly",
// "Turnips: ready to harvest", "Tilled soil: ready to plant".
std::string PlotStatus(const Plot& plot);
// "Ready in about 4 days if watered."
std::string ReadyInText(CropKind kind);
}
