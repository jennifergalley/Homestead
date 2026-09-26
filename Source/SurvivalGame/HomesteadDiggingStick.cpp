#include "HomesteadDiggingStick.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadToolGeometry.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

UHomesteadDiggingStick::UHomesteadDiggingStick(const FObjectInitializer& ObjectInitializer)
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

void UHomesteadDiggingStick::BeginPlay()
{
    Super::BeginPlay();
    if (const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner()))
        AddTickPrerequisiteComponent(Avatar->GetMesh());
    auto* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
    if (!Base)
    {
        UE_LOG(LogTemp, Error, TEXT("Digging stick requires the existing field material."));
        return;
    }
    FToolSurface Surface;
    Surface.Lathe({{0.65f, -42}, {0.9f, -39}, {1.15f, 36}, {0.45f, 45}});
    TArray<FVector> Normals;
    TArray<FProcMeshTangent> Tangents;
    UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
        Surface.Vertices, Surface.Triangles, Surface.UV, Normals, Tangents);
    CreateMeshSection_LinearColor(0, Surface.Vertices, Surface.Triangles,
        Normals, Surface.UV, {}, Tangents, false);
    auto* Material = UMaterialInstanceDynamic::Create(Base, this);
    Material->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.24f, 0.115f, 0.045f));
    Material->SetScalarParameterValue(TEXT("Roughness"), 0.92f);
    SetMaterial(0, Material);
}

void UHomesteadDiggingStick::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner());
    const auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
    const auto* Animation = Avatar
        ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const float Phase = Animation ? Animation->TillPhase() : 0;
    // The MetaHuman tills with the held stone hoe prop instead.
    const bool Visible = PC && Animation && Animation->IsTilling() && !Avatar->UsesHoeTill()
        && Animation->TillWeight() > 0.5f && Phase >= 0.22f && Phase <= 1.48f
        && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
        && PC->Simulation().Count(Homestead::Item::DiggingStick) > 0
        && GetNumSections() == 1;
    SetHiddenInGame(!Visible);
    if (Visible)
        UpdateHandToolGrip(*this, *Avatar->GetMesh(), FRotator(-18, Avatar->TillTargetYaw(), 0));
}
