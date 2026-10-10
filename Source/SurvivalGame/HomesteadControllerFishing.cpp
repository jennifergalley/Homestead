#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadFishingPresentation.h"
#include "HomesteadFishingPresentationRules.h"
#include "HomesteadEstateTerrain.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Simulation/HomesteadFishing.h"

namespace HomesteadWaterProbe { bool ShoreContains(const USplineComponent& Spline, const FVector2D& Point); }
DEFINE_LOG_CATEGORY_STATIC(LogHomesteadFishingController, Log, All);

static_assert(HomesteadFishingTiming::CastSplashSeconds - Homestead::Fishing::CastSplashSeconds < 0.0001
    && HomesteadFishingTiming::CastSplashSeconds - Homestead::Fishing::CastSplashSeconds > -0.0001,
    "The simulation splash gate must match the authored cast.");
static_assert(HomesteadFishingTiming::CatchLiftSeconds - Homestead::Fishing::CatchLiftSeconds < 0.0001
    && HomesteadFishingTiming::CatchLiftSeconds - Homestead::Fishing::CatchLiftSeconds > -0.0001,
    "The simulation reward gate must match the authored lift.");

Homestead::FishingWater AHomesteadController::ProbeFishingWater(Homestead::Point Position) const
{
    if (!bEstateMap)
        return Sim.NearWater(Position) ? Homestead::FishingWater::River : Homestead::FishingWater::None;
    WaterEdgeDistance(Position, false);
    const FVector Here(Position.x, Position.y, GroundHeight(Position.x, Position.y));
    double Best = Homestead::Fishing::BankReachCm;
    auto Water = Homestead::FishingWater::None;
    for (const auto& Weak : EstateWaterSplines)
        if (const USplineComponent* Spline = Weak.Get())
        {
            const FVector Closest = Spline->FindLocationClosestToWorldLocation(Here, ESplineCoordinateSpace::World);
            const float Key = Spline->FindInputKeyClosestToWorldLocation(Here);
            double Distance = FVector::Dist2D(Closest, Here)
                - (Spline->IsClosedLoop() ? 0.0 : 100.0 * Spline->GetScaleAtSplineInputKey(Key).Y);
            if (Spline->IsClosedLoop() && HomesteadWaterProbe::ShoreContains(*Spline, FVector2D(Position.x, Position.y)))
                Distance = -Distance;
            if (Distance <= Best)
            {
                Best = Distance;
                Water = Spline->IsClosedLoop() ? Homestead::FishingWater::Lake : Homestead::FishingWater::River;
            }
        }
    if (Water != Homestead::FishingWater::None) return Water;
    return WaterEdgeDistance(Position, true) <= Homestead::Fishing::BankReachCm
        ? Homestead::FishingWater::Ocean : Homestead::FishingWater::None;
}

bool AHomesteadController::IsFishing() const
{
    return Sim.FishingCast().phase != Homestead::FishingPhase::Idle;
}

float AHomesteadController::ChooseFishingCastYaw(const AHomesteadCharacter& Avatar) const
{
    // Straight out in front of her (Jenny, 2026-10-09: the high fishing camera sees the float past her),
    // angling off only where straight ahead isn't open water (shallows, a beach).
    constexpr float Sides[] = {0.0f, 14.0f, -14.0f, 25.0f, -25.0f};
    constexpr double OpenWaterCm = -120.0;
    const FVector From = Avatar.GetActorLocation();
    const FVector Facing = Avatar.GetActorForwardVector().GetSafeNormal2D();
    float BestYaw = 0.0f;
    double Best = TNumericLimits<double>::Max();
    for (const float Yaw : Sides)
    {
        const FVector At = From + Facing.RotateAngleAxis(Yaw, FVector::UpVector) * AHomesteadCharacter::FishingCastReachCm;
        const double Edge = WaterEdgeDistance({At.X, At.Y}, true);
        if (Edge <= OpenWaterCm) return Yaw;
        if (Edge < Best) { Best = Edge; BestYaw = Yaw; }
    }
    return BestYaw;
}

TOptional<float> AHomesteadController::FishingWaterSurface(const FVector2D& At) const
{
    // River and pond splines run along the water surface; the sea and anything else is left to the
    // float's own trace. (The water meshes don't block visibility traces, so a trace finds the bed.)
    TOptional<float> Surface;
    double Best = TNumericLimits<double>::Max();
    const FVector Here(At.X, At.Y, GroundHeight(At.X, At.Y));
    for (const auto& Weak : EstateWaterSplines)
        if (const USplineComponent* Spline = Weak.Get())
        {
            const FVector Closest = Spline->FindLocationClosestToWorldLocation(Here, ESplineCoordinateSpace::World);
            const double Distance = FVector::Dist2D(Closest, Here);
            const bool bInside = Spline->IsClosedLoop()
                ? HomesteadWaterProbe::ShoreContains(*Spline, At)
                : Distance <= 100.0 * Spline->GetScaleAtSplineInputKey(Spline->FindInputKeyClosestToWorldLocation(Here)).Y;
            if (bInside && Distance < Best)
            {
                Best = Distance;
                Surface = static_cast<float>(Closest.Z);
            }
        }
    return Surface;
}

bool AHomesteadController::UpdateFishingFocus(Homestead::Point Position)
{
    FocusedFishingWater = Homestead::FishingWater::None;
    FishingFocusText.Reset();
    if (SelectedCarriedTool() != Homestead::Item::FishingPole) return false;
    FocusedFishingWater = Sim.FishingWaterAt(Position);
    if (FocusedFishingWater == Homestead::FishingWater::None) return false;
    // The focus card names the verb only; a refusal (too tired, pack full) is said once, by the click's
    // notice (Jenny, 2026-10-09: it was shown twice, in the card and the notice).
    if (!IsFishing())
        FishingFocusText = bGamepad ? TEXT("[RT] Cast line") : TEXT("[LMB] Cast line");
    return true;
}

void AHomesteadController::FishingInput()
{
    if (!ShouldShowHotbar() || HasNativeMenu() || IsNewGameSetup() || IsNamingSetup()) return;
    const auto Before = Sim.FishingCast().phase;
    if (Before == Homestead::FishingPhase::Bite || Before == Homestead::FishingPhase::Landing)
        if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
            if (auto* Animation = Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()))
                Animation->PlayFishingStrike();
    const auto Result = Sim.FishingPress(PlayerPoint());
    PresentFishing();
    if (!Result.message.empty())
    {
        if (Result.ok && IsFishing()) NotifyResourceAction(Result, nullptr);
        else Notify(Result);
    }
}

void AHomesteadController::TickFishing(float DeltaSeconds)
{
    if (!IsFishing()) { PresentFishing(); return; }
    if (!ShouldShowHotbar() || HasNativeMenu() || IsNewGameSetup() || IsNamingSetup()
        || HotbarItem(SelectedHotbarSlot) != Homestead::Item::FishingPole)
    {
        Notify(Sim.CancelFishing());
        PresentFishing();
        return;
    }
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (Animation && Sim.FishingCast().token == FishingPresentedToken
        && Animation->FishInterruptions() != ObservedFishInterruptions)
    {
        ObservedFishInterruptions = Animation->FishInterruptions();
        const auto Interrupted = Sim.FishingAnimationInterrupted(FishingPresentedToken);
        if (!Interrupted.ok)
            UE_LOG(LogHomesteadFishingController, Warning, TEXT("Fishing interruption refused: %s"),
                UTF8_TO_TCHAR(Interrupted.message.c_str()));
        if (!Interrupted.message.empty()) Notify(Interrupted);
        PresentFishing();
        return;
    }
    const auto Result = Sim.AdvanceFishing(DeltaSeconds, PlayerPoint());
    if (!Result.ok)
    {
        UE_LOG(LogHomesteadFishingController, Error, TEXT("Fishing update refused: %s"), UTF8_TO_TCHAR(Result.message.c_str()));
        Sim.CancelFishing();
    }
    if (!Result.message.empty()) Notify(Result);
    if (Animation && IsFishing())
    {
        const uint32 Splashes = Animation->FishCastSplashes();
        const uint32 Lifts = Animation->FishCatchLifts();
        const auto Phase = Sim.FishingCast().phase;
        const bool Splash = Phase == Homestead::FishingPhase::Casting && Splashes != ObservedFishSplashes;
        const bool Lift = Phase == Homestead::FishingPhase::Catching && Lifts != ObservedFishLifts;
        ObservedFishSplashes = Splashes;
        ObservedFishLifts = Lifts;
        if (Splash || Lift)
        {
            const auto Contact = Sim.FishingAnimationContact(Splash ? Homestead::FishingContact::CastSplash
                : Homestead::FishingContact::CatchLift, FishingPresentedToken, PlayerPoint());
            bFishingLiftSucceeded = Lift && Contact.ok && !IsFishing();
            if (!Contact.ok)
            {
                UE_LOG(LogHomesteadFishingController, Warning, TEXT("Fishing contact refused: %s"), UTF8_TO_TCHAR(Contact.message.c_str()));
                if (IsFishing()) Sim.CancelFishing();
            }
            if (!Contact.message.empty()) Notify(Contact);
        }
    }
    PresentFishing();
}

void AHomesteadController::PresentFishing()
{
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
    if (!Animation) return;
    const auto& Session = Sim.FishingCast();
    FHomesteadFishingCue Cue;
    Cue.Nibble = static_cast<float>(Homestead::Fishing::Nibble(Session));
    if (Homestead::Fishing::FloatUnder(Session)) Cue.Window = static_cast<float>(Homestead::Fishing::Marker(Session));
    if (Session.phase == Homestead::FishingPhase::Waiting)
    {
        Cue.WaitSeconds = static_cast<float>(Session.elapsed);
        Cue.BiteSeconds = static_cast<float>(Session.biteAfter);
    }
    Cue.bHooked = Session.phase == Homestead::FishingPhase::Bite || Session.phase == Homestead::FishingPhase::Landing;
    Avatar->SetFishingCue(Cue);
    if (Session.phase == Homestead::FishingPhase::Idle
        && HomesteadFishingPresentationRules::FinishedMiss(
            Animation->FishingPose() == EHomesteadFishingPose::Miss, Animation->HasFishingClip(),
            Animation->FishingClipTime(), HomesteadFishingTiming::MissEnd,
            HomesteadFishingTiming::ClipFrameSeconds + KINDA_SMALL_NUMBER))
        Animation->SetFishingPose(EHomesteadFishingPose::None);
    const bool bNewCast = HomesteadFishingPresentationRules::NewCast(Session, FishingPresentedToken);
    if (Session.token != FishingPresentedToken && Session.phase != Homestead::FishingPhase::Idle)
    {
        FishingPresentedToken = Session.token;
        Avatar->SetFishingCatch(Session.catchItem);
        const float CastYaw = ChooseFishingCastYaw(*Avatar);
        const FVector Landing = Avatar->GetActorLocation() + Avatar->GetActorForwardVector().GetSafeNormal2D()
            .RotateAngleAxis(CastYaw, FVector::UpVector) * AHomesteadCharacter::FishingCastReachCm;
        Avatar->SetFishingCast(CastYaw, FishingWaterSurface(FVector2D(Landing.X, Landing.Y)));
        bFishingLiftSucceeded = false;
        ObservedFishSplashes = Animation->FishCastSplashes();
        ObservedFishLifts = Animation->FishCatchLifts();
        ObservedFishInterruptions = Animation->FishInterruptions();
    }
    if (Session.phase == FishingPresentedPhase && !bNewCast) return;
    EHomesteadFishingPose Pose = EHomesteadFishingPose::None;
    switch (Session.phase)
    {
    case Homestead::FishingPhase::Casting: Pose = EHomesteadFishingPose::Cast; break;
    case Homestead::FishingPhase::Waiting: Pose = EHomesteadFishingPose::Wait; break;
    case Homestead::FishingPhase::Bite: Pose = EHomesteadFishingPose::Bite; break;
    case Homestead::FishingPhase::Landing: Pose = EHomesteadFishingPose::Fight; break;
    case Homestead::FishingPhase::Catching:
        Avatar->SetFishingCatch(Session.catchItem);
        Pose = EHomesteadFishingPose::Catch;
        break;
    case Homestead::FishingPhase::Idle:
        Pose = bFishingLiftSucceeded ? EHomesteadFishingPose::None : EHomesteadFishingPose::Miss;
        break;
    }
    FishingPresentedPhase = Session.phase;
    Animation->SetFishingPose(Pose);
}
