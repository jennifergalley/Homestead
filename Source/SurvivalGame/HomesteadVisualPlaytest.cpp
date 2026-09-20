#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadTestPaths.h"
#include "Components/SkeletalMeshComponent.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#if WITH_EDITOR
#include "ShaderCompiler.h"
#endif

AHomesteadVisualPlaytest::AHomesteadVisualPlaytest()
{
#if !UE_BUILD_SHIPPING
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
#endif
}

void AHomesteadVisualPlaytest::Tap(FKey Key)
{
    if (!Key.IsValid()) return;
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0));
}

void AHomesteadVisualPlaytest::ApplyAxes(FVector2D Move, FVector2D Look)
{
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftX, IE_Axis, Move.X, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_LeftY, IE_Axis, Move.Y, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightX, IE_Axis, Look.X, 1));
    PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_RightY, IE_Axis, Look.Y, 1));
}

void AHomesteadVisualPlaytest::Prepare()
{
    OutputDirectory = HomesteadTestOutputDirectory();
    IFileManager::Get().MakeDirectory(*FPaths::Combine(OutputDirectory, TEXT("Frames")), true);
    Telemetry.Add(TEXT("frame,seconds,pass,x,y,z,speed,yaw,view_yaw,left_toe_x,left_toe_y,left_toe_z,right_toe_x,right_toe_y,right_toe_z,walk_weight,gait_rate,left_hand_x,left_hand_y,left_hand_z,right_hand_x,right_hand_y,right_hand_z,walk_phase"));
    Observations.Add(TEXT("Observational visual playtest: normal mapped controls; no teleports, state edits, or time skips."));
    Observations.Add(TEXT("Frames are sampled at 8 Hz. Screenshot readback can disturb pacing; do not use this run as a frame-rate benchmark."));
    Passes = {
        {TEXT("close-notes"), 1, {}, {}, EKeys::Gamepad_Special_Right},
        {TEXT("idle"), 3},
        {TEXT("slow-walk"), 4, FVector2D(0, 0.5f)},
        {TEXT("stop-from-slow-walk"), 2},
        {TEXT("full-walk"), 3, FVector2D(0, 1)},
        {TEXT("turn-while-moving"), 3, FVector2D(0.7f, 0.45f)},
        {TEXT("stop-from-turn"), 2},
        {TEXT("orbit-standing-character"), 4, {}, FVector2D(0.65f, 0)},
        {TEXT("open-pack"), 0.5f, {}, {}, EKeys::Gamepad_Special_Right},
        {TEXT("open-look"), 3, {}, {}, EKeys::Gamepad_LeftShoulder},
        {TEXT("portrait-idle"), 3, {}, {}, EKeys::Gamepad_RightThumbstick},
        {TEXT("portrait-orbit"), 4, {}, FVector2D(0.45f, 0)},
        {TEXT("return-to-world"), 1, {}, {}, EKeys::Gamepad_FaceButton_Right},
        {TEXT("walk-to-forage"), 30, {}, {}, FKey(), true},
        {TEXT("gather"), 3},
        {TEXT("after-gather"), 2}
    };
    LastWallTime = FPlatformTime::Seconds();
}

void AHomesteadVisualPlaytest::Capture(const FString& Label)
{
    const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn());
    if (!Avatar) return;
    const FVector Position = Avatar->GetActorLocation();
    const FVector Left = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_l"));
    const FVector Right = Avatar->GetMesh()->GetBoneLocation(TEXT("ball_r"));
    const FVector LeftHand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_l"));
    const FVector RightHand = Avatar->GetMesh()->GetBoneLocation(TEXT("hand_r"));
    const auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance());
    Telemetry.Add(FString::Printf(TEXT("%d,%.4f,%s,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.4f"),
        CaptureIndex, Elapsed, *Label, Position.X, Position.Y, Position.Z, Avatar->GetVelocity().Size2D(),
        Avatar->GetActorRotation().Yaw, PC->GetControlRotation().Yaw,
        Left.X, Left.Y, Left.Z, Right.X, Right.Y, Right.Z,
        Animation ? Animation->WalkWeight() : -1, Animation ? Animation->GaitRate() : -1,
        LeftHand.X, LeftHand.Y, LeftHand.Z, RightHand.X, RightHand.Y, RightHand.Z,
        Animation ? Animation->WalkPhase() : -1));
    const FString Name = FString::Printf(TEXT("frame-%05d.png"), CaptureIndex++);
    FScreenshotRequest::RequestScreenshot(FPaths::Combine(OutputDirectory, TEXT("Frames"), Name), false, false);
}

void AHomesteadVisualPlaytest::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) return;
    if (!bReady)
    {
        Elapsed += DeltaSeconds;
        PC = Cast<AHomesteadController>(UGameplayStatics::GetPlayerController(this, 0));
        if (!PC || !PC->GetPawn() || Elapsed < 8) return;
#if WITH_EDITOR
        if (GShaderCompilingManager && GShaderCompilingManager->IsCompiling()) return;
#endif
        bReady = true;
        Prepare();
        Elapsed = 0;
    }
    const double Now = FPlatformTime::Seconds();
    const float WallDelta = FMath::Min(static_cast<float>(Now - LastWallTime), 1.0f);
    LastWallTime = Now;
    Elapsed += WallDelta;
    if (!Passes.IsValidIndex(PassIndex) || Elapsed > 100) { Finish(); return; }
    FPass& Pass = Passes[PassIndex];
    if (!bEntered)
    {
        bEntered = true;
        PassElapsed = 0;
        CaptureElapsed = 0;
        Tap(Pass.Press);
        if (Pass.WalkToForage)
        {
            double Best = 1.e20;
            const auto Position = PC->PlayerPoint();
            for (const auto& Node : PC->State().resources)
            {
                if (!PC->Simulation().CanHarvest(Node.id)
                    || (Node.kind != Homestead::ResourceKind::BerryBush && Node.kind != Homestead::ResourceKind::Flowers)) continue;
                const double Distance = FMath::Square(Node.position.x - Position.x) + FMath::Square(Node.position.y - Position.y);
                if (Distance < Best)
                {
                    Best = Distance;
                    ForageId = Node.id;
                    ForageTarget = FVector2D(Node.position.x, Node.position.y);
                }
            }
            FoodBefore = PC->Simulation().Count(Homestead::Item::Berries);
            HerbBefore = PC->Simulation().Count(Homestead::Item::Flowers);
            Observations.Add(FString::Printf(TEXT("Forage target id=%d position=%s"), ForageId, *ForageTarget.ToString()));
        }
        if (Pass.Label == TEXT("gather"))
        {
            if (bReachedForage && PC->IsResourceFocused(ForageId)) Tap(EKeys::Gamepad_FaceButton_Bottom);
            else Observations.Add(TEXT("Forage approach did not reach its target in time; gather was not faked."));
        }
        Observations.Add(FString::Printf(TEXT("BEGIN %.2fs %s: %s"), Elapsed, *Pass.Label, *PC->FocusTitle()));
    }
    FVector2D Move = Pass.Move;
    FVector2D Look = Pass.Look;
    if (Pass.WalkToForage && ForageId >= 0)
    {
        const auto Position = PC->PlayerPoint();
        const FVector2D Offset = ForageTarget - FVector2D(Position.x, Position.y);
        const float DesiredYaw = FMath::RadiansToDegrees(FMath::Atan2(Offset.Y, Offset.X));
        const float Difference = FMath::FindDeltaAngleDegrees(static_cast<float>(PC->GetControlRotation().Yaw), DesiredYaw);
        Look.X = FMath::Clamp(Difference / 45.0f, -0.7f, 0.7f);
        Move.Y = FMath::Abs(Difference) < 40 ? (Offset.Size() < 90 ? 0.4f : 0.6f) : 0;
        if (Offset.Size() < 25)
        {
            Move = Look = FVector2D::ZeroVector;
            if (PC->IsResourceFocused(ForageId))
            {
                bReachedForage = true;
                PassElapsed = Pass.Duration;
            }
        }
    }
    ApplyAxes(Move, Look);
    PassElapsed += WallDelta;
    CaptureElapsed += WallDelta;
    if (CaptureElapsed >= 0.125f)
    {
        Capture(Pass.Label);
        CaptureElapsed = 0;
    }
    if (PassElapsed >= Pass.Duration)
    {
        ApplyAxes({}, {});
        ++PassIndex;
        bEntered = false;
    }
}

void AHomesteadVisualPlaytest::Finish()
{
    if (bFinished) return;
    bFinished = true;
    ApplyAxes({}, {});
    const bool Gathered = PC->Simulation().Count(Homestead::Item::Berries) > FoodBefore
        || PC->Simulation().Count(Homestead::Item::Flowers) > HerbBefore;
    Observations.Add(FString::Printf(TEXT("Forage target reached=%d; resources actually gathered=%d"), bReachedForage, Gathered));
    Observations.Add(TEXT("This observational capture is not a visual-quality pass or a replacement for human feel/listening review."));
    bool Saved = FFileHelper::SaveStringToFile(FString::Join(Telemetry, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("telemetry.csv")));
    Saved = FFileHelper::SaveStringToFile(FString::Join(Observations, TEXT("\n")) + TEXT("\n"),
        *FPaths::Combine(OutputDirectory, TEXT("observations.txt"))) && Saved;
    UE_LOG(LogTemp, Display, TEXT("Visual playtest captured %d frames in %s"), CaptureIndex, *OutputDirectory);
    FPlatformMisc::RequestExitWithStatus(false, Saved && bReachedForage && Gathered ? 0 : 1);
}
