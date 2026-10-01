#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"

#include "Engine/World.h"

using HomesteadControllerText::Text;

void AHomesteadController::BeginPlacement(Homestead::Piece Kind)
{
    // A plan she can't afford yet says what's short in the book, as Craft does ("Gather 2 Branch
    // first."), and doesn't start placing (Jenny 2026-09-30). The spot is judged once placing.
    if (Kind != Homestead::Piece::Count)
        if (const auto Afford = Sim.CheckBuildCost(Kind); !Afford) { Notify(Afford); return; }
    CloseBook();
    HoveredHotbarSlot = INDEX_NONE;
    bPlanning = true;
    bDeconstructing = Kind == Homestead::Piece::Count;
    DeconstructId = INDEX_NONE;
    BuildKind = bDeconstructing ? Homestead::Piece::Foundation : Kind;
    BuildRotation = 0;
    BuildYawOffset = 0.0;
    BuildCheckKey.Reset();
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetPlanning(true);
    UpdatePlacement(true);
}

void AHomesteadController::EndPlacement()
{
    bPlanning = false;
    bDeconstructing = false;
    DeconstructId = INDEX_NONE;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->SetPlanning(false);
    if (Landscape) Landscape->SetPlacementPreview(false, BuildTarget, false);
}

void AHomesteadController::ToggleDeconstruct()
{
    // Outside build planning, keyboard X does what controller X does (the secondary action: weed,
    // plant berry seeds, add fuel), so a prompt reading "[X]" is right on either device.
    if (!bPlanning && !bBookOpen && !IsFailed()) { Secondary(); return; }
    if (!bPlanning || bBookOpen || IsFailed()) return;
    bDeconstructing = !bDeconstructing;
    DeconstructId = INDEX_NONE;
    BuildCheckKey.Reset();
    PlayEffect(UIClick, 0.08f);
    if (Landscape) Landscape->SetPlacementPreview(false, BuildTarget, false);
    UpdatePlacement(true);
}

void AHomesteadController::UpdateDeconstruct(bool bForce)
{
    const auto Position = PlayerPoint();
    const FRotator View(0, GetControlRotation().Yaw, 0);
    Homestead::Point Aim{Position.x + View.Vector().X * 350.0, Position.y + View.Vector().Y * 350.0};
    FVector Eye;
    FRotator EyeRotation;
    GetPlayerViewPoint(Eye, EyeRotation);
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadDeconstructAim), false, GetPawn());
    if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Eye, Eye + EyeRotation.Vector() * 1600.0f,
        ECC_Visibility, Query))
        Aim = {Hit.ImpactPoint.X, Hit.ImpactPoint.Y};
    const int32 Target = Sim.FindDeconstructTarget(Aim, 120.0);
    const FString Key = FString::Printf(TEXT("D:%d:%llu"), Target, static_cast<unsigned long long>(Sim.GetRevision()));
    const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
    if (bForce || Key != BuildCheckKey || Now - LastBuildCheckTime >= 0.25)
    {
        DeconstructId = Target;
        const auto Check = Sim.CheckDeconstruct(Target, Position);
        bBuildValid = Check.ok;
        BuildBlocker = Check.ok ? FString() : Text(Check.message.c_str());
        BuildCheckKey = Key;
        LastBuildCheckTime = Now;
    }
    if (Landscape) Landscape->SetDeconstructPreview(State(), DeconstructId, bBuildValid);
}

void AHomesteadController::UpdatePlacement(bool bForce)
{
    if (!bPlanning) return;
    if (bDeconstructing) { UpdateDeconstruct(bForce); return; }
    const auto Position = PlayerPoint();
    // Aim 3.5 m ahead of her along the camera; whole centimetres keep the snap search stable.
    const FRotator View(0, GetControlRotation().Yaw, 0);
    const FVector Forward = View.Vector();
    const Homestead::Point Aim{FMath::RoundToDouble(Position.x + Forward.X * 350.0),
        FMath::RoundToDouble(Position.y + Forward.Y * 350.0)};
    // Free-standing pieces face the way the camera does, in 5 degree steps, plus her own turns.
    const double FreeYaw = FMath::RoundToDouble(View.Yaw / 5.0) * 5.0 + BuildYawOffset;
    BuildTarget = Sim.ResolvePlacement(BuildKind, Aim, FreeYaw, BuildRotation);
    const FString Key = FString::Printf(TEXT("%d:%d:%d:%d:%d:%.0f:%.0f:%.1f:%llu"), static_cast<int>(BuildTarget.kind),
        BuildTarget.buildingId, BuildTarget.cellX, BuildTarget.cellY, BuildTarget.rotation,
        BuildTarget.frame.origin.x, BuildTarget.frame.origin.y, BuildTarget.frame.yaw,
        static_cast<unsigned long long>(Sim.GetRevision()));
    const double Now = GetWorld() ? GetWorld()->GetRealTimeSeconds() : 0.0;
    if (bForce || (Key != BuildCheckKey && Now - LastBuildCheckTime >= 0.1))
    {
        auto Check = Sim.CheckPlacement(BuildTarget, Position, true);
        if (Check.ok) Check = Sim.CheckBuildCost(BuildKind);
        bBuildValid = Check.ok;
        BuildBlocker = Check.ok ? FString() : Text(Check.message.c_str());
        BuildCheckKey = Key;
        LastBuildCheckTime = Now;
    }
    if (Landscape) Landscape->SetPlacementPreview(true, BuildTarget, bBuildValid);
}

void AHomesteadController::RotatePlacement() { RotatePlacementBy(1); }

void AHomesteadController::RotatePlacementBy(int32 Direction)
{
    if (!bPlanning || bDeconstructing) return;
    const bool bQuarterTurns = BuildTarget.snapped || BuildKind == Homestead::Piece::Wall
        || BuildKind == Homestead::Piece::Doorway || BuildKind == Homestead::Piece::Roof;
    if (bQuarterTurns) BuildRotation = ((BuildRotation + Direction) % 4 + 4) % 4;
    else BuildYawOffset = FMath::Fmod(BuildYawOffset + 15.0 * Direction + 360.0, 360.0);
    UpdatePlacement(true);
}

FString AHomesteadController::PlacementLabel() const
{
    if (bDeconstructing)
    {
        for (const auto& Structure : State().structures)
            if (Structure.id == DeconstructId)
            {
                FString Cost = Text(Homestead::PieceRequirements(Structure.kind));
                int32 Notes = INDEX_NONE;
                if (Cost.FindChar(TEXT(';'), Notes)) Cost.LeftInline(Notes);
                return FString::Printf(TEXT("Take down %s  |  Returns %s"),
                    *Text(Homestead::PieceName(Structure.kind)), *Cost);
            }
        return TEXT("Take down  |  Aim at something you built");
    }
    return FString::Printf(TEXT("%s  |  %s"), *Text(Homestead::PieceName(BuildKind)), *Text(Homestead::PieceRequirements(BuildKind)));
}

FString AHomesteadController::PlacementStatus() const
{
    if (!BuildBlocker.IsEmpty()) return BuildBlocker;
    if (bDeconstructing)
        return TEXT("Everything it cost comes back to your pack; a chest's contents come with it.");
    return BuildTarget.snapped ? FString(TEXT("Snaps onto your building."))
        : FString(TEXT("Free-standing: it faces the way you look; rotate turns it."));
}

void AHomesteadController::CycleZoom()
{
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->CycleZoom();
}
