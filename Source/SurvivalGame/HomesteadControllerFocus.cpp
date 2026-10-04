#include "HomesteadController.h"
#include "HomesteadControllerConfig.h"
#include "HomesteadActionHints.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadBed.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadGardenTarget.h"
#include "Simulation/HomesteadItems.h"
#include "Simulation/HomesteadOvergrowth.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/ConfigCacheIni.h"

using HomesteadControllerConfig::ActionHintSection;
using HomesteadControllerConfig::PersistIntProperty;
using HomesteadControllerHelpers::IsHotbarTool;
using HomesteadControllerHelpers::SwingVerb;
using HomesteadControllerHelpers::ToolPrompt;
using HomesteadControllerHelpers::ToolWhereabouts;
using HomesteadControllerText::Text;

void AHomesteadController::UpdateFocus()
{
    const bool bHeldBed = Focus == EFocus::Bed;
    Focus = EFocus::None;
    FocusId = -1;
    bBedFocusActionable = false;
    const auto Position = PlayerPoint();
    double Best = 280.0;
    constexpr double FurnitureBroadphaseCm = 425.0; // 280 cm focus plus the largest furniture offset.
    Homestead::Point FocusTarget{};
    bool bHasFocusTarget = false;
    auto Consider = [&](EFocus Kind, int Id, Homestead::Point Target)
    {
        const double Distance = FMath::Sqrt(FMath::Square(Target.x - Position.x) + FMath::Square(Target.y - Position.y));
        if (Distance < Best)
        {
            Best = Distance; Focus = Kind; FocusId = Id;
            FocusTarget = Target; bHasFocusTarget = true;
        }
    };
    for (const auto& Node : State().resources)
        if (!Node.cleared) Consider(EFocus::Resource, Node.id, Node.position);
    for (const auto& Drop : State().worldDrops)
        Consider(EFocus::Drop, Drop.id, Drop.position);
    for (const auto& Plot : State().plots)
        Consider(EFocus::Plot, Plot.id, Homestead::PlotCenter(Plot));
    // In a garden of adjoining squares the nearest centre is ambiguous (and a weed at her feet can
    // win): the square her reach lands in, 60 cm ahead of her, takes the focus.
    if (const APawn* Avatar = GetPawn(); Avatar && !State().plots.empty())
    {
        const FVector Forward = Avatar->GetActorForwardVector();
        const int ReachX = Homestead::GardenCell(Position.x + Forward.X * Homestead::GardenReach::PailAheadCm);
        const int ReachY = Homestead::GardenCell(Position.y + Forward.Y * Homestead::GardenReach::PailAheadCm);
        for (const auto& Plot : State().plots)
            if (Plot.cellX == ReachX && Plot.cellY == ReachY)
            {
                Best = FMath::Min(Best, FMath::Sqrt(FMath::Square(Homestead::PlotCenter(Plot).x - Position.x)
                    + FMath::Square(Homestead::PlotCenter(Plot).y - Position.y)));
                Focus = EFocus::Plot;
                FocusId = Plot.id;
                FocusTarget = Homestead::PlotCenter(Plot);
                bHasFocusTarget = true;
                break;
            }
    }
    for (const auto& Structure : State().structures)
    {
        EFocus Kind = EFocus::None;
        if (Structure.kind == Homestead::Piece::Fire) Kind = EFocus::Fire;
        if (Structure.kind == Homestead::Piece::Hearth) Kind = EFocus::Hearth;
        if (Structure.kind == Homestead::Piece::Chest) Kind = EFocus::Chest;
        if (Kind != EFocus::None)
        {
            // The furthest furniture offset is 142 cm; avoid footprint/foundation scans far from her.
            const auto Cell = Homestead::StructureCenter(State(), Structure);
            if (FMath::Square(Cell.x - Position.x) + FMath::Square(Cell.y - Position.y) < FMath::Square(FurnitureBroadphaseCm))
                Consider(Kind, Structure.id, Homestead::StructureFootprint(State(), Structure).center);
        }
    }
    ConsiderStoreFocus(Consider);
    ConsiderRoadSignFocus(Consider);
    const EFocus BeforeHeldTool = Focus;
    const int BeforeHeldId = FocusId;
    FocusHeldToolTarget(Position);
    if (Focus != BeforeHeldTool || FocusId != BeforeHeldId) bHasFocusTarget = false;
    if (Sim.NearWater(Position))
    {
        // With the watering can out and not full, the stream wins over a crop on the bank when she
        // is at least as close to the water's edge, and always once the can is empty.
        const bool bCan = HotbarItem(SelectedHotbarSlot) == Homestead::Item::WateringCan
            && Sim.Count(Homestead::Item::WateringCan) > 0;
        const int32 Water = Sim.Count(Homestead::Item::Water);
        const double Edge = FMath::Max(0.0, WaterEdgeDistance(Position, false));
        if (Focus == EFocus::None || (bCan && Water < Homestead::PailPortions && (Water == 0 || Edge <= Best)))
        {
            Focus = EFocus::Water;
            FocusId = -1;
            bHasFocusTarget = false;
        }
    }
    if (UpdateFishingFocus(Position))
    {
        Focus = EFocus::Water;
        FocusId = -1;
        bHasFocusTarget = false;
    }
    // With the machete out, the nearest bush or bramble within arm's reach takes the focus.
    const bool bMachete = HotbarItem(SelectedHotbarSlot) == Homestead::Item::Machete
        && Sim.Count(Homestead::Item::Machete) > 0;
    AHomesteadWorld::FUnderbrushTarget Brush;
    if (bMachete && Landscape && Landscape->FindUnderbrushNear(Sim, FVector2D(Position.x, Position.y), 110.0f, Brush))
    {
        Focus = EFocus::Underbrush;
        FocusId = Brush.Index;
        FocusBrushChunk = Brush.Chunk;
        FocusBrushIndex = Brush.Index;
        FocusBrushSpecies = Brush.Species;
        FocusBrushPosition = Brush.Position;
        bFocusBrushWoody = Brush.bWoody;
        bHasFocusTarget = false;
    }
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ZeroVector;
    bool bOtherFacing = Focus != EFocus::None;
    if (bOtherFacing && bHasFocusTarget)
    {
        const double X = FocusTarget.x - Position.x, Y = FocusTarget.y - Position.y;
        const double Distance = FMath::Sqrt(X * X + Y * Y);
        bOtherFacing = Distance < 1e-6
            || (Forward.X * X + Forward.Y * Y) >= Homestead::BedFacingCosine * Distance;
    }
    const int Bed = Homestead::BedFocusCandidate(State(), Position, {Forward.X, Forward.Y}, bOtherFacing, bHeldBed);
    if (Bed != -1)
    {
        Focus = EFocus::Bed; FocusId = Bed;
        bBedFocusActionable = Homestead::ReachableBed(State(), Position, {Forward.X, Forward.Y}) == Bed;
    }
}

FString AHomesteadController::FocusTitle() const
{
    switch (Focus)
    {
    case EFocus::Resource:
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                FString Status;
                if (Node.kind == Homestead::ResourceKind::ForestTree && ToolWhereabouts(Sim, Homestead::Item::Hatchet) == 0)
                    Status = TEXT("  (axe needed)");
                else if (const auto* Overgrowth = Homestead::FindOvergrowth(Node.kind);
                    Overgrowth && Overgrowth->tool != Homestead::ToolKind::Count && !Overgrowth->byHand)
                {
                    const auto Needed = FMath::Max(Overgrowth->minTier, Node.minTier);
                    if (ToolWhereabouts(Sim, Homestead::ToolItem(Overgrowth->tool)) == 0)
                        Status = FString::Printf(TEXT("  (%s needed)"), UTF8_TO_TCHAR(Homestead::ToolName(Overgrowth->tool)));
                    else if (Sim.GetToolTier(Overgrowth->tool) < Needed)
                        Status = TEXT("  (") + Text(Homestead::NeedsToolMessage(Overgrowth->tool, Needed).c_str()).ToLower() + TEXT(")");
                }
                if (Node.kind == Homestead::ResourceKind::DeerRemains) return TEXT("Deer bones");
                return Text(Homestead::ResourceName(Node.kind)) + Status;
            }
        break;
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
        {
            if (Plot.id != FocusId) continue;
            if (!Plot.planted) return TEXT("Tilled soil");
            return Text(Homestead::PlotStatus(Plot, State().hour).c_str());
        }
        break;
    case EFocus::Drop:
        for (const auto& Drop : State().worldDrops)
            if (Drop.id == FocusId)
            {
                if (Drop.wearableId)
                {
                    const auto* Wearable = Sim.GetWearable(Drop.wearableId);
                    return Wearable ? Text(Homestead::WearableName(Wearable->definition))
                        : TEXT("Dropped garment");
                }
                if (Drop.item == Homestead::Item::OilLamp)
                    return Sim.LampOil() > 0.0 ? TEXT("Oil lamp") : TEXT("Oil lamp (out of oil)");
                return FString::Printf(TEXT("%s x%d"),
                    *Text(Homestead::ItemName(Drop.item)), Drop.quantity);
            }
        break;
    case EFocus::Fire: return TEXT("Cookfire");
    case EFocus::Hearth: return TEXT("Hearth");
    case EFocus::Bed: return TEXT("Bed");
    case EFocus::Chest: return ChestDisplayName(FocusId);
    case EFocus::Water:
        if (SelectedCarriedTool() == Homestead::Item::FishingPole)
            return FString(UTF8_TO_TCHAR(Homestead::Fishing::WaterName(FocusedFishingWater))) + TEXT(" fishing");
        return TEXT("Fresh stream water");
    case EFocus::Underbrush: return AHomesteadWorld::UnderbrushName(FocusBrushSpecies);
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: return StoreFocusTitle();
    case EFocus::RoadSign: return RoadSignTitle();
    default: break;
    }
    if (Focus == EFocus::None && SelectedCarriedTool() == Homestead::Item::OilLamp)
    {
        const double Oil = Sim.LampOil();
        return Oil <= 0.0 ? FString(TEXT("Oil lamp - out of oil"))
            : FString::Printf(TEXT("Oil lamp - %s left"), Oil >= 1.5 ? *FString::Printf(TEXT("%.0f hours"), FMath::RoundToDouble(Oil))
                : *FString::Printf(TEXT("%d minutes"), FMath::Max(1, FMath::RoundToInt(Oil * 60.0))));
    }
    return TEXT("Woodland");
}

FString AHomesteadController::FocusActions() const
{
    const FString A = bGamepad ? TEXT("[A]") : TEXT("[E]");
    const FString X = bGamepad ? TEXT("[X]") : TEXT("[F]");
    const FString Use = bGamepad ? TEXT("[RT]") : TEXT("[LMB]");
    Homestead::Item SelectedTool = Homestead::Item::Count;
    const bool ToolAvailable = HotbarItem(SelectedHotbarSlot) != Homestead::Item::Count
        && IsHotbarTool(SelectedTool = HotbarItem(SelectedHotbarSlot))
        && Sim.Count(SelectedTool) > 0;
    switch (Focus)
    {
    case EFocus::Resource:
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                if (Node.readyAtHour > State().hour) return FString();
                // Trees are felled with the axe on the tool button; nothing else is offered on them.
                if (Node.kind == Homestead::ResourceKind::ForestTree)
                    return ToolAvailable && SelectedTool == Homestead::Item::Hatchet ? Use + TEXT(" Fell") : FString();
                if (Node.kind == Homestead::ResourceKind::DeerRemains || Node.kind == Homestead::ResourceKind::Reeds)
                    return FString();
                if (const auto* Overgrowth = Homestead::FindOvergrowth(Node.kind))
                {
                    // Only a tool that clears this overgrowth offers a swing; the lamp (no ToolKind) never does.
                    const bool Handles = ToolAvailable && Homestead::ToolForItem(SelectedTool) != Homestead::ToolKind::Count
                        && Homestead::ToolForItem(SelectedTool) == Overgrowth->tool;
                    if (Node.kind == Homestead::ResourceKind::SalvagePile) return A + TEXT(" Search");
                    // Weeds and nettles are pulled, rubbish is cleared away, a fallen bough gathered.
                    const FString Hand = A + (Node.kind == Homestead::ResourceKind::Weeds || Node.kind == Homestead::ResourceKind::Nettles
                        ? TEXT(" Pull") : Homestead::IsRubbish(Node.kind) ? TEXT(" Clear away") : TEXT(" Gather"));
                    // Too worn for it: say which upgrade it needs rather than offering a swing that glances off.
                    const Homestead::ToolTier Needed = FMath::Max(Overgrowth->minTier, Node.minTier);
                    const bool bTooWorn = Overgrowth->tool != Homestead::ToolKind::Count && Sim.GetToolTier(Overgrowth->tool) < Needed;
                    if (Handles && !bTooWorn)
                        return (Overgrowth->byHand ? Hand + TEXT("   ") : FString()) + Use + TEXT(" ") + SwingVerb(SelectedTool);
                    if (Overgrowth->byHand) return Hand;
                    if (Handles && bTooWorn) return UTF8_TO_TCHAR(Homestead::NeedsToolMessage(Overgrowth->tool, Needed).c_str());
                    return FString();
                }
                // Loose stones are small enough to pick up by hand, unlike the rocks the pickaxe breaks.
                if (Node.kind == Homestead::ResourceKind::Stones) return A + TEXT(" Pick up");
                return A + TEXT(" Gather");
            }
        return A + TEXT(" Gather");
    case EFocus::Plot:
        for (const auto& Plot : State().plots)
            if (Plot.id == FocusId)
            {
                // Every action that would work now, interactions first ([E] plant or harvest, [F] pull
                // weeds), then the tool in hand (the pail waters, the hoe hoes out a withered crop). E
                // never waters or hoes (Jenny 2026-09-30).
                TArray<FString> Actions;
                const Homestead::Item Chosen = HotbarItem(SelectedHotbarSlot);
                if (!Plot.planted && Chosen != Homestead::Item::Count)
                    if (Homestead::CropForSeed(Chosen))
                    {
                        const auto Cue = Homestead::DescribeSow(Sim, Plot.id, PlayerPoint(), Chosen, {});
                        Actions.Add((Cue.keyed ? A + TEXT(" ") : FString()) + Text(Cue.text.c_str()));
                    }
                const auto HarvestCue = Homestead::DescribeHarvest(Plot);
                if (!HarvestCue.empty()) Actions.Add(A + TEXT(" ") + Text(HarvestCue.c_str()));
                if (Homestead::HasVisibleWeeds(Plot)) Actions.Add(X + TEXT(" Pull weeds"));
                if (Plot.planted && Plot.withered && ToolAvailable && SelectedTool == Homestead::Item::DiggingStick)
                    Actions.Add(Use + TEXT(" Hoe out"));
                if (Plot.planted && !Plot.withered && !Homestead::IsRipe(Plot) && Homestead::NeedsWater(Plot)
                    && ToolAvailable && SelectedTool == Homestead::Item::WateringCan)
                    Actions.Add(Sim.Count(Homestead::Item::Water) <= 0 ? FString(TEXT("Pail empty")) : Use + TEXT(" Water"));
                return FString::Join(Actions, TEXT("   ")) + (Plot.planted ? FString() : SeedPouchHint());
            }
        break;
    case EFocus::Fire: return A + TEXT(" Cook   ") + X + TEXT(" Add fuel");
    case EFocus::Hearth: return A + TEXT(" Cook");
    case EFocus::Drop: return A + TEXT(" Pick up");
    case EFocus::Bed:
        if (!bBedFocusActionable) return FString();
        if (const auto Offer = BedSleepOffer())
            return A + (Offer->choice == Homestead::SleepChoice::UntilMorning
                ? TEXT(" Sleep until morning") : TEXT(" Sleep until rested"));
        return FString();
    case EFocus::Chest: return A + TEXT(" Open");
    case EFocus::Water:
        if (ToolAvailable && SelectedTool == Homestead::Item::FishingPole)
        {
            if (IsFishing()) return FString();
            return FishingFocusText;
        }
        // The pail is filled with the tool button; only offered with it in hand.
        if (!ToolAvailable || SelectedTool != Homestead::Item::WateringCan) return FString();
        return Sim.Count(Homestead::Item::Water) >= Homestead::PailPortions ? FString(TEXT("Pail full")) : Use + TEXT(" Fill pail");
    case EFocus::Underbrush: return Use + TEXT(" Clear with Machete");
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: return StoreFocusActions();
    case EFocus::RoadSign: return RoadSignActions();
    default:
        if (ToolAvailable && SelectedTool == Homestead::Item::OilLamp)
            return Use + TEXT(" Set lamp down   ") + X + TEXT(" Fill lamp");
        // With seed out over untilled ground, the red outline says to till it first (UpdateGardenOutline).
        if (Homestead::CropForSeed(HotbarItem(SelectedHotbarSlot)) && !GardenOutlineReason.IsEmpty())
            return GardenOutlineReason + SeedPouchHint();
        if (!SeedPouchHint().IsEmpty())
            return (bGamepad ? TEXT("[Menu] Field book") : TEXT("[I] Field book")) + SeedPouchHint();
        // Food on the hotbar is eaten with A / E (or X / F) when there's nothing else to use them on.
        if (const auto Food = SelectedHotbarFood(); Food != Homestead::Item::Count && Sim.Count(Food) > 0)
            return A + TEXT(" Eat ") + Text(Homestead::ItemName(Food)).ToLower();
        // With the hoe out, a red outline says why the square ahead can't be tilled (UpdateGardenOutline).
        if (ToolAvailable && SelectedTool == Homestead::Item::DiggingStick && !GardenOutlineReason.IsEmpty())
            return GardenOutlineReason;
        return ToolAvailable && SelectedTool == Homestead::Item::DiggingStick
        ? Use + TEXT(" Till") : (bGamepad ? TEXT("[Menu] Field book") : TEXT("[I] Field book"));
    }
    return FString();
}

FString AHomesteadController::HintId(const FString& Verb) const
{
    FString Noun;
    if (Focus == EFocus::Resource)
        for (const auto& Node : State().resources)
            if (Node.id == FocusId)
            {
                Noun = Node.kind == Homestead::ResourceKind::ForestTree || Node.kind == Homestead::ResourceKind::Sapling
                    ? FString(TEXT("Tree")) : Text(Homestead::ResourceName(Node.kind));
                break;
            }
    FString Id;
    for (const TCHAR Letter : Verb + TEXT("_") + Noun)
        if (FChar::IsAlnum(Letter) || Letter == TEXT('_')) Id.AppendChar(Letter);
    return Id;
}

int32 AHomesteadController::HintUseCount(const FString& Verb) const
{
    const int32* Uses = HintUses.Find(HintId(Verb));
    return Uses ? *Uses : 0;
}

bool AHomesteadController::IsHintRetired(const FString& Verb) const
{
    return HomesteadActionHints::ShouldRetire(TCHAR_TO_UTF8(*Verb), HintUseCount(Verb), HintRetireUses);
}

AHomesteadController::FHintUse AHomesteadController::BeginHintUse(const FString& Button) const
{
    FHintUse Use;
    Use.Serial = NoticeSerial;
    Use.QuietSerial = QuietActionSerial;
    Use.bHackPending = bHackPending;
    TArray<FString> Parts;
    FocusActions().ParseIntoArray(Parts, TEXT("   "));
    const FString Prefix = TEXT("[") + Button + TEXT("] ");
    for (const FString& Part : Parts)
        if (Part.TrimStartAndEnd().StartsWith(Prefix))
        {
            Use.Id = HintId(Part.TrimStartAndEnd().Mid(Prefix.Len()));
            break;
        }
    return Use;
}

void AHomesteadController::EndHintUse(const FHintUse& Use)
{
    if (Use.Id.IsEmpty()) return;
    const bool Succeeded = QuietActionSerial != Use.QuietSerial
        || (NoticeSerial != Use.Serial && !bToastError) || (!Use.bHackPending && bHackPending);
    if (!Succeeded) return;
    int32& Uses = HintUses.FindOrAdd(Use.Id);
    if (Uses >= HintRetireUses) return;
    ++Uses;
    if (const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr)
        if (PersistIntProperty(Branch->IniPath, ActionHintSection, *Use.Id, Uses))
            GConfig->SetInt(ActionHintSection, *Use.Id, Uses, GGameUserSettingsIni);
}

void AHomesteadController::LoadActionHints()
{
    HintUses.Reset();
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (!Branch || !Disk.Combine(Branch->IniPath)) return;
    if (const FConfigSection* Section = Disk.FindSection(ActionHintSection))
        for (const auto& Pair : *Section)
        {
            const int32 Uses = FCString::Atoi(*Pair.Value.GetValue());
            if (Uses > 0) HintUses.Add(Pair.Key.ToString(), FMath::Min(Uses, HintRetireUses));
        }
}

void AHomesteadController::ResetActionHints()
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    for (const auto& Pair : HintUses)
        if (Branch && PersistIntProperty(Branch->IniPath, ActionHintSection, *Pair.Key, 0))
            GConfig->SetInt(ActionHintSection, *Pair.Key, 0, GGameUserSettingsIni);
    HintUses.Reset();
    ControlsHint.Restart();
    Notify(TEXT("Action hints will show again for your next few tries."));
}

bool AHomesteadController::IsControlsHintOnScreen() const
{
    // Where AHomesteadHUD::DrawHUD draws the strip: the open world, not a book, shop, setup or failure.
    return bWorldReady && ControlsHint.Showing() && !HasNativeMenu() && !bBookOpen && !ShopScreen.IsValid()
        && !IsFailed() && !IsNewGameSetup() && !IsNamingSetup();
}

void AHomesteadController::Notify(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result.ok);
    if (Result.ok && SuccessCue) PlayEffect(SuccessCue);
    RefreshRemaining = 0;
}

void AHomesteadController::NotifyResourceAction(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    // Refusals, and a full pack leaving things on the ground, are worth a notice; plain success isn't.
    if (!Result.ok || Result.code == Homestead::ResultCode::PackOverflow)
    {
        Notify(Result, SuccessCue);
        return;
    }
    // Quiet success must not erase a still-live refusal, save or first-visit unlock notice.
    ++QuietActionSerial;
    if (SuccessCue) PlayEffect(SuccessCue);
    RefreshRemaining = 0;
}

void AHomesteadController::Notify(const FString& Message, bool Error)
{
    ++NoticeSerial;
    ToastText = Message;
    bToastError = Error;
    ToastRemaining = Error ? 8 : 5;
    if (Error) UE_LOG(LogTemp, Warning, TEXT("Homestead: %s"), *Message);
}
