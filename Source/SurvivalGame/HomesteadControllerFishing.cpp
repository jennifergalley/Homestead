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

bool AHomesteadController::UpdateFishingFocus(Homestead::Point Position)
{
    FocusedFishingWater = Homestead::FishingWater::None;
    FishingFocusText.Reset();
    if (SelectedCarriedTool() != Homestead::Item::FishingPole) return false;
    FocusedFishingWater = Sim.FishingWaterAt(Position);
    if (FocusedFishingWater == Homestead::FishingWater::None) return false;
    if (!IsFishing())
    {
        const auto Ready = Sim.CheckFishing(Position);
        FishingFocusText = Ready.ok ? bGamepad ? TEXT("[RT] Cast line") : TEXT("[LMB] Cast line")
            : UTF8_TO_TCHAR(Ready.message.c_str());
    }
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
    const auto Result = Sim.AdvanceFishing(DeltaSeconds, PlayerPoint());
    if (!Result.ok)
    {
        UE_LOG(LogHomesteadFishingController, Error, TEXT("Fishing update refused: %s"), UTF8_TO_TCHAR(Result.message.c_str()));
        Sim.CancelFishing();
    }
    if (!Result.message.empty()) Notify(Result);
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
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
        bFishingLiftSucceeded = false;
        ObservedFishSplashes = Animation->FishCastSplashes();
        ObservedFishLifts = Animation->FishCatchLifts();
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

FString AHomesteadController::FishingPrompt() const
{
    const auto& Cast = Sim.FishingCast();
    const TCHAR* Press = UsesGamepad() ? TEXT("[RT]") : TEXT("[LMB]");
    switch (Cast.phase)
    {
    case Homestead::FishingPhase::Casting: return TEXT("Casting...");
    case Homestead::FishingPhase::Waiting: return TEXT("Watch the float...");
    case Homestead::FishingPhase::Bite: return FString::Printf(TEXT("Bite! %s"), Press);
    case Homestead::FishingPhase::Landing:
        return Homestead::Fishing::StrikeReady(Cast) ? FString::Printf(TEXT("Strike! %s"), Press) : TEXT("Hold steady...");
    case Homestead::FishingPhase::Catching: return TEXT("Lifting the catch...");
    default: return FString();
    }
}
