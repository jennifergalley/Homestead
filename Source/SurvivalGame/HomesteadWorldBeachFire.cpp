#include "HomesteadWorld.h"
#include "HomesteadBeachLook.h"
#include "HomesteadWorldLog.h"
#include "Simulation/HomesteadBeach.h"
#include "Simulation/HomesteadLampLight.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"

// add-beach-gathering: the beach campfire while it burns. Jenny (2026-10-10): "it then burns for a few hours
// until burning out, shedding light in an area around it just like the lamp, but more orange and flickering."
// The pit and the unlit stack are ordinary resource visuals (BuildResource); this adds the burning stack,
// the ember bed, the flame tongues and the light over the pit while Homestead::Beach::CampfireLit holds.
namespace HomesteadBeachFireLook
{
// The lamp's gentle falloff and reach (HomesteadLampLight.h: 0.2 lux at about 13 m, nothing past 20 m), a
// little brighter for a fire of driftwood than a wick, and the colour of burning wood rather than lamp oil.
constexpr float LightIntensity = 6.0f;
constexpr float LightFalloffExponent = static_cast<float>(Homestead::OilLampLight.falloffExponent);
constexpr float LightHeightCm = 45.0f;
const FLinearColor LightColor(1.0f, 0.40f, 0.12f);
// Shadows only while the camera is near, like a lamp set down.
constexpr double ShadowDistanceCm = Homestead::PlacedLampShadowDistanceCm;

// A wood fire's light: a slow breath, the flames' licks and a fast flutter (louder than the lamp's wick).
float Flicker(float Time)
{
    return 0.8f + 0.12f * FMath::PerlinNoise1D(Time * 1.4f) + 0.1f * FMath::PerlinNoise1D(Time * 5.7f)
        + 0.06f * FMath::PerlinNoise1D(Time * 14.0f);
}
}

void AHomesteadWorld::RefreshBeachCampfires(const Homestead::State& State)
{
    TSet<int32> Lit;
    for (const auto& Node : State.resources)
    {
        if (!Homestead::Beach::CampfireLit(Node, State.hour)) continue;
        Lit.Add(Node.id);
        if (const FBeachCampfireFire* Existing = BeachCampfires.Find(Node.id))
        {
            if (Existing->Light.IsValid()) continue;
            // Its components went with a world rebuild: build it again.
            for (const auto& Part : Existing->Parts)
                if (USceneComponent* Component = Part.Get()) Component->DestroyComponent();
            BeachCampfires.Remove(Node.id);
        }
        // Sit the fire exactly on the pit the resource visual placed (same origin, yaw and ground height).
        const FHomesteadWorldVisual* Visual = ResourceVisuals.Find(Node.id);
        const USceneComponent* Pit = nullptr;
        if (Visual)
            for (const auto& Component : Visual->Components)
                if (const auto* Mesh = Cast<UStaticMeshComponent>(Component.Get());
                    Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetName() == TEXT("SM_BeachFirePit"))
                { Pit = Mesh; break; }
        if (!Pit) continue;
        const FTransform At = Pit->GetRelativeTransform();
        FBeachCampfireFire& Fire = BeachCampfires.Add(Node.id);
        Fire.Phase = static_cast<float>(Node.id % 97) * 1.3f;
        auto AddPart = [&](const TCHAR* Name, const TCHAR* MaterialName) -> UStaticMeshComponent*
        {
            UStaticMesh* Mesh = HomesteadBeachLook::LoadProp(TEXT("BeachCampfire"), Name);
            if (!Mesh)
            {
                UE_LOG(LogHomesteadWorld, Error, TEXT("Beach campfire %d is missing authored mesh BeachCampfire/%s"), Node.id, Name);
                return nullptr;
            }
            auto* Part = NewObject<UStaticMeshComponent>(this);
            Part->SetupAttachment(GetRootComponent());
            Part->SetMobility(EComponentMobility::Movable);
            Part->SetStaticMesh(Mesh);
            Part->SetRelativeTransform(At);
            Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Part->SetCanEverAffectNavigation(false);
            if (MaterialName)
            {
                // Unlit, exposure-relative glow (Scripts/Props/build_campfire_materials.py), so the fire reads in sun too.
                if (UMaterialInterface* Glow = LoadObject<UMaterialInterface>(nullptr, *FString::Printf(
                        TEXT("/Game/SurvivalGame/Environment/Props/BeachCampfire/%s.%s"), MaterialName, MaterialName), nullptr, LOAD_NoWarn | LOAD_Quiet))
                    for (int32 Slot = 0; Slot < Mesh->GetStaticMaterials().Num(); ++Slot) Part->SetMaterial(Slot, Glow);
                else
                    UE_LOG(LogHomesteadWorld, Error, TEXT("Beach campfire material %s is missing."), MaterialName);
                Part->SetCastShadow(false);
                Fire.Glowing.Add(Part);
            }
            Part->RegisterComponent();
            Fire.Parts.Add(Part);
            return Part;
        };
        AddPart(TEXT("SM_CampfireBurning"), nullptr);
        AddPart(TEXT("SM_CampfireEmbers"), TEXT("M_BeachCampfireEmbers"));
        if (UStaticMeshComponent* Flames = AddPart(TEXT("SM_CampfireFlames"), TEXT("M_BeachCampfireFlame")))
        {
            Fire.Flames.Add(Flames);
            Fire.FlameScales.Add(Flames->GetRelativeScale3D());
        }
        auto* Light = NewObject<UPointLightComponent>(this);
        Light->SetupAttachment(GetRootComponent());
        Light->SetMobility(EComponentMobility::Movable);
        Light->SetRelativeLocation(At.GetLocation() + FVector(0, 0, HomesteadBeachFireLook::LightHeightCm));
        Light->SetLightColor(HomesteadBeachFireLook::LightColor);
        Light->SetUseInverseSquaredFalloff(false);
        Light->SetLightFalloffExponent(HomesteadBeachFireLook::LightFalloffExponent);
        Light->SetAttenuationRadius(static_cast<float>(Homestead::Beach::CampfireLightRadiusCm));
        Light->SetIntensity(HomesteadBeachFireLook::LightIntensity);
        Light->SetSourceRadius(12.0f);
        Light->SetSoftSourceRadius(20.0f);
        Light->SetCastShadows(false);
        Light->RegisterComponent();
        Fire.Light = Light;
        Fire.Parts.Add(Light);
    }
    for (auto It = BeachCampfires.CreateIterator(); It; ++It)
    {
        if (Lit.Contains(It.Key())) continue;
        for (const auto& Part : It.Value().Parts)
            if (USceneComponent* Component = Part.Get()) Component->DestroyComponent();
        It.RemoveCurrent();
    }
}

void AHomesteadWorld::UpdateBeachCampfires(float DeltaSeconds)
{
    if (BeachCampfires.IsEmpty()) return;
    BeachCampfireTime += DeltaSeconds;
    const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(this, 0);
    for (auto& Pair : BeachCampfires)
    {
        FBeachCampfireFire& Fire = Pair.Value;
        const float T = BeachCampfireTime + Fire.Phase;
        const float Breath = HomesteadBeachFireLook::Flicker(T);
        if (UPointLightComponent* Light = Fire.Light.Get())
        {
            Light->SetIntensity(HomesteadBeachFireLook::LightIntensity * Breath);
            if (Camera)
            {
                const bool bShadows = FVector::Dist(Camera->GetCameraLocation(), Light->GetComponentLocation())
                    < HomesteadBeachFireLook::ShadowDistanceCm;
                if (Light->CastShadows != bShadows) Light->SetCastShadows(bShadows);
            }
        }
        // The flames and coals brighten and dim with the light.
        for (const auto& Part : Fire.Glowing)
            if (UStaticMeshComponent* Glowing = Part.Get()) Glowing->SetScalarParameterValueOnMaterials(TEXT("Glow"), Breath);
        // The tongues stretch and lean as they lick up, quicker than the light breathes.
        for (int32 Index = 0; Index < Fire.Flames.Num(); ++Index)
            if (UStaticMeshComponent* Flames = Fire.Flames[Index].Get())
            {
                const float Lick = 0.86f + 0.2f * FMath::PerlinNoise1D(T * 3.1f + Index) + 0.08f * FMath::PerlinNoise1D(T * 9.0f);
                const float Sway = 1.0f + 0.06f * FMath::PerlinNoise1D(T * 2.3f + 5.0f);
                Flames->SetRelativeScale3D(Fire.FlameScales[Index] * FVector(Sway, 2.0f - Sway, Lick));
            }
    }
}
