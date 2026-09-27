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
};
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
    if (Pieces.IsEmpty()) Rebuild();
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
    SetActorLocation(FVector(MinX, MinY, Ground));
    for (const FRuinPiece& Entry : ManorRuinPlan)
    {
        UStaticMesh* Asset = Mesh(Entry.Mesh);
        if (!Asset) continue;
        UStaticMeshComponent* Piece = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
        Piece->SetupAttachment(GetRootComponent());
        Piece->SetMobility(EComponentMobility::Static);
        Piece->SetStaticMesh(Asset);
        // V runs north (+X) and U east (+Y); a run's own X axis lies along V at yaw 0.
        Piece->SetRelativeLocation(FVector(Entry.V, Entry.U, 0.0f));
        Piece->SetRelativeRotation(FRotator(0.0f, Entry.Yaw, 0.0f));
        Piece->SetRelativeScale3D(FVector(Entry.Scale));
        Piece->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        Piece->SetCanEverAffectNavigation(true);
        Piece->RegisterComponent();
        Pieces.Add(Piece);
    }
    return Pieces.Num();
}
