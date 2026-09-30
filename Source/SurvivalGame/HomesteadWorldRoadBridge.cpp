// The road bridge over the river (Jenny, 2026-09-29): a period oak beam bridge on granite abutments where
// the public road crosses the river, built from Homestead::EstatePublicRoad().deck (Scripts/Terrain/
// public_road.py measures it; road_grade.py holds the road level over it). Oak stringers carry cross
// planks between kerbs, with posted two-rail railings. An invisible slab at the planks' top is what she
// walks on, and invisible rail walls stop her and her things (not the camera) at the edges. Assembled
// from the engine cube until an authored mesh replaces it; nothing about it is game state.
#include "HomesteadWorld.h"
#include "HomesteadEstateTerrain.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "Simulation/HomesteadEstatePublicRoad.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"

namespace HomesteadRoadBridgeLook
{
const FLinearColor Oak(0.29f, 0.19f, 0.105f);
const FLinearColor OakWeathered(0.36f, 0.30f, 0.23f);   // silvered plank tops and rails
const FLinearColor OakDark(0.20f, 0.13f, 0.07f);
constexpr float PlankWidthCm = 24.0f;
constexpr float PlankGapCm = 1.5f;
constexpr float PlankThicknessCm = 7.0f;
constexpr float PlankOverhangCm = 18.0f;   // past the kerbs each side
constexpr float StringerWidthCm = 26.0f;
constexpr float StringerDepthCm = 40.0f;   // oak over an 8 m clear span
constexpr int32 Stringers = 4;
constexpr float KerbWidthCm = 20.0f;
constexpr float KerbHeightCm = 16.0f;
constexpr float PostSizeCm = 15.0f;
constexpr float PostSpacingCm = 185.0f;
constexpr float RailHeightCm = 110.0f;     // top of the top rail over the deck
constexpr float RailSizeCm = 11.0f;
constexpr float MidRailHeightCm = 58.0f;
constexpr float AbutmentLengthCm = 150.0f; // along the road, under each end
constexpr float AbutmentFootingCm = 60.0f; // below the lowest ground it stands on
constexpr float AbutmentWingCm = 45.0f;    // wider than the deck each side
constexpr float WalkSlabCm = 20.0f;        // the invisible slab she walks on
constexpr float RailWallThicknessCm = 20.0f;
constexpr float RailWallHeightCm = 140.0f;
}

void AHomesteadWorld::BuildRoadBridge()
{
    using namespace HomesteadRoadBridgeLook;
    if (bRoadBridgeBuilt) return;
    bRoadBridgeBuilt = true;
    const Homestead::PublicRoadBridge& Deck = Homestead::EstatePublicRoad().deck;
    if (!Deck.valid || !Cube || !HomesteadEstateTerrain::IsActive())
    {
        UE_LOG(LogHomesteadWorld, Warning, TEXT("The road bridge has no deck data or mesh; the road fords the river."));
        return;
    }
    const FRotator Along(0.0f, static_cast<float>(Deck.yaw), 0.0f);
    const FVector Centre(Deck.centre.x, Deck.centre.y, Deck.deckZ);
    const float HalfLength = static_cast<float>(Deck.halfLength);
    const float HalfWidth = static_cast<float>(Deck.halfWidth);
    // Local frame: X along the road toward town, Y to its right, Z up from the walking surface.
    auto World = [&](const FVector& Local) { return Centre + Along.RotateVector(Local); };
    enum class ECollide : uint8 { None, Walk, PawnWall };
    auto Piece = [&](const FVector& LocalCentre, const FVector& Size, const FLinearColor& Color, ECollide Collide,
        const FRotator& Turn = FRotator::ZeroRotator)
    {
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(GetRootComponent());
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Cube);
        Part->SetRelativeTransform(FTransform((Along.Quaternion() * Turn.Quaternion()).Rotator(), World(LocalCentre), Size / 100.0f));
        Part->SetGenerateOverlapEvents(false);
        if (Collide == ECollide::None)
        {
            Part->SetMaterial(0, Material(Color, 0.85f));
            Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Part->SetCanEverAffectNavigation(false);
        }
        else
        {
            // Invisible collision: the walking slab blocks everything; the rail walls stop pawns and
            // dropped things but let the camera through, so it doesn't snap in against the rails.
            Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            if (Collide == ECollide::PawnWall) Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            Part->SetVisibility(false);
            Part->SetHiddenInGame(true);
            Part->SetCastShadow(false);
            Part->SetCanEverAffectNavigation(true);
        }
        Part->RegisterComponent();
        RoadBridgeVisual.Components.Add(Part);
    };

    const float PlankLength = 2.0f * (HalfWidth + KerbWidthCm + PlankOverhangCm);
    // Cross planks, alternately a little darker, their tops the walking surface.
    int32 Index = 0;
    for (float X = -HalfLength + PlankWidthCm * 0.5f; X <= HalfLength - PlankWidthCm * 0.5f; X += PlankWidthCm + PlankGapCm, ++Index)
        Piece(FVector(X, 0.0f, -PlankThicknessCm * 0.5f), FVector(PlankWidthCm, PlankLength, PlankThicknessCm),
            Index % 3 == 1 ? Oak : OakWeathered, ECollide::None);
    // Stringers under them, bearing on the abutments.
    const float StringerZ = -PlankThicknessCm - StringerDepthCm * 0.5f;
    for (int32 Beam = 0; Beam < Stringers; ++Beam)
    {
        const float Y = FMath::Lerp(-HalfWidth, HalfWidth, (Beam + 0.5f) / Stringers);
        Piece(FVector(0.0f, Y, StringerZ), FVector(2.0f * HalfLength, StringerWidthCm, StringerDepthCm), OakDark, ECollide::None);
    }
    // Kerbs (wheel guards) along both edges, posts through them, and two rails.
    for (const float Side : {-1.0f, 1.0f})
    {
        const float KerbY = Side * (HalfWidth + KerbWidthCm * 0.5f);
        Piece(FVector(0.0f, KerbY, KerbHeightCm * 0.5f), FVector(2.0f * HalfLength, KerbWidthCm, KerbHeightCm), Oak, ECollide::None);
        const int32 Bays = FMath::Max(2, FMath::RoundToInt(2.0f * HalfLength / PostSpacingCm));
        const float PostY = Side * (HalfWidth + KerbWidthCm + PostSizeCm * 0.5f);
        const float PostBottom = StringerZ;   // bolted to the outer stringer's side
        for (int32 Post = 0; Post <= Bays; ++Post)
        {
            const float X = FMath::Lerp(-HalfLength + PostSizeCm, HalfLength - PostSizeCm, static_cast<float>(Post) / Bays);
            Piece(FVector(X, PostY, (RailHeightCm + PostBottom) * 0.5f), FVector(PostSizeCm, PostSizeCm, RailHeightCm - PostBottom),
                Oak, ECollide::None);
        }
        for (const float Height : {RailHeightCm - RailSizeCm * 0.5f, MidRailHeightCm})
            Piece(FVector(0.0f, PostY - Side * (PostSizeCm + RailSizeCm) * 0.5f, Height),
                FVector(2.0f * HalfLength - PostSizeCm, RailSizeCm, RailSizeCm), OakWeathered, ECollide::None);
        Piece(FVector(0.0f, PostY, RailWallHeightCm * 0.5f), FVector(2.0f * HalfLength, RailWallThicknessCm, RailWallHeightCm),
            FLinearColor::Black, ECollide::PawnWall);
    }
    // The slab she walks on, a touch longer than the deck so the road runs onto it without a lip.
    Piece(FVector(0.0f, 0.0f, -WalkSlabCm * 0.5f), FVector(2.0f * HalfLength + 40.0f, 2.0f * HalfWidth, WalkSlabCm),
        FLinearColor::Black, ECollide::Walk);
    // Granite abutments under each end, from below the lowest ground on their footprint up to the stringers.
    const float AbutmentTop = StringerZ - StringerDepthCm * 0.5f;
    const float AbutmentHalfWidth = HalfWidth + KerbWidthCm + AbutmentWingCm;
    for (const float Side : {-1.0f, 1.0f})
    {
        const float X = Side * (HalfLength - AbutmentLengthCm * 0.5f + 10.0f);
        float Lowest = TNumericLimits<float>::Max();
        for (const float DX : {-0.5f, 0.0f, 0.5f})
            for (const float DY : {-1.0f, 0.0f, 1.0f})
            {
                const FVector Foot = World(FVector(X + DX * AbutmentLengthCm, DY * AbutmentHalfWidth, 0.0f));
                const float FootZ = HomesteadEstateTerrain::Height(Foot.X, Foot.Y);
                if (FMath::IsFinite(FootZ)) Lowest = FMath::Min(Lowest, FootZ);
            }
        if (Lowest == TNumericLimits<float>::Max()) Lowest = static_cast<float>(Deck.bedZ);
        const float Bottom = Lowest - AbutmentFootingCm - static_cast<float>(Deck.deckZ);
        Piece(FVector(X, 0.0f, (AbutmentTop + Bottom) * 0.5f), FVector(AbutmentLengthCm, 2.0f * AbutmentHalfWidth, AbutmentTop - Bottom),
            HomesteadWorldLook::Stone, ECollide::None);
        Piece(FVector(X, 0.0f, (AbutmentTop + Bottom) * 0.5f), FVector(AbutmentLengthCm, 2.0f * AbutmentHalfWidth, AbutmentTop - Bottom),
            FLinearColor::Black, ECollide::PawnWall);
    }
    UE_LOG(LogHomesteadWorld, Log, TEXT("Road bridge: %.1f m long, %.1f m between the railings, deck %.2f m (%.2f m over the water)."),
        HalfLength / 50.0f, HalfWidth / 50.0f, Deck.deckZ / 100.0, (Deck.deckZ - Deck.waterZ) / 100.0);
}
