// The village's dressing (village-path-and-dressing): the well, benches and noticeboard, the general store's
// barrels and crates, flowers on the square, and four cottage gardens with their fences and hazel hedges.
// Scripts/Terrain/village_dress.py derives every piece from the town layout and writes
// HomesteadVillageDressing.inc (world cm); this lays them down on the baked terrain, so a rebake only
// re-runs that script. Pieces are instanced batches, one per mesh; the paved square and lanes are one mesh
// built in HomesteadWorldVillagePaving.cpp. Nothing here is game state.
#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadEstateTerrain.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"

namespace VillageStyle
{
enum class EMode : uint8
{
    Flat,    // lies on the slope, no collision (tilled beds and their plants)
    Upright, // stands on the ground, no collision (flowers, hazel)
    Solid,   // stands on the ground and blocks (well, benches, store clutter)
    Post,    // an upright fence post, sunk into the ground, blocks
    Rail,    // three rails from this post toward Yaw, one bay long (Scale is the bay length in cm), no collision
};

struct FPiece
{
    EMode Mode;
    const TCHAR* Folder;
    const TCHAR* Mesh;
    float X, Y, Yaw, Scale;
};

#define V(Mode, Folder, Mesh, X, Y, Yaw, Scale) {EMode::Mode, Folder, Mesh, X, Y, Yaw, Scale},
constexpr FPiece Pieces[] = {
#include "HomesteadVillageDressing.inc"
};
#undef V

// SM_FarmFencePost: rail mortises at these heights above its foot; it stands PostSink into the ground.
constexpr float FenceSlotZ[3] = {35.0f, 70.0f, 105.0f};
constexpr float FencePostSink = 8.0f;
constexpr float FenceBay = 275.0f;     // post centres; SM_FarmFenceRail is 290 cm with its tenons
constexpr float FenceRailLift = 4.5f;  // the rail's pivot sits this far below its centreline
constexpr float FlatSink = 1.2f;       // the tilled bed's ragged rim and the cobbles' feathered edge sink below the ground line
constexpr float SolidSink = 1.0f;
constexpr float SlopeProbeCm = 40.0f;
constexpr float SolidFootCm = 45.0f;   // a bench or barrel stands on the lowest ground within this reach

float TerrainZ(double X, double Y)
{
    const float Height = HomesteadEstateTerrain::Height(X, Y);
    return FMath::IsFinite(Height) ? Height : 0.0f;
}

FVector SlopeNormal(double X, double Y)
{
    return FVector(TerrainZ(X - SlopeProbeCm, Y) - TerrainZ(X + SlopeProbeCm, Y),
        TerrainZ(X, Y - SlopeProbeCm) - TerrainZ(X, Y + SlopeProbeCm), 2.0 * SlopeProbeCm).GetSafeNormal();
}

float LowestGround(double X, double Y)
{
    float Low = TerrainZ(X, Y);
    for (int32 Step = 0; Step < 4; ++Step)
    {
        const float Angle = Step * UE_HALF_PI;
        Low = FMath::Min(Low, TerrainZ(X + SolidFootCm * FMath::Cos(Angle), Y + SolidFootCm * FMath::Sin(Angle)));
    }
    return Low;
}
}

void AHomesteadWorld::BuildVillage()
{
    using namespace VillageStyle;
    if (bVillageBuilt) return;
    bVillageBuilt = true;
    if (!HomesteadEstateTerrain::Activate()) return;

    TMap<FName, TObjectPtr<UInstancedStaticMeshComponent>> Batches;
    TMap<FName, TObjectPtr<UStaticMesh>> Loaded;
    TSet<FName> Missing;
    int32 Placed = 0;
    const auto Add = [&](const FPiece& Piece, const FTransform& World, bool bBlocks)
    {
        const FName Key(*FString::Printf(TEXT("%s/%s/%d"), Piece.Folder, Piece.Mesh, bBlocks ? 1 : 0));
        TObjectPtr<UInstancedStaticMeshComponent>* Found = Batches.Find(Key);
        if (!Found)
        {
            const FName MeshKey(*FString::Printf(TEXT("%s/%s"), Piece.Folder, Piece.Mesh));
            if (Missing.Contains(MeshKey)) return;
            UStaticMesh* Mesh = nullptr;
            if (TObjectPtr<UStaticMesh>* Cached = Loaded.Find(MeshKey)) Mesh = Cached->Get();
            else
            {
                Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"),
                    Piece.Folder, Piece.Mesh, Piece.Mesh), nullptr, LOAD_NoWarn | LOAD_Quiet);
                Loaded.Add(MeshKey, Mesh);
            }
            if (!Mesh)
            {
                Missing.Add(MeshKey);
                return;
            }
            UInstancedStaticMeshComponent* Batch = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Movable);
            Batch->SetStaticMesh(Mesh);
            Batch->SetCollisionProfileName(bBlocks ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
            if (bBlocks) Batch->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            Batch->SetCanEverAffectNavigation(bBlocks);
            Batch->SetGenerateOverlapEvents(false);
            Batch->RegisterComponent();
            VillageVisual.Components.Add(Batch);
            Found = &Batches.Add(Key, Batch);
        }
        (*Found)->AddInstance(World, /*bWorldSpace=*/true);
        ++Placed;
    };

    for (const FPiece& Piece : Pieces)
    {
        const FRotator Yaw(0.0f, Piece.Yaw, 0.0f);
        switch (Piece.Mode)
        {
        case EMode::Flat:
        {
            const FRotator Lie = FRotationMatrix::MakeFromZX(SlopeNormal(Piece.X, Piece.Y), Yaw.Vector()).Rotator();
            Add(Piece, FTransform(Lie, FVector(Piece.X, Piece.Y, TerrainZ(Piece.X, Piece.Y) - FlatSink)), false);
            break;
        }
        case EMode::Upright:
            Add(Piece, FTransform(Yaw, FVector(Piece.X, Piece.Y, TerrainZ(Piece.X, Piece.Y)), FVector(Piece.Scale)), false);
            break;
        case EMode::Solid:
            Add(Piece, FTransform(Yaw, FVector(Piece.X, Piece.Y, LowestGround(Piece.X, Piece.Y) - SolidSink), FVector(Piece.Scale)), true);
            break;
        case EMode::Post:
            Add(Piece, FTransform(Yaw, FVector(Piece.X, Piece.Y, TerrainZ(Piece.X, Piece.Y) - FencePostSink)), true);
            break;
        case EMode::Rail:
        {
            const FVector Dir = Yaw.Vector();
            const FVector2D To(Piece.X + Dir.X * Piece.Scale, Piece.Y + Dir.Y * Piece.Scale);
            for (const float SlotZ : FenceSlotZ)
            {
                const FVector From(Piece.X, Piece.Y, TerrainZ(Piece.X, Piece.Y) - FencePostSink + SlotZ);
                const FVector End(To.X, To.Y, TerrainZ(To.X, To.Y) - FencePostSink + SlotZ);
                const FVector Span = End - From;
                const FVector Along = Span.GetSafeNormal();
                const FRotator Rotation = FRotationMatrix::MakeFromXZ(Along, FVector::UpVector).Rotator();
                const FVector Up = FRotationMatrix(Rotation).GetUnitAxis(EAxis::Z);
                const float Fit = FMath::Clamp(static_cast<float>(Span.Size()) / FenceBay, 0.8f, 1.25f);
                Add(Piece, FTransform(Rotation, (From + End) * 0.5f - Up * FenceRailLift, FVector(Fit, 1.0f, 1.0f)), false);
            }
            break;
        }
        }
    }
    for (const FName& Name : Missing)
        UE_LOG(LogHomesteadWorld, Log, TEXT("Village dressing: %s is not imported; its pieces are left out."), *Name.ToString());
    UE_LOG(LogHomesteadWorld, Log, TEXT("Village dressing: %d pieces in %d batches."), Placed, Batches.Num());
    BuildVillagePaving();
}
