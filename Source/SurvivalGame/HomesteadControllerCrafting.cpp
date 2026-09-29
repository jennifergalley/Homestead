// Period crafting at the controller (rework-farming-calendar-and-period-crafting, lane E): the focus
// and prompts for field gates, the workbench and the sawhorse, and what [E]/[A] does at each. The
// rules are Simulation::ToggleGate and Simulation::Craft (Simulation/HomesteadCrafting.cpp).
#include "HomesteadController.h"

#include "HomesteadCharacter.h"
#include "Simulation/HomesteadCrafting.h"
#include "UI/SHomesteadMenu.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadCrafting, Log, All);

void AHomesteadController::ConsiderCraftingFocus(TFunctionRef<void(EFocus, int32, Homestead::Point)> Consider) const
{
    for (const auto& Structure : State().structures)
    {
        const EFocus Kind = Structure.kind == Homestead::Piece::FenceGate ? EFocus::Gate
            : Structure.kind == Homestead::Piece::Workbench ? EFocus::Workbench
            : Structure.kind == Homestead::Piece::Sawhorse ? EFocus::Sawhorse : EFocus::None;
        // The footprint's centre: a workbench on a floor stands off its cell's centre.
        if (Kind != EFocus::None) Consider(Kind, Structure.id, Homestead::StructureFootprint(State(), Structure).center);
    }
}

FString AHomesteadController::CraftingFocusTitle() const
{
    for (const auto& Structure : State().structures)
    {
        if (Structure.id != FocusId) continue;
        if (Structure.kind == Homestead::Piece::FenceGate)
            return Structure.open ? TEXT("Field gate (open)") : TEXT("Field gate");
        return UTF8_TO_TCHAR(Homestead::PieceName(Structure.kind));
    }
    return FString();
}

FString AHomesteadController::CraftingFocusActions() const
{
    const FString A = bGamepad ? TEXT("[A]") : TEXT("[E]");
    for (const auto& Structure : State().structures)
    {
        if (Structure.id != FocusId) continue;
        switch (Structure.kind)
        {
        case Homestead::Piece::FenceGate: return A + (Structure.open ? TEXT(" Shut the gate") : TEXT(" Open the gate"));
        case Homestead::Piece::Workbench: return A + TEXT(" Work at the bench");
        case Homestead::Piece::Sawhorse:
            return Sim.Count(Homestead::Item::Timber) > 0 ? A + TEXT(" Saw planks")
                : FString(TEXT("Bring timber to saw into planks"));
        default: return FString();
        }
    }
    return FString();
}

void AHomesteadController::OpenCraftAtStation(Homestead::Piece Station)
{
    OpenBook(1);
    for (int32 Index = 0; Index < static_cast<int32>(Homestead::Recipe::Count); ++Index)
        if (Homestead::Crafting::StationFor(static_cast<Homestead::Recipe>(Index)) == Station)
        {
            Selection = Index;
            if (NativeMenu && !NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe, Index, 0))
                UE_LOG(LogHomesteadCrafting, Warning, TEXT("The Craft page could not select recipe %d."), Index);
            return;
        }
}

void AHomesteadController::InteractWithCrafting()
{
    const auto Position = PlayerPoint();
    const Homestead::Structure* Target = nullptr;
    for (const auto& Structure : State().structures)
        if (Structure.id == FocusId) { Target = &Structure; break; }
    if (!Target) return;
    const Homestead::Point Centre = Homestead::StructureFootprint(State(), *Target).center;
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    switch (Target->kind)
    {
    case Homestead::Piece::FenceGate:
    {
        const auto Result = Sim.ToggleGate(FocusId, Position);
        Notify(Result, WoodTapA);
        if (Result.ok && Avatar) Avatar->PlayGather();
        break;
    }
    case Homestead::Piece::Workbench: OpenCraftAtStation(Homestead::Piece::Workbench); break;
    case Homestead::Piece::Sawhorse:
    {
        const auto Result = Sim.Craft(Homestead::Recipe::SawPlanks, Position);
        Notify(Result, WoodTapB);
        // Stand-in until the two-handed saw stroke is authored: two of the axe's ground strokes at the
        // sawhorse (or the generic clearing motion when she has no axe to hold).
        if (Result.ok && Avatar && !(Avatar->CanStrike(Homestead::Item::Hatchet)
            && Avatar->PlayStrike(Centre, Homestead::Item::Hatchet, 2, -1.0f)))
            Avatar->PlayClear(Centre);
        break;
    }
    default: break;
    }
}
