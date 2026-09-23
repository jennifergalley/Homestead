#include "HomesteadSmokeTest.h"
#include "HomesteadCharacter.h"
#include "HomesteadController.h"
#include "HomesteadTestPaths.h"
#include "UI/SHomesteadMenu.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"

namespace
{
constexpr const TCHAR* CameraSection = TEXT("Homestead.Camera");
constexpr const TCHAR* AudioSection = TEXT("Homestead.Audio");
constexpr const TCHAR* AutosaveSection = TEXT("Homestead.Autosave");

}

void AHomesteadSmokeTest::PrepareCameraPreferenceChecks()
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    const FString Expected = FPaths::ConvertRelativePathToFull(
        FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("Graphics"), TEXT("GameUserSettings.ini")));
    if (!Branch || !FPaths::IsSamePath(
        FPaths::ConvertRelativePathToFull(Branch->IniPath), Expected))
    {
        Finish(false, TEXT("Camera preference fixture requires its explicit synthetic settings file."));
        return;
    }
    FString Phase;
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadCameraPreferencePhase="), Phase);
    if (Phase != TEXT("write") && Phase != TEXT("read") && Phase != TEXT("invalid"))
    {
        Finish(false, TEXT("Unknown camera preference fixture phase."));
        return;
    }
    Results.Add(TEXT("CAMERA_CONFIG=") + Expected);
    Results.Add(TEXT("DISCLOSURE mapped offscreen input and native menu proof; physical mouse feel remains ordinary-play acceptance."));
    const auto World = MakeShared<std::string>();
    const auto Stable = [this, World]()
    {
        return !World->empty() && Controller->Simulation().Serialize() == *World;
    };
    const auto CameraRows = [this](float Sensitivity, bool Inverted)
    {
        const auto Rows = Controller->Rows();
        return Rows.IsValidIndex(3) && Rows.IsValidIndex(4)
            && Rows[3].Label == FString::Printf(TEXT("Camera sensitivity: %.1f"), Sensitivity)
            && Rows[4].Label == FString::Printf(TEXT("Invert camera Y: %s"),
                Inverted ? TEXT("On") : TEXT("Off"));
    };
    const auto InjectLook = [this](bool Mouse, float Y)
    {
        auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
        auto* Subsystem = Controller->GetLocalPlayer()
            ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(
                Controller->GetLocalPlayer()) : nullptr;
        if (!Avatar || !Subsystem) return false;
        Subsystem->InjectInputForAction(Mouse ? Avatar->MouseLookAction : Avatar->StickLookAction,
            FInputActionValue(FVector2D(0, Y)), {}, {});
        return true;
    };
    const auto SlateClick = [this]()
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        const auto Widget = Slate.GetKeyboardFocusedWidget();
        if (!Widget) return;
        const FGeometry Geometry = Widget->GetCachedGeometry();
        const FVector2D Position = Geometry.GetAbsolutePosition()
            + Geometry.GetAbsoluteSize() * 0.5f;
        const FVector2D Previous = Slate.GetCursorPos();
        Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Previous, TSet<FKey>(),
            EKeys::Invalid, 0, FModifierKeysState()));
        TSet<FKey> Pressed;
        Pressed.Add(EKeys::LeftMouseButton);
        Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position,
            Pressed, EKeys::LeftMouseButton, 0, FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position,
            TSet<FKey>(), EKeys::LeftMouseButton, 0, FModifierKeysState()));
    };

    if (Phase == TEXT("invalid"))
    {
        Add(TEXT("Invalid camera properties independently fall back to safe defaults"),
            []() {}, [this]()
            {
                return FMath::IsNearlyEqual(Controller->Sensitivity, 1.0f)
                    && !Controller->bInvertY
                    && FMath::IsNearlyEqual(Controller->MusicVolume, 0.65f)
                    && FMath::IsNearlyEqual(Controller->AmbienceVolume, 0.70f)
                    && FMath::IsNearlyEqual(Controller->EffectsVolume, 0.80f)
                    && Controller->IsAutosaveEnabled() && Controller->AutosaveIntervalMinutes() == 5;
            });
        return;
    }
    if (Phase == TEXT("read"))
    {
        Add(TEXT("Second process loads sensitivity and inversion without loading a world save"),
            []() {}, [this]()
            {
                return FMath::IsNearlyEqual(Controller->Sensitivity, 1.2f)
                    && Controller->bInvertY
                    && FMath::IsNearlyEqual(Controller->MusicVolume, 0.70f)
                    && FMath::IsNearlyEqual(Controller->AmbienceVolume, 0.65f)
                    && FMath::IsNearlyEqual(Controller->EffectsVolume, 0.85f)
                    && !Controller->IsAutosaveEnabled() && Controller->AutosaveIntervalMinutes() == 10;
            });
        Add(TEXT("Open Settings in the second process"),
            [this, World]()
            {
                Tap(EKeys::Gamepad_FaceButton_Right);
                Tap(EKeys::Escape);
                *World = Controller->Simulation().Serialize();
            },
            [this]() { return Controller->BookPage() == 4; });
        Add(TEXT("Persisted inversion row remains a direct left-side control"),
            [this]() { Controller->NativeMenu->FocusLegacySubject(4); },
            [this, CameraRows, Stable]()
            {
                return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")
                    && Controller->NativeMenu->GetActionCount() == 0
                    && CameraRows(1.2f, true) && Stable();
            });
        return;
    }

    Add(TEXT("No stored camera section starts with sensitivity1 and inversion Off"),
        []() {}, [this]()
        {
            return FMath::IsNearlyEqual(Controller->Sensitivity, 1.0f)
                && !Controller->bInvertY;
        });
    Add(TEXT("Close the initial Notes page before mapped camera checks"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });

    const auto MousePitch = MakeShared<float>(0);
    Add(TEXT("With inversion Off, positive mapped mouse Y raises camera pitch"),
        [this, InjectLook]()
        {
            Controller->SetControlRotation(FRotator::ZeroRotator);
            if (!InjectLook(true, 1.0f))
                Finish(false, TEXT("Enhanced mouse-look action is unavailable."));
        },
        [this, MousePitch]()
        {
            *MousePitch = FMath::FindDeltaAngleDegrees(
                0.0f, Controller->GetControlRotation().Pitch);
            Results.Add(FString::Printf(TEXT("CAMERA_DELTA mouse_off=%.6f legacy_equivalent=%.6f"),
                *MousePitch, -*MousePitch));
            return *MousePitch > 0.01f;
        }, 0.15f);
    Steps.Last().Repeat = [InjectLook]() { InjectLook(true, 1.0f); };
    const auto StickPitch = MakeShared<float>(0);
    Add(TEXT("With inversion Off, positive mapped right-stick Y raises camera pitch"),
        [this, InjectLook]()
        {
            Controller->SetControlRotation(FRotator::ZeroRotator);
            if (!InjectLook(false, 1.0f))
                Finish(false, TEXT("Enhanced stick-look action is unavailable."));
        },
        [this, StickPitch]()
        {
            *StickPitch = FMath::FindDeltaAngleDegrees(
                0.0f, Controller->GetControlRotation().Pitch);
            Results.Add(FString::Printf(TEXT("CAMERA_DELTA stick_off=%.6f legacy_equivalent=%.6f"),
                *StickPitch, -*StickPitch));
            return *StickPitch > 0.01f;
        }, 0.15f);
    Steps.Last().Repeat = [InjectLook]() { InjectLook(false, 1.0f); };

    Add(TEXT("Open Settings directly through mapped Escape"),
        [this, World]()
        {
            Tap(EKeys::Escape);
            *World = Controller->Simulation().Serialize();
        },
        [this]() { return Controller->BookPage() == 4 && Controller->HasNativeMenu(); });
    Add(TEXT("Sensitivity row is a direct left-side control with no action button"),
        [this]() { Controller->NativeMenu->FocusLegacySubject(3); },
        [this]()
        {
            return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")
                && Controller->NativeMenu->GetActionCount() == 0;
        });
    Add(TEXT("Gamepad A advances and immediately persists sensitivity exactly once"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, CameraRows, Stable]()
        {
            FConfigFile Disk;
            float Value = 0;
            return FMath::IsNearlyEqual(Controller->Sensitivity, 1.2f)
                && CameraRows(1.2f, false)
                && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")
                && Disk.Combine(GConfig->FindBranch(TEXT("GameUserSettings"), {})->IniPath)
                && Disk.GetFloat(CameraSection, TEXT("Sensitivity"), Value)
                && FMath::IsNearlyEqual(Value, 1.2f) && Stable();
        });
    Add(TEXT("Inversion row is direct and retains details without Change setting"),
        [this]() { Controller->NativeMenu->FocusLegacySubject(4); },
        [this]()
        {
            return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")
                && Controller->NativeMenu->GetActionCount() == 0
                && Controller->NativeMenu->GetDisplayedDetails().Contains(TEXT("vertical look"));
        });
    Add(TEXT("Pointer click on the left inversion row toggles and persists exactly once"),
        [SlateClick]() { SlateClick(); },
        [this, CameraRows, Stable]()
        {
            FConfigFile Disk;
            bool Value = false;
            return Controller->bInvertY && CameraRows(1.2f, true)
                && !Controller->UsesGamepad()
                && Disk.Combine(GConfig->FindBranch(TEXT("GameUserSettings"), {})->IniPath)
                && Disk.GetBool(CameraSection, TEXT("InvertY"), Value) && Value && Stable();
        });
    Add(TEXT("Keyboard Enter toggles the same left row without moving to Actions"),
        [this]() { Tap(EKeys::Enter); },
        [this, CameraRows, Stable]()
        {
            return !Controller->bInvertY && CameraRows(1.2f, false)
                && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")
                && Controller->NativeMenu->GetActionCount() == 0 && Stable();
        });
    Add(TEXT("Gamepad A enables inversion for mapped direction and legacy-save checks"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, CameraRows]() { return Controller->bInvertY && CameraRows(1.2f, true); });
    Add(TEXT("Close Settings before inverted camera checks"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });

    Add(TEXT("With inversion On, positive mapped mouse Y lowers camera pitch"),
        [this, InjectLook]()
        {
            Controller->SetControlRotation(FRotator::ZeroRotator);
            if (!InjectLook(true, 1.0f))
                Finish(false, TEXT("Enhanced mouse-look action is unavailable."));
        },
        [this, MousePitch]()
        {
            *MousePitch = FMath::FindDeltaAngleDegrees(
                0.0f, Controller->GetControlRotation().Pitch);
            Results.Add(FString::Printf(TEXT("CAMERA_DELTA mouse_on=%.6f invert=%d"),
                *MousePitch, Controller->bInvertY));
            return *MousePitch < -0.01f;
        }, 0.15f);
    Steps.Last().Repeat = [InjectLook]() { InjectLook(true, 1.0f); };
    Add(TEXT("With inversion On, positive mapped right-stick Y lowers camera pitch"),
        [this, InjectLook]()
        {
            Controller->SetControlRotation(FRotator::ZeroRotator);
            if (!InjectLook(false, 1.0f))
                Finish(false, TEXT("Enhanced stick-look action is unavailable."));
        },
        [this, StickPitch]()
        {
            *StickPitch = FMath::FindDeltaAngleDegrees(
                0.0f, Controller->GetControlRotation().Pitch);
            Results.Add(FString::Printf(TEXT("CAMERA_DELTA stick_on=%.6f invert=%d"),
                *StickPitch, Controller->bInvertY));
            return *StickPitch < -0.01f;
        }, 0.15f);
    Steps.Last().Repeat = [InjectLook]() { InjectLook(false, 1.0f); };

    Add(TEXT("Write isolated world save containing legacy inversion On"),
        [this, World]()
        {
            *World = Controller->Simulation().Serialize();
            Tap(EKeys::F5);
        },
        [this]() { return !Controller->ToastIsError(); });
    Add(TEXT("Open Settings after writing the legacy world save"),
        [this]() { Tap(EKeys::Escape); },
        [this]()
        {
            return Controller->BookPage() == 4 && Controller->NativeMenu.IsValid();
        });
    Add(TEXT("Change user-level inversion Off after the world save"),
        [this]()
        {
            if (!Controller->NativeMenu->FocusLegacySubject(4))
            {
                Finish(false, TEXT("Inversion row is unavailable after world save."));
                return;
            }
            Tap(EKeys::Enter);
        },
        [this]() { return !Controller->bInvertY; });
    Add(TEXT("Close Settings before loading conflicting legacy world fields"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Loading a world save cannot override user-level camera preferences"),
        [this]() { Tap(EKeys::F9); },
        [this, CameraRows]()
        {
            return !Controller->bInvertY && FMath::IsNearlyEqual(Controller->Sensitivity, 1.2f)
                && CameraRows(1.2f, false);
        });

    Add(TEXT("Open Settings after loading the conflicting world save"),
        [this, World]()
        {
            Tap(EKeys::Escape);
            *World = Controller->Simulation().Serialize();
        },
        [this]()
        {
            return Controller->BookPage() == 4 && Controller->NativeMenu.IsValid();
        });
    Add(TEXT("Focus direct sensitivity row for read-only rollback"),
        [this]()
        {
            if (!Controller->NativeMenu->FocusLegacySubject(3))
                Finish(false, TEXT("Sensitivity row is unavailable after world load."));
        },
        [this]() { return Controller->NativeMenu->GetActionCount() == 0; });
    Add(TEXT("Read-only settings file rejects sensitivity and restores runtime value"),
        [this, Expected]()
        {
            FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Expected, true);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Expected, Stable]()
        {
            const bool Passed = Controller->ToastIsError()
                && Controller->Toast().Contains(TEXT("Could not save camera sensitivity"))
                && FMath::IsNearlyEqual(Controller->Sensitivity, 1.2f) && Stable();
            FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Expected, false);
            return Passed;
        });
    Add(TEXT("Set final inversion On for the separate-process consumer"),
        [this]()
        {
            Controller->NativeMenu->FocusLegacySubject(4);
            Tap(EKeys::Enter);
        },
        [this, CameraRows, Stable]()
        {
            return Controller->bInvertY && CameraRows(1.2f, true) && Stable();
        });
    Add(TEXT("Keyboard adjustment persists Music volume in user settings"),
        [this]() { Controller->NativeMenu->FocusLegacySubject(5); Tap(EKeys::Right); },
        [this, Stable]()
        {
            FConfigFile Disk; float Value = 0;
            return FMath::IsNearlyEqual(Controller->MusicVolume, 0.70f) && Stable()
                && Disk.Combine(GConfig->FindBranch(TEXT("GameUserSettings"), {})->IniPath)
                && Disk.GetFloat(AudioSection, TEXT("Music"), Value) && FMath::IsNearlyEqual(Value, 0.70f);
        });
    Add(TEXT("Keyboard adjustment persists Ambience and Effects independently"),
        [this]()
        {
            Controller->NativeMenu->FocusLegacySubject(6); Tap(EKeys::Left);
            Controller->NativeMenu->FocusLegacySubject(7); Tap(EKeys::Right);
        },
        [this, Stable]()
        {
            FConfigFile Disk; float Ambience = 0, Effects = 0;
            return FMath::IsNearlyEqual(Controller->AmbienceVolume, 0.65f)
                && FMath::IsNearlyEqual(Controller->EffectsVolume, 0.85f) && Stable()
                && Disk.Combine(GConfig->FindBranch(TEXT("GameUserSettings"), {})->IniPath)
                && Disk.GetFloat(AudioSection, TEXT("Ambience"), Ambience)
                && Disk.GetFloat(AudioSection, TEXT("Effects"), Effects)
                && FMath::IsNearlyEqual(Ambience, 0.65f) && FMath::IsNearlyEqual(Effects, 0.85f);
        });
    Add(TEXT("Autosave controls persist Off and ten-minute interval independently"),
        [this]()
        {
            Controller->NativeMenu->FocusLegacySubject(12); Tap(EKeys::Left);
            Controller->NativeMenu->FocusLegacySubject(13); Tap(EKeys::Right);
        },
        [this, Stable]()
        {
            FConfigFile Disk; bool Enabled = true; int32 Minutes = 0;
            return !Controller->IsAutosaveEnabled() && Controller->AutosaveIntervalMinutes() == 10 && Stable()
                && Disk.Combine(GConfig->FindBranch(TEXT("GameUserSettings"), {})->IniPath)
                && Disk.GetBool(AutosaveSection, TEXT("Enabled"), Enabled) && !Enabled
                && Disk.GetInt(AutosaveSection, TEXT("IntervalMinutes"), Minutes) && Minutes == 10;
        });
    Add(TEXT("New woodland preserves user-level camera preferences"),
        [this]() { Controller->NewGame(); },
        [this]()
        {
            return Controller->bInvertY && FMath::IsNearlyEqual(Controller->Sensitivity, 1.2f)
                && FMath::IsNearlyEqual(Controller->MusicVolume, 0.70f)
                && FMath::IsNearlyEqual(Controller->AmbienceVolume, 0.65f)
                && FMath::IsNearlyEqual(Controller->EffectsVolume, 0.85f)
                && !Controller->IsAutosaveEnabled() && Controller->AutosaveIntervalMinutes() == 10;
        });
}
