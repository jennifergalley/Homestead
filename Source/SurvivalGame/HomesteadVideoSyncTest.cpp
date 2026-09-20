#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadTestPaths.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
FString VideoSnapshot()
{
    const auto* Settings = GEngine->GetGameUserSettings();
    float Normalized = 0, Scale = 0, Minimum = 0, Maximum = 0;
    Settings->GetResolutionScaleInformationEx(Normalized, Scale, Minimum, Maximum);
    const FIntPoint Resolution = Settings->GetScreenResolution();
    const FIntPoint View = GEngine->GameViewport->Viewport->GetSizeXY();
    FString Result = FString::Printf(TEXT("resolution=%d,%d mode=%d preferred=%d scale=%.3f cap=%.3f dynamic=%d viewport=%d,%d actual_mode=%d"),
        Resolution.X, Resolution.Y, static_cast<int32>(Settings->GetFullscreenMode()),
        static_cast<int32>(Settings->GetPreferredFullscreenMode()), Scale, Settings->GetFrameRateLimit(),
        Settings->IsDynamicResolutionEnabled(), View.X, View.Y, static_cast<int32>(GEngine->GameViewport->Viewport->GetWindowMode()));
    for (const TCHAR* Name : {TEXT("r.ScreenPercentage"), TEXT("t.MaxFPS"), TEXT("r.FullScreenMode"),
        TEXT("r.AntiAliasingMethod"), TEXT("r.DynamicRes.OperationMode"), TEXT("sg.ViewDistanceQuality"),
        TEXT("sg.ShadowQuality"), TEXT("r.TSR.History.ScreenPercentage")})
        Result += FString::Printf(TEXT(" %s=%s"), Name, *IConsoleManager::Get().FindConsoleVariable(Name)->GetString());
    return Result;
}

FString OtherStoredPreferences(const FString& Path)
{
    FConfigFile Disk;
    Disk.Read(Path);
    TArray<FString> Entries;
    for (const auto& Section : Disk)
        for (const auto& Entry : Section.Value)
            if (Entry.Key != TEXT("bUseVSync"))
                Entries.Add(Section.Key + TEXT(":") + Entry.Key.ToString() + TEXT("=") + Entry.Value.GetValue());
    Entries.Sort();
    return FString::Join(Entries, TEXT("\n"));
}
}

void AHomesteadSmokeTest::PrepareVideoSyncChecks()
{
    const auto* Branch = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    const FString Expected = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("Graphics"), TEXT("GameUserSettings.ini")));
    if (!Branch || !FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(Branch->IniPath), Expected))
    {
        Finish(false, TEXT("Video fixture requires the explicit synthetic output/Graphics/GameUserSettings.ini destination."));
        return;
    }
    FString Mode;
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadVideoSyncPhase="), Mode);
    if (Mode != TEXT("write-on") && Mode != TEXT("read-on-write-off")
        && Mode != TEXT("read-off") && Mode != TEXT("override"))
    {
        Finish(false, TEXT("Unknown isolated video-sync fixture phase."));
        return;
    }
    auto* Settings = GEngine->GetGameUserSettings();
    auto* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
    const auto OtherVideo = MakeShared<FString>(VideoSnapshot());
    const auto Stored = MakeShared<FString>(OtherStoredPreferences(Expected));
    const auto World = MakeShared<std::string>(Controller->Simulation().Serialize());
    const auto View = MakeShared<FRotator>(Controller->GetControlRotation());
    Results.Add(TEXT("GRAPHICS_CONFIG=") + FPaths::ConvertRelativePathToFull(Branch->IniPath));
    Results.Add(TEXT("BASELINE ") + *OtherVideo);
    FFileHelper::SaveStringToFile(*Stored, *FPaths::Combine(HomesteadTestOutputDirectory(), Mode + TEXT("-other-preferences.txt")));
    Results.Add(TEXT("DISCLOSURE offscreen native UI/runtime setting proof; not observed scanout or tearing acceptance."));
    const auto Stable = [this, OtherVideo, Stored, Expected, World, View]()
    {
        const bool Video = VideoSnapshot() == *OtherVideo;
        const bool Config = OtherStoredPreferences(Expected) == *Stored;
        const bool Simulation = Controller->Simulation().Serialize() == *World;
        const bool Camera = Controller->GetControlRotation() == *View;
        if (!Video || !Config || !Simulation || !Camera)
            Results.Add(FString::Printf(TEXT("STABILITY video=%d config=%d simulation=%d camera=%d\nAFTER %s\nCONFIG_AFTER\n%s"),
                Video, Config, Simulation, Camera, *VideoSnapshot(), *OtherStoredPreferences(Expected)));
        return Video && Config && Simulation && Camera;
    };
    const auto StateMatches = [this, Settings, VSync](bool Value)
    {
        const auto Rows = Controller->Rows();
        return Settings->IsVSyncEnabled() == Value && (VSync->GetInt() != 0) == Value
            && Rows.IsValidIndex(11) && Rows[11].Id == 11
            && Rows[11].Label == (Value ? TEXT("Vertical sync: On") : TEXT("Vertical sync: Off"));
    };
    Add(TEXT("Initial preference loaded from the shared synthetic file"),
        []() {}, [Settings, VSync, Mode]()
        {
            const bool On = Mode == TEXT("read-on-write-off");
            return Settings->IsVSyncEnabled() == On && (VSync->GetInt() != 0) == (On || Mode == TEXT("override"));
        });
    Add(TEXT("Controller navigates from Notes to existing Settings page"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 4 && Controller->UsesGamepad(); });
    Add(TEXT("Existing Settings row identities remain0-10; VSync appended as11"),
        []() {}, [this]()
        {
            const auto Rows = Controller->Rows();
            if (Rows.Num() != 12) return false;
            for (int32 Index = 0; Index < Rows.Num(); ++Index) if (Rows[Index].Id != Index) return false;
            return true;
        });
    QueueSelectRow(11);
    Add(TEXT("Scrolled toggle retains correct controller action hint"),
        []() {}, [this, Stable]() { return Stable() && Controller->BookFooter().Contains(TEXT("A: toggle")); });
    if (Mode == TEXT("override"))
    {
        Add(TEXT("Higher-priority runtime override is visible without falsely changing preference"),
            []() {}, [this, VSync]() { return (VSync->GetFlags() & ECVF_SetByMask) > ECVF_SetByGameSetting
                && Controller->Rows()[11].Label.Contains(TEXT("Off | active On (override)")); });
        QueueBookCapture(TEXT("sync-override"));
        Add(TEXT("Matching active override can store an intentional On preference"),
            [this]() { Tap(EKeys::Enter); },
            [this, Settings, VSync, Stable]() { return !Controller->ToastIsError() && Settings->IsVSyncEnabled()
                && VSync->GetInt() == 1 && Stable(); });
        for (FKey Key : {EKeys::Gamepad_FaceButton_Bottom, EKeys::Enter})
            Add(TEXT("Override rejects contradictory toggle through ") + Key.ToString(),
                [this, Key]() { Tap(Key); },
                [this, Settings, VSync, Stable]()
                {
                    return Settings->IsVSyncEnabled() && VSync->GetInt() == 1 && Controller->ToastIsError()
                        && Controller->Toast().Contains(TEXT("override")) && Stable();
                });
    }
    else if (Mode == TEXT("read-off"))
    {
        Add(TEXT("Off survives second relaunch and all unrelated settings remain"),
            []() {}, [StateMatches, Stable]() { return StateMatches(false) && Stable(); });
    }
    else
    {
        const bool InitialOn = Mode == TEXT("read-on-write-off");
        Add(TEXT("Actual loaded preference agrees with row and runtime"),
            []() {}, [StateMatches, InitialOn]() { return StateMatches(InitialOn); });
        if (!InitialOn) QueueBookCapture(TEXT("sync-off-gamepad"));
        const auto Toggle = [this, StateMatches, Stable](FKey Key, bool On, const FString& Capture)
        {
            Add(FString(TEXT("Toggle to ")) + (On ? TEXT("On via ") : TEXT("Off via ")) + Key.ToString(),
                [this, Key]() { Tap(Key); },
                [this, StateMatches, Stable, Key, On]()
                {
                    return StateMatches(On) && Stable() && !Controller->ToastIsError()
                        && Controller->UsesGamepad() == Key.IsGamepadKey();
                });
            if (!Capture.IsEmpty()) QueueBookCapture(Capture);
        };
        if (!InitialOn)
        {
            Toggle(EKeys::Gamepad_FaceButton_Bottom, true, TEXT("sync-on-gamepad"));
            Toggle(EKeys::Enter, false, TEXT("sync-off-keyboard"));
            Toggle(EKeys::Enter, true, TEXT("sync-on-keyboard"));
            Toggle(EKeys::Gamepad_FaceButton_Bottom, false, {});
            Toggle(EKeys::Gamepad_FaceButton_Bottom, true, {});
        }
        else
        {
            Toggle(EKeys::Enter, false, {});
            Add(TEXT("Read-only synthetic graphics file surfaces save failure and rolls back"),
                [this, Expected]() { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Expected, true); Tap(EKeys::Enter); },
                [this, StateMatches, Stable, Expected]()
                {
                    const bool Passed = Controller->ToastIsError() && Controller->Toast().Contains(TEXT("Could not save"))
                        && StateMatches(false) && Stable();
                    FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Expected, false);
                    return Passed;
                });
            Toggle(EKeys::Gamepad_FaceButton_Bottom, true, {});
            Toggle(EKeys::Enter, false, {});
        }
    }
    Add(TEXT("Native setting proof leaves world, camera and other video preferences unchanged"),
        []() {}, Stable);
    Add(TEXT("Keyboard page navigation remains available"), [this]() { Tap(EKeys::Right); },
        [this]() { return Controller->BookPage() == 5 && !Controller->UsesGamepad(); });
    Add(TEXT("Controller page navigation restores Settings"), [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this]() { return Controller->BookPage() == 4 && Controller->SelectedRow() == 0 && Controller->UsesGamepad(); });
}
