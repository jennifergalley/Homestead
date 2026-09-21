#include "HomesteadVisualPlaytest.h"
#include "HomesteadController.h"
#include "HomesteadCharacter.h"
#include "HomesteadAnimInstance.h"
#include "HomesteadWateringTool.h"
#include "HomesteadHatchet.h"
#include "HomesteadSave.h"
#include "HomesteadWorld.h"
#include "UI/SHomesteadMenu.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/ConfigCacheIni.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UObjectArray.h"
#include "UnrealClient.h"

namespace
{
bool AtomicText(const FString& Path, const FString& Text)
{
    return FFileHelper::SaveStringToFile(Text, *(Path + TEXT(".tmp")))
        && IFileManager::Get().Move(*Path, *(Path + TEXT(".tmp")), true, true, false, true);
}

bool ReadObject(const FString& Path, TSharedPtr<FJsonObject>& Object)
{
    FString Text;
    return FFileHelper::LoadFileToString(Text, *Path)
        && FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object) && Object.IsValid();
}

bool IsNormalLit()
{
    const auto* Viewport = GEngine ? GEngine->GameViewport.Get() : nullptr;
    return Viewport && Viewport->ViewModeIndex == VMI_Lit
        && Viewport->EngineShowFlags.Lighting && !Viewport->EngineShowFlags.ShaderComplexity;
}

TSharedRef<FJsonObject> EndurancePresentation()
{
    auto Object = MakeShared<FJsonObject>();
    const auto* Viewport = GEngine ? GEngine->GameViewport.Get() : nullptr;
    Object->SetBoolField(TEXT("available"), Viewport != nullptr);
    if (Viewport)
    {
        Object->SetNumberField(TEXT("viewMode"), Viewport->ViewModeIndex);
        Object->SetBoolField(TEXT("lighting"), Viewport->EngineShowFlags.Lighting);
        Object->SetBoolField(TEXT("shaderComplexity"), Viewport->EngineShowFlags.ShaderComplexity);
        Object->SetStringField(TEXT("showFlags"), Viewport->EngineShowFlags.ToString());
    }
    return Object;
}
}

void AHomesteadVisualPlaytest::EnduranceEvent(const FString& Message)
{
    const FString Entry = FString::Printf(TEXT("%.3f %s"), FPlatformTime::Seconds() - Endurance.Started, *Message);
    Endurance.Events.Add(Entry);
    UE_LOG(LogTemp, Display, TEXT("ENDURANCE %s"), *Entry);
}

void AHomesteadVisualPlaytest::PrepareEndurance()
{
    auto& E = Endurance;
    E.Started = E.LastTick = E.StepStarted = FPlatformTime::Seconds();
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadEnduranceSeconds="), E.Duration);
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadEnduranceControl="), E.ControlPath);
    E.FreshWorld = FParse::Param(FCommandLine::Get(), TEXT("HomesteadEnduranceFresh"));
    const auto* Config = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    if ((E.Duration != 180 && E.Duration != 2700 && !(E.FreshWorld && E.Duration == 4200)) || E.ControlPath.IsEmpty()
        || !Config || !FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(Config->IniPath),
            FPaths::ConvertRelativePathToFull(FPaths::Combine(OutputDirectory, TEXT("Graphics"), TEXT("GameUserSettings.ini")))))
    { FinishEndurance(TEXT("failed"), TEXT("Invalid duration/control path or non-isolated graphics destination.")); return; }
    for (const TCHAR* Flag : {TEXT("HomesteadSmokeTest"), TEXT("HomesteadPresentationDiagnostics"),
        TEXT("HomesteadWateringPlaytest"), TEXT("HomesteadWeedingPlaytest"), TEXT("HomesteadClearingPlaytest")})
        if (FParse::Param(FCommandLine::Get(), Flag))
        { FinishEndurance(TEXT("failed"), TEXT("Endurance cannot overlap other test modes.")); return; }
    TSharedPtr<FJsonObject> Control;
    FString Deadline, State, Completion;
    if (!ReadObject(E.ControlPath, Control) || !Control->TryGetStringField(TEXT("id"), E.RunId)
        || !Control->TryGetStringField(TEXT("state"), State) || State != TEXT("running"))
    { FinishEndurance(TEXT("failed"), TEXT("Run control does not allow this bounded duration.")); return; }
    Control->TryGetStringField(TEXT("completionPolicy"), Completion);
    E.CompletionDriven = Completion == TEXT("until-complete");
    if (!E.CompletionDriven && (!Control->TryGetStringField(TEXT("deadlineUtc"), Deadline)
        || !FDateTime::ParseIso8601(*Deadline, E.DeadlineUtc)
        || (E.DeadlineUtc - FDateTime::UtcNow()).GetTotalSeconds() < E.Duration + 90))
    { FinishEndurance(TEXT("failed"), TEXT("Run deadline does not allow this exercise.")); return; }
    E.FrameHistogram.Init(0, 10001);
    E.Samples.Add(TEXT("wall_seconds,paused_seconds,unpaused_seconds,engine_unpaused_seconds,game_hour,hunger,energy,warmth,moving_seconds,distance_cm,physical_bytes,virtual_bytes,object_slots_in_use,resources,available_resources,structures,plots,gathers,eats,waypoints,autosave_writes,refreshes"));
    if (E.FreshWorld)
    {
        if (IFileManager::Get().FileExists(*PC->SavePath(TEXT("Homestead_Manual"))) || PC->WorldId.IsEmpty())
        { FinishEndurance(TEXT("failed"), TEXT("Fresh-world endurance requires a new isolated world without a manual fixture.")); return; }
        E.WorldId = PC->WorldId;
        E.Step = FHomesteadEnduranceState::Phase::Settle;
    }
    else
    {
        const auto* Seed = PC->ReadSave(PC->SavePath(TEXT("Homestead_Manual")));
        if (!Seed) { FinishEndurance(TEXT("failed"), TEXT("Disclosed fixture is missing or corrupt.")); return; }
        E.WorldId = Seed->WorldId;
        E.SavedState = Seed->SimulationData;
    }
    if (!GEngine || !GEngine->GameViewport || !PC->PlayerInput
        || !FPaths::IsUnderDirectory(FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir()), OutputDirectory))
    { FinishEndurance(TEXT("failed"), TEXT("Missing viewport/input or non-isolated screenshot directory.")); return; }
    E.StartupViewMode = GEngine->GameViewport->ViewModeIndex;
    E.StartupLighting = GEngine->GameViewport->EngineShowFlags.Lighting;
    E.StartupShaderComplexity = GEngine->GameViewport->EngineShowFlags.ShaderComplexity;
    E.StartupShowFlags = GEngine->GameViewport->EngineShowFlags.ToString();
#if !UE_BUILD_SHIPPING
    E.DebugBindingQueryAvailable = true;
    E.F5Binding = PC->PlayerInput->GetBind(EKeys::F5);
    E.F9Binding = PC->PlayerInput->GetBind(EKeys::F9);
#else
    if (!E.FreshWorld || !FParse::Param(FCommandLine::Get(), TEXT("HomesteadShippingQA")))
    { FinishEndurance(TEXT("failed"), TEXT("Shipping endurance requires the explicit guarded fresh-world route.")); return; }
#endif
    if (!E.F5Binding.IsEmpty() || !E.F9Binding.IsEmpty())
    { FinishEndurance(TEXT("failed"), TEXT("Conflicting effective F5/F9 debug bindings remain.")); return; }
    if (!IsNormalLit())
    { FinishEndurance(TEXT("failed"), TEXT("Endurance requires normal Lit/lighting without ShaderComplexity.")); return; }
    RecordPresentationSettings(TEXT("start"));
    EnduranceEvent(E.FreshWorld
        ? TEXT("Prepared fresh isolated world; mapped travel/actions only; no fixture, retries or direct simulation edits.")
        : TEXT("Prepared existing test-world fixture; mapped travel/actions only; no retries or direct simulation edits."));
}

bool AHomesteadVisualPlaytest::TapEnduranceLoad()
{
    const bool Before = FScreenshotRequest::IsScreenshotRequested();
    Tap(EKeys::F9);
    const bool After = FScreenshotRequest::IsScreenshotRequested();
    EnduranceEvent(FString::Printf(TEXT("F9 screenshot request before=%d after=%d"), Before, After));
    if (Before || After)
    { FinishEndurance(TEXT("failed"), TEXT("Unexpected screenshot request around mapped F9.")); return false; }
    ++Endurance.F9ScreenshotChecks;
    return true;
}

bool AHomesteadVisualPlaytest::InspectEnduranceSaves()
{
    auto& E = Endurance;
    for (const TCHAR* Slot : {TEXT("Homestead_Manual"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"),
        TEXT("Homestead_Auto_2"), TEXT("Homestead_Recovery")})
        for (const FString& Suffix : {FString(), FString(TEXT(".bak"))})
        {
            const FString Path = PC->SavePath(Slot) + Suffix;
            if (!IFileManager::Get().FileExists(*Path)) continue;
            const auto* Save = PC->ReadSave(Path);
            if (!Save || Save->WorldId != E.WorldId) return false;
            if (Suffix.IsEmpty() && FString(Slot).StartsWith(TEXT("Homestead_Auto_")))
            {
                const int64* Previous = E.SaveStamps.Find(Slot);
                if (!Previous || *Previous != Save->SavedAtUtc)
                {
                    E.SaveStamps.Add(Slot, Save->SavedAtUtc);
                    ++E.AutosaveWrites;
                    EnduranceEvent(FString::Printf(TEXT("valid rotating autosave %s at UTC %lld"), Slot, Save->SavedAtUtc));
                }
            }
        }
    return true;
}

bool AHomesteadVisualPlaytest::WriteEnduranceProgress(const FString& Status, const FString& Reason)
{
    const auto& E = Endurance;
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("status"), Status);
    Object->SetStringField(TEXT("reason"), Reason);
    Object->SetBoolField(TEXT("freshWorld"), E.FreshWorld);
    Object->SetBoolField(TEXT("completionDriven"), E.CompletionDriven);
    Object->SetBoolField(TEXT("debugBindingQueryAvailable"), E.DebugBindingQueryAvailable);
    Object->SetNumberField(TEXT("dayMinutes"), PC->State().dayMinutes);
    Object->SetBoolField(TEXT("raining"), PC->Simulation().IsRaining());
    Object->SetBoolField(TEXT("night"), PC->Simulation().IsNight());
    Object->SetNumberField(TEXT("litGuardVersion"), 1);
    Object->SetNumberField(TEXT("litGuardTicks"), E.LitGuardTicks);
    Object->SetNumberField(TEXT("startupViewMode"), E.StartupViewMode);
    Object->SetBoolField(TEXT("startupLighting"), E.StartupLighting);
    Object->SetBoolField(TEXT("startupShaderComplexity"), E.StartupShaderComplexity);
    Object->SetStringField(TEXT("startupShowFlags"), E.StartupShowFlags);
    Object->SetStringField(TEXT("effectiveF5DebugBinding"), E.F5Binding);
    Object->SetStringField(TEXT("effectiveF9DebugBinding"), E.F9Binding);
    Object->SetNumberField(TEXT("f9NoScreenshotChecks"), E.F9ScreenshotChecks);
    Object->SetObjectField(TEXT("presentation"), EndurancePresentation());
    Object->SetNumberField(TEXT("pid"), FPlatformProcess::GetCurrentProcessId());
    Object->SetNumberField(TEXT("wallSeconds"), E.Loaded ? FPlatformTime::Seconds() - E.Started : 0);
    Object->SetNumberField(TEXT("targetSeconds"), E.Duration);
    Object->SetNumberField(TEXT("pausedSeconds"), E.Paused);
    Object->SetNumberField(TEXT("unpausedSeconds"), E.Unpaused);
    Object->SetNumberField(TEXT("engineUnpausedSeconds"), E.EngineUnpaused);
    Object->SetNumberField(TEXT("movingSeconds"), E.Moving);
    Object->SetNumberField(TEXT("distanceCm"), E.Distance);
    Object->SetNumberField(TEXT("initialHour"), E.InitialHour);
    Object->SetNumberField(TEXT("hour"), PC->State().hour);
    Object->SetNumberField(TEXT("actionHours"), E.ActionHours);
    Object->SetNumberField(TEXT("naturalGameHours"), PC->State().hour - E.InitialHour - E.ActionHours);
    Object->SetNumberField(TEXT("gathers"), E.Gathers);
    Object->SetNumberField(TEXT("eats"), E.Eats);
    Object->SetNumberField(TEXT("waypoints"), E.Waypoints);
    Object->SetNumberField(TEXT("manualSaves"), E.ManualSaves);
    Object->SetNumberField(TEXT("loads"), E.Loads);
    Object->SetBoolField(TEXT("exactSameWorldRoundTrip"), E.RoundTrip);
    Object->SetNumberField(TEXT("autosaveSlots"), E.SaveStamps.Num());
    Object->SetNumberField(TEXT("autosaveWrites"), E.AutosaveWrites);
    Object->SetNumberField(TEXT("resourceRefreshes"), E.Refreshes);
    Object->SetNumberField(TEXT("verifiedHarvestedResourceRegrowth"), E.VerifiedRegrowth.Num());
    Object->SetNumberField(TEXT("navigationFailures"), E.NavigationFailures);
    Object->SetNumberField(TEXT("recoveryAttempts"), 0);
    Object->SetNumberField(TEXT("captures"), CaptureIndex);
    Object->SetNumberField(TEXT("timedFrames"), E.FrameCount);
    Object->SetNumberField(TEXT("meanTickMs"), E.FrameCount ? E.FrameSum / E.FrameCount : 0);
    Object->SetNumberField(TEXT("maxTickMs"), E.FrameMaximum);
    for (const double Quantile : {0.95, 0.99})
    {
        uint64 Count = 0;
        for (int32 Bin = 0; Bin < E.FrameHistogram.Num(); ++Bin)
        {
            Count += E.FrameHistogram[Bin];
            if (Count >= E.FrameCount * Quantile)
            { Object->SetNumberField(Quantile == 0.95 ? TEXT("p95TickMs") : TEXT("p99TickMs"), Bin / 10.0); break; }
        }
    }
    Object->SetStringField(TEXT("timingLimits"), TEXT("Instrumented actor wall-tick cadence, not GPU/Present/scanout. First60s and capture+2s excluded;1000ms histogram overflow bin."));
    FString Text;
    FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
    return AtomicText(FPaths::Combine(OutputDirectory, TEXT("progress.json")), Text)
        && AtomicText(FPaths::Combine(OutputDirectory, TEXT("endurance-samples.csv")), FString::Join(E.Samples, TEXT("\n")) + TEXT("\n"))
        && AtomicText(FPaths::Combine(OutputDirectory, TEXT("endurance-events.txt")), FString::Join(E.Events, TEXT("\n")) + TEXT("\n"));
}

void AHomesteadVisualPlaytest::FinishEndurance(const FString& Status, const FString& Reason)
{
    if (bFinished) return;
    bFinished = true;
    ApplyAxes({}, {});
    EnduranceEvent(Status + TEXT(": ") + Reason);
    RecordPresentationSettings(TEXT("end"));
    bool Saved = WriteEnduranceProgress(Status, Reason);
    Saved = AtomicText(FPaths::Combine(OutputDirectory, TEXT("presentation-settings.txt")),
        FString::Join(PresentationSettings, TEXT("\n")) + TEXT("\n")) && Saved;
    if (!Saved) UE_LOG(LogTemp, Error, TEXT("ENDURANCE evidence persistence failed"));
    FPlatformMisc::RequestExitWithStatus(false, Saved && Status != TEXT("failed") ? 0 : 1);
}

void AHomesteadVisualPlaytest::TickEndurance(float EngineDelta)
{
    auto& E = Endurance;
    using Phase = FHomesteadEnduranceState::Phase;
    const double Now = FPlatformTime::Seconds(), Delta = Now - E.LastTick;
    E.LastTick = Now;
    const double Age = Now - E.Started;
    const auto Go = [&E, Now](Phase Next) { E.Step = Next; E.StepStarted = Now; };
    if (!IsNormalLit())
    { FinishEndurance(TEXT("failed"), TEXT("Endurance requires normal Lit/lighting without ShaderComplexity.")); return; }
    ++E.LitGuardTicks;
    if (Now >= E.NextControl)
    {
        E.NextControl = Now + 1;
        TSharedPtr<FJsonObject> Control;
        FString State, Deadline, RunId, Completion;
        FDateTime DeadlineUtc;
        if (!ReadObject(E.ControlPath, Control) || !Control->TryGetStringField(TEXT("state"), State)
            || !Control->TryGetStringField(TEXT("id"), RunId))
        { FinishEndurance(TEXT("failed"), TEXT("Run control became unreadable/invalid.")); return; }
        Control->TryGetStringField(TEXT("completionPolicy"), Completion);
        const bool CompletionDriven = Completion == TEXT("until-complete");
        if (!CompletionDriven && (!Control->TryGetStringField(TEXT("deadlineUtc"), Deadline)
            || !FDateTime::ParseIso8601(*Deadline, DeadlineUtc)))
        { FinishEndurance(TEXT("failed"), TEXT("Run deadline became unreadable/invalid.")); return; }
        if (State != TEXT("running") || RunId != E.RunId || CompletionDriven != E.CompletionDriven
            || (!CompletionDriven && (FDateTime::UtcNow() >= DeadlineUtc || FDateTime::UtcNow() >= E.DeadlineUtc))
            || IFileManager::Get().FileExists(*FPaths::Combine(OutputDirectory, TEXT("stop-endurance.txt"))))
        { FinishEndurance(TEXT("cancelled"), TEXT("Stop marker, run state, or deadline requested graceful exit.")); return; }
    }
    if (PC->IsFailed()) { FinishEndurance(TEXT("failed"), TEXT("Natural survival failure; recovery policy cap0, no concealed retry.")); return; }
    if (E.Events.Num() > 2000 || E.Samples.Num() > FMath::CeilToInt(E.Duration / 30) + 3 || Age > E.Duration + 30)
    { FinishEndurance(TEXT("failed"), TEXT("Bounded driver/evidence timeout.")); return; }
    if (E.Loaded)
    {
        if (PC->IsBookOpen() || PC->IsPlanning()) E.Paused += Delta;
        else { E.Unpaused += Delta; E.EngineUnpaused += EngineDelta; }
        const FVector Position = PC->GetPawn()->GetActorLocation();
        if (PC->GetPawn()->GetVelocity().Size2D() > 20) E.Moving += Delta;
        E.Distance += FVector::Dist2D(Position, E.LastPosition);
        E.LastPosition = Position;
        const auto* Avatar = Cast<AHomesteadCharacter>(PC->GetPawn());
        const auto* Animation = Avatar ? Cast<UHomesteadAnimInstance>(Avatar->GetMesh()->GetAnimInstance()) : nullptr;
        if (!Animation) { FinishEndurance(TEXT("failed"), TEXT("Animation instance unavailable.")); return; }
        const bool Water = Avatar->GetWateringTool()->IsPresented(), Hatchet = Avatar->GetHatchet()->IsPresented();
        E.StuckAction = Animation->ActionWeight() > 0.001f || Water || Hatchet ? E.StuckAction + Delta : 0;
        if (E.StuckAction > 5 || (Water && !Animation->IsWatering()) || (Hatchet && !Animation->IsClearing()) || (Water && Hatchet))
        { FinishEndurance(TEXT("failed"), TEXT("Action/prop failed bounded recovery or arbitration.")); return; }
        if (Age > 60 && Now > E.CaptureExcludeUntil)
        {
            const double Ms = Delta * 1000;
            ++E.FrameCount; E.FrameSum += Ms; E.FrameMaximum = FMath::Max(E.FrameMaximum, Ms);
            ++E.FrameHistogram[FMath::Clamp(FMath::RoundToInt(Ms * 10), 0, 10000)];
        }
        if (Age >= E.NextSample)
        {
            E.NextSample = Age + 30;
            if (!InspectEnduranceSaves()) { FinishEndurance(TEXT("failed"), TEXT("Invalid/cross-world save found.")); return; }
            int32 Available = 0;
            for (const auto& Node : PC->State().resources)
            {
                if (Node.cleared) continue;
                if (PC->Simulation().CanHarvest(Node.id))
                {
                    ++Available;
                    if (E.PreviouslyUnavailable.Remove(Node.id))
                    {
                        ++E.Refreshes;
                        if (E.HarvestedProduceCounts.Contains(Node.id))
                        {
                            auto* Landscape = Cast<AHomesteadWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AHomesteadWorld::StaticClass()));
                            const auto* Base = Landscape ? Landscape->ResourceVisuals.Find(Node.id) : nullptr;
                            const auto* Produce = Landscape ? Landscape->ResourceProduceVisuals.Find(Node.id) : nullptr;
                            if (!Base || !Produce || Base->Components.Num() != E.HarvestedBaseCounts[Node.id]
                                || Produce->Components.Num() != E.HarvestedProduceCounts[Node.id])
                            { FinishEndurance(TEXT("failed"), TEXT("Naturally renewed resource did not restore its actual component inventory.")); return; }
                            for (const auto& Component : Produce->Components)
                                if (!Component || !Component->IsRegistered() || !Component->IsVisible())
                                { FinishEndurance(TEXT("failed"), TEXT("Renewed produce component is absent or hidden.")); return; }
                            E.VerifiedRegrowth.Add(Node.id);
                            EnduranceEvent(FString::Printf(TEXT("naturally renewed previously harvested resource%d; base=%d produce=%d"),
                                Node.id, Base->Components.Num(), Produce->Components.Num()));
                        }
                    }
                }
                else E.PreviouslyUnavailable.Add(Node.id);
            }
            const auto Memory = FPlatformMemory::GetStats();
            E.Samples.Add(FString::Printf(TEXT("%.3f,%.3f,%.3f,%.3f,%.8f,%.5f,%.5f,%.5f,%.3f,%.2f,%llu,%llu,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"),
                Age, E.Paused, E.Unpaused, E.EngineUnpaused, PC->State().hour, PC->State().hunger, PC->State().energy, PC->State().warmth,
                E.Moving, E.Distance, Memory.UsedPhysical, Memory.UsedVirtual, GUObjectArray.GetObjectArrayNumMinusAvailable(),
                static_cast<int32>(PC->State().resources.size()), Available, static_cast<int32>(PC->State().structures.size()),
                static_cast<int32>(PC->State().plots.size()), E.Gathers, E.Eats, E.Waypoints, E.AutosaveWrites, E.Refreshes));
            if (!WriteEnduranceProgress(TEXT("running"), TEXT("Mapped circuit and actual game progression.")))
            { FinishEndurance(TEXT("failed"), TEXT("Could not publish atomic progress.")); return; }
        }
        if (Age >= E.NextCapture && Age < E.Duration - 5)
        {
            E.NextCapture = FMath::Min(Age + 600, E.Duration - 10);
            if (Age >= E.Duration - 11) E.NextCapture = E.Duration;
            const FString Base = FPaths::Combine(OutputDirectory, TEXT("Frames"), FString::Printf(TEXT("milestone-%02d"), CaptureIndex));
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(Base), true);
            auto Metadata = EndurancePresentation();
            Metadata->SetNumberField(TEXT("wallSeconds"), Age);
            Metadata->SetNumberField(TEXT("gameHour"), PC->State().hour);
            Metadata->SetBoolField(TEXT("raining"), PC->Simulation().IsRaining());
            Metadata->SetBoolField(TEXT("night"), PC->Simulation().IsNight());
            Metadata->SetNumberField(TEXT("litGuardTicks"), E.LitGuardTicks);
            FString Text;
            FJsonSerializer::Serialize(Metadata, TJsonWriterFactory<>::Create(&Text));
            if (!AtomicText(Base + TEXT(".json"), Text))
            { FinishEndurance(TEXT("failed"), TEXT("Could not persist milestone presentation evidence.")); return; }
            FScreenshotRequest::RequestScreenshot(Base + TEXT(".png"), true, false);
            ++CaptureIndex;
            E.CaptureExcludeUntil = Now + 2;
        }
        if (Age >= E.Duration && !E.Finalizing) { E.Finalizing = true; Go(Phase::Save); }
    }
    FVector2D Move = FVector2D::ZeroVector, Look = FVector2D::ZeroVector;
    if (E.Step == Phase::Walk)
    {
        const auto P = PC->PlayerPoint();
        const double Remaining = FVector2D::Distance(E.Target, FVector2D(P.x, P.y));
        if (Remaining < E.ProgressDistance - 10) { E.ProgressAt = Now; E.ProgressDistance = Remaining; }
        if (WalkWaterTarget(E.Target, E.ForageId >= 0 ? 35 : 45, Delta, Move, Look))
        {
            if (E.ForageId >= 0)
            {
                if (!PC->IsResourceFocused(E.ForageId)) { FinishEndurance(TEXT("failed"), TEXT("Mapped approach reached mismatching focus.")); return; }
                E.BeforeCount = PC->Simulation().UsedCapacity();
                auto* Landscape = Cast<AHomesteadWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AHomesteadWorld::StaticClass()));
                const auto* Base = Landscape ? Landscape->ResourceVisuals.Find(E.ForageId) : nullptr;
                const auto* Produce = Landscape ? Landscape->ResourceProduceVisuals.Find(E.ForageId) : nullptr;
                if (!Base || !Produce || Produce->Components.IsEmpty())
                { FinishEndurance(TEXT("failed"), TEXT("Harvestable target has no actual produce component inventory.")); return; }
                E.HarvestedBaseCounts.Add(E.ForageId, Base->Components.Num());
                E.HarvestedProduceCounts.Add(E.ForageId, Produce->Components.Num());
                const double BeforeHour = PC->State().hour;
                Tap(EKeys::Gamepad_FaceButton_Bottom);
                E.ActionHours += PC->State().hour - BeforeHour;
                Go(Phase::Gather);
            }
            else { ++E.Waypoints; Go(Phase::Choose); }
        }
        else if (Now - E.ProgressAt > 12 || Now - E.StepStarted > 45)
        {
            ++E.NavigationFailures;
            EnduranceEvent(FString::Printf(TEXT("Navigation miss%d target=%s remaining=%.1f"), E.NavigationFailures, *E.Target.ToString(), Remaining));
            if (E.NavigationFailures > 3) { FinishEndurance(TEXT("failed"), TEXT("Navigation failure cap exceeded.")); return; }
            E.ForageId = -1; Go(Phase::Choose); Move = Look = FVector2D::ZeroVector;
        }
        ApplyAxes(Move, Look);
        return;
    }
    ApplyAxes({}, {});
    if (Now - E.LastDecision < 0.25) return;
    E.LastDecision = Now;
    switch (E.Step)
    {
    case Phase::Load:
        if (!TapEnduranceLoad()) return;
        Tap(EKeys::Gamepad_Special_Right); Go(Phase::Settle); break;
    case Phase::Settle:
        if (Now - E.StepStarted < 1) break;
        if (PC->WorldId != E.WorldId || (!E.FreshWorld && UTF8_TO_TCHAR(PC->Simulation().Serialize().c_str()) != E.SavedState))
        { FinishEndurance(TEXT("failed"), TEXT("Fixture load did not restore exact disclosed state.")); return; }
        E.Loaded = true; E.Started = E.LastTick = Now; E.InitialHour = PC->State().hour;
        E.LastPosition = PC->GetPawn()->GetActorLocation();
        if (PC->IsBookOpen()) Tap(EKeys::Gamepad_FaceButton_Right);
        Go(Phase::Choose); break;
    case Phase::Choose:
    {
        if (Age >= E.NextSave) { Go(Phase::Save); break; }
        if (PC->State().hunger < (E.FreshWorld && E.Eats == 0 ? 95 : 75))
        {
            E.FoodId = -1;
            for (const auto Item : {Homestead::Item::HerbedRoots, Homestead::Item::RoastedRoots, Homestead::Item::Berries})
                if (PC->Simulation().Count(Item) > 0) { E.FoodId = static_cast<int32>(Item); break; }
            if (E.FoodId >= 0)
            {
                E.LastMenuSubject.Empty(); E.MenuDirection = 1;
                Tap(EKeys::Gamepad_Special_Right); Go(Phase::EatSelect); break;
            }
        }
        E.ForageId = -1;
        if (Age >= E.NextForage && PC->Simulation().UsedCapacity() < 105)
        {
            E.NextForage = Age + 60;
            double Best = 1.e20;
            const auto P = PC->PlayerPoint();
            for (const auto& Node : PC->State().resources)
            {
                if (!PC->Simulation().CanHarvest(Node.id) || Node.position.x < -600 || Node.position.x > 150
                    || Node.position.y < -450 || Node.position.y > -250) continue;
                if (E.FreshWorld && E.Eats == 0 && Node.kind != Homestead::ResourceKind::BerryBush) continue;
                Homestead::Item Item;
                int32 Desired;
                switch (Node.kind)
                {
                case Homestead::ResourceKind::BerryBush: Item = Homestead::Item::Berries; Desired = 18; break;
                case Homestead::ResourceKind::Branches: Item = Homestead::Item::Branch; Desired = 18; break;
                case Homestead::ResourceKind::Flowers: Item = Homestead::Item::Flowers; Desired = 5; break;
                default: continue;
                }
                if (PC->Simulation().Count(Item) >= Desired) continue;
                const double Dist = FMath::Square(P.x - Node.position.x) + FMath::Square(P.y - Node.position.y);
                if (Dist < Best) { Best = Dist; E.ForageId = Node.id; E.Target = FVector2D(Node.position.x, Node.position.y); }
            }
        }
        if (E.ForageId < 0)
        {
            const FVector2D Circuit[] = {{-1300, 350}, {-500, 350}, {-500, 0}, {-1300, 0}};
            E.Target = Circuit[E.Waypoint++ % UE_ARRAY_COUNT(Circuit)];
        }
        const auto P = PC->PlayerPoint();
        E.ProgressDistance = FVector2D::Distance(E.Target, FVector2D(P.x, P.y));
        E.ProgressAt = Now; WaterSettle = 0; Go(Phase::Walk);
        break;
    }
    case Phase::Gather:
    {
        if (Now - E.StepStarted < 2) break;
        if (PC->ToastIsError() || PC->Simulation().UsedCapacity() <= E.BeforeCount || PC->Simulation().CanHarvest(E.ForageId))
        { FinishEndurance(TEXT("failed"), TEXT("Mapped gather did not succeed exactly as observed.")); return; }
        auto* Landscape = Cast<AHomesteadWorld>(UGameplayStatics::GetActorOfClass(GetWorld(), AHomesteadWorld::StaticClass()));
        const auto* Base = Landscape ? Landscape->ResourceVisuals.Find(E.ForageId) : nullptr;
        const auto* Produce = Landscape ? Landscape->ResourceProduceVisuals.Find(E.ForageId) : nullptr;
        if (!Base || Base->Components.Num() != E.HarvestedBaseCounts[E.ForageId]
            || (Produce && !Produce->Components.IsEmpty()))
        { FinishEndurance(TEXT("failed"), TEXT("Harvest did not preserve the base and remove the actual produce.")); return; }
        E.PreviouslyUnavailable.Add(E.ForageId);
        ++E.Gathers; EnduranceEvent(FString::Printf(TEXT("gathered resource%d"), E.ForageId)); Go(Phase::Choose); break;
    }
    case Phase::EatSelect:
    {
        const auto* Subject = PC->NativeMenu ? PC->NativeMenu->GetSelectedSubject() : nullptr;
        if (Now - E.StepStarted > 8 || !Subject || PC->BookPage() != 0)
        { FinishEndurance(TEXT("failed"), TEXT("Pack navigation hung.")); return; }
        if (Subject->Subject != EHomesteadMenuSubject::ItemGroup || Subject->Id != E.FoodId)
        {
            const FString Key = FString::Printf(TEXT("%d:%d"), static_cast<int32>(Subject->Subject), Subject->SubjectId);
            if (Key == E.LastMenuSubject)
            { Tap(EKeys::Gamepad_DPad_Down); E.MenuDirection *= -1; E.LastMenuSubject.Empty(); }
            else
            { E.LastMenuSubject = Key; Tap(E.MenuDirection > 0 ? EKeys::Gamepad_DPad_Right : EKeys::Gamepad_DPad_Left); }
            break;
        }
        E.BeforeCount = PC->State().inventory[E.FoodId];
        Tap(EKeys::Gamepad_FaceButton_Bottom);
        Tap(EKeys::Gamepad_FaceButton_Bottom);
        Go(Phase::EatCheck); break;
    }
    case Phase::EatCheck:
        if (PC->ToastIsError() || PC->State().inventory[E.FoodId] != E.BeforeCount - 1)
        { FinishEndurance(TEXT("failed"), TEXT("Mapped eating failed.")); return; }
        ++E.Eats; EnduranceEvent(FString(TEXT("ate1 ")) + UTF8_TO_TCHAR(Homestead::ItemName(static_cast<Homestead::Item>(E.FoodId))));
        Go(Phase::Close); break;
    case Phase::Close:
        Tap(EKeys::Gamepad_FaceButton_Right); Go(Phase::Choose); break;
    case Phase::Save:
        if (!PC->IsBookOpen()) Tap(EKeys::Gamepad_Special_Right);
        E.SavedState = UTF8_TO_TCHAR(PC->Simulation().Serialize().c_str());
        Tap(EKeys::F5);
        if (PC->ToastIsError() || !InspectEnduranceSaves())
        { FinishEndurance(TEXT("failed"), TEXT("Manual save/integrity check failed.")); return; }
        ++E.ManualSaves; EnduranceEvent(TEXT("manual save; no progression rewind"));
        Go(E.RoundTrip ? Phase::Resume : Phase::Reload); break;
    case Phase::Reload:
        if (!TapEnduranceLoad()) return;
        Tap(EKeys::Gamepad_Special_Right); ++E.Loads; Go(Phase::Resume); break;
    case Phase::Resume:
        if (PC->WorldId != E.WorldId || UTF8_TO_TCHAR(PC->Simulation().Serialize().c_str()) != E.SavedState)
        { FinishEndurance(TEXT("failed"), TEXT("Paused save/load roundtrip changed world/state.")); return; }
        E.RoundTrip = true; E.NextSave = Age + 300;
        if (E.Finalizing)
        {
            const bool Long = E.Duration >= 2700;
            const bool Passed = E.Unpaused >= E.Duration * 0.9 && E.Moving >= E.Duration * 0.3
                && E.Gathers >= (Long ? 2 : 1) && E.Eats >= 1 && E.Waypoints >= (Long ? 40 : 4)
                && E.Loads == 1 && E.RoundTrip && InspectEnduranceSaves()
                && (!Long || (PC->State().hour - E.InitialHour - E.ActionHours >= 15 && E.SaveStamps.Num() == 3
                    && E.AutosaveWrites >= 8))
                && (E.Duration != 4200 || (PC->State().hour - E.InitialHour - E.ActionHours >= 24
                    && E.VerifiedRegrowth.Num() >= 1));
            FinishEndurance(Passed ? TEXT("passed") : TEXT("failed"), TEXT("Fixed duration/participation/action/save criteria evaluated."));
            return;
        }
        Tap(EKeys::Gamepad_FaceButton_Right); Go(Phase::Choose); break;
    }
}
