#pragma once

#include "HomesteadSimulation.h"

#include <iosfwd>
#include <string>

// add-ruined-manor-and-arrival: the heritage standing room, the reserved manor footprint and the
// new-game names. Presentation lives in the Unreal module; this is the pure game state.
namespace Homestead
{
struct EstateLayout;

namespace Manor
{
// Placeholders until Jenny chooses the real ones.
constexpr const char* DefaultHeroineName = "Eleanor";
constexpr const char* DefaultFamilyName = "Cavendish";
constexpr const char* DefaultEstateName = "Trevennor";
constexpr int ArrivalYear = 1851;
// Characters (Unicode code points), after trimming.
constexpr int MaxNameLength = 24;
constexpr const char* FootprintBlocked = "The old manor stands here.";

// The standing room's plan in its own building grid (cells 0..1 x 0..1, CellSize each). Cell x
// runs along the room's heading (north at yaw 0), cell y to its right (east). The doorway is on
// the west side of cell (0, 0), facing into the ruined hall.
constexpr int RoomCells = 2;
constexpr int DoorCellX = 0, DoorCellY = 0, DoorRotation = 2;
constexpr int HearthCellX = 0, HearthCellY = 1, HearthRotation = 0;
constexpr int BedCellX = 1, BedCellY = 1, BedRotation = 0;
constexpr int ChestCellX = 1, ChestCellY = 0, ChestRotation = 0;
constexpr int SeededBranches = 4;

// Strips leading and trailing whitespace (ASCII and U+00A0).
std::string TrimName(const std::string& utf8);
// Code points in well-formed UTF-8, or -1 when it isn't (or holds control characters).
int NameLength(const std::string& utf8);
// Why a trimmed name can't be used ("Enter a first name." ...), or empty when it can. `what` is
// the field ("first name", "surname", "estate name").
std::string NameProblem(const std::string& trimmed, const char* what);

// Adds the heritage standing room centred on the layout's StandingRoomOrigin: stone foundations, walls,
// a doorway and roof, the hearth, the bed and a chest holding the pail and four branches.
// Returns false (leaving `state` unchanged) when the layout has no room anchor.
bool SeedStandingRoom(State& state, const EstateLayout& layout);
// New estate games only (never on load, never SeedStandingRoomAt): provisions and a change of
// clothes in the standing room's chest beside the pail and branches. Returns false, changing
// nothing, when there is no heritage chest or they wouldn't fit.
constexpr int StarterPasties = 3;
constexpr int StarterBread = 2;
// One of each finished outfit piece she isn't already wearing, in this order.
constexpr WearableDefinition StarterWardrobe[] = {
    WearableDefinition::LinenShirt, WearableDefinition::LinenLongShirt, WearableDefinition::Trousers,
    WearableDefinition::FurCoat, WearableDefinition::FurBoots, WearableDefinition::WovenSandals,
    WearableDefinition::TurnShoes};
bool StockStarterChest(State& state);
// The heritage building's id, or 0 when there is none.
int HeritageBuildingId(const State& state);
// The roofless hall she may build in (Jenny's playtest: the estate is hers, but nothing could go inside
// the ruin). In the manor's own frame (cm; U east from the footprint's west end, V north from its south
// front, as AHomesteadManorRuin draws it): east of the cross wall (U 1800), inside the east gable and
// the rear wall, and north of the strip in front of the standing room's doorway, so her way out to the
// cross-wall openings and the west rooms stays open. Each edge keeps the wall's half thickness (28 cm)
// plus her capsule (45 cm) clear of the masonry.
constexpr double SafeHallMinU = 1873.0, SafeHallMaxU = 2899.0;
constexpr double SafeHallMinV = 700.0, SafeHallMaxV = 1699.0;
// Where the ruin draws its fallen roof timbers in the hall, and their reach plus her capsule: kept clear
// while they are still ruin scenery (once they are clearable debris, the placement rules keep off them).
constexpr double HallTimbersU = 2450.0, HallTimbersV = 1300.0, HallTimbersClearance = 290.0;
// Whether a world point lies in the safe hall.
bool InSafeHall(const EstateLayout& layout, Point point);
// Whether building over `area` is refused because it lies on the ruined manor's footprint. Inside the
// footprint only three things are allowed: furnishing the heritage standing room's own floor; a new
// foundation of her own; and a fire, bed or chest. The last two need the whole footprint in the safe hall.
// Walls, doorways and roofs never go up inside the ruin.
bool BlockedByManor(const State& state, const EstateLayout& layout, const PlacementTarget& target,
    const Footprint& area);

// "Eleanor Cavendish — Trevennor, Spring 1" (an empty string for unnamed woodland games).
std::string SaveLabel(const State& state, const char* season, int day);

// Journal entries: the arrival note is written when she first comes home.
constexpr const char* ArrivalEntry = "arrival";
// Title and body of a journal entry, or empty strings for an unknown key.
std::string JournalTitle(const std::string& key);
std::string JournalText(const std::string& key, const State& state);

// Optional trailing save section (tag "manor"): names plus per-structure skins and heritage flags.
constexpr const char* SaveTag = "manor";
bool HasSaveSection(const State& state);
void WriteSaveSection(std::ostream& output, const State& state);
// Reads the section after its tag; structures must already be loaded.
bool ReadSaveSection(std::istream& input, State& state);
}
}
