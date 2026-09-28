#include "HomesteadManorRuin.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Simulation/HomesteadEstate.h"

namespace
{
// One piece of the ruin in the manor's own frame: U runs east along its 30 m length from the
// west end, V north across its 18 m depth from the south front (cm). Yaw 0 lays a run along V.
struct FRuinPiece
{
    const TCHAR* Mesh;
    float U;
    float V;
    float Yaw;
    float Scale = 1.0f;
};

// Walls stand 28 cm in from the footprint's edge (their 56 cm thickness flush with it). The
// standing room is the south-east corner, U 2400..3000 by V 0..600.
constexpr float RuinWallInset = 28.0f;
const FRuinPiece ManorRuinPlan[] = {
    // South front: a collapsed south-west corner, two tall window runs either side of the
    // fallen front door, and a lower run meeting the standing room.
    {TEXT("RuinWallLow"), 150, RuinWallInset, 90},
    {TEXT("RuinWallTall"), 600, RuinWallInset, 90},
    {TEXT("RuinWallTall"), 1500, RuinWallInset, 270},
    {TEXT("RuinWallMid"), 2100, RuinWallInset, 90},
    // West gable with its chimney stack still standing.
    {TEXT("RuinWallMid"), RuinWallInset, 300, 0},
    {TEXT("RuinWallTall"), RuinWallInset, 900, 180},
    {TEXT("RuinWallLow"), RuinWallInset, 1350, 0},
    {TEXT("RuinWallLow"), RuinWallInset, 1650, 180},
    {TEXT("RuinChimney"), 55, 900, 0},
    // The rear wall, fallen almost to its footings.
    {TEXT("RuinWallLow"), 150, 1800 - RuinWallInset, 270},
    {TEXT("RuinWallMid"), 600, 1800 - RuinWallInset, 270},
    {TEXT("RuinWallLow"), 1050, 1800 - RuinWallInset, 90},
    {TEXT("RuinWallLow"), 1650, 1800 - RuinWallInset, 270},
    {TEXT("RuinWallMid"), 2100, 1800 - RuinWallInset, 90},
    {TEXT("RuinWallLow"), 2550, 1800 - RuinWallInset, 270},
    {TEXT("RuinWallLow"), 2850, 1800 - RuinWallInset, 90},
    // East gable, north of the standing room.
    {TEXT("RuinWallMid"), 3000 - RuinWallInset, 900, 180},
    {TEXT("RuinWallLow"), 3000 - RuinWallInset, 1350, 0},
    {TEXT("RuinWallLow"), 3000 - RuinWallInset, 1650, 180},
    // The cross wall between the hall and the rooms to the west.
    {TEXT("RuinWallLow"), 1800, 750, 0},
    {TEXT("RuinWallLow"), 1800, 1350, 180},
    // Rubble: under the fallen door, along the collapsed runs and in the hall.
    {TEXT("GraniteRubble"), 1050, 120, 20, 1.1f},
    {TEXT("GraniteBlockTalus"), 980, 260, 140, 0.9f},
    {TEXT("GraniteRubble"), 170, 160, 75, 1.2f},
    {TEXT("GraniteBlockTalus"), 1350, 1650, 200, 1.0f},
    {TEXT("GraniteRubble"), 2500, 1650, 310, 0.9f},
    {TEXT("GraniteCobbles"), 1300, 900, 45, 1.3f},
    {TEXT("GraniteRubble"), 700, 1000, 160, 1.0f},
    {TEXT("GraniteSpalls"), 2150, 1100, 250, 1.2f},
    {TEXT("GraniteCobbles"), 400, 600, 300, 1.1f},
    {TEXT("GraniteSpalls"), 2700, 900, 10, 1.0f},
    // Detail: fallen roof timbers in the hall and the west rooms, clear of the paths from the
    // standing room's door to the front and rear gaps.
    {TEXT("RuinFallenTimbers"), 2450, 1300, 35},
    {TEXT("RuinFallenTimbers"), 900, 1350, 110},
    // Slate slid off the roofs, heaped against the walls (the prop's -X edge is its wall side).
    {TEXT("RuinSlateScatter"), 1500, 195, 0},
    {TEXT("RuinSlateScatter"), 2150, 1600, 180},
    {TEXT("RuinSlateScatter"), 200, 1200, 90},
    {TEXT("RuinSlateScatter"), 700, -145, 180},
};

// Ivy hanging from a wall run's broken head. X runs along the host run from its centre; Side +1
// is the run's +Y face; Z is the head's height there (measured off the ruin_wall_* meshes).
struct FRuinCling
{
    int32 Host;
    float X;
    float Side;
    float Z;
};
constexpr float RuinWallHalfThickness = 30.0f;
const FRuinCling ManorRuinIvy[] = {
    {1, -175, 1, 560},   // south front, west tall run: faces the sea
    {2, 190, -1, 388},   // south front, east tall run, over the fallen door
    {5, -175, 1, 560},   // west gable
    {10, -175, 1, 305},  // rear wall: faces the forecourt and the road
    {13, -175, -1, 305}, // rear wall, east
    {16, -175, 1, 305},  // east gable, inside the hall
};

bool RuinPieceBlocks(const TCHAR* Mesh)
{
    return FCString::Strcmp(Mesh, TEXT("RuinSlateScatter")) != 0 && FCString::Strcmp(Mesh, TEXT("RuinIvy")) != 0;
}
}

AHomesteadManorRuin::AHomesteadManorRuin()
{
    PrimaryActorTick.bCanEverTick = false;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("ManorRoot"));
    Root->SetMobility(EComponentMobility::Static);
    SetRootComponent(Root);
}

void AHomesteadManorRuin::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    Rebuild();
}

void AHomesteadManorRuin::BeginPlay()
{
    Super::BeginPlay();
    // Rebuild in play as well; the ivy traces need the streamed level's collision.
    Rebuild();
}

UStaticMesh* AHomesteadManorRuin::Mesh(const TCHAR* Name)
{
    const FName Key(Name);
    if (const TObjectPtr<UStaticMesh>* Found = Meshes.Find(Key)) return Found->Get();
    UStaticMesh* Loaded = LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/SM_%s.SM_%s"), Name, Name, Name),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (Loaded) Meshes.Add(Key, Loaded);
    return Loaded;
}

int32 AHomesteadManorRuin::Rebuild()
{
    for (UStaticMeshComponent* Piece : Pieces)
        if (Piece) Piece->DestroyComponent();
    Pieces.Reset();
    const Homestead::EstateLayout& Layout = Homestead::ProvisionalEstateLayout();
    const Homestead::LandmarkPolygon* Footprint = Layout.FindPolygon(Homestead::Anchor::ManorFootprint);
    const Homestead::Landmark* Room = Layout.FindLandmark(Homestead::Anchor::StandingRoomOrigin);
    if (!Footprint || Footprint->points.empty()) return 0;
    double MinX = TNumericLimits<double>::Max(), MinY = MinX;
    for (const auto& Point : Footprint->points)
    {
        MinX = FMath::Min(MinX, Point.x);
        MinY = FMath::Min(MinY, Point.y);
    }
    const double Ground = Room ? Room->z : GetActorLocation().Z;
    const FVector Target(MinX, MinY, Ground);
    if (!GetActorLocation().Equals(Target, 1.0))
    {
        // The static root may only move in the editor; a game world that finds the anchor moved
        // since the level was saved makes it movable rather than drawing the ruin off its footprint.
        UWorld* World = GetWorld();
        if (World && World->IsGameWorld()) GetRootComponent()->SetMobility(EComponentMobility::Movable);
        SetActorLocation(Target);
    }
    auto Place = [this](const TCHAR* MeshName, const FTransform& Relative) -> UStaticMeshComponent* {
        UStaticMesh* Asset = Mesh(MeshName);
        if (!Asset) return nullptr;
        UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
        Piece->SetupAttachment(GetRootComponent());
        Piece->SetMobility(GetRootComponent()->Mobility);
        Piece->SetStaticMesh(Asset);
        Piece->SetRelativeTransform(Relative);
        if (RuinPieceBlocks(MeshName))
        {
            Piece->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Piece->SetCanEverAffectNavigation(true);
        }
        else
        {
            Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Piece->SetCanEverAffectNavigation(false);
        }
        Piece->RegisterComponent();
        Pieces.Add(Piece);
        return Piece;
    };
    // V runs north (+X) and U east (+Y); a run's own X axis lies along V at yaw 0.
    auto PlanTransform = [](const FRuinPiece& Entry) {
        return FTransform(FRotator(0.0f, Entry.Yaw, 0.0f), FVector(Entry.V, Entry.U, 0.0f), FVector(Entry.Scale));
    };
    TArray<UStaticMeshComponent*> PlanPieces;
    for (const FRuinPiece& Entry : ManorRuinPlan) PlanPieces.Add(Place(Entry.Mesh, PlanTransform(Entry)));
    // The ivy's own +Y is its leafy face, so it turns to face whichever side of the run it hangs on.
    for (const FRuinCling& Cling : ManorRuinIvy)
    {
        const float HeadZ = RunHeadHeight(PlanPieces[Cling.Host], Cling.X, Cling.Side, Cling.Z);
        const FTransform OnRun(FRotator(0.0f, Cling.Side > 0 ? 0.0f : 180.0f, 0.0f),
            FVector(Cling.X, Cling.Side * RuinWallHalfThickness, HeadZ));
        Place(TEXT("RuinIvy"), OnRun * PlanTransform(ManorRuinPlan[Cling.Host]));
    }
    return Pieces.Num();
}

float AHomesteadManorRuin::RunHeadHeight(UStaticMeshComponent* Run, float X, float Side, float Fallback)
{
    if (!Run) return Fallback;
    // Sample the broken head just inside the face across the ivy's width. Seat the mat on the
    // lowest sample (a buried top is hidden; a floating one shows), ignoring drops into openings.
    const FTransform& World = Run->GetComponentTransform();
    const float FaceY = Side * (RuinWallHalfThickness - 12.0f);
    // UPrimitiveComponent::LineTraceComponent misses these Nanite runs, so trace the world while
    // ignoring every other ruin piece and keep only hits on this run.
    UWorld* Level = Run->GetWorld();
    if (!Level) return Fallback;
    FCollisionQueryParams Params(NAME_None, true);
    for (UStaticMeshComponent* Other : Pieces)
        if (Other && Other != Run) Params.AddIgnoredComponent(Other);
    TArray<float, TInlineAllocator<16>> Heights;
    for (float Offset = -110.0f; Offset <= 110.0f; Offset += 20.0f)
    {
        FHitResult Hit;
        const FVector Start = World.TransformPosition(FVector(X + Offset, FaceY, 1200.0f));
        const FVector End = World.TransformPosition(FVector(X + Offset, FaceY, -20.0f));
        if (Level->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) && Hit.GetComponent() == Run)
            Heights.Add(World.InverseTransformPosition(Hit.ImpactPoint).Z);
    }
    if (Heights.IsEmpty()) return Fallback;
    float Highest = Heights[0];
    for (const float Height : Heights) Highest = FMath::Max(Highest, Height);
    float Seat = Highest;
    for (const float Height : Heights)
        if (Height > Highest - 120.0f) Seat = FMath::Min(Seat, Height);
    return Seat;
}
