// Fishing tackle presentation (refine-fishing-art-and-casting): the flax line from the hazel pole's
// tip, the float flying out on the cast and riding the water, and the fish coming up on the line at
// the catch. Timing follows AN_HeroineMH_Fishing (homestead_agent.fish_cast) through the anim
// instance's clip time; the rules (bite delay, hook window, reward) live in the simulation.
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadFishingPresentation.h"
#include "Simulation/HomesteadItems.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "CollisionQueryParams.h"
#include "Materials/MaterialInstanceDynamic.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadFishingTackle, Log, All);

namespace HomesteadFishingTackle
{
// The working tip of SM_FishingPole in the prop's frame (fishing_pole.py REPORT tip_cm, with the
// Blender export's Y mirror).
const FVector PoleTip(3.6f, 1.0f, 175.0f);
constexpr int32 LineSegments = 8;
// Flax line diameter as shown (cm): a touch thicker than the 0.9 mm cord so it reads on screen.
constexpr float LineDiameter = 0.35f;
// Before the release the float hangs this far below the tip (cm).
constexpr float DangleCm = 38.0f;
// Where the float lands: beyond the bank, off to one side (the controller picks a side that's over
// open water, SetFishingCastYaw), so the float and its rings sit clear of her silhouette from the
// chase camera behind her (cm).
constexpr float CastReachCm = AHomesteadCharacter::FishingCastReachCm;
// Peak of the float's flight above the straight line from release to landing (cm).
constexpr float FlightArcCm = 140.0f;
// The water is looked for this far above and below her feet (cm); a bank higher than that leaves
// the float at the lower bound.
constexpr float WaterAboveFeetCm = 20.0f;
constexpr float WaterBelowFeetCm = 90.0f;
// Line sag as a fraction of its span: slack while waiting, nearly taut with a fish on.
constexpr float SlackSag = 0.07f;
constexpr float TautSag = 0.008f;
// Float motion (cm, Hz): a lazy bob, the fight's drag across while the fish rests between runs.
constexpr float WaitBobCm = 0.6f, WaitBobHz = 0.5f;
constexpr float FightDownCm = 6.0f, FightSwayCm = 10.0f, FightSwayHz = 0.9f;
constexpr float FightTrembleCm = 0.4f, FightTrembleHz = 9.0f, FightTiltDegrees = 14.0f;
// A false nibble (Homestead::Fishing::Nibble): a small dip, a sideways shiver and a tilt.
constexpr float NibbleDipCm = 1.8f, NibbleShakeCm = 1.2f, NibbleShakeHz = 13.0f, NibbleTiltDegrees = 16.0f;
// The float's depth follows a spring toward "under" or "riding" (Jenny, 2026-10-09: no sudden pop in or
// out). Pulled under it eases down, critically damped, in about SinkSeconds (the ring starts at once);
// let go it rises in about RiseSeconds and bobs once past the surface (RiseDamping < 1).
constexpr float SinkSeconds = 0.35f, RiseSeconds = 0.6f, RiseDamping = 0.45f;
// How far below its riding depth the float is drawn when fully under (cm, before display scale).
constexpr float SunkDepthCm = 10.0f;
// SM_FishingFloat (fishing_float.py REPORT): the paint seam rides the water.
constexpr float FloatWaterlineCm = 4.5f, FloatTopCm = 11.0f;
// Shown well above its authored 48 mm cork, like the readable bobbers of cosy fishing games, so it
// reads from the gameplay camera about 9-10 m away (4.7 m arm plus the 4.8 m cast).
constexpr float FloatDisplayScale = 2.8f;
// Near her (dangling, flying out, reeled back) it shows at its true size, growing to the display size
// over this distance from her (cm), so it never looks oversized in her hand.
constexpr float FloatGrowCm = 300.0f;
const TCHAR* const FloatMeshPath = TEXT("/Game/SurvivalGame/Environment/Props/FishingFloat/SM_FishingFloat.SM_FishingFloat");
// The fish under the water before it's caught (Jenny, 2026-10-09): it swims in from SwimStartCm out,
// rising from SwimStartDepthCm to SwimDepthCm (the water's absorption fades it in), arriving partway
// through the wait so its arrival doesn't announce the bite; then it circles the float at
// LoiterRadiusCm and darts in to nose it on each false nibble. Hooked, it holds under the float,
// thrashing. cm, seconds, radians/s, degrees, Hz.
// The estate water is dark and absorbs fast, so the fish swims just under the surface to be seen.
constexpr float SwimStartCm = 300.0f, SwimStartDepthCm = 35.0f, SwimDepthCm = 6.0f;
constexpr float SwimArriveMinFraction = 0.3f, SwimArriveMaxFraction = 0.6f, SwimMinApproachSeconds = 1.5f;
constexpr float LoiterRadiusCm = 45.0f, LoiterSpeed = 0.6f, NoseRadiusCm = 10.0f;
constexpr float SwimWiggleDegrees = 7.0f, SwimWiggleHz = 2.2f, ThrashDegrees = 22.0f, ThrashHz = 6.0f;
constexpr float SwimFollow = 6.0f, SwimScale = 1.4f;
// The fishing camera: pitched down over the water, centred FishCamMidFraction of the way from her
// feet to the float, eased in and out over FishCamBlendSeconds (degrees, cm, seconds).
constexpr float FishCamPitch = -50.0f, FishCamArmCm = 620.0f, FishCamMidFraction = 0.5f, FishCamBlendSeconds = 0.9f;
constexpr float FishCamPivotLiftCm = 60.0f;
// How long after the catch's lift the fish takes to swing up into her left hand (s).
constexpr float SwingInSeconds = 0.55f;
const FLinearColor FlaxColour(0.72f, 0.64f, 0.48f);
const FLinearColor FloatColour(0.55f, 0.16f, 0.08f);
const TCHAR* const FishMeshes[] = {
    TEXT("SM_RiverTrout"), TEXT("SM_RiverSalmon"), TEXT("SM_LakePerch"),
    TEXT("SM_LakeCarp"), TEXT("SM_SeaMackerel"), TEXT("SM_SeaBass")};
}

void AHomesteadCharacter::CreateFishingTackle()
{
    using namespace HomesteadFishingTackle;
    if (FishingFloat) return;
    UStaticMesh* Cylinder = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Shape = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Cylinder || !Sphere) return;
    const auto MakeLoose = [this](const FName Name, UStaticMesh* PartMesh, const FLinearColor& Colour, UMaterialInterface* Base)
    {
        auto* Part = NewObject<UStaticMeshComponent>(this, Name);
        Part->SetupAttachment(GetMesh());
        Part->SetStaticMesh(PartMesh);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetUsingAbsoluteLocation(true);
        Part->SetUsingAbsoluteRotation(true);
        Part->SetUsingAbsoluteScale(true);
        Part->SetCastShadow(false);
        Part->SetVisibility(false);
        Part->RegisterComponent();
        if (Base)
            if (UMaterialInstanceDynamic* Tint = UMaterialInstanceDynamic::Create(Base, Part))
            {
                Tint->SetVectorParameterValue(TEXT("Color"), Colour);
                Part->SetMaterial(0, Tint);
            }
        return Part;
    };
    FishingLine.Reset();
    for (int32 Index = 0; Index < LineSegments; ++Index)
        FishingLine.Add(MakeLoose(*FString::Printf(TEXT("FishingLine%d"), Index), Cylinder, FlaxColour, Shape));
    FishingFloat = MakeLoose(TEXT("FishingFloat"), Sphere, FloatColour, Shape);
    if (UStaticMesh* Painted = LoadObject<UStaticMesh>(nullptr, FloatMeshPath, nullptr, LOAD_NoWarn | LOAD_Quiet))
    {
        FishingFloat->SetStaticMesh(Painted);
        FishingFloat->EmptyOverrideMaterials();
        bPaintedFishingFloat = true;
    }
    else UE_LOG(LogHomesteadFishingTackle, Warning, TEXT("Fishing float mesh is missing; showing a plain float."));
    FishingCatch = MakeLoose(TEXT("FishingCatch"), Sphere, FLinearColor::White, nullptr);
    FishingCatch->SetCastShadow(true);
    FishingSwimmer = MakeLoose(TEXT("FishingSwimmer"), Sphere, FLinearColor::White, nullptr);
    CaughtFishMeshes.Reset();
    for (const TCHAR* Name : FishMeshes)
    {
        UStaticMesh* Fish = LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/SurvivalGame/Environment/Props/CaughtFish/%s.%s"), Name, Name), nullptr, LOAD_NoWarn | LOAD_Quiet);
        if (!Fish) UE_LOG(LogHomesteadFishingTackle, Warning, TEXT("Caught fish mesh %s is missing."), Name);
        CaughtFishMeshes.Add(Fish);
    }
    SetFishingCatch(Homestead::Item::RiverTrout);
}

void AHomesteadCharacter::SetFishingCatch(Homestead::Item Fish)
{
    const int32 First = static_cast<int32>(Homestead::Item::RiverTrout);
    int32 Index = static_cast<int32>(Fish) - First;
    if (Index < 0 || Index >= CaughtFishMeshes.Num()) Index = 0;
    FishingCatchItem = static_cast<Homestead::Item>(First + Index);
    if (FishingCatch && CaughtFishMeshes.IsValidIndex(Index) && CaughtFishMeshes[Index])
        FishingCatch->SetStaticMesh(CaughtFishMeshes[Index]);
    if (FishingSwimmer && CaughtFishMeshes.IsValidIndex(Index) && CaughtFishMeshes[Index])
        FishingSwimmer->SetStaticMesh(CaughtFishMeshes[Index]);
    bFishingSwimPlanned = false;
}

void AHomesteadCharacter::UpdateFishingTackle(UHomesteadAnimInstance& Animation, UStaticMeshComponent* Pole, float DeltaSeconds)
{
    using namespace HomesteadFishingTackle;
    namespace T = HomesteadFishingTiming;
    if (!FishingFloat || FishingLine.IsEmpty()) return;
    const float Clip = Animation.FishingClipTime();
    const EHomesteadFishingPose Pose = Animation.FishingPose();
    const bool bShown = Pole && Pole->IsVisible() && Clip >= 0 && Animation.FishingWeight() > 0.01f;
    const auto HideAll = [this]
    {
        for (UStaticMeshComponent* Segment : FishingLine) if (Segment) Segment->SetVisibility(false);
        FishingFloat->SetVisibility(false);
        if (FishingCatch) FishingCatch->SetVisibility(false);
        if (FishingSwimmer) FishingSwimmer->SetVisibility(false);
    };
    if (!bShown)
    {
        HideAll();
        bFishingFloatOut = false;
        bFishingBobOnWater = false;
        FishingSubmerge = 0.0f;
        FishingSubmergeSpeed = 0.0f;
        bFishingSwimPlanned = false;
        UpdateFishingCamera(false, DeltaSeconds);
        return;
    }
    FishingBobTime += DeltaSeconds;
    const FVector Tip = Pole->GetComponentTransform().TransformPosition(PoleTip);
    const FVector Dangle = Tip - FVector(0, 0, DangleCm);

    // The landing spot follows her until the float leaves the tip, then stays put.
    const bool bBeforeRelease = Pose == EHomesteadFishingPose::Cast && Clip < T::CastReleaseSeconds;
    if (bBeforeRelease || !bFishingFloatOut)
    {
        const FVector Feet = GetActorLocation() - FVector(0, 0, GetSimpleCollisionHalfHeight());
        const FVector Ahead = GetActorForwardVector().GetSafeNormal2D().RotateAngleAxis(FishingCastYaw, FVector::UpVector);
        FVector Landing = Feet + Ahead * CastReachCm;
        float WaterZ = Feet.Z - WaterBelowFeetCm;
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(FishingFloat), false, this);
        if (FishingCastWaterZ.IsSet())
            WaterZ = FishingCastWaterZ.GetValue();
        else if (GetWorld()->LineTraceSingleByChannel(Hit, Landing + FVector(0, 0, WaterAboveFeetCm),
                Landing - FVector(0, 0, WaterBelowFeetCm), ECC_Visibility, Query))
            WaterZ = Hit.ImpactPoint.Z;
        Landing.Z = WaterZ;
        FishingFloatAt = Landing;
        FishingFloatFrom = Dangle;
        bFishingFloatOut = !bBeforeRelease;
    }

    FVector End = FishingFloatAt;
    float Sag = SlackSag;
    bool bFloat = true, bFish = false;
    // The float's tilt: its top leans toward her on a taut line, and rocks on a nibble.
    const FVector Toward = (Tip - FishingFloatAt).GetSafeNormal2D();
    const FVector Across = FVector::CrossProduct(FVector::UpVector, Toward).GetSafeNormal();
    float TiltDegrees = 0.0f;
    const float Shiver = FishingCue.Nibble * FMath::Sin(UE_TWO_PI * NibbleShakeHz * FishingBobTime);
    const bool bResting = Pose == EHomesteadFishingPose::Wait || Pose == EHomesteadFishingPose::Bite
        || Pose == EHomesteadFishingPose::Fight;
    // 0 riding the water, 1 fully under: a spring toward the cue, so the float never pops in or out.
    const bool bPulledUnder = bResting && FishingCue.Under();
    {
        const float Omega = UE_TWO_PI / ((bPulledUnder ? SinkSeconds : RiseSeconds) * 2.0f);
        const float Damping = bPulledUnder ? 1.0f : RiseDamping;
        const float Target = bPulledUnder ? 1.0f : 0.0f;
        constexpr float MaxStep = 1.0f / 60.0f;
        for (float Left = DeltaSeconds; Left > 0.0f; Left -= MaxStep)
        {
            const float Dt = FMath::Min(MaxStep, Left);
            FishingSubmergeSpeed += (Omega * Omega * (Target - FishingSubmerge) - 2.0f * Damping * Omega * FishingSubmergeSpeed) * Dt;
            FishingSubmerge += FishingSubmergeSpeed * Dt;
        }
    }
    if (Pose == EHomesteadFishingPose::Cast) { FishingSubmerge = 0.0f; FishingSubmergeSpeed = 0.0f; }
    switch (Pose)
    {
    case EHomesteadFishingPose::Cast:
        if (Clip < T::CastReleaseSeconds) { End = Dangle; Sag = 0; }
        else if (Clip < T::CastSplashSeconds)
        {
            const float A = (Clip - T::CastReleaseSeconds) / (T::CastSplashSeconds - T::CastReleaseSeconds);
            End = FMath::Lerp(FishingFloatFrom, FishingFloatAt, A) + FVector(0, 0, 4.0f * A * (1.0f - A) * FlightArcCm);
            Sag = 0;
        }
        break;
    case EHomesteadFishingPose::Wait:
    case EHomesteadFishingPose::Bite:
        End.Z += WaitBobCm * FMath::Sin(UE_TWO_PI * WaitBobHz * FishingBobTime);
        break;
    case EHomesteadFishingPose::Fight:
        // Between runs the fish rests: the float rides back up, dragged across and trembling.
        End += Across * FightSwayCm * FMath::Sin(UE_TWO_PI * FightSwayHz * FishingBobTime);
        End.Z += FightTrembleCm * FMath::Sin(UE_TWO_PI * FightTrembleHz * FishingBobTime);
        TiltDegrees = FightTiltDegrees;
        Sag = TautSag;
        break;
    case EHomesteadFishingPose::Catch:
    {
        const float Lift = Clip - (T::CatchStart + T::CatchLiftSeconds);
        if (Lift >= 0)
        {
            // The fish swings up out of the water into her left hand.
            const FVector Hand = GetMesh()->GetSocketLocation(TEXT("hand_l"));
            const float A = FMath::SmoothStep(0.0f, 1.0f, Lift / SwingInSeconds);
            End = FMath::Lerp(FishingFloatAt, Hand, A) + FVector(0, 0, 60.0f * FMath::Sin(PI * A));
            bFish = true;
            bFloat = false;
        }
        Sag = TautSag;
        break;
    }
    case EHomesteadFishingPose::Miss:
    {
        const float A = FMath::Clamp((Clip - T::MissStart) / (T::MissEnd - T::MissStart), 0.0f, 1.0f);
        End = FMath::Lerp(FishingFloatAt, Dangle, FMath::SmoothStep(0.0f, 1.0f, A)) + FVector(0, 0, 50.0f * FMath::Sin(PI * A));
        Sag = 0;
        break;
    }
    default:
        break;
    }
    if (bResting)
    {
        End += Across * NibbleShakeCm * Shiver - FVector(0, 0, NibbleDipCm * FishingCue.Nibble);
        TiltDegrees += NibbleTiltDegrees * Shiver;
        if (FishingCue.Under()) Sag = TautSag;
    }
    // Pulled under, or coming back up (a negative depth is the bob just past the surface).
    End.Z -= FishingSubmerge * (FloatTopCm - FloatWaterlineCm + SunkDepthCm) * FloatDisplayScale;
    if (Animation.IsFishingStrike())
    {
        // The strike snaps the line taut and drags the float toward her.
        End += (Tip - End).GetSafeNormal2D() * 20.0f;
        Sag = 0;
    }
    bFishingBobOnWater = bFishingFloatOut && (bResting || (Pose == EHomesteadFishingPose::Cast && Clip >= T::CastSplashSeconds)
        || (Pose == EHomesteadFishingPose::Miss && Clip < T::MissStart + 0.1f));
    FishingBobWater = FVector(End.X, End.Y, FishingFloatAt.Z);

    // The float: SM_FishingFloat's pivot is the quill's foot, so it stands with its paint seam on End.
    const FQuat Tilt(Across, FMath::DegreesToRadians(-TiltDegrees));
    const bool bPainted = bPaintedFishingFloat;
    const float FloatScale = FMath::Lerp(1.0f, FloatDisplayScale,
        FMath::Clamp(static_cast<float>(FVector::Dist2D(End, GetActorLocation())) / FloatGrowCm, 0.0f, 1.0f));
    const float Waterline = (bPainted ? FloatWaterlineCm : 0.0f) * FloatScale;
    FishingFloat->SetWorldLocationAndRotation(End - Tilt.RotateVector(FVector(0, 0, Waterline)), Tilt);
    FishingFloat->SetWorldScale3D(bPainted ? FVector(FloatScale)
        : FVector(0.048f, 0.048f, 0.06f) * FloatScale);
    FishingFloat->SetVisibility(bFloat);
    // The line is tied to the top of the float's quill throughout: dangling, in flight and on the water.
    const FVector LineEnd = bFloat
        ? End + Tilt.RotateVector(FVector(0, 0, (bPainted ? FloatTopCm - FloatWaterlineCm : 3.0f) * FloatScale))
        : End;

    // The line: a shallow parabola from the tip to the float, drawn as straight segments.
    const float Span = FVector::Dist(Tip, LineEnd);
    const auto LinePoint = [&](float S) { return FMath::Lerp(Tip, LineEnd, S) - FVector(0, 0, Sag * Span * 4.0f * S * (1.0f - S)); };
    const float Thickness = LineDiameter / 100.0f;
    for (int32 Index = 0; Index < FishingLine.Num(); ++Index)
    {
        UStaticMeshComponent* Segment = FishingLine[Index];
        if (!Segment) continue;
        const FVector A = LinePoint(static_cast<float>(Index) / FishingLine.Num());
        const FVector B = LinePoint(static_cast<float>(Index + 1) / FishingLine.Num());
        const float Length = FVector::Dist(A, B);
        if (Length < KINDA_SMALL_NUMBER) { Segment->SetVisibility(false); continue; }
        Segment->SetWorldLocationAndRotation((A + B) * 0.5f, FRotationMatrix::MakeFromZ(B - A).ToQuat());
        Segment->SetWorldScale3D(FVector(Thickness, Thickness, Length / 100.0f));
        Segment->SetVisibility(true);
    }
    if (FishingSwimmer) UpdateFishingSwimmer(End, Toward, Pose, DeltaSeconds);
    UpdateFishingCamera(Pose != EHomesteadFishingPose::None && Pose != EHomesteadFishingPose::Miss, DeltaSeconds);
    if (FishingCatch)
    {
        // The fish hangs nose-up from the hook; its length runs along the mesh's X.
        const FBox Box = FishingCatch->GetStaticMesh() ? FishingCatch->GetStaticMesh()->GetBoundingBox() : FBox(FVector::ZeroVector, FVector::ZeroVector);
        const FQuat Hang = FRotationMatrix::MakeFromXY(FVector::UpVector, GetActorForwardVector()).ToQuat();
        FishingCatch->SetWorldScale3D(FVector::OneVector);
        FishingCatch->SetWorldLocationAndRotation(End - Hang.RotateVector(FVector(Box.Max.X, 0, 0)), Hang);
        FishingCatch->SetVisibility(bFish && FishingCatch->GetStaticMesh() != nullptr);
    }
}

void AHomesteadCharacter::UpdateFishingSwimmer(const FVector& Float, const FVector& TowardHer, EHomesteadFishingPose Pose, float DeltaSeconds)
{
    using namespace HomesteadFishingTackle;
    const bool bWaiting = FishingCue.WaitSeconds >= 0.0f;
    const bool bSwim = bFishingFloatOut && FishingSwimmer->GetStaticMesh()
        && (bWaiting || FishingCue.bHooked) && Pose != EHomesteadFishingPose::Catch && Pose != EHomesteadFishingPose::Miss;
    if (!bSwim)
    {
        FishingSwimmer->SetVisibility(false);
        if (!FishingCue.bHooked) bFishingSwimPlanned = false;
        return;
    }
    const float WaterZ = FishingFloatAt.Z;
    const FVector Centre(Float.X, Float.Y, WaterZ - SwimDepthCm);
    const FVector Away = -TowardHer;
    bool bSnap = false;
    if (!bFishingSwimPlanned && bWaiting)
    {
        // Plan the approach: from out in the water, away from her, arriving partway through the wait.
        bFishingSwimPlanned = true;
        bSnap = true;
        FishingSwimStart = FishingCue.WaitSeconds;
        const float Arrive = FishingCue.BiteSeconds * FMath::FRandRange(SwimArriveMinFraction, SwimArriveMaxFraction);
        FishingSwimArrive = FMath::Min(FMath::Max(Arrive, FishingSwimStart + SwimMinApproachSeconds),
            FishingCue.BiteSeconds - 0.3f);
        const FVector From = Away.RotateAngleAxis(FMath::FRandRange(-100.0f, 100.0f), FVector::UpVector);
        FishingSwimAngle = FMath::Atan2(From.Y, From.X);
        FishingSwimTurn = FMath::RandBool() ? 1.0f : -1.0f;
        FishingSwimFrom = FVector(Float.X, Float.Y, WaterZ - SwimStartDepthCm) + From * SwimStartCm;
        FishingSwimAt = FishingSwimFrom;
        FishingSwimHeading = -From;
    }
    FVector Target;
    FVector Facing = FishingSwimHeading;
    float Wiggle = SwimWiggleDegrees, WiggleHz = SwimWiggleHz;
    const FVector Ring(FMath::Cos(FishingSwimAngle), FMath::Sin(FishingSwimAngle), 0.0f);
    if (FishingCue.bHooked)
    {
        // On the hook: under the float, pulling away from her and thrashing.
        Target = Centre + Away * NoseRadiusCm;
        Facing = Away;
        Wiggle = ThrashDegrees;
        WiggleHz = ThrashHz;
    }
    else if (FishingCue.WaitSeconds < FishingSwimArrive)
    {
        const float A = FMath::Clamp((FishingCue.WaitSeconds - FishingSwimStart)
            / FMath::Max(0.01f, FishingSwimArrive - FishingSwimStart), 0.0f, 1.0f);
        Target = FMath::Lerp(FishingSwimFrom, Centre + Ring * LoiterRadiusCm, FMath::InterpEaseOut(0.0f, 1.0f, A, 2.0f));
    }
    else
    {
        // Circling the float; a false nibble darts it in to nose the float.
        FishingSwimAngle += FishingSwimTurn * LoiterSpeed * DeltaSeconds * (1.0f - FishingCue.Nibble);
        const FVector Around(FMath::Cos(FishingSwimAngle), FMath::Sin(FishingSwimAngle), 0.0f);
        Target = Centre + Around * FMath::Lerp(LoiterRadiusCm, NoseRadiusCm, FishingCue.Nibble);
        const FVector Tangent = FVector::CrossProduct(FVector::UpVector, Around) * FishingSwimTurn;
        Facing = FMath::Lerp(Tangent, -Around, FishingCue.Nibble).GetSafeNormal();
    }
    const FVector Before = FishingSwimAt;
    FishingSwimAt = bSnap ? Target : FMath::VInterpTo(FishingSwimAt, Target, DeltaSeconds, SwimFollow);
    const FVector Moved = (FishingSwimAt - Before).GetSafeNormal2D();
    if (!FishingCue.bHooked && !Moved.IsNearlyZero() && FishingCue.WaitSeconds < FishingSwimArrive) Facing = Moved;
    FishingSwimHeading = FMath::Lerp(FishingSwimHeading, Facing.GetSafeNormal2D(),
        FMath::Clamp(DeltaSeconds * 5.0f, 0.0f, 1.0f)).GetSafeNormal2D();
    // caught_fish.py authors the fish nose-down -Y in Blender, which the export mirrors to +Y here.
    const FQuat Heading = FRotationMatrix::MakeFromYZ(FishingSwimHeading, FVector::UpVector).ToQuat()
        * FQuat(FVector::UpVector, FMath::DegreesToRadians(Wiggle * FMath::Sin(UE_TWO_PI * WiggleHz * FishingBobTime)));
    const FBox Box = FishingSwimmer->GetStaticMesh()->GetBoundingBox();
    FishingSwimmer->SetWorldScale3D(FVector(SwimScale));
    FishingSwimmer->SetWorldLocationAndRotation(FishingSwimAt - Heading.RotateVector(Box.GetCenter() * SwimScale), Heading);
    FishingSwimmer->SetVisibility(true);
}
void AHomesteadCharacter::UpdateFishingCamera(bool bFishing, float DeltaSeconds)
{
    using namespace HomesteadFishingTackle;
    AController* Viewer = GetController();
    if (!Viewer || !CameraArm) return;
    if (bFishing && !bFishingCamSaved)
    {
        // Remember the view she had, to ease back to it when she's done.
        bFishingCamSaved = true;
        FishingCamSavedView = Viewer->GetControlRotation();
        FishingCamSavedOffset = CameraArm->TargetOffset;
        FishingCamSavedSocket = CameraArm->SocketOffset;
        FishingCamSavedArm = CameraArm->TargetArmLength;
        bFishingCamSavedCollision = CameraArm->bDoCollisionTest;
        // The high view swings the arm over the bank behind her; its collision probe catching the
        // ground there pushed the camera in and out every frame (the overhead jitter).
        CameraArm->bDoCollisionTest = false;
    }
    if (!bFishingCamSaved) return;
    FishingCamBlend = FMath::Clamp(FishingCamBlend + (bFishing ? 1.0f : -1.0f) * DeltaSeconds / FishCamBlendSeconds, 0.0f, 1.0f);
    const float A = FMath::SmoothStep(0.0f, 1.0f, FishingCamBlend);
    const FVector Feet = GetActorLocation() - FVector(0, 0, GetSimpleCollisionHalfHeight());
    const FVector Water = bFishingFloatOut || bFishing ? FishingFloatAt : Feet + GetActorForwardVector() * FishingCastReachCm;
    // Pivot a little above the water, not on it, so nothing about the arm starts inside the ground.
    const FVector Mid = FMath::Lerp(Feet, FVector(Water.X, Water.Y, FMath::Min(Water.Z, Feet.Z)), FishCamMidFraction)
        + FVector(0, 0, FishCamPivotLiftCm);
    const FVector Out = (Water - Feet).GetSafeNormal2D();
    const FRotator Fishing(FishCamPitch, Out.IsNearlyZero() ? GetActorRotation().Yaw : Out.Rotation().Yaw, 0.0f);
    FRotator View = FishingCamSavedView;
    View.Yaw = FishingCamSavedView.Yaw + FMath::FindDeltaAngleDegrees(FishingCamSavedView.Yaw, Fishing.Yaw) * A;
    View.Pitch = FMath::Lerp(FRotator::NormalizeAxis(FishingCamSavedView.Pitch), Fishing.Pitch, A);
    Viewer->SetControlRotation(View);
    CameraArm->TargetOffset = FMath::Lerp(FishingCamSavedOffset, Mid - GetActorLocation(), A);
    CameraArm->SocketOffset = FMath::Lerp(FishingCamSavedSocket, FVector::ZeroVector, A);
    CameraArm->TargetArmLength = FMath::Lerp(FishingCamSavedArm, FishCamArmCm, A);
    if (!bFishing && FishingCamBlend <= 0.0f)
    {
        CameraArm->TargetOffset = FishingCamSavedOffset;
        CameraArm->SocketOffset = FishingCamSavedSocket;
        CameraArm->TargetArmLength = FishingCamSavedArm;
        CameraArm->bDoCollisionTest = bFishingCamSavedCollision;
        bFishingCamSaved = false;
    }
}