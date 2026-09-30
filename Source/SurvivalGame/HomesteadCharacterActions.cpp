#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadDiggingStick.h"
#include "HomesteadKnife.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

void AHomesteadCharacter::PlayWater()
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    WaterYaw.Reset();
    bFillingPail = false;
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestWater();
    else
        UE_LOG(LogTemp, Error, TEXT("Watering succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayWater(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    bFillingPail = false;
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Water committed without a valid presentation target; using heroine facing."));
        PlayWater();
        return;
    }
    WaterYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (PailPourAnimation && bMetaHumanActive)
    {
        // Step up to the square so the stream lands on its middle (pail_pour.SPOT).
        const float Yaw = *WaterYaw - FMath::RadiansToDegrees(FMath::Atan2(PourRight, PourForward));
        const FVector To = FVector(Target.x, Target.y, GetActorLocation().Z)
            - FRotator(0, Yaw, 0).RotateVector(FVector(PourForward, PourRight, 0));
        WaterYaw = Yaw;
        BeginStanceStep(FVector::Dist2D(To, GetActorLocation()) < 150.0f ? To : GetActorLocation(), Yaw);
    }
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestWater();
    else
        UE_LOG(LogTemp, Error, TEXT("Watering succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayFillPail(Homestead::Point Stream)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!PailFillAnimation || !bMetaHumanActive || !Animation) return;
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    bFillingPail = true;
    const FVector2D Delta(Stream.x - GetActorLocation().X, Stream.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
    {
        // Kneel facing the water so the pail goes in ahead of her (pail_fill.SPOT). The prompt shows
        // up to a metre or so back from the waterline, so she steps down the bank until the pail's
        // reach lands on the dip point, never further than FillStepMax.
        const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X))
            - FMath::RadiansToDegrees(FMath::Atan2(FillRight, FillForward));
        WaterYaw = Yaw;
        const float Reach = FVector2D(FillForward, FillRight).Size();
        const float Step = FMath::Clamp(static_cast<float>(Delta.Size()) - Reach, 0.0f, FillStepMax);
        const FVector2D Toward = Delta.GetSafeNormal() * Step;
        BeginStanceStep(GetActorLocation() + FVector(Toward.X, Toward.Y, 0.0), Yaw);
        bStanceStepFollowsGround = Step > 1.0f;
    }
    Animation->RequestWater();
}

void AHomesteadCharacter::CancelAction(bool Immediate)
{
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->CancelAction(Immediate);
    PendingKneel.Reset();
    // Every gather prop, not just the sticks: a cancelled stone gather used to leave its stones on
    // her forearm, because stage 0 then matched and nothing hid them.
    HideKneelProps();
    StickStage = 0;
    bStickPileOnGround = false;
    StickAlignRemaining = 0;
    WateringTool->SetHiddenInGame(true, true);
    Hatchet->SetHiddenInGame(true, true);
    DiggingStick->SetHiddenInGame(true, true);
    Knife->SetHiddenInGame(true, true);
    ClearYaw.Reset();
    TillYaw.Reset();
    bFellApproach = false;
    WaterYaw.Reset();
}

void AHomesteadCharacter::PlayClear()
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    ClearYaw.Reset();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestClear();
    else
        UE_LOG(LogTemp, Error, TEXT("Sapling clearing succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayClear(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Chop committed without a valid presentation target; using heroine facing."));
        PlayClear();
        return;
    }

    ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestClear();
    else
        UE_LOG(LogTemp, Error, TEXT("Chopping succeeded but its animation instance is unavailable."));
}

void AHomesteadCharacter::PlayKnifeCut(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
        ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Knife work committed without a valid presentation target; using heroine facing."));
        ClearYaw.Reset();
    }
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestKnifeCut();
    else
        UE_LOG(LogTemp, Error, TEXT("Knife work succeeded but its distinct animation instance is unavailable."));
}

float AHomesteadCharacter::CraftingPhase() const
{
    if (const auto* PC = Cast<AHomesteadController>(Controller))
    {
        const float Progress = PC->CraftProgress();
        return Progress > 0.0f ? Progress : -1.0f;
    }
    if (!InCharacterLab() || !GetWorld()) return -1.0f;
    const double Now = GetWorld()->GetTimeSeconds();
    return Now < LabCraftUntil ? FMath::Fmod(static_cast<float>(Now - LabCraftStart) / CraftCycleSeconds, 1.0f) : -1.0f;
}

void AHomesteadCharacter::PlayLabCraft(int32 Cycles)
{
    if (!GetWorld()) return;
    LabCraftStart = GetWorld()->GetTimeSeconds();
    LabCraftUntil = LabCraftStart + CraftCycleSeconds * FMath::Max(1, Cycles);
}

bool AHomesteadCharacter::PlayEat(bool bBerry)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!bMetaHumanActive || !EatAnimation || !EatenFood || !Animation || Animation->IsEating()) return false;
    bEatBerry = bBerry;
    Animation->RequestEat();
    return true;
}

void AHomesteadCharacter::UpdateEating()
{
    if (!EatenFood) return;
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const float Time = Animation && Animation->IsEating() ? Animation->EatPhase() : -1.0f;
    const bool bInHand = Time >= EatPick && Time < EatBite;
    if (bInHand == bEatFoodInHand) return;
    bEatFoodInHand = bInHand;
    if (!bInHand)
    {
        EatenFood->SetVisibility(false);
        return;
    }
    // Pinched between thumb and fingertips, like the forage she stows (UpdateCarriedSticks).
    USkeletalMeshComponent* Body = GetMesh();
    const FVector Hand = Body->GetSocketLocation(TEXT("hand_r"));
    const FVector Fingers = (Body->GetSocketLocation(TEXT("middle_01_r")) - Hand).GetSafeNormal();
    const FVector Across = (Body->GetSocketLocation(TEXT("index_01_r")) - Body->GetSocketLocation(TEXT("pinky_01_r"))).GetSafeNormal();
    UStaticMesh* FoodMesh = bEatBerry ? ForageBerryMesh.Get() : ForageRootMesh.Get();
    const bool bAuthored = FoodMesh != nullptr;
    if (bAuthored) EatenFood->SetStaticMesh(FoodMesh);
    // A small bite: a few berries off the cluster, or a short piece of root.
    const FVector Scale = bAuthored ? FVector(bEatBerry ? 0.65f : 0.45f) : FVector(0.035f);
    const FRotator Rotation = bEatBerry ? FRotationMatrix::MakeFromZX(-Fingers, Across).Rotator()
        : FRotationMatrix::MakeFromZX(Fingers, Across).Rotator();
    // Between the pinched thumb and fingertips (the last knuckles plus a little toward the tips).
    const FVector Index = Body->GetSocketLocation(TEXT("index_03_r"));
    const FVector Thumb = Body->GetSocketLocation(TEXT("thumb_03_r"));
    const FVector Pinch = (Index + Thumb) * 0.5f + ((Index - Hand).GetSafeNormal() + (Thumb - Hand).GetSafeNormal()).GetSafeNormal() * 1.2f;
    EatenFood->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
    EatenFood->SetWorldLocationAndRotation(Pinch, Rotation);
    EatenFood->SetWorldScale3D(Scale);
    EatenFood->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, TEXT("hand_r"));
    EatenFood->SetVisibility(true);
}

bool AHomesteadCharacter::PlayMacheteHack(Homestead::Point Target)
{
    return PlayMacheteHack(Target, Homestead::Item::Machete);
}

bool AHomesteadCharacter::PlayMacheteHack(Homestead::Point Target, Homestead::Item Tool)
{
    if (!bMetaHumanActive || !MacheteAnimation || !GetHeldProp(Tool)) return false;
    HackTool = Tool;
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation) return false;
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
    {
        ClearYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
        BeginStanceStep(GetActorLocation(), *ClearYaw);
    }
    else ClearYaw.Reset();
    Animation->RequestMacheteHack();
    return true;
}

void AHomesteadCharacter::BeginStanceStep(const FVector& To, float Yaw)
{
    FellStepFrom = GetActorLocation();
    FellStepTo = To;
    FellStepFromYaw = GetActorRotation().Yaw;
    FellStepToYaw = Yaw;
    FellStepRemaining = FellStepSeconds;
    bStanceStepFollowsGround = false;
}

void AHomesteadCharacter::SettleOnGround()
{
    // Down a stream bank the step's straight line leaves her hovering over the slope; put her feet
    // back on the ground below (the Landscape) rather than letting her drop and land.
    UWorld* World = GetWorld();
    if (!World) return;
    const float HalfHeight = GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    const FVector Here = GetActorLocation();
    FHitResult Hit;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadSettleOnGround), false, this);
    if (World->LineTraceSingleByChannel(Hit, Here + FVector(0, 0, StanceGroundProbeUp),
            Here - FVector(0, 0, HalfHeight + StanceGroundProbeDown), ECC_Visibility, Query))
        SetActorLocation(FVector(Here.X, Here.Y, Hit.ImpactPoint.Z + HalfHeight + 1.0f), false);
}

bool AHomesteadCharacter::CanFell() const
{
    return bMetaHumanActive && FellAnimation && GetHeldProp(Homestead::Item::Hatchet);
}

UAnimSequence* AHomesteadCharacter::GetFellAnimation() const
{
    if (FellTool == Homestead::Item::Scythe) return MowAnimation;
    if (FellTool == Homestead::Item::Pickaxe || bStrikeHatchet) return StrikeAnimation;
    return FellAnimation;
}

bool AHomesteadCharacter::CanStrike(Homestead::Item Tool) const
{
    if (!bMetaHumanActive || !GetHeldProp(Tool)) return false;
    if (Tool == Homestead::Item::Scythe) return MowAnimation != nullptr;
    return (Tool == Homestead::Item::Pickaxe || Tool == Homestead::Item::Hatchet) && StrikeAnimation;
}

bool AHomesteadCharacter::PlayFell(Homestead::Point Target, int32 Strokes, float TrunkRadius)
{
    if (!CanFell()) return false;
    FellTool = Homestead::Item::Hatchet;
    bStrikeHatchet = false;
    // The trunk's centre sits past the bit along its travel, 2 cm of bite in.
    return BeginTwoHanded(Target, Strokes, TrunkRadius > 0 ? FMath::Max(0.0f, TrunkRadius - 2.0f) : -1.0f,
        FellBitLeft, FellBitForward, FellCutLeft, FellCutForward);
}

bool AHomesteadCharacter::PlayStrike(Homestead::Point Target, Homestead::Item Tool, int32 Strokes, float Radius)
{
    if (!CanStrike(Tool)) return false;
    FellTool = Tool;
    bStrikeHatchet = Tool == Homestead::Item::Hatchet;
    const bool bPick = Tool == Homestead::Item::Pickaxe;
    // The blow comes straight down, so the target's centre sits beyond the point by half its radius:
    // the point lands on its near side.
    return BeginTwoHanded(Target, Strokes, Tool == Homestead::Item::Scythe ? -1.0f : Radius,
        bPick ? StrikePickLeft : StrikeAxeLeft, bPick ? StrikePickForward : StrikeAxeForward, 0.0f, 0.5f);
}

bool AHomesteadCharacter::BeginTwoHanded(Homestead::Point Target, int32 Strokes, float Bite, float BitLeft,
    float BitForward, float CutLeft, float CutForward)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation) return false;
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (FMath::IsFinite(Delta.X) && FMath::IsFinite(Delta.Y) && Delta.SizeSquared() >= 1)
    {
        const float TreeYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
        ClearYaw = TreeYaw;
        FellStepRemaining = 0;
        if (Bite >= 0)
        {
            const float Left = BitLeft + CutLeft * Bite, Forward = BitForward + CutForward * Bite;
            const float Standoff = FMath::Sqrt(Left * Left + Forward * Forward);
            // The bit lands to one side of straight ahead (her right for the right-shoulder chop),
            // so she faces a little past the tree to the other side.
            ClearYaw = TreeYaw + FMath::RadiansToDegrees(FMath::Atan2(Left, Forward));
            const FVector2D To = FVector2D(Target.x, Target.y) - Delta.GetSafeNormal() * Standoff;
            UE_LOG(LogTemp, Verbose, TEXT("Fell: target (%.0f, %.0f) bite %.1f from (%.0f, %.0f) stance (%.0f, %.0f) yaw %.1f"), Target.x, Target.y, Bite, GetActorLocation().X, GetActorLocation().Y, To.X, To.Y, *ClearYaw);
            if (FVector2D::Distance(To, FVector2D(GetActorLocation())) > 35.0f && Delta.Size() > Standoff)
            {
                // Too far for a stance step: walk up to it, then settle and swing.
                bFellApproach = true;
                FellApproachTo = To;
                FellApproachYaw = *ClearYaw;
                FellApproachTime = 0;
                FellApproachStrokes = Strokes;
                return true;
            }
            BeginStanceStep(FVector(To.X, To.Y, GetActorLocation().Z), *ClearYaw);
        }
        else BeginStanceStep(GetActorLocation(), *ClearYaw);
    }
    else ClearYaw.Reset();
    Animation->RequestFell(Strokes);
    return true;
}

void AHomesteadCharacter::UpdateFellApproach(float DeltaSeconds)
{
    if (!bFellApproach) return;
    FellApproachTime += DeltaSeconds;
    const FVector2D Remaining = FellApproachTo - FVector2D(GetActorLocation());
    const float Distance = Remaining.Size();
    // Close enough for the stance step to finish the placement (or blocked): address the trunk.
    if (Distance > 12.0f && FellApproachTime < 2.5f)
    {
        const float Scale = FMath::Clamp(Distance / 70.0f, 0.4f, 1.0f);
        AddMovementInput(FVector(Remaining.GetSafeNormal(), 0.0), Scale);
        return;
    }
    bFellApproach = false;
    UE_LOG(LogTemp, Verbose, TEXT("Fell: approach ended at (%.0f, %.0f) after %.2fs, %.0f cm short"), GetActorLocation().X, GetActorLocation().Y, FellApproachTime, Distance);
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation || !(FellTool == Homestead::Item::Hatchet && !bStrikeHatchet ? CanFell() : CanStrike(FellTool))) return;
    GetCharacterMovement()->StopMovementImmediately();
    ClearYaw = FellApproachYaw;
    BeginStanceStep(FVector(FellApproachTo.X, FellApproachTo.Y, GetActorLocation().Z), FellApproachYaw);
    Animation->RequestFell(FellApproachStrokes);
}

void AHomesteadCharacter::PlayTill(Homestead::Point Target)
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Target.x - GetActorLocation().X, Target.y - GetActorLocation().Y);
    if (!FMath::IsFinite(Delta.X) || !FMath::IsFinite(Delta.Y) || Delta.SizeSquared() < 1)
    {
        UE_LOG(LogTemp, Warning, TEXT("Till committed without a valid presentation target; using heroine facing."));
        TillYaw.Reset();
    }
    else
        TillYaw = FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X));
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestTill();
    else
        UE_LOG(LogTemp, Error, TEXT("Tilling succeeded but its animation instance is unavailable."));
}
