#include "HomesteadDerelictFarm.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadEstate.h"

namespace
{
// Stable 0..1 noise from integers (the layout must not change between editor and game).
float FarmHash01(int32 A, int32 B, int32 Salt)
{
    uint32 H = static_cast<uint32>(A) * 73856093u ^ static_cast<uint32>(B) * 19349663u ^ static_cast<uint32>(Salt) * 83492791u;
    H ^= H >> 13;
    H *= 0x5bd1e995u;
    H ^= H >> 15;
    return static_cast<float>(H & 0xFFFFFFu) / 16777216.0f;
}

// Smooth 1D value noise, so sound and ruined stretches of fence come in runs.
float FarmSmooth01(float T, int32 Salt)
{
    const int32 I = FMath::FloorToInt(T);
    float F = T - I;
    F = F * F * (3.0f - 2.0f * F);
    return FMath::Lerp(FarmHash01(I, 0, Salt), FarmHash01(I + 1, 0, Salt), F);
}

// SM_FarmFencePost: rail mortises at these heights above its foot; it stands PostSink into the ground.
constexpr float FenceSlotZ[3] = {35.0f, 70.0f, 105.0f};
constexpr float FencePostSink = 8.0f;
constexpr float FenceBay = 275.0f;     // post centres; SM_FarmFenceRail is 290 cm with its tenons
constexpr float FenceRailLift = 4.5f;  // rail pivot (bottom) below its centreline
constexpr float FencePostHalf = 6.0f;  // half the post's thickness, for posts lying on the ground
// SM_FarmGateway: hanging post at its origin, shutting post GateSpan along +X.
constexpr float GateSpan = 310.0f;
// SM_FarmFurrows: a 600 x 400 cm patch of ridges running along X; tiled on the ridge pitch.
constexpr float RidgeStepU = 560.0f;
constexpr float RidgeStepV = 360.0f;
constexpr float RidgeFlatten = 0.55f;  // decades of grazing and frost have slumped the ridges

enum class EPost : uint8 { Upright, Leaning, Snapped, Gone, Gateway };

struct FFencePost
{
    FVector Foot;  // mesh pivot: the foot, PostSink below ground
    FVector Up;
    FVector Along; // horizontal fence direction here
    EPost State;
    float Quality;
    bool Holds() const { return State == EPost::Upright || State == EPost::Leaning || State == EPost::Gateway; }
    FVector Slot(int32 K) const { return Foot + Up * FenceSlotZ[K]; }
};
}

AHomesteadDerelictFarm::AHomesteadDerelictFarm()
{
    PrimaryActorTick.bCanEverTick = false;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("FarmRoot"));
    Root->SetMobility(EComponentMobility::Static);
    SetRootComponent(Root);
}

void AHomesteadDerelictFarm::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Rebuild();
}

void AHomesteadDerelictFarm::BeginPlay()
{
    Super::BeginPlay();
    // Cooked levels don't keep the transient parts, so lay them out again in play.
    Rebuild();
}

UStaticMesh* AHomesteadDerelictFarm::Mesh(const TCHAR* Folder, const TCHAR* Name)
{
    const FName Key(*FString::Printf(TEXT("%s/%s"), Folder, Name));
    if (const TObjectPtr<UStaticMesh>* Found = Meshes.Find(Key)) return Found->Get();
    UStaticMesh* Loaded = LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/%s.%s"), Folder, Name, Name), nullptr,
        LOAD_NoWarn | LOAD_Quiet);
    Meshes.Add(Key, Loaded);
    return Loaded;
}

UInstancedStaticMeshComponent* AHomesteadDerelictFarm::Batch(const TCHAR* Folder, const TCHAR* Name, bool bBlocks)
{
    const FName Key(*FString::Printf(TEXT("%s/%s/%d"), Folder, Name, bBlocks ? 1 : 0));
    if (const TObjectPtr<UInstancedStaticMeshComponent>* Found = Batches.Find(Key)) return Found->Get();
    UStaticMesh* Asset = Mesh(Folder, Name);
    if (!Asset) return nullptr;
    UInstancedStaticMeshComponent* Part = NewObject<UInstancedStaticMeshComponent>(this, NAME_None, RF_Transient);
    Part->SetupAttachment(GetRootComponent());
    Part->SetMobility(GetRootComponent()->Mobility);
    Part->SetStaticMesh(Asset);
    Part->SetCollisionProfileName(bBlocks ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    Part->SetCanEverAffectNavigation(bBlocks);
    Part->RegisterComponent();
    AddInstanceComponent(Part);
    Parts.Add(Part);
    Batches.Add(Key, Part);
    return Part;
}

bool AHomesteadDerelictFarm::Add(const TCHAR* Folder, const TCHAR* Name, const FTransform& World, bool bBlocks)
{
    UInstancedStaticMeshComponent* Part = Batch(Folder, Name, bBlocks);
    if (!Part) return false;
    Part->AddInstance(World, /*bWorldSpace=*/true);
    ++Placed;
    return true;
}

float AHomesteadDerelictFarm::Ground(double X, double Y) const
{
    const float Height = HomesteadEstateTerrain::Height(X, Y);
    return FMath::IsFinite(Height) ? Height : FallbackZ;
}

FVector AHomesteadDerelictFarm::GroundUp(double X, double Y, double Span) const
{
    const double H = FMath::Max(Span * 0.5, 50.0);
    const float DX = Ground(X + H, Y) - Ground(X - H, Y);
    const float DY = Ground(X, Y + H) - Ground(X, Y - H);
    return FVector(-DX, -DY, 2.0 * H).GetSafeNormal();
}

FTransform AHomesteadDerelictFarm::OnGround(double X, double Y, float Yaw, float Scale, double Span, float Lift) const
{
    const FVector Up = Span > 0.0 ? GroundUp(X, Y, Span) : FVector::UpVector;
    const FVector Heading = FRotator(0.0f, Yaw, 0.0f).Vector();
    const FQuat Rotation = FRotationMatrix::MakeFromZX(Up, Heading).ToQuat();
    return FTransform(Rotation, FVector(X, Y, Ground(X, Y)) + Up * Lift, FVector(Scale));
}

void AHomesteadDerelictFarm::Fence(const TArray<FVector2D>& Line, float Condition, int32 Salt, bool bSkipFirst, bool bSkipLast)
{
    const int32 Count = Line.Num();
    if (Count < 2) return;
    TArray<FFencePost> Posts;
    Posts.Reserve(Count);
    for (int32 I = 0; I < Count; ++I)
    {
        const FVector2D Prev = Line[FMath::Max(I - 1, 0)], Next = Line[FMath::Min(I + 1, Count - 1)];
        const FVector Along = FVector(Next - Prev, 0.0).GetSafeNormal();
        const FVector Across(-Along.Y, Along.X, 0.0);
        const float Quality = FMath::Clamp(0.55f * FarmSmooth01(I / 4.0f, Salt) + 0.45f * FarmHash01(I, Salt, 7)
            + (Condition - 0.5f) * 0.7f, 0.0f, 1.0f);
        EPost State = Quality > 0.62f ? EPost::Upright : Quality > 0.40f ? EPost::Leaning
            : Quality > 0.22f ? EPost::Snapped : EPost::Gone;
        if ((I == 0 && bSkipFirst) || (I == Count - 1 && bSkipLast)) State = EPost::Gateway;
        // Leaning posts lean together, out of or into the field, in runs.
        const float Side = FarmSmooth01(I / 6.0f, Salt + 3) > 0.5f ? 1.0f : -1.0f;
        const float Lean = State == EPost::Leaning ? FMath::Lerp(10.0f, 26.0f, FarmHash01(I, Salt, 9))
            : State == EPost::Gateway ? 0.0f : FMath::Lerp(0.0f, 5.0f, FarmHash01(I, Salt, 10));
        const FVector Up = (FVector::UpVector * FMath::Cos(FMath::DegreesToRadians(Lean))
            + Across * Side * FMath::Sin(FMath::DegreesToRadians(Lean))).GetSafeNormal();
        const FVector Foot(Line[I].X, Line[I].Y, Ground(Line[I].X, Line[I].Y));
        Posts.Add({Foot - Up * FencePostSink, Up, Along, State, Quality});
    }
    for (int32 I = 0; I < Count; ++I)
    {
        const FFencePost& Post = Posts[I];
        const FQuat Rotation = FRotationMatrix::MakeFromZX(Post.Up, Post.Along).ToQuat();
        if (Post.State == EPost::Upright || Post.State == EPost::Leaning)
            Add(TEXT("FarmFence"), TEXT("SM_FarmFencePost"), FTransform(Rotation, Post.Foot), true);
        else if (Post.State == EPost::Snapped)
            Add(TEXT("FarmFence"), TEXT("SM_FarmFencePostSnapped"), FTransform(Rotation, Post.Foot), true);
        else if (Post.State == EPost::Gone && FarmHash01(I, Salt, 12) < 0.5f)
        {
            // The whole post rotted off at the foot and lies in the grass beside its hole.
            const FVector Across(-Post.Along.Y, Post.Along.X, 0.0);
            const FVector At = Post.Foot + Across * FMath::Lerp(-70.0f, 70.0f, FarmHash01(I, Salt, 13));
            const float Yaw = Post.Along.Rotation().Yaw + FMath::Lerp(-35.0f, 35.0f, FarmHash01(I, Salt, 14));
            const FTransform Lying = FTransform(FRotator(-90.0f, 0.0f, 0.0f)) * OnGround(At.X, At.Y, Yaw, 1.0f, 150.0, FencePostHalf);
            Add(TEXT("FarmFence"), TEXT("SM_FarmFencePost"), Lying, false);
        }
    }
    // A rail between two points (its centreline), fitted to their distance.
    const auto Rail = [this](const FVector& A, const FVector& B, float Roll, bool bBlocks)
    {
        const FVector Dir = B - A;
        const float Length = Dir.Size();
        if (Length < 50.0f) return;
        FQuat Rotation = FRotationMatrix::MakeFromXZ(Dir, FVector::UpVector).ToQuat();
        Rotation = Rotation * FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Roll));
        const FVector Pivot = (A + B) * 0.5f - Rotation.GetUpVector() * FenceRailLift;
        const float Fit = FMath::Clamp(Length / FenceBay, 0.8f, 1.25f);
        Add(TEXT("FarmFence"), TEXT("SM_FarmFenceRail"), FTransform(Rotation, Pivot, FVector(Fit, 1.0f, 1.0f)), bBlocks);
    };
    // A rail lying in the grass along the bay.
    const auto Fallen = [this](const FFencePost& A, const FFencePost& B, int32 Key, int32 SaltIn)
    {
        const FVector Across(-A.Along.Y, A.Along.X, 0.0);
        const FVector Mid = FMath::Lerp(A.Foot, B.Foot, FMath::Lerp(0.35f, 0.65f, FarmHash01(Key, SaltIn, 31)))
            + Across * FMath::Lerp(-70.0f, 70.0f, FarmHash01(Key, SaltIn, 32));
        const float Yaw = (B.Foot - A.Foot).Rotation().Yaw + FMath::Lerp(-14.0f, 14.0f, FarmHash01(Key, SaltIn, 33));
        const bool bBroken = FarmHash01(Key, SaltIn, 34) < 0.35f;
        Add(TEXT("FarmFence"), bBroken ? TEXT("SM_FarmFenceRailBroken") : TEXT("SM_FarmFenceRail"),
            OnGround(Mid.X, Mid.Y, Yaw, 1.0f, 280.0), false);
    };
    for (int32 I = 0; I + 1 < Count; ++I)
    {
        const FFencePost& A = Posts[I];
        const FFencePost& B = Posts[I + 1];
        const float Quality = (A.Quality + B.Quality) * 0.5f;
        bool bDangled = false;
        for (int32 K = 0; K < 3; ++K)
        {
            const int32 Key = I * 3 + K;
            const float Roll = FMath::Lerp(-9.0f, 9.0f, FarmHash01(Key, Salt, 20));
            const float R = FarmHash01(Key, Salt, 21);
            if (A.Holds() && B.Holds())
            {
                if (R < 0.30f + 0.55f * Quality) Rail(A.Slot(K), B.Slot(K), Roll, true);
                else if (R < 0.85f) Fallen(A, B, Key, Salt);
            }
            else if (A.Holds() || B.Holds())
            {
                const FFencePost& Held = A.Holds() ? A : B;
                const FFencePost& Loose = A.Holds() ? B : A;
                if (!bDangled && R < 0.5f)
                {
                    // One end still in its mortise, the other dropped to the ground by the broken post.
                    bDangled = true;
                    const FVector From = Held.Slot(K);
                    const FVector2D Flat = FVector2D(Loose.Foot - From).GetSafeNormal();
                    float Drop = From.Z - Ground(Loose.Foot.X, Loose.Foot.Y) - 5.0f;
                    FVector End = From;
                    for (int32 Pass = 0; Pass < 2; ++Pass)
                    {
                        const float Reach = FMath::Sqrt(FMath::Max(FenceBay * FenceBay - Drop * Drop, 900.0f));
                        End = FVector(From.X + Flat.X * Reach, From.Y + Flat.Y * Reach, 0.0);
                        End.Z = Ground(End.X, End.Y) + 5.0f;
                        Drop = FMath::Clamp(From.Z - End.Z, 0.0f, FenceBay * 0.9f);
                    }
                    Rail(From, End, Roll, false);
                }
                else if (R < 0.8f) Fallen(A, B, Key, Salt);
            }
            else if (R < 0.45f) Fallen(A, B, Key, Salt);
        }
    }
}

int32 AHomesteadDerelictFarm::Rebuild()
{
    // PIE duplicates the editor's instance components without the transient Parts list, so
    // clear every batch the actor owns, not just the ones this copy made.
    TArray<UInstancedStaticMeshComponent*> Owned;
    GetComponents(Owned);
    for (UInstancedStaticMeshComponent* Part : Owned)
        if (Part) Part->DestroyComponent();
    for (UInstancedStaticMeshComponent* Part : Parts)
        if (IsValid(Part)) Part->DestroyComponent();
    Parts.Reset();
    Batches.Reset();
    Placed = 0;
    FallbackZ = GetActorLocation().Z;
    if (!HomesteadEstateTerrain::Activate()) return 0;
    const Homestead::DerelictFarmPlan Farm = Homestead::EstateDerelictFarm();
    if (Farm.valid)
    {
        const double U = Farm.lengthU, V = Farm.lengthV;
        const auto World = [&](double UU, double VV) { const Homestead::Point P = Farm.World(UU, VV); return FVector2D(P.x, P.y); };
        // One run of posts from the gate's west post round the field to its east post, each side
        // divided evenly into bays of about FenceBay.
        TArray<FVector2D> Line;
        const auto Side = [&](FVector2D From, FVector2D To, bool bFirst)
        {
            const int32 Bays = FMath::Max(1, FMath::RoundToInt(FVector2D::Distance(From, To) / FenceBay));
            for (int32 I = bFirst ? 0 : 1; I <= Bays; ++I) Line.Add(FMath::Lerp(From, To, static_cast<double>(I) / Bays));
        };
        const double GateWest = Farm.gateV - GateSpan * 0.5, GateEast = Farm.gateV + GateSpan * 0.5;
        Side(World(0, GateWest), World(0, 0), true);
        Side(World(0, 0), World(U, 0), false);
        Side(World(U, 0), World(U, V), false);
        Side(World(U, V), World(0, V), false);
        Side(World(0, V), World(0, GateEast), false);
        Fence(Line, 0.45f, 41, true, true);
        // The broken field gate hangs from its west post, swung into the field.
        const FVector2D Hanging = World(0, GateWest);
        const FVector2D Shutting = World(0, GateEast);
        const FVector2D GateDir = (Shutting - Hanging).GetSafeNormal();
        const float GateYaw = FMath::RadiansToDegrees(FMath::Atan2(GateDir.Y, GateDir.X));
        Add(TEXT("FarmFence"), TEXT("SM_FarmGateway"), OnGround(Hanging.X, Hanging.Y, GateYaw, 1.0f, 0.0, -FencePostSink), true);
        // The ghost of the crop rows: ridge patches tiled on the ridge pitch, rows running downhill.
        // They are grassed over: M_FarmFurrowsGrass samples the pasture like the landscape does, and
        // the ridges are flattened so they read as a shallow ripple in the sward, not rows of soil.
        if (UInstancedStaticMeshComponent* Ridges = Batch(TEXT("FarmField"), TEXT("SM_FarmFurrows"), false))
            if (UMaterialInterface* Sward = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/Props/FarmField/M_FarmFurrowsGrass.M_FarmFurrowsGrass"), nullptr,
                    LOAD_NoWarn | LOAD_Quiet))
                Ridges->SetMaterial(0, Sward);
        int32 Key = 0;
        for (const auto& Block : Farm.ridgeBlocks)
        {
            for (double UU = Block[0] + RidgeStepU * 0.5; UU <= Block[2] - RidgeStepU * 0.4; UU += RidgeStepU)
                for (double VV = Block[1] + RidgeStepV * 0.5; VV <= Block[3] - RidgeStepV * 0.4; VV += RidgeStepV)
                {
                    const FVector2D At = World(UU, VV);
                    FTransform Ridge = OnGround(At.X, At.Y, 0.0f, 1.0f, 500.0);
                    Ridge.SetScale3D(Ridge.GetScale3D() * FVector(1.0, 1.0, RidgeFlatten));
                    Add(TEXT("FarmField"), TEXT("SM_FarmFurrows"), Ridge, false);
                    ++Key;
                    // Dead bolted stalks stand here and there along the ridges.
                    if (FarmHash01(Key, 3, 51) < 0.45f)
                    {
                        const FVector2D Stalk = World(UU + FMath::Lerp(-250.0, 250.0, FarmHash01(Key, 3, 52)),
                            VV + 90.0 * FMath::RoundToInt(FMath::Lerp(-1.5f, 1.5f, FarmHash01(Key, 3, 53))));
                        Add(TEXT("FarmField"), TEXT("SM_FarmDeadStalks"),
                            OnGround(Stalk.X, Stalk.Y, 360.0f * FarmHash01(Key, 3, 54), FMath::Lerp(0.85f, 1.2f, FarmHash01(Key, 3, 55)), 120.0), false);
                    }
                }
        }
        // Rows of rotten bean poles along the east strip's ridges.
        if (Farm.ridgeBlocks.size() > 1)
        {
            const auto& Block = Farm.ridgeBlocks[1];
            for (int32 I = 0; I < 4; ++I)
            {
                const FVector2D At = World(FMath::Lerp(Block[0], Block[2], 0.18 + 0.2 * I), FMath::Lerp(Block[1], Block[3], 0.25 + 0.5 * FarmHash01(I, 5, 61)));
                Add(TEXT("FarmField"), TEXT("SM_FarmStakes"), OnGround(At.X, At.Y, FMath::Lerp(-8.0f, 8.0f, FarmHash01(I, 5, 62)) + (I % 2) * 180.0f, 1.0f, 200.0), false);
            }
        }
        // The plough, left tipped over in mid-furrow, its nose uphill.
        const FVector2D Plough = World(Farm.plough.x, Farm.plough.y);
        Add(TEXT("FarmPlough"), TEXT("SM_FarmPlough"), OnGround(Plough.X, Plough.Y, 8.0f, 1.0f, 250.0), true);
    }
    // Debris round the ruin and the drive's toppled fence.
    TMap<int32, TArray<FVector2D>> Runs;
#define DEBRIS(Folder, Name, X, Y, Yaw, Scale, Blocks) Add(Folder, Name, OnGround(X, Y, static_cast<float>(Yaw), static_cast<float>(Scale), 250.0), Blocks);
#define FENCE(Run, X, Y) Runs.FindOrAdd(Run).Add(FVector2D(X, Y));
#include "HomesteadEstateDebrisPlacements.inc"
#undef DEBRIS
#undef FENCE
    for (const TPair<int32, TArray<FVector2D>>& Run : Runs)
        Fence(Run.Value, 0.12f, 100 + Run.Key, false, false);
    return Placed;
}
