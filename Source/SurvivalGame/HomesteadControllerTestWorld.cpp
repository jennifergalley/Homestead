// Agent test tools: set the weather, open or close a screen by name, and reset to a known state.
// Time of day is HomesteadMorning <hour>; items are HomesteadGive; the lamp is HomesteadLampOil.
// Everything logs to LogHomesteadPlaytest. Recipes: .github\skills\homestead-test-tools\SKILL.md.
#include "HomesteadController.h"
#include "HomesteadPlaytestSupport.h"
#include "HomesteadCharacter.h"
#include "Simulation/HomesteadRain.h"
#include "Simulation/HomesteadShops.h"

namespace TestWorld
{
// Search step and horizon when looking for a clear or wet hour to jump to.
constexpr double SearchStepHours = 0.25;
constexpr double SearchHorizonHours = 24.0 * 28.0;
// Each SkipToHourOfDay lands on the next occurrence of an hour, so a far target needs a few of them.
constexpr int32 MaxSkips = 40;
// How far into a spell the "rain" jump lands, so the rain is well under way but not about to end.
constexpr double IntoSpellHours = 0.5;
// The hour a reset moves the clock to when it is night.
constexpr float MorningHour = 8.0f;
// The lamp's full tank, in game hours (Simulation/HomesteadLamp.h CapacityHours).
constexpr float FullLampHours = 6.0f;
// Chest reach for the "chest" screen, matching Withdraw.
constexpr double ChestReachCm = 280.0;

// Skips the clock forward to an absolute game hour (the sim only skips to the next hour of the day).
void SkipToHour(Homestead::Simulation& Sim, double Target)
{
    for (int32 Skip = 0; Skip < MaxSkips && Sim.GetState().hour < Target - 1e-6; ++Skip)
        Sim.SkipToHourOfDay(FMath::Fmod(Target, 24.0));
}

struct FScreenName
{
    const TCHAR* Name;
    int32 BookPage;
};

const FScreenName BookScreens[] = {
    {TEXT("inventory"), 0}, {TEXT("pack"), 0}, {TEXT("craft"), 1}, {TEXT("build"), 2},
    {TEXT("settings"), 4}, {TEXT("appearance"), 6}, {TEXT("map"), 7},
};
}

void AHomesteadController::HomesteadWeather(const FString& Kind)
{
    if (RejectPendingGroundSnapAction()) return;
    const FString Wanted = HomesteadPlaytest::Compact(Kind);
    const double Now = State().hour;
    const bool bRain = Wanted == TEXT("rain") || Wanted == TEXT("wet") || Wanted == TEXT("storm");
    const bool bClear = Wanted == TEXT("clear") || Wanted == TEXT("dry") || Wanted == TEXT("sun") || Wanted == TEXT("sunny");
    if (!bRain && !bClear)
    {
        UE_LOG(LogHomesteadPlaytest, Warning, TEXT("WEATHER: unknown '%s'; use rain or clear"), *Kind);
        Notify(TEXT("Weather: use rain or clear."), true);
        return;
    }
    double Target = Now;
    bool bFound = false;
    if (bRain)
    {
        Homestead::RainSpell Spell;
        if (Homestead::IsRainingAt(Now)) bFound = true;
        else if (Homestead::NextRainSpell(Now, Spell))
        {
            Target = Spell.start + FMath::Min(TestWorld::IntoSpellHours, (Spell.end - Spell.start) * 0.5);
            bFound = true;
        }
    }
    else
    {
        const auto IsClear = [](double Hour) { return Homestead::Overcast(Hour) <= 0.0 && Homestead::GroundWetness(Hour) <= 0.0; };
        if (IsClear(Now)) bFound = true;
        else
        {
            for (double Hour = Now + TestWorld::SearchStepHours; Hour < Now + TestWorld::SearchHorizonHours; Hour += TestWorld::SearchStepHours)
                if (IsClear(Hour)) { Target = Hour; bFound = true; break; }
        }
    }
    if (!bFound)
    {
        UE_LOG(LogHomesteadPlaytest, Warning, TEXT("WEATHER: no %s hour found within %.0f days"), *Kind, TestWorld::SearchHorizonHours / 24.0);
        Notify(TEXT("No such weather found soon."), true);
        return;
    }
    TestWorld::SkipToHour(Sim, Target);
    RefreshRemaining = 0;
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("WEATHER: %s -> hour %.2f raining=%d overcast=%.2f wetness=%.2f (skipped %.2f h)"),
        *Kind, State().hour, Homestead::IsRainingAt(State().hour) ? 1 : 0, Homestead::Overcast(State().hour),
        Homestead::GroundWetness(State().hour), State().hour - Now);
}

void AHomesteadController::HomesteadScreen(const FString& Name)
{
    if (RejectPendingGroundSnapAction()) return;
    const FString Wanted = HomesteadPlaytest::Compact(Name);
    const auto Describe = [this](const TCHAR* Result)
    {
        UE_LOG(LogHomesteadPlaytest, Display, TEXT("SCREEN: %s book=%d page=%d shop=%d chest=%d planning=%d title='%s' summary='%s' toast='%s'"),
            Result, IsBookOpen() ? 1 : 0, BookPage(), IsShopScreenOpen() ? 1 : 0, ActiveStorageChest().IsSet() ? 1 : 0, IsPlanning() ? 1 : 0,
            IsBookOpen() ? *BookTitle() : TEXT(""), IsBookOpen() ? *BookSummary() : TEXT(""), *Toast());
    };
    if (Wanted.IsEmpty() || Wanted == TEXT("close") || Wanted == TEXT("none") || Wanted == TEXT("game"))
    {
        if (ShopScreen.IsValid()) CloseShopScreen();
        if (bBookOpen) CloseBook();
        EndPlacement();
        Describe(TEXT("closed"));
        return;
    }
    if (Wanted == TEXT("shop") || Wanted == TEXT("generalstore") || Wanted == TEXT("store"))
    {
        const Homestead::Shop* Shop = Sim.FindShop(Homestead::ShopKind::GeneralStore);
        if (!Shop) { Describe(TEXT("no general store")); return; }
        OpenShopScreen(Shop->id, false);
        Describe(IsShopScreenOpen() ? TEXT("shop opened") : TEXT("shop refused (closed? use HomesteadMorning 9)"));
        return;
    }
    if (Wanted == TEXT("chest") || Wanted == TEXT("storage"))
    {
        const int32 ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, TestWorld::ChestReachCm);
        if (ChestId < 0) { Notify(TEXT("There is no storage chest nearby."), true); Describe(TEXT("no chest within reach")); return; }
        if (bBookOpen) CloseBook();
        OpenChestStorage(ChestId);
        Describe(TEXT("chest opened"));
        return;
    }
    for (const TestWorld::FScreenName& Screen : TestWorld::BookScreens)
    {
        if (Wanted != Screen.Name) continue;
        if (ShopScreen.IsValid()) CloseShopScreen();
        OpenBook(Screen.BookPage);
        Describe(TEXT("book opened"));
        return;
    }
    UE_LOG(LogHomesteadPlaytest, Warning, TEXT("SCREEN: unknown '%s'; use close, inventory, craft, build, settings, appearance, map, shop or chest"), *Name);
    Notify(TEXT("Unknown screen name."), true);
}

void AHomesteadController::HomesteadReset()
{
    if (RejectPendingGroundSnapAction()) return;
    if (ShopScreen.IsValid()) CloseShopScreen();
    if (bBookOpen) CloseBook();
    EndPlacement();
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CancelAction(true);
    Notify(Sim.SetEnergy(100.0));
    Sim.SetLampOil(TestWorld::FullLampHours);
    if (Sim.IsNight()) Sim.SkipToHourOfDay(TestWorld::MorningHour);
    ToastRemaining = 0;
    RefreshRemaining = 0;
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("RESET: energy=%.0f lampOil=%.1f hour=%.2f; screens closed. Items, purse and placements are untouched."),
        State().energy, Sim.LampOil(), State().hour);
}
