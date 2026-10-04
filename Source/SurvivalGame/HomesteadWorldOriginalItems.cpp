#include "HomesteadWorld.h"
#include "HomesteadOriginalItemArt.h"
#include "Simulation/HomesteadEstatePublicRoad.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadOriginalDrops, Log, All);

void AHomesteadWorld::BuildOriginalItemDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop)
{
    const auto* Art = HomesteadOriginalItemArt::Find(Drop.item);
    if (!Art) return;
    const int32 ItemId = static_cast<int32>(Drop.item);
    UStaticMesh* OriginalMesh = nullptr;
    if (const auto* Cached = OriginalDropMeshes.Find(ItemId)) OriginalMesh = Cached->Get();
    else
    {
        const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s"),
            UTF8_TO_TCHAR(Art->folder), UTF8_TO_TCHAR(Art->serving));
        OriginalMesh = LoadObject<UStaticMesh>(nullptr, *Path);
        if (!OriginalMesh)
        {
            UE_LOG(LogHomesteadOriginalDrops, Error, TEXT("Original item drop mesh is missing: %s"), *Path);
            return;
        }
        OriginalDropMeshes.Add(ItemId, OriginalMesh);
    }
    FVector DropGround = AtGround(Drop.position.x, Drop.position.y, 0);
    if (bRoadBridgeBuilt && RoadBridgeVisual.Components.Num() > 0)
        DropGround.Z = Homestead::EstatePublicRoad().deck.RestZ(Drop.position, DropGround.Z);
    // Fish length is Y; the original pole is 195cm along Z. Both must lie down, not stand upright.
    const bool bLayOnSide = Art->fish || Drop.item == Homestead::Item::FishingPole;
    const FRotator Rotation(bLayOnSide ? 90.0 : 0.0, Drop.id * 37 % 360, 0);
    const FBox Bounds = OriginalMesh->GetBoundingBox().TransformBy(FTransform(Rotation));
    constexpr double GroundClearanceCm = 0.5;
    DropGround.Z += GroundClearanceCm - Bounds.Min.Z;
    auto* Part = NewObject<UStaticMeshComponent>(this, MakeUniqueObjectName(this, UStaticMeshComponent::StaticClass(), TEXT("OriginalItemDrop")));
    Part->SetupAttachment(GetRootComponent());
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetStaticMesh(OriginalMesh);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetWorldLocationAndRotation(DropGround, Rotation);
    Part->RegisterComponent();
    Visual.Components.Add(Part);
}
