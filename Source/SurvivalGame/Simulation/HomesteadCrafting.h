#pragma once

#include "HomesteadSimulation.h"

#include <functional>
#include <iosfwd>
#include <unordered_map>

// rework-farming-calendar-and-period-crafting, lane E: the workbench and sawhorse stations, planks,
// post-and-rail fences and field gates, and small furniture. The rules live here; the Unreal module
// presents them (HomesteadWorldCrafting.cpp, HomesteadControllerCrafting.cpp).
namespace Homestead
{
namespace Crafting
{
// A station counts when its centre is within this of her (a 1.8 m bench plus arm's reach).
constexpr double StationReach = 300.0;
// Energy for one saw through a length of timber, and for a workbench job.
constexpr double SawEnergy = 1.5;
constexpr double JoineryEnergy = 1.0;
constexpr int PlanksPerTimber = 4;

// One fence section or gate spans FenceSpan cm between its post centres, along its own +X.
constexpr double FenceSpan = 240.0;
// Its footprint stops short of the posts, so bays meeting at a shared post at any angle don't clash.
constexpr double FenceFootprintHalfLength = 108.0;
constexpr double FenceFootprintHalfWidth = 8.0;
// A new section joins an existing post when its centre would land within this of the aim.
constexpr double FenceSnapReach = 170.0;
// Fences turn in whole steps of this.
constexpr double FenceYawStep = 15.0;
// Two post positions closer than this are the same post.
constexpr double PostMergeDistance = 3.0;
constexpr double GateReach = 300.0;

bool IsStation(Piece piece);
bool IsFence(Piece piece);
// Stool, table, chair and shelf: several may share a foundation cell, each at its own spot.
bool IsMovable(Piece piece);
// The Craft page's tabs. Stations lists the workbench and sawhorse (built from the Build page).
enum class CraftCategory : int { Tools, Stations, Farm, Furniture, Cooking, Count };
constexpr int CraftCategoryCount = static_cast<int>(CraftCategory::Count);
CraftCategory CategoryOf(Recipe recipe);
const char* CraftCategoryName(CraftCategory category);
// The station a recipe needs within StationReach, or Piece::Count for none (hafting stays by hand;
// cooking needs a fire, checked separately).
Piece StationFor(Recipe recipe);
// "Workbench within reach".
const char* StationLabel(Piece station);
// "Needs a workbench within reach. Build one from 6 Timber + 4 Twine on the Build page."
std::string StationMissingMessage(Piece station);
// Footprint half extents (piece space) for the stations and small furniture.
Point FurnitureHalf(Piece piece);

// The two post positions of a fence piece: side 0 is its -X end (a gate's hinge), side 1 its +X end.
Point FenceEnd(const Building& frame, int cellX, int cellY, int rotation, int side);
Point FenceEnd(const State& state, const Structure& piece, int side);
// Which fence pieces draw which posts, so a post shared by two bays is drawn once: bit 0 for the -X
// end, bit 1 for the +X end. Every fence piece has an entry.
std::unordered_map<int, unsigned> PostOwners(const State& state);
// Resolves a fence or gate at `aim`: its heading is `freeYaw` in whole FenceYawStep steps, and it
// joins the end post of an existing section when one is near (the nearest candidate `buildable`
// accepts wins, else the nearest), otherwise it stands free centred on the aim.
PlacementTarget ResolveFence(const State& state, Piece kind, Point aim, double freeYaw,
    const std::function<bool(const PlacementTarget&)>& buildable);
// Where small furniture aimed at `aim` stands in floor cell (cellX, cellY) of `building`: the aim,
// in 5 cm steps, kept clear of the cell's walls, in piece space for `rotation`.
Point MovableSpot(const Building& building, Piece kind, int cellX, int cellY, int rotation, Point aim);

// Optional trailing save sections: "gates" lists the open gates, "spots" small furniture spots.
// Each is left out when it holds nothing.
constexpr const char* GateTag = "gates";
constexpr const char* SpotTag = "spots";
void WriteSaveSections(std::ostream& output, const State& state);
// Read after their tags, once the structures are loaded. False on anything malformed or repeated.
bool ReadGates(std::istream& input, State& state);
bool ReadSpots(std::istream& input, State& state);
}
}
