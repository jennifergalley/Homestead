// Foliage shadow motion (Jenny, 2026-09-29: the shrubs' shadows crawl). The estate's swaying shrubs
// (brambles, toyon hedge, deer brush, thimbleberry) sway by world-position offset in M_PropFoliage
// (Scripts/Blender/import_props.py FOLIAGE_WIND_CODE: WindStrength, LeafFlutter, weighted by height
// squared, so the tips move most) and cast shadows, which follow the sway every frame with ray-traced
// shadows. The candidate causes are the sway itself, their shadows, and the camera-safe dither. Each one
// has a console variable here, so a single package can A/B them at fixed cameras. The default is the fix
// under test: the sway cut to ShrubWindDefault of what was authored, shadows and dither kept.
#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"

#include "Components/MeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

namespace HomesteadFoliageMotion
{
constexpr float ShrubWindDefault = 0.4f;
TAutoConsoleVariable<float> CVarShrubWind(TEXT("homestead.ShrubWind"), ShrubWindDefault,
    TEXT("Sway of the estate's brambles, hedges, deer brush and thimbleberry as a share of their authored ")
    TEXT("wind (1 = as authored, 0 = still). The sway is tip-weighted (height squared)."));
TAutoConsoleVariable<int32> CVarShrubShadows(TEXT("homestead.ShrubShadows"), 1,
    TEXT("1 = those shrubs cast shadows (default); 0 = they don't (A/B for shadow motion)."));
TAutoConsoleVariable<int32> CVarFoliageDither(TEXT("homestead.FoliageDither"), 1,
    TEXT("1 = the camera-safe dither fades plants between the camera and her (default); 0 = off (A/B)."));
// Names are made on use, not during static initialisation.
FName SwayingShrubTag() { return FName(TEXT("SwayingShrub")); }
FName WindStrength() { return FName(TEXT("WindStrength")); }
FName LeafFlutter() { return FName(TEXT("LeafFlutter")); }
// MPC_CameraSafeFoliage's radii (Scripts/Environment/create_camera_safe_foliage.py): zero turns the dither off.
constexpr const TCHAR* DitherRadii[] = {TEXT("NearRadiusCm"), TEXT("CorridorRadiusCm")};

bool IsSwayingShrub(const UStaticMesh* Mesh)
{
    static const FName Names[] = {TEXT("SM_BlackberryBramble"), TEXT("SM_BlackberryBrambleLarge"), TEXT("SM_ToyonHedge"),
        TEXT("SM_DeerBrush"), TEXT("SM_Thimbleberry")};
    if (!Mesh) return false;
    for (const FName& Name : Names)
        if (Mesh->GetFName() == Name) return true;
    return false;
}
}

void AHomesteadWorld::TagSwayingShrub(UMeshComponent& Component)
{
    using namespace HomesteadFoliageMotion;
    const UStaticMeshComponent* Static = Cast<UStaticMeshComponent>(&Component);
    if (!Static || !IsSwayingShrub(Static->GetStaticMesh())) return;
    Component.ComponentTags.AddUnique(SwayingShrubTag());
    SwayingShrubs.Add(&Component);
    // One dynamic instance per authored material, shared by every shrub that uses it.
    for (int32 Slot = 0; Slot < Component.GetNumMaterials(); ++Slot)
    {
        UMaterialInterface* Material = Component.GetMaterial(Slot);
        if (!Material) continue;
        if (UMaterialInstanceDynamic* Existing = Cast<UMaterialInstanceDynamic>(Material);
            Existing && ShrubWindBase.Contains(TWeakObjectPtr<UMaterialInstanceDynamic>(Existing)))
            continue;
        TObjectPtr<UMaterialInstanceDynamic>& Dynamic = ShrubWindMaterials.FindOrAdd(Material);
        if (!Dynamic)
        {
            float Strength = 0.0f, Flutter = 0.0f;
            const bool bStrength = Material->GetScalarParameterValue(FHashedMaterialParameterInfo(WindStrength()), Strength);
            const bool bFlutter = Material->GetScalarParameterValue(FHashedMaterialParameterInfo(LeafFlutter()), Flutter);
            if (!bStrength && !bFlutter)
            {
                ShrubWindMaterials.Remove(Material);
                continue;
            }
            Dynamic = UMaterialInstanceDynamic::Create(Material, this);
            ShrubWindBase.Add(TWeakObjectPtr<UMaterialInstanceDynamic>(Dynamic.Get()), FVector2f(Strength, Flutter));
            const float Scale = AppliedShrubWind >= 0.0f ? AppliedShrubWind : ShrubWindDefault;
            Dynamic->SetScalarParameterValue(WindStrength(), Strength * Scale);
            Dynamic->SetScalarParameterValue(LeafFlutter(), Flutter * Scale);
        }
        Component.SetMaterial(Slot, Dynamic);
    }
    if (AppliedShrubShadows == 0) Component.SetCastShadow(false);
    if (AppliedShrubWind == 0.0f) Component.SetEvaluateWorldPositionOffset(false);
}

void AHomesteadWorld::UpdateFoliageMotion()
{
    using namespace HomesteadFoliageMotion;
    const float Wind = FMath::Max(0.0f, CVarShrubWind.GetValueOnGameThread());
    const int32 Shadows = CVarShrubShadows.GetValueOnGameThread() != 0 ? 1 : 0;
    const int32 Dither = CVarFoliageDither.GetValueOnGameThread() != 0 ? 1 : 0;
    if (Wind == AppliedShrubWind && Shadows == AppliedShrubShadows && Dither == AppliedFoliageDither) return;

    if (Wind != AppliedShrubWind)
        for (const TPair<TWeakObjectPtr<UMaterialInstanceDynamic>, FVector2f>& Entry : ShrubWindBase)
            if (UMaterialInstanceDynamic* Dynamic = Entry.Key.Get())
            {
                Dynamic->SetScalarParameterValue(WindStrength(), Entry.Value.X * Wind);
                Dynamic->SetScalarParameterValue(LeafFlutter(), Entry.Value.Y * Wind);
            }
    if (Wind != AppliedShrubWind || Shadows != AppliedShrubShadows)
    {
        SwayingShrubs.RemoveAll([](const TWeakObjectPtr<UMeshComponent>& Shrub) { return !Shrub.IsValid(); });
        for (const TWeakObjectPtr<UMeshComponent>& Shrub : SwayingShrubs)
        {
            Shrub->SetCastShadow(Shadows != 0);
            Shrub->SetEvaluateWorldPositionOffset(Wind > 0.0f);
        }
    }
    if (Dither != AppliedFoliageDither && GetWorld())
    {
        if (!CameraFoliageCollection)
            CameraFoliageCollection = LoadObject<UMaterialParameterCollection>(nullptr,
                TEXT("/Game/SurvivalGame/Environment/CameraSafeFoliage/MPC_CameraSafeFoliage.MPC_CameraSafeFoliage"),
                nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (CameraFoliageCollection)
            if (UMaterialParameterCollectionInstance* Instance = GetWorld()->GetParameterCollectionInstance(CameraFoliageCollection))
                for (const TCHAR* Radius : DitherRadii)
                    if (const FCollectionScalarParameter* Parameter = CameraFoliageCollection->GetScalarParameterByName(FName(Radius)))
                        Instance->SetScalarParameterValue(FName(Radius), Dither ? Parameter->DefaultValue : 0.0f);
    }
    if (AppliedShrubWind >= 0.0f)
        UE_LOG(LogHomesteadWorld, Display, TEXT("Foliage motion: shrub wind %.2f, shrub shadows %d, dither %d (%d shrubs)"),
            Wind, Shadows, Dither, SwayingShrubs.Num());
    AppliedShrubWind = Wind;
    AppliedShrubShadows = Shadows;
    AppliedFoliageDither = Dither;
}
