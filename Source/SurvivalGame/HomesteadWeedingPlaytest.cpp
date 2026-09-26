#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"

void AHomesteadVisualPlaytest::TickWeeding(float WallDelta)
{
    if (!Passes.IsValidIndex(PassIndex) || Elapsed > 150 || PC->IsFailed()) { Finish(); return; }
    const auto& Pass = Passes[PassIndex];
    auto Fail = [this](const FString& Reason)
    {
        Observations.Add(TEXT("FAILED disclosed-fixture weeding approach/action: ") + Reason);
        Finish();
    };
    if (!bEntered)
    {
        bEntered = true;
        PassElapsed = 0;
        CaptureElapsed = 0;
        WaterSettle = 0;
        Observations.Add(FString::Printf(TEXT("BEGIN %.2fs %s: %s"), Elapsed, *Pass.Label, *PC->FocusTitle()));
        if (PassIndex == 3) { Tap(EKeys::Gamepad_RightThumbstick); Tap(EKeys::Gamepad_RightThumbstick); }
        if (PassIndex == 6)
        {
            if (!PC->FocusActions().Contains(TEXT("Weed"))) { Fail(TEXT("Approached plot does not offer weeding.")); return; }
            WaterBefore = PC->Simulation().Count(Homestead::Item::Water);
        }
        Tap(Pass.Press);
    }
    PassElapsed += WallDelta;
    FVector2D Move = Pass.Move, Look = Pass.Look;
    bool Complete = PassElapsed >= Pass.Duration;
    if (PassIndex == 0 && Complete)
    {
        if (PC->ToastIsError() || PC->IsBookOpen()) { Fail(TEXT("Explicit fixture failed to load.")); return; }
        for (const auto& Plot : PC->State().plots)
            if (Plot.planted && Plot.weeds >= 0.125 && Plot.growth < 1)
            {
                WaterPlotId = Plot.id;
                const auto Center = Homestead::PlotCenter(Plot);
                GardenCenter = FVector2D(Center.x, Center.y);
                Observations.Add(FString::Printf(TEXT("Loaded existing planted plot %d with %.6f weeds at game hour %.6f; no setup mutation."),
                    Plot.id, Plot.weeds, PC->State().hour));
                break;
            }
        if (WaterPlotId < 0) { Fail(TEXT("Save has no immature planted plot with visible weeds.")); return; }
    }
    if (PassIndex == 1 || PassIndex == 2)
    {
        const FVector2D Target = GardenCenter + FVector2D(0, PassIndex == 1 ? -450 : -60);
        Complete = WalkWaterTarget(Target, 15, WallDelta, Move, Look);
        if (!Complete && PassElapsed >= Pass.Duration) { Fail(TEXT("Bounded mapped walking timed out; no teleport fallback.")); return; }
    }
    if (PassIndex == 6 && PassElapsed >= 0.3f && !bWeeded)
    {
        for (const auto& Plot : PC->State().plots)
            if (Plot.id == WaterPlotId)
                bWeeded = Plot.planted && Plot.weeds < 0.001 && PC->Simulation().Count(Homestead::Item::Water) == WaterBefore;
        if (!bWeeded || PC->ToastIsError()) { Fail(TEXT("Mapped X did not perform the existing successful weed transaction.")); return; }
    }
    ApplyAxes(Move, Look);
    CaptureElapsed += WallDelta;
    if (CaptureElapsed >= (PassIndex >= 3 ? 0.125f : 0.5f))
    {
        Capture(Pass.Label);
        CaptureElapsed = 0;
    }
    if (Complete) { ++PassIndex; bEntered = false; }
}
