#include "HomesteadCrops.h"

#include <algorithm>
#include <cmath>
#include <ostream>
#include <istream>
#include <vector>

namespace Homestead
{
namespace
{
template <typename T> bool CropValidEnum(T value, T count)
{
    return static_cast<int>(value) >= 0 && static_cast<int>(value) < static_cast<int>(count);
}

// Season masks for the table (design §4 crop table).
constexpr SeasonMask CropSeasonsSpringOnly = SeasonBit(Season::Spring);
constexpr SeasonMask CropSeasonsSpringSummer = SeasonBit(Season::Spring) | SeasonBit(Season::Summer);
constexpr SeasonMask CropSeasonsAutumnWinter = SeasonBit(Season::Autumn) | SeasonBit(Season::Winter);
// Wild-sown legacy crops grow wherever wild roots and berries do.
constexpr SeasonMask CropSeasonsSpringToAutumn = CropSeasonsSpringSummer | SeasonBit(Season::Autumn);

// One row per CropKind, in enum order. Growing times are game hours at full care; the new period
// crops follow Coral Island / Stardew pacing (a handful of days), not real gardening time.
const CropInfo CropTable[] = {
    // Legacy crops from the woodland prototype, kept with their original timings and yields.
    {CropKind::Roots, "Roots", "roots", Item::Seeds, Item::Roots, 4, Item::Seeds, 2, 30.0, 0.0,
        HarvestStyle::Pull, "CropCarrot", CropSeasonsSpringToAutumn},
    {CropKind::Berries, "Berries", "berries", Item::Berries, Item::Berries, 6, Item::Count, 0, 42.0, 24.0,
        HarvestStyle::Pick, "CropStrawberry", CropSeasonsSpringToAutumn},
    // Period crops sold as seed at the general store (improve-crops-and-harvest design table).
    {CropKind::Turnips, "Turnips", "turnips", Item::TurnipSeed, Item::Turnip, 2, Item::Count, 0, 96.0, 0.0,
        HarvestStyle::Pull, "CropTurnip", CropSeasonsAutumnWinter},
    {CropKind::Carrots, "Carrots", "carrots", Item::CarrotSeed, Item::Carrot, 3, Item::Count, 0, 120.0, 0.0,
        HarvestStyle::Pull, "CropCarrot", CropSeasonsSpringSummer},
    {CropKind::Potatoes, "Potatoes", "potatoes", Item::SeedPotato, Item::Potato, 4, Item::Count, 0, 144.0, 0.0,
        HarvestStyle::Pull, "CropPotato", CropSeasonsSpringOnly},
    {CropKind::Cabbage, "Cabbage", "cabbage", Item::CabbageSeed, Item::Cabbage, 1, Item::Count, 0, 216.0, 0.0,
        HarvestStyle::Cut, "CropCabbage", CropSeasonsAutumnWinter},
    {CropKind::BroadBeans, "Broad beans", "broad beans", Item::BroadBeanSeed, Item::BroadBeans, 6, Item::Count, 0, 168.0, 72.0,
        HarvestStyle::Pick, "CropBroadBean", CropSeasonsSpringOnly},
    {CropKind::Strawberries, "Strawberries", "strawberries", Item::StrawberryRunner, Item::Strawberries, 5, Item::Count, 0,
        192.0, 72.0, HarvestStyle::Pick, "CropStrawberry", CropSeasonsSpringSummer},
};
static_assert(sizeof(CropTable) / sizeof(CropTable[0]) == static_cast<int>(CropKind::Count),
    "Every CropKind needs exactly one CropTable row.");

bool CropTableInOrder()
{
    for (int i = 0; i < static_cast<int>(CropKind::Count); ++i)
        if (static_cast<int>(CropTable[i].kind) != i) return false;
    return true;
}

const CropInfo UnknownCropInfo{CropKind::Count, "Unknown crop", "unknown crop"};

int CropDaysFor(double hours) { return hours <= 0.0 ? 0 : std::max(1, static_cast<int>(std::ceil(hours / 24.0 - 1e-9))); }
}

const CropInfo& GetCropInfo(CropKind kind)
{
    static const bool ordered = CropTableInOrder();
    if (!ordered || !CropValidEnum(kind, CropKind::Count)) return UnknownCropInfo;
    return CropTable[static_cast<int>(kind)];
}

const CropInfo* CropForSeed(Item seed)
{
    for (const auto& info : CropTable)
        if (info.seed == seed) return &info;
    return nullptr;
}

int CropDays(CropKind kind) { return CropDaysFor(GetCropInfo(kind).growHours); }
int CropRegrowDays(CropKind kind) { return CropDaysFor(GetCropInfo(kind).regrowHours); }

double MoistureGrowthFactor(double moisture)
{
    if (moisture >= CropCare::WellWatered) return 1.0;
    const double wet = std::max(0.0, moisture) / CropCare::WellWatered;
    return CropCare::DryFloor + (1.0 - CropCare::DryFloor) * wet;
}

double WeedGrowthFactor(double weeds)
{
    const double over = std::max(0.0, weeds - CropCare::WeedyFrom) / (1.0 - CropCare::WeedyFrom);
    return 1.0 - CropCare::WeedPenalty * std::min(1.0, over);
}

CropStage StageOf(const Plot& plot)
{
    if (!plot.planted) return CropStage::Bare;
    if (plot.withered) return CropStage::Withered;
    if (plot.growth >= 1.0) return CropStage::Ripe;
    // A picked plant (beans, strawberries) restarts part-way up this scale, so it shows the stage
    // before its pods or fruit form again.
    if (plot.growth < 0.06) return CropStage::Sown;
    if (plot.growth < 0.30) return CropStage::Sprout;
    if (plot.growth < 0.55) return CropStage::Young;
    if (plot.growth < 0.80) return CropStage::Growing;
    return CropStage::Mature;
}

const char* StageName(CropStage stage)
{
    static const char* names[] = {"Bare", "Sown", "Sprout", "Young", "Growing", "Mature", "Ripe", "Withered"};
    return CropValidEnum(stage, CropStage::Count) ? names[static_cast<int>(stage)] : "Bare";
}

int CropDay(const Plot& plot)
{
    const int days = CropDays(plot.kind);
    if (days <= 0) return 0;
    return std::clamp(static_cast<int>(std::floor(plot.growth * days)) + 1, 1, days);
}

std::string ReadyInText(CropKind kind)
{
    const int days = CropDays(kind);
    return "Ready in about " + std::to_string(days) + (days == 1 ? " day" : " days") + " if watered.";
}

bool GrowsIn(CropKind kind, Season season) { return InSeason(GetCropInfo(kind).seasons, season); }

std::string OutOfSeasonText(CropKind kind)
{
    const auto& info = GetCropInfo(kind);
    // "Turnips grow", "Cabbage grows": the plural names end in s.
    const std::string name = info.name;
    const bool plural = !name.empty() && name.back() == 's';
    return name + (plural ? " grow in " : " grows in ") + Calendar::SeasonList(info.seasons) + ".";
}

double HoursLeftInSeasons(CropKind kind, double hour)
{
    const auto today = Calendar::DateAt(hour);
    const int days = Calendar::DaysLeftInRun(today, GetCropInfo(kind).seasons);
    if (days <= 0) return 0.0;
    // The run ends at the 06:00 rollover after its last day.
    const double end = (static_cast<double>(today.dayIndex) + days) * 24.0 + Calendar::DayStartHour;
    return std::max(0.0, end - hour);
}

std::string TooLateText(CropKind kind, double hoursToRipe, double hour)
{
    const auto& info = GetCropInfo(kind);
    if ((info.seasons & AllSeasons) == AllSeasons || hoursToRipe <= 0.0) return {};
    const double left = HoursLeftInSeasons(kind, hour);
    if (left <= 0.0 || hoursToRipe <= left + 1e-9) return {};
    const auto today = Calendar::DateAt(hour);
    const auto last = Calendar::DateOfDay(today.dayIndex + Calendar::DaysLeftInRun(today, info.seasons) - 1);
    return std::string("Won't ripen before ") + Calendar::SeasonName(last.season) + " ends.";
}

std::string PlotStatus(const Plot& plot, double hour)
{
    if (!plot.planted) return "Tilled soil: ready to plant";
    const auto& info = GetCropInfo(plot.kind);
    std::string text = info.name;
    if (plot.withered) return text + ": withered. Clear it with the hoe";
    if (IsRipe(plot)) return text + ": ready to harvest";
    const int regrowDays = CropRegrowDays(plot.kind);
    if (plot.picked && regrowDays > 0)
    {
        // Count the regrowth from where picking left the plant, matching "more will ripen in about N days".
        const double start = std::max(0.0, 1.0 - info.regrowHours / info.growHours);
        const double through = start < 1.0 ? (plot.growth - start) / (1.0 - start) : 1.0;
        const int day = std::clamp(static_cast<int>(std::floor(through * regrowDays)) + 1, 1, regrowDays);
        text += ": ripening again, day " + std::to_string(day) + " of " + std::to_string(regrowDays);
    }
    else
        text += ": day " + std::to_string(CropDay(plot)) + " of " + std::to_string(CropDays(plot.kind));
    const bool dry = NeedsWater(plot), weedy = IsWeedy(plot);
    if (dry && weedy) text += "  |  needs water and weeding, growing slowly";
    else if (dry) text += "  |  needs water, growing slowly";
    else if (weedy) text += "  |  weedy, growing slowly";
    if (hour >= 0.0)
    {
        const auto late = TooLateText(plot.kind, (1.0 - plot.growth) * info.growHours, hour);
        if (!late.empty()) text += "  |  " + late;
    }
    return text;
}

namespace Crops
{
void WriteSaveSection(std::ostream& output, const State& state)
{
    std::vector<int> picked;
    for (const auto& plot : state.plots) if (plot.planted && plot.picked) picked.push_back(plot.id);
    if (picked.empty()) return;
    output << SaveTag << ' ' << picked.size();
    for (int id : picked) output << ' ' << id;
    output << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    int count = 0;
    if (!(input >> count) || count < 0 || count > static_cast<int>(state.plots.size())) return false;
    for (int i = 0; i < count; ++i)
    {
        int id = 0;
        if (!(input >> id)) return false;
        const auto plot = std::find_if(state.plots.begin(), state.plots.end(), [id](const Plot& p) { return p.id == id; });
        if (plot == state.plots.end() || !plot->planted || plot->picked || GetCropInfo(plot->kind).regrowHours <= 0.0)
            return false;
        plot->picked = true;
    }
    return true;
}

void WriteWitheredSection(std::ostream& output, const State& state)
{
    std::vector<int> withered;
    for (const auto& plot : state.plots) if (plot.planted && plot.withered) withered.push_back(plot.id);
    if (withered.empty()) return;
    output << WitheredSaveTag << ' ' << withered.size();
    for (int id : withered) output << ' ' << id;
    output << '\n';
}

bool ReadWitheredSection(std::istream& input, State& state)
{
    int count = 0;
    if (!(input >> count) || count < 0 || count > static_cast<int>(state.plots.size())) return false;
    for (int i = 0; i < count; ++i)
    {
        int id = 0;
        if (!(input >> id)) return false;
        const auto plot = std::find_if(state.plots.begin(), state.plots.end(), [id](const Plot& p) { return p.id == id; });
        if (plot == state.plots.end() || !plot->planted || plot->withered) return false;
        plot->withered = true;
    }
    return true;
}

int WitherOutOfSeason(State& state, Season season)
{
    int count = 0;
    for (auto& plot : state.plots)
    {
        if (!plot.planted || plot.withered || GrowsIn(plot.kind, season)) continue;
        plot.withered = true;
        plot.picked = false;
        ++count;
    }
    return count;
}
}
}
