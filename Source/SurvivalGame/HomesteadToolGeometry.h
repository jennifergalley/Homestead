#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

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

inline bool UpdateHandToolGrip(USceneComponent& Tool, const USkeletalMeshComponent& Mesh, FRotator Rotation)
{
    const auto* Asset = Mesh.GetSkeletalMeshAsset();
    if (!Asset) { Tool.SetHiddenInGame(true); UE_LOG(LogTemp, Error, TEXT("Contextual tool requires a skeletal mesh.")); return false; }
    const auto& Skeleton = Asset->GetRefSkeleton();
    const int32 Finger = Skeleton.FindBoneIndex(TEXT("middle_01_r"));
    const int32 Hand = Skeleton.FindBoneIndex(TEXT("hand_r"));
    if (Finger == INDEX_NONE || Hand == INDEX_NONE || Skeleton.GetParentIndex(Finger) != Hand)
    {
        Tool.SetHiddenInGame(true);
        UE_LOG(LogTemp, Error, TEXT("Contextual tool grip requires the shared hand/middle-finger bind."));
        return false;
    }
    Tool.SetRelativeLocation(Skeleton.GetRefBonePose()[Finger].GetTranslation() * 0.6f);
    Tool.SetWorldRotation(Rotation);
    return true;
}
