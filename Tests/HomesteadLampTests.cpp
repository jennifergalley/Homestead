// Portable tests for the oil lamp (add-oil-lamp).
#include "HomesteadEstate.h"
#include "HomesteadItems.h"
#include "HomesteadLamp.h"
#include "HomesteadShops.h"
#include "HomesteadSimulation.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>

using namespace Homestead;

namespace
{
int checks = 0;
int cases = 0;
void Check(bool condition, const char* expression, int line)
{
    ++checks;
    if (!condition)
    {
        std::cerr << "FAIL line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

void Okay(const Result& result, int line)
{
    ++checks;
    if (!result.ok)
    {
        std::cerr << "FAIL line " << line << ": " << result.message << '\n';
        std::exit(1);
    }
}
#define OK(expression) Okay(expression, __LINE__)

bool Near(double a, double b) { return std::abs(a - b) < 1e-6; }

// A save's header with `payload` in place of its own.
std::string Reseal(const std::string& saved, const std::string& payload)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : payload) { hash ^= c; hash *= UINT64_C(1099511628211); }
    std::istringstream header(saved.substr(0, saved.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(hash) + "\n" + payload;
}

// The save with its "lamp" line replaced by `line` (empty to drop the section).
std::string WithLampLine(const std::string& saved, const std::string& line)
{
    const auto newline = saved.find('\n');
    std::string payload = saved.substr(newline + 1);
    const auto start = payload.find("\nlamp ");
    CHECK(start != std::string::npos);
    const auto end = payload.find('\n', start + 1);
    payload = payload.substr(0, start + 1) + line + payload.substr(end + 1);
    return Reseal(saved, payload);
}

struct Estate
{
    Simulation sim;
    Point home{};
};

Estate NewEstate()
{
    Estate estate;
    OK(estate.sim.NewEstateGame(ProvisionalEstateLayout(), ProvisionalEstatePlacements()));
    // Out in the town square: dry, open ground away from the manor's structures.
    estate.home = ProvisionalEstateLayout().PointOr(Anchor::TownSquare, {});
    return estate;
}

void NewEstateStartsWithTheLamp()
{
    Estate estate = NewEstate();
    const auto& sim = estate.sim;
    CHECK(sim.Count(Item::OilLamp) == 1);
    CHECK(sim.Count(Item::OilFlask) == Lamp::StartingFlasks && Lamp::StartingFlasks == 3);
    CHECK(Near(sim.LampOil(), Lamp::CapacityHours));
    CHECK(sim.GetState().lampKitGranted);
    CHECK(IsTool(Item::OilLamp) && !IsTool(Item::OilFlask));
    CHECK(std::string(ItemName(Item::OilLamp)) == "Oil lamp" && CountedName(Item::OilFlask, 3) == "3 oil flasks");
    // The seeded woodland has no lamp.
    Simulation woodland;
    CHECK(woodland.Count(Item::OilLamp) == 0 && woodland.LampOil() == 0.0);
}

void BurnsOnlyWhileLit()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    // Put away: no burn.
    CHECK(!sim.IsLampLit());
    sim.AdvanceGameHours(2.0, estate.home);
    CHECK(Near(sim.LampOil(), Lamp::CapacityHours));
    // In hand: one hour of oil per game hour.
    sim.SetLampInHand(true);
    CHECK(sim.IsLampInHand() && sim.IsLampLit() && !sim.IsLampLit(true));
    sim.AdvanceGameHours(2.0, estate.home);
    CHECK(Near(sim.LampOil(), Lamp::CapacityHours - 2.0));
    sim.SetLampInHand(false);
    sim.AdvanceGameHours(1.0, estate.home);
    CHECK(Near(sim.LampOil(), Lamp::CapacityHours - 2.0));
    // It goes out at empty.
    sim.SetLampInHand(true);
    sim.AdvanceGameHours(5.0, estate.home);
    CHECK(sim.LampOil() == 0.0 && !sim.IsLampLit());
}

void RefillFromAFlask()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    const auto full = sim.RefillLamp();
    CHECK(!full.ok && full.message == "The lamp is already full.");
    CHECK(sim.Count(Item::OilFlask) == 3);
    sim.SetLampInHand(true);
    sim.AdvanceGameHours(3.0, estate.home);
    const auto filled = sim.RefillLamp();
    OK(filled);
    CHECK(filled.message == "Filled the lamp.");
    CHECK(Near(sim.LampOil(), Lamp::CapacityHours) && sim.Count(Item::OilFlask) == 2);
    // Without oil.
    sim.AdvanceGameHours(1.0, estate.home);
    for (int i = 0; i < 2; ++i) { sim.AdvanceGameHours(1.0, estate.home); OK(sim.RefillLamp()); }
    CHECK(sim.Count(Item::OilFlask) == 0);
    sim.AdvanceGameHours(1.0, estate.home);
    const auto dry = sim.RefillLamp();
    CHECK(!dry.ok && dry.message == "You have no oil. Oil flasks are sold at the general store.");
}

void FlasksAreSoldInTown()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    bool listed = false;
    for (Item good : ShopGoods(ShopKind::GeneralStore)) listed |= good == Item::OilFlask;
    CHECK(listed);
    CHECK(BuyPrice(Item::OilFlask) == 15 && !ShopBuys(ShopKind::GeneralStore, Item::OilFlask));
    CHECK(!ShopBuys(ShopKind::GeneralStore, Item::OilLamp));
    const Shop* shop = sim.FindShop(ShopKind::GeneralStore);
    CHECK(shop != nullptr);
    const Point counter = ProvisionalEstateLayout().PointOr(Anchor::GeneralStoreCounter, {});
    sim.SkipToHourOfDay(9.0);
    const auto bought = sim.Buy(shop->id, Item::OilFlask, 2, false, {counter.x, counter.y - 150.0});
    OK(bought);
    CHECK(bought.message == "Bought 2 oil flasks for $0.30.");
    CHECK(sim.Count(Item::OilFlask) == 5 && sim.GetState().money == StartingMoney - 30);
}

void SetDownPickUpAndSave()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    const Point spot{estate.home.x + 60.0, estate.home.y};
    // Too far away.
    CHECK(!sim.SetDownLamp({estate.home.x + 5000.0, estate.home.y}, estate.home).ok);
    const auto set = sim.SetDownLamp(spot, estate.home);
    OK(set);
    CHECK(set.message == "Set the lamp down.");
    CHECK(sim.Count(Item::OilLamp) == 0);
    const WorldDrop* drop = sim.SetDownLampDrop();
    CHECK(drop && drop->position.x == spot.x && drop->position.y == spot.y);
    // Set down, it burns without her holding it, and she can't fill it from afar.
    CHECK(sim.IsLampLit() && !sim.IsLampInHand());
    sim.AdvanceGameHours(1.5, estate.home);
    CHECK(Near(sim.LampOil(), Lamp::CapacityHours - 1.5));
    CHECK(sim.RefillLamp().message == "Pick up the lamp to fill it.");
    // It's still there, burning, after a save and load.
    const std::string saved = sim.Serialize();
    Simulation loaded;
    loaded.SetPlacements(ProvisionalEstatePlacements());
    OK(loaded.Deserialize(saved));
    CHECK(loaded.SetDownLampDrop() && loaded.SetDownLampDrop()->position.x == spot.x);
    CHECK(Near(loaded.LampOil(), sim.LampOil()) && loaded.Count(Item::OilLamp) == 0 && loaded.Count(Item::OilFlask) == 3);
    CHECK(loaded.Serialize() == saved);
    loaded.AdvanceGameHours(0.5, estate.home);
    CHECK(Near(loaded.LampOil(), Lamp::CapacityHours - 2.0));
    // Picked up, it's back in the pack and out until she holds it.
    OK(loaded.PickUpDrop(loaded.SetDownLampDrop()->id, estate.home));
    CHECK(loaded.Count(Item::OilLamp) == 1 && !loaded.SetDownLampDrop() && !loaded.IsLampLit());
    loaded.AdvanceGameHours(1.0, estate.home);
    CHECK(Near(loaded.LampOil(), Lamp::CapacityHours - 2.0));
    OK(loaded.SetDownLamp(spot, estate.home));
    CHECK(loaded.SetDownLampDrop() && loaded.Count(Item::OilLamp) == 0);
}

void OlderSavesGetTheKitOnce()
{
    Estate estate = NewEstate();
    auto& sim = estate.sim;
    const std::string saved = sim.Serialize();
    CHECK(saved.find("\nlamp 6 1\n") != std::string::npos);
    // A save from before the lamp: no section, so the kit arrives on load, once.
    Simulation older;
    older.SetPlacements(ProvisionalEstatePlacements());
    OK(older.Deserialize(WithLampLine(saved, "")));
    CHECK(older.GetState().lampKitGranted && older.Count(Item::OilFlask) == 2 * Lamp::StartingFlasks);
    CHECK(Near(older.LampOil(), Lamp::CapacityHours));
    Simulation again;
    again.SetPlacements(ProvisionalEstatePlacements());
    OK(again.Deserialize(older.Serialize()));
    CHECK(again.Count(Item::OilFlask) == 2 * Lamp::StartingFlasks);
    // Bad values are refused.
    Simulation bad;
    bad.SetPlacements(ProvisionalEstatePlacements());
    CHECK(!bad.Deserialize(WithLampLine(saved, "lamp 99 1\n")).ok);
    CHECK(!bad.Deserialize(WithLampLine(saved, "lamp -1 1\n")).ok);
    CHECK(!bad.Deserialize(WithLampLine(saved, "lamp 3 2\n")).ok);
    CHECK(!bad.Deserialize(WithLampLine(saved, "lamp nan 1\n")).ok);
    OK(bad.Deserialize(WithLampLine(saved, "lamp 2.5 1\n")));
    CHECK(Near(bad.LampOil(), 2.5) && bad.Count(Item::OilFlask) == Lamp::StartingFlasks);
}

const char* filter = nullptr;
void Run(const char* name, void (*test)())
{
    if (filter && !std::strstr(name, filter)) return;
    test();
    ++cases;
    std::cout << "PASS " << name << '\n';
}
}

int main(int argc, char** argv)
{
    if (argc > 1) filter = argv[1];
    Run("a new estate starts with the lamp and oil", NewEstateStartsWithTheLamp);
    Run("the lamp burns only while lit", BurnsOnlyWhileLit);
    Run("a flask refills the lamp", RefillFromAFlask);
    Run("flasks are sold in town", FlasksAreSoldInTown);
    Run("set down, picked up, saved and loaded", SetDownPickUpAndSave);
    Run("older saves get the kit once", OlderSavesGetTheKitOnce);
    std::cout << cases << " scenarios, " << checks << " explicit checks passed.\n";
    return 0;
}
