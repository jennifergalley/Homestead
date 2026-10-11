// The sea cave in the cove's west cliff (openspec add-sea-cave), placed at runtime from Homestead::EstateSeaCave().
// The mesh, its pool, the lit fissure, the dressing and the sound are presentation only.
#include "HomesteadSeaCaveScenery.h"

#include "HomesteadController.h"
#include "HomesteadWaterPool.h"
#include "Simulation/HomesteadSeaCave.h"

#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadSeaCave, Log, All);

namespace HomesteadSeaCaveLook
{
const TCHAR* const RockMesh = TEXT("/Game/SurvivalGame/Environment/Props/SeaCave/SM_SeaCave.SM_SeaCave");
const TCHAR* const SkyMaterial = TEXT("/Game/SurvivalGame/Environment/SeaCave/M_SeaCaveSky.M_SeaCaveSky");
const TCHAR* const PoolMaterial = TEXT("/Game/SurvivalGame/Environment/SeaCave/MI_SeaCavePool.MI_SeaCavePool");
const TCHAR* const PoolFallback = TEXT("/Game/SurvivalGame/Materials/MI_EstatePond.MI_EstatePond");
const TCHAR* const PlaneMesh = TEXT("/Engine/BasicShapes/Plane.Plane");
const TCHAR* const DripsSound = TEXT("/Game/SurvivalGame/Audio/Ambience/SeaCaveDrips.SeaCaveDrips");
const TCHAR* const SurfSound = TEXT("/Game/SurvivalGame/Audio/Ambience/SeaCaveSurf.SeaCaveSurf");
const TCHAR* const CliffMaterialFolder = TEXT("/Game/SurvivalGame/Environment/SeaCave");

// The fissure (Recipes/sea_cave.py's fissure box): 6 m along its crossing joint, 0.6 m wide, its joint turned
// 28 degrees left of the cave's axis (mirrored into Unreal's frame). The card closes its top, just inside it.
constexpr float FissureLengthCm = 560.0f;
constexpr float FissureWidthCm = 50.0f;
constexpr float FissureYawDeg = -28.0f;
constexpr float CardBelowTopCm = 15.0f;
// The thread of sky through it, as a narrow spot light down onto the pool (lumens). Daylight sky through a
// slot this size, against eye adaptation in a dim chamber; at night a faint blue moonlight (Balance: night is
// never pitch black in the estate, and her lamp lights the rest).
constexpr float ShaftDayLumens = 3500.0f;   // tuned in RT-on PIE against the cave's dimmed sky fill
constexpr float ShaftNightLumens = 450.0f;
constexpr float ShaftOvercastScale = 0.45f;
constexpr float ShaftInnerConeDeg = 10.0f;
constexpr float ShaftOuterConeDeg = 24.0f;
constexpr float ShaftReachCm = 1600.0f;
const FLinearColor DaySky(0.93f, 0.97f, 1.0f);
const FLinearColor NightSky(0.45f, 0.58f, 1.0f);
// The card's glow (emissive), matched to the shaft by eye in the dim chamber.
constexpr float CardDayGlow = 14.0f;
constexpr float CardNightGlow = 0.6f;
// Wading in from the pool's edge: it shelves to about knee deep in the middle, so a short way only.
constexpr float PoolWadeInsetCm = 70.0f;
constexpr float PoolShelfSlope = 0.2f;
// Drips fill the chamber and fade before the mouth; the sea is heard from the mouth, muffled, deep inside.
constexpr float DripsFalloffCm = 2600.0f;
constexpr float SurfFalloffCm = 4200.0f;
constexpr float DripsGain = 0.55f;
constexpr float SurfGain = 0.5f;
constexpr float SurfLowPassHz = 900.0f;

bool IsGroundCover(const FString& Path)
{
    return Path.Contains(TEXT("Fern")) || Path.Contains(TEXT("Campion")) || Path.Contains(TEXT("Cobbles"))
        || Path.Contains(TEXT("Spalls")) || Path.Contains(TEXT("Rubble"));
}
}

AHomesteadSeaCave::AHomesteadSeaCave()
{
    PrimaryActorTick.bCanEverTick = false;
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    Root->SetMobility(EComponentMobility::Movable);
    SetRootComponent(Root);
}

void AHomesteadSeaCave::EndPlay(const EEndPlayReason::Type Reason)
{
    if (IsValid(Pool)) Pool->Destroy();
    Pool = nullptr;
    Super::EndPlay(Reason);
}

bool AHomesteadSeaCave::Build()
{
    using namespace HomesteadSeaCaveLook;
    if (bBuilt) return true;
    const Homestead::SeaCave& Cave = Homestead::EstateSeaCave();
    if (!Cave.valid()) return false;
    SetActorLocationAndRotation(FVector(Cave.anchor.x, Cave.anchor.y, Cave.floorZ),
        FRotator(0.0f, static_cast<float>(Cave.yaw), 0.0f));
    // The cliff dressing stands on its own: it goes down even before the cave mesh is imported. Its instances are
    // added in world space, so the actor must already stand at the cave's anchor.
    BuildCliff();
    UStaticMesh* RockAsset = LoadObject<UStaticMesh>(nullptr, RockMesh, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!RockAsset)
    {
        UE_LOG(LogHomesteadSeaCave, Log, TEXT("Sea cave: SM_SeaCave isn't imported; not placing the cave."));
        return !Cliff.IsEmpty();
    }
    bBuilt = true;

    // The rock: walked on (complex collision on its coarse LOD), and the camera stays inside it.
    Rock = NewObject<UStaticMeshComponent>(this, TEXT("Rock"));
    Rock->SetupAttachment(GetRootComponent());
    Rock->SetMobility(EComponentMobility::Movable);
    Rock->SetStaticMesh(RockAsset);
    Rock->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
    Rock->SetGenerateOverlapEvents(false);
    Rock->RegisterComponent();

    // The pool: still fresh water in its rock basin.
    UMaterialInterface* Water = LoadObject<UMaterialInterface>(nullptr, PoolMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (!Water) Water = LoadObject<UMaterialInterface>(nullptr, PoolFallback, nullptr, LOAD_NoWarn | LOAD_Quiet);
    if (Cave.shore.size() >= 3)
    {
        FActorSpawnParameters Spawn;
        Spawn.ObjectFlags |= RF_Transient;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Pool = GetWorld()->SpawnActor<AHomesteadWaterPool>(AHomesteadWaterPool::StaticClass(),
            FVector(Cave.shore[0].x, Cave.shore[0].y, Cave.waterZ), FRotator::ZeroRotator, Spawn);
        if (Pool)
        {
            Pool->WadeInset = PoolWadeInsetCm;
            Pool->ShelfSlope = PoolShelfSlope;
            Pool->Material = Water;
            TArray<FVector2D> Shore;
            for (const Homestead::Point& Point : Cave.shore) Shore.Add(FVector2D(Point.x, Point.y));
            Pool->SetShore(Shore, static_cast<float>(Cave.waterZ));
        }
    }

    // The fissure: a strip of sky closing its top and a narrow light down onto the pool.
    const FVector LightAt(Cave.lightX, Cave.lightY, Cave.lightZ);
    const float FissureYaw = static_cast<float>(Cave.yaw) + FissureYawDeg;
    if (UStaticMesh* Plane = LoadObject<UStaticMesh>(nullptr, PlaneMesh, nullptr, LOAD_NoWarn | LOAD_Quiet))
    {
        SkyCard = NewObject<UStaticMeshComponent>(this, TEXT("FissureSky"));
        SkyCard->SetupAttachment(GetRootComponent());
        SkyCard->SetMobility(EComponentMobility::Movable);
        SkyCard->SetStaticMesh(Plane);
        // The engine plane is 100 cm square facing +Z: turned over to face down the fissure.
        SkyCard->SetWorldTransform(FTransform(FRotator(0.0f, FissureYaw, 180.0f), LightAt - FVector(0, 0, CardBelowTopCm),
            FVector(FissureLengthCm / 100.0f, FissureWidthCm / 100.0f, 1.0f)));
        SkyCard->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
        SkyCard->SetCastShadow(false);
        if (UMaterialInterface* Sky = LoadObject<UMaterialInterface>(nullptr, SkyMaterial, nullptr, LOAD_NoWarn | LOAD_Quiet))
        {
            SkyCardMaterial = UMaterialInstanceDynamic::Create(Sky, this);
            SkyCard->SetMaterial(0, SkyCardMaterial);
        }
        SkyCard->RegisterComponent();
    }
    Shaft = NewObject<USpotLightComponent>(this, TEXT("FissureShaft"));
    Shaft->SetupAttachment(GetRootComponent());
    Shaft->SetMobility(EComponentMobility::Movable);
    Shaft->SetWorldLocationAndRotation(LightAt - FVector(0, 0, CardBelowTopCm + 10.0f), FRotator(-90.0f, FissureYaw, 0.0f));
    Shaft->SetIntensityUnits(ELightUnits::Lumens);
    Shaft->SetInnerConeAngle(ShaftInnerConeDeg);
    Shaft->SetOuterConeAngle(ShaftOuterConeDeg);
    Shaft->SetAttenuationRadius(ShaftReachCm);
    Shaft->SetSourceLength(FissureLengthCm * 0.8f);
    Shaft->SetSourceRadius(FissureWidthCm * 0.5f);
    Shaft->SetVolumetricScatteringIntensity(3.0f);
    Shaft->SetCastShadows(true);
    Shaft->RegisterComponent();

    // Reused rocks, ferns and flowers.
    TMap<FString, UStaticMesh*> Loaded;
    int32 Missing = 0;
    for (const Homestead::SeaCaveProp& Prop : Cave.props)
    {
        const FString Path = UTF8_TO_TCHAR(Prop.mesh);
        UStaticMesh*& Mesh = Loaded.FindOrAdd(Path);
        if (!Mesh) Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (!Mesh)
        {
            ++Missing;
            continue;
        }
        UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
        Part->SetupAttachment(GetRootComponent());
        Part->SetMobility(EComponentMobility::Movable);
        Part->SetStaticMesh(Mesh);
        Part->SetWorldTransform(FTransform(FRotator(0.0f, static_cast<float>(Prop.yaw), 0.0f),
            FVector(Prop.position.x, Prop.position.y, Prop.z), FVector(static_cast<float>(Prop.scale))));
        if (IsGroundCover(Path))
        {
            // Cobbles, spalls, ferns and flowers are walked over, never snag her.
            Part->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
            Part->SetCanEverAffectNavigation(false);
        }
        else
        {
            Part->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
            Part->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
        }
        Part->SetGenerateOverlapEvents(false);
        Part->RegisterComponent();
        Dressing.Add(Part);
    }

    // Drips in the chamber over the pool; the sea at the mouth, heard muffled from inside.
    const auto AddSound = [this](const TCHAR* Path, const FVector& At, float Falloff, float Gain, float LowPassHz)
    {
        USoundWave* Wave = LoadObject<USoundWave>(nullptr, Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (!Wave) return;
        UAudioComponent* Audio = NewObject<UAudioComponent>(this);
        Audio->SetupAttachment(GetRootComponent());
        Audio->SetWorldLocation(At);
        Audio->SetSound(Wave);
        Audio->bAutoActivate = false;
        Audio->bOverrideAttenuation = true;
        Audio->AttenuationOverrides.bAttenuate = true;
        Audio->AttenuationOverrides.bSpatialize = true;
        Audio->AttenuationOverrides.DistanceAlgorithm = EAttenuationDistanceModel::NaturalSound;
        Audio->AttenuationOverrides.dBAttenuationAtMax = -60.0f;
        Audio->AttenuationOverrides.AttenuationShapeExtents = FVector(400.0f);
        Audio->AttenuationOverrides.FalloffDistance = Falloff;
        if (LowPassHz > 0.0f)
        {
            Audio->SetLowPassFilterEnabled(true);
            Audio->SetLowPassFilterFrequency(LowPassHz);
        }
        Audio->SetVolumeMultiplier(Gain);
        Audio->RegisterComponent();
        Audio->Play(FMath::FRandRange(0.0f, 20.0f));
        Sounds.Add(Audio);
        SoundGains.Add(Gain);
    };
    AddSound(DripsSound, FVector(LightAt.X, LightAt.Y, Cave.floorZ + 250.0), DripsFalloffCm, DripsGain, 0.0f);
    AddSound(SurfSound, FVector(Cave.anchor.x, Cave.anchor.y, Cave.floorZ + 200.0), SurfFalloffCm, SurfGain, SurfLowPassHz);

    UE_LOG(LogHomesteadSeaCave, Log, TEXT("Sea cave: placed at (%.0f, %.0f, %.0f) yaw %.0f; pool %s, %d dressing parts (%d missing), %d sounds."),
        Cave.anchor.x, Cave.anchor.y, Cave.floorZ, Cave.yaw, Pool ? TEXT("filled") : TEXT("missing"),
        Dressing.Num(), Missing, Sounds.Num());
    return true;
}

void AHomesteadSeaCave::BuildCliff()
{
    using namespace HomesteadSeaCaveLook;
    if (!Cliff.IsEmpty()) return;
    const std::vector<Homestead::CoveCliffProp>& Props = Homestead::EstateCoveCliff();
    TMap<FString, UInstancedStaticMeshComponent*> Batches;
    TSet<FString> Missing;
    for (const Homestead::CoveCliffProp& Prop : Props)
    {
        const FString Path = UTF8_TO_TCHAR(Prop.mesh);
        UInstancedStaticMeshComponent** Found = Batches.Find(Path);
        if (!Found)
        {
            if (Missing.Contains(Path)) continue;
            UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
            if (!Mesh)
            {
                Missing.Add(Path);
                continue;
            }
            UInstancedStaticMeshComponent* Batch = NewObject<UInstancedStaticMeshComponent>(this);
            Batch->SetupAttachment(GetRootComponent());
            Batch->SetMobility(EComponentMobility::Movable);
            Batch->SetStaticMesh(Mesh);
            if (Homestead::CoveCliffPropBlocks(Prop))
            {
                // The cliff's weathered stone reads darker than the estate's sunlit boulders (build_sea_cave.py).
                const FString Name = FPaths::GetBaseFilename(Path);
                const FString Dark = FString::Printf(TEXT("%s/MI_%s_Cliff.MI_%s_Cliff"), CliffMaterialFolder,
                    *Name.RightChop(3), *Name.RightChop(3));
                if (UMaterialInterface* Override = LoadObject<UMaterialInterface>(nullptr, *Dark, nullptr, LOAD_NoWarn | LOAD_Quiet))
                    Batch->SetMaterial(0, Override);
                Batch->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
                Batch->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
            }
            else
            {
                Batch->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
                Batch->SetCastShadow(false);
            }
            Batch->SetCanEverAffectNavigation(false);
            Batch->SetGenerateOverlapEvents(false);
            Batch->RegisterComponent();
            Cliff.Add(Batch);
            Found = &Batches.Add(Path, Batch);
        }
        const FVector Up = FVector(Prop.upX, Prop.upY, Prop.upZ).GetSafeNormal(UE_SMALL_NUMBER, FVector::UpVector);
        const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Up)
            * FQuat(FVector::UpVector, FMath::DegreesToRadians(static_cast<float>(Prop.yaw)));
        (*Found)->AddInstance(FTransform(Rotation, FVector(Prop.position.x, Prop.position.y, Prop.z),
            FVector(static_cast<float>(Prop.scale), static_cast<float>(Prop.scale), static_cast<float>(Prop.scaleZ))),
            /*bWorldSpace=*/true);
    }
    UE_LOG(LogHomesteadSeaCave, Log, TEXT("Cove cliff: %d pieces in %d batches%s."), static_cast<int32>(Props.size()),
        Cliff.Num(), Missing.IsEmpty() ? TEXT("") : *FString::Printf(TEXT(" (%d meshes not imported)"), Missing.Num()));
}

void AHomesteadSeaCave::SetSky(float Daylight, float Overcast)
{
    using namespace HomesteadSeaCaveLook;
    if (!bBuilt) return;
    // The player's ambience setting carries the cave's drips and sea with the rest of the ambience.
    const AHomesteadController* Game = GetWorld() ? Cast<AHomesteadController>(GetWorld()->GetFirstPlayerController()) : nullptr;
    const float Setting = Game ? Game->AmbienceVolume : 0.7f;
    for (int32 Index = 0; Index < Sounds.Num(); ++Index)
        if (Sounds[Index]) Sounds[Index]->SetVolumeMultiplier(SoundGains[Index] * Setting);
    if (FMath::Abs(Daylight - AppliedDaylight) < 0.01f && FMath::Abs(Overcast - AppliedOvercast) < 0.02f) return;
    AppliedDaylight = Daylight;
    AppliedOvercast = Overcast;
    const float Cloud = FMath::Lerp(1.0f, ShaftOvercastScale, Overcast);
    const FLinearColor Colour = FMath::Lerp(NightSky, DaySky, Daylight);
    if (Shaft)
    {
        Shaft->SetIntensity(FMath::Lerp(ShaftNightLumens, ShaftDayLumens * Cloud, Daylight));
        Shaft->SetLightColor(Colour);
    }
    if (SkyCardMaterial)
    {
        SkyCardMaterial->SetVectorParameterValue(TEXT("Color"), Colour);
        SkyCardMaterial->SetScalarParameterValue(TEXT("Intensity"), FMath::Lerp(CardNightGlow, CardDayGlow * Cloud, Daylight));
    }
}
