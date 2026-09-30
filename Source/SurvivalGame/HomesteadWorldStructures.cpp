#include "HomesteadWorld.h"
#include "HomesteadWeather.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"
#include "HomesteadLampLook.h"
#include "Simulation/HomesteadRoomAudio.h"
#include "Simulation/HomesteadRuinDebris.h"

#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundWave.h"

using HomesteadWorldLook::Bark;
using HomesteadWorldLook::DeconstructColor;
using HomesteadWorldLook::PreviewBlockedColor;
using HomesteadWorldLook::PreviewColor;
using HomesteadWorldLook::Stone;
using HomesteadWorldLook::Wood;

UStaticMesh* AHomesteadWorld::ManorMesh(const TCHAR* Name)
{
    const FName Key(Name);
    if (const TObjectPtr<UStaticMesh>* Found = ManorMeshes.Find(Key)) return Found->Get();
    // Not cached while missing, so a mesh imported mid-session shows up on the next rebuild.
    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/%s/SM_%s.SM_%s"), Name, Name, Name),
        nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (Mesh) ManorMeshes.Add(Key, Mesh);
    return Mesh;
}

void AHomesteadWorld::UpdateHearthFlicker(float DeltaSeconds)
{
    if (HearthLights.IsEmpty()) return;
    HearthFlickerTime += DeltaSeconds;
    HearthLights.RemoveAll([](const TWeakObjectPtr<UPointLightComponent>& Light) { return !Light.IsValid(); });
    for (int32 Index = 0; Index < HearthLights.Num(); ++Index)
    {
        const float T = HearthFlickerTime + Index * 7.3f;
        // Layered slow breathing and quick licks, like a settled wood fire.
        const float Flicker = 0.82f + 0.1f * FMath::PerlinNoise1D(T * 1.3f) + 0.08f * FMath::PerlinNoise1D(T * 7.1f)
            + 0.05f * FMath::PerlinNoise1D(T * 17.0f);
        HearthLights[Index]->SetIntensity(5200.0f * Flicker);
    }
}

float AHomesteadWorld::GetIndoorMix() const
{
    return Weather ? Weather->GetIndoorMix() : 0.0f;
}

void AHomesteadWorld::UpdateHearthSound(float DeltaSeconds)
{
    HearthSounds.RemoveAll([](const FHearthSound& Sound) { return !Sound.Audio.IsValid(); });
    if (HearthSounds.IsEmpty()) return;
    APlayerController* Controller = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!Controller) return;
    FVector Listener, Front, Right;
    Controller->GetAudioListenerPosition(Listener, Front, Right);
    for (FHearthSound& Sound : HearthSounds)
    {
        bool bHeard = false;
        // Past the attenuation radius there's nothing to hear, so skip the traces.
        if (FVector::DistSquared(Listener, Sound.Mouth) < FMath::Square(900.0f))
        {
            FCollisionQueryParams Params(SCENE_QUERY_STAT(HearthSound), false);
            if (APawn* Pawn = Controller->GetPawn()) Params.AddIgnoredActor(Pawn);
            for (const TWeakObjectPtr<UPrimitiveComponent>& Part : Sound.Ignored)
                if (Part.IsValid()) Params.AddIgnoredComponent(Part.Get());
            // Two sight lines (the fire's mouth and the breast above it), so a chair or her own arm
            // between her and the grate doesn't cut the sound out.
            bHeard = !GetWorld()->LineTraceTestByChannel(Listener, Sound.Mouth, ECC_Visibility, Params)
                || !GetWorld()->LineTraceTestByChannel(Listener, Sound.Chimney, ECC_Visibility, Params);
        }
        const float Target = bHeard ? 1.0f : 0.0f;
        Sound.Gate = Sound.Gate < 0.0f ? Target : FMath::FInterpConstantTo(Sound.Gate, Target, DeltaSeconds, 2.5f);
        // A roofed hearth (the standing room's) stays in its room: heard from outside through the open
        // door it keeps only RoomAudio::HearthOutdoorLeak.
        const bool bRoofed = Weather && Weather->IsUnderShelter(Sound.Mouth);
        const float Gain = static_cast<float>(Homestead::RoomAudio::HearthGainFor(Sound.Gate, GetIndoorMix(), bRoofed));
        Sound.Audio->SetVolumeMultiplier(FMath::Max(Gain, 0.001f));
    }
}

void AHomesteadWorld::BuildStructure(FHomesteadWorldVisual& Visual, const Homestead::Structure& Structure,
    const Homestead::Building& Frame, bool bOnFoundation, bool bPreview, bool bValid, bool bDeconstruct)
{
    const Homestead::Point Center = Homestead::BuildingCellCenter(Frame, Structure.cellX, Structure.cellY);
    FVector Base(Center.x, Center.y, StructureBase(Center, Frame.yaw));
    // UE positive yaw rotates +X toward +Y; negative yaw maps the north edge to east.
    FRotator Rotation(0, Homestead::PieceYaw(Frame, Structure.rotation), 0);
    const bool bFurniture = Homestead::IsFurniture(Structure.kind);
    if (bFurniture && !bOnFoundation)
    {
        // Off a foundation the piece stands centred where it was placed, on the ground under its own
        // footprint, leaning with the slope so a bedroll or chest doesn't hover on the downhill side.
        const FVector2D Local = Structure.kind == Homestead::Piece::Bed ? FVector2D(95, -10)
            : Structure.kind == Homestead::Piece::Chest ? FVector2D(-100, -100)
            : Structure.kind == Homestead::Piece::Hearth ? FVector2D(0, 98) : FVector2D(-100, 95);
        const FVector2D Half = Structure.kind == Homestead::Piece::Bed ? FVector2D(35, 78)
            : Structure.kind == Homestead::Piece::Chest ? FVector2D(35, 28)
            : Structure.kind == Homestead::Piece::Hearth ? FVector2D(85, 32) : FVector2D(34, 34);
        const FVector Pivot = FVector(Center.x, Center.y, 0);
        const FVector AxisX = Rotation.RotateVector(FVector::ForwardVector);
        const FVector AxisY = Rotation.RotateVector(FVector::RightVector);
        auto GroundAt = [&](float DX, float DY)
        {
            const FVector P = Pivot + AxisX * DX + AxisY * DY;
            return GroundHeight(P.X, P.Y);
        };
        const float SlopeX = (GroundAt(Half.X, 0) - GroundAt(-Half.X, 0)) / (2 * Half.X);
        const float SlopeY = (GroundAt(0, Half.Y) - GroundAt(0, -Half.Y)) / (2 * Half.Y);
        const float PivotZ = GroundHeight(Pivot.X, Pivot.Y) - 1.5f;
        Rotation = FRotationMatrix::MakeFromXY(AxisX + FVector::UpVector * SlopeX,
            AxisY + FVector::UpVector * SlopeY).Rotator();
        // Keep the footprint centre on the ground once the tilt swings the cell-centre origin.
        Base = FVector(Pivot.X, Pivot.Y, PivotZ) - Rotation.RotateVector(FVector(Local, 0));
    }
    auto Part = [&](UStaticMesh* Mesh, FVector Offset, FVector Size, FLinearColor Color,
        bool bSolid = false, FRotator LocalRotation = FRotator::ZeroRotator, float Glow = 0.0f)
    {
        const FRotator Combined = (Rotation.Quaternion() * LocalRotation.Quaternion()).Rotator();
        const FVector Shown = bDeconstruct ? Size * 1.02f + FVector(4.0f) : Size;
        const FLinearColor PreviewTint = bDeconstruct ? (bValid ? DeconstructColor : PreviewBlockedColor)
            : (bValid ? PreviewColor : PreviewBlockedColor);
        return AddPart(Visual, Mesh, Base + Rotation.RotateVector(Offset), Shown,
            bPreview ? PreviewTint : Color, bSolid && !bPreview, Combined, 0.85f, bPreview ? 0.0f : Glow);
    };
    // An imported kit mesh at the piece's pivot, keeping its own baked materials.
    auto KitPart = [&](const TCHAR* Name, float HeightScale = 1.0f) -> UStaticMeshComponent*
    {
        UStaticMesh* Mesh = ManorMesh(Name);
        if (!Mesh) return nullptr;
        UStaticMeshComponent* Placed = Part(Mesh, FVector::ZeroVector, FVector(100.0f, 100.0f, 100.0f * HeightScale),
            FLinearColor::White, true);
        if (Placed && !bPreview)
            for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot)
                Placed->SetMaterial(Slot, Mesh->GetMaterial(Slot));
        return Placed;
    };
    if (Structure.skin == Homestead::StructureSkin::Stone)
    {
        const TCHAR* Kit = Structure.kind == Homestead::Piece::Foundation ? TEXT("StoneFoundation")
            : Structure.kind == Homestead::Piece::Wall ? TEXT("StoneWall")
            : Structure.kind == Homestead::Piece::Doorway ? TEXT("StoneDoorway")
            : Structure.kind == Homestead::Piece::Roof ? TEXT("StoneRoof") : nullptr;
        // The roof tiles only at yaw 0 about the building's own grid.
        if (Kit && Structure.kind == Homestead::Piece::Roof) Rotation = FRotator(0, Frame.yaw, 0);
        // The roof's joists run one way only, so the walls rise past their coping (258 cm) to the deck
        // underside (279 cm): the joist ends bed into the masonry instead of leaving daylight between them.
        const bool bWallPiece = Structure.kind == Homestead::Piece::Wall || Structure.kind == Homestead::Piece::Doorway;
        const float HeightScale = bWallPiece ? 1.085f : 1.0f;
        if (Kit && KitPart(Kit, HeightScale)) return;
    }
    if (Structure.kind == Homestead::Piece::Hearth)
    {
        if (!KitPart(TEXT("StoneHearth")))
        {
            // Blockout until the hearth mesh is imported: jambs, lintel and breast.
            Part(Cube, FVector(0, 85, 1.5f), FVector(192, 90, 3), Stone, true);
            for (int Side : {-1, 1})
                Part(Cube, FVector(Side * 70.5f, 98, 55), FVector(29, 64, 104), Stone, true);
            Part(Cube, FVector(0, 97, 124), FVector(180, 67, 32), Stone, true);
            Part(Cube, FVector(0, 98, 199), FVector(170, 64, 118), Stone, true);
            Part(Cube, FVector(0, 126, 55), FVector(112, 8, 104), FLinearColor(0.03f, 0.028f, 0.026f));
        }
        if (bPreview) return;
        const FVector Fire(0, 100, 0);
        // Low flames licking up from the logs, and embers glowing under them.
        Part(Cube, Fire + FVector(0, 4, 9), FVector(70, 26, 4), FLinearColor(0.9f, 0.18f, 0.02f), false,
            FRotator::ZeroRotator, 2.2f);
        const FVector Flames[] = {{-18, 0, 30}, {6, -3, 34}, {24, 3, 28}, {-4, 6, 40}};
        const FVector FlameSizes[] = {{16, 12, 30}, {20, 14, 40}, {14, 11, 26}, {9, 8, 26}};
        for (int I = 0; I < 4; ++I)
        {
            Part(Cone, Fire + Flames[I], FlameSizes[I], FLinearColor(0.95f, 0.26f, 0.03f), false, FRotator::ZeroRotator, 3.0f);
            Part(Cone, Fire + Flames[I] - FVector(0, 0, 4), FlameSizes[I] * 0.55f, FLinearColor(1.0f, 0.66f, 0.16f),
                false, FRotator::ZeroRotator, 4.5f);
        }
        UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
        Light->SetupAttachment(GetRootComponent());
        Light->SetMobility(EComponentMobility::Movable);
        // Just in front of the opening, so the room (not the firebox) takes the light.
        Light->SetRelativeLocation(Base + Rotation.RotateVector(FVector(0, 55, 55)));
        Light->SetLightColor(FLinearColor(1.0f, 0.45f, 0.16f));
        Light->SetIntensity(5200.0f);
        Light->SetAttenuationRadius(900.0f);
        Light->SetSourceRadius(30.0f);
        Light->SetCastShadows(true);
        Light->RegisterComponent();
        Visual.Components.Add(Light);
        HearthLights.Add(Light);
        if (!HearthCrackle)
            HearthCrackle = LoadObject<USoundWave>(nullptr,
                TEXT("/Game/SurvivalGame/Audio/Ambience/HearthCrackle.HearthCrackle"), nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (HearthCrackle)
        {
            UAudioComponent* Crackle = NewObject<UAudioComponent>(this);
            Crackle->SetupAttachment(GetRootComponent());
            Crackle->SetRelativeLocation(Base + Rotation.RotateVector(Fire + FVector(0, 0, 30)));
            Crackle->SetSound(HearthCrackle);
            Crackle->bAutoActivate = false;
            // A small fire: full within a couple of metres, gone about 7 m away, so it fills its own room only.
            Crackle->bOverrideAttenuation = true;
            Crackle->AttenuationOverrides.bAttenuate = true;
            Crackle->AttenuationOverrides.bSpatialize = true;
            Crackle->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
            Crackle->AttenuationOverrides.dBAttenuationAtMax = -60.0f;
            Crackle->AttenuationOverrides.AttenuationShapeExtents = FVector(150.0f);
            Crackle->AttenuationOverrides.FalloffDistance = 550.0f;
            Crackle->SetVolumeMultiplier(static_cast<float>(Homestead::RoomAudio::HearthGain));
            Crackle->RegisterComponent();
            Crackle->Play(FMath::FRandRange(0.0f, 20.0f));
            Visual.Components.Add(Crackle);
            FHearthSound& Sound = HearthSounds.AddDefaulted_GetRef();
            Sound.Audio = Crackle;
            Sound.Mouth = Light->GetComponentLocation();
            Sound.Chimney = Base + Rotation.RotateVector(FVector(0, 45, 130));
            for (const TObjectPtr<USceneComponent>& Component : Visual.Components)
                if (UPrimitiveComponent* Primitive = Cast<UPrimitiveComponent>(Component.Get()))
                    Sound.Ignored.Add(Primitive);
        }
        return;
    }
    switch (Structure.kind)
    {
    case Homestead::Piece::Foundation:
        Part(Cube, FVector(0, 0, -10), FVector(298, 298, 20), Wood, true);
        for (int I = 0; I < 6; ++I)
        {
            Part(Cube, FVector(-125 + I * 50, 0, 0.8f), FVector(2, 292, 1), Bark);
        }
        break;
    case Homestead::Piece::Wall:
        Part(Cube, FVector(0, 144, 130), FVector(300, 12, 260), Wood, true);
        for (int I = 0; I < 4; ++I)
        {
            Part(Cube, FVector(-142 + I * 94.67f, 142, 130), FVector(12, 20, 260), Bark);
        }
        Part(Cube, FVector(0, 142, 252), FVector(300, 22, 16), Bark);
        break;
    case Homestead::Piece::Doorway:
        Part(Cube, FVector(-107.5f, 144, 130), FVector(85, 12, 260), Wood, true);
        Part(Cube, FVector(107.5f, 144, 130), FVector(85, 12, 260), Wood, true);
        Part(Cube, FVector(0, 144, 247.5f), FVector(130, 16, 25), Bark, true);
        // Tied-back cloth suggests a simple shelter entrance without a hidden collider.
        Part(Cube, FVector(-61, 133, 123), FVector(8, 8, 214), HomesteadWorldLook::Cloth);
        Part(Cube, FVector(61, 133, 123), FVector(8, 8, 214), HomesteadWorldLook::Cloth);
        Part(Cube, FVector(0, 137, 235), FVector(124, 5, 8), HomesteadWorldLook::Cloth);
        break;
    case Homestead::Piece::Roof:
        Part(Cube, FVector(0, 0, 273), FVector(310, 310, 22), FLinearColor(0.27f, 0.25f, 0.105f), true);
        for (int I = -1; I <= 1; ++I)
        {
            Part(Cube, FVector(I * 110, 0, 256), FVector(12, 300, 12), Bark);
        }
        break;
    case Homestead::Piece::Fire:
    {
        const FVector Hearth(-100, 95, 0);
        for (int I = 0; I < 8; ++I)
        {
            const float Angle = I * PI / 4;
            Part(Sphere, Hearth + FVector(FMath::Cos(Angle) * 28, FMath::Sin(Angle) * 28, 7),
                FVector(19, 16, 14), Stone);
        }
        Part(Cylinder, Hearth + FVector(0, 0, 8), FVector(10, 10, 42), Bark, false, FRotator(85, 30, 0));
        Part(Cylinder, Hearth + FVector(0, 0, 10), FVector(10, 10, 42), Bark, false, FRotator(85, -30, 0));
        if (Structure.fuelHours > 0 && !bPreview)
        {
            Part(Cone, Hearth + FVector(0, 0, 28), FVector(24, 24, 45),
                FLinearColor(0.95f, 0.2f, 0.015f), false, FRotator::ZeroRotator, 3.0f);
            Part(Cone, Hearth + FVector(0, 0, 24), FVector(13, 13, 28),
                FLinearColor(1.0f, 0.63f, 0.08f), false, FRotator::ZeroRotator, 4.0f);
            UPointLightComponent* Light = NewObject<UPointLightComponent>(this);
            Light->SetupAttachment(GetRootComponent());
            Light->SetMobility(EComponentMobility::Movable);
            Light->SetRelativeLocation(Base + Rotation.RotateVector(Hearth + FVector(0, 0, 65)));
            Light->SetLightColor(FLinearColor(1.0f, 0.49f, 0.19f));
            Light->SetIntensity(1800.0f);
            Light->SetAttenuationRadius(550.0f);
            Light->SetSourceRadius(18.0f);
            Light->SetCastShadows(false);
            Light->RegisterComponent();
            Visual.Components.Add(Light);
        }
        break;
    }
    case Homestead::Piece::Bed:
        Part(Cube, FVector(95, -10, 6), FVector(70, 155, 12), HomesteadWorldLook::Cloth, true);
        Part(Cube, FVector(95, 46, 14), FVector(62, 32, 12), FLinearColor(0.7f, 0.65f, 0.46f));
        Part(Cube, FVector(95, -28, 14), FVector(68, 106, 8), FLinearColor(0.23f, 0.31f, 0.21f));
        break;
    case Homestead::Piece::Chest:
        Part(Cube, FVector(-100, -100, 27), FVector(70, 55, 54), Wood, true);
        Part(Cube, FVector(-100, -100, 55), FVector(74, 59, 6), Bark);
        Part(Cube, FVector(-100, -71, 35), FVector(12, 4, 12), Stone);
        break;
    default:
        break;
    }
    if (bPreview)
    {
        for (USceneComponent* Component : Visual.Components)
        {
            if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Component))
            {
                Mesh->SetCastShadow(false);
            }
        }
    }
}

bool AHomesteadWorld::BuildLampDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop)
{
    // Stand it on whatever is underfoot: the terrain outdoors, a floor or hearthstone indoors. Start
    // low enough to miss lintels and roofs, and skip starts inside a wall's collision.
    FVector Base = AtGround(Drop.position.x, Drop.position.y, 0.0f);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadLampDrop), false, UGameplayStatics::GetPlayerPawn(this, 0));
    for (const float Lift : {90.0f, 45.0f, 15.0f})
    {
        FHitResult Hit;
        if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Base + FVector(0, 0, Lift), Base - FVector(0, 0, 150), ECC_WorldStatic, Params)
            && !Hit.bStartPenetrating)
        {
            Base.Z = Hit.ImpactPoint.Z;
            break;
        }
    }
    auto* Stand = NewObject<USceneComponent>(this, MakeUniqueObjectName(this, USceneComponent::StaticClass(), TEXT("LampDrop")));
    Stand->SetupAttachment(GetRootComponent());
    Stand->SetMobility(EComponentMobility::Movable);
    Stand->SetWorldLocationAndRotation(Base, FRotator(0, Drop.id * 53 % 360, 0));
    Stand->RegisterComponent();
    const TArray<UStaticMeshComponent*> Parts = HomesteadLampLook::AddParts(this, Stand, FVector::ZeroVector, TEXT("LampDropPart"));
    if (Parts.IsEmpty())
    {
        Stand->DestroyComponent();
        return false;
    }
    Visual.Components.Add(Stand);
    for (UStaticMeshComponent* Part : Parts) if (Part) Visual.Components.Add(Part);
    UPointLightComponent* Light = HomesteadLampLook::AddLight(this, Stand, FVector::ZeroVector, TEXT("LampDropLight"));
    if (Light) Visual.Components.Add(Light);
    LampDropFlame = Parts.Last();
    LampDropGlass = Parts.Num() >= 3 ? Parts[1] : nullptr;
    LampDropLight = Light;
    HomesteadLampLook::SetLit(Parts.Last(), Light, bLampDropLit, LampDropFlickerTime, LampDropGlass.Get());
    return true;
}

void AHomesteadWorld::BuildDrop(FHomesteadWorldVisual& Visual, const Homestead::WorldDrop& Drop)
{
    if (Drop.wearableId == 0 && Drop.item == Homestead::Item::OilLamp && BuildLampDrop(Visual, Drop)) return;
    FLinearColor Tint(0.56f, 0.43f, 0.22f);
    if (Drop.wearableId != 0) Tint = FLinearColor(0.64f, 0.42f, 0.52f);
    else
    {
        switch (Homestead::CategoryOf(Drop.item))
        {
        case Homestead::ItemCategory::Tool: Tint = FLinearColor(0.22f, 0.28f, 0.26f); break;
        case Homestead::ItemCategory::Forage:
        case Homestead::ItemCategory::Food: Tint = FLinearColor(0.62f, 0.30f, 0.19f); break;
        case Homestead::ItemCategory::Supply: Tint = FLinearColor(0.34f, 0.54f, 0.48f); break;
        default: break;
        }
    }
    const FVector Base = AtGround(Drop.position.x, Drop.position.y, 7);
    AddPart(Visual, Cylinder, Base, FVector(48, 48, 14), Wood, false);
    AddPart(Visual, Cube, Base + FVector(0, 0, 16), FVector(44, 34, 14), Tint,
        false, FRotator(0, Drop.id * 37 % 360, 8), 0.75f);
    const int32 Marks = FMath::Clamp(Drop.quantity, 1, 5);
    for (int32 Index = 0; Index < Marks; ++Index)
        AddPart(Visual, Sphere, Base + FVector(-12 + Index * 6, 0, 29),
            FVector(5, 5, 5), FLinearColor(0.93f, 0.82f, 0.52f), false);
    const FLinearColor Tie(0.78f, 0.57f, 0.18f);
    AddPart(Visual, Cube, Base + FVector(0, 0, 24), FVector(7, 36, 4),
        Tie, false, FRotator::ZeroRotator, 0.7f);
    AddPart(Visual, Cube, Base + FVector(0, 0, 24), FVector(44, 7, 4),
        Tie, false, FRotator::ZeroRotator, 0.7f);
    AddPart(Visual, Sphere, Base + FVector(0, 0, 30), FVector(9, 9, 9),
        FLinearColor(0.93f, 0.72f, 0.24f), false, FRotator::ZeroRotator, 0.65f, 0.05f);
}
