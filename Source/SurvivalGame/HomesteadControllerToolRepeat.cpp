#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "Simulation/HomesteadToolRepeat.h"

#include "Components/SkeletalMeshComponent.h"
#include "InputCoreTypes.h"

// Hold-to-repeat tool strikes. The rule (when another blow follows) is Homestead::ToolRepeat::AfterBlow;
// this file feeds it what the controller sees and starts the next blow. Only the tool button repeats:
// E / A (interact) never performs a tool action.

bool AHomesteadController::IsToolButtonHeld() const
{
    return IsInputKeyDown(EKeys::LeftMouseButton) || IsInputKeyDown(EKeys::Gamepad_RightTrigger);
}

bool AHomesteadController::ContinueHeldStrike(int32 Node, Homestead::Item Tool)
{
    const auto Position = PlayerPoint();
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    const Homestead::Point Facing{Forward.X, Forward.Y};
    Homestead::ToolRepeat::Held Now;
    Now.held = IsToolButtonHeld();
    const bool bSelected = SelectedHotbarSlot >= 0 && SelectedHotbarSlot < Homestead::PackRowSize
        && HotbarItem(SelectedHotbarSlot) == Tool;
    Now.sameTool = bSelected && Sim.Count(Tool) > 0;
    Now.menuOpen = !ShouldShowHotbar();
    if (Tool == Homestead::Item::Scythe)
    {
        for (const int32 Id : Sim.ScytheArcTargets(Position, Facing)) if (Id == Node) Now.aimed = Node;
    }
    else Now.aimed = Sim.FindAimedOvergrowth(Position, Facing, Tool);
    const auto Decision = Homestead::ToolRepeat::AfterBlow(Sim, Node, Tool, Position, Now);
    if (Decision.more) return true;
    if (!Decision.refusal.ok)
    {
        // Too tired below the energy floor, out of reach and so on: said once, and she lowers the tool.
        Notify(Decision.refusal);
        return false;
    }
    // She stopped with the target still standing: how many blows it still needs, as a click says.
    if (Tool != Homestead::Item::Scythe && SwingNode == Node && Homestead::ToolRepeat::Standing(Sim, Node))
    {
        const int32 Left = Sim.OvergrowthSwings(Node) - SwingsLanded;
        if (Left > 0) Notify(FString::Printf(TEXT("%d more %s."), Left, Left == 1 ? TEXT("swing") : TEXT("swings")));
    }
    return false;
}

void AHomesteadController::UpdateHeldRepeat()
{
    if (HeldRepeatNode == INDEX_NONE || bSwingPending) return;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    // A new hack or strike is refused until the last one has finished and blended out.
    if (Animation && Animation->IsHandActionBusy()) return;
    const int32 Node = HeldRepeatNode;
    const auto Tool = HeldRepeatTool;
    HeldRepeatNode = INDEX_NONE;
    // Checked again now: letting go during the last blow's recovery ends it there.
    if (!ContinueHeldStrike(Node, Tool)) return;
    SwingAtOvergrowth(Tool);
}
