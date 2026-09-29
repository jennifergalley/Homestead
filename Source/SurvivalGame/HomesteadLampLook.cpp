#include "HomesteadLampLook.h"

#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

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
    Light->SetIntensity(LightIntensity);
    Light->SetAttenuationRadius(LightRadius);
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
        if (bLit) Light->SetIntensity(LightIntensity * Shiver);
    }
}
}
