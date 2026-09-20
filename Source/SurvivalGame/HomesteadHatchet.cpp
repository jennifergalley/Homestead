#include "HomesteadHatchet.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadToolGeometry.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

UHomesteadHatchet::UHomesteadHatchet(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    SetHiddenInGame(true);
    SetAbsolute(false, true, true);
}

void UHomesteadHatchet::BeginPlay()
{
    Super::BeginPlay();
    if (const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner())) AddTickPrerequisiteComponent(Avatar->GetMesh());
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
    if (!Base) { UE_LOG(LogTemp, Error, TEXT("Hatchet requires the existing field material.")); return; }
    FToolSurface Wood, Stone, Fiber;
    Wood.Lathe({{0.01f, -7}, {1.2f, -7}, {1.4f, 0}, {1.1f, 23}, {0.01f, 23}});
    const FVector A(-4, -2.8, 18), B(-4, 2.8, 18), C(-4, 2.8, 26), D(-4, -2.8, 26);
    const FVector E(14, -0.4, 20), F(14, 0.4, 20), G(14, 0.4, 24), H(14, -0.4, 24);
    Stone.Quad(A, D, C, B); Stone.Quad(E, F, G, H);
    Stone.Quad(A, B, F, E); Stone.Quad(D, H, G, C);
    Stone.Quad(A, E, H, D); Stone.Quad(B, C, G, F);
    for (const float X : {0.5f, 2.5f})
        Fiber.Tube({{X, -3.3, 17}, {X, 3.3, 17}, {X, 3.3, 26.5},
            {X, -3.3, 26.5}, {X, -3.3, 17}}, 0.5f);
    const TArray<FToolSurface*> Surfaces{&Wood, &Stone, &Fiber};
    const FLinearColor Colors[]{FLinearColor(0.27f, 0.145f, 0.065f), FLinearColor(0.25f, 0.28f, 0.24f), FLinearColor(0.46f, 0.33f, 0.17f)};
    for (int32 Index = 0; Index < Surfaces.Num(); ++Index)
    {
        auto& S = *Surfaces[Index];
        TArray<FVector> Normals;
        TArray<FProcMeshTangent> Tangents;
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(S.Vertices, S.Triangles, S.UV, Normals, Tangents);
        CreateMeshSection_LinearColor(Index, S.Vertices, S.Triangles, Normals, S.UV, {}, Tangents, false);
        auto* Material = UMaterialInstanceDynamic::Create(Base, this);
        Material->SetVectorParameterValue(TEXT("Tint"), Colors[Index]);
        Material->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
        SetMaterial(Index, Material);
    }
}

float UHomesteadHatchet::SwingAngle(float Phase)
{
    return 60 * FMath::SmoothStep(0.45f, 0.85f, Phase) * (1 - FMath::SmoothStep(1.15f, 1.8f, Phase));
}

void UHomesteadHatchet::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner());
    const auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const float Phase = Animation ? Animation->ClearPhase() : 0;
    const bool Visible = PC && Animation && Animation->IsClearing() && Animation->ClearWeight() > 0.5f
        && Phase >= 0.32f && Phase <= 1.55f && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
        && PC->Simulation().Count(Homestead::Item::Hatchet) > 0 && GetNumSections() == 3;
    SetHiddenInGame(!Visible);
    if (Visible)
        UpdateHandToolGrip(*this, *Avatar->GetMesh(), FRotator(-SwingAngle(Phase), Avatar->GetActorRotation().Yaw, 0));
}
