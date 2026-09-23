#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadWorld.h"
#include "HomesteadTestPaths.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include <cfloat>

void AHomesteadSmokeTest::PrepareCreekChecks()
{
    for (const TCHAR* Other : {TEXT("HomesteadGeneratedWoodland"),
        TEXT("HomesteadDirectionalNavigationTest"), TEXT("HomesteadNativeMenuTest"),
        TEXT("HomesteadFullLoop"), TEXT("HomesteadWateringTest"),
        TEXT("HomesteadVisualPlaytest")})
    {
        if (FParse::Param(FCommandLine::Get(), Other))
        {
            Finish(false, TEXT("Creek presentation requires its own isolated route."));
            return;
        }
    }
    Add(TEXT("Natural creek uses one noncolliding varied water section over muddy terrain"),
        []() {}, [this]()
        {
            const auto* Landscape = Controller->Landscape.Get();
            const auto* Chunk = Landscape
                ? Landscape->TerrainChunks.Find(FIntPoint(0, 0)) : nullptr;
            auto* Terrain = Chunk ? Chunk->Terrain.Get() : nullptr;
            const auto* Ground = Terrain ? Terrain->GetProcMeshSection(0) : nullptr;
            const auto* Water = Terrain ? Terrain->GetProcMeshSection(1) : nullptr;
            if (!Ground || !Water || Terrain->GetProcMeshSection(2)
                || Terrain->GetProcMeshSection(3) || Water->bEnableCollision
                || Water->ProcVertexBuffer.Num() != 50
                || Water->ProcIndexBuffer.Num() != 144)
                return false;
            double MinimumWidth = DBL_MAX;
            double MaximumWidth = -DBL_MAX;
            bool Asymmetric = false;
            for (int32 Row = 0; Row < 25; ++Row)
            {
                const FVector Left(Water->ProcVertexBuffer[Row * 2].Position);
                const FVector Right(Water->ProcVertexBuffer[Row * 2 + 1].Position);
                const double Center = Homestead::StreamX(Left.Y);
                const double LeftWidth = Center - Left.X;
                const double RightWidth = Right.X - Center;
                if (LeftWidth < Homestead::Generation::CreekWaterMinimumHalfWidthCm - 0.1
                    || LeftWidth > Homestead::Generation::CreekWaterMaximumHalfWidthCm + 0.1
                    || RightWidth < Homestead::Generation::CreekWaterMinimumHalfWidthCm - 0.1
                    || RightWidth > Homestead::Generation::CreekWaterMaximumHalfWidthCm + 0.1)
                    return false;
                MinimumWidth = FMath::Min(MinimumWidth, FMath::Min(LeftWidth, RightWidth));
                MaximumWidth = FMath::Max(MaximumWidth, FMath::Max(LeftWidth, RightWidth));
                Asymmetric |= !FMath::IsNearlyEqual(LeftWidth, RightWidth, 0.01);
            }
            int32 MudVertices = 0;
            int32 ForestVertices = 0;
            for (const auto& Vertex : Ground->ProcVertexBuffer)
            {
                const double Distance = FMath::Abs(
                    Vertex.Position.X - Homestead::StreamX(Vertex.Position.Y));
                if (Distance < 90 && Vertex.Color.R <= 1) ++MudVertices;
                if (Distance > 400 && Vertex.Color.R >= 12) ++ForestVertices;
            }
            int32 BankCover = 0;
            bool Nonblocking = true;
            for (USceneComponent* Component : Chunk->Cover.Components)
            {
                const auto* Batch = Cast<UHierarchicalInstancedStaticMeshComponent>(Component);
                if (!Batch) continue;
                Nonblocking &= Batch->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                    && !Batch->CanEverAffectNavigation();
                for (int32 Index = 0; Index < Batch->GetInstanceCount(); ++Index)
                {
                    FTransform Transform;
                    if (!Batch->GetInstanceTransform(Index, Transform, true)) continue;
                    const FVector Position = Transform.GetLocation();
                    const double Distance = FMath::Abs(
                        Position.X - Homestead::StreamX(Position.Y));
                    if (Distance >= 95 && Distance < 220) ++BankCover;
                }
            }
            Results.Add(FString::Printf(TEXT("CREEK_RUNTIME min_half_width=%.3f max_half_width=%.3f asymmetric=%d mud_vertices=%d forest_vertices=%d bank_cover=%d nonblocking=%d"),
                MinimumWidth, MaximumWidth, Asymmetric, MudVertices, ForestVertices,
                BankCover, Nonblocking));
            return MaximumWidth - MinimumWidth >= 4.0 && Asymmetric
                && MudVertices > 0 && ForestVertices > 0 && BankCover > 0 && Nonblocking;
        });
    Add(TEXT("Close Notes before ordinary creek traversal"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Face east toward the creek from the ordinary spawn"),
        [this]() { Controller->SetControlRotation(FRotator(-8, 0, 0)); },
        [this]() { return FMath::Abs(Controller->GetControlRotation().Yaw) < 0.1; });
    Add(TEXT("Walk from spawn to an ordinary creek approach"),
        []() {}, [this]()
        {
            return Controller->GetPawn()->GetActorLocation().X >= 1050;
        }, 18.0f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1.0f); };
    Add(TEXT("Release approach movement"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this]() { return Controller->GetPawn()->GetVelocity().Size2D() < 5; }, 0.4f);
    Add(TEXT("Capture ordinary creek approach"),
        [this]() { Screenshot(TEXT("creek-approach")); }, []() { return true; }, 0.8f);
    Add(TEXT("Walk to the naturalized creek bank"),
        []() {}, [this]()
        {
            return Controller->GetPawn()->GetActorLocation().X >= 1340;
        }, 4.0f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1.0f); };
    Add(TEXT("Release bank movement"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this]() { return Controller->GetPawn()->GetVelocity().Size2D() < 5; }, 0.4f);
    Add(TEXT("Capture naturalized bank at gameplay distance"),
        [this]() { Screenshot(TEXT("creek-bank")); }, []() { return true; }, 0.8f);
    Add(TEXT("Orbit along the creek with mapped camera input"),
        []() {}, [this]()
        {
            return Controller->GetControlRotation().Yaw >= 65;
        }, 1.4f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_RightX, 0.8f); };
    Add(TEXT("Release camera orbit and capture along-creek continuity"),
        [this]() { Axis(EKeys::Gamepad_RightX, 0); Screenshot(TEXT("creek-along")); },
        []() { return true; }, 0.8f);
    Add(TEXT("Face across the creek before mapped crossing"),
        [this]() { Controller->SetControlRotation(FRotator(-8, 0, 0)); },
        [this]() { return FMath::Abs(Controller->GetControlRotation().Yaw) < 0.1; });
    Add(TEXT("Cross the former bank ribbons on colliding terrain"),
        []() {}, [this]()
        {
            return Controller->GetPawn()->GetActorLocation().X >= 1720;
        }, 4.0f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1.0f); };
    Add(TEXT("Release crossing movement"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this]()
        {
            const FVector Position = Controller->GetPawn()->GetActorLocation();
            return Position.X >= 1720 && Controller->GetPawn()->GetVelocity().Size2D() < 5
                && FMath::IsNearlyEqual(Position.Z, Controller->GroundHeight(Position.X, Position.Y), 100);
        }, 0.5f);
    Add(TEXT("Capture completed creek crossing"),
        [this]() { Screenshot(TEXT("creek-crossed")); }, []() { return true; }, 0.8f);
}
