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
        Add(TEXT("Hold mapped L3 after ordinary walking"),
            [this, SprintStartEnergy]()
            {
                *SprintStartEnergy = Controller->State().energy;
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    EKeys::Gamepad_LeftThumbstick, IE_Pressed, 1));
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
                Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                    EKeys::Gamepad_LeftThumbstick, IE_Released, 0));
        },
        [this, HeroineSequence, SprintStartEnergy]()
        {
            const FVector Position = Controller->GetPawn()->GetActorLocation();
            if (HeroineSequence)
                Results.Add(FString::Printf(TEXT("HEROINE_SEQUENCE crossing_x=%.2f sprint_energy_spent=%.3f"),
                    Position.X, *SprintStartEnergy - Controller->State().energy));
            return Position.X >= 1720 && Controller->GetPawn()->GetVelocity().Size2D() < 5
                && FMath::IsNearlyEqual(Position.Z, Controller->GroundHeight(Position.X, Position.Y), 100)
                && (!HeroineSequence || (*SprintStartEnergy - Controller->State().energy > .15
                    && *SprintStartEnergy - Controller->State().energy < 2.0));
        }, 0.5f);
    Add(TEXT("Capture completed creek crossing"),
        [this]() { Screenshot(TEXT("creek-crossed")); }, []() { return true; }, 0.8f);

    if (HeroineSequence)
    {
        const auto ClearId = MakeShared<int32>(-1);
        const auto ClearPoint = MakeShared<Homestead::Point>();
        const auto CutBefore = MakeShared<uint32>(0);
        Add(TEXT("Locate real reachable low growth after the mapped sprint"),
            [this, ClearId, ClearPoint, CutBefore]()
            {
                const auto Player = Controller->PlayerPoint();
                double Nearest = TNumericLimits<double>::Max();
                for (const auto& Node : Controller->State().resources)
                {
                    if (Node.kind != Homestead::ResourceKind::Flowers || Node.cleared) continue;
                    const double Distance = FMath::Square(Node.position.x - Player.x)
                        + FMath::Square(Node.position.y - Player.y);
                    if (Distance >= Nearest) continue;
                    Nearest = Distance;
                    *ClearId = Node.id;
                    *ClearPoint = Node.position;
                }
                if (*ClearId < 0 || Nearest > FMath::Square(700.0))
                {
                    Finish(false, TEXT("No naturally generated Knife-clearable flowers were reachable."));
                    return;
                }
                const FVector2D Delta(ClearPoint->x - Player.x, ClearPoint->y - Player.y);
                Controller->SetControlRotation(FRotator(-8,
                    FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0));
                const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                const auto* Animation = Avatar
                    ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
                if (!Animation) { Finish(false, TEXT("Held-Knife animation is missing.")); return; }
                *CutBefore = Animation->KnifeCutStarts();
            }, [ClearId]() { return *ClearId >= 0; });
        Add(TEXT("Walk to the actual low-growth patch with mapped movement"),
            []() {}, [this, ClearId, ClearPoint]()
            {
                const auto Player = Controller->PlayerPoint();
                return FVector2D::Distance(FVector2D(Player.x, Player.y),
                    FVector2D(ClearPoint->x, ClearPoint->y)) < 165
                    && Controller->IsResourceFocused(*ClearId);
            }, 6.0f);
        Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 0.8f); };
        Steps.Last().bCompleteWhenReady = true;
        Add(TEXT("Stop in front of low growth without sliding away"),
            [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
            [this, ClearId]()
            {
                return Controller->GetPawn()->GetVelocity().Size2D() < 5
                    && Controller->IsResourceFocused(*ClearId);
            }, 0.6f);
        Add(TEXT("Select only the carried starter Knife through slot one"),
            [this]() { Tap(EKeys::One); }, [this]()
            {
                const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                return Controller->SelectedHotbarSlot == 0
                    && Avatar && Avatar->GetKnife() && Avatar->GetKnife()->IsVisible();
            });
        Add(TEXT("Clear one real patch with mapped Knife use"),
            [this]() { Tap(EKeys::LeftMouseButton); },
            [this, ClearId, CutBefore]()
            {
                const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                const auto* Animation = Avatar
                    ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
                const auto& Nodes = Controller->State().resources;
                bool Cleared = false;
                for (const auto& Node : Nodes)
                    if (Node.id == *ClearId) { Cleared = Node.cleared; break; }
                return Cleared && Animation
                    && Animation->KnifeCutStarts() == *CutBefore + 1
                    && !Controller->ToastIsError();
            }, 0.3f);
        Add(TEXT("Capture the actual one-time Knife clear before Reed gathering"),
            [this]()
            {
                const FString Pose = DescribeHeroineAction(TEXT("clear"));
                if (Pose.IsEmpty()) { Finish(false, TEXT("The Knife-clear full-body pose was unavailable.")); return; }
                Results.Add(Pose);
                Screenshot(TEXT("heroine-sequence-clear"));
            },
            []() { return true; }, 0.8f);
    }

    const auto ReedId = MakeShared<int32>(-1);
    const auto ReedPosition = MakeShared<Homestead::Point>();
    const auto FiberBefore = MakeShared<int32>(0);
    Add(TEXT("Identify a seeded ready reed patch beside the real creek"),
        [this, ReedId, ReedPosition, FiberBefore]()
        {
            *FiberBefore = Controller->Simulation().Count(Homestead::Item::Fiber);
            const auto Player = Controller->PlayerPoint();
            double Nearest = TNumericLimits<double>::Max();
            for (const auto& Node : Controller->State().resources)
            {
                if (Node.kind != Homestead::ResourceKind::Reeds || Node.cleared
                    || Node.readyAtHour > Controller->State().hour) continue;
                const double Distance = FMath::Square(Node.position.x - Player.x)
                    + FMath::Square(Node.position.y - Player.y);
                if (Distance >= Nearest) continue;
                Nearest = Distance;
                *ReedId = Node.id;
                *ReedPosition = Node.position;
            }
            if (*ReedId < 0 || Nearest > FMath::Square(1000.0))
            {
                Finish(false, TEXT("No first-play reachable ready bank reeds were generated."));
                return;
            }
            const FVector2D Delta(ReedPosition->x - Player.x, ReedPosition->y - Player.y);
            Controller->SetControlRotation(FRotator(-8,
                FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0));
        },
        [ReedId]() { return *ReedId >= 0; });
    Add(TEXT("Approach real reeds with mapped movement, not a teleport"),
        []() {}, [this, ReedId, ReedPosition]()
        {
            const auto Player = Controller->PlayerPoint();
            const double Distance = FVector2D::Distance(
                FVector2D(Player.x, Player.y), FVector2D(ReedPosition->x, ReedPosition->y));
            return Distance < 165 && Controller->IsResourceFocused(*ReedId);
        }, HeroineSequence ? 6.0f : 1.55f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 0.8f); };
    if (HeroineSequence) Steps.Last().bCompleteWhenReady = true;
    Add(TEXT("Release movement at the reed bank"),
        [this]() { Axis(EKeys::Gamepad_LeftY, 0); },
        [this, ReedId]() { return Controller->GetPawn()->GetVelocity().Size2D() < 5
            && Controller->IsResourceFocused(*ReedId); }, 0.5f);
    Add(TEXT("Ready reed visual is a grounded noncolliding original clump"),
        []() {}, [this, ReedId]()
        {
            const auto* Visual = Controller->Landscape->ResourceProduceVisuals.Find(*ReedId);
            const auto* Component = Visual && Visual->Components.Num() == 1
                ? Cast<UStaticMeshComponent>(Visual->Components[0]) : nullptr;
            const UStaticMesh* Mesh = Component ? Component->GetStaticMesh() : nullptr;
            Results.Add(FString::Printf(TEXT("REED_VISUAL id=%d produce_parts=%d mesh=%s height_cm=%.1f collision=%d camera_ignored=%d"),
                *ReedId, Visual ? Visual->Components.Num() : -1, *GetPathNameSafe(Mesh),
                Mesh ? Mesh->GetBoundingBox().GetSize().Z : -1,
                Component ? static_cast<int32>(Component->GetCollisionEnabled()) : -1,
                Component && Component->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore));
            return Mesh && Mesh->GetPathName()
                == TEXT("/Game/SurvivalGame/Environment/Reeds/SM_ReedClump.SM_ReedClump")
                && Mesh->GetBoundingBox().GetSize().Z > 100
                && Component->GetCollisionEnabled() == ECollisionEnabled::NoCollision
                && Component->GetCollisionResponseToChannel(ECC_Camera) == ECR_Ignore;
        });
    Add(TEXT("Orbit the gameplay camera for an unobstructed look at the reeds"),
        []() {}, []() { return true; }, 0.95f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_RightX, 0.8f); };
    Add(TEXT("Capture identifiable ready creek-bank reeds"),
        [this]() { Axis(EKeys::Gamepad_RightX, 0); Screenshot(TEXT("creek-reeds")); },
        [this, ReedId]() { return Controller->IsResourceFocused(*ReedId)
            && Controller->FocusTitle().Contains(TEXT("reeds"))
            && !Controller->FocusTitle().Contains(TEXT("renew")); }, 0.8f);
    const auto HarvestHour = MakeShared<double>(0);
    const auto ReedDeadline = MakeShared<double>(0);
    const auto KnifeStarts = MakeShared<uint32>(0);
    Add(TEXT("Mapped gathering supplies real Fiber once without a hatchet"),
        [this, HarvestHour, KnifeStarts]()
        {
            *HarvestHour = Controller->State().hour;
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Animation) { Finish(false, TEXT("Knife gather animation instance is unavailable.")); return; }
            *KnifeStarts = Animation->KnifeCutStarts();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, ReedId, FiberBefore, HarvestHour, ReedDeadline]()
        {
            bool Depleted = false;
            for (const auto& Node : Controller->State().resources)
                if (Node.id == *ReedId)
                {
                    Depleted = Node.readyAtHour > Controller->State().hour
                        && FMath::Abs(Node.readyAtHour - *HarvestHour - 24.0) < 1.e-6;
                    *ReedDeadline = Node.readyAtHour;
                }
            return Depleted
                && Controller->Simulation().Count(Homestead::Item::Fiber) == *FiberBefore + 5
                && Controller->Toast().IsEmpty() && !Controller->ToastIsError();
        }, 0.2f);
    Add(TEXT("Reed gather presents one restrained Knife cut with the owned held prop"),
        []() {},
        [this, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Avatar || !Animation || !Avatar->GetKnife()->IsPresented()
                || Animation->KnifeCutStarts() != *KnifeStarts + 1
                || Animation->KnifeCutWeight() <= 0.3f)
                return false;
            return true;
        }, 0.2f);
    Add(TEXT("Capture the held Knife cutting reeds at close gameplay distance"),
        [this, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            if (!Animation || Animation->KnifeCutWeight() <= 0.01f
                || Animation->KnifeCutStarts() != *KnifeStarts + 1)
            {
                Finish(false, TEXT("Knife work finished before its ordinary action capture."));
                return;
            }
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::LeftControl, IE_Pressed, 1));
            Axis(EKeys::MouseWheelAxis, 2);
            Axis(EKeys::MouseWheelAxis, 0);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::LeftControl, IE_Released, 0));
            if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineSequence")))
            {
                const FString Pose = DescribeHeroineAction(TEXT("reeds"));
                if (Pose.IsEmpty()) { Finish(false, TEXT("The Reed Knife full-body pose was unavailable.")); return; }
                Results.Add(Pose);
            }
            Screenshot(TEXT("creek-knife-gesture"));
        },
        [this, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Avatar && Animation && Avatar->CameraDistance() < 400
                && Animation->KnifeCutStarts() == *KnifeStarts + 1;
        }, 0.4f);
    Add(TEXT("Return the gameplay camera to its normal crossing distance"),
        [this]()
        {
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::LeftControl, IE_Pressed, 1));
            Axis(EKeys::MouseWheelAxis, -2);
            Axis(EKeys::MouseWheelAxis, 0);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::LeftControl, IE_Released, 0));
        },
        [this]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && Avatar->CameraDistance() >= 400;
        });
    Add(TEXT("Depleted reeds keep a small stubble instead of generic grass"),
        []() {}, [this, ReedId]()
        {
            const auto* Visual = Controller->Landscape->ResourceVisuals.Find(*ReedId);
            const auto* Component = Visual && Visual->Components.Num() == 1
                ? Cast<UStaticMeshComponent>(Visual->Components[0]) : nullptr;
            return Component && Component->GetStaticMesh()
                && Component->GetStaticMesh()->GetName() == TEXT("SM_ReedStubble")
                && !Controller->FocusTitle().Contains(TEXT("renew"))
                && !Controller->FocusActions().Contains(TEXT("Gather"));
        });
    Add(TEXT("Capture quiet depleted reed patch"),
        [this]() { Screenshot(TEXT("creek-reeds-depleted")); }, []() { return true; }, 0.6f);
    Add(TEXT("Immediate second gather cannot duplicate Fiber"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, FiberBefore, KnifeStarts]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto* Animation = Avatar
                ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
            return Controller->Simulation().Count(Homestead::Item::Fiber) == *FiberBefore + 5
                && Controller->ToastIsError() && Controller->Toast().Contains(TEXT("Nothing to gather"))
                && Animation && Animation->KnifeCutStarts() == *KnifeStarts + 1;
        });
    Add(TEXT("Save the real depleted reed patch and Fiber to the isolated current-version profile"),
        [this]() { Tap(EKeys::F5); },
        [this, FiberBefore]() { return Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual"))) != nullptr
            && Controller->Simulation().Count(Homestead::Item::Fiber) == *FiberBefore + 5; });
    Add(TEXT("Reload preserves exact Fiber and the visibly depleted bank patch"),
        [this]() { Tap(EKeys::F9); },
        [this, ReedId, FiberBefore, ReedDeadline]()
        {
            bool Depleted = false;
            for (const auto& Node : Controller->State().resources)
                if (Node.id == *ReedId) Depleted = Node.readyAtHour == *ReedDeadline
                    && Node.readyAtHour > Controller->State().hour;
            const auto* Visual = Controller->Landscape->ResourceVisuals.Find(*ReedId);
            const auto* Component = Visual && Visual->Components.Num() == 1
                ? Cast<UStaticMeshComponent>(Visual->Components[0]) : nullptr;
            return Controller->IsWorldReady() && Depleted
                && Controller->Simulation().Count(Homestead::Item::Fiber) == *FiberBefore + 5
                && Component && Component->GetStaticMesh()
                && Component->GetStaticMesh()->GetName() == TEXT("SM_ReedStubble");
        }, 0.4f);
    const auto RecipeProof = MakeShared<bool>(false);
    Add(TEXT("The real hatchet recipe consumes gathered Fiber with ordinary other ingredients"),
        [this, RecipeProof, FiberBefore]()
        {
            Homestead::Simulation Candidate = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Candidate.GetState());
            for (const auto Pair : {TPair<Homestead::Item, int32>(Homestead::Item::Branch, 4),
                TPair<Homestead::Item, int32>(Homestead::Item::Stone, 3)})
            {
                State.inventory[static_cast<int32>(Pair.Key)] += Pair.Value;
                State.inventoryLayout.push_back({State.nextGroupId++, Pair.Key, Pair.Value, 0});
            }

            Homestead::Simulation Proof = Controller->Simulation();
            const auto Restored = Proof.Deserialize(Candidate.Serialize());
            *RecipeProof = Restored.ok && Proof.Craft(Homestead::Recipe::Hatchet, Controller->PlayerPoint()).ok
                && Proof.Count(Homestead::Item::Fiber) == *FiberBefore + 3
                && Proof.Count(Homestead::Item::Hatchet) == 1;
        },
        [this, RecipeProof, FiberBefore]()
        {
            return *RecipeProof && Controller->Simulation().Count(Homestead::Item::Fiber) == *FiberBefore + 5
                && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0;
        });
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
