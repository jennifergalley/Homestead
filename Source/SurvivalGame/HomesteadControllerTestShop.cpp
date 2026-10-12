// Playtest aid: HomesteadSellAtStore. Opens the General Store's hours, stands her at the real counter
// in the village, sells a list of carried items through the same Simulation::Sell the shop screen
// uses, opens the shop screen and logs the purse delta and per-item prices to LogHomesteadPlaytest.
// Works in Development builds as well as the editor. Items resolve by name or key, so goods added on
// other branches work without changes here.
#include "HomesteadController.h"
#include "HomesteadPlaytestSupport.h"
#include "HomesteadCharacter.h"
#include "HomesteadGeneralStore.h"
#include "Simulation/HomesteadShops.h"

#include "Engine/World.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY(LogHomesteadPlaytest);

namespace TestShop
{
// Game hour of day the shop is brought to when it is closed; the shop opens at 8 and closes at 18.
constexpr double OpenAtHourOfDay = 9.0;
// A closed shop is skipped forward at most this many days (to get past Sunday).
constexpr int32 MaxDaysToSkip = 7;
// How far inside the door she stands, in cm from the counter; within Homestead::CounterReach (400).
constexpr double StandFromCounterCm = 250.0;
using HomesteadPlaytest::ToText;
}

using HomesteadPlaytest::FindItem;

void AHomesteadController::HomesteadSellAtStore(const FString& SellList)
{
    if (RejectPendingGroundSnapAction()) return;
    GetWorldTimerManager().ClearTimer(TestTeleportTimer);
    TestShopSales.Reset();

    const Homestead::Shop* Shop = Sim.FindShop(Homestead::ShopKind::GeneralStore);
    if (!Shop)
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("SELLAT: this map has no village General Store. Use HomesteadOpenStore here."));
        Notify(TEXT("There is no village General Store on this map. Try HomesteadOpenStore."), true);
        return;
    }
    if (!GetPawn())
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("SELLAT: no pawn to teleport."));
        return;
    }

    // "Coal 3,CopperOre 2;Quartz:2": a name (spaces allowed) then an optional quantity (default 1).
    TArray<FString> Parts;
    SellList.Replace(TEXT("\""), TEXT("")).ParseIntoArray(Parts, TEXT(","));
    TArray<FString> Semicolons;
    for (const FString& Part : Parts)
    {
        TArray<FString> Pieces;
        Part.ParseIntoArray(Pieces, TEXT(";"));
        Semicolons.Append(Pieces);
    }
    for (const FString& Entry : Semicolons)
    {
        TArray<FString> Words;
        Entry.Replace(TEXT(":"), TEXT(" ")).Replace(TEXT("="), TEXT(" ")).ParseIntoArrayWS(Words);
        if (Words.IsEmpty()) continue;
        int32 Quantity = 1;
        if (Words.Num() > 1 && Words.Last().IsNumeric())
        {
            Quantity = FCString::Atoi(*Words.Last());
            Words.Pop();
        }
        const FString Name = FString::Join(Words, TEXT(" "));
        Homestead::Item Item = Homestead::Item::Count;
        if (Quantity <= 0 || !FindItem(Name, Item))
        {
            UE_LOG(LogHomesteadPlaytest, Warning, TEXT("SELLAT: skipping \"%s\" (no item by that name, or quantity %d is not positive)."),
                *Entry, Quantity);
            Notify(FString::Printf(TEXT("No item called %s."), *Name), true);
            continue;
        }
        TestShopSales.Add({Item, Quantity});
    }
    if (TestShopSales.IsEmpty())
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("SELLAT: nothing to sell in \"%s\". Format: \"Coal 3,CopperOre 2\"."), *SellList);
        return;
    }

    // Bring the shop's hours round first so the door is open by the time she arrives.
    for (int32 Day = 0; Day <= TestShop::MaxDaysToSkip && !Homestead::IsShopOpen(*Shop, State().hour); ++Day)
        Sim.SkipToHourOfDay(TestShop::OpenAtHourOfDay);
    RefreshRemaining = 0;
    if (!Homestead::IsShopOpen(*Shop, State().hour))
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("SELLAT: could not bring the General Store's hours round (hour %.1f)."), State().hour);
        Notify(TEXT("The General Store would not open. See LogHomesteadPlaytest."), true);
        TestShopSales.Reset();
        return;
    }
    TickStores(0.0f);

    // The real door is 600 cm out from the counter along its facing; she stands just inside it, facing the counter.
    const double FacingRadians = FMath::DegreesToRadians(Shop->counterYaw);
    const double StandX = Shop->counterX + FMath::Cos(FacingRadians) * TestShop::StandFromCounterCm;
    const double StandY = Shop->counterY + FMath::Sin(FacingRadians) * TestShop::StandFromCounterCm;
    TestShopId = Shop->id;
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("SELLAT: General Store %d open (hour %.1f). Counter (%.0f, %.0f), standing at (%.0f, %.0f), %d item kinds to sell."),
        TestShopId, State().hour, Shop->counterX, Shop->counterY, StandX, StandY, TestShopSales.Num());

    const bool bStarted = BeginTestTeleport(StandX, StandY, FRotator(0.0, FMath::UnwindDegrees(Shop->counterYaw + 180.0), 0.0),
        TEXT("SELLAT"), [this]() { FinishTestShop(); });
    if (!bStarted) TestShopSales.Reset();
}

void AHomesteadController::FinishTestShop()
{
    const TArray<FTestShopSale> Sales = MoveTemp(TestShopSales);
    TestShopSales.Reset();

    const Homestead::Shop* Shop = Sim.FindShop(TestShopId);
    if (const AHomesteadGeneralStore* Store = Stores.IsEmpty() ? nullptr : Stores[0].Get())
        UE_LOG(LogHomesteadPlaytest, Display, TEXT("SELLAT: store door at (%.0f, %.0f); she is at (%.0f, %.0f)."),
            Store->DoorPoint().X, Store->DoorPoint().Y, PlayerPoint().x, PlayerPoint().y);
    if (!Shop || !Homestead::IsShopOpen(*Shop, State().hour))
    {
        UE_LOG(LogHomesteadPlaytest, Error, TEXT("SELLAT: the General Store is not open now (hour %.1f)."), State().hour);
        return;
    }

    const int64 PurseBefore = State().money;
    int32 Sold = 0;
    int32 Refused = 0;
    for (const FTestShopSale& Sale : Sales)
    {
        const FString Name = TestShop::ToText(Homestead::ItemName(Sale.Item));
        const int32 Carried = Sim.Count(Sale.Item);
        const int64 ListPrice = Homestead::SellPrice(Sale.Item);
        const int64 Before = State().money;
        const Homestead::Result Result = ShopTrade(TestShopId, Sale.Item, Sale.Quantity, true, false);
        const int64 Gained = State().money - Before;
        if (Result.ok)
        {
            ++Sold;
            UE_LOG(LogHomesteadPlaytest, Display, TEXT("SELLAT: SOLD %d x %s (carried %d): list price %s each, purse %s (%s each)."),
                Sale.Quantity, *Name, Carried, *TestShop::ToText(Homestead::FormatMoney(ListPrice)),
                *TestShop::ToText(Homestead::FormatMoneyDelta(Gained)),
                *TestShop::ToText(Homestead::FormatMoney(Gained / Sale.Quantity)));
        }
        else
        {
            ++Refused;
            UE_LOG(LogHomesteadPlaytest, Warning, TEXT("SELLAT: REFUSED %d x %s (carried %d, list price %s): %s"),
                Sale.Quantity, *Name, Carried, *TestShop::ToText(Homestead::FormatMoney(ListPrice)), *TestShop::ToText(Result.message));
        }
    }

    const int64 Delta = State().money - PurseBefore;
    const FString Summary = FString::Printf(TEXT("Sold %d of %d item kinds: purse %s -> %s (%s)."), Sold, Sold + Refused,
        *TestShop::ToText(Homestead::FormatMoney(PurseBefore)), *TestShop::ToText(Homestead::FormatMoney(State().money)),
        *TestShop::ToText(Homestead::FormatMoneyDelta(Delta)));
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("SELLAT: %s purse_before=%lld purse_after=%lld delta=%lld"), *Summary,
        static_cast<long long>(PurseBefore), static_cast<long long>(State().money), static_cast<long long>(Delta));
    Notify(Summary, Sold == 0);
    OpenShopScreen(TestShopId, false);
    UE_LOG(LogHomesteadPlaytest, Display, TEXT("SELLAT: shop screen open=%d."), IsShopScreenOpen() ? 1 : 0);
}
