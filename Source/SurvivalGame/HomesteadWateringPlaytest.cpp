#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadActionTestState.h"

bool AHomesteadVisualPlaytest::WalkWaterTarget(FVector2D Target, float Tolerance, float Delta, FVector2D& Move, FVector2D& Look)
{
    const auto Position = PC->PlayerPoint();
    const FVector2D Offset = Target - FVector2D(Position.x, Position.y);
    if (Offset.Size() <= Tolerance)
    {
        WaterSettle += Delta;
        return WaterSettle >= 0.65f && PC->GetPawn()->GetVelocity().Size2D() < 1;
    }
    WaterSettle = 0;
    const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
    const float Difference = FMath::FindDeltaAngleDegrees(static_cast<float>(PC->GetControlRotation().Yaw), Yaw);
    Look.X = FMath::Clamp(Difference / 45.0f, -0.7f, 0.7f);
    Move.Y = FMath::Abs(Difference) < 35 ? (Offset.Size() < 100 ? 0.35f : 0.85f) : 0;
    return false;
}

void AHomesteadVisualPlaytest::TickWatering(float WallDelta)
{
    static const TCHAR* Labels[] = {TEXT("close-notes"), TEXT("gather-supplies"), TEXT("craft-digging-stick"),
        TEXT("craft-watering-can"), TEXT("walk-to-stream"), TEXT("refill"), TEXT("walk-to-garden"),
        TEXT("approach-tilling-position"), TEXT("till"), TEXT("approach-plot"), TEXT("plant"),
        TEXT("settle-water"), TEXT("view-watering-side"), TEXT("before-water"), TEXT("water"), TEXT("after-water"),
        TEXT("walk-to-sapling-staging"), TEXT("approach-sapling"), TEXT("settle-clear"),
        TEXT("view-clearing-side"), TEXT("before-clear"), TEXT("clear"), TEXT("after-clear")};
    if (Elapsed > 360 || PC->IsFailed() || WaterStage >= UE_ARRAY_COUNT(Labels)) { Finish(); return; }
    const FString Label = bClearRoute && WaterStage == 2 ? TEXT("craft-hatchet") : Labels[WaterStage];
    const bool Entered = PreviousWaterStage != WaterStage;
    if (Entered)
    {
        PreviousWaterStage = WaterStage;
        WaterStageElapsed = 0;
        WaterSettle = 0;
        CaptureElapsed = 0;
        Observations.Add(FString::Printf(TEXT("BEGIN %.2fs %s: %s"), Elapsed, *Label, *PC->FocusTitle()));
    }
    WaterStageElapsed += WallDelta;
    FVector2D Move = FVector2D::ZeroVector, Look = FVector2D::ZeroVector;
    auto Fail = [this](const FString& Reason)
    {
        Observations.Add(TEXT("FAILED ordinary hand-action setup/action: ")
            + (Reason.IsEmpty() ? FString(TEXT("Mapped input did not produce its expected transaction.")) : Reason));
        Finish();
    };
    auto ClearOfResources = [this](FVector2D Position, float Radius)
    {
        for (const auto& Node : PC->State().resources)
            if (!Node.cleared && FVector2D::Distance(Position, FVector2D(Node.position.x, Node.position.y)) < Radius) return false;
        return true;
    };
    switch (WaterStage)
    {
    case 0:
        if (Entered && PC->IsBookOpen()) Tap(EKeys::Gamepad_Special_Right);
        if (!PC->IsBookOpen() && WaterStageElapsed > 0.5f) ++WaterStage;
        break;
    case 1:
    {
        if (bWaterInputPending)
        {
            if (WaterStageElapsed < 0.3f) break;
            if (PC->ToastIsError() || PC->Simulation().Count(static_cast<Homestead::Item>(WaterSupplyItem)) <= WaterSupplyBefore)
            { Fail(PC->Toast()); return; }
            Observations.Add(FString::Printf(TEXT("Gathered actual supply node %d via gamepad A."), ForageId));
            bWaterInputPending = false;
            ForageId = -1; WaterSettle = 0;
            break;
        }
        Homestead::ResourceKind Kind = Homestead::ResourceKind::Count;
        Homestead::Item Item = Homestead::Item::Count;
        if (PC->Simulation().Count(Homestead::Item::Branch) < (bClearRoute ? 4 : 6)) { Kind = Homestead::ResourceKind::Branches; Item = Homestead::Item::Branch; }
        else if (PC->Simulation().Count(Homestead::Item::Stone) < (bClearRoute ? 3 : 1)) { Kind = Homestead::ResourceKind::Stones; Item = Homestead::Item::Stone; }
        else if (PC->Simulation().Count(Homestead::Item::Fiber) < 2) { Kind = Homestead::ResourceKind::Reeds; Item = Homestead::Item::Fiber; }
        else if (!bClearRoute && PC->Simulation().Count(Homestead::Item::Seeds) < 1) { Kind = Homestead::ResourceKind::Roots; Item = Homestead::Item::Seeds; }
        if (Kind == Homestead::ResourceKind::Count) { ++WaterStage; break; }
        if (ForageId < 0)
        {
            float Best = TNumericLimits<float>::Max();
            const auto Position = PC->PlayerPoint();
            for (const auto& Node : PC->State().resources)
            {
                const FVector2D Target(Node.position.x, Node.position.y);
                const float Distance = FVector2D::Distance(Target, FVector2D(Position.x, Position.y));
                if (Node.kind == Kind && PC->Simulation().CanHarvest(Node.id) && Distance < Best)
                {
                    Best = Distance; ForageId = Node.id; ForageTarget = Target;
                }
            }
            if (ForageId < 0) { Fail(TEXT("No available supply patch.")); return; }
            WaterStageElapsed = 0;
        }
        if (WalkWaterTarget(ForageTarget, 25, WallDelta, Move, Look))
        {
            if (!PC->IsResourceFocused(ForageId)) { Fail(TEXT("Supply focus did not match the approached patch.")); return; }
            WaterSupplyItem = static_cast<int32>(Item);
            WaterSupplyBefore = PC->Simulation().Count(Item);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            bWaterInputPending = true;
            WaterStageElapsed = 0;
        }
        break;
    }
    case 2:
    case 3:
    {
        const bool Digging = WaterStage == 2;
        const int32 Recipe = static_cast<int32>(bClearRoute ? Homestead::Recipe::Hatchet : Digging ? Homestead::Recipe::DiggingStick : Homestead::Recipe::WateringCan);
        if (bWaterInputPending)
        {
            if (WaterStageElapsed < 0.3f) break;
            if (PC->ToastIsError() || PC->Simulation().Count(bClearRoute ? Homestead::Item::Hatchet : Digging ? Homestead::Item::DiggingStick : Homestead::Item::WateringCan) != 1)
            { Fail(PC->Toast()); return; }
            Observations.Add(FString::Printf(TEXT("Crafted %s through mapped recipe selection; current hour %.6f."),
                bClearRoute ? TEXT("wood/stone/fiber hatchet") : Digging ? TEXT("digging stick") : TEXT("wood/fiber watering can"), PC->State().hour));
            if (!Digging || bClearRoute) Tap(EKeys::Gamepad_FaceButton_Right);
            bWaterInputPending = false;
            ++WaterStage;
            if (bClearRoute) { WaterStage = 16; ForageId = -1; }
            break;
        }
        if (Entered && !PC->IsBookOpen()) Tap(EKeys::C);
        if (WaterStageElapsed < 0.3f) break;
        const auto Rows = PC->Rows();
        if (!PC->IsBookOpen() || PC->BookPage() != 1 || !Rows.IsValidIndex(PC->SelectedRow())) { Fail(TEXT("Craft book navigation failed.")); return; }
        if (Rows[PC->SelectedRow()].Id != Recipe) { Tap(EKeys::Gamepad_DPad_Down); WaterStageElapsed = 0; break; }
        Tap(EKeys::Gamepad_FaceButton_Bottom);
        bWaterInputPending = true;
        WaterStageElapsed = 0;
        break;
    }
    case 4:
        if (!bWaterTargetReady)
        {
            float Best = TNumericLimits<float>::Max();
            const auto Position = PC->PlayerPoint();
            for (int32 Y = -3000; Y <= 3000; Y += 150)
            {
                const FVector2D Target(Homestead::StreamX(Y) - 90, Y);
                const float Distance = FVector2D::Distance(Target, FVector2D(Position.x, Position.y));
                if (ClearOfResources(Target, 330) && Distance < Best) { Best = Distance; WaterTarget = Target; bWaterTargetReady = true; }
            }
            if (!bWaterTargetReady) { Fail(TEXT("No unobstructed stream approach.")); return; }
        }
        if (WalkWaterTarget(WaterTarget, 20, WallDelta, Move, Look)) ++WaterStage;
        break;
    case 5:
        if (bWaterInputPending)
        {
            if (WaterStageElapsed < 0.3f) break;
            if (PC->ToastIsError() || PC->Simulation().Count(Homestead::Item::Water) != 6) { Fail(PC->Toast()); return; }
            bWaterInputPending = false;
            ++WaterStage;
            bWaterTargetReady = false;
            break;
        }
        if (PC->FocusTitle() != TEXT("Fresh stream water")) { Fail(TEXT("Stream approach has another focus.")); return; }
        Tap(EKeys::Gamepad_FaceButton_Bottom);
        bWaterInputPending = true;
        WaterStageElapsed = 0;
        break;
    case 6:
        if (!bWaterTargetReady)
        {
            float Best = TNumericLimits<float>::Max();
            const auto Position = PC->PlayerPoint();
            for (int32 X = -7; X <= 3; ++X)
                for (int32 Y = -7; Y <= 7; ++Y)
                {
                    const auto Center = Homestead::CellCenter(X, Y);
                    const FVector2D Approach(Center.x, Center.y - 220);
                    const FVector2D Staging(Center.x, Center.y - 340);
                    auto Trial = PC->Simulation();
                    const float Distance = FVector2D::Distance(Staging, FVector2D(Position.x, Position.y));
                    if (Distance < Best && ClearOfResources(Approach, 330) && ClearOfResources(FVector2D(Center.x, Center.y), 90)
                        && Trial.Till(X, Y, {Approach.X, Approach.Y}).ok)
                    {
                        Best = Distance; GardenCenter = FVector2D(Center.x, Center.y);
                        WaterTarget = Staging; bWaterTargetReady = true;
                    }
                }
            if (!bWaterTargetReady) { Fail(TEXT("No unobstructed garden approach.")); return; }
            Observations.Add(TEXT("Garden chosen at ") + GardenCenter.ToString() + TEXT("; trial feasibility used a copy, not a live-state edit."));
        }
        if (WalkWaterTarget(WaterTarget, 12, WallDelta, Move, Look)) ++WaterStage;
        break;
    case 7:
        if (WalkWaterTarget(GardenCenter + FVector2D(0, -220), 10, WallDelta, Move, Look)) ++WaterStage;
        break;
    case 8:
        if (!bWaterInputPending)
        {
            Tap(EKeys::Gamepad_FaceButton_Left);
            bWaterInputPending = true;
            WaterStageElapsed = 0;
            break;
        }
        if (WaterStageElapsed < 0.3f) break;
        if (PC->ToastIsError() || PC->State().plots.empty()) { Fail(PC->Toast()); return; }
        for (const auto& Plot : PC->State().plots)
        {
            const auto Center = Homestead::CellCenter(Plot.cellX, Plot.cellY);
            if (FVector2D::Distance(GardenCenter, FVector2D(Center.x, Center.y)) < 1) WaterPlotId = Plot.id;
        }
        if (WaterPlotId < 0) { Fail(TEXT("Mapped tilling did not create the chosen plot.")); return; }
        bWaterInputPending = false;
        ++WaterStage;
        break;
    case 9:
        if (WalkWaterTarget(GardenCenter + FVector2D(0, -55), 12, WallDelta, Move, Look)) ++WaterStage;
        break;
    case 10:
        if (bWaterInputPending)
        {
            if (WaterStageElapsed < 0.3f) break;
            bool Planted = false;
            for (const auto& Plot : PC->State().plots) if (Plot.id == WaterPlotId) Planted = Plot.planted;
            if (PC->ToastIsError() || !Planted) { Fail(PC->Toast()); return; }
            bWaterInputPending = false;
            ++WaterStage;
            break;
        }
        if (PC->FocusTitle() != TEXT("A little patch of earth")) { Fail(TEXT("Plant focus was not the new plot.")); return; }
        Tap(EKeys::Gamepad_FaceButton_Bottom);
        bWaterInputPending = true;
        WaterStageElapsed = 0;
        break;
    case 11:
        if (Entered) { Tap(EKeys::Gamepad_RightThumbstick); Tap(EKeys::Gamepad_RightThumbstick); }
        if (WaterStageElapsed > 0.65f) ++WaterStage;
        break;
    case 12:
        Look.X = -0.65f;
        if (WaterStageElapsed > 1.15f) { Look.X = 0; ++WaterStage; }
        break;
    case 13:
        if (WaterStageElapsed > 0.8f) ++WaterStage;
        break;
    case 14:
        if (Entered)
        {
            WaterBefore = PC->Simulation().Count(Homestead::Item::Water);
            if (!PC->FocusActions().Contains(TEXT("Water"))) { Fail(TEXT("Plot does not offer watering.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            bWaterInputPending = true;
        }
        if (bWaterInputPending && WaterStageElapsed > 0.3f)
        {
            for (const auto& Plot : PC->State().plots)
                if (Plot.id == WaterPlotId)
                    bWatered = Plot.planted && Plot.moisture > 0.99 && PC->Simulation().Count(Homestead::Item::Water) == WaterBefore - 1;
            if (PC->ToastIsError() || !bWatered) { Fail(PC->Toast()); return; }
            bWaterInputPending = false;
        }
        if (WaterStageElapsed > 3.5f) ++WaterStage;
        break;
    case 15:
        if (WaterStageElapsed > 2) { Finish(); return; }
        break;
    case 16:
        if (ForageId < 0)
        {
            float Best = TNumericLimits<float>::Max();
            const auto Position = PC->PlayerPoint();
            for (const auto& Node : PC->State().resources)
            {
                if (Node.kind != Homestead::ResourceKind::Sapling || !PC->Simulation().CanHarvest(Node.id)) continue;
                const FVector2D Target(Node.position.x, Node.position.y);
                const float Distance = FVector2D::Distance(Target, FVector2D(Position.x, Position.y));
                if (Distance < Best) { Best = Distance; ForageId = Node.id; ForageTarget = Target; }
            }
            if (ForageId < 0) { Fail(TEXT("No ready actual sapling to approach.")); return; }
        }
        if (WalkWaterTarget(ForageTarget + FVector2D(0, -420), 15, WallDelta, Move, Look)) ++WaterStage;
        break;
    case 17:
        if (WalkWaterTarget(ForageTarget + FVector2D(0, -110), 15, WallDelta, Move, Look))
        {
            if (!PC->IsResourceFocused(ForageId)) { Fail(TEXT("Approached focus is not the selected sapling.")); return; }
            ++WaterStage;
        }
        break;
    case 18:
        if (Entered) { Tap(EKeys::Gamepad_RightThumbstick); Tap(EKeys::Gamepad_RightThumbstick); }
        if (WaterStageElapsed > 0.65f) ++WaterStage;
        break;
    case 19:
        Look.X = -0.65f;
        if (WaterStageElapsed > 1.15f) { Look.X = 0; ++WaterStage; }
        break;
    case 20:
        if (WaterStageElapsed > 0.8f) ++WaterStage;
        break;
    case 21:
        if (Entered)
        {
            ClearingExpected = PC->Simulation(); ClearingHour = PC->State().hour;
            if (!PC->IsResourceFocused(ForageId) || !ClearingExpected.Clear(ForageId, PC->PlayerPoint()).ok)
            { Fail(TEXT("Actual sapling transaction not available.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Left);
        }
        if (WaterStageElapsed > 0.3f)
        {
            bCleared = MatchesActionState(*PC, ClearingExpected, ClearingHour);
            if (PC->ToastIsError() || !bCleared) { Fail(TEXT("Mapped clear differs from the single expected transaction.")); return; }
        }
        if (WaterStageElapsed > 3.5f) ++WaterStage;
        break;
    case 22:
        if (WaterStageElapsed > 2) { Finish(); return; }
        break;
    }
    if (WaterStageElapsed > 90) { Fail(TEXT("Bounded ordinary approach timed out; no teleport fallback.")); return; }
    ApplyAxes(Move, Look);
    CaptureElapsed += WallDelta;
    if (CaptureElapsed >= ((bClearRoute ? WaterStage >= 18 : WaterStage >= 11) ? 0.125f : 1.0f))
    {
        Capture(Label);
        CaptureElapsed = 0;
    }
}
