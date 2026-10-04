#include "HomesteadEstate.h"
#include "HomesteadEstatePublicRoad.h"
#include "HomesteadTreeFelling.h"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>

namespace
{
int TreeFellingChecks = 0;
void Check(bool condition, const char* expression, int line)
{
    ++TreeFellingChecks;
    if (!condition) { std::cerr << "FAIL " << line << ": " << expression << '\n'; std::exit(1); }
}
#define CHECK(expression) Check(static_cast<bool>(expression), #expression, __LINE__)

using Homestead::Point;
namespace TF = Homestead::TreeFelling;

Homestead::Simulation Estate()
{
    Homestead::Simulation sim;
    Homestead::EstatePlacements empty;
    empty.bakeVersion = Homestead::ProvisionalEstatePlacements().bakeVersion;
    CHECK(sim.NewEstateGame(Homestead::ProvisionalEstateLayout(), empty));
    return sim;
}

// Scans the estate's metre-spaced surroundings for a tree spot with the wanted rules.
Point Find(bool regrows, bool allowed)
{
    const Point manor = Homestead::ProvisionalEstateLayout().FindPolygon(Homestead::Anchor::ManorFootprint)->points[0];
    for (double radius = 1000.0; radius < 40000.0; radius += 700.0)
        for (int step = 0; step < 24; ++step)
        {
            const double angle = step * 0.2618;
            const Point at{manor.x + radius * std::cos(angle), manor.y + radius * std::sin(angle)};
            if ((*TF::ProtectedReason(at) == 0) == allowed && TF::Regrows(at) == regrows) return at;
        }
    std::cerr << "no spot regrows=" << regrows << " allowed=" << allowed << '\n';
    CHECK(false);
    return {};
}

Homestead::Simulation Armed()
{
    auto sim = Estate();
    sim.GrantItems(Homestead::Item::Hatchet, 1);
    sim.SetEnergy(100.0);
    return sim;
}

void Rules()
{
    const Point far = Find(true, true);
    const Point player{far.x + 100.0, far.y};
    auto sim = Estate();
    sim.SetEnergy(100.0);
    CHECK(!sim.FellSceneryTree(far, player));
    sim = Armed();
    CHECK(!sim.FellSceneryTree(far, Point{far.x + 5000.0, far.y}));
    sim.SetEnergy(3.0);
    CHECK(!sim.FellSceneryTree(far, player));
    sim.SetEnergy(100.0);
    const int timber = sim.Count(Homestead::Item::Timber), branch = sim.Count(Homestead::Item::Branch);
    const double energy = sim.GetState().energy;
    const auto result = sim.FellSceneryTree(far, player);
    CHECK(result);
    CHECK(sim.Count(Homestead::Item::Timber) == timber + 6 && sim.Count(Homestead::Item::Branch) == branch + 4);
    CHECK(sim.GetState().energy < energy);
    CHECK(TF::StageOf(sim.GetState(), far) == TF::Stage::Stump);
    CHECK(!sim.FellSceneryTree(far, player));
}

void Protected()
{
    auto sim = Armed();
    const auto& line = Homestead::EstatePublicRoad().points;
    const Point road = line[line.size() / 2];
    CHECK(*TF::ProtectedReason(road) != 0);
    CHECK(!sim.FellSceneryTree(road, road));
    CHECK(sim.GetState().felledTrees.empty());
}

void Regrowth()
{
    const Point far = Find(true, true), near = Find(false, true);
    CHECK(TF::Regrows(far) && !TF::Regrows(near));
    auto sim = Armed();
    CHECK(sim.FellSceneryTree(far, far));
    sim.SetEnergy(100.0);
    CHECK(sim.FellSceneryTree(near, near));
    sim.AdvanceGameHours(TF::StumpHours + 1.0, far);
    CHECK(TF::StageOf(sim.GetState(), far) == TF::Stage::Sapling);
    CHECK(TF::StageOf(sim.GetState(), near) == TF::Stage::Stump);
    CHECK(TF::NextChangeHour(sim.GetState()) > sim.GetState().hour);
    sim.AdvanceGameHours(TF::RegrowHours, far);
    CHECK(TF::StageOf(sim.GetState(), far) == TF::Stage::Standing);
    CHECK(TF::StageOf(sim.GetState(), near) == TF::Stage::Stump);
    sim.SetEnergy(100.0);
    CHECK(sim.FellSceneryTree(far, far));
    CHECK(sim.GetState().felledTrees.size() == 2);
}

void SaveRoundTrip()
{
    const Point far = Find(true, true), near = Find(false, true);
    auto sim = Armed();
    CHECK(sim.FellSceneryTree(near, near));
    sim.SetEnergy(100.0);
    CHECK(sim.FellSceneryTree(far, far));
    const std::string saved = sim.Serialize();
    auto loaded = Estate();
    const auto loadResult = loaded.Deserialize(saved);
    CHECK(loadResult);
    CHECK(loaded.GetState().felledTrees.size() == 2);
    CHECK(TF::StageOf(loaded.GetState(), far) == TF::Stage::Stump);
    CHECK(loaded.Serialize() == saved);
    auto clean = Estate();
    CHECK(clean.Serialize().find("felled") == std::string::npos);
}

void BadSections()
{
    for (const char* text : {"-1", "5000", "1 0 0 5 2", "2 5 5 1 0 5 5 1 0", "2 9 9 1 0 3 3 1 0", "1 0 0 nan 1", "x"})
    {
        std::istringstream input(text);
        Homestead::State candidate;
        candidate.fixedEstate = true;
        CHECK(!TF::ReadSaveSection(input, candidate));
        CHECK(candidate.felledTrees.empty());
    }
}

std::string Reseal(const std::string& saved, const std::string& payload)
{
    std::uint64_t hash = UINT64_C(14695981039346656037);
    for (unsigned char c : payload) { hash ^= c; hash *= UINT64_C(1099511628211); }
    std::istringstream header(saved.substr(0, saved.find('\n')));
    std::string magic, version;
    header >> magic >> version;
    return magic + " " + version + " " + std::to_string(payload.size()) + " " + std::to_string(hash) + "\n" + payload;
}

void Capacity()
{
    auto sim = Armed();
    const std::string saved = sim.Serialize();
    std::string payload = saved.substr(saved.find('\n') + 1);
    std::ostringstream section;
    section << "felled " << TF::MaxFelled;
    for (int i = 0; i < TF::MaxFelled; ++i) section << ' ' << -900000 + i << " 0 0 0";
    section << '\n';
    payload += section.str();
    auto loaded = Estate();
    CHECK(loaded.Deserialize(Reseal(saved, payload)));
    CHECK(static_cast<int>(loaded.GetState().felledTrees.size()) == TF::MaxFelled);
    const Point far = Find(true, true);
    CHECK(!loaded.FellSceneryTree(far, far));
    CHECK(!loaded.Deserialize(Reseal(saved, payload + section.str())));
}
}
int main()
{
    Rules();
    Protected();
    Regrowth();
    SaveRoundTrip();
    BadSections();
    Capacity();
    std::cout << "HomesteadTreeFellingTests passed (" << TreeFellingChecks << " checks)\n";
    return 0;
}
