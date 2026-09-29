#include "HomesteadWeather.h"

#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HomesteadController.h"
#include "HomesteadEstateTerrain.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Sound/SoundWave.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadWeather, Log, All);

namespace HomesteadWeatherDetail
{
constexpr const TCHAR* Folder = TEXT("/Game/SurvivalGame/Estate/Weather/");
// The cloud layer's sphere is the engine's 1 m sphere scaled to 8 km round the camera.
constexpr float CloudScale = 16000.0f;
const FName ShelterNames[UHomesteadWeather::MaxShelters] = {
    TEXT("Shelter0"), TEXT("Shelter1"), TEXT("Shelter2"), TEXT("Shelter3"),
    TEXT("Shelter4"), TEXT("Shelter5"), TEXT("Shelter6"), TEXT("Shelter7")};
}

UHomesteadWeather::UHomesteadWeather()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UHomesteadWeather::LoadAssets()
{
    using namespace HomesteadWeatherDetail;
    if (bAssetsTried) return bAssetsReady;
    bAssetsTried = true;
    auto Load = [](const TCHAR* Name) -> UObject*
    {
        const FString Path = FString(Folder) + Name + TEXT(".") + Name;
        UObject* Asset = StaticLoadObject(UObject::StaticClass(), nullptr, *Path);
        if (!Asset) UE_LOG(LogHomesteadWeather, Warning, TEXT("Weather asset missing: %s. Run Scripts/Terrain/build_weather.py."), *Path);
        return Asset;
    };
    auto* StreakMesh = Cast<UStaticMesh>(Load(TEXT("SM_RainStreaks")));
    auto* RainMaterial = Cast<UMaterialInterface>(Load(TEXT("MI_Rain")));
    auto* CloudMaterial = Cast<UMaterialInterface>(Load(TEXT("MI_RainClouds")));
    Parameters = Cast<UMaterialParameterCollection>(Load(TEXT("MPC_EstateWeather")));
    auto* CloudMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    auto* RainSound = LoadObject<USoundWave>(nullptr, TEXT("/Game/SurvivalGame/Audio/Ambience/RainLoop.RainLoop"));
    if (!StreakMesh || !RainMaterial || !CloudMaterial || !Parameters || !CloudMesh) return false;

    auto MakeMesh = [this](UStaticMesh* Mesh, UMaterialInterface* Material, const TCHAR* Name)
    {
        auto* Component = NewObject<UStaticMeshComponent>(GetOwner(), Name);
        Component->SetupAttachment(this);
        Component->SetUsingAbsoluteLocation(true);
        Component->SetUsingAbsoluteRotation(true);
        Component->SetMobility(EComponentMobility::Movable);
        Component->SetStaticMesh(Mesh);
        Component->SetMaterial(0, Material);
        Component->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCanEverAffectNavigation(false);
        Component->SetCastShadow(false);
        Component->bAffectDistanceFieldLighting = false;
        Component->SetVisibleInRayTracing(false);
        Component->bReceivesDecals = false;
        Component->SetGenerateOverlapEvents(false);
        Component->SetVisibility(false);
        Component->RegisterComponent();
        return Component;
    };
    Streaks = MakeMesh(StreakMesh, RainMaterial, TEXT("RainStreaks"));
    // The material moves every streak round the camera, so bounds are the whole wrapped volume.
    Streaks->SetBoundsScale(1.5f);
    Clouds = MakeMesh(CloudMesh, CloudMaterial, TEXT("RainClouds"));
    Clouds->SetWorldScale3D(FVector(CloudScale));
    Clouds->TranslucencySortPriority = -10; // behind the rain

    if (RainSound)
    {
        RainSound->bLooping = true;
        Sound = NewObject<UAudioComponent>(GetOwner(), TEXT("RainSound"));
        Sound->SetupAttachment(this);
        Sound->bAutoActivate = false;
        Sound->bAllowSpatialization = false;
        Sound->bIsUISound = false;
        Sound->SetSound(RainSound);
        Sound->SetLowPassFilterEnabled(true);
        Sound->SetLowPassFilterFrequency(20000.0f);
        Sound->RegisterComponent();
    }
    else UE_LOG(LogHomesteadWeather, Warning, TEXT("RainLoop is not imported; the rain is silent. Run Scripts/Terrain/build_weather.py."));
    bAssetsReady = true;
    return true;
}

void UHomesteadWeather::Update(const Homestead::State& State, float InDaylight)
{
    Rain = static_cast<float>(Homestead::RainAmount(State.hour));
    Overcast = static_cast<float>(Homestead::Overcast(State.hour));
    Daylight = InDaylight;
    Shelters.Reset();
    for (const auto& Structure : State.structures)
    {
        if (Structure.kind != Homestead::Piece::Roof) continue;
        const Homestead::Footprint Box = Homestead::StructureFootprint(State, Structure);
        const float Ground = HomesteadEstateTerrain::IsActive()
            ? HomesteadEstateTerrain::Height(Box.center.x, Box.center.y) : 0.0f;
        Shelters.Add(FVector4f(Box.center.x, Box.center.y,
            FMath::Max(ShelterRadiusCm, static_cast<float>(FMath::Sqrt(Box.half.x * Box.half.x + Box.half.y * Box.half.y))),
            (FMath::IsFinite(Ground) ? Ground : 0.0f) + ShelterTopCm));
    }
    ShelterFrom = FVector(FLT_MAX); // re-pick the nearest roofs on the next tick
}

bool UHomesteadWeather::IsUnderShelter(const FVector& Point) const
{
    for (const FVector4f& Roof : Shelters)
        if (Point.Z < Roof.W && FVector2D::DistSquared(FVector2D(Point), FVector2D(Roof.X, Roof.Y)) < Roof.Z * Roof.Z)
            return true;
    return false;
}

void UHomesteadWeather::TickWeather(float DeltaSeconds)
{
    UWorld* World = GetWorld();
    if (!World || !LoadAssets()) return;
    const APlayerController* Viewer = World->GetFirstPlayerController();
    if (!Viewer || !Viewer->PlayerCameraManager) return;
    const FVector Camera = Viewer->PlayerCameraManager->GetCameraLocation();
    if (Camera.ContainsNaN()) return;

    // Under a building piece's roof the material hides the streaks overhead and the rain outside
    // stays in view; under any other roof (the store, a doorway) the camera sees none at all.
    bInShelter = IsUnderShelter(Camera);
    OverheadCheckIn -= DeltaSeconds;
    if (OverheadCheckIn <= 0.0f && (Rain > 0.0f || Overcast > 0.0f))
    {
        OverheadCheckIn = 0.25f;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadWeatherOverhead), false);
        if (APawn* Pawn = Viewer->GetPawn()) Query.AddIgnoredActor(Pawn);
        bOverhead = World->LineTraceTestByChannel(Camera, Camera + FVector(0, 0, OverheadCheckCm), ECC_Visibility, Query);
    }
    const bool bIndoors = bInShelter || bOverhead;
    Indoors = FMath::FInterpConstantTo(Indoors, bIndoors ? 1.0f : 0.0f, DeltaSeconds, 2.0f);
    StreakFade = FMath::FInterpConstantTo(StreakFade, bOverhead && !bInShelter ? 0.0f : 1.0f, DeltaSeconds, 3.0f);

    const bool bRaining = Rain > 0.001f && StreakFade > 0.001f;
    Streaks->SetVisibility(bRaining);
    if (bRaining) Streaks->SetWorldLocation(Camera);
    const bool bCloudy = Overcast > 0.001f;
    Clouds->SetVisibility(bCloudy);
    if (bCloudy) Clouds->SetWorldLocation(Camera);

    if (UMaterialParameterCollectionInstance* Values = World->GetParameterCollectionInstance(Parameters))
    {
        Values->SetScalarParameterValue(TEXT("Rain"), Rain * StreakFade);
        Values->SetScalarParameterValue(TEXT("Overcast"), Overcast);
        Values->SetScalarParameterValue(TEXT("Daylight"), Daylight);
        // The nearest roofs to the camera, refreshed as it moves a few metres.
        if (FVector::DistSquared2D(Camera, ShelterFrom) > FMath::Square(300.0f))
        {
            ShelterFrom = Camera;
            TArray<FVector4f> Nearest = Shelters;
            Nearest.Sort([&Camera](const FVector4f& A, const FVector4f& B)
            {
                return FVector2D::DistSquared(FVector2D(Camera), FVector2D(A.X, A.Y))
                    < FVector2D::DistSquared(FVector2D(Camera), FVector2D(B.X, B.Y));
            });
            for (int32 Index = 0; Index < MaxShelters; ++Index)
            {
                const FVector4f Roof = Nearest.IsValidIndex(Index) ? Nearest[Index] : FVector4f(0, 0, 0, -1.0e7f);
                Values->SetVectorParameterValue(HomesteadWeatherDetail::ShelterNames[Index], FLinearColor(Roof.X, Roof.Y, Roof.Z, Roof.W));
            }
        }
    }

    if (Sound)
    {
        const auto* Game = Cast<AHomesteadController>(Viewer);
        const float Setting = Game ? Game->AmbienceVolume : 0.7f;
        const float Gain = FMath::Pow(Rain, 0.7f) * Setting * FMath::Lerp(OutdoorGain, IndoorGain, Indoors);
        if (Gain > 0.001f)
        {
            // Fade to full and let the volume multiplier carry the gain: FadeIn's level multiplies it, so
            // fading to Gain as well played the rain at Gain squared (inaudible in drizzle).
            if (!Sound->IsPlaying()) Sound->FadeIn(2.0f, 1.0f, FMath::FRandRange(0.0f, 30.0f));
            Sound->SetVolumeMultiplier(Gain);
            Sound->SetLowPassFilterFrequency(FMath::Lerp(20000.0f, IndoorCutoffHz, Indoors));
        }
        else if (Sound->IsPlaying()) Sound->Stop();
    }
}
