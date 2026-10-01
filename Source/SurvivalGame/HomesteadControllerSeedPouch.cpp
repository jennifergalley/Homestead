// The seed pouch: a hotbar cell holding sowing seed swaps in each other seed stack from her pack
// (D-pad up/down on a gamepad, Q / Shift+Q on the keyboard), so newly bought seed is one press
// away however full the hotbar is. Sowing still takes the seed shown in the selected slot, and only
// while she has some (the crops lane's rule, AHomesteadController::Interact on a bare plot).
#include "HomesteadController.h"
#include "Simulation/HomesteadAudioLevels.h"

#include "HomesteadCharacter.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadPackRow.h"

namespace SeedPouch
{
// Every seed type she could sow, in catalogue order, that she has at least one of and that isn't
// already in another hotbar cell.
TArray<Homestead::Item> Choices(const AHomesteadController& Controller, const Homestead::Simulation& Sim, int32 Cell)
{
    TArray<Homestead::Item> Result;
    for (int32 Value = 0; Value < static_cast<int32>(Homestead::Item::Count); ++Value)
    {
        const auto Item = static_cast<Homestead::Item>(Value);
        if (!AHomesteadController::IsSowingSeed(Item) || Sim.Count(Item) <= 0) continue;
        bool Elsewhere = false;
        for (int32 Other = 0; Other < Homestead::PackRowSize; ++Other)
            Elsewhere = Elsewhere || (Other != Cell && Controller.HotbarItem(Other) == Item);
        if (!Elsewhere) Result.Add(Item);
    }
    return Result;
}
}

bool AHomesteadController::IsSowingSeed(Homestead::Item Item)
{
    // Berries plant a bush too, but on the hotbar they're food (eaten with the use button).
    return Item != Homestead::Item::Berries && Homestead::CropForSeed(Item) != nullptr;
}

int32 AHomesteadController::OtherPouchSeeds(int32 SlotIndex) const
{
    const auto Current = HotbarItem(SlotIndex);
    if (Current == Homestead::Item::Count || !IsSowingSeed(Current)) return 0;
    const auto Choices = SeedPouch::Choices(*this, Sim, SlotIndex);
    return Choices.Num() - (Choices.Contains(Current) ? 1 : 0);
}

bool AHomesteadController::CycleSeedPouch(int32 Direction)
{
    if (!ShouldShowHotbar() || Direction == 0) return false;
    const auto Current = HotbarItem(SelectedHotbarSlot);
    if (Current == Homestead::Item::Count || !IsSowingSeed(Current)) return false;
    const auto Choices = SeedPouch::Choices(*this, Sim, SelectedHotbarSlot);
    const int32 At = Choices.IndexOfByKey(Current);
    if (Choices.Num() == 0 || (Choices.Num() == 1 && At == 0))
    {
        Notify(TEXT("No other seed in your pack."), true);
        return true;
    }
    const int32 Count = Choices.Num();
    const int32 Next = At == INDEX_NONE ? (Direction > 0 ? 0 : Count - 1)
        : (At + (Direction > 0 ? 1 : Count - 1)) % Count;
    const auto Chosen = Choices[Next];
    // The row holds real stacks: the chosen seed's stack comes up from her pack into this cell and
    // the seed that was here goes down in its place.
    int32 Group = 0;
    for (const auto& Entry : State().inventoryLayout)
        if (Entry.wearableId == 0 && Entry.item == Chosen) { Group = Entry.groupId; break; }
    const auto Moved = Sim.MoveToPackRow(Group, 0, SelectedHotbarSlot, Sim.GetRevision());
    if (!Moved) { Notify(Moved); return true; }
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CancelAction(true);
    ToastText = FString::Printf(TEXT("%s (%d)  %d of %d"), UTF8_TO_TCHAR(Homestead::ItemName(Chosen)),
        Sim.Count(Chosen), Next + 1, Count);
    bToastError = false;
    ToastRemaining = 1.4f;
    PlayEffect(UIClick, Homestead::AudioLevels::Gain::UIClickFaint);
    return true;
}

void AHomesteadController::NextSeed()
{
    if (bBookOpen || bPlanning || IsFailed()) return;
    const bool Shift = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
    if (!CycleSeedPouch(Shift ? -1 : 1) && ShouldShowHotbar())
        Notify(TEXT("Select a seed slot on the hotbar to switch seed."), true);
}

FString AHomesteadController::SeedPouchHint() const
{
    if (OtherPouchSeeds(SelectedHotbarSlot) <= 0) return FString();
    return FString(TEXT("   ")) + (bGamepad ? TEXT("[D-pad]") : TEXT("[Q]")) + TEXT(" Other seed");
}
