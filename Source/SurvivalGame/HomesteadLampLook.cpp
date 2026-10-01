#include "HomesteadLampLook.h"
#include "Simulation/HomesteadLampLight.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

namespace HomesteadLampLookTuning
{
TAutoConsoleVariable<float> CVarLampIntensity(TEXT("homestead.LampIntensity"),
    static_cast<float>(Homestead::OilLampLight.intensity),
    TEXT("Oil lamp light: unitless intensity (about lux at the flame) with the gentle falloff."));
TAutoConsoleVariable<float> CVarLampRadius(TEXT("homestead.LampRadius"),
    static_cast<float>(Homestead::OilLampLight.radiusCm / 100.0),
    TEXT("Oil lamp light: attenuation radius in metres."));
TAutoConsoleVariable<float> CVarLampFalloff(TEXT("homestead.LampFalloff"),
    static_cast<float>(Homestead::OilLampLight.falloffExponent),
    TEXT("Oil lamp light: falloff exponent, (1 - (d/R)^2)^exponent."));
TAutoConsoleVariable<int32> CVarLampLegacy(TEXT("homestead.LampLegacy"), 0,
    TEXT("1: the first lamp's inverse-square light (1400, 10 m), for A/B against the tuned one."));
TAutoConsoleVariable<float> CVarPlacedLampShadowDistance(TEXT("homestead.PlacedLampShadowDistance"),
    static_cast<float>(Homestead::PlacedLampShadowDistanceCm / 100.0),
    TEXT("A lamp set down casts shadows only while the camera is within this many metres (0: always)."));

Homestead::LampLightProfile CurrentProfile()
{
    if (CVarLampLegacy.GetValueOnGameThread() != 0) return Homestead::LegacyOilLampLight;
    Homestead::LampLightProfile Profile = Homestead::OilLampLight;
    Profile.intensity = FMath::Max(0.0f, CVarLampIntensity.GetValueOnGameThread());
    Profile.radiusCm = FMath::Max(100.0f, CVarLampRadius.GetValueOnGameThread() * 100.0f);
    Profile.falloffExponent = FMath::Max(0.1f, CVarLampFalloff.GetValueOnGameThread());
    return Profile;
}

// The profile on a light, the intensity scaled by the flame's shiver; the shape only when it changed.
void ApplyProfile(UPointLightComponent* Light, float Shiver)
{
    const Homestead::LampLightProfile Profile = CurrentProfile();
    const float Radius = static_cast<float>(Profile.radiusCm);
    const float Exponent = static_cast<float>(Profile.falloffExponent);
    if (Light->bUseInverseSquaredFalloff != Profile.inverseSquared) Light->SetUseInverseSquaredFalloff(Profile.inverseSquared);
    if (!FMath::IsNearlyEqual(Light->AttenuationRadius, Radius)) Light->SetAttenuationRadius(Radius);
    if (!FMath::IsNearlyEqual(Light->LightFalloffExponent, Exponent)) Light->SetLightFalloffExponent(Exponent);
    Light->SetIntensity(static_cast<float>(Profile.intensity) * Shiver);
}
}

namespace HomesteadLampLook
{
namespace
{
const TCHAR* PropFolder = TEXT("/Game/SurvivalGame/Environment/Props/OilLamp/");

template <typename T>
T* LoadLampAsset(const TCHAR* Name)
{
    // Not cached, so a prop imported mid-session shows up on the next build.
    return LoadObject<T>(nullptr, *FString::Printf(TEXT("%s%s.%s"), PropFolder, Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
}

UStaticMeshComponent* AddPart(UObject* Outer, USceneComponent* Parent, UStaticMesh* Mesh, const FVector& Offset, FName Name)
{
    auto* Part = NewObject<UStaticMeshComponent>(Outer, MakeUniqueObjectName(Outer, UStaticMeshComponent::StaticClass(), Name));
    Part->SetStaticMesh(Mesh);
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Part->SetupAttachment(Parent);
    Part->SetRelativeLocation(Offset);
    Part->RegisterComponent();
    return Part;
}
}

TArray<UStaticMeshComponent*> AddParts(UObject* Outer, USceneComponent* Parent, const FVector& Offset, FName Prefix)
{
    TArray<UStaticMeshComponent*> Parts;
    UStaticMesh* Body = LoadLampAsset<UStaticMesh>(TEXT("SM_OilLamp"));
    if (!Outer || !Parent || !Body) return Parts;
    const FString Base = Prefix.ToString();
    UStaticMeshComponent* Frame = AddPart(Outer, Parent, Body, Offset, *(Base + TEXT("_Body")));
    Frame->SetCastShadow(true);
    Parts.Add(Frame);
    if (UStaticMesh* GlassMesh = LoadLampAsset<UStaticMesh>(TEXT("SM_OilLampGlass")))
    {
        UStaticMeshComponent* Chimney = AddPart(Outer, Parent, GlassMesh, Offset, *(Base + TEXT("_Glass")));
        if (UMaterialInterface* Material = LoadLampAsset<UMaterialInterface>(TEXT("M_OilLampGlass")))
            Chimney->SetMaterial(0, Material);
        // Glass throws no shadow of its own, and no light reaches it (lighting channel 2 only): the flame
        // an inch away would blow it out to white. It glows from its own material instead (SetLit).
        Chimney->SetCastShadow(false);
        Chimney->SetLightingChannels(false, false, true);
        Parts.Add(Chimney);
    }
    UStaticMeshComponent* Flame = nullptr;
    if (UStaticMesh* FlameMesh = LoadLampAsset<UStaticMesh>(TEXT("SM_OilLampFlame")))
    {
        Flame = AddPart(Outer, Parent, FlameMesh, Offset, *(Base + TEXT("_Flame")));
        if (UMaterialInterface* Material = LoadLampAsset<UMaterialInterface>(TEXT("M_OilLampFlame")))
            Flame->SetMaterial(0, Material);
        Flame->SetCastShadow(false);
    }
    Parts.Add(Flame);
    return Parts;
}

UPointLightComponent* AddLight(UObject* Outer, USceneComponent* Parent, const FVector& Offset, FName Name)
{
    if (!Outer || !Parent) return nullptr;
    auto* Light = NewObject<UPointLightComponent>(Outer, MakeUniqueObjectName(Outer, UPointLightComponent::StaticClass(), Name));
    Light->SetMobility(EComponentMobility::Movable);
    Light->SetupAttachment(Parent);
    Light->SetRelativeLocation(Offset + FVector(0, 0, FlameHeight));
    Light->SetLightColor(LightColor);
    HomesteadLampLookTuning::ApplyProfile(Light, 1.0f);
    Light->SetSourceRadius(1.5f);
    Light->SetSoftSourceRadius(3.0f);
    Light->SetCastShadows(true);
    Light->RegisterComponent();
    return Light;
}

float Flicker(float Time)
{
    // A trimmed wick burns steadily behind glass: a slow breath and a faint quick shiver.
    return 0.96f + 0.04f * FMath::PerlinNoise1D(Time * 1.7f) + 0.025f * FMath::PerlinNoise1D(Time * 11.0f);
}

void SetLit(UStaticMeshComponent* Flame, UPointLightComponent* Light, bool bLit, float Time, UStaticMeshComponent* GlassPart)
{
    const float Shiver = Flicker(Time);
    if (Flame && Flame->IsVisible() != bLit) Flame->SetVisibility(bLit);
    if (GlassPart) GlassPart->SetScalarParameterValueOnMaterials(TEXT("Glow"), bLit ? Shiver : 0.0f);
    if (Light)
    {
        if (Light->IsVisible() != bLit) Light->SetVisibility(bLit);
        if (bLit) HomesteadLampLookTuning::ApplyProfile(Light, Shiver);
    }
}

void UpdatePlacedShadows(UPointLightComponent* Light, const UObject* WorldContext)
{
    if (!Light || !Light->IsVisible()) return;
    const APlayerCameraManager* Camera = UGameplayStatics::GetPlayerCameraManager(WorldContext, 0);
    if (!Camera) return;
    const double Distance = FVector::Dist(Camera->GetCameraLocation(), Light->GetComponentLocation());
    const bool bWant = Homestead::PlacedLampCastsShadows(Distance,
        HomesteadLampLookTuning::CVarPlacedLampShadowDistance.GetValueOnGameThread() * 100.0);
    if (Light->CastShadows != bWant) Light->SetCastShadows(bWant);
}
}
