// rework-farming-calendar-and-period-crafting (lane A): the hunger and season-change toasts.
#include "HomesteadController.h"

#include "Simulation/HomesteadCalendar.h"

void AHomesteadController::TickCalendarNotices()
{
    // Hunger toasts fire once each time she drops into a hungrier band; eating resets them.
    const Homestead::HungerState Hunger = Sim.GetHungerState();
    if (static_cast<int32>(Hunger) > static_cast<int32>(SeenHunger) && !IsFailed())
        Notify(Hunger == Homestead::HungerState::Famished
            ? TEXT("You're famished. Everything's slower until you eat.")
            : TEXT("You're getting hungry. Eat something soon."));
    SeenHunger = Hunger;

    if (Sim.SeasonChanges() > SeenSeasonChanges)
    {
        const auto& Change = Sim.LastSeasonChange();
        FString Text = FString::Printf(TEXT("%s has come."), UTF8_TO_TCHAR(Homestead::Calendar::SeasonName(Change.to)));
        if (Change.witheredPlots > 0)
            Text += FString::Printf(TEXT(" %d %s withered with the change of season. Hoe them out to replant."),
                Change.witheredPlots, Change.witheredPlots == 1 ? TEXT("plot") : TEXT("plots"));
        Notify(Text);
    }
    SeenSeasonChanges = Sim.SeasonChanges();
}
