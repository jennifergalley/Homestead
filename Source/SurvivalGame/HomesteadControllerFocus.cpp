#include "HomesteadController.h"
#include "HomesteadControllerConfig.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadWorld.h"
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
    Focus = EFocus::None;
    FocusId = -1;
    const auto Position = PlayerPoint();
    double Best = 280.0;
    auto Consider = [&](EFocus Kind, int Id, Homestead::Point Target)
    {
        const double Distance = FMath::Sqrt(FMath::Square(Target.x - Position.x) + FMath::Square(Target.y - Position.y));
        if (Distance < Best) { Best = Distance; Focus = Kind; FocusId = Id; }
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
                break;
            }
    }
    for (const auto& Structure : State().structures)
    {
        EFocus Kind = EFocus::None;
        if (Structure.kind == Homestead::Piece::Fire) Kind = EFocus::Fire;
        if (Structure.kind == Homestead::Piece::Hearth) Kind = EFocus::Hearth;
        if (Structure.kind == Homestead::Piece::Bed) Kind = EFocus::Bed;
        if (Structure.kind == Homestead::Piece::Chest) Kind = EFocus::Chest;
        if (Kind != EFocus::None) Consider(Kind, Structure.id, Homestead::StructureCenter(State(), Structure));
    }
    ConsiderStoreFocus(Consider);
    FocusHeldToolTarget(Position);
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
        }
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
            if (!Plot.planted) return TEXT("A little patch of earth");
            return Text(Homestead::PlotStatus(Plot).c_str());
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
    case EFocus::Bed: return TEXT("Bedroll");
    case EFocus::Chest: return TEXT("Storage chest");
    case EFocus::Water: return TEXT("Fresh stream water");
    case EFocus::Underbrush: return AHomesteadWorld::UnderbrushName(FocusBrushSpecies);
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: return StoreFocusTitle();
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
                if (Node.kind == Homestead::ResourceKind::ForestTree)
                    return ToolAvailable && SelectedTool == Homestead::Item::Hatchet
                        ? Use + TEXT(" Fell with Axe") : ToolPrompt(Sim, Homestead::Item::Hatchet, TEXT("axe"), TEXT(" to fell"));
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
                    if (Handles)
                    {
                        // Out of tier: say which upgrade it needs rather than offering a swing that glances off.
                        const Homestead::ToolTier Needed = FMath::Max(Overgrowth->minTier, Node.minTier);
                        if (Sim.GetToolTier(Overgrowth->tool) < Needed)
                            return UTF8_TO_TCHAR(Homestead::NeedsToolMessage(Overgrowth->tool, Needed).c_str());
                        return Use + TEXT(" ") + SwingVerb(SelectedTool)
                            + (Overgrowth->byHand ? TEXT("   ") + Hand : FString());
                    }
                    if (Overgrowth->byHand) return Hand;
                    // Too worn for it: name the upgrade whichever tool is in hand, rather than
                    // suggesting she select a tool that would only be refused (Jenny, 09-29).
                    if (const Homestead::ToolTier Needed = FMath::Max(Overgrowth->minTier, Node.minTier);
                        Sim.GetToolTier(Overgrowth->tool) < Needed)
                        return UTF8_TO_TCHAR(Homestead::NeedsToolMessage(Overgrowth->tool, Needed).c_str());
                    return ToolPrompt(Sim, Homestead::ToolItem(Overgrowth->tool), UTF8_TO_TCHAR(Homestead::ToolName(Overgrowth->tool)));
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
                // [F]/[X] pulls weeds by hand whatever is selected (the hoe's [LMB] works the square
                // ahead instead), so the prompt says so on any square where weeds show: bare, growing
                // or ripe. It never sows.
                const FString Pull = Homestead::HasVisibleWeeds(Plot) ? X + TEXT(" Pull weeds") : FString();
                const FString AndPull = Pull.IsEmpty() ? FString() : TEXT("   ") + Pull;
                if (!Plot.planted)
                {
                    // [A]/[E] sows the seed stack chosen on the hotbar (a berry sows berry seed; wild
                    // roots are chosen as Seeds).
                    if (HotbarItem(SelectedHotbarSlot) != Homestead::Item::Count)
                        if (const auto* Seed = Homestead::CropForSeed(HotbarItem(SelectedHotbarSlot)))
                        {
                            const auto Chosen = HotbarItem(SelectedHotbarSlot);
                            const FString What = Seed->kind == Homestead::CropKind::Berries ? FString(TEXT("berry seeds"))
                                : Seed->kind == Homestead::CropKind::Roots ? FString(TEXT("roots")) : Text(Seed->lower);
                            if (Sim.Count(Chosen) <= 0)
                                return TEXT("No ") + FString(UTF8_TO_TCHAR(Homestead::ItemName(Chosen))).ToLower()
                                    + TEXT(" left") + AndPull + SeedPouchHint();
                            return A + TEXT(" Sow ") + What
                                + (Chosen == Homestead::Item::Berries ? TEXT("   ") + Use + TEXT(" Eat") : FString()) + AndPull + SeedPouchHint();
                        }
                    return (Pull.IsEmpty() ? FString() : Pull + TEXT("   ")) + TEXT("Choose seeds on the hotbar to sow") + SeedPouchHint();
                }
                if (Homestead::IsRipe(Plot)) return A + TEXT(" Harvest") + AndPull;
                FString Actions;
                if (Homestead::NeedsWater(Plot))
                {
                    // The pail in her pack waters on [E]/[A]; otherwise say where it is.
                    const int32 Pail = ToolWhereabouts(Sim, Homestead::Item::WateringCan);
                    Actions = Pail == 2 && Sim.Count(Homestead::Item::Water) <= 0 ? FString(UTF8_TO_TCHAR(Homestead::EmptyPailText))
                        : Pail == 2 ? (ToolAvailable && SelectedTool == Homestead::Item::WateringCan ? Use : A) + TEXT(" Water")
                        : ToolPrompt(Sim, Homestead::Item::WateringCan, TEXT("pail"), TEXT(" to water"));
                }
                if (!Pull.IsEmpty()) Actions += (Actions.IsEmpty() ? TEXT("") : TEXT("   ")) + Pull;
                return Actions;
            }
        break;
    case EFocus::Fire: return A + TEXT(" Cook   ") + X + TEXT(" Add firewood / branch");
    case EFocus::Hearth: return A + TEXT(" Cook");
    case EFocus::Drop: return A + TEXT(" Pick up");
    case EFocus::Bed:
    {
        const auto Options = BedSleepOptions();
        const int32 Index = BedSleepIndex();
        if (!Options.size()) return FString();
        FString Line = A + TEXT(" ") + SleepOptionLabel(Options[Index]);
        if (Options.size() > 1)
        {
            TArray<FString> Others;
            for (int32 Other = 0; Other < static_cast<int32>(Options.size()); ++Other)
                if (Other != Index)
                    Others.Add(Options[Other].choice == Homestead::SleepChoice::UntilMorning ? TEXT("until morning")
                        : Options[Other].choice == Homestead::SleepChoice::UntilRested ? TEXT("until rested") : TEXT("nap"));
            Line += FString(TEXT("   ")) + (bGamepad ? TEXT("[D-pad]") : TEXT("[Up/Down]")) + TEXT(" ") + FString::Join(Others, TEXT(" / "));
        }
        return Line;
    }
    case EFocus::Chest: return A + TEXT(" Open pack / storage");
    case EFocus::Water:
        // Offer the fill only when it can happen: say where the pail is, or that it's already full.
        switch (ToolWhereabouts(Sim, Homestead::Item::WateringCan))
        {
        case 2:
            if (Sim.Count(Homestead::Item::Water) >= Homestead::PailPortions) return TEXT("Your pail is full");
            return ToolAvailable && SelectedTool == Homestead::Item::WateringCan
                ? Use + TEXT(" Fill Pail") : A + TEXT(" Fill carried Pail");
        case 1: return TEXT("Take your pail from the chest to fill it");
        default: return TEXT("Requires a pail");
        }
    case EFocus::Underbrush: return Use + TEXT(" Clear with Machete");
    case EFocus::Shopkeeper:
    case EFocus::StoreDoor: return StoreFocusActions();
    default:
        if (ToolAvailable && SelectedTool == Homestead::Item::OilLamp)
            return Use + TEXT(" Set lamp down   ") + X + TEXT(" Fill lamp");
        if (!SeedPouchHint().IsEmpty())
            return (bGamepad ? TEXT("[Menu] Field book") : TEXT("[I] Field book")) + SeedPouchHint();
        // Food on the hotbar is eaten with A / E (or X / F) when there's nothing else to use them on.
        if (const auto Food = SelectedHotbarFood(); Food != Homestead::Item::Count && Sim.Count(Food) > 0)
            return A + TEXT(" Eat ") + Text(Homestead::ItemName(Food)).ToLower();
        // With the hoe out, a red outline says why the square ahead can't be tilled (UpdateGardenOutline).
        if (ToolAvailable && SelectedTool == Homestead::Item::DiggingStick && !GardenOutlineReason.IsEmpty())
            return GardenOutlineReason;
        return ToolAvailable && SelectedTool == Homestead::Item::DiggingStick
        ? Use + TEXT(" Till ground") : (bGamepad ? TEXT("[Menu] Field book") : TEXT("[I] Field book"));
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
    return HintUseCount(Verb) >= HintRetireUses;
}

AHomesteadController::FHintUse AHomesteadController::BeginHintUse(const FString& Button) const
{
    FHintUse Use;
    Use.Serial = NoticeSerial;
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
    const bool Succeeded = (NoticeSerial != Use.Serial && !bToastError) || (!Use.bHackPending && bHackPending);
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
    Notify(TEXT("Action hints will show again for your next few tries."));
}

void AHomesteadController::Notify(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    Notify(UTF8_TO_TCHAR(Result.message.c_str()), !Result.ok);
    if (Result.ok && SuccessCue) PlayEffect(SuccessCue);
    RefreshRemaining = 0;
}

void AHomesteadController::NotifyResourceAction(const Homestead::Result& Result, USoundBase* SuccessCue)
{
    if (!Result.ok)
    {
        Notify(Result);
        return;
    }
    ToastText.Reset();
    ToastRemaining = 0;
    bToastError = false;
    ++NoticeSerial;
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
