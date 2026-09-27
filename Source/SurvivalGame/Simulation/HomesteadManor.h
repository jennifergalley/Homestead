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
constexpr const char* DefaultFamilyName = "Trelawney";
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
// The heritage building's id, or 0 when there is none.
int HeritageBuildingId(const State& state);
// Whether building over `area` is refused because it lies on the ruined manor's footprint.
bool BlockedByManor(const State& state, const EstateLayout& layout, const PlacementTarget& target,
    const Footprint& area);

// "Eleanor Trelawney — Trevennor, Spring 1" (an empty string for unnamed woodland games).
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
