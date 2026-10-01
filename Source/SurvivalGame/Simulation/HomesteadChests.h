#pragma once

#include "HomesteadSimulation.h"

#include <iosfwd>
#include <string>

// Her storage chests: custom names (keyed by the chest's stable structure id) and the matching-stack
// rules behind Simulation::StoreMatching. Presentation lives in the Unreal module.
namespace Homestead
{
namespace Chests
{
// What a chest is called until she names it.
constexpr const char* DefaultName = "Storage chest";
// Characters (Unicode code points), after trimming.
constexpr int MaxNameLength = 24;

// Her name for the chest, or DefaultName (also for an id that isn't a chest).
std::string DisplayName(const State& state, int chestId);
// Why a trimmed name can't be used, or empty when it can. An empty name is allowed: it clears the
// chest back to DefaultName.
std::string NameProblem(const std::string& trimmed);
// Whether auto-store may move this item onto a matching chest stack: goods only. Tools, the pail's
// water and her lamp stay with her.
bool AutoStores(Item item);

// Optional trailing save section (tag "chestnames"), written only when a chest has a name:
// a count, then each chest id with its hex-encoded UTF-8 name.
constexpr const char* SaveTag = "chestnames";
bool HasSaveSection(const State& state);
void WriteSaveSection(std::ostream& output, const State& state);
// Reads the section after its tag; structures must already be loaded.
bool ReadSaveSection(std::istream& input, State& state);
}
}
