#include "HomesteadController.h"
#include "HomesteadControllerConfig.h"
#include "HomesteadControllerPreferences.h"
#include "HomesteadControllerText.h"
#include "HomesteadCharacter.h"
#include "HomesteadMapComponent.h"
#include "UI/HomesteadUITheme.h"
#include "UI/SHomesteadMenu.h"
#include "TimerManager.h"

#include "AudioDevice.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

using HomesteadControllerConfig::AudioKeys;
using HomesteadControllerConfig::AudioKeyIndex;
using HomesteadControllerConfig::AutosaveEnabledKey;
using HomesteadControllerConfig::AutosaveMinutesKey;
using HomesteadControllerConfig::AutosaveSettingsSection;
using HomesteadControllerConfig::CameraInvertYKey;
using HomesteadControllerConfig::CameraSensitivityKey;
using HomesteadControllerConfig::CameraSettingsSection;
using HomesteadControllerConfig::CaptureCameraConfig;
using HomesteadControllerConfig::FCameraConfigSnapshot;
using HomesteadControllerConfig::PersistBoolProperty;
using HomesteadControllerConfig::PersistFloatProperty;
using HomesteadControllerConfig::PersistIntProperty;
using HomesteadControllerConfig::RestoreCameraConfig;
using HomesteadControllerPreferences::AudioSettingsSection;
using HomesteadControllerText::Text;

void AHomesteadController::LoadCameraPreferences()
{
    Sensitivity = 1.0f;
    bInvertY = false;
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (!Branch || !Disk.Combine(Branch->IniPath))
    {
        UE_LOG(LogTemp, Display, TEXT("CAMERA_SETTINGS defaults active; no readable user-settings file."));
        return;
    }
    float StoredSensitivity = Sensitivity;
    if (Disk.GetFloat(CameraSettingsSection, CameraSensitivityKey, StoredSensitivity)
        && FMath::IsFinite(StoredSensitivity) && StoredSensitivity >= 0.2f && StoredSensitivity <= 3.0f)
    {
        Sensitivity = StoredSensitivity;
    }
    bool StoredInvertY = false;
    if (Disk.GetBool(CameraSettingsSection, CameraInvertYKey, StoredInvertY))
    {
        bInvertY = StoredInvertY;
    }
    UE_LOG(LogTemp, Display, TEXT("CAMERA_SETTINGS loaded sensitivity=%.3f invert_y=%d file=%s"),
        Sensitivity, bInvertY, *Branch->IniPath);
}

void AHomesteadController::LoadUserPreferences()
{
    MusicVolume = 0.65f;
    AmbienceVolume = 0.70f;
    EffectsVolume = 0.80f;
    MasterVolume = 1.0f;
    ApplyMasterVolume();
    bAutosaveEnabled = true;
    AutosaveMinutes = 5;
    LoadActionHints();
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    FConfigFile Disk;
    if (!Branch || !Disk.Combine(Branch->IniPath))
    {
        AutosaveRemaining = AutosaveMinutes * 60.0f;
        return;
    }
    for (const int32 AudioId : {5, 6, 7, 16})
    {
        float Stored = MenuAudioVolume(AudioId);
        if (Disk.GetFloat(AudioSettingsSection, AudioKeys[AudioKeyIndex(AudioId)], Stored) && FMath::IsFinite(Stored)
            && Stored >= 0 && Stored <= 1)
            MenuPreviewAudioVolume(AudioId, Stored);
    }
    FString EnabledText;
    if (Disk.GetString(AutosaveSettingsSection, AutosaveEnabledKey, EnabledText))
    {
        if (EnabledText.Equals(TEXT("True"), ESearchCase::IgnoreCase)) bAutosaveEnabled = true;
        else if (EnabledText.Equals(TEXT("False"), ESearchCase::IgnoreCase)) bAutosaveEnabled = false;
    }
    int32 Minutes = 5;
    if (Disk.GetInt(AutosaveSettingsSection, AutosaveMinutesKey, Minutes)
        && (Minutes == 5 || Minutes == 10 || Minutes == 20 || Minutes == 30))
        AutosaveMinutes = Minutes;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
}

float AHomesteadController::MenuAudioVolume(int32 Id) const
{
    return Id == 16 ? MasterVolume : Id == 5 ? MusicVolume : Id == 6 ? AmbienceVolume : EffectsVolume;
}

void AHomesteadController::ApplyMasterVolume() const
{
    if (const UWorld* World = GetWorld())
        if (FAudioDeviceHandle Device = World->GetAudioDevice())
            Device->SetTransientPrimaryVolume(MasterVolume);
}

void AHomesteadController::MenuPreviewAudioVolume(int32 Id, float Value)
{
    Value = FMath::Clamp(Value, 0.0f, 1.0f);
    if (Id == 16)
    {
        MasterVolume = Value;
        ApplyMasterVolume();
    }
    else if (Id == 5)
    {
        MusicVolume = Value;
        Music->SetVolumeMultiplier(MusicLevel());
    }
    else if (Id == 6)
    {
        AmbienceVolume = Value;
        Ambience->SetVolumeMultiplier(Value);
        Creek->SetVolumeMultiplier(Value * CreekGain);
    }
    else EffectsVolume = Value;
}

bool AHomesteadController::PersistAudioVolume(int32 Id, float Requested, float Previous)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    const int32 Index = AudioKeyIndex(Id);
    Requested = FMath::Clamp(Requested, 0.0f, 1.0f);
    if (!Branch || Index < 0
        || !PersistFloatProperty(Branch->IniPath, AudioSettingsSection, AudioKeys[Index], Requested))
    {
        MenuPreviewAudioVolume(Id, Previous);
        Notify(TEXT("Could not save that audio preference. The previous level was restored."), true);
        return false;
    }
    GConfig->SetFloat(AudioSettingsSection, AudioKeys[Index], Requested, GGameUserSettingsIni);
    MenuPreviewAudioVolume(Id, Requested);
    ++AudioPersistWrites;
    return true;
}

bool AHomesteadController::MenuCommitAudioVolume(int32 Id, float Value, float Previous)
{
    return PersistAudioVolume(Id, Value, Previous);
}

bool AHomesteadController::PersistAutosaveEnabled(bool Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch || !PersistBoolProperty(Branch->IniPath, AutosaveSettingsSection, AutosaveEnabledKey, Requested))
    {
        Notify(TEXT("Could not save the Autosave preference. The previous choice was restored."), true);
        return false;
    }
    GConfig->SetBool(AutosaveSettingsSection, AutosaveEnabledKey, Requested, GGameUserSettingsIni);
    return true;
}

bool AHomesteadController::PersistAutosaveInterval(int32 Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch || (Requested != 5 && Requested != 10 && Requested != 20 && Requested != 30)
        || !PersistIntProperty(Branch->IniPath, AutosaveSettingsSection, AutosaveMinutesKey, Requested))
    {
        Notify(TEXT("Could not save the Autosave interval. The previous interval was restored."), true);
        return false;
    }
    GConfig->SetInt(AutosaveSettingsSection, AutosaveMinutesKey, Requested, GGameUserSettingsIni);
    return true;
}

void AHomesteadController::MenuSetAutosaveEnabled(bool Enabled)
{
    if (!PersistAutosaveEnabled(Enabled)) return;
    bAutosaveEnabled = Enabled;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
    Notify(Enabled ? TEXT("Autosave On.") : TEXT("Autosave Off. Existing autosaves are retained."));
}

void AHomesteadController::MenuSetAutosaveInterval(int32 Minutes)
{
    if (!PersistAutosaveInterval(Minutes)) return;
    AutosaveMinutes = Minutes;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
    Notify(FString::Printf(TEXT("Autosave interval %d minutes."), AutosaveMinutes));
}

void AHomesteadController::MenuSetDarkBook(bool bDark)
{
    if (HomesteadUITheme::IsDark() == bDark) return;
    HomesteadUITheme::Set(bDark ? HomesteadUITheme::ETheme::Dark : HomesteadUITheme::ETheme::Parchment);
}

void AHomesteadController::HandleThemeChanged()
{
    // Next tick: the change may come from the book's own button click.
    if (bThemeRebuildPending || !GetWorld()) return;
    bThemeRebuildPending = true;
    GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
    {
        bThemeRebuildPending = false;
        if (NativeMenu.IsValid()) NativeMenu->RebuildForTheme();
        if (HotbarRoot.IsValid())
        {
            HideHotbar();
            ShowHotbar();
        }
    }));
}

void AHomesteadController::MenuSetGameSpeed(double DayMinutes)
{
    if (RejectPendingGroundSnapAction()) return;
    Notify(Sim.SetDayMinutes(DayMinutes));
}

void AHomesteadController::MenuAdjustSetting(int32 Id, int32 Direction)
{
    if (RejectPendingGroundSnapAction()) return;
    if (!Direction) return;
    if (Id == 2)
    {
        const double Values[] = {120, 60, 30};
        int32 Index = State().dayMinutes >= 119 ? 0 : State().dayMinutes <= 31 ? 2 : 1;
        MenuSetGameSpeed(Values[FMath::Clamp(Index + Direction, 0, 2)]);
    }
    else if (Id == 3) PersistCameraSensitivity(FMath::Clamp(Sensitivity + Direction * 0.2f, 0.2f, 3.0f));
    else if (Id == 4) PersistCameraInversion(Direction > 0);
    else if ((Id >= 5 && Id <= 7) || Id == 16)
    {
        const float Previous = MenuAudioVolume(Id);
        PersistAudioVolume(Id, Previous + Direction * 0.05f, Previous);
    }
    else if (Id == 12) MenuSetAutosaveEnabled(Direction > 0);
    else if (Id == 17 && Map) Map->SetRotatesWithCamera(Direction > 0);
    else if (Id == 18) MenuSetDarkBook(Direction > 0);
    else if (Id == 13 && bAutosaveEnabled)
    {
        const int32 Values[] = {5, 10, 20, 30};
        int32 Index = 0;
        for (int32 I = 0; I < UE_ARRAY_COUNT(Values); ++I) if (Values[I] == AutosaveMinutes) Index = I;
        MenuSetAutosaveInterval(Values[FMath::Clamp(Index + Direction, 0, UE_ARRAY_COUNT(Values) - 1)]);
    }
}

bool AHomesteadController::PersistCameraSensitivity(float Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch || !FMath::IsFinite(Requested) || Requested < 0.2f || Requested > 3.0f)
    {
        Notify(TEXT("Camera sensitivity settings are unavailable. Your previous preference is unchanged."), true);
        return false;
    }
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Branch->IniPath, Snapshot))
    {
        Notify(TEXT("Could not read camera settings before saving. Your previous preference is unchanged."), true);
        return false;
    }
    FConfigFile Property;
    Property.SetFloat(CameraSettingsSection, CameraSensitivityKey, Requested);
    const bool Saved = Property.UpdateSinglePropertyInSection(
        *Branch->IniPath, CameraSensitivityKey, CameraSettingsSection);
    FConfigFile Disk;
    float Persisted = -1.0f;
    const bool Verified = Saved && Disk.Combine(Branch->IniPath)
        && Disk.GetFloat(CameraSettingsSection, CameraSensitivityKey, Persisted)
        && FMath::IsNearlyEqual(Persisted, Requested, 0.001f);
    if (!Verified)
    {
        const bool Restored = !Saved || RestoreCameraConfig(Branch->IniPath, Snapshot);
        Notify(Restored
            ? TEXT("Could not save camera sensitivity. Your previous preference was restored.")
            : TEXT("Could not save or restore camera sensitivity. Check the settings file permissions."), true);
        UE_LOG(LogTemp, Error, TEXT("CAMERA_SETTINGS sensitivity persistence failed file=%s restored=%d"),
            *Branch->IniPath, Restored);
        return false;
    }
    GConfig->SetFloat(CameraSettingsSection, CameraSensitivityKey, Requested, GGameUserSettingsIni);
    Sensitivity = Requested;
    Notify(FString::Printf(TEXT("Camera sensitivity %.1f. Choice saved in game settings."), Sensitivity));
    return true;
}

bool AHomesteadController::PersistCameraInversion(bool Requested)
{
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Branch)
    {
        Notify(TEXT("Camera inversion settings are unavailable. Your previous preference is unchanged."), true);
        return false;
    }
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Branch->IniPath, Snapshot))
    {
        Notify(TEXT("Could not read camera settings before saving. Your previous preference is unchanged."), true);
        return false;
    }
    FConfigFile Property;
    Property.SetBool(CameraSettingsSection, CameraInvertYKey, Requested);
    const bool Saved = Property.UpdateSinglePropertyInSection(
        *Branch->IniPath, CameraInvertYKey, CameraSettingsSection);
    FConfigFile Disk;
    bool Persisted = false;
    const bool Verified = Saved && Disk.Combine(Branch->IniPath)
        && Disk.GetBool(CameraSettingsSection, CameraInvertYKey, Persisted)
        && Persisted == Requested;
    if (!Verified)
    {
        const bool Restored = !Saved || RestoreCameraConfig(Branch->IniPath, Snapshot);
        Notify(Restored
            ? TEXT("Could not save camera inversion. Your previous preference was restored.")
            : TEXT("Could not save or restore camera inversion. Check the settings file permissions."), true);
        UE_LOG(LogTemp, Error, TEXT("CAMERA_SETTINGS inversion persistence failed file=%s restored=%d"),
            *Branch->IniPath, Restored);
        return false;
    }
    GConfig->SetBool(CameraSettingsSection, CameraInvertYKey, Requested, GGameUserSettingsIni);
    bInvertY = Requested;
    Notify(Requested ? TEXT("Camera Y inversion On. Choice saved in game settings.")
        : TEXT("Camera Y inversion Off. Choice saved in game settings."));
    return true;
}

bool AHomesteadController::PersistResolutionScale(float Requested)
{
    auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    if (!Settings)
    {
        PendingResolutionScale = Requested;
        GraphicsSaveError = TEXT("Video settings are unavailable; the requested 3D resolution scale was not saved.");
        Notify(GraphicsSaveError, true);
        return false;
    }
    float Normalized = 0, Previous = 100, Minimum = 0, Maximum = 100;
    Settings->GetResolutionScaleInformationEx(Normalized, Previous, Minimum, Maximum);
    Settings->SetResolutionScaleValueEx(Requested);
    Settings->ApplyNonResolutionSettings();
    if (!FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest")))
    {
        Settings->SaveSettings();
        FConfigFile Disk;
        float Persisted = -1;
        if (!Disk.Combine(GGameUserSettingsIni)
            || !Disk.GetFloat(TEXT("ScalabilityGroups"), TEXT("sg.ResolutionQuality"), Persisted)
            || !FMath::IsNearlyEqual(Persisted, Requested, 0.1f))
        {
            Settings->SetResolutionScaleValueEx(Previous);
            Settings->ApplyNonResolutionSettings();
            PendingResolutionScale = Requested;
            GraphicsSaveError = TEXT("Could not verify the saved 3D resolution scale. The previous runtime scale was restored; the disk preference is unverified.");
            Notify(GraphicsSaveError, true);
            return false;
        }
    }
    PendingResolutionScale.Reset();
    GraphicsSaveError.Reset();
    return true;
}

void AHomesteadController::ToggleVerticalSync()
{
    auto* Settings = GEngine ? GEngine->GetGameUserSettings() : nullptr;
    auto* VSync = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"));
    const auto* Branch = GConfig ? GConfig->FindBranch(TEXT("GameUserSettings"), {}) : nullptr;
    if (!Settings || !VSync || !Branch)
    {
        Notify(TEXT("Vertical sync settings are unavailable in this session."), true);
        UE_LOG(LogTemp, Error, TEXT("VSYNC_SETTING unavailable"));
        return;
    }
    const bool Previous = Settings->IsVSyncEnabled();
    const int32 PreviousRuntime = VSync->GetInt();
    const bool Requested = !Previous;
    VSync->Set(Requested ? 1 : 0, ECVF_SetByGameSetting);
    if ((VSync->GetInt() != 0) != Requested)
    {
        Notify(TEXT("An engine override controls vertical sync. Your preference was not changed."), true);
        UE_LOG(LogTemp, Warning, TEXT("VSYNC_SETTING blocked requested=%d applied=%d priority=%u"),
            Requested, VSync->GetInt(), VSync->GetFlags() & ECVF_SetByMask);
        return;
    }
    Settings->SetVSyncEnabled(Requested);
    // Avoid reapplying other video settings or flushing unrelated pending config changes.
    const FString Section = Settings->GetClass()->GetPathName();
    FConfigFile Property;
    Property.SetBool(*Section, TEXT("bUseVSync"), Settings->IsVSyncEnabled());
    const bool Saved = Property.UpdateSinglePropertyInSection(*Branch->IniPath, TEXT("bUseVSync"), *Section);
    FConfigFile Disk;
    bool Persisted = false;
    const bool Read = Saved && Disk.Combine(Branch->IniPath)
        && Disk.GetBool(*Section, TEXT("bUseVSync"), Persisted);
    if (!Read || Persisted != Requested)
    {
        Settings->SetVSyncEnabled(Previous);
        VSync->Set(PreviousRuntime, ECVF_SetByGameSetting);
        Notify(TEXT("Could not save vertical sync. Your previous preference was restored."), true);
        UE_LOG(LogTemp, Error, TEXT("VSYNC_SETTING persistence failed file=%s requested=%d applied=%d"),
            *Branch->IniPath, Requested, VSync->GetInt());
        return;
    }
    GConfig->SetBool(*Section, TEXT("bUseVSync"), Requested, GGameUserSettingsIni);
    UE_LOG(LogTemp, Display, TEXT("VSYNC_SETTING saved requested=%d applied=%d priority=%u file=%s"),
        Requested, VSync->GetInt(), VSync->GetFlags() & ECVF_SetByMask, *Branch->IniPath);
    Notify(Requested ? TEXT("Vertical sync On. Choice saved for this game's graphics settings.")
        : TEXT("Vertical sync Off. Choice saved for this game's graphics settings."));
}
