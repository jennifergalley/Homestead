// Eating from the hotbar with the interact buttons: with food selected and nothing in front of her to
// use them on, A / E (Interact) and X / F (Secondary) eat one, as RT / LMB always has. A focused
// chest, drop, plot, weed, fire, bed, door or shopkeeper keeps its own action, so she never eats by
// accident. One press eats one, even while she's still chewing the last (the bite clip isn't
// restarted), and the key bindings fire on press only, so a held button never repeats.
#include "HomesteadController.h"

#include "Simulation/HomesteadItems.h"

Homestead::Item AHomesteadController::SelectedHotbarFood() const
{
    if (!ShouldShowHotbar() || HotbarItem(SelectedHotbarSlot) == Homestead::Item::Count)
        return Homestead::Item::Count;
    const auto Item = HotbarItem(SelectedHotbarSlot);
    return Homestead::IsEdible(Item) ? Item : Homestead::Item::Count;
}

bool AHomesteadController::EatSelectedFoodInstead()
{
    const auto Food = SelectedHotbarFood();
    if (Food == Homestead::Item::Count) return false;
    if (Sim.Count(Food) <= 0)
        Notify(FString::Printf(TEXT("No %s left in your pack."), *FString(UTF8_TO_TCHAR(Homestead::ItemName(Food))).ToLower()), true);
    else EatFromHotbar(Food);
    return true;
}
