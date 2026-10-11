// The estate mine on the controller side (openspec change open-estate-mine-shallow-level): focus on the trapdoor
// and ladder, the climb between the mine house and the tunnels, the pickaxe at a rock face, and the underground
// stuck and exhaustion rules. The rules themselves live in Simulation/HomesteadMine.
#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadMine.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadMine, Log, All);

namespace HomesteadControllerMineTuning
{
constexpr double TrapdoorReachCm = 170.0;     // from the lid's centre: her focus on the trapdoor
constexpr double LadderReachCm = 230.0;       // from the ladder foot: her focus on the ladder
constexpr double HouseLevelBandCm = 300.0;    // within this of the house floor she is "upstairs" and can use the trapdoor
constexpr double LidOpenLeadSeconds = 0.6;    // the lid is up before she goes
constexpr double LidShutAfterSeconds = 1.6;   // and shut once she has gone (from pressing the key)
constexpr float ArrivalLiftCm = 150.0f;       // the ground snap's start height over the spot, as MenuTravel does
constexpr float StrikeRadiusCm = 60.0f;       // where the pick meets the face, for her approach and aim
}

bool AHomesteadController::IsUnderground() const
{
    const APawn* Avatar = GetPawn();
    return bEstateMap && Avatar && Homestead::Mine::IsUnderground(PlayerPoint(), Avatar->GetActorLocation().Z);
}

void AHomesteadController::ConsiderTrapdoorFocus(TFunctionRef<void(EFocus, int32, Homestead::Point)> Consider) const
{
    const APawn* Avatar = GetPawn();
    if (!bEstateMap || !Avatar) return;
    const auto& Layout = Homestead::Mine::GetLayout();
    if (Layout.floors.empty()) return;
    const auto Position = PlayerPoint();
    const double Z = Avatar->GetActorLocation().Z;
    if (Homestead::Mine::IsUnderground(Position, Z))
    {
        const Homestead::Point Foot{Layout.ladderFoot.x, Layout.ladderFoot.y};
        if (FVector2D::Distance(FVector2D(Foot.x, Foot.y), FVector2D(Position.x, Position.y))
            <= HomesteadControllerMineTuning::LadderReachCm)
            Consider(EFocus::Trapdoor, Homestead::Mine::LadderFootId, Foot);
        return;
    }
    if (FMath::Abs(Z - Layout.houseFloor.z) > HomesteadControllerMineTuning::HouseLevelBandCm) return;
    const double Yaw = FMath::DegreesToRadians(Layout.trapdoor.yaw);
    const Homestead::Point Lid{Layout.trapdoor.hingeX + FMath::Cos(Yaw) * Layout.trapdoor.lidLength * 0.5,
        Layout.trapdoor.hingeY + FMath::Sin(Yaw) * Layout.trapdoor.lidLength * 0.5};
    if (FVector2D::Distance(FVector2D(Lid.x, Lid.y), FVector2D(Position.x, Position.y))
        <= HomesteadControllerMineTuning::TrapdoorReachCm)
        Consider(EFocus::Trapdoor, Homestead::Mine::TrapdoorId, Lid);
}

void AHomesteadController::MoveWithinMine(const Homestead::Mine::Spot& Spot, float EyeYawDegrees)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    ResetOvergrowthSwing();
    HeldRepeatNode = INDEX_NONE;
    Avatar->CancelAction(true);
    Avatar->ResetSprint();
    // A snap that is abandoned puts her back where she is now, not at an older safe spot.
    LastSafeWorldPosition = Avatar->GetActorLocation();
    // Underground the ground snap takes the tunnels' own collision as the floor and never waits for the landscape.
    BeginGroundSnap(FVector(Spot.x, Spot.y, Spot.z + HomesteadControllerMineTuning::ArrivalLiftCm));
    if (!bPendingGroundSnap) return;
    Avatar->SetActorRotation(FRotator(0, EyeYawDegrees, 0));
    if (bBookOpen) CloseBook();
    SetControlRotation(FRotator(-12, EyeYawDegrees, 0));
    Avatar->SetRestingViewRotation(GetControlRotation());
}

void AHomesteadController::ClimbMineLadder()
{
    if (!bEstateMap || bMineLadderBusy || RejectPendingGroundSnapAction()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    bMineLadderDescending = !IsUnderground();
    bMineLadderBusy = true;
    bMineLadderMovePending = true;
    MineLadderMoveAt = Now + HomesteadControllerMineTuning::LidOpenLeadSeconds;
    MineLadderDoneAt = Now + HomesteadControllerMineTuning::LidShutAfterSeconds;
    if (Landscape) Landscape->SetMineTrapdoorOpen(true);
    UE_LOG(LogHomesteadMine, Log, TEXT("Mine ladder: %s."), bMineLadderDescending ? TEXT("down") : TEXT("up"));
}

int32 AHomesteadController::MineSwingsNeeded() const
{
    return SwingNode != INDEX_NONE && Homestead::Mine::FindNode(SwingNode) ? Sim.MineSwings(SwingNode) : 0;
}

void AHomesteadController::SwingAtMine(Homestead::Item Tool)
{
    if (bSwingPending) return;
    HeldRepeatNode = INDEX_NONE;
    HeldStrokeNode = INDEX_NONE;
    const auto Position = PlayerPoint();
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    const FVector Forward = Avatar->GetActorForwardVector();
    const double Z = Avatar->GetActorLocation().Z;
    // Walking off resets a half-mined face: its earlier blows never persist.
    if (SwingNode != INDEX_NONE && FVector2D::Distance(SwingFrom, FVector2D(Position.x, Position.y)) > 150.0)
        ResetOvergrowthSwing();

    const int32 Target = Sim.FindAimedMine(Position, Z, {Forward.X, Forward.Y});
    if (Target == INDEX_NONE || Target < 0 || !Homestead::Mine::FindNode(Target))
    {
        Notify(TEXT("Aim at a rock face, seam, vein or gem pocket."), true);
        return;
    }
    const auto Ready = Sim.CheckMine(Target, Tool, Position, Z);
    if (!Ready)
    {
        Notify(Ready);
        return;
    }
    if (Target != SwingNode)
    {
        SwingNode = Target;
        SwingsLanded = 0;
    }
    SwingFrom = FVector2D(Position.x, Position.y);
    SwingTool = Tool;
    const Homestead::Mine::Node* Face = Homestead::Mine::FindNode(Target);
    const Homestead::Point Aim{Face->x, Face->y};
    bSwingFellTimed = false;
    SwingStrokes = 1;
    SwingStrokesLanded = 0;
    // A click is exactly one blow; holding the button keeps striking until it breaks (ContinueHeldStrike).
    bSwingFellTimed = Avatar->PlayStrike(Aim, Tool, SwingStrokes, HomesteadControllerMineTuning::StrikeRadiusCm);
    if (!bSwingFellTimed)
    {
        Avatar->PlayClear(Aim);
        LandMineSwing(false);
        return;
    }
    bSwingPending = true;
    SwingSince = GetWorld()->GetTimeSeconds();
    if (const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()))
    {
        SwingFellStartsBefore = Animation->FellStarts();
        SwingHackStartsBefore = Animation->MacheteStarts();
    }
}

void AHomesteadController::LandMineSwing(bool bMoreComing)
{
    const APawn* Avatar = GetPawn();
    if (!Avatar || SwingNode == INDEX_NONE) return;
    const auto Position = PlayerPoint();
    ++SwingsLanded;
    SwingFrom = FVector2D(Position.x, Position.y);
    const int32 Needed = Sim.MineSwings(SwingNode);
    if (SwingsLanded < Needed)
    {
        PlayStrikeCue(SwingTool, SwingsLanded, false);
        if (bMoreComing) return;
        const int32 Left = Needed - SwingsLanded;
        Notify(FString::Printf(TEXT("%d more %s."), Left, Left == 1 ? TEXT("swing") : TEXT("swings")));
        return;
    }
    const auto Result = Sim.MineNode(SwingNode, SwingTool, Position, Avatar->GetActorLocation().Z);
    const int32 Swing = SwingsLanded;
    ResetOvergrowthSwing();
    NotifyResourceAction(Result, nullptr);
    // The breaking strike rings only when it breaks (a refusal gets no reward).
    if (Result.ok) PlayStrikeCue(Homestead::Item::Pickaxe, Swing, true);
}

void AHomesteadController::TickMine(float)
{
    if (!bEstateMap || !GetPawn() || bPendingSpawn) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (bMineLadderBusy)
    {
        const auto& Layout = Homestead::Mine::GetLayout();
        if (bMineLadderMovePending && Now >= MineLadderMoveAt && !bPendingGroundSnap)
        {
            bMineLadderMovePending = false;
            const auto& Arrival = bMineLadderDescending ? Layout.ladderFoot : Layout.ladderTop;
            MoveWithinMine(Arrival, static_cast<float>(Arrival.yaw));
            if (!bPendingGroundSnap)
            {
                bMineLadderBusy = false;
                if (Landscape) Landscape->SetMineTrapdoorOpen(false);
            }
        }
        else if (!bMineLadderMovePending && Now >= MineLadderDoneAt && !bPendingGroundSnap)
        {
            bMineLadderBusy = false;
            if (Landscape) Landscape->SetMineTrapdoorOpen(false);
        }
    }
    if (bPendingGroundSnap || bMineLadderBusy || IsFailed()) return;

    const FVector Position = GetPawn()->GetActorLocation();
    const Homestead::Point Here{Position.X, Position.Y};
    if (!Homestead::Mine::IsUnderground(Here, Position.Z)) return;

    // Out of energy below: she wakes upstairs, rested, with her pack as it was (design D4).
    if (Homestead::Mine::IsExhausted(State().energy))
    {
        const auto Woke = Sim.WakeAtMine(Here, Position.Z);
        SeenDozes = Sim.DozeCount();
        ++MineWakes;
        if (!Woke) { Notify(Woke); return; }
        MoveWithinMine(Homestead::Mine::GetLayout().houseFloor, static_cast<float>(Homestead::Mine::GetLayout().houseFloor.yaw));
        Notify(Woke);
        RefreshRemaining = 0;
        return;
    }
    // Fallen out of the level, or off every walkable floor: back to the foot of the ladder, never the surface.
    if (Homestead::Mine::IsStuck(Here, Position.Z))
    {
        ++MineStuckRecoveries;
        UE_LOG(LogHomesteadMine, Warning, TEXT("Mine stuck rule: (%.0f, %.0f, %.0f) back to the ladder foot."),
            Position.X, Position.Y, Position.Z);
        Notify(TEXT("You find your way back to the ladder."));
        MoveWithinMine(Homestead::Mine::StuckTarget(), static_cast<float>(Homestead::Mine::StuckTarget().yaw));
    }
}
