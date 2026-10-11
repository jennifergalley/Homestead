#include "HomesteadRoadEndGate.h"

#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HomesteadController.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadPlayableBounds.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadRoadEnd, Log, All);

namespace RoadEndGateStyle
{
const TCHAR* PostMesh = TEXT("/Game/SurvivalGame/Environment/Props/FarmFence/SM_FarmFencePost.SM_FarmFencePost");
const TCHAR* RailMesh = TEXT("/Game/SurvivalGame/Environment/Props/FarmFence/SM_FarmFenceRail.SM_FarmFenceRail");
const TCHAR* StoneMesh = TEXT("/Game/SurvivalGame/Environment/Props/CoveKerb/SM_CoveKerb_Straight.SM_CoveKerb_Straight");

// SM_FarmFencePost / SM_FarmFenceRail (Scripts/Blender/Recipes/farm_fence.py, as HomesteadDerelictFarm.cpp lays
// them): rail mortises at these heights above the post's foot, which stands GatePostSink into the ground; a rail
// is 290 cm with its tenons for a 275 cm bay, its pivot GateRailLift below its centreline.
constexpr float GateRailSlotZ[3] = {35.0f, 70.0f, 105.0f};
constexpr float GatePostSink = 8.0f;
constexpr float GateBay = 275.0f;
constexpr float GateRailLift = 4.5f;
// The gate leaf fills the middle bay: five bars at farm_fence.py's rail_heights, stiles at each end and a brace
// rising from the hanging side, all stopping LeafInset short of the post centres (strap hinges and latch gap).
constexpr float LeafBarZ[5] = {22.0f, 42.0f, 62.0f, 84.0f, 108.0f};
constexpr float LeafInset = 16.0f;
constexpr float LeafFootZ = 8.0f;
constexpr float LeafHeadZ = 116.0f;
constexpr float BraceFootZ = 23.0f;
constexpr float BraceHeadZ = 104.0f;
// Four posts: the leaf in the middle and a fence bay either side, about 8.3 m across a ~5 m road and its verges.
constexpr float PostAcross[4] = {-1.5f * GateBay, -0.5f * GateBay, 0.5f * GateBay, 1.5f * GateBay};

// The milestone: SM_CoveKerb_Straight (cove_kerb.py: 100 cm long, 30 cm tall, 15 cm thick, pivot on the path-side
// top edge, +Y toward the drop) stood on end and widened to a dressed granite post, its path-side face the
// lettered face. It stands on the right-hand verge just before the gate, turned to face back down the road.
constexpr float KerbLength = 100.0f, KerbHeight = 30.0f, KerbThick = 15.0f;
constexpr float StoneWiden = 1.2f;   // 36 cm across the face
constexpr float StoneThicken = 1.6f; // 24 cm deep
constexpr float StoneBury = 25.0f;   // 75 cm stands proud, like the county's granite milestones
constexpr float MilestoneBack = 180.0f;   // cm back down the road from the gate line
constexpr float MilestoneAcross = 400.0f; // cm right of the centreline, off the road bed
constexpr float MilestoneFaceTurn = 210.0f; // face yaw from the road heading: back down the road, toward its middle
const TCHAR* MilestoneText = TEXT("MARLBURY\n9\nMILES");
constexpr float MilestoneLetterSize = 6.5f;
constexpr float LetterField = 30.0f;   // the lettering stays inside the face's width
constexpr float LetterCentreZ = 50.0f; // above the ground
constexpr float LetterProud = 0.6f;    // off the face, so it never z-fights the stone
const FColor LetterInk(46, 43, 38);

// The pawn-only stop across the whole run, and the touch zone a little in front of it on both sides.
constexpr float BlockerHalfThick = 15.0f;
constexpr float BlockerHalfHeight = 150.0f;
constexpr float BlockerBelowGround = 60.0f;
constexpr float TouchReach = 55.0f; // in front of the blocker's faces
}

AHomesteadRoadEndGate::AHomesteadRoadEndGate()
{
    using namespace RoadEndGateStyle;
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
    const auto Batch = [this](const TCHAR* Name)
    {
        UInstancedStaticMeshComponent* Part = CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);
        Part->SetupAttachment(Root);
        Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Part->SetCanEverAffectNavigation(false);
        return Part;
    };
    Posts = Batch(TEXT("GatePosts"));
    Rails = Batch(TEXT("GateRails"));
    MilestoneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MilestoneRoot"));
    MilestoneRoot->SetupAttachment(Root);
    Milestone = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Milestone"));
    Milestone->SetupAttachment(MilestoneRoot);
    Milestone->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Milestone->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    Milestone->SetCanEverAffectNavigation(false);
    MilestoneWords = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MilestoneWords"));
    MilestoneWords->SetupAttachment(MilestoneRoot);
    MilestoneWords->SetHorizontalAlignment(EHTA_Center);
    MilestoneWords->SetVerticalAlignment(EVRTA_TextCenter);
    MilestoneWords->SetTextRenderColor(LetterInk);
    MilestoneWords->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    MilestoneWords->SetCastShadow(false);

    const auto MakeBox = [this](const TCHAR* Name, ECollisionResponse PawnResponse)
    {
        UBoxComponent* Part = CreateDefaultSubobject<UBoxComponent>(Name);
        Part->SetupAttachment(Root);
        Part->SetCollisionObjectType(ECC_WorldStatic);
        Part->SetCollisionResponseToAllChannels(ECR_Ignore);
        Part->SetCollisionResponseToChannel(ECC_Pawn, PawnResponse);
        Part->SetCollisionEnabled(PawnResponse == ECR_Block ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::QueryOnly);
        Part->SetGenerateOverlapEvents(PawnResponse == ECR_Overlap);
        Part->CanCharacterStepUpOn = ECB_No;
        Part->SetCanEverAffectNavigation(false);
        Part->SetHiddenInGame(true);
        return Part;
    };
    Blocker = MakeBox(TEXT("PawnBlocker"), ECR_Block);
    Touch = MakeBox(TEXT("TouchZone"), ECR_Overlap);
    Touch->OnComponentBeginOverlap.AddDynamic(this, &AHomesteadRoadEndGate::OnTouched);
}

float AHomesteadRoadEndGate::GroundAt(const FVector2D& At) const
{
    const float Height = HomesteadEstateTerrain::Height(At.X, At.Y);
    return FMath::IsFinite(Height) ? Height : static_cast<float>(GetActorLocation().Z);
}

void AHomesteadRoadEndGate::Place(const FVector2D& Centre, float RoadYaw)
{
    using namespace RoadEndGateStyle;
    HomesteadEstateTerrain::Activate();
    const FVector2D Ahead = FVector2D(FRotator(0.0f, RoadYaw, 0.0f).Vector());
    const FVector2D Across = FVector2D(FRotator(0.0f, RoadYaw + 90.0f, 0.0f).Vector());
    const auto At = [&](float Back, float Right) { return Centre + Ahead * Back + Across * Right; };
    SetActorLocationAndRotation(FVector(Centre, GroundAt(Centre)), FRotator(0.0f, RoadYaw, 0.0f));

    UStaticMesh* PostAsset = LoadObject<UStaticMesh>(nullptr, PostMesh, nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* RailAsset = LoadObject<UStaticMesh>(nullptr, RailMesh, nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* StoneAsset = LoadObject<UStaticMesh>(nullptr, StoneMesh, nullptr, LOAD_NoWarn | LOAD_Quiet);
    Posts->ClearInstances();
    Rails->ClearInstances();
    Posts->SetStaticMesh(PostAsset);
    Rails->SetStaticMesh(RailAsset);
    if (!PostAsset || !RailAsset || !StoneAsset)
        UE_LOG(LogHomesteadRoadEnd, Warning, TEXT("Road-end gate: missing %s%s%s; standing what there is."),
            PostAsset ? TEXT("") : TEXT("SM_FarmFencePost "), RailAsset ? TEXT("") : TEXT("SM_FarmFenceRail "),
            StoneAsset ? TEXT("") : TEXT("SM_CoveKerb_Straight"));

    // Posts upright on the ground; every rail and bar runs between two points on its centreline (as the farm's).
    FVector Feet[UE_ARRAY_COUNT(PostAcross)];
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(PostAcross); ++Index)
    {
        const FVector2D Foot = At(0.0f, PostAcross[Index]);
        Feet[Index] = FVector(Foot, GroundAt(Foot) - GatePostSink);
        if (PostAsset) Posts->AddInstance(FTransform(FRotator(0.0f, RoadYaw + 90.0f, 0.0f), Feet[Index]), /*bWorldSpace=*/true);
    }
    const auto AddRail = [this](const FVector& A, const FVector& B, const FVector& Hint)
    {
        const FVector Dir = B - A;
        const float Length = Dir.Size();
        if (Length < 10.0f || !Rails->GetStaticMesh()) return;
        const FQuat Rotation = FRotationMatrix::MakeFromXZ(Dir, Hint).ToQuat();
        const FVector Pivot = (A + B) * 0.5f - Rotation.GetUpVector() * GateRailLift;
        Rails->AddInstance(FTransform(Rotation, Pivot, FVector(Length / GateBay, 1.0f, 1.0f)), /*bWorldSpace=*/true);
    };
    const FVector Up = FVector::UpVector;
    const FVector Forward(Ahead, 0.0);
    for (int32 Wing : {0, 2})
        for (const float Slot : GateRailSlotZ)
            AddRail(Feet[Wing] + Up * Slot, Feet[Wing + 1] + Up * Slot, Up);
    // The leaf: hung on the left post (as she faces the gate), latched to the right.
    const FVector Hanging = Feet[1], Shutting = Feet[2];
    const FVector Inward = (Shutting - Hanging).GetSafeNormal2D();
    const FVector LeafA = Hanging + Inward * LeafInset, LeafB = Shutting - Inward * LeafInset;
    for (const float Bar : LeafBarZ) AddRail(LeafA + Up * Bar, LeafB + Up * Bar, Up);
    AddRail(LeafA + Up * LeafFootZ, LeafA + Up * LeafHeadZ, Forward);
    AddRail(LeafB + Up * LeafFootZ, LeafB + Up * LeafHeadZ, Forward);
    AddRail(LeafA + Up * BraceFootZ, LeafB + Up * BraceHeadZ, Up);

    // The milestone, lettered on its path-side face. The kerb is scaled in its own frame, stood on end (its
    // length up, its path-side face toward -Y), centred and bedded, then turned so that face looks along +X.
    const FVector2D StoneAt = At(-MilestoneBack, MilestoneAcross);
    MilestoneRoot->SetWorldLocationAndRotation(FVector(StoneAt, GroundAt(StoneAt)),
        FRotator(0.0f, RoadYaw + MilestoneFaceTurn, 0.0f));
    const float Wide = KerbHeight * StoneWiden, Deep = KerbThick * StoneThicken;
    const FTransform Stand = FTransform(FQuat::Identity, FVector::ZeroVector, FVector(1.0f, StoneThicken, StoneWiden))
        * FTransform(FRotator(90.0f, 0.0f, 0.0f))
        * FTransform(FVector(-Wide * 0.5f, -Deep * 0.5f, KerbLength * 0.5f - StoneBury))
        * FTransform(FRotator(0.0f, 90.0f, 0.0f));
    Milestone->SetStaticMesh(StoneAsset);
    Milestone->SetRelativeTransform(Stand);
    Milestone->SetVisibility(StoneAsset != nullptr);
    MilestoneWords->SetVisibility(StoneAsset != nullptr);
    MilestoneWords->SetRelativeLocationAndRotation(FVector(Deep * 0.5f + LetterProud, 0.0f, LetterCentreZ), FRotator::ZeroRotator);
    MilestoneWords->SetWorldSize(MilestoneLetterSize);
    MilestoneWords->SetText(FText::FromString(MilestoneText));
    const FVector Lettering = MilestoneWords->GetTextLocalSize();
    if (Lettering.Y > LetterField) MilestoneWords->SetWorldSize(MilestoneLetterSize * LetterField / Lettering.Y);

    const float HalfRun = FMath::Abs(PostAcross[0]) + BlockerHalfThick;
    Blocker->SetBoxExtent(FVector(BlockerHalfThick, HalfRun, BlockerHalfHeight), false);
    Blocker->SetRelativeLocation(FVector(0.0f, 0.0f, BlockerHalfHeight - BlockerBelowGround));
    Touch->SetBoxExtent(FVector(BlockerHalfThick + TouchReach, HalfRun + TouchReach, BlockerHalfHeight), false);
    Touch->SetRelativeLocation(FVector(0.0f, 0.0f, BlockerHalfHeight - BlockerBelowGround));
    UE_LOG(LogHomesteadRoadEnd, Log, TEXT("Road-end gate at (%.0f, %.0f), heading %.1f: %d posts, %d rails, milestone %s."),
        Centre.X, Centre.Y, RoadYaw, Posts->GetInstanceCount(), Rails->GetInstanceCount(), StoneAsset ? TEXT("up") : TEXT("missing"));
}

void AHomesteadRoadEndGate::OnTouched(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
    const APawn* Pawn = Cast<APawn>(Other);
    AHomesteadController* Controller = Pawn ? Cast<AHomesteadController>(Pawn->GetController()) : nullptr;
    if (!Controller || !Controller->IsLocalController() || !GetWorld()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Homestead::RoadEndRefusalDue(Now, LastRefusalSeconds)) return;
    LastRefusalSeconds = Now;
    ++Refusals;
    Controller->PostHudNotice(UTF8_TO_TCHAR(Homestead::RoadEndRefusalMessage), true);
}
