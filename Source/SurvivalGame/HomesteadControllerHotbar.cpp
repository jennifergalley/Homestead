#include "HomesteadController.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadHotbarLayout.h"
#include "Simulation/HomesteadPail.h"
#include "UI/SHomesteadHotbar.h"
#include "UI/SHomesteadHudScale.h"
#include "UI/SHomesteadVitals.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Misc/ScopeExit.h"
#include "Widgets/Layout/SBox.h"

using HomesteadControllerHelpers::FindPlotWhere;
using HomesteadControllerHelpers::HotbarIcon;
using HomesteadControllerHelpers::IsFoodItem;
using HomesteadControllerHelpers::IsHotbarTool;
using HomesteadControllerHelpers::PlantingCrop;
using HomesteadControllerText::Text;

void AHomesteadController::ShowHotbar()
{
    if (HotbarRoot.IsValid() || !GEngine || !GEngine->GameViewport) return;
    HotbarWidget = SNew(SHomesteadHotbar).Controller(this);
    HotbarRoot = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            return ShouldShowHotbar()
                ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed;
        })
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Bottom)
        [
            // Sized in Canvas HUD units, like the calendar and minimap, so it keeps its proportion at 4K.
            SNew(HomesteadMenus::SHomesteadHudScale)
            [
                SNew(SBox).Padding(0, 0, 0, 22)
                [
                    HotbarWidget.ToSharedRef()
                ]
            ]
        ];
    GEngine->GameViewport->AddViewportWidgetContent(HotbarRoot.ToSharedRef(), 50);
    // Unlike the hotbar, the vitals stay up while she places a plan.
    VitalsRoot = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            return bWorldReady && !bBookOpen && !IsFailed() && !ShopScreen.IsValid() && !HasNativeMenu()
                && !IsNewGameSetup() && !IsNamingSetup() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
        })
        [
            SNew(HomesteadMenus::SHomesteadVitals).Controller(this)
        ];
    GEngine->GameViewport->AddViewportWidgetContent(VitalsRoot.ToSharedRef(), 50);
}

void AHomesteadController::HideHotbar()
{
    HoveredHotbarSlot = INDEX_NONE;
    if (HotbarRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(HotbarRoot.ToSharedRef());
    if (VitalsRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(VitalsRoot.ToSharedRef());
    VitalsRoot.Reset();
    HotbarWidget.Reset();
    HotbarRoot.Reset();
}

bool AHomesteadController::ShouldShowHotbar() const
{
    return bWorldReady && !bBookOpen && !bPlanning && !IsFailed() && !ShopScreen.IsValid();
}

void AHomesteadController::ResetHotbar()
{
    HotbarSlots.Init(-1, 10);
    // In the order she hafts them: the billhook first, for the bramble at the door.
    HotbarSlots[0] = static_cast<int32>(Homestead::Item::Billhook);
    HotbarSlots[1] = static_cast<int32>(Homestead::Item::Hatchet);
    HotbarSlots[2] = static_cast<int32>(Homestead::Item::Scythe);
    HotbarSlots[3] = static_cast<int32>(Homestead::Item::Pickaxe);
    HotbarSlots[4] = static_cast<int32>(Homestead::Item::DiggingStick);
    HotbarSlots[5] = static_cast<int32>(Homestead::Item::WateringCan);
    HotbarSlots[6] = static_cast<int32>(Homestead::Item::Berries);
    HotbarSlots[7] = static_cast<int32>(Homestead::Item::OilLamp);
    SelectedHotbarSlot = 0;
    HoveredHotbarSlot = INDEX_NONE;
}

void AHomesteadController::SanitizeHotbar(const TArray<int32>& Slots, int32 Selected, int32 Layout)
{
    // The rules (and their native tests) live in Simulation/HomesteadHotbarLayout.h. Copied element
    // by element: an old save's empty array has no data pointer to take a range from.
    std::vector<int> Saved;
    Saved.reserve(Slots.Num());
    for (const int32 Value : Slots) Saved.push_back(Value);
    const auto Clean = Homestead::SanitizeHotbarLayout(Saved, Layout);
    HotbarSlots.Init(-1, 10);
    for (int32 Index = 0; Index < 10; ++Index) HotbarSlots[Index] = Clean[Index];
    SelectedHotbarSlot = FMath::Clamp(Selected, 0, 9);
}

bool AHomesteadController::CanPinToHotbar(Homestead::Item Item)
{
    return Homestead::CanPinToHotbar(Item);
}

void AHomesteadController::PinNewSeed(Homestead::Item Item)
{
    // Bought or given crop seed goes straight onto the hotbar, ready to sow: into a free slot, or
    // else into the slot of a seed she has run out of.
    const auto* Crop = Homestead::CropForSeed(Item);
    if (!Crop || Item == Homestead::Item::Berries || IsPinnedToHotbar(Item)) return;
    if (HotbarSlots.IndexOfByKey(-1) != INDEX_NONE)
    {
        TogglePinnedToHotbar(Item);
        return;
    }
    for (int32& Slot : HotbarSlots)
        if (Slot >= 0 && static_cast<Homestead::Item>(Slot) != Homestead::Item::Berries
            && Homestead::CropForSeed(static_cast<Homestead::Item>(Slot)) && Sim.Count(static_cast<Homestead::Item>(Slot)) <= 0)
        {
            Slot = static_cast<int32>(Item);
            return;
        }
}

bool AHomesteadController::IsPinnedToHotbar(Homestead::Item Item) const
{
    return HotbarSlots.Contains(static_cast<int32>(Item));
}

bool AHomesteadController::TogglePinnedToHotbar(Homestead::Item Item)
{
    const FString Name = UTF8_TO_TCHAR(Homestead::ItemName(Item));
    if (!CanPinToHotbar(Item))
    {
        Notify(TEXT("Only tools, food and seeds can go on the hotbar."), true);
        return false;
    }
    const int32 Value = static_cast<int32>(Item);
    const int32 Pinned = HotbarSlots.IndexOfByKey(Value);
    if (Pinned != INDEX_NONE)
    {
        HotbarSlots[Pinned] = -1;
        Notify(Name + TEXT(" unpinned from the hotbar."));
        return true;
    }
    // Food goes to the right-hand slots first, leaving 1-5 for tools.
    int32 Free = INDEX_NONE;
    for (int32 Step = 0; Step < 10 && Free == INDEX_NONE; ++Step)
    {
        const int32 Index = (Step + 5) % 10;
        if (HotbarSlots.IsValidIndex(Index) && HotbarSlots[Index] < 0) Free = Index;
    }
    if (Free == INDEX_NONE)
    {
        Notify(TEXT("The hotbar is full. Unpin something first."), true);
        return false;
    }
    HotbarSlots[Free] = Value;
    Notify(FString::Printf(TEXT("%s pinned to hotbar slot %d."), *Name, Free == 9 ? 0 : Free + 1));
    return true;
}

bool AHomesteadController::ChooseOnHotbar(Homestead::Item Item)
{
    if (!IsPinnedToHotbar(Item) && !TogglePinnedToHotbar(Item)) return false;
    SelectHotbarSlot(HotbarSlots.IndexOfByKey(static_cast<int32>(Item)));
    return SelectedHotbarSlot == HotbarSlots.IndexOfByKey(static_cast<int32>(Item));
}

void AHomesteadController::EatFromHotbar(Homestead::Item Food)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    // Every deliberate press is a mouthful, even mid-chew: the meal lands now and the one bite clip
    // already playing stands for them all (PlayEat won't restart or stack it).
    const double FoodBefore = State().hunger, EnergyBefore = State().energy;
    const auto Result = Sim.Eat(Food);
    // Success shows as the vitals' +N popups (MealGain); only refusals need words.
    NotifyResourceAction(Result, nullptr);
    if (!Result.ok) return;
    if (Avatar) Avatar->PlayEat(Food == Homestead::Item::Berries);
    MealGain.Food = State().hunger - FoodBefore;
    MealGain.Energy = State().energy - EnergyBefore;
    ++MealGain.Serial;
}

TArray<FHomesteadHotbarSlot> AHomesteadController::HotbarSnapshot() const
{
    TArray<FHomesteadHotbarSlot> Result;
    Result.Reserve(10);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        FHomesteadHotbarSlot Slot;
        Slot.Index = Index;
        Slot.Selected = Index == SelectedHotbarSlot;
        if (HotbarSlots.IsValidIndex(Index) && HotbarSlots[Index] >= 0)
        {
            Slot.Tool = static_cast<Homestead::Item>(HotbarSlots[Index]);
            Slot.Assigned = CanPinToHotbar(Slot.Tool);
            Slot.Food = IsFoodItem(Slot.Tool);
            Slot.Count = Slot.Assigned ? Sim.Count(Slot.Tool) : 0;
            Slot.Available = Slot.Assigned && Slot.Count > 0;
            Slot.Icon = HotbarIcon(Slot.Tool);
            if (Slot.Tool == Homestead::Item::OilLamp && Slot.Available)
                Slot.Fill = static_cast<float>(Sim.LampOil() / Homestead::Lamp::CapacityHours);
            // The pail's water shows on the pail (HomesteadPail.h), like the lamp's oil.
            if (Slot.Tool == Homestead::Item::WateringCan && Slot.Available)
                if (const auto Pail = Homestead::PresentPail(State()); Pail.gauge)
                    Slot.Fill = static_cast<float>(Pail.charge) / Homestead::PailCapacity;
            Slot.Seed = IsSowingSeed(Slot.Tool);
            Slot.Pouch = Slot.Seed && OtherPouchSeeds(Index) > 0;
        }

        Result.Add(Slot);
    }
    return Result;
}

bool AHomesteadController::KnifePreviewRequested() const
{
    return PresentedTool() == Homestead::Item::Knife;
}

Homestead::Item AHomesteadController::PresentedTool() const
{
    if (!ShouldShowHotbar()) return Homestead::Item::Count;
    const int32 Slot = HoveredHotbarSlot != INDEX_NONE ? HoveredHotbarSlot : SelectedHotbarSlot;
    if (!HotbarSlots.IsValidIndex(Slot) || HotbarSlots[Slot] < 0) return Homestead::Item::Count;
    const auto Tool = static_cast<Homestead::Item>(HotbarSlots[Slot]);
    return IsHotbarTool(Tool) && Sim.Count(Tool) > 0 ? Tool : Homestead::Item::Count;
}

Homestead::Item AHomesteadController::SelectedCarriedTool() const
{
    if (!HotbarSlots.IsValidIndex(SelectedHotbarSlot) || HotbarSlots[SelectedHotbarSlot] < 0) return Homestead::Item::Count;
    const auto Tool = static_cast<Homestead::Item>(HotbarSlots[SelectedHotbarSlot]);
    return IsHotbarTool(Tool) && Sim.Count(Tool) > 0 ? Tool : Homestead::Item::Count;
}

void AHomesteadController::SelectHotbarSlot(int32 Index)
{
    if (!ShouldShowHotbar() || Index < 0 || Index >= 10) return;
    if (Index != SelectedHotbarSlot)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
            Avatar->CancelAction(true);
    SelectedHotbarSlot = Index;
    const auto Snapshot = HotbarSnapshot();
    ToastText = !Snapshot[Index].Assigned ? FString(TEXT("Empty slot"))
        : Snapshot[Index].Available || IsHotbarTool(Snapshot[Index].Tool) ? Text(Homestead::ItemName(Snapshot[Index].Tool))
        : FString::Printf(TEXT("Empty slot (no %s left)"), *Text(Homestead::ItemName(Snapshot[Index].Tool)).ToLower());
    bToastError = false;
    ToastRemaining = 1.0f;
    PlayEffect(UIClick, 0.05f);
}

void AHomesteadController::CycleHotbar(int32 Direction)
{
    if (!ShouldShowHotbar() || Direction == 0) return;
    SelectHotbarSlot((SelectedHotbarSlot + (Direction > 0 ? 1 : 9)) % 10);
}

void AHomesteadController::UseSelectedTool()
{
    // While planning, the left mouse button or right trigger places the piece, as E / A does.
    if (bPlanning && !bBookOpen) { Interact(); return; }
    if (!ShouldShowHotbar() || !HotbarSlots.IsValidIndex(SelectedHotbarSlot)) return;
    const int32 ToolValue = HotbarSlots[SelectedHotbarSlot];
    // Seeds on bare tilled soil: plant them there. A selected berry is always eaten (X / F sows
    // berry seed into a bare plot), so she can snack beside her own garden.
    if (ToolValue >= 0 && static_cast<Homestead::Item>(ToolValue) != Homestead::Item::Berries)
        if (const auto Crop = PlantingCrop(static_cast<Homestead::Item>(ToolValue)))
        {
            if (Sim.Count(static_cast<Homestead::Item>(ToolValue)) <= 0)
            {
                Notify(FString::Printf(TEXT("No %s left. Choose another seed on the hotbar."),
                    *Text(Homestead::ItemName(static_cast<Homestead::Item>(ToolValue))).ToLower()), true);
                return;
            }
            if (bWorldReady && PrepareWorldAt(PlayerPoint()))
            {
                UpdateFocus();
                const auto* Bare = Focus == EFocus::Plot
                    ? FindPlotWhere(State().plots, [this](const Homestead::Plot& Plot) { return Plot.id == FocusId; })
                    : nullptr;
                if (Bare && !Bare->planted)
                {
                    PlantFocusedPlot(*Crop);
                    return;
                }
            }
            Notify(TEXT("Aim at bare tilled soil to sow it."), true);
            return;
        }
    if (ToolValue >= 0 && IsFoodItem(static_cast<Homestead::Item>(ToolValue)))
    {
        const auto Food = static_cast<Homestead::Item>(ToolValue);
        if (Sim.Count(Food) <= 0)
            Notify(FString::Printf(TEXT("No %s left in your pack."), UTF8_TO_TCHAR(Homestead::ItemName(Food))), true);
        else EatFromHotbar(Food);
        return;
    }
    if (ToolValue < 0 || !IsHotbarTool(static_cast<Homestead::Item>(ToolValue)))
    {
        Notify(TEXT("Choose a carried tool first."), true);
        return;
    }
    const auto Tool = static_cast<Homestead::Item>(ToolValue);
    if (Sim.Count(Tool) <= 0)
    {
        Notify(TEXT("That tool is not in your pack."), true);
        return;
    }
    const auto Position = PlayerPoint();
    if (!bWorldReady || !PrepareWorldAt(Position)) return;
    UpdateFocus();
    const FHintUse Hint = BeginHintUse(bGamepad ? TEXT("RT") : TEXT("LMB"));
    ON_SCOPE_EXIT { EndHintUse(Hint); };

    if (Tool == Homestead::Item::OilLamp)
    {
        StartLampSetDown();
        return;
    }
    if (Tool == Homestead::Item::Hatchet && Focus == EFocus::Resource)
        for (const auto& Node : State().resources)
            if (Node.id == FocusId && Node.kind == Homestead::ResourceKind::ForestTree)
            {
                // Standing trees keep the axe's felling presentation.
                const Homestead::Point Target = Node.position;
                const int32 Cleared = FocusId;
                auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
                const bool bFell = Avatar && Avatar->CanFell();
                const auto Result = Sim.Clear(FocusId, Position);
                NotifyResourceAction(Result, bFell ? nullptr : WoodTapB.Get());
                if (Result.ok && Avatar) PresentFelling(Cleared, Target, true);
                return;
            }
    if (Tool == Homestead::Item::Hatchet || Tool == Homestead::Item::Billhook
        || Tool == Homestead::Item::Scythe || Tool == Homestead::Item::Pickaxe)
    {
        SwingAtOvergrowth(Tool);
        return;
    }

    if (Tool == Homestead::Item::WateringCan)
    {
        if (Focus == EFocus::Water)
        {
            FillPailAtStream(Position);
            return;
        }
        if (Focus != EFocus::Plot)
        {
            Notify(TEXT("Aim at a growing crop or stand by the stream."), true);
            return;
        }
        for (const auto& Plot : State().plots)
            if (Plot.id == FocusId)
            {
                const auto Result = Sim.Water(FocusId, Position);
                Notify(Result, GrassStepB);
                if (Result.ok)
                    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                        Avatar->PlayWater(Homestead::PlotCenter(Plot));
                return;
            }
    }

    if (Tool == Homestead::Item::DiggingStick) HoeSquareAhead();
}
