#include "HomesteadController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "InputKeyEventArgs.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

bool AHomesteadController::PrepareStartupProbe()
{
    if (!FParse::Value(FCommandLine::Get(), TEXT("HomesteadStartupProbe="), StartupProbeDirectory)) return true;
    StartupProbeDirectory = FPaths::ConvertRelativePathToFull(StartupProbeDirectory);
    const auto* Branch = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    const FString ExpectedProfile = TEXT("offline-") + FMD5::HashAnsiString(*StartupProbeDirectory).Left(12).ToLower();
    if (SaveRoute.Mode != TEXT("preview") || SaveRoute.Profile != ExpectedProfile || !Branch
        || !FPaths::IsSamePath(Branch->IniPath, FPaths::Combine(StartupProbeDirectory, TEXT("Graphics/GameUserSettings.ini")))
        || !FPaths::IsUnderDirectory(FPaths::ProjectSavedDir(), StartupProbeDirectory))
    {
        UE_LOG(LogTemp, Error, TEXT("Startup probe requires exact isolated profile, graphics and user directories."));
        FPlatformMisc::RequestExitWithStatus(true, 2);
        return false;
    }
    StartupProbeNext = FPlatformTime::Seconds() + 3;
    StartupProbeDeadline = StartupProbeNext + 45;
    return true;
}

void AHomesteadController::TickStartupProbe()
{
    if (StartupProbeStep < 0) return;
    const auto* Viewport = GEngine->GameViewport.Get();
    if (!Viewport || Viewport->ViewModeIndex != VMI_Lit || !Viewport->EngineShowFlags.Lighting
        || Viewport->EngineShowFlags.ShaderComplexity)
    { FinishStartupProbe(TEXT("Actual renderer is not normal Lit.")); return; }
    ++StartupProbeLitTicks;
    const double Now = FPlatformTime::Seconds();
    if (Now > StartupProbeDeadline || IFileManager::Get().FileExists(*FPaths::Combine(StartupProbeDirectory, TEXT("stop-probe.txt"))))
    { FinishStartupProbe(TEXT("Startup probe cancelled or timed out.")); return; }
    if (Now < StartupProbeNext || !GetPawn()) return;
    StartupProbeNext = Now + 1;
    const auto Tap = [this](FKey Key)
    {
        InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Pressed, 1));
        InputKey(FInputKeyEventArgs::CreateSimulated(Key, IE_Released, 0));
    };
    switch (StartupProbeStep++)
    {
    case 0:
        StartupProbeWorld = WorldId;
        if (!bBookOpen) Tap(EKeys::Tab);
        break;
    case 1:
        if (!bBookOpen) { FinishStartupProbe(TEXT("Mapped Tab failed to open the paused book.")); return; }
        StartupProbeExpectedState = UTF8_TO_TCHAR(Sim.Serialize().c_str());
        Tap(EKeys::F5);
        break;
    case 2:
        if (TestQuickSaves != 1 || ToastIsError() || WorldId != StartupProbeWorld)
        { FinishStartupProbe(TEXT("Mapped F5 did not save exactly once.")); return; }
        Tap(EKeys::F9);
        break;
    case 3:
        if (TestQuickLoads != 1 || ToastIsError() || WorldId != StartupProbeWorld
            || StartupProbeExpectedState != StartupProbeLoadedState
            || FScreenshotRequest::IsScreenshotRequested())
        { FinishStartupProbe(TEXT("Mapped F9 save roundtrip/state/no-screenshot check failed.")); return; }
        StartupProbeNext = Now + 10;
        break;
    case 4:
        FinishStartupProbe(FString());
        break;
    }
}

void AHomesteadController::FinishStartupProbe(const FString& Error)
{
    StartupProbeStep = -1;
    auto Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("status"), Error.IsEmpty() ? TEXT("passed") : TEXT("failed"));
    Result->SetStringField(TEXT("error"), Error);
    Result->SetBoolField(TEXT("shipping"), UE_BUILD_SHIPPING != 0);
    Result->SetBoolField(TEXT("traceCompiled"), UE_TRACE_ENABLED != 0);
    Result->SetStringField(TEXT("profile"), SaveRoute.Profile);
    Result->SetStringField(TEXT("saveDirectory"), SaveRoute.Directory);
    Result->SetStringField(TEXT("projectSavedDirectory"), FPaths::ProjectSavedDir());
    const auto* Branch = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    Result->SetStringField(TEXT("graphicsFile"), Branch ? Branch->IniPath : TEXT("missing"));
    Result->SetStringField(TEXT("world"), WorldId);
    Result->SetStringField(TEXT("simulationMd5"), FMD5::HashAnsiString(UTF8_TO_TCHAR(Sim.Serialize().c_str())));
    Result->SetStringField(TEXT("savedSimulationMd5"), FMD5::HashAnsiString(*StartupProbeExpectedState));
    Result->SetStringField(TEXT("loadedSimulationMd5"), FMD5::HashAnsiString(*StartupProbeLoadedState));
    Result->SetBoolField(TEXT("heroinePresent"), HasHeroine());
    Result->SetBoolField(TEXT("pausedBook"), bBookOpen);
    Result->SetBoolField(TEXT("probeOnlySimulatedInput"), bAutomatedInputOnly);
    Result->SetNumberField(TEXT("saveDispatches"), TestQuickSaves);
    Result->SetNumberField(TEXT("loadDispatches"), TestQuickLoads);
    Result->SetNumberField(TEXT("litGuardTicks"), static_cast<double>(StartupProbeLitTicks));
    Result->SetBoolField(TEXT("shotRequested"), FScreenshotRequest::IsScreenshotRequested());
    if (GEngine && GEngine->GameViewport && GEngine->GameViewport->Viewport)
    {
        const auto* Viewport = GEngine->GameViewport.Get();
        Result->SetNumberField(TEXT("viewMode"), Viewport->ViewModeIndex);
        Result->SetStringField(TEXT("showFlags"), Viewport->EngineShowFlags.ToString());
        Result->SetNumberField(TEXT("actualWindowMode"), Viewport->Viewport->GetWindowMode());
        const FIntPoint Size = Viewport->Viewport->GetSizeXY();
        Result->SetNumberField(TEXT("viewportWidth"), Size.X);
        Result->SetNumberField(TEXT("viewportHeight"), Size.Y);
    }
    if (const auto* Settings = GEngine->GetGameUserSettings())
    {
        Result->SetNumberField(TEXT("savedWindowMode"), Settings->GetFullscreenMode());
        Result->SetNumberField(TEXT("savedWidth"), Settings->GetScreenResolution().X);
        Result->SetNumberField(TEXT("savedHeight"), Settings->GetScreenResolution().Y);
        Result->SetBoolField(TEXT("vsyncPreference"), Settings->IsVSyncEnabled());
        Result->SetNumberField(TEXT("frameLimit"), Settings->GetFrameRateLimit());
    }
    for (const TCHAR* Name : {TEXT("r.VSync"), TEXT("r.ScreenPercentage"), TEXT("r.AntiAliasingMethod"), TEXT("t.MaxFPS")})
        if (const auto* Variable = IConsoleManager::Get().FindConsoleVariable(Name))
            Result->SetStringField(Name, Variable->GetString());
    FString Json;
    FJsonSerializer::Serialize(Result, TJsonWriterFactory<>::Create(&Json));
    const FString Path = FPaths::Combine(StartupProbeDirectory, TEXT("startup-probe.json"));
    const bool Written = FFileHelper::SaveStringToFile(Json, *(Path + TEXT(".tmp")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && IFileManager::Get().Move(*Path, *(Path + TEXT(".tmp")), true, true);
    if (!Written) Notify(TEXT("The isolated startup probe could not write its evidence."), true);
    FPlatformMisc::RequestExitWithStatus(false, Error.IsEmpty() && Written ? 0 : 1);
}
