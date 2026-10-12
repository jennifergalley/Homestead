// Tool focus (Jenny's playtest: a bare bramble in reach showed no prompt). The focus used to be the
// nearest centre inside 280 cm while the simulation clears overgrowth out to 300 cm, and a weed or
// grass tuft at her feet won over the bramble she faced. With a clearing tool in hand, the prompt and
// the swing name the same aimed target (Simulation::HeldToolFocus / FindAimedOvergrowth), never one
// behind her; empty-handed, overgrowth in the 280-300 cm band still gets its prompt. Plots, drops, structures, the store and water are never overridden.
#include "HomesteadController.h"

#include "Simulation/HomesteadOvergrowth.h"

namespace ToolFocus
{
// Everyday focus radius for anything she can use (cm); overgrowth reaches Homestead::Overgrowth::Reach.
constexpr double FocusReachCm = 280.0;
}

void AHomesteadController::FocusHeldToolTarget(Homestead::Point Position)
{
    // Down in the tunnels the pickaxe names the node she's aimed at, even over the lamp she set down
    // beside it, and nothing on the surface.
    if (IsUnderground())
    {
        // The node she faces (any tool, or none) wins over a lamp she set down beside it; the ladder keeps its prompt.
        if (Focus == EFocus::Drop || Focus == EFocus::None || SelectedCarriedTool() == Homestead::Item::Pickaxe)
        {
            const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
            const int Aimed = Sim.FindAimedMine(Position, GetPawn() ? GetPawn()->GetActorLocation().Z : 0.0, {Forward.X, Forward.Y});
            if (Aimed != -1 && Focus != EFocus::Trapdoor)
            {
                Focus = EFocus::Resource;
                FocusId = Aimed;
            }
        }
        return;
    }
    if (Focus != EFocus::None && Focus != EFocus::Resource) return;
    // With a clearing tool in hand, what she's aimed at is what the prompt names and the swing strikes
    // (Simulation::HeldToolFocus): nearer overgrowth behind her or to the side doesn't keep the focus,
    // even when this tool clears it too. A forageable she's standing at keeps it.
    const Homestead::Item Tool = SelectedCarriedTool();
    if (Tool != Homestead::Item::Count && Homestead::ToolForItem(Tool) != Homestead::ToolKind::Count)
    {
        const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
        const int Current = Focus == EFocus::Resource ? FocusId : -1;
        const int Chosen = Sim.HeldToolFocus(Current, Position, {Forward.X, Forward.Y}, Tool);
        if (Chosen != -1)
        {
            Focus = EFocus::Resource;
            FocusId = Chosen;
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
