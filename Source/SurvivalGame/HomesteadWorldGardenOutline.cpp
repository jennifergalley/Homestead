// The garden outline (AHomesteadController::UpdateGardenOutline): a thin border lying on the ground round
// one garden square. Four edges of short segments that follow the ground between sampled points, lifted
// a little so the soil or grass never swallows them, with no collision and no shadow. Rebuilt only when
// the square or its colour changes.
#include "HomesteadWorld.h"

#include "Simulation/HomesteadSimulation.h"

namespace GardenOutlineStyle
{
constexpr int32 SegmentsPerEdge = 4;
constexpr float LineWidthCm = 3.0f;
constexpr float LineHeightCm = 1.5f;
constexpr float LiftCm = 2.5f;           // above the sampled ground, so turned soil and turf don't hide it
constexpr float Glow = 0.6f;             // emissive, so it reads in shade and at dusk (and casts no shadow)
const FLinearColor Valid(0.32f, 0.86f, 0.36f);
const FLinearColor Blocked(0.9f, 0.3f, 0.24f);
}

void AHomesteadWorld::SetGardenOutline(bool bVisible, int32 CellX, int32 CellY, bool bValid)
{
    using namespace GardenOutlineStyle;
    if (!bVisible || !bInitialized || !Cube)
    {
        if (!GardenOutline.Signature.IsEmpty()) ClearVisual(GardenOutline);
        GardenOutline.Signature.Reset();
        return;
    }
    const FString Signature = FString::Printf(TEXT("%d:%d:%d"), CellX, CellY, bValid ? 1 : 0);
    if (GardenOutline.Signature == Signature) return;
    ClearVisual(GardenOutline);
    GardenOutline.Signature = Signature;

    const Homestead::Point Centre = Homestead::GardenCellCenter(CellX, CellY);
    const float Half = static_cast<float>(Homestead::GardenCellSize) * 0.5f;
    const FVector2D Corners[] = {{Centre.x - Half, Centre.y - Half}, {Centre.x + Half, Centre.y - Half},
                                 {Centre.x + Half, Centre.y + Half}, {Centre.x - Half, Centre.y + Half}};
    const FLinearColor Colour = bValid ? Valid : Blocked;
    for (int32 Edge = 0; Edge < 4; ++Edge)
    {
        const FVector2D From = Corners[Edge], To = Corners[(Edge + 1) % 4];
        for (int32 Segment = 0; Segment < SegmentsPerEdge; ++Segment)
        {
            const FVector2D A = FMath::Lerp(From, To, static_cast<float>(Segment) / SegmentsPerEdge);
            const FVector2D B = FMath::Lerp(From, To, static_cast<float>(Segment + 1) / SegmentsPerEdge);
            const FVector PA(A, GroundHeight(A.X, A.Y) + LiftCm), PB(B, GroundHeight(B.X, B.Y) + LiftCm);
            const FVector Along = PB - PA;
            const FRotator Rotation = Along.Rotation();
            // Overlap the joints by the line width so the corners close.
            AddPart(GardenOutline, Cube, (PA + PB) * 0.5f, FVector(Along.Size() + LineWidthCm, LineWidthCm, LineHeightCm),
                Colour, false, Rotation, 0.9f, Glow);
        }
    }
}
