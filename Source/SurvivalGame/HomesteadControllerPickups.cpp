// "+3 Berries": what she has just gained, floated beside her (UI/SHomesteadPickups). One path for
// every way of getting things - gathering, clearing, harvest, crafting, the shop - by watching her
// counts rather than each action: a gain raises both the pack and everything she owns (pack, chests
// and things set down), while taking from a chest or picking her own drop back up raises only the
// pack, so moves never show. Loads and new games resync quietly.
#include "HomesteadController.h"

#include "UI/SHomesteadPickups.h"

namespace PickupCounts
{

void Count(const Homestead::State& State, Homestead::Inventory& Pack, Homestead::Inventory& Owned)
{
    Pack = State.inventory;
    Owned = State.inventory;
    for (const auto& Piece : State.structures)
        for (int32 Index = 0; Index < Homestead::ItemCount; ++Index) Owned[Index] += Piece.storage[Index];
    for (const auto& Drop : State.worldDrops)
        if (static_cast<int32>(Drop.item) >= 0 && static_cast<int32>(Drop.item) < Homestead::ItemCount)
            Owned[static_cast<int32>(Drop.item)] += Drop.quantity;
}
}

bool AHomesteadController::PickupsVisible() const
{
    return bWorldReady && !bPendingSpawn && !bBookOpen && !IsFailed() && !ShopScreen.IsValid() && !HasNativeMenu()
        && !IsNewGameSetup() && !IsNamingSetup();
}

bool AHomesteadController::PickupAnchor(FVector2D& Pixel, FVector2D& ViewportPixels) const
{
    const APawn* Heroine = GetPawn();
    int32 Width = 0, Height = 0;
    GetViewportSize(Width, Height);
    if (!Heroine || Width <= 0 || Height <= 0) return false;
    ViewportPixels = FVector2D(Width, Height);
    // Chest height, so the line sits beside her rather than over her head or feet.
    return ProjectWorldLocationToScreen(Heroine->GetActorLocation() + FVector(0, 0, 30), Pixel, true);
}

void AHomesteadController::UpdatePickups(float DeltaSeconds)
{
    const bool bVisible = PickupsVisible();
    // Lines wait while the book or shop is up, so a crafted or bought thing still shows after.
    if (bVisible)
        for (int32 Index = Pickups.Num() - 1; Index >= 0; --Index)
            if ((Pickups[Index].Shown += DeltaSeconds) >= HomesteadPickupTiming::Seconds) Pickups.RemoveAt(Index);
    if (!bWorldReady || bPendingSpawn || !bPickupsPrimed)
    {
        // A load, new game or the first frame: take the counts as they are, and show nothing.
        PickupCounts::Count(State(), PickupPack, PickupOwned);
        PickupRevision = Sim.GetRevision();
        bPickupsPrimed = bWorldReady && !bPendingSpawn;
        if (!bPickupsPrimed) Pickups.Reset();
        return;
    }
    if (Sim.GetRevision() == PickupRevision) return;
    PickupRevision = Sim.GetRevision();
    Homestead::Inventory Pack, Owned;
    PickupCounts::Count(State(), Pack, Owned);
    for (int32 Index = 0; Index < Homestead::ItemCount; ++Index)
    {
        const auto Item = static_cast<Homestead::Item>(Index);
        // Pail water shows on the pail (its fill), not as a pickup.
        if (Item == Homestead::Item::Water) continue;
        const int32 Gain = FMath::Min(Pack[Index] - PickupPack[Index], Owned[Index] - PickupOwned[Index]);
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
    PickupPack = Pack;
    PickupOwned = Owned;
}
