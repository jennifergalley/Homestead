#include "HomesteadController.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadGardenTarget.h"
#include "Simulation/HomesteadGatherPose.h"
#include "Simulation/HomesteadOvergrowth.h"
#include "UI/SHomesteadMenu.h"

#include "Engine/World.h"
#include "Misc/ScopeExit.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadInteract, Log, All);

using HomesteadControllerHelpers::FindPlotWhere;
using HomesteadControllerHelpers::PlantingCrop;
using HomesteadControllerText::Text;

void AHomesteadController::Interact()
{
    if (RejectPendingGroundSnapAction()) return;
    if (IsFailed()) { RetryCheckpoint(); return; }
    if (bBookOpen) { ActivateRow(); return; }
    const auto Position = PlayerPoint();
    if (!bWorldReady || !PrepareWorldAt(Position)) return;
    if (IsFishing()) return;
    if (bPlanning)
    {
        if (bDeconstructing)
        {
            UpdateDeconstruct(true);
            const int32 Removed = DeconstructId;
            const auto Result = Sim.Deconstruct(Removed, Position);
            NotifyResourceAction(Result, WoodTapB);
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
        NotifyResourceAction(Result, WoodTapA);
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
        auto Kind = Homestead::ResourceKind::Count;
        Homestead::Point ActionTarget = Position;
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Tree = Node.kind == Homestead::ResourceKind::ForestTree;
                Reeds = Node.kind == Homestead::ResourceKind::Reeds;
                Kind = Node.kind;
                Forage = !Tree && Node.kind != Homestead::ResourceKind::Sapling;
                ActionTarget = Node.position;
                break;
            }
        const int32 Harvested = FocusId;
        const Homestead::GatherPose Pose = Homestead::HandGatherPose(Kind);
        // A standing tree is felled with the axe on the tool button, never on E / A (Jenny 2026-09-30:
        // no crossover between interacting and using a tool).
        if (Tree) break;
        const auto* Feller = Cast<AHomesteadCharacter>(GetPawn());
        const bool bFell = Tree && Feller && Feller->CanFell();
        // Weeds and nettles are pulled on both knees and only count once the second root is out.
        if ((Kind == Homestead::ResourceKind::Weeds || Kind == Homestead::ResourceKind::Nettles)
            && StartWeedPull(FocusId, INDEX_NONE, ActionTarget))
            break;
        const auto Result = Sim.Harvest(FocusId, Position);
        // Salvage and fallen boughs say what she found; ordinary forage shows it in her hands instead.
        if (Homestead::IsOvergrowth(Kind)) NotifyResourceAction(Result, WoodTapA);
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
                else if (Pose == Homestead::GatherPose::KnifeCut)
                    Avatar->PlayKnifeCut(ActionTarget); // Work the dried hide free with the knife.
                else if (Pose == Homestead::GatherPose::Sticks || Pose == Homestead::GatherPose::Stones
                    || Pose == Homestead::GatherPose::Pouch)
                {
                    const bool Berries = Kind == Homestead::ResourceKind::BerryBush;
                    // Homestead::HandGatherPose: boughs and rotten boards are broken into sticks;
                    // stones, salvage, middens and slates are lifted with the stone kneel; anything
                    // that grows (roots, berries, flowers, weeds) is picked into the hip pouch.
                    const auto Gather = Pose == Homestead::GatherPose::Sticks ? EHomesteadKneelGather::Sticks
                        : Pose == Homestead::GatherPose::Stones ? EHomesteadKneelGather::Stones : EHomesteadKneelGather::Pouch;
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
                else
                    UE_LOG(LogHomesteadInteract, Warning, TEXT("Resource %d (%s) was gathered by hand but has no gather pose."),
                        Harvested, UTF8_TO_TCHAR(Homestead::ResourceName(Kind)));
        if (Result.ok && Tree) PresentFelling(Harvested, ActionTarget, true);
        break;
    }
    case EFocus::Drop:
        // The lamp is taken up with a kneel; it reaches her hand when her fingers close on the bail.
        if (const auto* Lamp = Sim.SetDownLampDrop(); Lamp && Lamp->id == FocusId && StartLampPickUp(FocusId)) break;
        NotifyResourceAction(Sim.PickUpDrop(FocusId, Position), nullptr);
        break;
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            const bool Planted = Plot.planted;
            const bool Mature = Plot.growth >= 1;
            if (!Planted)
            {
                // Only the seed stack chosen on the hotbar is sown (a chosen berry sows berry seed; wild
                // roots too are chosen as Seeds): nothing is ever taken from the pack unasked. A refusal is
                // CheckSow's, the reason the red outline shows; with no seed chosen, say which to select.
                const auto Chosen = HotbarItem(SelectedHotbarSlot);
                if (const auto Crop = PlantingCrop(Chosen))
                {
                    if (Sim.Count(Chosen) <= 0)
                    {
                        Notify(FString::Printf(TEXT("No %s left"),
                            *FString(UTF8_TO_TCHAR(Homestead::ItemName(Chosen))).ToLower()), true);
                        break;
                    }
                    PlantFocusedPlot(*Crop);
                }
                break;
            }
            const Homestead::CropKind Harvested = Plot.kind;
            const Homestead::Point Center = Homestead::PlotCenter(Plot);
            // E only harvests. A withered crop is hoed out and a growing one watered with the tool button
            // (the hoe or the pail), never on E / A (Jenny 2026-09-30).
            if (Plot.withered) break;
            if (!Mature)
            {
                Notify(TEXT("Not ready yet"), true);
                break;
            }
            const auto Result = Sim.HarvestCrop(FocusId, Position);
            // A harvest shows as its "+N" pickups beside her; refusals still say why.
            NotifyResourceAction(Result, GrassStepB);
            if (Result.ok) PresentHarvest(FocusId, Harvested, Center);
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
    case EFocus::Bed:
        if (bBedFocusActionable && !bBedSleepHeld) { bBedSleepHeld = true; SleepAtBed(Position); }
        break;
    case EFocus::Chest: OpenChestStorage(FocusId); break;
    // The pail is filled with the tool button (UseSelectedTool), not on E / A.
    case EFocus::Water: break;
    case EFocus::Underbrush: StartMacheteHack(); break;
    case EFocus::SceneryTree: StartSceneryFell(); break;
    case EFocus::SceneryStump: StartSceneryStumpClear(); break;
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: InteractWithStore(); break;
    case EFocus::RoadSign: InteractWithRoadSign(); break;
    default: EatSelectedFoodInstead(); break;
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
    if (RejectPendingGroundSnapAction()) return false;
    if (bPlanning || IsFailed() || ChestId <= 0) return false;
    const auto Position = PlayerPoint();
    const Homestead::Structure* Target = nullptr;
    for (const auto& Structure : State().structures)
        if (Structure.id == ChestId && Structure.kind == Homestead::Piece::Chest)
        { Target = &Structure; break; }
    if (!Target)
    { Notify(TEXT("That storage chest is no longer available."), true); return false; }
    const auto Center = Homestead::StructureFootprint(State(), *Target).center;
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
    if (RejectPendingGroundSnapAction()) return;
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
        // F / X pulls weeds and nettles by hand; it never fells, clears or hoes (that's the tool button).
        Homestead::ResourceKind Kind = Homestead::ResourceKind::Count;
        Homestead::Point ActionTarget = PlayerPoint();
        for (const auto& Node : State().resources)
            if (Node.id == FocusId) { Kind = Node.kind; ActionTarget = Node.position; break; }
        if (Kind != Homestead::ResourceKind::Weeds && Kind != Homestead::ResourceKind::Nettles) return;
        if (StartWeedPull(FocusId, INDEX_NONE, ActionTarget)) return;
        NotifyResourceAction(Sim.Harvest(FocusId, PlayerPoint()), GrassStepA);
    }
    else if (Focus == EFocus::Plot)
    {
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            // X / F only ever weeds (Jenny): with none showing it does nothing, and never sows.
            if (!Homestead::HasVisibleWeeds(Plot)) break;
            // By hand she kneels and pulls them, and the square is weeded when the second root is out;
            // without that clip she pulls them into the hip pouch like the estate's.
            if (StartWeedPull(INDEX_NONE, FocusId, Homestead::PlotCenter(Plot))) break;
            const auto Result = Sim.Weed(FocusId, PlayerPoint());
            NotifyResourceAction(Result, GrassStepA);
            if (Result.ok)
                if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
                    Avatar->PlayKneelGather(EHomesteadKneelGather::Pouch,
                        FVector2D(Homestead::PlotCenter(Plot).x, Homestead::PlotCenter(Plot).y));
            break;
        }
    }
    else if (Focus == EFocus::Fire) NotifyResourceAction(Sim.AddFuel(FocusId, PlayerPoint()), WoodTapA);
    else if (SelectedCarriedTool() == Homestead::Item::OilLamp) MenuRefillLamp();
    else if (Focus == EFocus::None) EatSelectedFoodInstead();
}
