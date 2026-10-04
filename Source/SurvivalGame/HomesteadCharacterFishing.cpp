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
// Where the float lands: straight ahead of her, beyond the bank (cm).
constexpr float CastReachCm = 430.0f;
// Peak of the float's flight above the straight line from release to landing (cm).
constexpr float FlightArcCm = 140.0f;
// The water is looked for this far above and below her feet (cm); a bank higher than that leaves
// the float at the lower bound.
constexpr float WaterAboveFeetCm = 20.0f;
constexpr float WaterBelowFeetCm = 90.0f;
// Line sag as a fraction of its span: slack while waiting, nearly taut with a fish on.
constexpr float SlackSag = 0.07f;
constexpr float TautSag = 0.008f;
// Float motion (cm, Hz): a lazy bob, the bite's sharp dips, the fight's drag under and across.
constexpr float WaitBobCm = 0.6f, WaitBobHz = 0.5f;
constexpr float BiteDipCm = 3.5f, BiteDipHz = 2.8f;
constexpr float FightDownCm = 6.0f, FightSwayCm = 18.0f, FightSwayHz = 1.3f;
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
    FishingCatch = MakeLoose(TEXT("FishingCatch"), Sphere, FLinearColor::White, nullptr);
    FishingCatch->SetCastShadow(true);
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
    };
    if (!bShown)
    {
        HideAll();
        bFishingFloatOut = false;
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
        const FVector Ahead = GetActorForwardVector().GetSafeNormal2D();
        FVector Landing = Feet + Ahead * CastReachCm;
        float WaterZ = Feet.Z - WaterBelowFeetCm;
        FHitResult Hit;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(FishingFloat), false, this);
        if (GetWorld()->LineTraceSingleByChannel(Hit, Landing + FVector(0, 0, WaterAboveFeetCm),
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
        End.Z += WaitBobCm * FMath::Sin(UE_TWO_PI * WaitBobHz * FishingBobTime);
        break;
    case EHomesteadFishingPose::Bite:
    {
        // Sharp nibbles: the float ducks quickly and pops back slowly.
        const float Phase = FMath::Frac(BiteDipHz * FishingBobTime);
        End.Z -= BiteDipCm * (Phase < 0.25f ? Phase / 0.25f : 1.0f - (Phase - 0.25f) / 0.75f);
        Sag = SlackSag * 0.5f;
        break;
    }
    case EHomesteadFishingPose::Fight:
    {
        const FVector Across = FVector::CrossProduct(FVector::UpVector, GetActorForwardVector()).GetSafeNormal2D();
        End += Across * FightSwayCm * FMath::Sin(UE_TWO_PI * FightSwayHz * FishingBobTime) - FVector(0, 0, FightDownCm);
        Sag = TautSag;
        bFloat = false;
        break;
    }
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
    if (Animation.IsFishingStrike())
    {
        // The strike snaps the line taut and drags the float toward her.
        End += (Tip - End).GetSafeNormal2D() * 20.0f;
        Sag = 0;
    }

    // The line: a shallow parabola from the tip to the float, drawn as straight segments.
    const float Span = FVector::Dist(Tip, End);
    const auto LinePoint = [&](float S) { return FMath::Lerp(Tip, End, S) - FVector(0, 0, Sag * Span * 4.0f * S * (1.0f - S)); };
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
    // The carved float: 7.5 x 1.6 cm (fishing_pole.py), standing upright on the water.
    FishingFloat->SetWorldLocationAndRotation(End, FQuat::Identity);
    FishingFloat->SetWorldScale3D(FVector(0.016f, 0.016f, 0.075f));
    FishingFloat->SetVisibility(bFloat);
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
