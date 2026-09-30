#include "HomesteadController.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "HomesteadSave.h"
#include "Simulation/HomesteadHotbarLayout.h"
#include "Simulation/HomesteadLamp.h"
#include "Simulation/HomesteadPackRow.h"
#include "Simulation/HomesteadPail.h"
#include "UI/SHomesteadHotbar.h"
#include "UI/SHomesteadHudScale.h"
#include "UI/SHomesteadPickups.h"
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

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadHotbar, Log, All);

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
    // Painted over the world without layout or hit testing, so nothing shifts and clicks pass through.
    PickupsRoot = SNew(SBox)
        .Visibility_Lambda([this]()
        {
            return PickupsVisible() && !Pickups.IsEmpty() ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
        })
        [
            SNew(HomesteadMenus::SHomesteadPickups).Controller(this)
        ];
    GEngine->GameViewport->AddViewportWidgetContent(PickupsRoot.ToSharedRef(), 50);
}

void AHomesteadController::HideHotbar()
{
    HoveredHotbarSlot = INDEX_NONE;
    if (HotbarRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(HotbarRoot.ToSharedRef());
    if (VitalsRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(VitalsRoot.ToSharedRef());
    VitalsRoot.Reset();
    if (PickupsRoot.IsValid() && GEngine && GEngine->GameViewport)
        GEngine->GameViewport->RemoveViewportWidgetContent(PickupsRoot.ToSharedRef());
    PickupsRoot.Reset();
    HotbarWidget.Reset();
    HotbarRoot.Reset();
}

bool AHomesteadController::ShouldShowHotbar() const
{
    return bWorldReady && !bBookOpen && !bPlanning && !IsFailed() && !ShopScreen.IsValid();
}

Homestead::Item AHomesteadController::HotbarItem(int32 Cell) const
{
    const auto* Entry = HotbarEntry(Cell);
    return Entry && Entry->wearableId == 0 ? Entry->item : Homestead::Item::Count;
}

const Homestead::LayoutEntry* AHomesteadController::HotbarEntry(int32 Cell) const
{
    return Homestead::PackRowRules::RowEntry(State(), Cell);
}

int32 AHomesteadController::HotbarCellOf(Homestead::Item Item) const
{
    for (int32 Cell = 0; Cell < Homestead::PackRowSize; ++Cell)
        if (HotbarItem(Cell) == Item) return Cell;
    return INDEX_NONE;
}

int32 AHomesteadController::FirstEmptyHotbarCell() const
{
    for (int32 Cell = 0; Cell < Homestead::PackRowSize; ++Cell)
        if (State().packRow[Cell].Empty()) return Cell;
    return INDEX_NONE;
}

void AHomesteadController::ResetHotbar()
{
    // The row holds real stacks now (Simulation/HomesteadPackRow.h): she starts with the lamp on
    // key 8 and the cells before it free for the tools she hafts, which arrive in the first empty
    // cell. Anything else she starts with sits below the row.
    std::array<int, Homestead::PackRowSize> Start;
    Start.fill(-1);
    Start[7] = static_cast<int>(Homestead::Item::OilLamp);
    Sim.ArrangePackRow(Start);
    SelectedHotbarSlot = 0;
    HoveredHotbarSlot = INDEX_NONE;
}

void AHomesteadController::SanitizeHotbar(const TArray<int32>& Slots, int32 Selected, int32 Layout)
{
    SelectedHotbarSlot = FMath::Clamp(Selected, 0, Homestead::PackRowSize - 1);
    // A save that already carries the row (layout 4 on) needs nothing more.
    if (Layout >= UHomesteadSave::CurrentHotbarLayout) return;
    // The old pinned hotbar (Simulation/HomesteadHotbarLayout.h). Copied element by element: an old
    // save's empty array has no data pointer to take a range from.
    std::vector<int> Saved;
    Saved.reserve(Slots.Num());
    for (const int32 Value : Slots) Saved.push_back(Value);
    const auto Clean = Homestead::SanitizeHotbarLayout(Saved, Layout);
    std::array<int, Homestead::PackRowSize> Items;
    for (int32 Cell = 0; Cell < Homestead::PackRowSize; ++Cell) Items[Cell] = Clean[Cell];
    const auto Result = Sim.ArrangePackRow(Items);
    UE_LOG(LogHomesteadHotbar, Display, TEXT("Old pinned hotbar moved into the pack row (layout %d): %s"),
        Layout, UTF8_TO_TCHAR(Result.message.c_str()));
}

bool AHomesteadController::ChooseOnHotbar(Homestead::Item Item)
{
    int32 Cell = HotbarCellOf(Item);
    if (Cell == INDEX_NONE)
    {
        const int32 Free = FirstEmptyHotbarCell();
        const Homestead::LayoutEntry* Stack = nullptr;
        for (const auto& Entry : State().inventoryLayout)
            if (Entry.wearableId == 0 && Entry.item == Item) { Stack = &Entry; break; }
        if (Free == INDEX_NONE || !Stack || !Sim.MoveToPackRow(Stack->groupId, 0, Free, Sim.GetRevision())) return false;
        Cell = Free;
    }
    SelectHotbarSlot(Cell);
    return SelectedHotbarSlot == Cell;
}

void AHomesteadController::EatFromHotbar(Homestead::Item Food)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    // Every deliberate press is a mouthful, even mid-chew: the meal lands now and the one bite clip
    // already playing stands for them all (PlayEat won't restart or stack it).
    const double FoodBefore = State().hunger, EnergyBefore = State().energy;
    // The selected cell's own stack goes down (the row holds real stacks); otherwise any.
    const auto* Stack = HotbarEntry(SelectedHotbarSlot);
    const auto Result = Stack && Stack->wearableId == 0 && Stack->item == Food
        ? Sim.EatGroup(Stack->groupId, Sim.GetRevision()) : Sim.Eat(Food);
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
    // The HUD and the book ask many times a frame; build it once per frame, pack revision and
    // selection (the row names real stacks, so each cell is a lookup in her pack).
    if (HotbarSnapshotCache.Num() == Homestead::PackRowSize && HotbarSnapshotFrame == GFrameCounter
        && HotbarSnapshotRevision == Sim.GetRevision() && HotbarSnapshotSelected == SelectedHotbarSlot)
        return HotbarSnapshotCache;
    HotbarSnapshotCache = BuildHotbarSnapshot();
    HotbarSnapshotFrame = GFrameCounter;
    HotbarSnapshotRevision = Sim.GetRevision();
    HotbarSnapshotSelected = SelectedHotbarSlot;
    return HotbarSnapshotCache;
}

TArray<FHomesteadHotbarSlot> AHomesteadController::BuildHotbarSnapshot() const
{
    TArray<FHomesteadHotbarSlot> Result;
    Result.Reserve(10);
    for (int32 Index = 0; Index < 10; ++Index)
    {
        FHomesteadHotbarSlot Slot;
        Slot.Index = Index;
        Slot.Selected = Index == SelectedHotbarSlot;
        if (const auto* Entry = HotbarEntry(Index))
        {
            Slot.Assigned = true;
            Slot.Available = true;
            if (Entry->wearableId != 0)
            {
                Slot.Garment = true;
                if (const auto* Instance = Sim.GetWearable(Entry->wearableId))
                    if (const auto* Info = Homestead::GetWearableDefinition(Instance->definition))
                        Slot.Icon = Instance->definition == Homestead::WearableDefinition::LeatherShoes
                            ? FName(TEXT("leather-shoes")) : FName(UTF8_TO_TCHAR(Info->key));
                Result.Add(Slot);
                continue;
            }
            Slot.Tool = Entry->item;
            Slot.Food = IsFoodItem(Slot.Tool);
            Slot.Count = Entry->quantity;
            Slot.Icon = HotbarIcon(Slot.Tool);
            if (Slot.Tool == Homestead::Item::OilLamp)
                Slot.Fill = static_cast<float>(Sim.LampOil() / Homestead::Lamp::CapacityHours);
            // The pail's water shows on the pail (HomesteadPail.h), like the lamp's oil.
            if (Slot.Tool == Homestead::Item::WateringCan)
                if (const auto Pail = Homestead::PresentPail(State()); Pail.gauge)
                    Slot.Fill = static_cast<float>(Pail.charge) / Homestead::PailCapacity;
            Slot.Seed = IsSowingSeed(Slot.Tool);
            Slot.Pouch = Slot.Seed && OtherPouchSeeds(Index) > 0;
            Slot.Material = !Slot.Food && !Slot.Seed && !IsHotbarTool(Slot.Tool);
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
    const auto Tool = HotbarItem(Slot);
    return Tool != Homestead::Item::Count && IsHotbarTool(Tool) ? Tool : Homestead::Item::Count;
}

Homestead::Item AHomesteadController::SelectedCarriedTool() const
{
    const auto Tool = HotbarItem(SelectedHotbarSlot);
    return Tool != Homestead::Item::Count && IsHotbarTool(Tool) ? Tool : Homestead::Item::Count;
}

void AHomesteadController::SelectHotbarSlot(int32 Index)
{
    if (!ShouldShowHotbar() || Index < 0 || Index >= 10) return;
    if (Index != SelectedHotbarSlot)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
            Avatar->CancelAction(true);
    SelectedHotbarSlot = Index;
    FHomesteadRow Held;
    ToastText = MenuHotbarRow(Index, Held) ? Held.Name : FString(TEXT("Empty slot"));
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
    if (!ShouldShowHotbar() || SelectedHotbarSlot < 0 || SelectedHotbarSlot >= Homestead::PackRowSize) return;
    const auto Selected = HotbarItem(SelectedHotbarSlot);
    const int32 ToolValue = Selected == Homestead::Item::Count ? -1 : static_cast<int32>(Selected);
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
