// The world's seasonal look (rework-farming-calendar-and-period-crafting, lane D): writes MPC_Season from
// the simulation clock. Materials that read it: the foliage parents (M_PropFoliage, M_CameraSafeFoliage;
// Scripts/Environment/build_season_materials.py) and the Weather lane's landscape and grass materials.

#include "HomesteadWorld.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Simulation/HomesteadSeasons.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadSeasons, Log, All);

namespace HomesteadSeasonLook
{
const TCHAR* const CollectionPath = TEXT("/Game/SurvivalGame/Environment/Seasons/MPC_Season.MPC_Season");
// The look is re-written at most every quarter of a game hour: frost eases out over a morning, and
// the season blends over days, so finer steps would be invisible.
constexpr double WriteStepHours = 0.25;

TAutoConsoleVariable<float> CVarSeasonDay(TEXT("homestead.SeasonDay"), -1.0f,
    TEXT("Playtest and screenshot aid: show the seasonal look of this day of the year (0-111; "
         "Spring 1 is 0, Summer 1 28, Autumn 1 56, Winter 1 84) at the current time of day. -1 follows the calendar."));
TAutoConsoleVariable<float> CVarFrost(TEXT("homestead.Frost"), -1.0f,
    TEXT("Playtest aid: force the morning frost (0-1). -1 follows the calendar."));
}

void AHomesteadWorld::UpdateSeasonLook(const Homestead::State& State)
{
    using namespace HomesteadSeasonLook;
    if (!State.fixedEstate) return;
    if (!SeasonCollection)
    {
        SeasonCollection = LoadObject<UMaterialParameterCollection>(nullptr, CollectionPath);
        if (!SeasonCollection)
        {
            UE_LOG(LogHomesteadSeasons, Error, TEXT("The seasonal look is missing %s; the estate stays in its default season."), CollectionPath);
            return;
        }
    }
    UWorld* World = GetWorld();
    UMaterialParameterCollectionInstance* Instance = World ? World->GetParameterCollectionInstance(SeasonCollection) : nullptr;
    if (!Instance) return;

    const float OverrideDay = CVarSeasonDay.GetValueOnGameThread();
    const float OverrideFrost = CVarFrost.GetValueOnGameThread();
    double Hour = State.hour;
    if (OverrideDay >= 0.0f)
        Hour = Homestead::Calendar::DayStartHour + FMath::Floor(OverrideDay) * 24.0
            + FMath::Fmod(State.hour - Homestead::Calendar::DayStartHour + 240.0, 24.0);
    // Overrides take effect at once; the calendar is followed in quarter-hour steps.
    const double Key = FMath::Floor(Hour / WriteStepHours) + (OverrideDay >= 0.0f ? 1e7 : 0.0) + OverrideFrost * 1e5;
    if (Key == LastSeasonLookHour) return;
    LastSeasonLookHour = Key;

    const Homestead::Seasons::Look Look = Homestead::Seasons::LookAt(Hour);
    Instance->SetScalarParameterValue(TEXT("SeasonBlend"), static_cast<float>(Look.seasonBlend));
    Instance->SetScalarParameterValue(TEXT("Autumn"), static_cast<float>(Look.autumn));
    Instance->SetScalarParameterValue(TEXT("WinterBare"), static_cast<float>(Look.winterBare));
    Instance->SetScalarParameterValue(TEXT("Frost"), OverrideFrost >= 0.0f ? FMath::Clamp(OverrideFrost, 0.0f, 1.0f)
        : static_cast<float>(Look.frost));
}
