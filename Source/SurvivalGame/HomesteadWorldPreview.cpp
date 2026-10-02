#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

using HomesteadWorldLook::PreviewBlockedColor;
using HomesteadWorldLook::PreviewColor;
using HomesteadWorldLook::DeconstructColor;

void AHomesteadWorld::SetPlacementPreview(bool Visible, const Homestead::PlacementTarget& Target, bool bValid)
{
    if (!Visible || !bInitialized || Target.kind == Homestead::Piece::Count)
    {
        ClearVisual(Preview);
        return;
    }
    const bool bOnFoundation = Target.buildingId >= 0
        && FoundationCells.Contains(FIntVector(Target.cellX, Target.cellY, Target.buildingId));
    const FString Signature = FString::Printf(TEXT("%d:%d:%d:%d:%d:%.1f:%.1f:%.2f:%d:%d"), static_cast<int>(Target.kind),
        Target.buildingId, Target.cellX, Target.cellY, Target.rotation, Target.frame.origin.x, Target.frame.origin.y,
        Target.frame.yaw, bOnFoundation, bValid);
    if (Preview.Signature == Signature)
    {
        return;
    }
    ClearVisual(Preview);
    Homestead::Structure Structure;
    Structure.kind = Target.kind;
    Structure.buildingId = Target.buildingId;
    Structure.cellX = Target.cellX;
    Structure.cellY = Target.cellY;
    Structure.rotation = Target.rotation;
    BuildStructure(Preview, Structure, Target.frame, bOnFoundation, true, bValid);
    Preview.Signature = Signature;
}

void AHomesteadWorld::SetDeconstructPreview(const Homestead::State& State, int32 StructureId, bool bValid)
{
    if (!bInitialized || StructureId < 0)
    {
        ClearVisual(Preview);
        return;
    }
    const Homestead::Structure* Structure = nullptr;
    for (const auto& Piece : State.structures)
        if (Piece.id == StructureId) { Structure = &Piece; break; }
    if (!Structure)
    {
        ClearVisual(Preview);
        return;
    }
    const bool bOnFoundation = Homestead::HasFoundation(State, Structure->buildingId, Structure->cellX, Structure->cellY);
    const FString Signature = FString::Printf(TEXT("D:%d:%d:%d"), StructureId, bOnFoundation, bValid);
    if (Preview.Signature == Signature) return;
    ClearVisual(Preview);
    const Homestead::Building* Frame = Homestead::FindBuilding(State, Structure->buildingId);
    BuildStructure(Preview, *Structure, Frame ? *Frame : Homestead::Building{}, bOnFoundation, true, bValid, true);
    Preview.Signature = Signature;
}
