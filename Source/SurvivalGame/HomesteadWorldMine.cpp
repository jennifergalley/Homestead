#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "Simulation/HomesteadMine.h"

#include "Components/AudioComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Sound/SoundWave.h"

// The estate mine on the actor side: the mine's resource nodes and pile, the trapdoor lid, the shaft of
// daylight at the ladder foot, the drips along the galleries, and the "To the mine" arm on the cove
// fingerpost. The house and tunnel shell are the Mine Art Agent's level actors (tag HomesteadMine). What
// she can mine, and what it gives, is Simulation/HomesteadMine.cpp.
namespace HomesteadWorldMineLook
{
const TCHAR* const NodeMeshFolder = TEXT("/Game/SurvivalGame/Environment/Props/MineNodes/");
const TCHAR* const LidMeshPath = TEXT("/Game/SurvivalGame/Environment/Props/MineHouse/SM_MineTrapdoorLid.SM_MineTrapdoorLid");
const TCHAR* const DripsPath = TEXT("/Game/SurvivalGame/Audio/Ambience/SeaCaveDrips.SeaCaveDrips");

// The trapdoor lid swings about its hinge: shut is flat (pitch 0), open stands past upright.
constexpr float LidOpenPitchDegrees = 100.0f;
constexpr float LidSwingSeconds = 0.5f;
// The stand-in lid, when the authored one isn't imported: a 120 cm square slab with its top flush at the hinge.
constexpr float StandInLidThicknessCm = 6.0f;

// The shaft of daylight at the ladder foot: a cool spot looking straight down, its brightness set in UpdateLighting.
const FLinearColor ShaftColour(0.75f, 0.87f, 1.0f);
constexpr float ShaftAttenuationRadiusCm = 2600.0f;
constexpr float ShaftInnerConeDegrees = 6.0f;
constexpr float ShaftVolumetricScattering = 2.0f;

// Drips along the galleries: one at the ladder foot and one every few floor segments, heard only underground.
constexpr int32 DripEveryNthFloor = 3;
constexpr int32 MaxDrips = 6;
constexpr float DripHeightAboveFloorCm = 250.0f;
constexpr float DripFalloffCm = 1800.0f;
constexpr float DripGain = 0.55f;
constexpr float DripAttenuationShapeCm = 300.0f;
constexpr float SoundMixEpsilon = 0.01f;

// Stand-in node shapes (cm): 60 cm deep into the gallery, the back buried toward -X.
constexpr float NodeDepthCm = 60.0f;
constexpr float NodeWidthCm = 120.0f;
constexpr float NodeHeightCm = 130.0f;
// The fingerpost's "To the mine" arm. The post stands at the cove fingerpost beside the manor ruin; the
// arm is the cove arm's size (SM_Fingerpost_ToTheCove: 62 x 13 x 3 cm), one board's depth below it.
const FVector CoveFingerpostFoot(-26689.6, -65471.1, 8646.4);
constexpr float SignArmHeightCm = 143.0f;
constexpr float SignArmLengthCm = 62.0f;
constexpr float SignArmDepthCm = 3.0f;
constexpr float SignArmHeightSizeCm = 13.0f;
constexpr float SignTextSizeCm = 7.0f;
constexpr float SignTextProudCm = 0.6f;
const FColor SignPaint(238, 226, 196);
}

using HomesteadWorldLook::Stone;

namespace
{
struct FNodeLook
{
    const TCHAR* Mesh;
    const TCHAR* SpentMesh;
    FLinearColor Colour;
    FLinearColor SpentColour;
};

FNodeLook LookFor(Homestead::ResourceKind Kind)
{
    using Homestead::ResourceKind;
    switch (Kind)
    {
    case ResourceKind::MineCoalSeam:
        return {TEXT("SM_MineCoalSeam"), TEXT("SM_MineCoalSeamSpent"), FLinearColor(0.02f, 0.02f, 0.025f), FLinearColor(0.1f, 0.1f, 0.11f)};
    case ResourceKind::MineCopperVein:
        return {TEXT("SM_MineCopperVein"), TEXT("SM_MineCopperVeinSpent"), FLinearColor(0.5f, 0.22f, 0.08f), FLinearColor(0.2f, 0.19f, 0.17f)};
    case ResourceKind::MineGemPocket:
        return {TEXT("SM_MineGemPocket"), TEXT("SM_MineGemPocketSpent"), FLinearColor(0.42f, 0.2f, 0.62f), FLinearColor(0.2f, 0.19f, 0.2f)};
    default:
        return {TEXT("SM_MineRockFace"), TEXT("SM_MineRockFaceSpent"), Stone, FLinearColor(0.2f, 0.22f, 0.21f)};
    }
}

UStaticMesh* LoadNodeMesh(const TCHAR* Name)
{
    const FString Path = FString(HomesteadWorldMineLook::NodeMeshFolder) + Name + TEXT(".") + Name;
    return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}
}

void AHomesteadWorld::BuildMine()
{
    if (bMineBuilt) return;
    bMineBuilt = true;
    using namespace HomesteadWorldMineLook;
    const Homestead::Mine::Layout& Layout = Homestead::Mine::GetLayout();

    // The shaft of daylight at the ladder foot.
    MineShaft = NewObject<USpotLightComponent>(this, TEXT("MineDaylightShaft"));
    MineShaft->SetupAttachment(GetRootComponent());
    MineShaft->SetMobility(EComponentMobility::Movable);
    MineShaft->SetWorldLocationAndRotation(FVector(Layout.daylight.x, Layout.daylight.y, Layout.daylight.z), FRotator(-90.0f, 0.0f, 0.0f));
    MineShaft->SetLightColor(ShaftColour);
    MineShaft->SetIntensity(0.0f);
    MineShaft->SetInnerConeAngle(ShaftInnerConeDegrees);
    MineShaft->SetOuterConeAngle(MineShaftOuterConeDegrees);
    MineShaft->SetAttenuationRadius(ShaftAttenuationRadiusCm);
    MineShaft->SetVolumetricScatteringIntensity(ShaftVolumetricScattering);
    MineShaft->SetCastShadows(true);
    MineShaft->RegisterComponent();

    // The trapdoor lid: pivot on the hinge, the lid lying along +X; the authored mesh's own pivot is the hinge.
    MineTrapdoorPivot = NewObject<USceneComponent>(this, TEXT("MineTrapdoorPivot"));
    MineTrapdoorPivot->SetupAttachment(GetRootComponent());
    MineTrapdoorPivot->SetMobility(EComponentMobility::Movable);
    MineTrapdoorPivot->SetWorldLocationAndRotation(
        FVector(Layout.trapdoor.hingeX, Layout.trapdoor.hingeY, Layout.trapdoor.hingeZ),
        FRotator(0.0f, static_cast<float>(Layout.trapdoor.yaw), 0.0f));
    MineTrapdoorPivot->RegisterComponent();
    UStaticMesh* LidMesh = LoadObject<UStaticMesh>(nullptr, LidMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet);
    MineTrapdoorLid = NewObject<UStaticMeshComponent>(this, TEXT("MineTrapdoorLid"));
    MineTrapdoorLid->SetupAttachment(MineTrapdoorPivot);
    MineTrapdoorLid->SetMobility(EComponentMobility::Movable);
    if (LidMesh)
    {
        MineTrapdoorLid->SetStaticMesh(LidMesh);
        MineTrapdoorLid->SetRelativeLocation(FVector::ZeroVector);
    }
    else
    {
        MineTrapdoorLid->SetStaticMesh(Cube);
        MineTrapdoorLid->SetMaterial(0, Material(HomesteadWorldLook::Wood));
        MineTrapdoorLid->SetRelativeLocationAndRotation(
            FVector(Layout.trapdoor.lidLength * 0.5, 0.0, -StandInLidThicknessCm * 0.5f), FRotator::ZeroRotator);
        MineTrapdoorLid->SetRelativeScale3D(FVector(static_cast<float>(Layout.trapdoor.lidLength) / 100.0f,
            static_cast<float>(Layout.trapdoor.lidWidth) / 100.0f, StandInLidThicknessCm / 100.0f));
    }
    MineTrapdoorLid->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    MineTrapdoorLid->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
    MineTrapdoorLid->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
    MineTrapdoorLid->SetGenerateOverlapEvents(false);
    MineTrapdoorLid->SetCanEverAffectNavigation(false);
    MineTrapdoorLid->RegisterComponent();

    // Drips: the ladder foot, then every few floors along the galleries.
    if (USoundWave* Drips = LoadObject<USoundWave>(nullptr, DripsPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
    {
        TArray<FVector> Spots;
        Spots.Add(FVector(Layout.ladderFoot.x, Layout.ladderFoot.y, Layout.ladderFoot.z + DripHeightAboveFloorCm));
        for (int32 Index = DripEveryNthFloor - 1; Index < static_cast<int32>(Layout.floors.size()) && Spots.Num() < MaxDrips; Index += DripEveryNthFloor)
        {
            const Homestead::Mine::FloorSegment& Floor = Layout.floors[Index];
            Spots.Add(FVector((Floor.a.x + Floor.b.x) * 0.5, (Floor.a.y + Floor.b.y) * 0.5, Floor.z + DripHeightAboveFloorCm));
        }
        for (const FVector& At : Spots)
        {
            UAudioComponent* Audio = NewObject<UAudioComponent>(this);
            Audio->SetupAttachment(GetRootComponent());
            Audio->SetWorldLocation(At);
            Audio->SetSound(Drips);
            Audio->bAutoActivate = false;
            Audio->bOverrideAttenuation = true;
            Audio->AttenuationOverrides.bAttenuate = true;
            Audio->AttenuationOverrides.bSpatialize = true;
            Audio->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
            Audio->AttenuationOverrides.dBAttenuationAtMax = -60.0f;
            Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(DripAttenuationShapeCm);
            Audio->AttenuationOverrides.FalloffDistance = DripFalloffCm;
            Audio->SetVolumeMultiplier(0.0f);
            Audio->RegisterComponent();
            Audio->Play(FMath::FRandRange(0.0f, 20.0f));
            MineSounds.Add(Audio);
            MineSoundGains.Add(DripGain);
        }
    }

    // The "To the mine" arm on the cove fingerpost, pointing from the post down the path to the mine.
    {
        const Homestead::Mine::Spot& Door = Layout.door;
        const FVector2D Toward(Door.x - CoveFingerpostFoot.X, Door.y - CoveFingerpostFoot.Y);
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Toward.Y, Toward.X));
        const FRotator ArmRotation(0.0f, Yaw, 0.0f);
        const FVector Pivot = CoveFingerpostFoot + FVector(0, 0, SignArmHeightCm);
        UStaticMeshComponent* Arm = NewObject<UStaticMeshComponent>(this, TEXT("MineSignArm"), RF_Transient);
        Arm->SetupAttachment(GetRootComponent());
        Arm->SetMobility(EComponentMobility::Movable);
        Arm->SetStaticMesh(Cube);
        Arm->SetMaterial(0, Material(HomesteadWorldLook::Wood, 0.9f));
        Arm->SetWorldTransform(FTransform(ArmRotation, Pivot + ArmRotation.RotateVector(FVector(SignArmLengthCm * 0.5f, 0, 0)),
            FVector(SignArmLengthCm, SignArmDepthCm, SignArmHeightSizeCm) / 100.0f));
        Arm->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Arm->SetGenerateOverlapEvents(false);
        Arm->SetCanEverAffectNavigation(false);
        Arm->RegisterComponent();
        MineVisual.Components.Add(Arm);
        for (const float Side : {1.0f, -1.0f})
        {
            const FRotator FaceRotation(0.0f, Yaw + 90.0f * Side, 0.0f);
            UTextRenderComponent* Words = NewObject<UTextRenderComponent>(this, NAME_None, RF_Transient);
            Words->SetupAttachment(GetRootComponent());
            Words->SetMobility(EComponentMobility::Movable);
            Words->SetHorizontalAlignment(EHTA_Center);
            Words->SetVerticalAlignment(EVRTA_TextCenter);
            Words->SetWorldSize(SignTextSizeCm);
            Words->SetTextRenderColor(SignPaint);
            Words->SetText(FText::FromString(TEXT("To the mine")));
            Words->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Words->SetCastShadow(false);
            Words->SetWorldLocationAndRotation(
                Pivot + ArmRotation.RotateVector(FVector(SignArmLengthCm * 0.5f, 0, 0)) + FaceRotation.Vector() * (SignArmDepthCm * 0.5f + SignTextProudCm),
                FaceRotation);
            Words->RegisterComponent();
            MineVisual.Components.Add(Words);
        }
    }

    UE_LOG(LogHomesteadWorld, Log, TEXT("Mine: %d nodes, %d floors, %d drips, trapdoor lid %s."),
        static_cast<int32>(Layout.nodes.size()), static_cast<int32>(Layout.floors.size()), MineSounds.Num(),
        LidMesh ? TEXT("authored") : TEXT("stand-in"));
}

void AHomesteadWorld::TickMine(float DeltaSeconds)
{
    if (!bMineBuilt) return;
    using namespace HomesteadWorldMineLook;
    if (!FMath::IsNearlyEqual(MineLidOpen, MineLidTarget) && MineTrapdoorPivot)
    {
        MineLidOpen = FMath::FInterpConstantTo(MineLidOpen, MineLidTarget, DeltaSeconds, 1.0f / LidSwingSeconds);
        const float Eased = FMath::InterpEaseInOut(0.0f, 1.0f, MineLidOpen, 2.0f);
        MineTrapdoorPivot->SetRelativeRotation(FRotator(LidOpenPitchDegrees * Eased, static_cast<float>(Homestead::Mine::GetLayout().trapdoor.yaw), 0.0f));
    }
    const float Mix = GetMineMix();
    if (FMath::Abs(Mix - MineSoundMix) > SoundMixEpsilon || (Mix == 0.0f && MineSoundMix != 0.0f))
    {
        MineSoundMix = Mix;
        for (int32 Index = 0; Index < MineSounds.Num(); ++Index)
        {
            if (MineSounds[Index]) MineSounds[Index]->SetVolumeMultiplier(MineSoundGains[Index] * Mix);
        }
    }
}

void AHomesteadWorld::BuildMineResource(FHomesteadWorldVisual& Visual, const Homestead::ResourceNode& Node, bool bProduceOnly)
{
    using namespace HomesteadWorldMineLook;
    using Homestead::ResourceKind;
    const Homestead::Mine::Layout& Layout = Homestead::Mine::GetLayout();
    const Homestead::Mine::Node* Socket = Homestead::Mine::FindNode(Node.id);
    const bool bPile = Node.kind == ResourceKind::MinePile;
    if (!Socket && !bPile) return;

    const FVector At = bPile ? FVector(Layout.pile.x, Layout.pile.y, Layout.pile.z) : FVector(Socket->x, Socket->y, Socket->z);
    const FRotator Rotation(0.0f, static_cast<float>(bPile ? Layout.pile.yaw : Socket->yaw), 0.0f);

    if (bPile)
    {
        // The one-time stone pile: a few rough blocks in a heap, built only while it's still there.
        if (bProduceOnly) return;
        static const FVector Offsets[] = {FVector(0, 0, 14), FVector(30, 22, 11), FVector(-26, 18, 9), FVector(8, -30, 12), FVector(-6, 4, 36)};
        static const FVector Sizes[] = {FVector(70, 60, 28), FVector(46, 40, 22), FVector(44, 38, 18), FVector(52, 40, 24), FVector(40, 36, 22)};
        static const float Yaws[] = {8.0f, 31.0f, -24.0f, 52.0f, 17.0f};
        for (int32 Index = 0; Index < UE_ARRAY_COUNT(Offsets); ++Index)
        {
            AddPart(Visual, Cube, At + Offsets[Index], Sizes[Index], Stone, false, FRotator(0.0f, Yaws[Index], 0.0f));
        }
        if (!bStagingResourceBuild)
        {
            UBoxComponent* Blocker = NewObject<UBoxComponent>(this);
            Blocker->SetupAttachment(GetRootComponent());
            Blocker->SetMobility(EComponentMobility::Movable);
            Blocker->SetBoxExtent(FVector(45.0f, 40.0f, 25.0f), false);
            Blocker->SetRelativeTransform(FTransform(FRotator::ZeroRotator, At + FVector(0, 0, 25.0f)));
            Blocker->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Blocker->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            Blocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
            Blocker->SetGenerateOverlapEvents(false);
            Blocker->SetCanEverAffectNavigation(false);
            Blocker->SetHiddenInGame(true);
            Blocker->ComponentTags.Add(TEXT("ClearoutBlocker"));
            Blocker->RegisterComponent();
            Visual.Components.Add(Blocker);
        }
        return;
    }

    const FNodeLook Look = LookFor(Node.kind);
    UStaticMesh* Authored = LoadNodeMesh(bProduceOnly ? Look.Mesh : Look.SpentMesh);
    if (Authored)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(GetRootComponent());
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Authored);
        Part->SetRelativeTransform(FTransform(Rotation, At));
        // The produce visual is the full node over the spent rubble: only the base carries the blocker.
        Part->SetCollisionProfileName(bProduceOnly ? UCollisionProfile::NoCollision_ProfileName : UCollisionProfile::BlockAll_ProfileName);
        Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        Part->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        Part->SetGenerateOverlapEvents(false);
        Part->SetCanEverAffectNavigation(false);
        if (bStagingResourceBuild)
        {
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Part->SetVisibility(false);
            Part->SetHiddenInGame(true);
        }
        Part->RegisterComponent();
        Visual.Components.Add(Part);
        return;
    }

    // Stand-ins until the authored node meshes are imported: a block per kind, a low mound when spent.
    const auto Local = [&](const FVector& Offset) { return At + Rotation.RotateVector(Offset); };
    if (bProduceOnly)
    {
        AddPart(Visual, Cube, Local(FVector(NodeDepthCm * 0.5f, 0, NodeHeightCm * 0.5f)),
            FVector(NodeDepthCm, NodeWidthCm, NodeHeightCm), Look.Colour, false, Rotation);
        if (Node.kind == ResourceKind::MineGemPocket || Node.kind == ResourceKind::MineCopperVein)
        {
            AddPart(Visual, Sphere, Local(FVector(NodeDepthCm, 0, NodeHeightCm * 0.6f)), FVector(26, 34, 26), Look.Colour, false, Rotation, 0.3f);
        }
        return;
    }
    AddPart(Visual, Sphere, Local(FVector(NodeDepthCm * 0.35f, 0, 12)), FVector(NodeDepthCm, NodeWidthCm * 0.8f, 34), Look.SpentColour, false, Rotation);
    if (!bStagingResourceBuild)
    {
        UBoxComponent* Blocker = NewObject<UBoxComponent>(this);
        Blocker->SetupAttachment(GetRootComponent());
        Blocker->SetMobility(EComponentMobility::Movable);
        Blocker->SetBoxExtent(FVector(NodeDepthCm * 0.5f, NodeWidthCm * 0.45f, NodeHeightCm * 0.5f), false);
        Blocker->SetRelativeTransform(FTransform(Rotation, Local(FVector(NodeDepthCm * 0.5f, 0, NodeHeightCm * 0.5f))));
        Blocker->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
        Blocker->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        Blocker->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        Blocker->SetGenerateOverlapEvents(false);
        Blocker->SetCanEverAffectNavigation(false);
        Blocker->SetHiddenInGame(true);
        Blocker->ComponentTags.Add(TEXT("ClearoutBlocker"));
        Blocker->RegisterComponent();
        Visual.Components.Add(Blocker);
    }
}
