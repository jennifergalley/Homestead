#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "UI/SHomesteadMenu.h"
#include "HomesteadSave.h"
#include "HomesteadTestPaths.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerInput.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "HAL/PlatformProcess.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void AHomesteadSmokeTest::PrepareHotkeyChecks()
{
#if !UE_BUILD_SHIPPING
    const FString Output = HomesteadTestOutputDirectory();
    FString Phase;
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadHotkeyPhase="), Phase);
    const bool Baseline = Phase == TEXT("baseline"), Reload = Phase == TEXT("reload");
    const FString Graphics = FPaths::Combine(Output, TEXT("Graphics"), TEXT("GameUserSettings.ini"));
    const auto* Branch = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    const FString ShotRoot = FPaths::ConvertRelativePathToFull(FPaths::ScreenShotDir());
    if ((Phase != TEXT("write") && !Baseline && !Reload) || !Branch
        || !FPaths::IsSamePath(Branch->IniPath, Graphics) || !FPaths::IsUnderDirectory(ShotRoot, Output))
    { Finish(false, TEXT("Hotkey fixture requires an explicit phase and isolated actual graphics/screenshot destinations.")); return; }

    if (!Baseline)
    {
        const FString Profile = TEXT("hotkey-") + FMD5::HashAnsiString(*Output).Left(12).ToLower();
        FHomesteadSaveRoute Route; FString Error;
        if (!ResolveHomesteadSaveRoute(*(TEXT("-HomesteadPreviewProfile=") + Profile), FPaths::ProjectSavedDir(),
            FPlatformProcess::UserSettingsDir(), Output, Route, Error) || Route.Mode != TEXT("preview")
            || Route.Profile != Profile || IFileManager::Get().DirectoryExists(*Route.Directory) != Reload)
        { Finish(false, TEXT("Refusing an unexpected existing/missing synthetic preview namespace: ") + Error); return; }
        Controller->SaveRoute = Route;
        Results.Add(TEXT("PROFILE=") + Profile);
    }
    Results.Add(TEXT("SAVE_DIRECTORY=") + Controller->SaveRoute.Directory);
    Results.Add(TEXT("GRAPHICS_CONFIG=") + Branch->IniPath);
    Results.Add(TEXT("SCREENSHOT_DIRECTORY=") + ShotRoot);
    Results.Add(TEXT("DISCLOSURE existing smoke actor, simulated inputs; fresh synthetic preview route resolved with the production helper. No personal saves or explicit fixture image requests."));
    const auto Expected = MakeShared<FString>();
    const auto WritesOkay = MakeShared<bool>(true);
    const auto Current = [this]()
    {
        const auto& A = Controller->Appearance;
        return Controller->WorldId + TEXT("\n") + UTF8_TO_TCHAR(Controller->Sim.Serialize().c_str())
            + FString::Printf(TEXT("\n%d,%d,%d,%d,%d,%d,%d"), A.HairStyle, A.HairColor,
                A.SkinTone, A.EyeColor, A.TunicColor, A.Outfit, A.BodyPreset);
    };
    const auto ShotCount = [ShotRoot]()
    {
        TArray<FString> Files;
        IFileManager::Get().FindFilesRecursive(Files, *ShotRoot, TEXT("*.png"), true, false);
        return Files.Num();
    };
    const auto Snapshot = [this, Output, Phase, WritesOkay, ShotCount](const FString& Label)
    {
        const auto* Viewport = GEngine->GameViewport.Get();
        auto Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("label"), Label);
        Object->SetNumberField(TEXT("viewMode"), Viewport->ViewModeIndex);
        Object->SetNumberField(TEXT("litEnum"), VMI_Lit);
        Object->SetNumberField(TEXT("shaderComplexityEnum"), VMI_ShaderComplexity);
        Object->SetStringField(TEXT("showFlags"), Viewport->EngineShowFlags.ToString());
        Object->SetBoolField(TEXT("shotRequested"), FScreenshotRequest::IsScreenshotRequested());
        Object->SetStringField(TEXT("shotFilename"), FScreenshotRequest::GetFilename());
        Object->SetNumberField(TEXT("screenshotFiles"), ShotCount());
        Object->SetNumberField(TEXT("saveDispatches"), Controller->TestQuickSaves);
        Object->SetNumberField(TEXT("loadDispatches"), Controller->TestQuickLoads);
        Object->SetStringField(TEXT("world"), Controller->WorldId);
        Object->SetStringField(TEXT("simulationMd5"), FMD5::HashAnsiString(UTF8_TO_TCHAR(Controller->Sim.Serialize().c_str())));
        const auto& Look = Controller->Appearance;
        Object->SetStringField(TEXT("look"), FString::Printf(TEXT("%d,%d,%d,%d,%d,%d,%d"),
            Look.HairStyle, Look.HairColor, Look.SkinTone, Look.EyeColor, Look.TunicColor, Look.Outfit, Look.BodyPreset));
        Object->SetStringField(TEXT("F5"), Controller->PlayerInput->GetBind(EKeys::F5));
        Object->SetStringField(TEXT("F9"), Controller->PlayerInput->GetBind(EKeys::F9));
        Object->SetStringField(TEXT("F2"), Controller->PlayerInput->GetBind(EKeys::F2));
        Object->SetStringField(TEXT("F3"), Controller->PlayerInput->GetBind(EKeys::F3));
        Object->SetStringField(TEXT("toast"), Controller->Toast());
        Object->SetBoolField(TEXT("error"), Controller->ToastIsError());
        FString Text; FJsonSerializer::Serialize(Object, TJsonWriterFactory<>::Create(&Text));
        Text.ReplaceInline(TEXT("\r"), TEXT("")); Text.ReplaceInline(TEXT("\n"), TEXT("")); Text += TEXT("\n");
        *WritesOkay &= FFileHelper::SaveStringToFile(Text, *FPaths::Combine(Output, TEXT("hotkey-") + Phase + TEXT(".jsonl")),
            FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append);
    };
    Add(TEXT("Fresh process is actually Lit with expected effective bindings and no pending screenshots"),
        [Snapshot]() { Snapshot(TEXT("fresh-process")); },
        [this, Baseline, ShotCount]()
        {
            auto* Input = Controller->PlayerInput.Get();
            return GEngine->GameViewport->ViewModeIndex == VMI_Lit
                && !GEngine->GameViewport->EngineShowFlags.ShaderComplexity
                && !FScreenshotRequest::IsScreenshotRequested() && ShotCount() == 0
                && Input->GetBind(EKeys::F5) == (Baseline ? TEXT("viewmode shadercomplexity") : TEXT(""))
                && Input->GetBind(EKeys::F9) == (Baseline ? TEXT("shot showui") : TEXT(""))
                && Input->GetBind(EKeys::F2) == TEXT("viewmode unlit") && Input->GetBind(EKeys::F3) == TEXT("viewmode lit");
        });
    const auto Action = [this, Baseline, Snapshot, Current, Expected, ShotCount](FKey Key, bool Load, bool Failure = false)
    {
        const auto Flags = MakeShared<FString>();
        const auto Mode = MakeShared<int32>();
        const auto Calls = MakeShared<uint32>();
        const auto Pending = MakeShared<bool>();
        const auto Files = MakeShared<int32>();
        Add(TEXT("Exactly one real action with observed renderer/request state: ") + Key.ToString(),
            [this, Key, Load, Failure, Snapshot, Current, Expected, ShotCount, Flags, Mode, Calls, Pending, Files]()
            {
                *Flags = GEngine->GameViewport->EngineShowFlags.ToString(); *Mode = GEngine->GameViewport->ViewModeIndex;
                *Calls = Load ? Controller->TestQuickLoads : Controller->TestQuickSaves; *Files = ShotCount();
                if (!Load && !Failure) *Expected = Current();
                Snapshot(TEXT("before-") + Key.ToString());
                Tap(Key);
                *Pending = FScreenshotRequest::IsScreenshotRequested();
                Snapshot(TEXT("after-dispatch-") + Key.ToString());
                if (Load && !Failure) Tap(EKeys::Gamepad_Special_Right);
            },
            [this, Key, Load, Failure, Baseline, Snapshot, Current, Expected, ShotCount, Flags, Mode, Calls, Pending, Files]()
            {
                Snapshot(TEXT("after-action-") + Key.ToString());
                const bool DebugSave = Baseline && Key == EKeys::F5;
                const bool DebugShot = Baseline && Key == EKeys::F9;
                const auto* Viewport = GEngine->GameViewport.Get();
                const bool Rendering = DebugSave
                    ? Viewport->ViewModeIndex == VMI_ShaderComplexity && Viewport->EngineShowFlags.ToString() != *Flags
                    : Viewport->ViewModeIndex == *Mode && Viewport->EngineShowFlags.ToString() == *Flags;
                if (!Rendering || *Pending != DebugShot || FScreenshotRequest::IsScreenshotRequested()
                    || ShotCount() != *Files + (DebugShot ? 1 : 0)
                    || (Load ? Controller->TestQuickLoads : Controller->TestQuickSaves) != *Calls + 1) return false;
                if (Failure) return Controller->ToastIsError() && Controller->Toast().Contains(Load ? TEXT("no usable save") : TEXT("Save failed"));
                if (Controller->ToastIsError() || Current() != *Expected) return false;
                const auto* Save = Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual")));
                return Save && Save->WorldId == Controller->WorldId
                    && Save->SimulationData == UTF8_TO_TCHAR(Controller->Sim.Serialize().c_str())
                    && Save->HairColor == Controller->Appearance.HairColor;
            }, 0.8f);
    };

    if (Baseline)
    {
        Action(EKeys::F9, true, true);
        Action(EKeys::F5, false);
        Action(EKeys::F9, true);
    }
    else if (Reload)
    {
        Add(TEXT("Load exact expected saved world/appearance marker from the first process"),
            [Output, Expected, WritesOkay]() { *WritesOkay &= FFileHelper::LoadFileToString(*Expected, *FPaths::Combine(Output, TEXT("expected-world.txt"))); },
            [Expected]() { return !Expected->IsEmpty(); });
        Action(EKeys::F9, true);
        Action(EKeys::F9, true);
    }
    else
    {
        Action(EKeys::F9, true, true);
        Add(TEXT("Close initial Notes for disclosed functional resource setup"), [this]() { Tap(EKeys::I); },
            [this]() { return !Controller->IsBookOpen(); });
        QueueGatherTo(Homestead::Item::Branch, 5);
        Add(TEXT("Open pack"), [this]() { Tap(EKeys::I); }, [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
        Add(TEXT("Open Look through existing controller pages"), [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
            [this]() { return Controller->BookPage() == 6; });
        QueueSelectRow(1);
        Add(TEXT("Choose a real nondefault appearance value straight from its row"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this]() { return Controller->Appearance.HairColor == 1; });
        Action(EKeys::F5, false);
        Action(EKeys::F9, true);
        Action(EKeys::F5, false);
        Action(EKeys::F9, true);
        for (int32 Page = 1; Page <= 4; ++Page)
            Add(TEXT("Controller opens Settings for ordinary menu save/load"),
                [this]() { Tap(EKeys::Gamepad_RightShoulder); }, [this, Page]() { return Controller->BookPage() == Page; });
        Add(TEXT("Focus semantic Save progress in native Settings"),
            [this]()
            {
                if (!Controller->NativeMenu->FocusLegacySubject(0))
                    Finish(false, TEXT("Native Settings Save progress is unavailable."));
            },
            [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content"); });
        Add(TEXT("Enter native Save progress actions"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
        Action(EKeys::Gamepad_FaceButton_Bottom, false);
        Add(TEXT("Focus semantic Load latest in native Settings"),
            [this]()
            {
                if (!Controller->NativeMenu->FocusLegacySubject(1))
                    Finish(false, TEXT("Native Settings Load latest is unavailable."));
            },
            [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content"); });
        Add(TEXT("Enter native Load latest actions"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Actions"); });
        Action(EKeys::Gamepad_FaceButton_Bottom, true);
        const FString Temporary = Controller->SavePath(TEXT("Homestead_Manual")) + TEXT(".tmp");
        const auto SavedHash = MakeShared<FString>();
        Add(TEXT("Block only the sandbox temporary save file"),
            [this, Temporary, SavedHash]()
            {
                *SavedHash = LexToString(FMD5Hash::HashFile(*Controller->SavePath(TEXT("Homestead_Manual"))));
                FFileHelper::SaveStringToFile(TEXT("owned hotkey failure fixture"), *Temporary);
                FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Temporary, true);
            }, [Temporary]() { return IFileManager::Get().IsReadOnly(*Temporary); });
        Action(EKeys::F5, false, true);
        Add(TEXT("Failed save preserved the existing envelope and renderer; remove only owned temporary fixture"),
            [Temporary]() { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Temporary, false); },
            [this, Temporary, SavedHash]()
            {
                return LexToString(FMD5Hash::HashFile(*Controller->SavePath(TEXT("Homestead_Manual")))) == *SavedHash
                    && IFileManager::Get().Delete(*Temporary, true);
            });
        Add(TEXT("Explicitly choose supported ShaderComplexity through the console, not a save workaround"),
            [this]() { Controller->ConsoleCommand(TEXT("viewmode shadercomplexity"), true); },
            []() { return GEngine->GameViewport->ViewModeIndex == VMI_ShaderComplexity; });
        Action(EKeys::F5, false);
        Action(EKeys::F9, true);
        Add(TEXT("Persist exact world/appearance marker for independent preview-profile relaunch"),
            [Output, Expected, WritesOkay]() { *WritesOkay &= FFileHelper::SaveStringToFile(*Expected, *FPaths::Combine(Output, TEXT("expected-world.txt"))); },
            [this, Current, Expected]() { return Current() == *Expected && Controller->Sim.Count(Homestead::Item::Branch) >= 5; });
    }
    Add(TEXT("All diagnostic state records were written successfully"), []() {},
        [WritesOkay]() { return *WritesOkay; });
#endif
}
