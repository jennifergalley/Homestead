#include "HomesteadManor.h"
#include "HomesteadEstate.h"
#include "HomesteadRuinDebris.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <istream>
#include <ostream>
#include <set>

namespace Homestead
{
namespace Manor
{
namespace
{
bool IsSpace(const std::string& text, std::size_t at, std::size_t& width)
{
    const unsigned char c = static_cast<unsigned char>(text[at]);
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v') { width = 1; return true; }
    if (c == 0xC2 && at + 1 < text.size() && static_cast<unsigned char>(text[at + 1]) == 0xA0) { width = 2; return true; }
    return false;
}

std::string Hex(const std::string& text)
{
    if (text.empty()) return "-";
    static const char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(text.size() * 2);
    for (unsigned char c : text)
    {
        result += digits[c >> 4];
        result += digits[c & 15];
    }
    return result;
}

bool Unhex(const std::string& token, std::string& text)
{
    text.clear();
    if (token == "-") return true;
    if (token.empty() || token.size() % 2 || token.size() > MaxNameLength * 8) return false;
    const auto nibble = [](char c) -> int
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (std::size_t i = 0; i < token.size(); i += 2)
    {
        const int high = nibble(token[i]), low = nibble(token[i + 1]);
        if (high < 0 || low < 0) return false;
        text += static_cast<char>(high * 16 + low);
    }
    return true;
}

// A saved name is either empty (woodland games) or already trimmed and valid.
bool ValidSavedName(const std::string& name)
{
    return name.empty() || (TrimName(name) == name && NameProblem(name, "name").empty());
}

void Add(State& state, Piece kind, int buildingId, int x, int y, int rotation)
{
    Structure piece{state.nextId++, kind, x, y, rotation, 0.0, {}};
    piece.buildingId = buildingId;
    piece.heritage = true;
    if (kind == Piece::Foundation || kind == Piece::Wall || kind == Piece::Doorway || kind == Piece::Roof)
        piece.skin = StructureSkin::Stone;
    state.structures.push_back(std::move(piece));
}
}

std::string TrimName(const std::string& utf8)
{
    std::size_t begin = 0, end = utf8.size(), width = 0;
    while (begin < end && IsSpace(utf8, begin, width)) begin += width;
    while (end > begin)
    {
        if (IsSpace(utf8, end - 1, width)) { end -= 1; continue; }
        if (end - begin >= 2 && IsSpace(utf8, end - 2, width) && width == 2) { end -= 2; continue; }
        break;
    }
    return utf8.substr(begin, end - begin);
}

int NameLength(const std::string& utf8)
{
    int count = 0;
    for (std::size_t i = 0; i < utf8.size();)
    {
        const unsigned char c = static_cast<unsigned char>(utf8[i]);
        int extra = 0;
        std::uint32_t code = 0;
        if (c < 0x80) code = c;
        else if ((c & 0xE0) == 0xC0) { extra = 1; code = c & 0x1F; }
        else if ((c & 0xF0) == 0xE0) { extra = 2; code = c & 0x0F; }
        else if ((c & 0xF8) == 0xF0) { extra = 3; code = c & 0x07; }
        else return -1;
        for (int k = 1; k <= extra; ++k)
        {
            if (i + k >= utf8.size()) return -1;
            const unsigned char next = static_cast<unsigned char>(utf8[i + k]);
            if ((next & 0xC0) != 0x80) return -1;
            code = (code << 6) | (next & 0x3F);
        }
        if (code < 0x20 || code == 0x7F || (code >= 0x80 && code < 0xA0)) return -1;
        if ((extra == 1 && code < 0x80) || (extra == 2 && code < 0x800) || (extra == 3 && code < 0x10000)
            || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) return -1;
        i += static_cast<std::size_t>(extra) + 1;
        ++count;
    }
    return count;
}

std::string NameProblem(const std::string& trimmed, const char* what)
{
    const int length = NameLength(trimmed);
    if (length < 0) return std::string("Use ordinary letters in the ") + what + ".";
    if (length == 0)
    {
        const bool vowel = what && std::string("aeiouAEIOU").find(what[0]) != std::string::npos;
        return std::string(vowel ? "Enter an " : "Enter a ") + what + ".";
    }
    if (length > MaxNameLength)
        return std::string("Keep the ") + what + " to " + std::to_string(MaxNameLength) + " characters.";
    return {};
}

bool SeedStandingRoom(State& state, const EstateLayout& layout)
{
    const Landmark* origin = layout.FindLandmark(Anchor::StandingRoomOrigin);
    if (!origin) return false;
    double yaw = std::fmod(origin->yaw, 360.0);
    if (yaw < 0.0) yaw += 360.0;
    if (yaw >= 360.0) yaw = 0.0;
    const int buildingId = state.nextId++;
    // The anchor is the room's centre; the grid's cell (0, 0) corner sits half the room back from it.
    const Point corner = RotateYaw({RoomCells * CellSize * 0.5, RoomCells * CellSize * 0.5}, yaw);
    state.buildings.push_back({buildingId, {origin->position.x - corner.x, origin->position.y - corner.y}, yaw});
    for (int x = 0; x < RoomCells; ++x)
        for (int y = 0; y < RoomCells; ++y)
            Add(state, Piece::Foundation, buildingId, x, y, 0);
    // Every outer edge is masonry except the doorway into the old hall.
    for (int i = 0; i < RoomCells; ++i)
    {
        Add(state, Piece::Wall, buildingId, i, RoomCells - 1, 0);
        Add(state, Piece::Wall, buildingId, RoomCells - 1, i, 1);
        Add(state, i == DoorCellX ? Piece::Doorway : Piece::Wall, buildingId, i, 0, 2);
        Add(state, Piece::Wall, buildingId, 0, i, 3);
    }
    for (int x = 0; x < RoomCells; ++x)
        for (int y = 0; y < RoomCells; ++y)
            Add(state, Piece::Roof, buildingId, x, y, 0);
    Add(state, Piece::Hearth, buildingId, HearthCellX, HearthCellY, HearthRotation);
    Add(state, Piece::Bed, buildingId, BedCellX, BedCellY, BedRotation);
    Add(state, Piece::Chest, buildingId, ChestCellX, ChestCellY, ChestRotation);
    Structure& chest = state.structures.back();
    chest.storage[static_cast<int>(Item::WateringCan)] = 1;
    chest.storage[static_cast<int>(Item::Branch)] = SeededBranches;
    chest.layout.push_back({state.nextGroupId++, Item::WateringCan, 1, 0});
    chest.layout.push_back({state.nextGroupId++, Item::Branch, SeededBranches, 0});
    return true;
}

bool StockStarterChest(State& state)
{
    Structure* chest = nullptr;
    for (auto& piece : state.structures)
        if (piece.heritage && piece.kind == Piece::Chest) { chest = &piece; break; }
    if (!chest) return false;
    int used = 0;
    for (int i = 0; i < ItemCount; ++i) used += chest->storage[i];
    std::vector<WearableDefinition> clothes;
    for (const WearableDefinition piece : StarterWardrobe)
        if (std::none_of(state.wearables.begin(), state.wearables.end(),
            [&](const WearableInstance& worn) { return worn.definition == piece; }))
            clothes.push_back(piece);
    for (const auto& item : state.wearables) used += item.owner == WearableOwner::Chest && item.chestId == chest->id;
    if (used + StarterPasties + StarterBread + static_cast<int>(clothes.size()) > ChestCapacity) return false;
    chest->storage[static_cast<int>(Item::Pasty)] += StarterPasties;
    chest->storage[static_cast<int>(Item::Bread)] += StarterBread;
    chest->layout.push_back({state.nextGroupId++, Item::Pasty, StarterPasties, 0});
    chest->layout.push_back({state.nextGroupId++, Item::Bread, StarterBread, 0});
    for (const WearableDefinition piece : clothes)
    {
        const int id = state.nextWearableId++;
        state.wearables.push_back({id, piece, 0, WearableOwner::Chest, chest->id});
        chest->layout.push_back({0, Item::Knife, 0, id});
    }
    return true;
}

int HeritageBuildingId(const State& state)
{
    for (const auto& piece : state.structures)
        if (piece.heritage && piece.buildingId > 0) return piece.buildingId;
    return 0;
}

bool InSafeHall(const EstateLayout& layout, Point point)
{
    const LandmarkPolygon* manor = layout.FindPolygon(Anchor::ManorFootprint);
    if (!manor || manor->points.size() < 3) return false;
    double west = manor->points[0].y, south = manor->points[0].x;
    for (const Point& corner : manor->points)
    {
        west = std::min(west, corner.y);
        south = std::min(south, corner.x);
    }
    // The ruin's frame: V runs north (+X), U east (+Y), from the footprint's south-west corner.
    const double u = point.y - west, v = point.x - south;
    if (u < SafeHallMinU || u > SafeHallMaxU || v < SafeHallMinV || v > SafeHallMaxV) return false;
    // Still ruin scenery (not yet an appended debris placement): keep clear of the fallen timbers.
    if (!RuinDebris::Replaces("RuinFallenTimbers", HallTimbersU, HallTimbersV)
        && std::hypot(u - HallTimbersU, v - HallTimbersV) < HallTimbersClearance) return false;
    return true;
}

bool BlockedByManor(const State& state, const EstateLayout& layout, const PlacementTarget& target,
    const Footprint& area)
{
    if (!state.fixedEstate) return false;
    const LandmarkPolygon* manor = layout.FindPolygon(Anchor::ManorFootprint);
    if (!manor || manor->points.size() < 3) return false;
    // She may still furnish the standing room itself: only on one of its own heritage floors.
    const int room = HeritageBuildingId(state);
    if (room != 0 && target.buildingId == room && target.kind != Piece::Foundation)
    {
        for (const auto& piece : state.structures)
            if (piece.heritage && piece.kind == Piece::Foundation && piece.buildingId == room
                && piece.cellX == target.cellX && piece.cellY == target.cellY) return false;
    }
    const double hx = std::max(0.0, area.half.x - 1.0), hy = std::max(0.0, area.half.y - 1.0);
    const Point samples[] = {{0, 0}, {hx, hy}, {-hx, hy}, {hx, -hy}, {-hx, -hy}, {hx, 0}, {-hx, 0}, {0, hy}, {0, -hy}};
    bool inside = false, allInHall = true;
    for (const Point& sample : samples)
    {
        const Point offset = RotateYaw(sample, area.yaw);
        const Point at{area.center.x + offset.x, area.center.y + offset.y};
        inside = inside || PointInPolygon(manor->points, at);
        allInHall = allInHall && InSafeHall(layout, at);
    }
    // Wholly outside the ruin, including the standing room's own grid extended south or east: as before.
    if (!inside) return false;
    // In the roofless hall: a foundation, fire, bed or chest, whether on her own building or the standing
    // room's grid extended north into it, and never walls, doorways or roofs.
    const bool hallPiece = target.kind == Piece::Foundation || target.kind == Piece::Fire || target.kind == Piece::Bed
        || target.kind == Piece::Chest;
    return !(allInHall && hallPiece);
}

std::string SaveLabel(const State& state, const char* season, int day)
{
    if (state.heroineName.empty() && state.familyName.empty() && state.estateName.empty()) return {};
    std::string label = state.heroineName;
    if (!state.familyName.empty()) label += (label.empty() ? "" : " ") + state.familyName;
    if (!state.estateName.empty()) label += " \xE2\x80\x94 " + state.estateName;
    return label + ", " + (season ? season : "") + " " + std::to_string(day);
}

bool HasSaveSection(const State& state)
{
    if (!state.heroineName.empty() || !state.familyName.empty() || !state.estateName.empty()
        || !state.journal.empty()) return true;
    for (const auto& piece : state.structures)
        if (piece.heritage || piece.skin != StructureSkin::Timber) return true;
    return false;
}

void WriteSaveSection(std::ostream& output, const State& state)
{
    int marked = 0;
    for (const auto& piece : state.structures)
        marked += piece.heritage || piece.skin != StructureSkin::Timber;
    output << SaveTag << ' ' << Hex(state.heroineName) << ' ' << Hex(state.familyName) << ' '
           << Hex(state.estateName) << ' ' << marked << '\n';
    for (const auto& piece : state.structures)
        if (piece.heritage || piece.skin != StructureSkin::Timber)
            output << piece.id << ' ' << static_cast<int>(piece.skin) << ' ' << (piece.heritage ? 1 : 0) << '\n';
    output << state.journal.size();
    for (const auto& key : state.journal) output << ' ' << key;
    output << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    std::string heroine, family, estate;
    int count = -1;
    if (!(input >> heroine >> family >> estate >> count)) return false;
    if (!Unhex(heroine, state.heroineName) || !Unhex(family, state.familyName) || !Unhex(estate, state.estateName)
        || !ValidSavedName(state.heroineName) || !ValidSavedName(state.familyName) || !ValidSavedName(state.estateName)
        || count < 0 || count > static_cast<int>(state.structures.size())) return false;
    std::set<int> seen;
    for (int i = 0; i < count; ++i)
    {
        int id = 0, skin = -1, heritage = -1;
        if (!(input >> id >> skin >> heritage) || !seen.insert(id).second || skin < 0
            || skin >= static_cast<int>(StructureSkin::Count) || (heritage != 0 && heritage != 1)) return false;
        Structure* found = nullptr;
        for (auto& piece : state.structures) if (piece.id == id) { found = &piece; break; }
        if (!found) return false;
        found->skin = static_cast<StructureSkin>(skin);
        found->heritage = heritage == 1;
    }
    int entries = -1;
    if (!(input >> entries) || entries < 0 || entries > 1000) return false;
    state.journal.clear();
    for (int i = 0; i < entries; ++i)
    {
        std::string key;
        if (!(input >> key) || JournalTitle(key).empty()
            || std::find(state.journal.begin(), state.journal.end(), key) != state.journal.end()) return false;
        state.journal.push_back(key);
    }
    return true;
}

std::string JournalTitle(const std::string& key)
{
    if (key == ArrivalEntry) return "Home at last";
    return {};
}

std::string JournalText(const std::string& key, const State& state)
{
    if (key != ArrivalEntry) return {};
    const std::string estate = state.estateName.empty() ? DefaultEstateName : state.estateName;
    return "Spring 1, " + std::to_string(ArrivalYear) + ". Home at last, to " + estate + ". The house is a ruin, "
        "the fields are bramble to the hedgerow, and the roof of the old hall lies where it fell. One room still keeps "
        "the weather out: the corner by the kitchen hearth, with a bed and a chest. The pail is in the chest, with a few "
        "dry branches, pasties and bread from the town, and a change of clothes. Father's garden tools always hung in the "
        "west rooms, by the chimney; something of them may be left under the rubble. It will do for a beginning.";
}
}
}
