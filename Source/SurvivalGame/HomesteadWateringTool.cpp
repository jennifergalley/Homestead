#include "HomesteadWateringTool.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace
{
struct FToolSurface
{
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector2D> UV;

    void Quad(FVector A, FVector B, FVector C, FVector D)
    {
        const int32 I = Vertices.Num();
        Vertices.Append({A, B, C, D});
        Triangles.Append({I, I + 1, I + 2, I, I + 2, I + 3});
        UV.Append({{0, 0}, {1, 0}, {1, 1}, {0, 1}});
    }

    void Lathe(const TArray<FVector2D>& Profile)
    {
        for (int32 Ring = 1; Ring < Profile.Num(); ++Ring)
            for (int32 Side = 0; Side < 20; ++Side)
            {
                const float A = Side * 2 * PI / 20, B = (Side + 1) * 2 * PI / 20;
                auto At = [](FVector2D P, float Angle) { return FVector(P.X * FMath::Cos(Angle), P.X * FMath::Sin(Angle), P.Y); };
                Quad(At(Profile[Ring - 1], A), At(Profile[Ring - 1], B), At(Profile[Ring], B), At(Profile[Ring], A));
            }
    }

    void Tube(const TArray<FVector>& Path, float Radius)
    {
        for (int32 Segment = 1; Segment < Path.Num(); ++Segment)
        {
            const FVector Direction = (Path[Segment] - Path[Segment - 1]).GetSafeNormal();
            const FVector U = FVector::CrossProduct(Direction, FMath::Abs(Direction.X) < 0.9f ? FVector::ForwardVector : FVector::RightVector).GetSafeNormal();
            const FVector V = FVector::CrossProduct(Direction, U);
            for (int32 Side = 0; Side < 10; ++Side)
            {
                auto Offset = [&](int32 I) { const float A = I * 2 * PI / 10; return Radius * (U * FMath::Cos(A) + V * FMath::Sin(A)); };
                Quad(Path[Segment - 1] + Offset(Side), Path[Segment - 1] + Offset(Side + 1),
                    Path[Segment] + Offset(Side + 1), Path[Segment] + Offset(Side));
            }
        }
    }
};
}

UHomesteadWateringTool::UHomesteadWateringTool(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetGenerateOverlapEvents(false);
    SetCanEverAffectNavigation(false);
    SetHiddenInGame(true);
    // FBX hand transforms carry unit-conversion scale; prop vertices are already centimeters.
    SetAbsolute(false, true, true);
}

void UHomesteadWateringTool::BeginPlay()
{
    Super::BeginPlay();
    const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner());
    if (Avatar) AddTickPrerequisiteComponent(Avatar->GetMesh());
    auto* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/SurvivalGame/Materials/M_Field.M_Field"));
    if (!Base)
    {
        UE_LOG(LogTemp, Error, TEXT("Watering tool requires the existing field material."));
        return;
    }
    FToolSurface Wood, Fiber;
    Wood.Lathe({{0.01f, -32}, {8, -32}, {10, -12}, {9, -12}, {7, -30}, {0.01f, -30}});
    Wood.Tube({{8, 0, -23}, {19, 0, -19}, {30, 0, -14}}, 2.2f);
    Wood.Tube({{30, 0, -14}, {19, 0, -19}, {8, 0, -23}}, 1.4f);
    Fiber.Lathe({{8.2f, -29}, {8.7f, -29}, {8.9f, -27}, {8.4f, -27}});
    Fiber.Lathe({{9.5f, -16}, {10.1f, -16}, {10.3f, -14}, {9.7f, -14}});
    Fiber.Tube({{0, -10, -14}, {0, -11, -7}, {0, -9, -2}, {0, -6, 0},
        {0, 6, 0}, {0, 9, -2}, {0, 11, -7}, {0, 10, -14}}, 0.9f);
    const TArray<FToolSurface*> Surfaces{&Wood, &Fiber};
    for (int32 Index = 0; Index < Surfaces.Num(); ++Index)
    {
        auto& S = *Surfaces[Index];
        TArray<FVector> Normals;
        TArray<FProcMeshTangent> Tangents;
        UKismetProceduralMeshLibrary::CalculateTangentsForMesh(S.Vertices, S.Triangles, S.UV, Normals, Tangents);
        CreateMeshSection_LinearColor(Index, S.Vertices, S.Triangles, Normals, S.UV, {}, Tangents, false);
        auto* Material = UMaterialInstanceDynamic::Create(Base, this);
        Material->SetVectorParameterValue(TEXT("Tint"), Index == 0 ? FLinearColor(0.27f, 0.145f, 0.065f) : FLinearColor(0.46f, 0.33f, 0.17f));
        Material->SetScalarParameterValue(TEXT("Roughness"), 0.85f);
        SetMaterial(Index, Material);
    }
}

float UHomesteadWateringTool::PourAngle(float Phase)
{
    return 32.0f * FMath::SmoothStep(0.45f, 0.75f, Phase) * (1 - FMath::SmoothStep(1.25f, 1.5f, Phase));
}

void UHomesteadWateringTool::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    const auto* Avatar = Cast<AHomesteadCharacter>(GetOwner());
    const auto* PC = Avatar ? Cast<AHomesteadController>(Avatar->GetController()) : nullptr;
    const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    const float Phase = Animation ? Animation->WaterPhase() : 0;
    const bool Visible = PC && Animation && Animation->IsWatering() && Animation->WaterWeight() > 0.5f
        && Phase >= 0.32f && Phase <= 1.65f && !PC->IsBookOpen() && !PC->IsPlanning() && !PC->IsFailed()
        && PC->Simulation().Count(Homestead::Item::WateringCan) > 0 && GetNumSections() == 2;
    SetHiddenInGame(!Visible);
    if (!Visible) return;
    const auto& Skeleton = Avatar->GetMesh()->GetSkeletalMeshAsset()->GetRefSkeleton();
    const int32 Finger = Skeleton.FindBoneIndex(TEXT("middle_01_r"));
    const int32 Hand = Skeleton.FindBoneIndex(TEXT("hand_r"));
    if (Finger == INDEX_NONE || Hand == INDEX_NONE || Skeleton.GetParentIndex(Finger) != Hand)
    {
        SetHiddenInGame(true);
        UE_LOG(LogTemp, Error, TEXT("Watering tool grip requires the shared hand/middle-finger bind."));
        return;
    }
    SetRelativeLocation(Skeleton.GetRefBonePose()[Finger].GetTranslation() * 0.6f);
    // The authored wrist uses the same tilt; world-up keeps the suspended can upright during the lift.
    SetWorldRotation(FRotator(-PourAngle(Phase), Avatar->GetActorRotation().Yaw, 0));
}
