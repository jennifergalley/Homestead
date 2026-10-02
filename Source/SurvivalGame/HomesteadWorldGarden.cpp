#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "HomesteadWorldVisualHelpers.h"
#include "Simulation/HomesteadCrops.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

using HomesteadWorldLook::Leaf;
using HomesteadWorldLook::LightLeaf;
using HomesteadWorldLook::Soil;
using HomesteadWorldVisualHelpers::Stage;

namespace HomesteadCropProduce
{
// Where each crop's produce sits on each stage's plants (generated from the Blender reports).
struct FAnchor
{
    const TCHAR* Visual;
    int32 Stage; // 0 Young, 1 Growing, 2 Mature, 3 Ripe
    float X, Y, Z, Yaw, Scale;
};
const FAnchor Anchors[] = {
#include "HomesteadCropProduceAnchors.inc"
    {nullptr, -1, 0, 0, 0, 0, 0},
};
// How each crop's produce grows in, tuned by eye at the gameplay camera (about 7 m):
// Appear: growth at which it shows. RiseCm: how far below the soil roots start before they push up.
// MinScale / MinLength: size across and along its hanging axis when it first shows (pods lengthen
// faster than they fatten). ColourPower: how late it colours up (higher stays pale longer).
struct FLook
{
    const TCHAR* Visual;
    float Appear, RiseCm, MinScale, MinLength, ColourPower;
    float Boost = 1.0f; // extra size at ripe where the mesh alone reads small at 7 m
};
const FLook Looks[] = {
    {TEXT("CropTurnip"), 0.30f, 3.0f, 0.50f, 0.50f, 1.0f, 1.3f},
    {TEXT("CropCarrot"), 0.30f, 3.0f, 0.50f, 0.50f, 1.0f, 1.9f},
    // Three hills of pale tubers on dark soil vanish in rain at 7 m below about 2.2x.
    {TEXT("CropPotato"), 0.30f, 3.0f, 0.45f, 0.45f, 1.2f, 2.3f},
    {TEXT("CropCabbage"), 0.30f, 0.0f, 0.25f, 0.25f, 1.2f},
    // Picked plants restart at growth 1 - regrow/grow (beans 0.57, strawberries 0.63), so these
    // appear just below that: a picked plant is left with tiny green fruit that swells again.
    {TEXT("CropBroadBean"), 0.50f, 0.0f, 0.45f, 0.30f, 1.3f, 1.8f},
    {TEXT("CropStrawberry"), 0.55f, 0.0f, 0.35f, 0.35f, 1.8f, 2.0f},
};
}

void AHomesteadWorld::AddCropProduce(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot,
    Homestead::CropStage CropStage, const FTransform& PlantTransform)
{
    using namespace HomesteadCropProduce;
    if (CropStage < Homestead::CropStage::Young || CropStage > Homestead::CropStage::Ripe) return;
    const FString CropVisual = UTF8_TO_TCHAR(Homestead::GetCropInfo(Plot.kind).visual);
    const FLook* Look = nullptr;
    for (const FLook& Candidate : Looks)
        if (CropVisual == Candidate.Visual) Look = &Candidate;
    if (!Look || Plot.growth < Look->Appear) return;
    UStaticMesh* Produce = CropMesh(Plot.kind, TEXT("Produce"));
    if (!Produce) return;
    const float T = FMath::Clamp(static_cast<float>((Plot.growth - Look->Appear) / (1.0 - Look->Appear)), 0.0f, 1.0f);
    const float Ripeness = Plot.growth >= 1.0 ? 1.0f : FMath::Pow(T, Look->ColourPower);
    const int32 StageIndex = static_cast<int32>(CropStage) - static_cast<int32>(Homestead::CropStage::Young);
    // One instanced mesh per plot: its instances carry their ripeness for M_CropProduce's tint.
    auto* Fruit = NewObject<UInstancedStaticMeshComponent>(this);
    Fruit->SetupAttachment(GetRootComponent());
    Fruit->SetMobility(EComponentMobility::Movable);
    Fruit->SetStaticMesh(Produce);
    Fruit->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Fruit->SetCanEverAffectNavigation(false);
    Fruit->SetGenerateOverlapEvents(false);
    Fruit->SetRelativeTransform(PlantTransform);
    Fruit->SetNumCustomDataFloats(1);
    Fruit->RegisterComponent();
    for (const FAnchor& Anchor : Anchors)
    {
        if (!Anchor.Visual || Anchor.Stage != StageIndex || CropVisual != Anchor.Visual) continue;
        const float Across = FMath::Lerp(Look->MinScale, 1.0f, T) * Anchor.Scale * Look->Boost;
        const float Along = FMath::Lerp(Look->MinLength, 1.0f, T) * Anchor.Scale * Look->Boost;
        const FTransform At(FRotator(0.0f, Anchor.Yaw, 0.0f),
            FVector(Anchor.X, Anchor.Y, Anchor.Z - (1.0f - T) * Look->RiseCm), FVector(Across, Across, Along));
        const int32 Index = Fruit->AddInstance(At);
        Fruit->SetCustomDataValue(Index, 0, Ripeness);
    }
    Visual.Components.Add(Fruit);
}

UStaticMesh* AHomesteadWorld::CropMesh(Homestead::CropKind Kind, const TCHAR* StageName)
{
    const FString Visual = UTF8_TO_TCHAR(Homestead::GetCropInfo(Kind).visual);
    if (Visual.IsEmpty()) return nullptr;
    const FName Key(*FString::Printf(TEXT("%s_%s"), *Visual, StageName));
    if (const TObjectPtr<UStaticMesh>* Cached = CropMeshes.Find(Key)) return Cached->Get();
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *FString::Printf(
        TEXT("/Game/SurvivalGame/Environment/Props/%s/SM_%s_%s.SM_%s_%s"), *Visual, *Visual, StageName, *Visual, StageName),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    CropMeshes.Add(Key, Mesh);
    return Mesh;
}

void AHomesteadWorld::BuildPlot(FHomesteadWorldVisual& Visual, const Homestead::Plot& Plot)
{
    const Homestead::Point Center = Homestead::PlotCenter(Plot);
    const float Moisture = Stage(Plot.moisture, 5) / 5.0f;
    const FLinearColor WetSoil = FMath::Lerp(Soil, FLinearColor(0.075f, 0.044f, 0.025f), Moisture);
    // One hoed square of turned soil, a little inside the garden square so neighbours read apart,
    // with one plant at its middle.
    {
        {
            const float PX = Center.x;
            const float PY = Center.y;
            // One hoed square of loose loam raked into three ridges (the Blender tilled bed; its
            // ragged rim sinks below the ground line), laid on the local slope of the terrain.
            // Watering swaps in the darker wet-soil texture.
            if (!TilledBedMesh)
                TilledBedMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/Props/TilledBed/SM_TilledBed.SM_TilledBed"), nullptr, LOAD_NoWarn | LOAD_Quiet);
            if (!TilledBedWetMaterial)
                TilledBedWetMaterial = LoadObject<UMaterialInterface>(nullptr,
                    TEXT("/Game/SurvivalGame/Environment/Props/TilledBed/MI_TilledBed_Wet.MI_TilledBed_Wet"), nullptr, LOAD_NoWarn | LOAD_Quiet);
            UStaticMesh* Bed = TilledBedMesh;
            UMaterialInterface* WetBed = TilledBedWetMaterial;
            if (Bed)
            {
                constexpr float Probe = 40.0f;
                const FVector Normal = FVector(GroundHeight(PX - Probe, PY) - GroundHeight(PX + Probe, PY),
                    GroundHeight(PX, PY - Probe) - GroundHeight(PX, PY + Probe), 2 * Probe).GetSafeNormal();
                const float Yaw = (Plot.id % 2) ? 180.0f : 0.0f;
                const FRotator Lie = FRotationMatrix::MakeFromZX(Normal, FRotator(0, Yaw, 0).Vector()).Rotator();
                if (auto* Part = AddPart(Visual, Bed, AtGround(PX, PY, -1.2f), FVector(100, 100, 100), WetSoil, false, Lie))
                    Part->SetMaterial(0, !Homestead::NeedsWater(Plot) && WetBed ? WetBed : Bed->GetMaterial(0));
            }
            else
                AddPart(Visual, Cube, AtGround(PX, PY, 0.4f), FVector(Homestead::GardenCellSize - 8.0f, Homestead::GardenCellSize - 8.0f, 1.2f),
                    WetSoil * 0.85f, false, FRotator::ZeroRotator, 0.95f - Moisture * 0.35f);
            constexpr int X = 0, Y = 0;
            const Homestead::CropStage CropStage = Homestead::StageOf(Plot);
            // A withered crop shows the Harvest lane's dead plant (SM_<visual>_Withered); until that
            // lands, its mature plant squashed low in a flat dead brown stands in.
            bool bWitheredStandIn = false;
            UStaticMesh* Plant = CropStage == Homestead::CropStage::Withered ? CropMesh(Plot.kind, TEXT("Withered"))
                : CropStage >= Homestead::CropStage::Sprout
                ? CropMesh(Plot.kind, UTF8_TO_TCHAR(Homestead::StageName(CropStage))) : nullptr;
            if (!Plant && CropStage == Homestead::CropStage::Withered)
            {
                Plant = CropMesh(Plot.kind, TEXT("Mature"));
                bWitheredStandIn = Plant != nullptr;
            }
            if (Plant)
            {
                // The Blender plant for this stage, sized for the square and set on the bed's ridges.
                constexpr float Probe = 40.0f;
                const FVector Normal = FVector(GroundHeight(PX - Probe, PY) - GroundHeight(PX + Probe, PY),
                    GroundHeight(PX, PY - Probe) - GroundHeight(PX, PY + Probe), 2 * Probe).GetSafeNormal();
                const float Yaw = (Plot.id % 2) ? 180.0f : 0.0f;
                const FRotator Lie = FRotationMatrix::MakeFromZX(Normal, FRotator(0, Yaw, 0).Vector()).Rotator();
                const FLinearColor DeadBrown(0.16f, 0.10f, 0.05f);
                if (auto* Part = AddPart(Visual, Plant, AtGround(PX, PY, Bed ? -1.2f : 0.0f),
                    FVector(100, 100, bWitheredStandIn ? 45 : 100), bWitheredStandIn ? DeadBrown : Leaf, false, Lie))
                    if (!bWitheredStandIn) Part->SetMaterial(0, Plant->GetMaterial(0));
                // Ripeness shows in the produce itself (size and colour), with no effect on top.
                AddCropProduce(Visual, Plot, CropStage, FTransform(Lie, AtGround(PX, PY, Bed ? -1.2f : 0.0f)));
            }
            else if (Plot.planted && CropStage == Homestead::CropStage::Sown)
            {
                // Just sown: a small mound of soil over the seed, with her fingertip's press.
                if (!SoilMoundMesh)
                    SoilMoundMesh = LoadObject<UStaticMesh>(nullptr,
                        TEXT("/Game/SurvivalGame/Environment/Props/Seeds/SM_SoilMound.SM_SoilMound"), nullptr, LOAD_NoWarn | LOAD_Quiet);
                UStaticMesh* Mound = SoilMoundMesh;
                if (auto* Part = AddPart(Visual, Mound ? Mound : Sphere.Get(), AtGround(PX, PY + 12, Mound ? (Bed ? 0.6f : 0.0f) : 0.5f),
                    Mound ? FVector(120, 120, 130) : FVector(15, 15, 4), WetSoil * 0.8f))
                    if (Mound) Part->SetMaterial(0, Mound->GetMaterial(0));
            }
            else if (Plot.planted)
            {
                const float Growth = Stage(Plot.growth, 12) / 12.0f;
                const bool BerryCrop = Plot.kind == Homestead::CropKind::Berries;
                const float Height = 7 + Growth * (BerryCrop ? 65 : 48);
                AddPart(Visual, Cone, AtGround(PX, PY, Height * 0.5f + 3),
                    FVector(12 + Growth * 23, 12 + Growth * 23, Height), Leaf);
                AddPart(Visual, Sphere, AtGround(PX, PY, Height * 0.55f + 3),
                    FVector(20 + Growth * 35, 12 + Growth * 20, 7 + Growth * 8), LightLeaf,
                    false, FRotator(0, (X + Y) * 52, 0));
                if (Plot.growth >= 1.0)
                {
                    if (BerryCrop)
                    {
                        for (int Berry = 0; Berry < 3; ++Berry)
                            AddPart(Visual, Sphere, AtGround(PX + (Berry - 1) * 10, PY + 5, Height * 0.6f),
                                FVector(9, 9, 9), FLinearColor(0.42f, 0.035f, 0.09f));
                    }

                    else
                        AddPart(Visual, Sphere, AtGround(PX, PY, 7), FVector(24, 24, 15),
                            FLinearColor(0.66f, 0.43f, 0.21f));
                }
            }
        }
    }
    FRandomStream Random(Plot.id * 193 + 51);
    const int WeedCount = Stage(Plot.weeds, 8);
    // Weeds creeping into the bed: young nettles at the rim and between the ridges (the cones remain
    // only if the mesh isn't imported). Plain green, no white flower heads, so they never read as ripe produce.
    if (!WeedTuftMesh)
        WeedTuftMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/SurvivalGame/Environment/Props/Nettle/SM_NettlePatch.SM_NettlePatch"), nullptr, LOAD_NoWarn | LOAD_Quiet);
    UStaticMesh* WeedTuft = WeedTuftMesh;
    for (int I = 0; I < WeedCount; ++I)
    {
        const float X = Center.x + Random.FRandRange(-40, 40);
        const float Y = Center.y + Random.FRandRange(-40, 40);
        if (WeedTuft)
        {
            const float Scale = Random.FRandRange(0.28f, 0.42f);
            if (auto* Part = AddPart(Visual, WeedTuft, AtGround(X, Y, -1.0f), FVector(100 * Scale), Leaf, false,
                FRotator(0, Random.FRandRange(0, 360), 0)))
                Part->SetMaterial(0, WeedTuft->GetMaterial(0));
        }
        else
            AddPart(Visual, Cone, AtGround(X, Y, 12), FVector(14, 14, 24),
                FLinearColor(0.34f, 0.31f, 0.07f), false, FRotator(0, I * 47, 16));
    }
}
