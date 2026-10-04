#include "HomesteadController.h"
#include "HomesteadControllerHelpers.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "Simulation/HomesteadCrops.h"
#include "Simulation/HomesteadGardenTarget.h"

#include "Engine/World.h"

using HomesteadControllerHelpers::FindPlotWhere;
using HomesteadControllerText::Text;

void AHomesteadController::TillSquareAhead(int32& X, int32& Y) const
{
    // The hoe's blade bites about 85 cm out (Homestead::HoeCellAhead, shared with the garden outline).
    const FVector Forward = GetPawn() ? GetPawn()->GetActorForwardVector() : FVector::ForwardVector;
    int CellX = 0, CellY = 0;
    Homestead::HoeCellAhead(PlayerPoint(), Forward.X, Forward.Y, CellX, CellY);
    X = CellX;
    Y = CellY;
}

void AHomesteadController::HoeSquareAhead()
{
    int32 X = 0, Y = 0;
    TillSquareAhead(X, Y);
    const auto Position = PlayerPoint();
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    // The focused withered crop is the one the card offers to hoe out ("[LMB] Hoe out"), so the hoe
    // takes that one even when the square ahead of her is its neighbour.
    if (Focus == EFocus::Plot)
        if (const auto* Withered = FindPlotWhere(State().plots, [this](const Homestead::Plot& Plot)
            { return Plot.id == FocusId && Plot.planted && Plot.withered; }))
        {
            const Homestead::Point Center = Homestead::PlotCenter(*Withered);
            const auto Result = Sim.ClearWithered(Withered->id, Position);
            NotifyResourceAction(Result, GrassStepA);
            if (Result.ok && Avatar) Avatar->PlayTill(Center);
            return;
        }
    // Already tilled: a withered crop is hoed out, otherwise the hoe takes out its weeds.
    if (const auto* Tilled = FindPlotWhere(State().plots, [X, Y](const Homestead::Plot& Plot)
        { return Plot.cellX == X && Plot.cellY == Y; }))
    {
        const Homestead::Point Center = Homestead::PlotCenter(*Tilled);
        const auto Result = Tilled->withered ? Sim.ClearWithered(Tilled->id, Position) : Sim.Weed(Tilled->id, Position);
        NotifyResourceAction(Result, GrassStepA);
        if (Result.ok && Avatar) Avatar->PlayTill(Center);
        return;
    }
    const auto Result = Sim.Till(X, Y, Position);
    NotifyResourceAction(Result, GrassStepB);
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
    const auto* Packet = HotbarEntry(SelectedHotbarSlot);
    const auto Result = Sim.Plant(FocusId, PlayerPoint(), Crop, Packet ? Packet->groupId : 0);
    NotifyResourceAction(Result, GrassStepB);
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
