#include "HomesteadChests.h"
#include "HomesteadManor.h"

#include <istream>
#include <ostream>
#include <set>

namespace Homestead
{
namespace Chests
{
namespace
{
const Structure* FindChest(const State& state, int chestId)
{
    for (const auto& piece : state.structures)
        if (piece.id == chestId) return piece.kind == Piece::Chest ? &piece : nullptr;
    return nullptr;
}

std::string ChestNameHex(const std::string& text)
{
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

bool ChestNameUnhex(const std::string& token, std::string& text)
{
    text.clear();
    if (token.empty() || token.size() % 2 || token.size() > static_cast<std::size_t>(MaxNameLength) * 8) return false;
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
}

std::string DisplayName(const State& state, int chestId)
{
    const auto* chest = FindChest(state, chestId);
    return chest && !chest->customName.empty() ? chest->customName : DefaultName;
}

std::string NameProblem(const std::string& trimmed)
{
    const int length = Manor::NameLength(trimmed);
    if (length < 0) return "Use ordinary letters in the chest's name.";
    if (length > MaxNameLength) return "Keep the chest's name to " + std::to_string(MaxNameLength) + " characters.";
    return {};
}

bool AutoStores(Item item)
{
    return item != Item::Water && item != Item::OilLamp && !IsTool(item);
}

bool HasSaveSection(const State& state)
{
    for (const auto& piece : state.structures)
        if (piece.kind == Piece::Chest && !piece.customName.empty()) return true;
    return false;
}

void WriteSaveSection(std::ostream& output, const State& state)
{
    int count = 0;
    for (const auto& piece : state.structures)
        if (piece.kind == Piece::Chest && !piece.customName.empty()) ++count;
    output << SaveTag << ' ' << count;
    for (const auto& piece : state.structures)
        if (piece.kind == Piece::Chest && !piece.customName.empty())
            output << ' ' << piece.id << ' ' << ChestNameHex(piece.customName);
    output << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    int count = -1;
    if (!(input >> count) || count < 0 || count > static_cast<int>(state.structures.size())) return false;
    std::set<int> seen;
    for (int i = 0; i < count; ++i)
    {
        int id = 0;
        std::string token, name;
        if (!(input >> id >> token) || !seen.insert(id).second || !ChestNameUnhex(token, name)) return false;
        // A saved name is already trimmed, non-empty and valid.
        if (name.empty() || Manor::TrimName(name) != name || !NameProblem(name).empty()) return false;
        Structure* chest = nullptr;
        for (auto& piece : state.structures) if (piece.id == id) { chest = &piece; break; }
        if (!chest || chest->kind != Piece::Chest) return false;
        chest->customName = name;
    }
    return true;
}
}
}
