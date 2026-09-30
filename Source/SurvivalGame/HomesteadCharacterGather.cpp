#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWorld.h"

#include "Animation/AnimSequence.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadGather, Log, All);

// Stick moments in AN_HeroineMH_KneelGatherSticks (seconds; homestead_agent.kneel_gather STICK_EVENTS).
// Stones share this clip.
namespace GatherSticksTiming
{
constexpr float Pick1 = 38.0f / 30.0f, Stack1 = 54.0f / 30.0f, Pick2 = 70.0f / 30.0f, Stack2 = 86.0f / 30.0f,
    Stow = 112.0f / 30.0f;
}
// Moments in AN_HeroineMH_KneelGatherPouch (seconds; homestead_agent.kneel_pouch POUCH_EVENTS).
namespace GatherPouchTiming
{
constexpr float Pick1 = 36.0f / 30.0f, Stow1 = 56.0f / 30.0f, Pick2 = 74.0f / 30.0f, Stow2 = 94.0f / 30.0f;
}
// Moments in AN_HeroineMH_KneelCutReeds (seconds; homestead_agent.kneel_reeds EVENTS): her left
// fist closes on the stems, the knife cuts them free.
namespace GatherReedsTiming
{
constexpr float Grab = 38.0f / 30.0f, Cut = 72.0f / 30.0f;
}

// Moments in AN_HeroineMH_KneelPullWeeds (seconds; homestead_agent.kneel_pull_weeds EVENTS): each
// root comes out of the ground, then leaves her hand behind her shoulder.
namespace PullWeedsTiming
{
constexpr float Pulled[2] = {56.0f / 30.0f, 102.0f / 30.0f};
constexpr float Toss[2] = {68.0f / 30.0f, 114.0f / 30.0f};
// A fistful's largest dimension in her hand (cm), whatever the clump's mesh.
constexpr float HandfulSizeCm = 24.0f;
// Let go behind the shoulder (actor frame: X forward, Y right): back, out to that side and up.
constexpr float TossBack = 260.0f, TossOut = 110.0f, TossUp = 150.0f, Gravity = 980.0f;
// Tumbling end over end while it flies (degrees per second).
constexpr float TossSpin = 240.0f;
}

// Moments in AN_HeroineMH_KneelPlant (seconds; homestead_agent.kneel_plant EVENTS).
namespace GatherPlantTiming
{
constexpr float Pick = 36.0f / 30.0f, Press = 58.0f / 30.0f, Covered = 102.0f / 30.0f;
}

void AHomesteadCharacter::PlayGather()
{
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    if (auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance()))
        Animation->RequestGather();
    else
        UE_LOG(LogTemp, Error, TEXT("Gather succeeded but the heroine gathering animation instance is unavailable."));
}

UStaticMesh* AHomesteadCharacter::LoadHandStone(int32 Index)
{
    static const TCHAR* Names[] = {TEXT("SM_HandStone_A"), TEXT("SM_HandStone_B"), TEXT("SM_HandStone_C")};
    if (Index < 0 || Index >= UE_ARRAY_COUNT(Names)) return nullptr;
    const FString Path = FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/HandStones/%s.%s"), Names[Index], Names[Index]);
    return LoadObject<UStaticMesh>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
}

bool AHomesteadCharacter::PlayGatherSticks(TOptional<FVector2D> Pile)
{
    return PlayKneelGather(EHomesteadKneelGather::Sticks, Pile);
}

UAnimSequence* AHomesteadCharacter::KneelClip(EHomesteadKneelGather Kind) const
{
    if (!bMetaHumanActive) return nullptr;
    switch (Kind)
    {
    case EHomesteadKneelGather::Pouch: return GatherPouchAnimation.Get();
    case EHomesteadKneelGather::Reeds: return GatherReedsAnimation.Get();
    case EHomesteadKneelGather::Plant: return GatherPlantAnimation.Get();
    case EHomesteadKneelGather::Harvest: return GatherHarvestAnimation.Get();
    case EHomesteadKneelGather::PullWeeds: return PullWeedsAnimation.Get();
    default: return GatherSticksAnimation.Get();
    }
}

void AHomesteadCharacter::HideKneelProps()
{
    for (UStaticMeshComponent* Prop : CarriedSticks) if (Prop) Prop->SetVisibility(false);
    for (UStaticMeshComponent* Prop : CarriedStones) if (Prop) Prop->SetVisibility(false);
    for (UStaticMeshComponent* Prop : {CarriedForage.Get(), CarriedReeds.Get(), CarriedSeed.Get(), PulledWeedL.Get(), PulledWeedR.Get()})
        if (Prop) Prop->SetVisibility(false);
}

bool AHomesteadCharacter::PlayKneelGather(EHomesteadKneelGather Kind, TOptional<FVector2D> Pile, bool bBerries,
    UStaticMesh* Produce)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const bool bPropsReady = Kind == EHomesteadKneelGather::Sticks ? CarriedSticks.Num() >= 2
        : Kind == EHomesteadKneelGather::Stones ? CarriedStones.Num() >= 2
        : Kind == EHomesteadKneelGather::Reeds ? CarriedReeds && GetHeldProp(Homestead::Item::Knife)
        : Kind == EHomesteadKneelGather::Plant ? CarriedSeed != nullptr
        : Kind == EHomesteadKneelGather::PullWeeds ? PulledWeedL && PulledWeedR
        : CarriedForage != nullptr;
    if (!KneelClip(Kind) || !bPropsReady || !Animation)
    {
        const bool bQuiet = Kind == EHomesteadKneelGather::Reeds || Kind == EHomesteadKneelGather::Plant
            || Kind == EHomesteadKneelGather::PullWeeds;
        // Only the legacy mannequin still has the generic knee-bend gather. On the MetaHuman a missing
        // kneel clip or prop is a broken build, not a reason to play the pose Jenny rejected (09-29).
        if (!bMetaHumanActive) { if (!bQuiet) PlayGather(); }
        else if (!bQuiet)
            UE_LOG(LogHomesteadGather, Error, TEXT("Kneeling gather %d can't play (clip %s, props %s, anim instance %s)."),
                static_cast<int32>(Kind), KneelClip(Kind) ? TEXT("ok") : TEXT("missing"), bPropsReady ? TEXT("ok") : TEXT("missing"),
                Animation ? TEXT("ok") : TEXT("missing"));
        return false;
    }
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    // A later pickup replaces one still waiting; one that is already playing finishes first, with
    // its own clip and props, so a quick second pickup never borrows the first one's animation.
    PendingKneel = FPendingKneel{Kind, Pile, bBerries, false, false, GetWorld()->GetTimeSeconds(), Produce};
    UpdatePendingKneel();
    return true;
}

void AHomesteadCharacter::UpdatePendingKneel()
{
    if (!PendingKneel) return;
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    FPendingKneel& Kneel = *PendingKneel;
    const double Waited = GetWorld()->GetTimeSeconds() - Kneel.Since;
    if (!Animation || Waited > 4.0)
    {
        UE_LOG(LogTemp, Warning, TEXT("A kneeling gather never started (waited %.1f s); the pickup plays no animation."), Waited);
        PendingKneel.Reset();
        return;
    }
    if (Kneel.bApplied && Animation->IsGatheringSticks())
    {
        PendingKneel.Reset();
        return;
    }
    if (Animation->IsHandActionBusy())
    {
        // Something long (a felling, say) still holds her hands: let it go rather than keep her waiting.
        if (!Kneel.bApplied && !Kneel.bCancelledBlocker && Waited > 1.2 && !Animation->IsGatheringSticks())
        {
            Kneel.bCancelledBlocker = true;
            Animation->CancelAction();
        }
        return;
    }
    if (!Kneel.bApplied) StartKneelGather(Kneel);
    else Animation->RequestGatherSticks(); // Refused last update (she was still settling); ask again.
}

void AHomesteadCharacter::StartKneelGather(FPendingKneel& Kneel)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const EHomesteadKneelGather Kind = Kneel.Kind;
    const TOptional<FVector2D>& Pile = Kneel.Pile;
    const bool bBerries = Kneel.bBerries;
    Kneel.bApplied = true;
    HideKneelProps();
    KneelKind = Kind;
    bForageBerries = bBerries;
    UStaticMesh* Produce = Kneel.Produce.Get();
    HarvestProduceMesh = Produce;
    if (Kind == EHomesteadKneelGather::Harvest || (Kind == EHomesteadKneelGather::Pouch && Produce))
    {
        CarriedForage->SetStaticMesh(Produce ? Produce : ForageRootMesh.Get());
        if (!CarriedForage->GetStaticMesh()) HarvestProduceMesh = nullptr;
        else for (int32 Slot = 0; Slot < CarriedForage->GetNumMaterials(); ++Slot) CarriedForage->SetMaterial(Slot, nullptr);
    }
    else if (Kind == EHomesteadKneelGather::Pouch)
    {
        UStaticMesh* ForageMesh = bBerries ? ForageBerryMesh.Get() : ForageRootMesh.Get();
        if (ForageMesh) CarriedForage->SetStaticMesh(ForageMesh);
        else
        {
            // Placeholder until the authored props are imported: a berry-red or root-brown ball.
            CarriedForage->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            if (auto* Tint = CarriedForage->CreateDynamicMaterialInstance(0,
                LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"))))
                Tint->SetVectorParameterValue(TEXT("Color"), bBerries ? FLinearColor(0.42f, 0.025f, 0.055f) : FLinearColor(0.65f, 0.43f, 0.19f));
        }
    }
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    StickStage = 0;
    bStickPileOnGround = true;
    bStickGatherStarted = false;
    SticksLifted = 0;
    StickAlignRemaining = 0;
    if (Pile)
    {
        // Her right hand picks both sticks up about 32 cm ahead and 26 cm to her right; reed stems
        // are gathered 34 cm ahead and 10 cm to her right, beside the forward knee (kneel_reeds.STEMS). Turn and settle her during the
        // first step so that spot lands on the pile.
        // Her forefinger presses the seed in 32 cm ahead and 9 cm to her right (kneel_plant.SPOT, as baked).
        const bool bReeds = Kind == EHomesteadKneelGather::Reeds, bPlant = Kind == EHomesteadKneelGather::Plant;
        const bool bHarvest = Kind == EHomesteadKneelGather::Harvest, bWeeds = Kind == EHomesteadKneelGather::PullWeeds;
        // Both hands close on the crop's crown 36 cm ahead and 6 cm to her right (kneel_harvest.CROWN);
        // the weed clump sits straight ahead of her knees, between her two pulls (kneel_pull_weeds).
        const float GrabForward = bReeds ? 34.0f : bHarvest ? HarvestCrownForward : bWeeds ? PullWeedsForward : 32.0f;
        const float GrabRight = bReeds ? 10.0f : bPlant ? 9.0f : bHarvest ? HarvestCrownRight : bWeeds ? PullWeedsRight : 26.0f;
        const FVector Here = GetActorLocation();
        const FVector2D ToPile = *Pile - FVector2D(Here);
        if (ToPile.Size() > 1.0f && ToPile.Size() < 150.0f)
        {
            const float Yaw = FMath::RadiansToDegrees(FMath::Atan2(ToPile.Y, ToPile.X))
                - FMath::RadiansToDegrees(FMath::Atan2(GrabRight, GrabForward));
            const FVector Desired = FVector(Pile->X, Pile->Y, Here.Z)
                - FRotator(0, Yaw, 0).RotateVector(FVector(GrabForward, GrabRight, 0));
            StickAlignFrom = GetActorTransform();
            StickAlignTo = FTransform(FRotator(0, Yaw, 0), Desired);
            StickAlignRemaining = StickAlignSeconds;
        }
    }
    Animation->RequestGatherSticks();
}

void AHomesteadCharacter::UpdatePouchSwing()
{
    USkeletalMeshComponent* Body = GetMesh();
    if (!ForagePouch || !Body || !ForagePouch->IsVisible()) return;
    const TArray<FTransform>& Pose = Body->GetComponentSpaceTransforms();
    const int32 Pelvis = Body->GetBoneIndex(TEXT("pelvis"));
    const int32 Thigh = Body->GetBoneIndex(TEXT("thigh_r"));
    const int32 Calf = Body->GetBoneIndex(TEXT("calf_r"));
    if (!Pose.IsValidIndex(Pelvis) || !Pose.IsValidIndex(Thigh) || !Pose.IsValidIndex(Calf)) return;
    // The thigh's direction as her pelvis sees it, in the reference pose's frame.
    const FVector Dir = PelvisRefPose.TransformVectorNoScale(Pose[Pelvis].InverseTransformVectorNoScale(
        Pose[Calf].GetLocation() - Pose[Thigh].GetLocation())).GetSafeNormal();
    const auto Swing = [this, &Dir](const FVector& Axis, float Alpha, float MinDegrees, float MaxDegrees)
    {
        const FVector From = FVector::VectorPlaneProject(ThighDirRef, Axis).GetSafeNormal();
        const FVector To = FVector::VectorPlaneProject(Dir, Axis).GetSafeNormal();
        if (From.IsNearlyZero() || To.IsNearlyZero()) return FQuat::Identity;
        const float Angle = FMath::Atan2(FVector::DotProduct(FVector::CrossProduct(From, To), Axis), FVector::DotProduct(From, To));
        return FQuat(Axis, FMath::Clamp(Angle * Alpha, FMath::DegreesToRadians(MinDegrees), FMath::DegreesToRadians(MaxDegrees)));
    };
    // Forward and back it pivots on the belt (the thigh swings under it about the same lateral
    // axis, so it stays flush); out and in it rides the thigh about the hip.
    const FQuat Flex = Swing(FVector::XAxisVector, 0.9f, -40.0f, 55.0f);
    const FQuat Abduct = Swing(FVector::YAxisVector, 1.0f, -20.0f, 20.0f);
    const FTransform AboutHip(Abduct, HipRef - Abduct.RotateVector(HipRef));
    const FTransform Placed = FTransform(Flex, PouchPivotRef) * AboutHip;
    ForagePouch->SetRelativeTransform(Placed.GetRelativeTransform(PelvisRefPose));
}

void AHomesteadCharacter::UpdateStickAlignment(float DeltaSeconds)
{
    if (StickAlignRemaining <= 0) return;
    StickAlignRemaining = FMath::Max(0.0f, StickAlignRemaining - DeltaSeconds);
    const float Alpha = FMath::SmoothStep(0.0f, 1.0f, 1.0f - StickAlignRemaining / StickAlignSeconds);
    const FVector Location = FMath::Lerp(StickAlignFrom.GetLocation(), StickAlignTo.GetLocation(), Alpha);
    const FQuat Rotation = FQuat::Slerp(StickAlignFrom.GetRotation(), StickAlignTo.GetRotation(), Alpha);
    SetActorLocationAndRotation(FVector(Location.X, Location.Y, GetActorLocation().Z), Rotation, true);
}

bool AHomesteadCharacter::PlayPlant(Homestead::Point Target)
{
    return PlayKneelGather(EHomesteadKneelGather::Plant, FVector2D(Target.x, Target.y));
}

bool AHomesteadCharacter::PlayHarvest(Homestead::Point Target, bool bPick, UStaticMesh* Produce)
{
    const FVector2D Spot(Target.x, Target.y);
    if (!bPick && bMetaHumanActive && GatherHarvestAnimation)
        return PlayKneelGather(EHomesteadKneelGather::Harvest, Spot, false, Produce);
    // Picked crops (and pulled ones until the harvest clip is imported) use the pouch forage clip,
    // picking from the near side of the plant.
    FVector2D Pick = Spot;
    const FVector2D Toward = FVector2D(GetActorLocation()) - Spot;
    if (bPick && Toward.Size() > 1.0f) Pick += Toward.GetSafeNormal() * 14.0f;
    return PlayKneelGather(EHomesteadKneelGather::Pouch, Pick, bPick, Produce);
}

bool AHomesteadCharacter::PlayPullWeeds(Homestead::Point Target, UStaticMesh* Handful)
{
    if (!CanPullWeeds()) return false;
    UStaticMesh* Mesh = Handful ? Handful : PulledWeedDefault.Get();
    for (UStaticMeshComponent* Prop : {PulledWeedL.Get(), PulledWeedR.Get()})
        if (Prop && Mesh) Prop->SetStaticMesh(Mesh);
    return PlayKneelGather(EHomesteadKneelGather::PullWeeds, FVector2D(Target.x, Target.y));
}

float AHomesteadCharacter::PullWeedsPhase() const
{
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    return KneelKind == EHomesteadKneelGather::PullWeeds && Animation && Animation->IsGatheringSticks()
        ? Animation->GatherSticksPhase() : -1.0f;
}

void AHomesteadCharacter::UpdatePulledWeeds(float DeltaSeconds)
{
    UStaticMeshComponent* Props[2] = {PulledWeedL.Get(), PulledWeedR.Get()};
    if (!Props[0] || !Props[1]) return;
    USkeletalMeshComponent* Body = GetMesh();
    const float Phase = PullWeedsPhase();
    for (int32 Hand = 0; Hand < 2; ++Hand)
    {
        UStaticMeshComponent* Prop = Props[Hand];
        // Before its pull, or once the clip has ended or been cancelled: nothing in her hand or the air.
        if (Phase < 0.0f || Phase < PullWeedsTiming::Pulled[Hand])
        {
            if (Prop->IsVisible()) Prop->SetVisibility(false);
            bPulledWeedHeld[Hand] = bPulledWeedFlying[Hand] = false;
            continue;
        }
        if (Phase < PullWeedsTiming::Toss[Hand])
        {
            if (bPulledWeedHeld[Hand] || !Prop->GetStaticMesh()) continue;
            // Uprooted in her fist: the tuft's base in her grip, its leaves out past the thumb side,
            // sized to a handful whatever the clump's mesh. Placed from the hand's own frame, so it
            // doesn't depend on the bone's axes.
            const TCHAR* Side = Hand == 0 ? TEXT("l") : TEXT("r");
            const FName HandBone(*FString::Printf(TEXT("hand_%s"), Side));
            const FVector Wrist = Body->GetSocketLocation(HandBone);
            const FVector Knuckles = Body->GetSocketLocation(*FString::Printf(TEXT("middle_01_%s"), Side));
            const FVector Fingers = (Knuckles - Wrist).GetSafeNormal();
            const FVector Thumbward = (Body->GetSocketLocation(*FString::Printf(TEXT("index_01_%s"), Side))
                - Body->GetSocketLocation(*FString::Printf(TEXT("pinky_01_%s"), Side))).GetSafeNormal();
            if (Thumbward.IsNearlyZero() || Fingers.IsNearlyZero()) continue;
            const FBoxSphereBounds Bounds = Prop->GetStaticMesh()->GetBounds();
            const float Scale = FMath::Clamp(PullWeedsTiming::HandfulSizeCm / FMath::Max(1.0f, Bounds.BoxExtent.GetMax() * 2.0f), 0.05f, 1.0f);
            const FRotator Turn = FRotationMatrix::MakeFromZX(Thumbward, Fingers).Rotator();
            const FVector Grip = Knuckles + Fingers * 3.0f;
            Prop->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            Prop->SetWorldLocationAndRotation(Grip - Thumbward * 3.0f, Turn);
            Prop->SetWorldScale3D(FVector(Scale));
            Prop->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, HandBone);
            Prop->SetVisibility(true);
            bPulledWeedHeld[Hand] = true;
            continue;
        }
        if (bPulledWeedHeld[Hand])
        {
            // Let go behind her shoulder: it carries on back and out to that side, then falls.
            Prop->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
            const float Out = Hand == 0 ? -PullWeedsTiming::TossOut : PullWeedsTiming::TossOut;
            PulledWeedVelocity[Hand] = GetActorRotation().RotateVector(
                FVector(-PullWeedsTiming::TossBack, Out, PullWeedsTiming::TossUp));
            bPulledWeedHeld[Hand] = false;
            bPulledWeedFlying[Hand] = Prop->IsVisible();
        }
        if (!bPulledWeedFlying[Hand]) continue;
        const float Ground = GetActorLocation().Z - GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 1.0f;
        FVector Location = Prop->GetComponentLocation();
        if (Location.Z <= Ground)
        {
            // Landed: it lies where it fell until the clip ends.
            bPulledWeedFlying[Hand] = false;
            continue;
        }
        PulledWeedVelocity[Hand].Z -= PullWeedsTiming::Gravity * DeltaSeconds;
        Location += PulledWeedVelocity[Hand] * DeltaSeconds;
        Location.Z = FMath::Max(Location.Z, Ground);
        const FQuat Tumble(GetActorRightVector(), FMath::DegreesToRadians(PullWeedsTiming::TossSpin * DeltaSeconds));
        Prop->SetWorldLocationAndRotation(Location, Tumble * Prop->GetComponentQuat());
    }
}

bool AHomesteadCharacter::IsCuttingReeds() const
{
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    return KneelKind == EHomesteadKneelGather::Reeds && Animation && Animation->IsGatheringSticks();
}

void AHomesteadCharacter::UpdateCarriedSticks()
{
    const bool bPouch = KneelKind == EHomesteadKneelGather::Pouch;
    const bool bStones = KneelKind == EHomesteadKneelGather::Stones;
    const bool bReeds = KneelKind == EHomesteadKneelGather::Reeds;
    const bool bPlant = KneelKind == EHomesteadKneelGather::Plant;
    const bool bHarvest = KneelKind == EHomesteadKneelGather::Harvest;
    const auto& Props = bStones ? CarriedStones : CarriedSticks;
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    const bool Active = Animation && Animation->IsGatheringSticks();
    // Whatever ended the gather (its clip, a cancel, another action), nothing stays in her hands.
    if (!Active) HideKneelProps();
    // Pulled weeds are tossed aside, never carried (UpdatePulledWeeds); the clump leaves the ground with the commit.
    if (KneelKind == EHomesteadKneelGather::PullWeeds)
    {
        bStickGatherStarted |= Active;
        bStickPileOnGround = false;
        return;
    }
    if (bPlant ? !CarriedSeed : bReeds ? !CarriedReeds : (bPouch || bHarvest) ? !CarriedForage : Props.Num() < 2) return;
    const float Time = Active ? Animation->GatherSticksPhase() : 0.0f;
    // Reeds come off the clump all at once, with the cut.
    // Reeds come off the clump all at once, with the cut; a planted square stays bare until covered.
    // A pulled crop comes out of the ground in one go.
    const float Pick1 = bPlant ? GatherPlantTiming::Covered : bReeds ? GatherReedsTiming::Cut
        : bHarvest ? HarvestPulled : bPouch ? GatherPouchTiming::Pick1 : GatherSticksTiming::Pick1;
    const float Pick2 = bPlant ? GatherPlantTiming::Covered : bReeds ? GatherReedsTiming::Cut
        : bHarvest ? HarvestPulled : bPouch ? GatherPouchTiming::Pick2 : GatherSticksTiming::Pick2;
    if (bReeds && Animation)
        Animation->SetLeftHandGrip(Active && Time >= GatherReedsTiming::Grab - 0.1f ? 1.0f : 0.0f);
    int32 Stage = 0;
    if (bPlant) Stage = Active && Time >= GatherPlantTiming::Pick && Time < GatherPlantTiming::Press ? 1 : 0;
    else if (bReeds) Stage = Active && Time >= GatherReedsTiming::Cut ? 1 : 0;
    else if (bHarvest) Stage = Active && Time >= HarvestPulled && Time < HarvestStowed ? 1 : 0;
    else if (bPouch)
    {
        using namespace GatherPouchTiming;
        Stage = !Active ? 0 : Time >= Stow2 ? 4 : Time >= Pick2 ? 3 : Time >= Stow1 ? 2 : Time >= Pick1 ? 1 : 0;
    }
    else
    {
        using namespace GatherSticksTiming;
        Stage = !Active || Time >= Stow ? 0 : Time >= Stack2 ? 4 : Time >= Pick2 ? 3 : Time >= Stack1 ? 2 : Time >= Pick1 ? 1 : 0;
    }
    bStickGatherStarted |= Active;
    if (Active) SticksLifted = Time >= Pick2 ? 2 : Time >= Pick1 ? 1 : 0;
    // The ground produce leaves with the second pickup (or if the gather ends early).
    if (bStickPileOnGround && ((Active && Time >= Pick2) || (bStickGatherStarted && !Active)))
        bStickPileOnGround = false;
    if (Stage == StickStage) return;
    StickStage = Stage;
    USkeletalMeshComponent* Body = GetMesh();
    // Hand frame: fingers, across the knuckles (index -> pinky reversed) and out of the palm.
    const auto HandFrame = [Body](FVector& Hand, FVector& Fingers, FVector& Across, FVector& Palm)
    {
        Hand = Body->GetSocketLocation(TEXT("hand_r"));
        Fingers = (Body->GetSocketLocation(TEXT("middle_01_r")) - Hand).GetSafeNormal();
        Across = (Body->GetSocketLocation(TEXT("index_01_r")) - Body->GetSocketLocation(TEXT("pinky_01_r"))).GetSafeNormal();
        Palm = FVector::CrossProduct(Fingers, Across).GetSafeNormal();
    };
    // Places a prop so its bounds centre (or authored pivot) lands on Centre, attached to Bone.
    const auto Put = [Body](UStaticMeshComponent* Prop, FVector Centre, FRotator Rotation, FVector Scale, FName Bone, bool bCentreBounds)
    {
        Prop->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
        const FVector Offset = bCentreBounds ? Rotation.RotateVector(Prop->GetStaticMesh()->GetBounds().Origin * Scale) : FVector::ZeroVector;
        Prop->SetWorldLocationAndRotation(Centre - Offset, Rotation);
        Prop->SetWorldScale3D(Scale);
        Prop->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, Bone);
        Prop->SetVisibility(true);
    };
    if (bPlant)
    {
        if (Stage != 1)
        {
            CarriedSeed->SetVisibility(false);
            return;
        }
        // Between the pinched thumb and forefinger tips.
        const FVector Index = Body->GetSocketLocation(TEXT("index_03_r"));
        const FVector Thumb = Body->GetSocketLocation(TEXT("thumb_03_r"));
        Put(CarriedSeed, (Index + Thumb) * 0.5f, Body->GetSocketRotation(TEXT("hand_r")), FVector(1.0f), TEXT("hand_r"), false);
        return;
    }
    if (bReeds)
    {
        if (Stage != 1)
        {
            CarriedReeds->SetVisibility(false);
            return;
        }
        // Upright through her left fist: the stems run out of the thumb side, cut ends a hand's
        // width below it, the clump narrowed to a gathered bunch.
        const FVector HandL = Body->GetSocketLocation(TEXT("hand_l"));
        const FVector FingersL = (Body->GetSocketLocation(TEXT("middle_01_l")) - HandL).GetSafeNormal();
        const FVector UpL = (Body->GetSocketLocation(TEXT("index_01_l")) - Body->GetSocketLocation(TEXT("pinky_01_l"))).GetSafeNormal();
        const FVector PalmL = -FVector::CrossProduct(FingersL, UpL).GetSafeNormal();
        const FVector Fist = HandL + FingersL * 7.0f + PalmL * 3.0f;
        Put(CarriedReeds, Fist - UpL * 14.0f, FRotationMatrix::MakeFromZX(UpL, FingersL).Rotator(),
            FVector(0.28f, 0.28f, 0.85f), TEXT("hand_l"), false);
        return;
    }
    if (bHarvest)
    {
        if (Stage != 1)
        {
            CarriedForage->SetVisibility(false);
            return;
        }
        // Held between both fists by its crown, hanging below them.
        const FVector Grip = (Body->GetSocketLocation(TEXT("middle_01_r")) + Body->GetSocketLocation(TEXT("middle_01_l"))) * 0.5f;
        const FVector Forward = GetActorForwardVector();
        Put(CarriedForage, Grip - FVector(0, 0, 2.0f), FRotationMatrix::MakeFromZX(FVector::UpVector, Forward).Rotator(),
            FVector(1.0f), TEXT("hand_r"), false);
        return;
    }
    if (bPouch)
    {
        if (Stage != 1 && Stage != 3)
        {
            CarriedForage->SetVisibility(false);
            return;
        }
        FVector Hand, Fingers, Across, Palm;
        HandFrame(Hand, Fingers, Across, Palm);
        const bool bProduce = HarvestProduceMesh && CarriedForage->GetStaticMesh() == HarvestProduceMesh.Get();
        if (bProduce)
        {
            // Crop produce hangs from the pinch by its stalk.
            const FVector Pinch = Hand + Fingers * 8.0f + Palm * 2.5f;
            Put(CarriedForage, Pinch, FRotationMatrix::MakeFromZX(-Fingers, Across).Rotator(), FVector(1.0f), TEXT("hand_r"), false);
            return;
        }
        const bool bAuthored = CarriedForage->GetStaticMesh() == (bForageBerries ? ForageBerryMesh.Get() : ForageRootMesh.Get())
            && CarriedForage->GetStaticMesh() != nullptr;
        // Pinched between thumb and fingers: authored props hang from their pinch pivot, a root
        // points along the fingers.
        const FVector Pinch = Hand + Fingers * 8.0f + Palm * 2.5f;
        const FRotator Rotation = bForageBerries ? FRotationMatrix::MakeFromZX(-Fingers, Across).Rotator()
            : FRotationMatrix::MakeFromZX(Fingers, Across).Rotator();
        const FVector Scale = bAuthored ? FVector(1.0f) : bForageBerries ? FVector(0.045f) : FVector(0.06f, 0.06f, 0.12f);
        Put(CarriedForage, bAuthored ? Pinch : Pinch + (bForageBerries ? FVector::ZeroVector : Fingers * 3.0f), Rotation, Scale,
            TEXT("hand_r"), !bAuthored);
        return;
    }
    // Stones match the woodland pile's components 1 and 2 (StonePileSize).
    const auto StoneScale = [](UStaticMeshComponent* Stone, int32 Index)
    {
        const float Size = Stone->GetStaticMesh()->GetBoundingBox().GetSize().GetMax();
        const bool bHandStone = Stone->GetStaticMesh() == LoadHandStone(Index + 1);
        return FVector(StonePileSize(Index + 1, bHandStone) / FMath::Max(Size, 1.0f));
    };
    // Grip: sticks run across the fingers; a stone sits in the palm.
    auto Grip = [&, this](int32 Index)
    {
        FVector Hand, Fingers, Across, Palm;
        HandFrame(Hand, Fingers, Across, Palm);
        UStaticMeshComponent* Prop = Props[Index];
        if (bStones)
        {
            const FVector Scale = StoneScale(Prop, Index);
            const float Radius = Prop->GetStaticMesh()->GetBoundingBox().GetSize().GetMax() * Scale.X * 0.4f;
            Put(Prop, Hand + Fingers * 6.0f + Palm * (Radius + 1.5f), FRotationMatrix::MakeFromYZ(Across, Palm).Rotator(),
                Scale, TEXT("hand_r"), true);
            return;
        }
        // Branch meshes run along their local Y.
        Put(Prop, Hand + Fingers * 6.0f + Palm * 3.0f, FRotationMatrix::MakeFromYZ(Across, Palm).Rotator(),
            FVector(CarriedStickScale), TEXT("hand_r"), true);
    };
    // Stack: sticks lie level on the left forearm across her body, like carried firewood hugged
    // against the belly; stones nest in the crook of the arm, side by side along the forearm.
    auto Stack = [&, this](int32 Index)
    {
        const FVector Elbow = Body->GetSocketLocation(TEXT("lowerarm_l"));
        const FVector Wrist = Body->GetSocketLocation(TEXT("hand_l"));
        UStaticMeshComponent* Prop = Props[Index];
        if (bStones)
        {
            const FVector Scale = StoneScale(Prop, Index);
            const float Radius = Prop->GetStaticMesh()->GetBoundingBox().GetSize().GetMax() * Scale.X * 0.4f;
            const FVector Centre = FMath::Lerp(Elbow, Wrist, Index == 0 ? 0.3f : 0.62f)
                + FVector(0, 0, Radius + 3.0f) + GetActorForwardVector() * (Index == 0 ? 2.0f : 4.0f);
            Put(Prop, Centre, FRotator(0, GetActorRotation().Yaw + Index * 70.0f, 0), Scale, TEXT("lowerarm_l"), true);
            return;
        }
        const float Height = Index == 0 ? 5.0f : 9.0f, Twist = Index == 0 ? -8.0f : 10.0f;
        const FVector Along = GetActorRightVector().GetSafeNormal2D().RotateAngleAxis(Twist, FVector::UpVector);
        const FVector Centre = FMath::Lerp(Elbow, Wrist, 0.55f) + FVector(0, 0, Height) + GetActorForwardVector() * 3.0f;
        Put(Prop, Centre, FRotationMatrix::MakeFromYZ(Along, FVector::UpVector).Rotator(), FVector(CarriedStickScale),
            TEXT("lowerarm_l"), true);
    };
    if (Stage == 0)
        for (UStaticMeshComponent* Prop : Props) Prop->SetVisibility(false);
    else if (Stage == 1) Grip(0);
    else if (Stage == 2) Stack(0);
    else if (Stage == 3) Grip(1);
    else Stack(1);
}
