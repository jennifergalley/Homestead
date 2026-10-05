#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadTreeFelling.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Sound/SoundBase.h"

// Felling the estate's decorative trees with the axe (openspec chop-any-tree). The simulation decides
// (Simulation::FellSceneryTree); the world lifts a copy of the tree out of its batch; the felling clip
// and chop sounds are the ones a woodland tree already uses (PresentFelling / UpdatePendingFell).

void AHomesteadController::StartSceneryFell()
{
    if (Focus != EFocus::SceneryTree || !Landscape) return;
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    const bool bFell = Avatar && Avatar->CanFell();
    const Homestead::Point Trunk{FocusSceneryTrunk.X, FocusSceneryTrunk.Y};
    const auto Result = Sim.FellSceneryTree(Trunk, PlayerPoint());
    NotifyResourceAction(Result, bFell ? nullptr : WoodTapB.Get());
    if (!Result.ok || !Avatar) return;

    const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
    constexpr int32 Strokes = 3;
    if (Animation && Landscape->BeginFellingScenery(Sim, FocusSceneryTrunk)
        && Avatar->PlayFell(Trunk, Strokes, FMath::Max(FocusSceneryRadius, 5.0f)))
    {
        FellResource = SceneryFellMarker;
        FellStrokes = Strokes;
        FellStrokesHeard = 0;
        FellStartsBefore = Animation->FellStarts();
        bFellSeen = false;
        FellSince = GetWorld()->GetTimeSeconds();
        return;
    }
    // No clip or no copy to topple: the tree is simply gone from the standing set.
    Landscape->UpdateSceneryTreeStages(Sim, true);
    Avatar->PlayClear(Trunk);
}
