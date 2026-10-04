#pragma once

#include "HomesteadSimulation.h"

#include <iosfwd>

// Felling the estate's decorative trees (openspec chop-any-tree). The Unreal world draws about 43,000
// woodland, hedge and avenue trees as scenery; any one of them can be chopped with the axe. A felled tree
// is remembered by its trunk position in whole centimetres (State::felledTrees), leaves a stump, and,
// well away from the manor and the derelict farm, grows back: a sapling after a day, a full tree after
// three. Next to the manor and farm the stump stays. Trees on the public road verge and in the town square
// are the village's and can't be felled. Growth rates beyond this fixed schedule are deferred.
namespace Homestead::TreeFelling
{
// Optional trailing save section: "felled <count>" then count entries "<xCm> <yCm> <fellHour> <regrows 0|1>".
constexpr const char* SaveTag = "felled";
// How close the trunk must be to chop it, cm (Simulation's Reach is 300; the underbrush's is 450).
constexpr double ReachCm = 450.0;
// A stump alone, then a sapling, until the tree is whole again (Balance: sapling day 1, tree day 3).
constexpr double StumpHours = 24.0;
constexpr double RegrowHours = 72.0;
// Trees this far or more from the manor and the derelict farm grow back, cm (Balance: 60 m).
constexpr double RegrowFromHomeCm = 6000.0;
// Trees this close to the public road's centreline, or to the town square, can't be felled, cm.
constexpr double RoadVergeCm = 900.0;
constexpr double TownSquareCm = 4500.0;
// Bounds the save (a dense wood would take weeks to clear this many).
constexpr int MaxFelled = 4096;

enum class Stage { Standing, Stump, Sapling };

int Cm(double value);
const FelledTree* Find(const State& state, Point tree);
// Standing when it was never felled or has regrown.
Stage StageOf(const State& state, Point tree);
Stage StageOf(const FelledTree& entry, double hour);
// Grows back (far enough from the manor and farm).
bool Regrows(Point tree);
// Why this tree is the village's and can't be felled, or empty.
const char* ProtectedReason(Point tree);
// The next game hour after state.hour at which a felled tree changes stage or regrows, or a huge value.
double NextChangeHour(const State& state);
// Removes entries that have regrown.
void Prune(State& state);

void WriteSaveSection(std::ostream& output, const State& state);
bool ReadSaveSection(std::istream& input, State& state);
}
