// Tool focus (Jenny's playtest: a bare bramble in reach showed no prompt). The focus used to be the
// nearest centre inside 280 cm while the simulation clears overgrowth out to 300 cm, and a weed or
// grass tuft at her feet won over the bramble she faced. With a clearing tool in hand, focus and
// swing now share Simulation::FindAimedOvergrowth; empty-handed, overgrowth in the 280-300 cm band
// still gets its prompt. Plots, drops, structures, the store and water are never overridden.
#include "HomesteadController.h"

#include "Simulation/HomesteadOvergrowth.h"

namespace ToolFocus
{
// Everyday focus radius for anything she can use (cm); overgrowth reaches Homestead::Overgrowth::Reach.
constexpr double FocusReachCm = 280.0;

bool Handles(Homestead::Item Tool, const Homestead::ResourceNode& Node)
{
    if (const auto* Info = Homestead::FindOvergrowth(Node.kind))
        return Info->tool != Homestead::ToolKind::Count && Info->tool == Homestead::ToolForItem(Tool);
    return Node.kind == Homestead::ResourceKind::ForestTree && Tool == Homestead::Item::Hatchet;
}
}

void AHomesteadController::FocusHeldToolTarget(Homestead::Point Position)
{
    if (Focus != EFocus::None && Focus != EFocus::Resource) return;
    const Homestead::ResourceNode* Current = nullptr;
    if (Focus == EFocus::Resource)
        for (const auto& Node : State().resources)
            if (Node.id == FocusId) Current = &Node;
    // A forageable she's standing at keeps the focus; only overgrowth the tool can't clear yields it.
    if (Current && !Homestead::IsOvergrowth(Current->kind)) return;

    const Homestead::Item Tool = SelectedCarriedTool();
    if (Tool != Homestead::Item::Count && Homestead::ToolForItem(Tool) != Homestead::ToolKind::Count
        && !(Current && ToolFocus::Handles(Tool, *Current)))
    {
        const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
        const int Aimed = Sim.FindAimedOvergrowth(Position, {Forward.X, Forward.Y}, Tool);
        if (Aimed != -1)
        {
            Focus = EFocus::Resource;
            FocusId = Aimed;
            return;
        }
    }
    if (Focus != EFocus::None) return;
    // Nothing within everyday reach: overgrowth just past it is still clearable, so name it.
    double Best = Homestead::Overgrowth::Reach;
    for (const auto& Node : State().resources)
    {
        if (Node.cleared || !Homestead::IsOvergrowth(Node.kind)) continue;
        const double Distance = FMath::Sqrt(FMath::Square(Node.position.x - Position.x) + FMath::Square(Node.position.y - Position.y));
        if (Distance >= ToolFocus::FocusReachCm && Distance <= Best)
        {
            Best = Distance;
            Focus = EFocus::Resource;
            FocusId = Node.id;
        }
    }
}
