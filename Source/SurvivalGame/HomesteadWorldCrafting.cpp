// Period crafting in the world (rework-farming-calendar-and-period-crafting, lane E): the workbench and
// sawhorse, post-and-rail fences, field gates and small furniture. The rules are in
// Simulation/HomesteadCrafting.cpp; this only presents them. Each piece uses its Blender mesh from
// /Game/SurvivalGame/Environment/Props/EstateFence and EstateFurniture when imported, otherwise a
// blockout of cubes with the same footprint, so the game works before the art lands.
#include "HomesteadWorld.h"

#include "Simulation/HomesteadCrafting.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadCraftVisuals, Log, All);

namespace HomesteadCraftVisuals
{
const FLinearColor Oak(0.30f, 0.22f, 0.14f);
const FLinearColor Deal(0.46f, 0.33f, 0.19f);
const FLinearColor DarkWood(0.20f, 0.13f, 0.07f);
const FLinearColor Rust(0.16f, 0.09f, 0.05f);
const FLinearColor PreviewOk(0.65f, 0.79f, 0.77f);
const FLinearColor PreviewBad(0.86f, 0.33f, 0.26f);
const FLinearColor TakeDown(0.93f, 0.62f, 0.2f);
// Fence geometry, matching Scripts/Blender/Recipes/estate_fence.py: rail centre heights and the post.
constexpr float RailHeights[] = {30.0f, 65.0f, 100.0f};
constexpr float PostHeight = 115.0f;
constexpr float PostSink = 12.0f;
constexpr float PostSize = 11.0f;
// The gate leaf runs from just off the hinge post to just short of the far post, 8-112 cm up.
constexpr float GateLeafStart = 7.0f;
constexpr float GateLeafEnd = 233.0f;
constexpr float GateLeafBottom = 8.0f;
constexpr float GateLeafTop = 112.0f;
// The Blender furniture faces Unreal +Y; a piece's front faces -Y in piece space (its back to the
// wall, like the hearth), so the meshes turn half round.
constexpr float FurnitureMeshYaw = 180.0f;
// How tall each station or piece of furniture blocks her, cm above its base.
float BlockHeight(Homestead::Piece Kind)
{
    switch (Kind)
    {
    case Homestead::Piece::Workbench: return 88.0f;
    case Homestead::Piece::Sawhorse: return 80.0f;
    case Homestead::Piece::Stool: return 40.0f;
    case Homestead::Piece::Table: return 78.0f;
    case Homestead::Piece::Chair: return 92.0f;
    case Homestead::Piece::Shelf: return 140.0f;
    default: return 100.0f;
    }
}
const TCHAR* MeshName(Homestead::Piece Kind)
{
    switch (Kind)
    {
    case Homestead::Piece::Workbench: return TEXT("EstateWorkbench");
    case Homestead::Piece::Sawhorse: return TEXT("EstateSawhorse");
    case Homestead::Piece::Stool: return TEXT("EstateStool");
    case Homestead::Piece::Table: return TEXT("EstateTable");
    case Homestead::Piece::Chair: return TEXT("EstateChair");
    case Homestead::Piece::Shelf: return TEXT("EstateShelf");
    default: return nullptr;
    }
}
}

UStaticMesh* AHomesteadWorld::CraftMesh(const TCHAR* Folder, const TCHAR* Name)
{
    const FName Key(*FString::Printf(TEXT("%s/%s"), Folder, Name));
    if (const TObjectPtr<UStaticMesh>* Found = CraftMeshes.Find(Key)) return Found->Get();
    // Not cached while missing, so a mesh imported mid-session shows up on the next rebuild.
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/SM_%s.SM_%s"), Folder, Name, Name),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (Mesh) CraftMeshes.Add(Key, Mesh);
    else UE_LOG(LogHomesteadCraftVisuals, Verbose, TEXT("%s not imported yet; drawing its blockout."), *Key.ToString());
    return Mesh;
}

bool AHomesteadWorld::BuildCraftedPiece(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure,
    const Homestead::Building& Frame, bool bOnFoundation, bool bPreview, bool bValid, bool bDeconstruct, uint32 PostMask)
{
    namespace Style = HomesteadCraftVisuals;
    namespace Crafting = Homestead::Crafting;
    const Homestead::Piece Kind = Structure.kind;
    const bool bFence = Crafting::IsFence(Kind);
    if (!bFence && !Crafting::IsStation(Kind) && !Crafting::IsMovable(Kind)) return false;
    const FLinearColor Tint = bDeconstruct ? (bValid ? Style::TakeDown : Style::PreviewBad)
        : (bValid ? Style::PreviewOk : Style::PreviewBad);
    const float Grow = bDeconstruct ? 1.02f : 1.0f;

    // An imported mesh keeping its own baked materials (tinted as a whole while previewing).
    auto Kit = [&](UStaticMesh* Mesh, const FVector& Where, const FRotator& Rotation) -> UStaticMeshComponent*
    {
        UStaticMeshComponent* Part = AddPart(Visual, Mesh, Where, FVector(100.0f * Grow), bPreview ? Tint : FLinearColor::White,
            false, Rotation);
        if (!Part) return nullptr;
        for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
            Part->SetMaterial(Slot, bPreview ? Material(Tint) : Mesh->GetMaterial(Slot));
        return Part;
    };
    // A blockout cube in a local frame (Origin, Rotation): Offset and Size in cm, Size centred.
    auto Block = [&](const FVector& Origin, const FRotator& Rotation, const FVector& Offset, const FVector& Size,
        const FLinearColor& Color, const FRotator& Local = FRotator::ZeroRotator) -> UStaticMeshComponent*
    {
        const FRotator Combined = (Rotation.Quaternion() * Local.Quaternion()).Rotator();
        return AddPart(Visual, Cube, Origin + Rotation.RotateVector(Offset), Size * Grow + FVector(bDeconstruct ? 4.0f : 0.0f),
            bPreview ? Tint : Color, false, Combined);
    };
    // A hidden box that stops her (and later livestock) but not the camera or aim traces.
    auto Blocker = [&](const FVector& Centre, const FVector& Extent, const FRotator& Rotation) -> UBoxComponent*
    {
        if (bPreview) return nullptr;
        auto* Box = NewObject<UBoxComponent>(this);
        Box->SetupAttachment(GetRootComponent());
        Box->SetMobility(EComponentMobility::Movable);
        Box->SetBoxExtent(Extent, false);
        Box->SetRelativeTransform(FTransform(Rotation, Centre));
        Box->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        Box->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        Box->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        Box->SetGenerateOverlapEvents(false);
        Box->SetCanEverAffectNavigation(false);
        Box->SetHiddenInGame(true);
        Box->ComponentTags.Add(TEXT("CraftedBlocker"));
        Box->RegisterComponent();
        Visual.Components.Add(Box);
        return Box;
    };

    const float Yaw = static_cast<float>(Homestead::PieceYaw(Frame, Structure.rotation));
    if (bFence)
    {
        const Homestead::Point A = Crafting::FenceEnd(Frame, Structure.cellX, Structure.cellY, Structure.rotation, 0);
        const Homestead::Point B = Crafting::FenceEnd(Frame, Structure.cellX, Structure.cellY, Structure.rotation, 1);
        const float ZA = GroundHeight(A.x, A.y), ZB = GroundHeight(B.x, B.y);
        auto Post = [&](Homestead::Point At, float Z)
        {
            const FVector Where(At.x, At.y, Z);
            if (UStaticMesh* Mesh = CraftMesh(TEXT("EstateFence"), TEXT("EstateFencePost"))) Kit(Mesh, Where, FRotator(0, Yaw, 0));
            else Block(Where, FRotator(0, Yaw, 0), FVector(0, 0, (Style::PostHeight - Style::PostSink) * 0.5f),
                FVector(Style::PostSize, Style::PostSize, Style::PostHeight + Style::PostSink), Style::Oak);
        };
        if (PostMask & 1u) Post(A, ZA);
        if (PostMask & 2u) Post(B, ZB);
        const FVector Mid((A.x + B.x) * 0.5, (A.y + B.y) * 0.5, (ZA + ZB) * 0.5f);
        if (Kind == Homestead::Piece::FenceRail)
        {
            // The rails follow the ground between their posts.
            const float Pitch = FMath::RadiansToDegrees(FMath::Atan2(ZB - ZA, static_cast<float>(Crafting::FenceSpan)));
            const FRotator Along(Pitch, Yaw, 0);
            if (UStaticMesh* Mesh = CraftMesh(TEXT("EstateFence"), TEXT("EstateFenceRail"))) Kit(Mesh, Mid, Along);
            else
                for (float Height : Style::RailHeights)
                    Block(Mid, Along, FVector(0, 0, Height), FVector(Crafting::FenceSpan - 6.0f, 4.0f, 9.0f), Style::Oak);
            Blocker(Mid + FVector(0, 0, 60), FVector(Crafting::FenceSpan * 0.5f, 7.0f, 60.0f), FRotator(0, Yaw, 0));
            return true;
        }
        // A field gate hangs from its -X post and swings a quarter turn open about it.
        const FVector Hinge(A.x, A.y, ZA);
        const float LeafYaw = Yaw + (Structure.open ? 90.0f : 0.0f);
        USceneComponent* Leaf = NewObject<USceneComponent>(this);
        Leaf->SetupAttachment(GetRootComponent());
        Leaf->SetMobility(EComponentMobility::Movable);
        Leaf->SetRelativeTransform(FTransform(FRotator(0, LeafYaw, 0), Hinge));
        Leaf->RegisterComponent();
        Visual.Components.Add(Leaf);
        const int32 First = Visual.Components.Num();
        if (UStaticMesh* Mesh = CraftMesh(TEXT("EstateFence"), TEXT("EstateFieldGate"))) Kit(Mesh, Hinge, FRotator(0, LeafYaw, 0));
        else
        {
            const FRotator Turn(0, LeafYaw, 0);
            const float Middle = (Style::GateLeafStart + Style::GateLeafEnd) * 0.5f;
            const float Length = Style::GateLeafEnd - Style::GateLeafStart;
            const float Rise = Style::GateLeafTop - Style::GateLeafBottom;
            Block(Hinge, Turn, FVector(Style::GateLeafStart + 4.5f, 0, Style::GateLeafBottom + Rise * 0.5f), FVector(9, 9, Rise), Style::Oak);
            Block(Hinge, Turn, FVector(Style::GateLeafEnd - 3.5f, 0, Style::GateLeafBottom + Rise * 0.5f), FVector(7, 7, Rise), Style::Oak);
            for (int32 Bar = 0; Bar < 5; ++Bar)
                Block(Hinge, Turn, FVector(Middle, 0, Style::GateLeafBottom + 8.0f + Bar * (Rise - 16.0f) / 4.0f),
                    FVector(Length - 14.0f, 3.5f, 9.0f), Style::Deal);
            const float Brace = FMath::RadiansToDegrees(FMath::Atan2(Rise - 16.0f, Length - 14.0f));
            Block(Hinge, Turn, FVector(Middle, 2.5f, Style::GateLeafBottom + Rise * 0.5f),
                FVector(FMath::Sqrt(FMath::Square(Length - 14.0f) + FMath::Square(Rise - 16.0f)), 3.0f, 8.0f), Style::Deal,
                FRotator(Brace, 0, 0));
            for (float Strap : {Style::GateLeafBottom + 12.0f, Style::GateLeafTop - 12.0f})
                Block(Hinge, Turn, FVector(Style::GateLeafStart + 22.0f, -5.5f, Strap), FVector(40, 1.5f, 5), Style::Rust);
        }
        // Everything drawn for the leaf turns with it about the hinge.
        for (int32 Index = First; Index < Visual.Components.Num(); ++Index)
            if (USceneComponent* Part = Visual.Components[Index].Get())
                Part->AttachToComponent(Leaf, FAttachmentTransformRules::KeepWorldTransform);
        if (!bPreview)
        {
            const FRotator Open(0, LeafYaw, 0);
            Blocker(Hinge + Open.RotateVector(FVector(Crafting::FenceSpan * 0.5f, 0, 60)),
                FVector(Crafting::FenceSpan * 0.5f - 6.0f, 6.0f, 58.0f), Open);
            // Swing from where it was shown last, if that differs.
            const bool* Shown = GateShownOpen.Find(Structure.id);
            if (Shown && *Shown != Structure.open)
            {
                FGateSwing& Swing = GateSwings.FindOrAdd(Structure.id);
                Swing.Leaf = Leaf;
                Swing.FromYaw = Yaw + (*Shown ? 90.0f : 0.0f);
                Swing.ToYaw = LeafYaw;
                Swing.Age = 0.0f;
                Leaf->SetRelativeRotation(FRotator(0, Swing.FromYaw, 0));
            }
            GateShownOpen.Add(Structure.id, Structure.open);
        }
        return true;
    }

    // Stations and small furniture: on a floor at their offset or spot, off it centred and leaning
    // with the slope under their own footprint.
    const Homestead::Point Cell = Homestead::BuildingCellCenter(Frame, Structure.cellX, Structure.cellY);
    const Homestead::Point Offset = bOnFoundation
        ? (Crafting::IsMovable(Kind) ? Structure.spot : Homestead::FurnitureOffset(Kind)) : Homestead::Point{};
    const Homestead::Point Turned = Homestead::RotateYaw(Offset, Yaw);
    const FVector2D Centre(Cell.x + Turned.x, Cell.y + Turned.y);
    FRotator Rotation(0, Yaw, 0);
    FVector Base(Centre.X, Centre.Y, 0);
    const Homestead::Point Half = Crafting::FurnitureHalf(Kind);
    if (bOnFoundation) Base.Z = StructureBase(Cell, Frame.yaw);
    else
    {
        const FVector AxisX = Rotation.RotateVector(FVector::ForwardVector);
        const FVector AxisY = Rotation.RotateVector(FVector::RightVector);
        auto GroundAt = [&](float DX, float DY)
        {
            const FVector P = FVector(Centre, 0) + AxisX * DX + AxisY * DY;
            return GroundHeight(P.X, P.Y);
        };
        const float SlopeX = (GroundAt(static_cast<float>(Half.x), 0) - GroundAt(-static_cast<float>(Half.x), 0)) / (2.0f * static_cast<float>(Half.x));
        const float SlopeY = (GroundAt(0, static_cast<float>(Half.y)) - GroundAt(0, -static_cast<float>(Half.y))) / (2.0f * static_cast<float>(Half.y));
        Rotation = FRotationMatrix::MakeFromXY(AxisX + FVector::UpVector * SlopeX, AxisY + FVector::UpVector * SlopeY).Rotator();
        Base.Z = GroundHeight(Centre.X, Centre.Y) - 1.5f;
    }
    const TCHAR* Name = Style::MeshName(Kind);
    if (UStaticMesh* Mesh = Name ? CraftMesh(TEXT("EstateFurniture"), Name) : nullptr)
        Kit(Mesh, Base, (Rotation.Quaternion() * FRotator(0, Style::FurnitureMeshYaw, 0).Quaternion()).Rotator());
    else
    {
        switch (Kind)
        {
        case Homestead::Piece::Workbench:
            Block(Base, Rotation, FVector(0, 0, 80.5f), FVector(180, 62, 7), Style::Deal);
            for (int32 X : {-1, 1}) for (int32 Y : {-1, 1})
                Block(Base, Rotation, FVector(X * 80.0f, Y * 24.0f, 38.5f), FVector(8, 8, 77), Style::Oak);
            Block(Base, Rotation, FVector(0, 0, 20.0f), FVector(170, 50, 3), Style::Oak);
            Block(Base, Rotation, FVector(-72.0f, -33.0f, 62.0f), FVector(12, 6, 34), Style::DarkWood);
            break;
        case Homestead::Piece::Sawhorse:
            Block(Base, Rotation, FVector(0, 0, 63.0f), FVector(95, 12, 10), Style::Oak);
            for (int32 X : {-1, 1}) for (int32 Y : {-1, 1})
                Block(Base, Rotation, FVector(X * 38.0f, Y * 14.0f, 31.0f), FVector(6, 6, 66), Style::Oak, FRotator(0, 0, Y * 14.0f));
            Block(Base, Rotation, FVector(0, 0, 78.0f), FVector(22, 110, 20), Style::DarkWood);
            break;
        case Homestead::Piece::Stool:
            Block(Base, Rotation, FVector(0, 0, 36.0f), FVector(32, 32, 4), Style::Deal);
            for (int32 Leg = 0; Leg < 3; ++Leg)
            {
                const float Angle = Leg * 2.0f * PI / 3.0f;
                Block(Base, Rotation, FVector(FMath::Cos(Angle) * 11.0f, FMath::Sin(Angle) * 11.0f, 17.0f), FVector(4, 4, 34), Style::Oak);
            }
            break;
        case Homestead::Piece::Table:
            Block(Base, Rotation, FVector(0, 0, 74.0f), FVector(120, 75, 4), Style::Deal);
            Block(Base, Rotation, FVector(0, 0, 66.0f), FVector(108, 63, 12), Style::Oak);
            for (int32 X : {-1, 1}) for (int32 Y : {-1, 1})
                Block(Base, Rotation, FVector(X * 54.0f, Y * 31.0f, 34.0f), FVector(6, 6, 68), Style::Oak);
            break;
        case Homestead::Piece::Chair:
            Block(Base, Rotation, FVector(0, 0, 44.0f), FVector(42, 40, 4), Style::Deal);
            for (int32 X : {-1, 1}) for (int32 Y : {-1, 1})
                Block(Base, Rotation, FVector(X * 18.0f, Y * 17.0f, 22.0f), FVector(4, 4, 44), Style::Oak);
            for (int32 X : {-1, 1})
                Block(Base, Rotation, FVector(X * 18.0f, 18.0f, 68.0f), FVector(4, 4, 48), Style::Oak);
            for (float Slat : {60.0f, 74.0f, 88.0f})
                Block(Base, Rotation, FVector(0, 18.0f, Slat), FVector(36, 2, 6), Style::Deal);
            break;
        case Homestead::Piece::Shelf:
            for (int32 X : {-1, 1})
                Block(Base, Rotation, FVector(X * 48.0f, 0, 70.0f), FVector(3, 32, 140), Style::Oak);
            for (float Board : {20.0f, 70.0f, 120.0f})
                Block(Base, Rotation, FVector(0, 0, Board), FVector(100, 32, 2.5f), Style::Deal);
            Block(Base, Rotation, FVector(0, 15.0f, 130.0f), FVector(100, 2, 8), Style::Oak);
            break;
        default:
            break;
        }
    }
    const float Height = Style::BlockHeight(Kind);
    Blocker(Base + FVector(0, 0, Height * 0.5f), FVector(Half.x * 0.9f, Half.y * 0.9f, Height * 0.5f), FRotator(0, Yaw, 0));
    if (bPreview)
        for (USceneComponent* Component : Visual.Components)
            if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component)) Mesh->SetCastShadow(false);
    return true;
}

void AHomesteadWorld::UpdateGateSwings(float DeltaSeconds)
{
    // About the time her hand takes to push a gate through a quarter turn.
    constexpr float GateSwingSeconds = 0.8f;
    for (auto It = GateSwings.CreateIterator(); It; ++It)
    {
        FGateSwing& Swing = It.Value();
        USceneComponent* Leaf = Swing.Leaf.Get();
        Swing.Age += DeltaSeconds;
        const float T = FMath::Clamp(Swing.Age / GateSwingSeconds, 0.0f, 1.0f);
        if (Leaf) Leaf->SetRelativeRotation(FRotator(0, FMath::Lerp(Swing.FromYaw, Swing.ToYaw, FMath::InterpEaseInOut(0.0f, 1.0f, T, 2.0f)), 0));
        if (!Leaf || T >= 1.0f) It.RemoveCurrent();
    }
}
