#include "HomesteadController.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"

#include "Engine/World.h"

using HomesteadControllerHelpers::FindPlotWhere;
using HomesteadControllerText::Text;

void AHomesteadController::TillSquareAhead(int32& X, int32& Y) const
{
    // The hoe's blade bites about 85 cm out; probing there keeps the bite inside the chosen square.
    const auto Position = PlayerPoint();
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    X = Homestead::GardenCell(Position.x + Forward.X * 85);
    Y = Homestead::GardenCell(Position.y + Forward.Y * 85);
}

void AHomesteadController::HoeSquareAhead()
{
    int32 X = 0, Y = 0;
    TillSquareAhead(X, Y);
    const auto Position = PlayerPoint();
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    // Already tilled: hoe out its weeds instead.
    if (const auto* Tilled = FindPlotWhere(State().plots, [X, Y](const Homestead::Plot& Plot)
        { return Plot.cellX == X && Plot.cellY == Y; }))
    {
        const auto Result = Sim.Weed(Tilled->id, Position);
        Notify(Result, GrassStepA);
        if (Result.ok && Avatar) Avatar->PlayTill(Homestead::PlotCenter(*Tilled));
        return;
    }
    const auto Result = Sim.Till(X, Y, Position);
    Notify(Result, GrassStepB);
    if (!Result.ok || !Avatar) return;
    Avatar->PlayTill(Homestead::GardenCellCenter(X, Y));
    // The turned soil appears when the hoe first bites.
    if (Avatar->UsesHoeTill() && Landscape && !State().plots.empty())
    {
        Landscape->HoldPlot(State().plots.back().id, true);
        HeldPlot = State().plots.back().id;
        HeldPlotSince = GetWorld()->GetTimeSeconds();
        bHeldPlotTilling = true;
    }
}

void AHomesteadController::PresentHarvest(int32 PlotId, Homestead::CropKind Crop, Homestead::Point Center)
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (!Avatar) return;
    const bool bPick = Homestead::GetCropInfo(Crop).style == Homestead::HarvestStyle::Pick;
    UStaticMesh* Produce = Landscape ? Landscape->CropMesh(Crop, TEXT("Harvest")) : nullptr;
    if (!Avatar->PlayHarvest(Center, bPick, Produce) || !Landscape) return;
    // The ripe plant stays in the ground until her hands lift the crop out of it.
    Landscape->HoldHarvest(PlotId, Crop);
    HeldHarvestPlot = PlotId;
    HeldHarvestSince = GetWorld()->GetTimeSeconds();
    RefreshRemaining = 0;
}

void AHomesteadController::PlantFocusedPlot(Homestead::CropKind Crop)
{
    const auto* Plot = FindPlotWhere(State().plots, [this](const Homestead::Plot& Candidate) { return Candidate.id == FocusId; });
    if (!Plot) return;
    const auto Target = Homestead::PlotCenter(*Plot);
    const auto Result = Sim.Plant(FocusId, PlayerPoint(), Crop);
    Notify(Result, GrassStepB);
    if (!Result.ok) return;
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    if (Avatar && Avatar->PlayPlant(Target) && Landscape)
    {
        Landscape->HoldPlot(FocusId);
        HeldPlot = FocusId;
        bHeldPlotTilling = false;
        HeldPlotSince = GetWorld()->GetTimeSeconds();
    }
}
