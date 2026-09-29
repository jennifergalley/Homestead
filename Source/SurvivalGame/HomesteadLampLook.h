#pragma once

#include "CoreMinimal.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

// add-oil-lamp: how the oil lamp looks, shared by the held lamp (AHomesteadCharacter) and a lamp
// set on the ground (AHomesteadWorld). The meshes come from Scripts/Blender/oil_lamp.py with the
// pivot at the bottom centre of the fount: SM_OilLamp (tin and brass), SM_OilLampGlass (the
// chimney) and SM_OilLampFlame.
namespace HomesteadLampLook
{
// In the lamp mesh's frame (cm): the top of the bail's grip, raised, and the flame's centre
// (oil_lamp.py reports both).
constexpr float BailTopHeight = 34.1f;
constexpr float FlameHeight = 13.55f;

// The light: warm, small, about ten metres of useful reach at night.
constexpr float LightIntensity = 1400.0f;
constexpr float LightRadius = 1000.0f;
const FLinearColor LightColor(1.0f, 0.62f, 0.30f);

// Adds the lamp's parts under Parent, offset so the mesh's pivot (bottom centre) sits at Offset in
// Parent's frame. Returns the parts, the flame last (null if missing), or an empty array when the
// lamp mesh hasn't been imported.
TArray<UStaticMeshComponent*> AddParts(UObject* Outer, USceneComponent* Parent, const FVector& Offset, FName Prefix);
// A point light at the flame, under Parent (same Offset convention). Registered, initially on.
UPointLightComponent* AddLight(UObject* Outer, USceneComponent* Parent, const FVector& Offset, FName Name);
// A small oil flame: mostly steady, with quick shivers. Time in seconds; about 0.9-1.05.
float Flicker(float Time);
// Shows the flame and light when lit, animating both, and warms the glass (its "Glow").
void SetLit(UStaticMeshComponent* Flame, UPointLightComponent* Light, bool bLit, float Time, UStaticMeshComponent* Glass = nullptr);
}
