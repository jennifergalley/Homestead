// The standing room's door (Jenny, 2026-09-29): an oak ledged-and-braced leaf in the heritage stone
// doorway, hung on a hinge pivot so it swings out west into the ruin's south range as she comes near and
// shuts behind her. The swing and its geometry are Homestead::Door (native-tested); this builds the leaf
// and turns it. Until Props' oak-plank mesh lands the leaf is assembled from the engine cube.
#include "HomesteadWorld.h"
#include "HomesteadWorldLook.h"
#include "Simulation/HomesteadDoor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace HomesteadDoorLook
{
// Weathered oak boards and near-black wrought iron.
const FLinearColor Oak(0.30f, 0.19f, 0.10f);
const FLinearColor OakDark(0.24f, 0.15f, 0.08f);
const FLinearColor Iron(0.035f, 0.033f, 0.032f);
constexpr int32 Boards = 5;
constexpr float BoardGapCm = 0.5f;
constexpr float FloorGapCm = 2.0f;          // under the leaf, so it clears the threshold
constexpr float HeadGapCm = 2.0f;           // under the lintel
constexpr float LedgeDepthCm = 2.5f;        // ledges and brace stand proud of the inner face
constexpr float LedgeHeightCm = 15.0f;
// She is only "at" the door within this height of its sill (a hearth-room floor, not the hall below).
constexpr float NearHeightCm = 250.0f;
}

void AHomesteadWorld::AddDoorLeaf(FHomesteadWorldVisual& Visual, int32 StructureId, const FVector& Base,
    const FRotator& Rotation, float HeightScale)
{
    using namespace HomesteadDoorLook;
    namespace Door = Homestead::Door;
    if (!Cube) return;
    // SM_StoneDoorway's opening is 220 cm clear, stretched with the wall to the roof deck.
    const float Height = 220.0f * HeightScale - FloorGapCm - HeadGapCm;
    const FVector HingeLocal(-Door::OpeningHalfWidthCm, Door::HingeYCm, static_cast<double>(FloorGapCm));

    USceneComponent* Hinge = NewObject<USceneComponent>(this);
    Hinge->SetupAttachment(GetRootComponent());
    Hinge->SetMobility(EComponentMobility::Movable);
    Hinge->SetRelativeLocationAndRotation(Base + Rotation.RotateVector(HingeLocal), FRotator(0.0f, Rotation.Yaw, 0.0f));
    Hinge->RegisterComponent();
    Visual.Components.Add(Hinge);

    // Hinge space: X from the hinge to the latch, +Y out of the room, Z up from the leaf's foot.
    auto Piece = [&](const FVector& Centre, const FVector& Size, const FLinearColor& Color, const FRotator& Turn,
        bool bBlocksSight)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(Hinge);
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Cube);
        Part->SetMaterial(0, Material(Color, 0.8f));
        Part->SetRelativeTransform(FTransform(Turn, Centre, Size / 100.0f));
        // Never solid to her or the camera, so it can't trap or shove her whatever the swing; the boards
        // stop sight lines, so a shut door keeps the hearth's crackle in the room.
        Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        if (bBlocksSight)
        {
            Part->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            Part->SetCollisionResponseToAllChannels(ECR_Ignore);
            Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        }
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        Part->RegisterComponent();
        Visual.Components.Add(Part);
    };
    const float Thickness = static_cast<float>(Door::LeafThicknessCm);
    const float LeafWidth = static_cast<float>(Door::LeafWidthCm);
    const float BoardWidth = LeafWidth / Boards;
    for (int32 Index = 0; Index < Boards; ++Index)
        Piece(FVector(BoardWidth * (Index + 0.5f), -Thickness * 0.5f, Height * 0.5f),
            FVector(BoardWidth - BoardGapCm, Thickness, Height), Index % 2 ? OakDark : Oak, FRotator::ZeroRotator, true);
    // Ledges and a brace on the room side, rising from the hinge's foot towards the latch's head.
    const float LedgeY = -Thickness - LedgeDepthCm * 0.5f;
    const float LedgeLength = LeafWidth - 12.0f;
    const float LowZ = Height * 0.16f, HighZ = Height * 0.84f;
    for (const float Z : {LowZ, HighZ})
        Piece(FVector(LeafWidth * 0.5f, LedgeY, Z), FVector(LedgeLength, LedgeDepthCm, LedgeHeightCm), Oak,
            FRotator::ZeroRotator, false);
    const float Rise = HighZ - LowZ - LedgeHeightCm, Run = LedgeLength - 16.0f;
    Piece(FVector(LeafWidth * 0.5f, LedgeY, (LowZ + HighZ) * 0.5f),
        FVector(FMath::Sqrt(Rise * Rise + Run * Run), LedgeDepthCm, LedgeHeightCm * 0.8f), OakDark,
        FRotator(FMath::RadiansToDegrees(FMath::Atan2(Rise, Run)), 0.0f, 0.0f), false);
    // Strap hinges and a ring pull on the outer face.
    for (const float Z : {LowZ, HighZ})
        Piece(FVector(42.0f, 0.6f, Z), FVector(84.0f, 1.2f, 4.5f), Iron, FRotator::ZeroRotator, false);
    Piece(FVector(LeafWidth - 16.0f, 1.8f, Height * 0.47f), FVector(2.4f, 2.4f, 11.0f), Iron,
        FRotator::ZeroRotator, false);

    FDoorLeaf& Leaf = DoorLeaves.FindOrAdd(StructureId);
    Leaf.Hinge = Hinge;
    Leaf.Opening = GetActorTransform().TransformPosition(Base + Rotation.RotateVector(FVector(0.0f, Door::OuterFaceCm, 0.0f)));
    Leaf.ClosedYaw = Rotation.Yaw;
    Hinge->SetRelativeRotation(FRotator(0.0f, Leaf.ClosedYaw + static_cast<float>(Door::Angle(Leaf.Openness)), 0.0f));
}

void AHomesteadWorld::UpdateDoors(float DeltaSeconds)
{
    namespace Door = Homestead::Door;
    if (DoorLeaves.IsEmpty()) return;
    const APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    for (auto It = DoorLeaves.CreateIterator(); It; ++It)
    {
        FDoorLeaf& Leaf = It.Value();
        USceneComponent* Hinge = Leaf.Hinge.Get();
        if (!Hinge)
        {
            It.RemoveCurrent();
            continue;
        }
        double Distance = TNumericLimits<double>::Max();
        if (Pawn)
        {
            const FVector Feet = Pawn->GetActorLocation();
            if (FMath::Abs(Feet.Z - Leaf.Opening.Z) < HomesteadDoorLook::NearHeightCm)
                Distance = FVector::Dist2D(Feet, Leaf.Opening);
        }
        Leaf.bWanted = Door::WantsOpen(Leaf.bWanted, Distance);
        const float Before = Leaf.Openness;
        Leaf.Openness = static_cast<float>(Door::Step(Leaf.Openness, Leaf.bWanted, DeltaSeconds));
        if (Leaf.Openness != Before)
            Hinge->SetRelativeRotation(FRotator(0.0f, Leaf.ClosedYaw + static_cast<float>(Door::Angle(Leaf.Openness)), 0.0f));
    }
}
