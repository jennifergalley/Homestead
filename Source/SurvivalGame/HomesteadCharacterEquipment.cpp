#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadDiggingStick.h"
#include "HomesteadKnife.h"
#include "HomesteadLampLook.h"

#include "Components/PointLightComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/IConsoleManager.h"

// Where the scythe's blade lies, in scythe.py's coordinates (cm, relative to the lower nib grip, the
// pivot): its back and its edge near the heel, in the middle and three quarters along, and its point
// (scythe.py blade_rows). The mowing pose shows the prop mirrored (HomesteadScythe::Mirror), so these
// hold as written: at the clip's 45 degree lean back and edge lie level, and on level ground the
// clip (scythe_mow.py) carries the blade flat just above the soil. On a slope or a hump it would cut
// into the soil uphill and hang in the air downhill.
namespace MowGround
{
const FVector BladeSamples[] = {
    FVector(6.3f, 11.0f, -100.9f), FVector(6.3f, 4.7f, -94.5f),
    FVector(45.8f, 9.2f, -97.8f), FVector(45.8f, 5.2f, -93.8f),
    FVector(67.3f, 5.5f, -94.5f), FVector(67.3f, 3.1f, -92.2f),
    FVector(88.8f, -0.3f, -89.7f)};
// Below this clearance (cm) the blade is lifted clear of the ground...
constexpr float MinClearance = 3.0f;
// ...and above this one (downhill) it is lowered just to it (a continuous target, so gentle downslopes
// don't set it bobbing across the threshold); in between it keeps
// the clip's own lay, so level mowing looks exactly as authored.
constexpr float MaxClearance = 20.0f;
// The roll about the nib line never exceeds this (radians, about 20 degrees): past it the blade
// would stand on its edge rather than lie on the swath.
constexpr float MaxRoll = 0.35f;
// How quickly the roll eases toward what the ground asks (per second).
constexpr float RollRate = 8.0f;
// How far above and below the blade to look for the ground (cm).
constexpr float TraceReach = 150.0f;
// The most the scythe tips up about the lower nib to keep its point out of the ground (radians, ~11 degrees),
// and how fast that eases back down once clear (per second). Enough for a 13-degree uphill swath: the baked mow
// (scythe_mow.py, c7e22276) keeps the blade 4 cm above level ground on every frame it's laid from her fists,
// and a larger tip pulled the upper nib off her left fist.
constexpr float MaxTipUp = 0.2f;
constexpr float TipDownRate = 6.0f;
// A hit this far above a blade sample is foliage or a branch overhead, not the ground under it (cm): the trace
// carries on below it. Deeper than the worst cut into the ground the clip ever made (38 cm).
constexpr float MaxGroundAbove = 60.0f;
}

namespace
{
// Resting tool carries: degrees the head tips down from level (the wrist supplies RestWristDegrees
// of it). Small values carry the tool nearly parallel to the ground. Negative = authored default.
TAutoConsoleVariable<float> CVarCarryHatchet(TEXT("homestead.CarryHatchet"), -1.0f, TEXT("Hatchet carry tilt (deg)."));
TAutoConsoleVariable<float> CVarCarryHoe(TEXT("homestead.CarryHoe"), -1.0f, TEXT("Stone hoe carry tilt (deg)."));
TAutoConsoleVariable<float> CVarCarryMachete(TEXT("homestead.CarryMachete"), -1.0f, TEXT("Machete carry tilt (deg)."));
TAutoConsoleVariable<float> CVarCarryKnife(TEXT("homestead.CarryKnife"), -1.0f, TEXT("Knife carry tilt (deg)."));
}

void AHomesteadCharacter::UpdateHeldTools(float DeltaSeconds)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation || !bMetaHumanActive) return;
    const auto* PC = Cast<AHomesteadController>(Controller);
    // At rest a selected tool rides in her hand; it gives way to authored actions and menus.
    Homestead::Item Presented = Homestead::Item::Count;
    if (PC)
    {
        if (!bAppearancePreview && !PC->IsPlanning() && !PC->IsFailed())
            Presented = PC->IsBookOpen() ? PC->SelectedCarriedTool() : PC->PresentedTool();
        // Crafting by hand needs both hands; the carried tool goes back to her belt meanwhile.
        if (CraftingPhase() >= 0 || Animation->CraftWeight() > 0.01f) Presented = Homestead::Item::Count;
    }
    else if (InCharacterLab() && LabHeldTool && LabCraftUntil < GetWorld()->GetTimeSeconds()) Presented = *LabHeldTool;
    const bool HandsFree = Animation->ActionWeight() < 0.01f && Animation->EatWeight() < 0.01f
        && Animation->CraftWeight() < 0.01f;
    const bool Hacking = Animation->MacheteWeight() > 0.01f;
    const bool Felling = Animation->FellWeight() > 0.01f;
    const bool CuttingReeds = IsCuttingReeds();
    const bool Hoeing = bHoeTill && Animation->TillWeight() > 0.01f;
    // Filling or pouring with the carved pail's own clips: the pail stays in her hand throughout.
    const bool PailWork = Animation->WaterWeight() > 0.01f && GetWaterAnimation() != WaterAnimation;
    bool bPouring = false;
    float Grip = 0, Carry = RestWristDegrees;
    // At rest a tool's handle crosses the palm diagonally (heel of the hand to the index knuckle),
    // which tips its head forward and down with the wrist nearly straight. Authored actions set
    // the tool's angle themselves, so the tilt eases out while one plays.
    const float TiltTarget = HandsFree && !Hacking && !Felling && !CuttingReeds && !Hoeing ? 1.0f : 0.0f;
    HeldToolTilt = FMath::FInterpConstantTo(HeldToolTilt, TiltTarget, DeltaSeconds, 1.0f / 0.15f);
    const auto Tilt = [this](const FTransform& Rest, float Degrees)
    {
        // Grip-local X is the palm normal; a positive turn about it tips the head toward the fingertips.
        return FTransform(FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Degrees * HeldToolTilt))) * Rest;
    };
    if (HeldMachete)
    {
        const bool Held = (Hacking && HackTool == Homestead::Item::Machete) || (HandsFree && Presented == Homestead::Item::Machete);
        HeldMachete->SetVisibility(Held);
        if (Held)
        {
            Grip = 1;
            const float MacheteCarry = CVarCarryMachete.GetValueOnGameThread() >= 0
                ? CVarCarryMachete.GetValueOnGameThread() : MacheteCarryDegrees;
            HeldMachete->SetRelativeTransform(Tilt(MacheteGrip, MacheteCarry - RestWristDegrees));
        }
    }
    for (int32 Index = 0; Index < HeldProps.Num(); ++Index)
    {
        UStaticMeshComponent* Prop = HeldProps[Index];
        const FHeldToolSpec& Spec = HeldToolSpecs[Index];
        const bool Held = Spec.Tool == FellTool && Felling
            || Spec.Tool == Homestead::Item::Knife && CuttingReeds
            || Spec.Tool == Homestead::Item::DiggingStick && Hoeing
            || (Hacking && Spec.Tool == HackTool)
            || Spec.Tool == Homestead::Item::WateringCan && PailWork
            || (HandsFree && !Hacking && Presented == Spec.Tool);
        Prop->SetVisibility(Held);
        if (!Held) continue;
        Grip = 1;
        const bool StoneHoe = Spec.Tool == Homestead::Item::DiggingStick && Prop->GetStaticMesh()
            && (Prop->GetStaticMesh()->GetName() == TEXT("SM_DrawHoe") || Prop->GetStaticMesh()->GetName() == TEXT("SM_StoneHoe"));
        const float Tuned = Spec.Tool == Homestead::Item::Hatchet ? CVarCarryHatchet.GetValueOnGameThread()
            : Spec.Tool == Homestead::Item::Knife ? CVarCarryKnife.GetValueOnGameThread()
            : StoneHoe ? CVarCarryHoe.GetValueOnGameThread() : -1.0f;
        const float CarryDegrees = Tuned >= 0 ? Tuned : Spec.CarryDegrees;
        // The authored saw stroke drives the wrist; the resting carry deviation would skew the blade.
        Carry = CuttingReeds || Hoeing ? 0.0f : Hacking ? RestWristDegrees : FMath::Min(CarryDegrees, RestWristDegrees);
        // Resting carries that differ from the working grip: the hatchet and knife hang edge-down and the
        // pickaxe point-down (Jenny, 09-29: it rode point-up), turned about the haft; the hoe is carried blade-low in front, turned end for end from
        // how she works it. The turn eases out with the tilt when an authored action takes over.
        FQuat Flip = FQuat::Identity;
        float Slide = 0;
        if (Spec.Tool == Homestead::Item::Hatchet || Spec.Tool == Homestead::Item::Knife || Spec.Tool == Homestead::Item::Pickaxe)
            Flip = FQuat(FVector::ZAxisVector, PI);
        else if (StoneHoe)
        {
            // Carried, her hand rides near the top of the haft so its end clears her hip.
            Flip = FQuat(FVector::XAxisVector, PI);
            Slide = -26.0f;
        }
        // The hoe keeps its carried grip through the tilling clip (hoe_till.py is authored for
        // it), so nothing turns in her hand as she starts or stops. The hatchet keeps its turn
        // too: turned, its edge runs along her knuckles, which is how she swings it.
        // The pickaxe too: the two-handed strike lays it from both fists regardless.
        const float TurnWeight = StoneHoe || Spec.Tool == Homestead::Item::Hatchet || Spec.Tool == Homestead::Item::Pickaxe
            ? 1.0f : HeldToolTilt;
        // hoe_till.py solves her right hand for the imported blade on +Y, so the hoe no longer
        // rolls in her fist as she sets it (that half-turn folded her wrist back on the forearm).
        const FTransform Turn = FTransform(FVector(0, 0, Slide * TurnWeight))
            * FTransform(FQuat::Slerp(FQuat::Identity, Flip, TurnWeight));
        const float Lean = CarryDegrees - (StoneHoe ? FMath::Min(CarryDegrees, RestWristDegrees) : Carry);
        const FTransform HeldPose = StoneHoe
            ? FTransform(FQuat(FVector::XAxisVector, FMath::DegreesToRadians(Lean))) * Spec.Rest
            : Tilt(Spec.Rest, Lean);
        if (!Spec.bHangs) Prop->SetRelativeTransform(Turn * HeldPose);
        if (Spec.bHangs)
        {
            // Hanging from the fist unless the pour lays it in both hands (UpdateWaterPail).
            Prop->SetRelativeLocation(Spec.Rest.GetLocation());
            if (PailWork && !bFillingPail) { UpdateWaterPail(*Prop, DeltaSeconds); bPouring = true; }
            else UpdateHangingPail(*Prop, DeltaSeconds);
        }
    }
    const bool bLampHeld = UpdateHeldLamp(*Animation, HandsFree && !Hacking && !Felling, Presented, DeltaSeconds);
    if (bLampHeld)
    {
        // Her fist closes on the bail with the wrist straight; the lamp hangs plumb below it.
        Grip = 1;
        Carry = 0;
    }
    if (PourStream && !bPouring) PourStream->SetVisibility(false);
    if (!bLampHeld && !HeldProps.ContainsByPredicate([](const UStaticMeshComponent* Prop) { return Prop->IsVisible(); })) bPailHandValid = false;
    Animation->SetRightHandGrip(Grip, Carry);
    // She ticks after the pose is final (TG_PostUpdateWork), so the felling haft is laid through
    // both fists here, over the one-handed placement just set.
    UpdateFellingHatchet();
    UpdateCraftPiece(Animation->CraftWeight());
}

bool AHomesteadCharacter::UpdateHeldLamp(UHomesteadAnimInstance& Animation, bool bHandsFree, Homestead::Item Presented, float DeltaSeconds)
{
    LampFlickerTime += DeltaSeconds;
    bool bShow = false;
    if (LampKneel != ELampKneel::None)
    {
        const float Phase = Animation.LampKneelPhase();
        if (Animation.IsLampKneeling() || Phase >= 0.0f)
        {
            // Setting down, it leaves her hand at the contact; picking up, it arrives there.
            const bool bBeforeContact = Phase < LampContactSeconds();
            bShow = LampKneel == ELampKneel::SetDown ? bBeforeContact : !bBeforeContact;
            if (!bBeforeContact && !bLampContactDone) bLampContactPending = bLampContactDone = true;
        }
        else LampKneel = ELampKneel::None;
    }
    if (LampKneel == ELampKneel::None) bShow = bHandsFree && Presented == Homestead::Item::OilLamp;
    Animation.SetLampRaised(bShow && LampKneel == ELampKneel::None);
    UStaticMeshComponent* Flame = HeldLampParts.IsEmpty() ? nullptr : HeldLampParts.Last().Get();
    for (UStaticMeshComponent* Part : HeldLampParts)
        if (Part && Part != Flame && Part->IsVisible() != bShow) Part->SetVisibility(bShow);
    const bool bLit = bShow && (bHeldLampLit || InCharacterLab());
    HomesteadLampLook::SetLit(Flame, HeldLampLight, bLit, LampFlickerTime, HeldLampParts.Num() >= 3 ? HeldLampParts[1].Get() : nullptr);
    if (bShow && LampHanger) UpdateHangingPail(*LampHanger, DeltaSeconds);
    return bShow && !HeldLampParts.IsEmpty();
}

bool AHomesteadCharacter::PlayLampKneel(Homestead::Point Spot, bool bSetDown)
{
    auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!bMetaHumanActive || !Animation || !GetLampSetDownAnimation() || HeldLampParts.IsEmpty() || Animation->IsLampKneeling())
        return false;
    CancelSprint();
    GetCharacterMovement()->StopMovementImmediately();
    const FVector2D Delta(Spot.x - GetActorLocation().X, Spot.y - GetActorLocation().Y);
    if (!Delta.IsNearlyZero()) SetActorRotation(FRotator(0, FMath::RadiansToDegrees(FMath::Atan2(Delta.Y, Delta.X)), 0));
    LampKneel = bSetDown ? ELampKneel::SetDown : ELampKneel::PickUp;
    bLampContactPending = bLampContactDone = false;
    Animation->RequestLampKneel();
    return true;
}

bool AHomesteadCharacter::ConsumeLampContact()
{
    const bool bContact = bLampContactPending;
    bLampContactPending = false;
    return bContact;
}

bool AHomesteadCharacter::IsLampKneeling() const
{
    return LampKneel != ELampKneel::None;
}

float AHomesteadCharacter::LampContactSeconds() const
{
    // The stick-gather stand-in lays its first stick on the ground at its first pick.
    return LampSetDownAnimation ? LampSetDownContact : 38.0f / 30.0f;
}

void AHomesteadCharacter::UpdateCraftPiece(float Weight)
{
    if (!CraftPiece) return;
    // Shown once her fists have mostly closed on it, so it never floats beside an open hand.
    const bool bShow = Weight > 0.6f;
    if (!bShow)
    {
        if (CraftPiece->IsVisible()) CraftPiece->SetVisibility(false);
        return;
    }
    // Upright through the left fist: along the knuckles (index above pinky, thumb up), centred
    // a little above the fist so its top runs through the right fist (homestead_agent.craft_hands).
    USkeletalMeshComponent* Body = GetMesh();
    const FVector Hand = Body->GetSocketLocation(TEXT("hand_l"));
    const FVector Fingers = (Body->GetSocketLocation(TEXT("middle_01_l")) - Hand).GetSafeNormal();
    const FVector Across = (Body->GetSocketLocation(TEXT("index_01_l")) - Body->GetSocketLocation(TEXT("pinky_01_l"))).GetSafeNormal();
    const FVector Palm = FVector::CrossProduct(Across, Fingers).GetSafeNormal();
    const FVector Grip = Hand + Fingers * 6.5f + Palm * 3.0f;
    // Branch meshes run along their local Y.
    const FRotator Rotation = FRotationMatrix::MakeFromYZ(Across, Palm).Rotator();
    const FVector Scale(CraftPieceScale);
    const FVector Offset = Rotation.RotateVector(CraftPiece->GetStaticMesh()->GetBounds().Origin * Scale);
    if (CraftPiece->GetAttachParent() != Body || CraftPiece->GetAttachSocketName() != TEXT("hand_l"))
        CraftPiece->AttachToComponent(Body, FAttachmentTransformRules::KeepWorldTransform, TEXT("hand_l"));
    CraftPiece->SetWorldLocationAndRotation(Grip + Across * 6.0f - Offset, Rotation);
    CraftPiece->SetWorldScale3D(Scale);
    CraftPiece->SetVisibility(true);
}

void AHomesteadCharacter::UpdateFellingHatchet()
{
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    UStaticMeshComponent* Prop = GetHeldProp(FellTool);
    if (!Animation || !Prop || !Prop->IsVisible()) return;
    const float Weight = Animation->FellWeight();
    if (Weight <= 0.01f)
    {
        MowGroundRoll = MowTipUp = 0.0f;
        return;
    }
    if (FellTool == Homestead::Item::Scythe)
    {
        UpdateMowingScythe(*Prop, Weight);
        return;
    }
    // The left fist holds the knob and the right closes just above it (axe_fell.py): the haft
    // runs up from the left grip centre, the edge from the left knuckles.
    USkeletalMeshComponent* Body = GetMesh();
    const auto GripCentre = [Body](const TCHAR* Side, FVector& Along)
    {
        const FVector Hand = Body->GetSocketLocation(*FString::Printf(TEXT("hand_%s"), Side));
        const FVector Knuckle = Body->GetSocketLocation(*FString::Printf(TEXT("middle_01_%s"), Side));
        const FVector Across = Body->GetSocketLocation(*FString::Printf(TEXT("index_01_%s"), Side))
            - Body->GetSocketLocation(*FString::Printf(TEXT("pinky_01_%s"), Side));
        Along = (Knuckle - Hand).GetSafeNormal();
        const bool bLeft = Side[0] == TEXT('l');
        const FVector Palm = (bLeft ? FVector::CrossProduct(Across, Along) : FVector::CrossProduct(Along, Across)).GetSafeNormal();
        return Hand + (Knuckle - Hand) * 0.75f + Palm * 3.3f;
    };
    FVector AlongL;
    const FVector Knob = GripCentre(TEXT("l"), AlongL);
    // The knob fist sits rolled on the haft through the felling clip so her wrist stays in line
    // with the forearm, and the right fist rolls with the swing (axe_fell.py KNOB_ROLL, ROLL_R);
    // turning the knob's knuckles back by its roll gives the edge. The strike clips (stumps, logs,
    // the pickaxe) keep the knuckles on the edge.
    constexpr float KnobRollDegrees = 105.0f;
    const bool bFellingTree = FellTool == Homestead::Item::Hatchet && !bStrikeHatchet;
    // Both fists stay together at the base of the haft (axe_fell.py), so their spacing can't set
    // the line; each closed fist's pinky-to-index axis runs along the haft. That axis slants
    // about 16 degrees toward the fingers, so with the fists rolled differently the felling clip
    // squares it to each hand's fingers first, as axe_fell.py authors it.
    const auto Across = [Body, bFellingTree](const TCHAR* Side)
    {
        const FVector Hand = Body->GetSocketLocation(*FString::Printf(TEXT("hand_%s"), Side));
        const FVector Along = (Body->GetSocketLocation(*FString::Printf(TEXT("middle_01_%s"), Side)) - Hand).GetSafeNormal();
        const FVector Raw = (Body->GetSocketLocation(*FString::Printf(TEXT("index_01_%s"), Side))
            - Body->GetSocketLocation(*FString::Printf(TEXT("pinky_01_%s"), Side))).GetSafeNormal();
        return bFellingTree ? (Raw - Along * FVector::DotProduct(Raw, Along)).GetSafeNormal() : Raw;
    };
    const FVector AcrossL = Across(TEXT("l"));
    const FVector AcrossR = Across(TEXT("r"));
    const FVector Haft = (AcrossL + AcrossR).GetSafeNormal().IsNearlyZero() ? AcrossL : (AcrossL + AcrossR).GetSafeNormal();
    const FVector Knuckles = bFellingTree ? AlongL.RotateAngleAxis(-KnobRollDegrees, Haft) : AlongL;
    const FVector Edge = (Knuckles - Haft * FVector::DotProduct(Knuckles, Haft)).GetSafeNormal();
    if (Edge.IsNearlyZero()) return;
    // The imported hatchet's edge is on +Y (the Blender export mirrors Y; the report says -Y), so
    // +Y goes along the edge to face the tree.
    const FTransform TwoHanded(FRotationMatrix::MakeFromZY(Haft, Edge).ToQuat(), Knob, Prop->GetComponentScale());
    const FTransform OneHanded = Prop->GetComponentTransform();
    FTransform Blended;
    Blended.Blend(OneHanded, TwoHanded, FMath::SmoothStep(0.0f, 1.0f, Weight));
    Prop->SetWorldTransform(Blended);
}

void AHomesteadCharacter::UpdateMowingScythe(UStaticMeshComponent& Prop, float Weight)
{
    // The right fist closes on the lower nib (the prop's pivot, the nib along the fist's pinky to
    // index axis toward the snath) and the left on the upper nib 42 cm up the snath (scythe_mow.py).
    USkeletalMeshComponent* Body = GetMesh();
    const auto GripCentre = [Body](const TCHAR* Side)
    {
        const FVector Hand = Body->GetSocketLocation(*FString::Printf(TEXT("hand_%s"), Side));
        const FVector Knuckle = Body->GetSocketLocation(*FString::Printf(TEXT("middle_01_%s"), Side));
        const FVector Across = Body->GetSocketLocation(*FString::Printf(TEXT("index_01_%s"), Side))
            - Body->GetSocketLocation(*FString::Printf(TEXT("pinky_01_%s"), Side));
        const FVector Along = (Knuckle - Hand).GetSafeNormal();
        const bool bLeft = Side[0] == TEXT('l');
        const FVector Palm = (bLeft ? FVector::CrossProduct(Across, Along) : FVector::CrossProduct(Along, Across)).GetSafeNormal();
        return Hand + (Knuckle - Hand) * 0.78f + Palm * 2.6f;
    };
    const FVector Lower = GripCentre(TEXT("r")), Upper = GripCentre(TEXT("l"));
    const FVector Nib = (Body->GetSocketLocation(TEXT("index_01_r")) - Body->GetSocketLocation(TEXT("pinky_01_r"))).GetSafeNormal();
    FVector Snath = Upper - Lower;
    Snath = (Snath - Nib * FVector::DotProduct(Snath, Nib)).GetSafeNormal();
    if (Nib.IsNearlyZero() || Snath.IsNearlyZero()) return;
    // scythe_mow.py's frame: the nib (prop +Y) along the right fist and the snath (+Z) toward the upper
    // grip, in scythe.py's coordinates, so the prop is shown mirrored (HomesteadScythe::Mirror) as at
    // rest; the ground probe (Rolled, below) places scythe.py points without the mirror.
    const FVector Scale = Prop.GetComponentScale().GetAbs();
    FTransform TwoHanded(FRotationMatrix::MakeFromYZ(Nib, Snath).ToQuat(), Lower, Scale * HomesteadScythe::Mirror);
    // Follow the ground: roll the whole scythe about the line through both nib grips, so both fists
    // stay on their nibs while the blade tilts clear of rising ground or down onto falling ground.
    const FVector NibLine = (Upper - Lower).GetSafeNormal();
    UWorld* World = GetWorld();
    float WantRoll = 0.0f;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadMowGround), false, this);
    const auto GroundUnder = [World, &Query](const FVector& At, float& Height)
    {
        if (!World) return false;
        FVector Start = At + FVector(0, 0, MowGround::TraceReach);
        const FVector End = At - FVector(0, 0, MowGround::TraceReach);
        // A bush or bough overhead isn't the ground: look again from just under it (a few times at most).
        for (int32 Try = 0; Try < 3; ++Try)
        {
            FHitResult Hit;
            if (!World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Query)) return false;
            if (Hit.ImpactPoint.Z - At.Z <= MowGround::MaxGroundAbove)
            {
                Height = static_cast<float>(Hit.ImpactPoint.Z);
                return true;
            }
            Start = Hit.ImpactPoint - FVector(0, 0, 2.0);
        }
        return false;
    };
    if (World && !NibLine.IsNearlyZero())
    {
        // Solve on the unrolled lay, twice (the second pass corrects the first's small-angle guess).
        for (int32 Pass = 0; Pass < 2; ++Pass)
        {
            const FTransform Rolled(FQuat(NibLine, WantRoll) * TwoHanded.GetRotation(), Lower, Scale);
            float Lowest = TNumericLimits<float>::Max(), LiftPerRadian = 0.0f;
            bool bAny = false;
            for (const FVector& Sample : MowGround::BladeSamples)
            {
                const FVector At = Rolled.TransformPosition(Sample);
                float Height = 0.0f;
                if (!GroundUnder(At, Height)) continue;
                const float Clearance = static_cast<float>(At.Z) - Height;
                if (Clearance < Lowest)
                {
                    Lowest = Clearance;
                    // How fast this sample rises per radian of roll about the nib line.
                    LiftPerRadian = static_cast<float>(FVector::CrossProduct(NibLine, At - Lower).Z);
                    bAny = true;
                }
            }
            if (!bAny || FMath::Abs(LiftPerRadian) < 1.0f) break;
            const float Shortfall = Lowest < MowGround::MinClearance ? MowGround::MinClearance - Lowest
                : Lowest > MowGround::MaxClearance ? MowGround::MaxClearance - Lowest : 0.0f;
            if (Shortfall == 0.0f) break;
            WantRoll = FMath::Clamp(WantRoll + Shortfall / LiftPerRadian, -MowGround::MaxRoll, MowGround::MaxRoll);
        }
    }
    const float Dt = World ? World->GetDeltaSeconds() : 0.0f;
    MowGroundRoll = FMath::FInterpTo(MowGroundRoll, WantRoll, Dt, MowGround::RollRate);
    TwoHanded.SetRotation(FQuat(NibLine, MowGroundRoll) * TwoHanded.GetRotation());
    FTransform Blended;
    Blended.Blend(Prop.GetComponentTransform(), TwoHanded, FMath::SmoothStep(0.0f, 1.0f, Weight));
    // The eased ground roll can't catch everything: in the wind-up the nib line yaws fast and the blade's point
    // lies nearly in the vertical plane through the grip line, where no roll about it lifts the point, and it
    // went up to 38 cm into the ground (PIE, 09-30). Tip the scythe up about the lower nib (her right fist stays
    // on it) just enough to clear: at once on the way up, easing back down slowly. Its axis follows the blade's
    // mean direction, so a change of lowest sample can't jump it; hits far overhead are skipped (GroundUnder).
    {
        const FTransform Laid(Blended.GetRotation(), Blended.GetLocation(), Scale);
        FVector Mean = FVector::ZeroVector;
        float Lowest = TNumericLimits<float>::Max();
        FVector LowestAt = FVector::ZeroVector;
        for (const FVector& Sample : MowGround::BladeSamples)
        {
            const FVector At = Laid.TransformPosition(Sample);
            Mean += At / UE_ARRAY_COUNT(MowGround::BladeSamples);
            float Height = 0.0f;
            if (GroundUnder(At, Height) && At.Z - Height < Lowest)
            {
                Lowest = static_cast<float>(At.Z - Height);
                LowestAt = At;
            }
        }
        const FVector Out = FVector(Mean.X - Lower.X, Mean.Y - Lower.Y, 0.0).GetSafeNormal();
        const float Lever = static_cast<float>(FVector::DotProduct(LowestAt - Lower, Out));
        float WantTip = 0.0f;
        if (!Out.IsNearlyZero() && Lowest < MowGround::MinClearance && Lever > 20.0f)
            WantTip = FMath::Min(FMath::Asin(FMath::Min(1.0f, (MowGround::MinClearance - Lowest) / Lever)), MowGround::MaxTipUp);
        MowTipUp = FMath::Max(WantTip, FMath::FInterpTo(MowTipUp, WantTip, Dt, MowGround::TipDownRate));
        if (!Out.IsNearlyZero() && MowTipUp > 0.0f)
        {
            const FQuat Tip(FVector::CrossProduct(Out, FVector::UpVector).GetSafeNormal(), MowTipUp);
            Blended.SetLocation(Lower + Tip.RotateVector(Blended.GetLocation() - Lower));
            Blended.SetRotation(Tip * Blended.GetRotation());
        }
    }
    Prop.SetWorldTransform(Blended);
}

void AHomesteadCharacter::SetLabHeldTool(Homestead::Item Tool)
{
    if (Tool == Homestead::Item::Count) LabHeldTool.Reset();
    else LabHeldTool = Tool;
}

UStaticMeshComponent* AHomesteadCharacter::GetHeldProp(Homestead::Item Tool) const
{
    if (!bMetaHumanActive) return nullptr;
    if (Tool == Homestead::Item::Machete) return HeldMachete;
    for (int32 Index = 0; Index < HeldToolSpecs.Num(); ++Index)
        if (HeldToolSpecs[Index].Tool == Tool) return HeldProps[Index];
    return nullptr;
}

void AHomesteadCharacter::UpdateHangingPail(USceneComponent& Pail, float DeltaSeconds)
{
    // A pendulum hanging from the bail: the hand's horizontal acceleration swings the pail the
    // other way, then gravity (a ~25 cm pendulum) and a little damping settle it plumb.
    const FVector Hand = Pail.GetComponentLocation();
    const float Dt = FMath::Clamp(DeltaSeconds, 1.0f / 240.0f, 1.0f / 20.0f);
    FVector2D Push = FVector2D::ZeroVector;
    if (bPailHandValid)
    {
        const FVector Velocity = (Hand - PailHandLast) / Dt;
        const FVector Smoothed = FMath::Lerp(PailHandVelocity, Velocity, FMath::Min(1.0f, Dt * 20.0f));
        const FVector Acceleration = (Smoothed - PailHandVelocity) / Dt;
        PailHandVelocity = Smoothed;
        Push = FVector2D(-Acceleration.X, -Acceleration.Y) / 980.0f;
        Push = Push.ClampAxes(-0.5f, 0.5f);
    }
    else
    {
        PailSwing = PailSwingRate = FVector2D::ZeroVector;
        PailHandVelocity = FVector::ZeroVector;
    }
    PailHandLast = Hand;
    bPailHandValid = true;
    constexpr float Stiffness = 980.0f / 25.0f, Damping = 3.5f;
    PailSwingRate += (-(PailSwing - Push) * Stiffness - PailSwingRate * Damping) * Dt;
    PailSwing = (PailSwing + PailSwingRate * Dt).ClampAxes(-0.7f, 0.7f);
    // The bail runs fore and aft in her fist; the spout faces out to her right.
    const FQuat Yaw(FVector::UpVector, FMath::DegreesToRadians(GetActorRotation().Yaw + 90.0f));
    const FQuat Swing = FQuat::FindBetweenNormals(-FVector::UpVector, FVector(PailSwing.X, PailSwing.Y, -1.0f).GetSafeNormal());
    Pail.SetWorldRotation(Swing * Yaw);
}

void AHomesteadCharacter::UpdateWaterPail(UStaticMeshComponent& Pail, float DeltaSeconds)
{
    // SM_WaterPail's pouring lip in its own space (pivot at the bail grip, hanging -Z, lip +X).
    static const FVector PailLipLocal(11.5f, 0.0f, -14.0f);
    // pail_pour.GRIP_DROP / GRIP_RADIUS: she holds its sides this far below the pivot.
    constexpr float GripDrop = 18.0f;
    constexpr float GripRadius = 11.5f;
    // Keep the pendulum running so the pail swings on naturally once she lets go.
    UpdateHangingPail(Pail, DeltaSeconds);
    const auto* Animation = Cast<UHomesteadAnimInstance>(GetMesh()->GetAnimInstance());
    if (!Animation) return;
    const float Time = Animation->WaterPhase();
    const float Hold = Animation->WaterWeight() * FMath::SmoothStep(PailTakeStart, PailTakeEnd, Time)
        * (1.0f - FMath::SmoothStep(PailGiveStart, PailGiveEnd, Time));
    if (Hold > 0.001f)
    {
        // Between the take and the give the pail rides against her left palm like a pot: its
        // side at the palm, its axis up her fingers, the lip ahead (the right hand mirrors it).
        USkeletalMeshComponent* Body = GetMesh();
        const FVector HandL = Body->GetSocketLocation(TEXT("hand_l"));
        const FVector KnuckleL = Body->GetSocketLocation(TEXT("middle_01_l"));
        const FVector FingersL = (KnuckleL - HandL).GetSafeNormal();
        const FVector AcrossL = Body->GetSocketLocation(TEXT("index_01_l")) - Body->GetSocketLocation(TEXT("pinky_01_l"));
        const FVector PalmL = FVector::CrossProduct(AcrossL, FingersL).GetSafeNormal();
        const FVector Up = FVector::VectorPlaneProject(FingersL, PalmL).GetSafeNormal();
        if (!PalmL.IsNearlyZero() && !Up.IsNearlyZero())
        {
            const FVector PalmPoint = HandL + (KnuckleL - HandL) * 0.6f + PalmL * 2.0f;
            const FTransform Held(FRotationMatrix::MakeFromYZ(PalmL, Up).ToQuat(),
                PalmPoint + PalmL * GripRadius + Up * GripDrop, Pail.GetComponentScale());
            FTransform Placed;
            Placed.Blend(Pail.GetComponentTransform(), Held, FMath::SmoothStep(0.0f, 1.0f, Hold));
            Pail.SetWorldTransform(Placed);
        }
    }

    if (!PourStream) return;
    const float Flow = Hold * FMath::SmoothStep(PailPourStart - 0.1f, PailPourStart + 0.15f, Time)
        * (1.0f - FMath::SmoothStep(PailPourStop - 0.15f, PailPourStop + 0.1f, Time));
    if (Flow < 0.02f || !GetWorld())
    {
        PourStream->SetVisibility(false);
        return;
    }
    // The water runs from the pouring lip straight down to the soil.
    const FVector LipPoint = Pail.GetComponentTransform().TransformPosition(PailLipLocal);
    FHitResult Hit;
    const FCollisionQueryParams Query(SCENE_QUERY_STAT(HomesteadPourStream), false, this);
    const FVector Reach = LipPoint - FVector(0, 0, 250);
    const FVector Ground = GetWorld()->LineTraceSingleByChannel(Hit, LipPoint, Reach, ECC_Visibility, Query) ? Hit.ImpactPoint : Reach;
    const float Length = FMath::Max(1.0f, FVector::Dist(LipPoint, Ground));
    const float Width = FMath::Lerp(1.5f, 3.6f, Flow) * (1.0f + 0.08f * FMath::Sin(GetWorld()->GetTimeSeconds() * 37.0f));
    PourStream->SetWorldLocationAndRotation((LipPoint + Ground) * 0.5f, FRotationMatrix::MakeFromZ(LipPoint - Ground).ToQuat());
    PourStream->SetWorldScale3D(FVector(Width / 100.0f, Width / 100.0f, Length / 100.0f));
    PourStream->SetVisibility(true);
}
