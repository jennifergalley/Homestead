#include "HomesteadCrops.h"

#include <algorithm>
#include <cmath>

namespace Homestead
{
namespace
{
template <typename T> bool ValidEnum(T value, T count)
{
    return static_cast<int>(value) >= 0 && static_cast<int>(value) < static_cast<int>(count);
}

// One row per CropKind, in enum order. Growing times are game hours at full care; the new period
// crops follow Coral Island / Stardew pacing (a handful of days), not real gardening time.
const CropInfo CropTable[] = {
    // Legacy crops from the woodland prototype, kept with their original timings and yields.
    {CropKind::Roots, "Roots", "roots", Item::Seeds, Item::Roots, 4, Item::Seeds, 2, 30.0, 0.0,
        HarvestStyle::Pull, "CropCarrot"},
    {CropKind::Berries, "Berries", "berries", Item::Berries, Item::Berries, 6, Item::Count, 0, 42.0, 24.0,
        HarvestStyle::Pick, "CropStrawberry"},
};
static_assert(sizeof(CropTable) / sizeof(CropTable[0]) == static_cast<int>(CropKind::Count),
    "Every CropKind needs exactly one CropTable row.");

bool TableInOrder()
{
    for (int i = 0; i < static_cast<int>(CropKind::Count); ++i)
        if (static_cast<int>(CropTable[i].kind) != i) return false;
    return true;
}

const CropInfo UnknownCrop{CropKind::Count, "Unknown crop", "unknown crop"};

int DaysFor(double hours) { return hours <= 0.0 ? 0 : std::max(1, static_cast<int>(std::ceil(hours / 24.0 - 1e-9))); }
}

const CropInfo& GetCropInfo(CropKind kind)
{
    static const bool ordered = TableInOrder();
    if (!ordered || !ValidEnum(kind, CropKind::Count)) return UnknownCrop;
    return CropTable[static_cast<int>(kind)];
}

const CropInfo* CropForSeed(Item seed)
{
    for (const auto& info : CropTable)
        if (info.seed == seed) return &info;
    return nullptr;
}

int CropDays(CropKind kind) { return DaysFor(GetCropInfo(kind).growHours); }
int CropRegrowDays(CropKind kind) { return DaysFor(GetCropInfo(kind).regrowHours); }

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
    if (plot.growth >= 1.0) return CropStage::Ripe;
    // A picked plant that ripens again stays leafy (Mature) rather than shrinking back.
    const auto& info = GetCropInfo(plot.kind);
    const bool regrowing = info.regrowHours > 0.0 && info.regrowHours < info.growHours
        && plot.growth >= 1.0 - info.regrowHours / info.growHours;
    if (regrowing) return CropStage::Mature;
    if (plot.growth < 0.06) return CropStage::Sown;
    if (plot.growth < 0.30) return CropStage::Sprout;
    if (plot.growth < 0.55) return CropStage::Young;
    if (plot.growth < 0.80) return CropStage::Growing;
    return CropStage::Mature;
}

const char* StageName(CropStage stage)
{
    static const char* names[] = {"Bare", "Sown", "Sprout", "Young", "Growing", "Mature", "Ripe"};
    return ValidEnum(stage, CropStage::Count) ? names[static_cast<int>(stage)] : "Bare";
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

std::string PlotStatus(const Plot& plot)
{
    if (!plot.planted) return "Tilled soil: ready to plant";
    const auto& info = GetCropInfo(plot.kind);
    std::string text = info.name;
    if (IsRipe(plot)) return text + ": ready to harvest";
    const double regrowStart = info.regrowHours > 0.0 && info.regrowHours < info.growHours
        ? 1.0 - info.regrowHours / info.growHours : 2.0;
    if (plot.growth >= regrowStart)
    {
        const int days = CropRegrowDays(plot.kind);
        const int day = std::clamp(static_cast<int>(std::floor((plot.growth - regrowStart) / (1.0 - regrowStart) * days)) + 1, 1, days);
        text += ": ripening again, day " + std::to_string(day) + " of " + std::to_string(days);
    }
    else
        text += ": day " + std::to_string(CropDay(plot)) + " of " + std::to_string(CropDays(plot.kind));
    const bool dry = NeedsWater(plot), weedy = IsWeedy(plot);
    if (dry && weedy) text += "  |  needs water and weeding, growing slowly";
    else if (dry) text += "  |  needs water, growing slowly";
    else if (weedy) text += "  |  weedy, growing slowly";
    return text;
}
}
