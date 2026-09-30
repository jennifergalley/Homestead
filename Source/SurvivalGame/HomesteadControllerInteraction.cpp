#include "HomesteadController.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "UI/SHomesteadMenu.h"

#include "Engine/World.h"
#include "Misc/ScopeExit.h"

using HomesteadControllerHelpers::FindPlotWhere;
using HomesteadControllerHelpers::PlantingCrop;
using HomesteadControllerText::Text;

void AHomesteadController::Interact()
{
    if (IsFailed()) { RetryCheckpoint(); return; }
    if (bBookOpen) { ActivateRow(); return; }
    const auto Position = PlayerPoint();
    if (!bWorldReady || !PrepareWorldAt(Position)) return;
    if (bPlanning)
    {
        if (bDeconstructing)
        {
            UpdateDeconstruct(true);
            const int32 Removed = DeconstructId;
            const auto Result = Sim.Deconstruct(Removed, Position);
            Notify(Result, WoodTapB);
            if (Result.ok)
            {
                if (ActiveChestId.IsSet() && ActiveChestId.GetValue() == Removed) ActiveChestId.Reset();
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayGather();
                Sim.AdvanceGameHours(0.1, Position);
            }
            UpdateDeconstruct(true);
            return;
        }
        UpdatePlacement(true);
        const auto Result = Sim.Place(BuildTarget, Position);
        Notify(Result, WoodTapA);
        if (Result.ok) Sim.AdvanceGameHours(0.1, Position);
        UpdatePlacement(true);
        return;
    }
    UpdateFocus();
    const FHintUse Hint = BeginHintUse(bGamepad ? TEXT("A") : TEXT("E"));
    ON_SCOPE_EXIT { EndHintUse(Hint); };
    switch (Focus)
    {
    case EFocus::Resource:
    {
        bool Forage = false;
        bool Tree = false;
        bool Reeds = false;
        bool Sticks = false;
        auto Kind = Homestead::ResourceKind::Count;
        Homestead::Point ActionTarget = Position;
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Tree = Node.kind == Homestead::ResourceKind::ForestTree;
                Reeds = Node.kind == Homestead::ResourceKind::Reeds;
                Sticks = Node.kind == Homestead::ResourceKind::Branches;
                Kind = Node.kind;
                Forage = !Tree && Node.kind != Homestead::ResourceKind::Sapling;
                ActionTarget = Node.position;
                break;
            }
        const int32 Harvested = FocusId;
        const auto* Feller = Cast<AHomesteadCharacter>(GetPawn());
        const bool bFell = Tree && Feller && Feller->CanFell();
        const auto Result = Sim.Harvest(FocusId, Position);
        // Salvage and fallen boughs say what she found; ordinary forage shows it in her hands instead.
        if (Homestead::IsOvergrowth(Kind)) Notify(Result, WoodTapA);
        else NotifyResourceAction(Result, bFell ? nullptr : Tree ? WoodTapB.Get() : GrassStepA.Get());
        if (Result.ok && Forage)
            if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                if (Reeds)
                {
                    // Kneel, gather the stems in her left fist and saw them free with the knife.
                    if (Avatar->PlayKneelGather(EHomesteadKneelGather::Reeds, FVector2D(ActionTarget.x, ActionTarget.y)) && Landscape)
                    {
                        Landscape->HoldProduce(FocusId);
                        HeldStickPile = FocusId;
                        HeldStickPileSince = GetWorld()->GetTimeSeconds();
                        HeldPartsFirst = 0;
                        HeldPartsCount = 1;
                    }
                    else if (!Avatar->IsCuttingReeds()) Avatar->PlayKnifeCut(ActionTarget);
                }
                else if (Kind == Homestead::ResourceKind::DeerRemains)
                    Avatar->PlayKnifeCut(ActionTarget); // Work the dried hide free with the knife.
                else if (Sticks || Kind == Homestead::ResourceKind::Stones || Kind == Homestead::ResourceKind::Roots
                    || Kind == Homestead::ResourceKind::BerryBush || Kind == Homestead::ResourceKind::FallenBranch
                    || Kind == Homestead::ResourceKind::SalvagePile || Kind == Homestead::ResourceKind::Weeds
                    || Kind == Homestead::ResourceKind::Nettles || Homestead::IsRubbish(Kind))
                {
                    const bool Berries = Kind == Homestead::ResourceKind::BerryBush;
                    // A fallen bough gathered by hand is broken into sticks, and so are the rotten
                    // boards of a crate, barrel or plank pile; searching a salvage pile or a midden
                    // lifts its stones aside, so it plays the stone gather. Weeds are pulled like roots.
                    const bool Boards = Kind == Homestead::ResourceKind::BrokenCrate || Kind == Homestead::ResourceKind::BrokenBarrel
                        || Kind == Homestead::ResourceKind::RottenPlanks;
                    const auto Gather = Sticks || Kind == Homestead::ResourceKind::FallenBranch || Boards ? EHomesteadKneelGather::Sticks
                        : Kind == Homestead::ResourceKind::Stones || Kind == Homestead::ResourceKind::SalvagePile
                            || Kind == Homestead::ResourceKind::RubbishHeap || Kind == Homestead::ResourceKind::SlateHeap
                        ? EHomesteadKneelGather::Stones : EHomesteadKneelGather::Pouch;
                    FVector2D Target(ActionTarget.x, ActionTarget.y);
                    // Berries are picked from the near side of the bush, not its centre; an estate
                    // blackberry bramble is a metre across, so she reaches in at its edge.
                    if (Berries)
                    {
                        const bool Bramble = FocusId >= Homestead::EstatePlacementIdBase && FocusId < Homestead::TransientResourceIdBase;
                        const FVector2D Toward = FVector2D(Position.x, Position.y) - Target;
                        if (Toward.Size() > 1.0f) Target += Toward.GetSafeNormal() * (Bramble ? 62.0f : 22.0f);
                    }
                    // Rubbish is a metre or two across: she works at its near edge, not kneeling in it.
                    else if (Homestead::IsRubbish(Kind))
                    {
                        const bool BigHeap = Kind == Homestead::ResourceKind::RubbishHeap && FocusId >= 570000 && FocusId < 570008;
                        const float Edge = BigHeap ? 95.0f : Kind == Homestead::ResourceKind::BrokenBarrel ? 55.0f
                            : Kind == Homestead::ResourceKind::RottenPlanks ? 50.0f
                            // A slate heap is nearly three metres by two: she stacks from its edge.
                            : Kind == Homestead::ResourceKind::SlateHeap ? 100.0f : 45.0f;
                        const FVector2D Toward = FVector2D(Position.x, Position.y) - Target;
                        if (Toward.Size() > 1.0f) Target += Toward.GetSafeNormal() * FMath::Clamp(Toward.Size() - 30.0f, 0.0f, Edge);
                    }
                    if (Avatar->PlayKneelGather(Gather, Target, Berries) && Landscape)
                    {
                        Landscape->HoldProduce(FocusId);
                        HeldStickPile = FocusId;
                        HeldStickPileSince = GetWorld()->GetTimeSeconds();
                        // Sticks and stones: component 1 is the first one lifted. Berries: the
                        // first half of the bush's clusters. Roots: the one root crown.
                        HeldPartsFirst = Berries ? 0 : Gather == EHomesteadKneelGather::Pouch ? 0 : 1;
                        HeldPartsCount = Berries ? 4 : 1;
                    }
                }
                else Avatar->PlayGather();
        if (Result.ok && Tree) PresentFelling(Harvested, ActionTarget, true);
        break;
    }
    case EFocus::Drop:
        // The lamp is taken up with a kneel; it reaches her hand when her fingers close on the bail.
        if (const auto* Lamp = Sim.SetDownLampDrop(); Lamp && Lamp->id == FocusId && StartLampPickUp(FocusId)) break;
        Notify(Sim.PickUpDrop(FocusId, Position));
        break;
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            const bool Planted = Plot.planted;
            const bool Mature = Plot.growth >= 1;
            if (!Planted)
            {
                // The seed stack chosen on the hotbar (a chosen berry sows berry seed). A chosen seed
                // that has run out says so, and with no seed chosen nothing is sown: nothing is ever
                // taken from the pack unasked (wild roots too are chosen as Seeds on the hotbar).
                TOptional<Homestead::CropKind> Seed;
                if (HotbarItem(SelectedHotbarSlot) != Homestead::Item::Count)
                {
                    const auto Chosen = HotbarItem(SelectedHotbarSlot);
                    if (const auto Crop = PlantingCrop(Chosen))
                    {
                        if (Sim.Count(Chosen) <= 0)
                        {
                            Notify(FString::Printf(TEXT("No %s left. Choose another seed on the hotbar."),
                                *FString(UTF8_TO_TCHAR(Homestead::ItemName(Chosen))).ToLower()), true);
                            break;
                        }
                        Seed = Crop;
                    }
                }
                if (!Seed)
                {
                    Notify(TEXT("Choose seeds on the hotbar to sow."), true);
                    break;
                }
                PlantFocusedPlot(*Seed);
                break;
            }
            const Homestead::CropKind Harvested = Plot.kind;
            const Homestead::Point Center = Homestead::PlotCenter(Plot);
            const auto Result = Mature ? Sim.HarvestCrop(FocusId, Position) : Sim.Water(FocusId, Position);
            // A harvest shows as its "+N" pickups beside her; watering and refusals still say so.
            if (Mature) NotifyResourceAction(Result, GrassStepB);
            else Notify(Result, GrassStepB);
            if (Result.ok && Mature) PresentHarvest(FocusId, Harvested, Center);
            if (Result.ok && !Mature)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                    Avatar->PlayWater(Center);
            break;
        }
        break;
    case EFocus::Fire:
    case EFocus::Hearth:
        OpenBook(1);
        Selection = static_cast<int32>(Homestead::Recipe::RoastedRoots);
        if (NativeMenu && !NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe, Selection, 0))
            Notify(TEXT("The cookfire recipe could not be selected."), true);
        break;
    case EFocus::Bed: SleepAtBed(Position); break;
    case EFocus::Chest: OpenChestStorage(FocusId); break;
    case EFocus::Water: FillPailAtStream(Position); break;
    case EFocus::Underbrush: StartMacheteHack(); break;
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: InteractWithStore(); break;
    default: if (!EatSelectedFoodInstead()) Notify(TEXT("Walk closer to a plant, resource, or work area.")); break;
    }

}

void AHomesteadController::OpenFocusedChestWithMouse()
{
    if (bBookOpen || bPlanning || IsFailed() || !bWorldReady) return;
    UpdateFocus();
    if (Focus == EFocus::Chest) OpenChestStorage(FocusId);
}

bool AHomesteadController::OpenChestStorage(int32 ChestId)
{
    if (bPlanning || IsFailed() || ChestId <= 0) return false;
    const auto Position = PlayerPoint();
    const Homestead::Structure* Target = nullptr;
    for (const auto& Structure : State().structures)
        if (Structure.id == ChestId && Structure.kind == Homestead::Piece::Chest)
        { Target = &Structure; break; }
    if (!Target)
    { Notify(TEXT("That storage chest is no longer available."), true); return false; }
    const auto Center = Homestead::StructureCenter(State(), *Target);
    if (FMath::Square(Center.x - Position.x) + FMath::Square(Center.y - Position.y)
        > FMath::Square(Homestead::ChestReach))
    { Notify(TEXT("Move within 280 cm of this chest."), true); return false; }
    ActiveChestId = ChestId;
    MenuInventoryViewIndex = 1;
    OpenBook(0);
    return true;
}

void AHomesteadController::Secondary()
{
    if (IsFailed()) return;
    if (bBookOpen)
    {
        if (Page != 0) return;
        const auto Items = Rows();
        if (!Items.IsValidIndex(Selection)) return;
        const int ChestId = Sim.FindNearestStructure(PlayerPoint(), Homestead::Piece::Chest, 280);
        if (ChestId < 0) { Notify(TEXT("Stand near a storage chest to put items away."), true); return; }
        const auto Item = static_cast<Homestead::Item>(Items[Selection].Id);
        Notify(Sim.Transfer(ChestId, Item, 1, PlayerPoint()));
        Selection = FMath::Clamp(Selection, 0, FMath::Max(0, Rows().Num() - 1));
        return;
    }
    if (bPlanning) { ToggleDeconstruct(); return; }
    if (!bWorldReady || !PrepareWorldAt(PlayerPoint())) return;
    UpdateFocus();
    const FHintUse Hint = BeginHintUse(bGamepad ? TEXT("X") : TEXT("F"));
    ON_SCOPE_EXIT { EndHintUse(Hint); };
    if (Focus == EFocus::Resource)
    {
        bool Sapling = false;
        Homestead::Point ActionTarget = PlayerPoint();
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Sapling = Node.kind == Homestead::ResourceKind::ForestTree;
                ActionTarget = Node.position;
                break;
            }
        const int32 Cleared = FocusId;
        auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
        const bool bFell = Sapling && Avatar && Avatar->CanFell();
        const auto Result = Sim.Clear(FocusId, PlayerPoint());
        NotifyResourceAction(Result, bFell ? nullptr : WoodTapB.Get());
        if (Result.ok && Avatar)
        {
            if (Sapling) PresentFelling(Cleared, ActionTarget, true);
            else Avatar->PlayClear(ActionTarget);
        }
    }
    else if (Focus == EFocus::Plot)
    {
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            if (!Plot.planted && !Homestead::HasVisibleWeeds(Plot))
            {
                // X / F only ever weeds (Jenny): clean bare soil has none, and nothing is sown by accident.
                Notify(TEXT("No weeds to pull here. Choose seeds on the hotbar and press ")
                    + FString(UsesGamepad() ? TEXT("A") : TEXT("E")) + TEXT(" to sow."), true);
                break;
            }
            const auto Result = Sim.Weed(FocusId, PlayerPoint());
            Notify(Result, GrassStepA);
            if (Result.ok)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->PlayGather();
            break;
        }
    }
    else if (Focus == EFocus::Fire) Notify(Sim.AddFuel(FocusId, PlayerPoint()), WoodTapA);
    else if (SelectedCarriedTool() == Homestead::Item::OilLamp) MenuRefillLamp();
    else if (Focus == EFocus::None && EatSelectedFoodInstead()) {}
    else HoeSquareAhead();
}
