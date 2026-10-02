#include "HomesteadSmokeTest.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadKnife.h"
#include "HomesteadWorld.h"
#include "HomesteadTestPaths.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include <cfloat>

void AHomesteadSmokeTest::PrepareCreekChecks()
{
    const bool HeroineSequence = FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineSequence"));
    if (HeroineSequence)
    {
        if (!FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineTrialVitruvian01"))
            && !FParse::Param(FCommandLine::Get(), TEXT("HomesteadTrialCMULevelHead")))
        {
            Finish(false, TEXT("The normal heroine sequence requires an explicit licensed gait trial."));
            return;
        }
        auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        FHomesteadAppearance Look = Controller->GetAppearance();
        Look.BodyPreset = 0;
        Look.HairStyle = 1;
        FString Error;
        if (!Avatar || !Avatar->PrepareEquipment(Controller->State(), Look, Error)
            || !Avatar->ApplyPreparedEquipment(Error))
        {
            Finish(false, TEXT("The normal heroine sequence could not prepare the clothed Bob: ") + Error);
            return;
        }
        Controller->Appearance = Look;
        Results.Add(TEXT("HEROINE_SEQUENCE appearance=Preferred/Bob; ordinary game inputs, no world teleport or inventory injection; recipe check uses an isolated clone"));
    }
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
    Add(TEXT("Natural creek uses one noncolliding shadowless water surface over muddy terrain"),
        []() {}, [this]()
        {
            const auto* Landscape = Controller->Landscape.Get();
            const auto* Chunk = Landscape
                ? Landscape->TerrainChunks.Find(FIntPoint(0, 0)) : nullptr;
            auto* Terrain = Chunk ? Chunk->Terrain.Get() : nullptr;
            auto* Surface = Chunk ? Chunk->Water.Get() : nullptr;
            const auto* Ground = Terrain ? Terrain->GetProcMeshSection(0) : nullptr;
            const auto* Water = Surface ? Surface->GetProcMeshSection(0) : nullptr;
            constexpr int32 Columns = AHomesteadWorld::CreekSurfaceColumns + 1;
            constexpr int32 Rows = Homestead::Generation::TerrainCellsPerChunk * 2 + 1;
            if (!Ground || !Water || Terrain->GetProcMeshSection(1) || Surface->GetProcMeshSection(1)
                || Water->bEnableCollision || Surface->IsCollisionEnabled() || Surface->CastShadow
                || !GetPathNameSafe(Surface->GetMaterial(0)).Contains(TEXT("M_CreekWater"))
                || Water->ProcVertexBuffer.Num() != Rows * Columns || Water->ProcIndexBuffer.IsEmpty())
            {
                UE_LOG(LogTemp, Warning, TEXT("CREEK_SURFACE ground=%d water=%d material=%s vertices=%d"),
                    Ground != nullptr, Water != nullptr, *GetPathNameSafe(Surface ? Surface->GetMaterial(0) : nullptr),
                    Water ? Water->ProcVertexBuffer.Num() : -1);
                return false;
            }
            // The visible shoreline is where the surface stands over the bed (vertex G > 0). Wet
            // vertices sit on a 20 cm column grid, so the true edge lies between the outermost wet
            // vertex and the next column out; the 6 cm drop pulls it a little up the bank.
            constexpr double Spacing = AHomesteadWorld::CreekSurfaceHalfSpanCm * 2.0 / AHomesteadWorld::CreekSurfaceColumns;
            double MinimumWidth = DBL_MAX;
            double MaximumWidth = -DBL_MAX;
            bool Asymmetric = false;
            for (int32 Row = 0; Row < Rows; ++Row)
            {
                const double Center = Homestead::StreamX(Water->ProcVertexBuffer[Row * Columns].Position.Y);
                double Left = 0;
                double Right = 0;
                for (int32 Column = 0; Column < Columns; ++Column)
                {
                    const auto& Vertex = Water->ProcVertexBuffer[Row * Columns + Column];
                    if (Vertex.Color.G == 0) continue;
                    Left = FMath::Max(Left, Center - Vertex.Position.X);
                    Right = FMath::Max(Right, Vertex.Position.X - Center);
                }
                if (Left + Spacing < Homestead::Generation::CreekWaterMinimumHalfWidthCm - 10
                    || Right + Spacing < Homestead::Generation::CreekWaterMinimumHalfWidthCm - 10
                    || Left > Homestead::Generation::CreekWaterMaximumHalfWidthCm + Spacing
                    || Right > Homestead::Generation::CreekWaterMaximumHalfWidthCm + Spacing)
                {
                    UE_LOG(LogTemp, Warning, TEXT("CREEK_SHORE row=%d left=%.1f right=%.1f"), Row, Left, Right);
                    return false;
                }
                MinimumWidth = FMath::Min(MinimumWidth, FMath::Min(Left, Right));
                MaximumWidth = FMath::Max(MaximumWidth, FMath::Max(Left, Right));
                Asymmetric |= !FMath::IsNearlyEqual(Left, Right, 0.01);
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
                // Boulders and large shrubs block on purpose; the bank's reeds and grass must not.
                if (!Batch || Batch->ComponentHasTag(TEXT("HomesteadBlocking"))) continue;
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
            const float X = Controller->GetPawn()->GetActorLocation().X;
            return X >= 1050 && X <= 1300;
        }, 12.0f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1.0f); };
    Add(TEXT("Release approach movement"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this]() { return Controller->GetPawn()->GetVelocity().Size2D() < 5; }, 0.4f);
    Add(TEXT("Capture ordinary creek approach"),
        [this]() { Screenshot(TEXT("creek-approach")); }, []() { return true; }, 0.8f);
    Add(TEXT("Walk to the naturalized creek bank"),
        []() {}, [this]()
        {
            const float X = Controller->GetPawn()->GetActorLocation().X;
            return X >= 1340 && X <= 1570;
        }, 1.7f);
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
    const auto SprintStartEnergy = MakeShared<double>(0);
    if (HeroineSequence)
    {
        Add(TEXT("Press mapped L3 (sprint on) after ordinary walking"),
            [this, SprintStartEnergy]()
            {
                *SprintStartEnergy = Controller->State().energy;
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    EKeys::Gamepad_LeftThumbstick, IE_Pressed, 1));
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    EKeys::Gamepad_LeftThumbstick, IE_Released, 0));
            }, []() { return true; });
    }
    Add(TEXT("Cross the former bank ribbons on colliding terrain"),
        []() {}, [this]()
        {
            const float X = Controller->GetPawn()->GetActorLocation().X;
            return X >= 1720 && X <= 1950;
        }, HeroineSequence ? 1.35f : 2.25f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1.0f); };
    Add(TEXT("Release crossing movement"),
        [this, HeroineSequence]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            if (HeroineSequence)
            {
                // Sprint is a toggle: a second L3 press turns it off.
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    EKeys::Gamepad_LeftThumbstick, IE_Pressed, 1));
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    EKeys::Gamepad_LeftThumbstick, IE_Released, 0));
            }
        },
        [this, HeroineSequence, SprintStartEnergy]()
        {
            const FVector Position = Controller->GetPawn()->GetActorLocation();
            if (HeroineSequence)
                Results.Add(FString::Printf(TEXT("HEROINE_SEQUENCE crossing_x=%.2f sprint_energy_spent=%.3f"),
                    Position.X, *SprintStartEnergy - Controller->State().energy));
            return Position.X >= 1720 && Controller->GetPawn()->GetVelocity().Size2D() < 5
                && FMath::IsNearlyEqual(Position.Z, Controller->GroundHeight(Position.X, Position.Y), 100)
                // Sprint is free: only the slow awake drain (0.6 an hour) passes during the crossing.
                && (!HeroineSequence || (*SprintStartEnergy - Controller->State().energy >= 0.0
                    && *SprintStartEnergy - Controller->State().energy < .15));
        }, 0.5f);
    Add(TEXT("Capture completed creek crossing"),
        [this]() { Screenshot(TEXT("creek-crossed")); }, []() { return true; }, 0.8f);

    // The knife clear and reed-to-fibre gathering that followed the crossing were retired with the
    // knife and fibre (add-overgrown-estate-clearing); reeds now stand as scenery.
}

FString AHomesteadSmokeTest::DescribeHeroineAction(const TCHAR* Label) const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    const auto* Mesh = Avatar ? Avatar->GetMesh() : nullptr;
    const auto* Animation = Mesh ? Cast<UHomesteadAnimInstance>(Mesh->GetAnimInstance()) : nullptr;
    const auto* Knife = Avatar ? Avatar->GetKnife() : nullptr;
    if (!Mesh || !Animation || !Knife) return {};
    const FVector LeftToe = Mesh->GetBoneLocation(TEXT("ball_l"));
    const FVector RightToe = Mesh->GetBoneLocation(TEXT("ball_r"));
    const FVector Hand = Mesh->GetBoneLocation(TEXT("hand_r"));
    const double LeftGround = Controller->GroundHeight(LeftToe.X, LeftToe.Y);
    const double RightGround = Controller->GroundHeight(RightToe.X, RightToe.Y);
    if (!FMath::IsFinite(LeftGround) || !FMath::IsFinite(RightGround)) return {};
    return FString::Printf(
        TEXT("HEROINE_SEQUENCE action=%s cut_phase=%.3f cut_weight=%.3f hand_to_knife_cm=%.2f left_toe_ground_cm=%.2f right_toe_ground_cm=%.2f camera_distance_cm=%.2f energy=%.3f held_knife=%d"),
        Label, Animation->KnifeCutPhase(), Animation->KnifeCutWeight(),
        FVector::Dist(Hand, Knife->GetComponentLocation()),
        LeftToe.Z - LeftGround, RightToe.Z - RightGround,
        Avatar->CameraDistance(), Controller->State().energy,
        static_cast<int32>(Knife->IsPresented()));
}
