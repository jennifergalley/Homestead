#include "HomesteadAuthoringProbeCommandlet.h"
#include "DerivedDataCacheInterface.h"
#include "DerivedDataCacheUsageStats.h"
#include "GenericPlatform/GenericPlatformCrashContext.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "Windows/WindowsHWrapper.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadAuthoringProbe, Log, All);

namespace
{
bool SaveEvidence(const FString& Path, const TSharedRef<FJsonObject>& Value)
{
    if (IFileManager::Get().FileExists(*Path)) return false;
    FString Json;
    return FJsonSerializer::Serialize(Value, TJsonWriterFactory<>::Create(&Json))
        && FFileHelper::SaveStringToFile(Json, *(Path + TEXT(".tmp")), FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM)
        && IFileManager::Get().Move(*Path, *(Path + TEXT(".tmp")), false, false);
}

bool ReadFalse(const TCHAR* Section, const TCHAR* Key, const FString& Ini,
    const TCHAR* OutputKey, const TSharedRef<FJsonObject>& Result)
{
    bool Value = true;
    const bool Found = GConfig->GetBool(Section, Key, Value, Ini);
    Result->SetBoolField(OutputKey, Value);
    Result->SetBoolField(FString(OutputKey) + TEXT("Found"), Found);
    return Found && !Value;
}

uint64 FileTimeValue(const FILETIME& Time)
{
    return (static_cast<uint64>(Time.dwHighDateTime) << 32) | Time.dwLowDateTime;
}
}

UHomesteadAuthoringProbeCommandlet::UHomesteadAuthoringProbeCommandlet()
{
    IsClient = false;
    IsServer = false;
    IsEditor = true;
    LogToConsole = true;
}

int32 UHomesteadAuthoringProbeCommandlet::Main(const FString& Params)
{
    FString Output;
    if (!FParse::Value(*Params, TEXT("EvidenceDirectory="), Output) || FPaths::IsRelative(Output)
        || Output != FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_PROBE_OUTPUT"))
        || !IFileManager::Get().DirectoryExists(*Output)
        || !FPaths::IsUnderDirectory(FPaths::ProjectSavedDir(), Output))
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Fresh isolated evidence/UserDir contract is missing."));
        return 2;
    }
    auto Result = MakeShared<FJsonObject>();
    bool Valid = true;
    const auto Require = [&Valid](bool Condition, const TCHAR* Message)
    {
        if (!Condition) { UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("%s"), Message); Valid = false; }
    };
    Result->SetNumberField(TEXT("pid"), GetCurrentProcessId());
    Result->SetStringField(TEXT("executable"), FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPlatformProcess::BaseDir(), FPlatformProcess::ExecutableName(false))));
    Result->SetStringField(TEXT("projectSavedDirectory"), FPaths::ProjectSavedDir());
    Result->SetStringField(TEXT("userDirectory"), FPlatformProcess::UserDir());
    Result->SetStringField(TEXT("engineIni"), GEngineIni);
    Result->SetStringField(TEXT("editorSettingsIni"), GEditorSettingsIni);
    Result->SetStringField(TEXT("editorPerProjectIni"), GEditorPerProjectIni);
    Result->SetStringField(TEXT("gameUserSettingsIni"), GGameUserSettingsIni);
    for (const FString& Ini : {GEngineIni, GEditorSettingsIni, GEditorPerProjectIni, GGameUserSettingsIni})
        Require(FPaths::IsUnderDirectory(Ini, Output), TEXT("An effective config path is outside the probe."));

    const FString HandleText = FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_GUARD_HANDLE"));
    TCHAR* End = nullptr;
    const uint64 HandleNumber = FCString::Strtoui64(*HandleText, &End, 10);
    Require(!HandleText.IsEmpty() && End && *End == 0 && HandleNumber != 0, TEXT("Invalid inherited marker handle."));
    const HANDLE Marker = reinterpret_cast<HANDLE>(static_cast<UPTRINT>(HandleNumber));
    BY_HANDLE_FILE_INFORMATION Info{};
    Require(GetFileInformationByHandle(Marker, &Info) != FALSE, TEXT("Inherited marker identity unavailable."));
    Require(Info.nFileSizeHigh == 0 && Info.nFileSizeLow == 0
        && (Info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) == 0,
        TEXT("Inherited marker is not an ordinary empty file."));
    char Byte = 0;
    DWORD Read = 0;
    Require(ReadFile(Marker, &Byte, 1, &Read, nullptr) && Read == 0, TEXT("Inherited marker is not readable/empty."));
    auto MarkerInfo = MakeShared<FJsonObject>();
    MarkerInfo->SetNumberField(TEXT("volume"), Info.dwVolumeSerialNumber);
    MarkerInfo->SetNumberField(TEXT("indexHigh"), Info.nFileIndexHigh);
    MarkerInfo->SetNumberField(TEXT("indexLow"), Info.nFileIndexLow);
    MarkerInfo->SetNumberField(TEXT("attributes"), Info.dwFileAttributes);
    MarkerInfo->SetStringField(TEXT("creationTime"), LexToString(FileTimeValue(Info.ftCreationTime)));
    MarkerInfo->SetStringField(TEXT("lastWriteTime"), LexToString(FileTimeValue(Info.ftLastWriteTime)));
    Result->SetObjectField(TEXT("marker"), MarkerInfo);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION Limits{};
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION Accounting{};
    BOOL InJob = FALSE;
    Require(IsProcessInJob(GetCurrentProcess(), nullptr, &InJob) && InJob, TEXT("Probe is not in a job."));
    Require(QueryInformationJobObject(nullptr, JobObjectExtendedLimitInformation, &Limits, sizeof(Limits), nullptr)
        && QueryInformationJobObject(nullptr, JobObjectBasicAccountingInformation, &Accounting, sizeof(Accounting), nullptr),
        TEXT("Actual job policy/accounting unavailable."));
    Result->SetNumberField(TEXT("jobFlags"), Limits.BasicLimitInformation.LimitFlags);
    Result->SetNumberField(TEXT("jobProcessLimit"), Limits.BasicLimitInformation.ActiveProcessLimit);
    Result->SetNumberField(TEXT("jobActiveProcesses"), Accounting.ActiveProcesses);
    Require(Limits.BasicLimitInformation.LimitFlags == (JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE)
        && Limits.BasicLimitInformation.ActiveProcessLimit == 1 && Accounting.ActiveProcesses == 1,
        TEXT("Actual leaf job policy differs."));
    FILETIME Created{}, Exited{}, Kernel{}, User{};
    Require(GetProcessTimes(GetCurrentProcess(), &Created, &Exited, &Kernel, &User) != FALSE, TEXT("Process creation identity unavailable."));
    Result->SetStringField(TEXT("processCreationTime"), LexToString(FileTimeValue(Created)));

    auto Config = MakeShared<FJsonObject>();
    Valid &= ReadFalse(TEXT("/Script/UdpMessaging.UdpMessagingSettings"), TEXT("EnabledByDefault"), GEngineIni, TEXT("udpDefault"), Config);
    Valid &= ReadFalse(TEXT("/Script/UdpMessaging.UdpMessagingSettings"), TEXT("EnableTransport"), GEngineIni, TEXT("udpTransport"), Config);
    Valid &= ReadFalse(TEXT("/Script/UdpMessaging.UdpMessagingSettings"), TEXT("EnableTunnel"), GEngineIni, TEXT("udpTunnel"), Config);
    Valid &= ReadFalse(TEXT("/Script/TcpMessaging.TcpMessagingSettings"), TEXT("EnableTransport"), GEngineIni, TEXT("tcpTransport"), Config);
    Valid &= ReadFalse(TEXT("/Script/PythonScriptPlugin.PythonScriptPluginSettings"), TEXT("bRemoteExecution"), GEngineIni, TEXT("pythonRemote"), Config);
    Valid &= ReadFalse(TEXT("/Script/PythonScriptPlugin.PythonScriptPluginSettings"), TEXT("bRunPipInstallOnStartup"), GEngineIni, TEXT("pythonPip"), Config);
    Valid &= ReadFalse(TEXT("DevOptions.Shaders"), TEXT("bAllowCompilingThroughWorkers"), GEngineIni, TEXT("shaderWorkers"), Config);
    Valid &= ReadFalse(TEXT("/Script/UnrealEd.CrashReportsPrivacySettings"), TEXT("bSendUnattendedBugReports"), GEditorSettingsIni, TEXT("sendReportsConfig"), Config);
    Valid &= ReadFalse(TEXT("/Script/UnrealEd.AnalyticsPrivacySettings"), TEXT("bSendUsageData"), GEditorSettingsIni, TEXT("sendUsageConfig"), Config);
    Result->SetObjectField(TEXT("effectiveConfig"), Config);
    const auto Crash = MakeUnique<FSharedCrashContext>();
    FGenericCrashContext::CopySharedCrashContext(*Crash);
    Result->SetBoolField(TEXT("cachedSendReports"), Crash->UserSettings.bSendUnattendedBugReports);
    Result->SetBoolField(TEXT("cachedSendUsage"), Crash->UserSettings.bSendUsageData);
    Require(!Crash->UserSettings.bSendUnattendedBugReports && !Crash->UserSettings.bSendUsageData,
        TEXT("Actual cached crash privacy is not disabled."));

    const bool PythonLoaded = FModuleManager::Get().IsModuleLoaded(TEXT("PythonScriptPlugin"));
    Result->SetBoolField(TEXT("pythonModuleLoaded"), PythonLoaded);
    Require(!PythonLoaded && FParse::Param(FCommandLine::Get(), TEXT("DisablePython")), TEXT("Python was not fully disabled."));
    TArray<TSharedPtr<FJsonValue>> Plugins;
    for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPlugins())
    {
        Plugins.Add(MakeShared<FJsonValueString>(Plugin->GetName()));
        Require(Plugin->GetName() != TEXT("PythonScriptPlugin") && Plugin->GetName() != TEXT("UdpMessaging")
            && Plugin->GetName() != TEXT("TcpMessaging"), TEXT("An excluded plugin is enabled."));
    }
    Result->SetArrayField(TEXT("enabledPlugins"), Plugins);
    Require(FParse::Param(FCommandLine::Get(), TEXT("noshaderworker"))
        && !FParse::Param(FCommandLine::Get(), TEXT("NoShaderCompile")), TEXT("Shader command policy differs."));
    const IConsoleVariable* Workers = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Shaders.AllowCompilingThroughWorkers"));
    Require(Workers && Workers->GetInt() == 0, TEXT("Shader-worker CVar is not disabled."));
    Result->SetBoolField(TEXT("shaderManagerPresent"), GShaderCompilingManager != nullptr);
    if (GShaderCompilingManager)
    {
        Result->SetBoolField(TEXT("shaderCompilationSkipped"), GShaderCompilingManager->IsShaderCompilationSkipped());
        Result->SetNumberField(TEXT("configuredShaderSlotsNotChildCount"), GShaderCompilingManager->GetNumLocalWorkers());
    }
    Result->SetStringField(TEXT("shaderEvidenceLimit"), TEXT("Settings-only NullRHI probe, not proof of a shader compile workload."));
    Result->SetStringField(TEXT("ddcGraph"), GetDerivedDataCacheRef().GetGraphName());
    TArray<TSharedPtr<FJsonValue>> Stores;
    // UE5.8 still uses this public legacy adapter for its own cache diagnostics.
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    GetDerivedDataCacheRef().GatherUsageStats()->ForEachDescendant([&Stores](TSharedRef<const FDerivedDataCacheStatsNode> Node)
    {
        auto Store = MakeShared<FJsonObject>();
        Store->SetStringField(TEXT("type"), Node->GetCacheType());
        Store->SetStringField(TEXT("name"), Node->GetCacheName());
        Store->SetBoolField(TEXT("local"), Node->IsLocal());
        auto Attributes = MakeShared<FJsonObject>();
        COOK_STAT(for (const auto& Attribute : Node->CustomStats) Attributes->SetStringField(Attribute.Key, Attribute.Value));
        Store->SetObjectField(TEXT("attributes"), Attributes);
        Stores.Add(MakeShared<FJsonValueObject>(Store));
    });
    PRAGMA_ENABLE_DEPRECATION_WARNINGS
    Result->SetArrayField(TEXT("ddcStores"), Stores);
    Result->SetBoolField(TEXT("valid"), Valid);
    if (!SaveEvidence(FPaths::Combine(Output, TEXT("effective-settings.json")), Result))
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Cannot persist fresh effective-settings evidence."));
        return 3;
    }
    if (!Valid)
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Effective settings or guard admission failed."));
        return 4;
    }
    const double Deadline = FPlatformTime::Seconds() + 90;
    FDateTime RunDeadline;
    if (!FDateTime::ParseIso8601(*FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_PROBE_DEADLINE")), RunDeadline))
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Missing native deadline."));
        return 5;
    }
    FString Stop;
    while (FPlatformTime::Seconds() < Deadline && FDateTime::UtcNow() < RunDeadline)
    {
        if (IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("stop-probe.txt"))))
        {
            if (!FFileHelper::LoadFileToString(Stop, *FPaths::Combine(Output, TEXT("stop-probe.txt")))) return 6;
            break;
        }
        FPlatformProcess::SleepNoStats(0.02f);
    }
    auto Exit = MakeShared<FJsonObject>();
    Exit->SetStringField(TEXT("stopReason"), Stop);
    Exit->SetBoolField(TEXT("cooperative"), true);
    Exit->SetBoolField(TEXT("passed"), Stop.TrimStartAndEnd() == TEXT("complete"));
    Require(SaveEvidence(FPaths::Combine(Output, TEXT("native-exit.json")), Exit), TEXT("Cannot persist stop evidence."));
    return Valid && Stop.TrimStartAndEnd() == TEXT("complete") ? 0 : 7;
}
