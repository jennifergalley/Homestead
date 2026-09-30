// rework-farming-calendar-and-period-crafting (lane A): the season-change toast. The estate has no
// hunger (Simulation/HomesteadFood.h), so there are no hunger toasts.
#include "HomesteadController.h"

#include "Simulation/HomesteadCalendar.h"

void AHomesteadController::TickCalendarNotices()
{
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
