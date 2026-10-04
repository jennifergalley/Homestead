#include "../Source/SurvivalGame/Simulation/HomesteadFlowerCover.h"

#include <cstdio>

namespace FlowerCoverTests
{
int Failures = 0;
void Check(bool ok, const char* message)
{
    if (!ok)
    {
        std::printf("FAIL: %s\n", message);
        ++Failures;
    }
}
}

int main()
{
    using FlowerCoverTests::Check;
    const auto& layout = Homestead::ProvisionalEstateLayout();
    Homestead::State state;
    const Homestead::Point centre = Homestead::GardenCellCenter(-280, -730);
    Check(!Homestead::FlowerCoverMask(state, layout).Excludes(centre, 60), "outside farm is initially flowerable");
    state.plots.push_back({1, -280, -730, false, 0.0, 0.35, 0.0});
    Homestead::FlowerCoverMask mask(state, layout);
    Check(mask.Excludes(centre, 60), "bare tilled square excludes flowers");
    Check(mask.Excludes({centre.x + 110, centre.y}, 60), "scaled clump touches square edge from outside");
    Check(!mask.Excludes({centre.x + 111, centre.y}, 60), "unaffected neighbouring pocket is retained");
    Check(!mask.Excludes({centre.x + 100, centre.y + 100}, 60), "circle outside square corner is retained");
    Check(mask.Excludes({centre.x + 100, centre.y + 100}, 71), "scaled clump touches square corner");
    Check(mask.Excludes({-19200, -67500}, 20), "farm interior excluded");
    Check(mask.Excludes({-22250, -67500}, 50), "farm edge excludes whole visible footprint");
    Check(mask.Excludes({-25000, -65000}, 20), "manor ruins excluded");
    Check(mask.Excludes({-25950, -65000}, 50), "manor edge excludes whole footprint");
    Check(!mask.Excludes({-25800, -63800}, 20), "concave manor notch is not incorrectly filled");
    state.plots.front().planted = true;
    state.plots.front().growth = 1.0;
    state.plots.front().withered = true;
    Check(Homestead::FlowerCoverMask(state, layout).Excludes(centre, 60), "crop identity/stage does not affect cultivation");
    state.plots.clear();
    Check(!Homestead::FlowerCoverMask(state, layout).Excludes(centre, 60), "derived mask restores flowers when plot is removed");
    Homestead::Simulation simulation;
    Homestead::EstatePlacements placements;
    Check(simulation.NewEstateGame(layout, placements).ok, "isolated Estate fixture");
    Check(simulation.GrantItems(Homestead::Item::DiggingStick, 1).ok, "fixture hoe");
    Check(simulation.Till(-280, -730, centre).ok, "till outside farm through real simulation command");
    Check(Homestead::FlowerCoverMask(simulation.GetState(), layout).Excludes(centre, 60), "real till excludes flower");
    const std::string save = simulation.Serialize();
    Homestead::Simulation loaded;
    loaded.SetLayout(layout);
    loaded.SetPlacements(placements);
    Check(loaded.Deserialize(save).ok, "current save round trip");
    Check(Homestead::FlowerCoverMask(loaded.GetState(), loaded.Layout()).Excludes(centre, 60), "outside plot exclusion survives save/load");
    Check(!Homestead::FlowerCoverMask(loaded.GetState(), loaded.Layout()).Excludes({centre.x + 300, centre.y}, 60),
        "uncultivated neighbour survives save/load");
    std::printf("HomesteadFlowerCoverTests: %d failures\n", FlowerCoverTests::Failures);
    return FlowerCoverTests::Failures == 0 ? 0 : 1;
}
