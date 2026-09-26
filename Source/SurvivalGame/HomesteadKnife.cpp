#include "HomesteadKnife.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadToolGeometry.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

UHomesteadKnife::UHomesteadKnife(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    SetHiddenInGame(true);
    SetAbsolute(false, true, true);
}

void UHomesteadKnife::BeginPlay()
{
    Super::BeginPlay();
    if (const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner()))
        AddTickPrerequisiteComponent(Avatar->GetMesh());
    auto* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
    if (!Base)
    {
        UE_LOG(LogTemp, Error, TEXT("Knife requires the existing field material."));
        return;
    }
    FToolSurface Handle, Blade, Binding;
    Handle.Lathe({{0.7f, -5}, {1.2f, -4.5f}, {1.3f, -2}, {1.2f, 2},
        {0.9f, 4}, {0.4f, 4.2f}});
    const FVector A(-0.6, -0.8, 4), B(0.6, -0.8, 4);
    const FVector C(1.0, -0.55, 11), D(0.4, -0.1, 15);
    const FVector E(-0.2, 0.1, 15), F(-1.2, -0.55, 10);
    const FVector G(0, 0.55, 6);
    Blade.Quad(A, B, C, F);
    Blade.Quad(F, C, D, E);
    Blade.Quad(B, G, D, C);
    Blade.Quad(A, F, E, G);
    Blade.Quad(A, F, C, B);
    Binding.Lathe({{1.38f, 3.4f}, {1.38f, 4.1f}});
    const TArray<FToolSurface*> Surfaces{&Handle, &Blade, &Binding};
    const FLinearColor Colors[]{
        FLinearColor(0.24f, 0.11f, 0.05f),
        FLinearColor(0.55f, 0.58f, 0.56f),
        FLinearColor(0.34f, 0.25f, 0.12f)
    };
    for (int32 Index = 0; Index < Surfaces.Num(); ++Index)
    {
        const auto& Surface = *Surfaces[Index];
        TArray<FVector> Normals;
        TArray<FProcMeshTangent> Tangents;
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
            Surface.Vertices, Surface.Triangles, Surface.UV, Normals, Tangents);
        CreateMeshSection_LinearColor(Index, Surface.Vertices, Surface.Triangles,
            Normals, Surface.UV, {}, Tangents, false);
        auto* Material = UMaterialInstanceDynamic::Create(Base, this);
        Material->SetVectorParameterValue(TEXT("Tint"), Colors[Index]);
        Material->SetScalarParameterValue(TEXT("Roughness"), Index == 1 ? 0.43f : 0.86f);
        SetMaterial(Index, Material);
    }
}

void UHomesteadKnife::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner());
    const auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
    const auto* Animation = Avatar
        ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const bool Visible = Avatar && PC && Animation && Avatar->HasHeroine()
        && PC->Simulation().Count(Homestead::Item::Knife) > 0
        && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
        && (Animation->KnifeCutWeight() > 0.01f
            || (PC->KnifePreviewRequested() && Animation->ActionWeight() < 0.01f
                && !Avatar->GetHeldProp(Homestead::Item::Knife)))
        && GetNumSections() == 3;
    SetHiddenInGame(!Visible);
    if (Visible)
        UpdateHandToolGrip(*this, *Avatar->GetMesh(), FRotator(0, Avatar->GetActorRotation().Yaw, 0));
}
