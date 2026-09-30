#include "HomesteadBackpack.h"

#include <istream>
#include <ostream>

namespace Homestead
{
int PackCapacity(const State& state) { return state.leatherBackpack ? MaxPackCapacity : InventoryCapacity; }
int ContainerCapacity(const State& state, int containerId) { return containerId > 0 ? ChestCapacity : PackCapacity(state); }

namespace Backpack
{
bool Offered(const State& state, ShopKind kind) { return kind == ShopKind::GeneralStore && !state.leatherBackpack; }

bool HasSaveSection(const State& state) { return state.leatherBackpack || !state.backpackShown; }

void WriteSaveSection(std::ostream& output, const State& state)
{
    output << SaveTag << ' ' << (state.leatherBackpack ? 1 : 0) << ' ' << (state.backpackShown ? 1 : 0) << '\n';
}

bool ReadSaveSection(std::istream& input, State& state)
{
    int owned = -1, shown = -1;
    if (!(input >> owned >> shown) || (owned != 0 && owned != 1) || (shown != 0 && shown != 1)) return false;
    state.leatherBackpack = owned == 1;
    state.backpackShown = shown == 1;
    return true;
}
}
}
