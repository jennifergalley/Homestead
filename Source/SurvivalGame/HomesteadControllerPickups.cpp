// "+3 Berries": what she has just gained, in the common HUD notice stack. One path for
// every way of getting things - gathering, clearing, harvest, crafting, the shop - by watching her
// counts rather than each action (Simulation/HomesteadHoldings.h decides what counts as a gain, so
// chest moves and picking her own drop back up never show). Loads and new games resync quietly.
#include "HomesteadController.h"

#include "UI/HomesteadPickupTiming.h"

bool AHomesteadController::PickupsVisible() const
{
    return bWorldReady && !bPendingSpawn && !bBookOpen && !IsFailed() && !ShopScreen.IsValid() && !HasNativeMenu()
        && !IsNewGameSetup() && !IsNamingSetup();
}

void AHomesteadController::UpdatePickups(float DeltaSeconds)
{
    const bool bVisible = PickupsVisible();
    // Lines wait while the book or shop is up, so a crafted or bought thing still shows after.
    if (bVisible)
        for (int32 Index = Pickups.Num() - 1; Index >= 0; --Index)
            if ((Pickups[Index].Shown += DeltaSeconds) >= HomesteadPickupTiming::Seconds) Pickups.RemoveAt(Index);
    // A revision that went backwards is a different simulation (a load, or a canceled change rolled
    // back), not something she gained.
    if (!bWorldReady || bPendingSpawn || !bPickupsPrimed || Sim.GetRevision() < PickupRevision)
    {
        // A load, new game or the first frame: take the counts as they are, and show nothing.
        PickupHoldings = Homestead::CountHoldings(State());
        PickupRevision = Sim.GetRevision();
        bPickupsPrimed = bWorldReady && !bPendingSpawn;
        if (!bPickupsPrimed) Pickups.Reset();
        return;
    }
    if (Sim.GetRevision() == PickupRevision) return;
    PickupRevision = Sim.GetRevision();
    const Homestead::Holdings Now = Homestead::CountHoldings(State());
    for (int32 Index = 0; Index < Homestead::ItemCount; ++Index)
    {
        const auto Item = static_cast<Homestead::Item>(Index);
        const int32 Gain = Homestead::PickupGain(PickupHoldings, Now, Item);
        if (Gain <= 0) continue;
        if (auto* Line = Pickups.FindByPredicate([Item](const FPickup& Value) { return Value.Item == Item; }))
        {
            Line->Amount += Gain;
            Line->Shown = FMath::Min(Line->Shown, HomesteadPickupTiming::FadeIn);
            continue;
        }
        if (Pickups.Num() >= HomesteadPickupTiming::MaxLines) Pickups.RemoveAt(0);
        Pickups.Add({Item, Gain, 0.0f});
    }
    PickupHoldings = Now;
}
