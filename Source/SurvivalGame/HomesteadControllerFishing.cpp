#include "HomesteadController.h"
#include "HomesteadEstateTerrain.h"
#include "Components/SplineComponent.h"
#include "Simulation/HomesteadFishing.h"

namespace HomesteadWaterProbe { bool ShoreContains(const USplineComponent& Spline, const FVector2D& Point); }
DEFINE_LOG_CATEGORY_STATIC(LogHomesteadFishingController, Log, All);

Homestead::FishingWater AHomesteadController::ProbeFishingWater(Homestead::Point Position) const
{
    if (!bEstateMap)
        return Sim.NearWater(Position) ? Homestead::FishingWater::River : Homestead::FishingWater::None;
    WaterEdgeDistance(Position, false);
    const FVector Here(Position.x, Position.y, GroundHeight(Position.x, Position.y));
    double Best = Homestead::Fishing::BankReachCm;
    auto Water = Homestead::FishingWater::None;
    for (const auto& Weak : EstateWaterSplines)
        if (const USplineComponent* Spline = Weak.Get())
        {
            const FVector Closest = Spline->FindLocationClosestToWorldLocation(Here, ESplineCoordinateSpace::World);
            const float Key = Spline->FindInputKeyClosestToWorldLocation(Here);
            double Distance = FVector::Dist2D(Closest, Here)
                - (Spline->IsClosedLoop() ? 0.0 : 100.0 * Spline->GetScaleAtSplineInputKey(Key).Y);
            if (Spline->IsClosedLoop() && HomesteadWaterProbe::ShoreContains(*Spline, FVector2D(Position.x, Position.y)))
                Distance = -Distance;
            if (Distance <= Best)
            {
                Best = Distance;
                Water = Spline->IsClosedLoop() ? Homestead::FishingWater::Lake : Homestead::FishingWater::River;
            }
        }
    if (Water != Homestead::FishingWater::None) return Water;
    return WaterEdgeDistance(Position, true) <= Homestead::Fishing::BankReachCm
        ? Homestead::FishingWater::Ocean : Homestead::FishingWater::None;
}

bool AHomesteadController::IsFishing() const
{
    return Sim.FishingCast().phase != Homestead::FishingPhase::Idle;
}

bool AHomesteadController::UpdateFishingFocus(Homestead::Point Position)
{
    FocusedFishingWater = Homestead::FishingWater::None;
    FishingFocusText.Reset();
    if (SelectedCarriedTool() != Homestead::Item::FishingPole) return false;
    FocusedFishingWater = Sim.FishingWaterAt(Position);
    if (FocusedFishingWater == Homestead::FishingWater::None) return false;
    if (!IsFishing())
    {
        const auto Ready = Sim.CheckFishing(Position);
        FishingFocusText = Ready.ok ? bGamepad ? TEXT("[RT] Cast line") : TEXT("[LMB] Cast line")
            : UTF8_TO_TCHAR(Ready.message.c_str());
    }
    return true;
}

void AHomesteadController::FishingInput()
{
    if (!ShouldShowHotbar() || HasNativeMenu() || IsNewGameSetup() || IsNamingSetup()) return;
    const auto Result = Sim.FishingPress(PlayerPoint());
    if (!Result.message.empty())
    {
        if (Result.ok && IsFishing()) NotifyResourceAction(Result, nullptr);
        else Notify(Result);
    }
}

void AHomesteadController::TickFishing(float DeltaSeconds)
{
    if (!IsFishing()) return;
    if (!ShouldShowHotbar() || HasNativeMenu() || IsNewGameSetup() || IsNamingSetup()
        || HotbarItem(SelectedHotbarSlot) != Homestead::Item::FishingPole)
    {
        Notify(Sim.CancelFishing());
        return;
    }
    const auto Result = Sim.AdvanceFishing(DeltaSeconds, PlayerPoint());
    if (!Result.ok)
    {
        UE_LOG(LogHomesteadFishingController, Error, TEXT("Fishing update refused: %s"), UTF8_TO_TCHAR(Result.message.c_str()));
        Sim.CancelFishing();
    }
    if (!Result.message.empty()) Notify(Result);
}

FString AHomesteadController::FishingPrompt() const
{
    const auto& Cast = Sim.FishingCast();
    const TCHAR* Press = UsesGamepad() ? TEXT("[A] / [RT]") : TEXT("[E] / [LMB]");
    switch (Cast.phase)
    {
    case Homestead::FishingPhase::Waiting: return TEXT("Line cast. Wait for the bite...");
    case Homestead::FishingPhase::Bite: return FString::Printf(TEXT("A bite! Press %s now to hook it."), Press);
    case Homestead::FishingPhase::Landing:
        return FString::Printf(TEXT("Press %s in the green band. Land it: %d / %d"),
            Press, Cast.landedBeats, Homestead::Fishing::LandingBeats);
    default: return FString();
    }
}
