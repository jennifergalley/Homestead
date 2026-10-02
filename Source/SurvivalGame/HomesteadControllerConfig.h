#pragma once

#include "CoreMinimal.h"
#include "HAL/FileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"

namespace HomesteadControllerConfig
{
inline constexpr const TCHAR* CameraSettingsSection = TEXT("Homestead.Camera");
inline constexpr const TCHAR* CameraSensitivityKey = TEXT("Sensitivity");
inline constexpr const TCHAR* CameraInvertYKey = TEXT("InvertY");
inline constexpr const TCHAR* AudioKeys[] = {TEXT("Music"), TEXT("Ambience"), TEXT("Effects"), TEXT("Master")};
inline int32 AudioKeyIndex(int32 Id) { return Id == 16 ? 3 : Id >= 5 && Id <= 7 ? Id - 5 : -1; }
inline constexpr const TCHAR* AutosaveSettingsSection = TEXT("Homestead.Autosave");
inline constexpr const TCHAR* AutosaveEnabledKey = TEXT("Enabled");
inline constexpr const TCHAR* AutosaveMinutesKey = TEXT("IntervalMinutes");
inline constexpr const TCHAR* ActionHintSection = TEXT("Homestead.ActionHints");

struct FCameraConfigSnapshot
{
    bool Existed = false;
    TArray<uint8> Bytes;
};

inline bool CaptureCameraConfig(const FString& Path, FCameraConfigSnapshot& Snapshot)
{
    Snapshot.Existed = IFileManager::Get().FileExists(*Path);
    return !Snapshot.Existed || FFileHelper::LoadFileToArray(Snapshot.Bytes, *Path);
}

inline bool RestoreCameraConfig(const FString& Path, const FCameraConfigSnapshot& Snapshot)
{
    return Snapshot.Existed
        ? FFileHelper::SaveArrayToFile(Snapshot.Bytes, *Path)
        : !IFileManager::Get().FileExists(*Path) || IFileManager::Get().Delete(*Path, false, true, true);
}

inline bool PersistFloatProperty(const FString& Path, const TCHAR* Section, const TCHAR* Key, float Value)
{
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Path, Snapshot)) return false;
    FConfigFile Property;
    Property.SetFloat(Section, Key, Value);
    const bool Saved = Property.UpdateSinglePropertyInSection(*Path, Key, Section);
    FConfigFile Disk;
    float Persisted = -1;
    const bool Verified = Saved && Disk.Combine(Path) && Disk.GetFloat(Section, Key, Persisted)
        && FMath::IsNearlyEqual(Persisted, Value, 0.001f);
    if (!Verified && Saved) RestoreCameraConfig(Path, Snapshot);
    return Verified;
}

inline bool PersistBoolProperty(const FString& Path, const TCHAR* Section, const TCHAR* Key, bool Value)
{
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Path, Snapshot)) return false;
    FConfigFile Property;
    Property.SetBool(Section, Key, Value);
    const bool Saved = Property.UpdateSinglePropertyInSection(*Path, Key, Section);
    FConfigFile Disk;
    bool Persisted = !Value;
    const bool Verified = Saved && Disk.Combine(Path) && Disk.GetBool(Section, Key, Persisted) && Persisted == Value;
    if (!Verified && Saved) RestoreCameraConfig(Path, Snapshot);
    return Verified;
}

inline bool PersistIntProperty(const FString& Path, const TCHAR* Section, const TCHAR* Key, int32 Value)
{
    FCameraConfigSnapshot Snapshot;
    if (!CaptureCameraConfig(Path, Snapshot)) return false;
    FConfigFile Property;
    Property.SetString(Section, Key, *FString::FromInt(Value));
    const bool Saved = Property.UpdateSinglePropertyInSection(*Path, Key, Section);
    FConfigFile Disk;
    int32 Persisted = -1;
    const bool Verified = Saved && Disk.Combine(Path) && Disk.GetInt(Section, Key, Persisted) && Persisted == Value;
    if (!Verified && Saved) RestoreCameraConfig(Path, Snapshot);
    return Verified;
}
}
