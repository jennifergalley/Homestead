#include "HomesteadWorld.h"
#include "HomesteadWorldLog.h"
#include "HomesteadWorldLook.h"

#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace HomesteadWorldGeometry
{
FName MaterialKey(const FLinearColor& Color, float Roughness, float Glow)
{
    return FName(*FString::Printf(TEXT("Tint_%d_%d_%d_R%d_G%d"),
        FMath::RoundToInt(Color.R * 1000), FMath::RoundToInt(Color.G * 1000),
        FMath::RoundToInt(Color.B * 1000), FMath::RoundToInt(Roughness * 100),
        FMath::RoundToInt(Glow * 100)));
}
}

using HomesteadWorldGeometry::MaterialKey;
using HomesteadWorldLook::Stone;

UMaterialInterface* AHomesteadWorld::Material(FLinearColor Color, float Roughness, float Glow)
{
    const FName Key = MaterialKey(Color, Roughness, Glow);
    if (TObjectPtr<UMaterialInstanceDynamic>* Existing = Materials.Find(Key))
    {
        return Existing->Get();
    }
    const bool bTexturedRock = RockMaterial && Color.Equals(Stone);
    UMaterialInstanceDynamic* Instance = UMaterialInstanceDynamic::Create(
        bTexturedRock ? RockMaterial.Get() : FieldMaterial.Get(), this);
    if (!Instance)
    {
        return UMaterial::GetDefaultMaterial(MD_Surface);
    }
    Instance->SetVectorParameterValue(TEXT("Tint"), bTexturedRock ? FLinearColor::White : Color);
    Instance->SetScalarParameterValue(TEXT("Roughness"), Roughness);
    Instance->SetScalarParameterValue(TEXT("Glow"), Glow);
    Materials.Add(Key, Instance);
    return Instance;
}

UStaticMeshComponent* AHomesteadWorld::AddPart(FHomesteadWorldVisual& Visual, UStaticMesh* Mesh,
    const FVector& Position, const FVector& Size, FLinearColor Color, bool bCollision,
    const FRotator& Rotation, float Roughness, float Glow)
{
    if (!Mesh)
    {
        return nullptr;
    }
    UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this);
    Part->SetupAttachment(GetRootComponent());
    Part->SetMobility(EComponentMobility::Movable);
    Part->SetStaticMesh(Mesh);
    Part->SetMaterial(0, Material(Color, Roughness, Glow));
    Part->SetRelativeTransform(FTransform(Rotation, Position, Size / 100.0f));
    Part->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
    Part->SetGenerateOverlapEvents(false);
    Part->SetCanEverAffectNavigation(bCollision);
    Part->SetCastShadow(Glow <= 0.0f);
    if (bStagingResourceBuild)
    {
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetVisibility(false);
        Part->SetHiddenInGame(true);
    }
    Part->RegisterComponent();
    Visual.Components.Add(Part);
    return Part;
}

void AHomesteadWorld::AddDecoration(UStaticMesh* Mesh, const FVector& Position, const FVector& Size,
    FLinearColor Color, bool bCollision, const FRotator& Rotation, bool bHideMesh)
{
    if (!Mesh)
    {
        return;
    }
    const FName Key(*FString::Printf(TEXT("%s_%s_%d_%d"), *Mesh->GetName(),
        *MaterialKey(Color, 0.85f, 0.0f).ToString(), bCollision, bHideMesh));
    UHierarchicalInstancedStaticMeshComponent* Batch = nullptr;
    if (TObjectPtr<UHierarchicalInstancedStaticMeshComponent>* Existing = DecorationBatches.Find(Key))
    {
        Batch = Existing->Get();
    }
    else
    {
        Batch = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        Batch->SetupAttachment(GetRootComponent());
        Batch->SetMobility(EComponentMobility::Static);
        Batch->SetStaticMesh(Mesh);
        for (int Slot = 0; Slot < FMath::Max(1, Mesh->GetStaticMaterials().Num()); ++Slot)
        {
            Batch->SetMaterial(Slot, Material(Color));
        }
        Batch->SetCollisionProfileName(bCollision ? UCollisionProfile::BlockAll_ProfileName : UCollisionProfile::NoCollision_ProfileName);
        Batch->SetGenerateOverlapEvents(false);
        Batch->SetCanEverAffectNavigation(bCollision);
        Batch->SetCullDistances(bCollision ? 0 : 4500, bCollision ? 0 : 7000);
        Batch->bAutoRebuildTreeOnInstanceChanges = false;
        Batch->SetVisibility(!bHideMesh);
        Batch->SetCastShadow(!bHideMesh);
        Batch->RegisterComponent();
        DecorationBatches.Add(Key, Batch);
    }
    // Imported FBX assets need not share the engine primitives' centered 100 cm bounds.
    const FBox Bounds = Mesh->GetBoundingBox();
    const FVector Dimensions = Bounds.GetSize().ComponentMax(FVector(0.01f));
    const FVector Scale = Size / Dimensions;
    const FVector Origin = Position - Rotation.RotateVector(Bounds.GetCenter() * Scale);
    Batch->AddInstance(FTransform(Rotation, Origin, Scale));
}
