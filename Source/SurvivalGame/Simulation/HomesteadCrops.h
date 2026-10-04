#pragma once

#include "HomesteadItems.h"
#include "HomesteadSimulation.h"

#include <iosfwd>
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
    int bonusCount = 0;        // maximum; cultivated seed bonuses use HarvestBonusCount
    // Hours of steady growth from sowing to ripe when the soil is kept watered and weeded.
    double growHours = 0.0;
    // Hours for a picked plant to ripen again; 0 when harvesting clears the plot.
    double regrowHours = 0.0;
    HarvestStyle style = HarvestStyle::Pull;
    // Stem of the plant meshes: /Game/SurvivalGame/Environment/Props/<visual>/SM_<visual>_<Stage>.
    const char* visual = "";
    // The seasons it grows in (design §4): planting outside them is refused, and a crop still in the
    // ground when they end withers at the season rollover.
    SeasonMask seasons = AllSeasons;
};

const CropInfo& GetCropInfo(CropKind kind);
// The crop sown from `seed`, or null when it isn't a seed.
const CropInfo* CropForSeed(Item seed);
// Whole days to ripen when watered (growHours rounded up to days).
int CropDays(CropKind kind);
int CropRegrowDays(CropKind kind);
// Seed bonuses are occasional, deterministic per plot/day; other bonus items retain their quantity.
int HarvestBonusCount(CropKind kind, int plotId, double hour);

// Growth-rate modifiers. Soil watered within the last day grows at full speed; drier soil and a
// plot more than half weedy grow slowly, never kill the crop.
namespace CropCare
{
constexpr double WellWatered = 0.4; // a full watering stays above this for a whole day
constexpr double DryFloor = 0.2;   // growth speed in bone-dry soil
constexpr double WeedyFrom = 0.5;  // weeds below this don't slow the crop (two daily passes)
constexpr double WeedPenalty = 0.7;
// The first weed tuft shows on a square from here (AHomesteadWorld draws one per eighth), so the
// [F]/[X] Pull weeds prompt appears exactly when she can see something to pull.
constexpr double VisibleWeeds = 0.125;
// Weeds come up once a day, never in between (Jenny, 2026-09-30: something to clear each morning, not
// twice a day): each pass adds this much to a plot, a little under the old 0.009 an hour (0.216 a day).
constexpr double DailyWeeds = 0.2;
// ...but only on some plots each day (Jenny, 2026-10-01: weeds shouldn't grow in every plot every day):
// each plot gets its own chance each day (Crops::WeedsComeUp).
constexpr double DailyWeedChance = 0.35;
}
double MoistureGrowthFactor(double moisture);
double WeedGrowthFactor(double weeds);
inline bool NeedsWater(const Plot& plot) { return plot.moisture < CropCare::WellWatered; }
// Shown on a dry plot's focus and returned by Water() when the pail she carries holds no water.
inline constexpr const char* EmptyPailText = "The pail is empty. Fill it at a body of water.";
// A full pail holds this many portions of water (Simulation::FillWater tops it up to this). Jenny, 2026-09-30: 15, up
// from 6 ("I don't want to be going to the lake that freaking often"). Saves keep their water; this is only the cap.
inline constexpr int PailPortions = 15;
inline bool IsWeedy(const Plot& plot) { return plot.weeds > CropCare::WeedyFrom; }
// Weeds she can see on the square, sown or bare, ripe or not: [F]/[X] offers to pull them.
inline bool HasVisibleWeeds(const Plot& plot) { return plot.weeds >= CropCare::VisibleWeeds; }
inline bool IsRipe(const Plot& plot) { return plot.planted && !plot.withered && plot.growth >= 1.0; }

// Crop seasons (design §4).
bool GrowsIn(CropKind kind, Season season);
// "Turnips grow in Autumn and Winter."
std::string OutOfSeasonText(CropKind kind);
// Game hours from `hour` until its crop's growing seasons end (the 06:00 rollover into the first
// season it doesn't grow in); 0 when out of season now.
double HoursLeftInSeasons(CropKind kind, double hour);
// "Won't ripen before Summer ends." when a crop needing `hoursToRipe` more hours at full care
// can't make it at `hour`; empty when it can.
std::string TooLateText(CropKind kind, double hoursToRipe, double hour);

// Visible growth stages. Sown is the soil mound; the rest map to the plant meshes. Withered is the
// dead plant left when the crop's seasons ended (Harvest lane supplies SM_CropWithered_* meshes).
enum class CropStage : int { Bare, Sown, Sprout, Young, Growing, Mature, Ripe, Withered, Count };
CropStage StageOf(const Plot& plot);
const char* StageName(CropStage stage); // "Sprout", ... (the mesh suffix)

// Day of growth she's on (1-based, capped at CropDays) for a growing crop.
int CropDay(const Plot& plot);
// The focus line for a plot: "Turnips: day 2 of 4  |  needs water, growing slowly",
// "Turnips: ready to harvest", "Tilled soil: ready to plant", "Potatoes: withered. Clear it with the
// hoe". With the hour (hour >= 0), a crop that can't ripen before its seasons end says so.
std::string PlotStatus(const Plot& plot, double hour = -1.0);
// "Ready in about 4 days if watered."
std::string ReadyInText(CropKind kind);

namespace Crops
{
// The calendar day the weeds keep, rolling over at the 6 AM day boundary (DayRolloverHour).
int WeedDay(double hour);
// Whether weeds come up on plot `plotId` at the 6 AM pass of calendar day `day` (WeedDay): a fixed hash of
// the two against DailyWeedChance, not a live random number, so a reload or a long sleep never rerolls a day.
bool WeedsComeUp(int plotId, int day);
// The daily weed pass for the `days` calendar days that just began (the last of them is WeedDay(state.hour)):
// DailyWeeds more on each plot whose weeds come up that day (WeedsComeUp), capped at fully weedy.
// Simulation::Step runs it when she is awake at 6 AM; Sleep and DozeOff run it on waking when the
// sleep crossed 6 AM. Each calendar day gets exactly one pass whichever way it comes, and nothing
// grows between passes, so a reload needs no record of when the last one ran.
void GrowDailyWeeds(State& state, int days);
// Optional trailing save section (tag "picked"): the ids of regrowing plots picked since sowing.
// Written only when there are some, so saves without it load every plot unpicked.
constexpr const char* SaveTag = "picked";
void WriteSaveSection(std::ostream& output, const State& state);
// Reads the section after its tag. Refuses ids that aren't planted regrowing plots, and repeats.
bool ReadSaveSection(std::istream& input, State& state);
// Optional trailing save section (tag "withered"): the ids of withered plots, count first. Written
// only when there are some; older saves load every plot living.
constexpr const char* WitheredSaveTag = "withered";
void WriteWitheredSection(std::ostream& output, const State& state);
bool ReadWitheredSection(std::istream& input, State& state);
// The season rollover into `season`: every planted crop that doesn't grow in it withers. Returns
// how many plots withered.
int WitherOutOfSeason(State& state, Season season);
}
}
