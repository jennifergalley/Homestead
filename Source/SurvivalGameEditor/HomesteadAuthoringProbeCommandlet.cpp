#include "HomesteadAuthoringProbeCommandlet.h"
#include "FernSpike.h"
#include "DerivedDataCacheInterface.h"
#include "DerivedDataCacheUsageStats.h"
#include "GenericPlatform/GenericPlatformCrashContext.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "IPythonScriptPlugin.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/FeedbackContext.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Serialization/JsonSerializer.h"
#include "ShaderCompiler.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "Windows/WindowsHWrapper.h"
#include <Psapi.h>

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

bool CapturePython(const TSharedRef<FJsonObject>& State)
{
    const IPythonScriptPlugin* Plugin = IPythonScriptPlugin::Get();
    State->SetBoolField(TEXT("moduleLoaded"), Plugin != nullptr);
    State->SetBoolField(TEXT("configured"), Plugin && Plugin->IsPythonConfigured());
    State->SetBoolField(TEXT("available"), Plugin && Plugin->IsPythonAvailable());
    State->SetBoolField(TEXT("initialized"), Plugin && Plugin->IsPythonInitialized());
    bool Valid = FParse::Param(FCommandLine::Get(), TEXT("DisablePython"))
        && (!Plugin || (Plugin->IsPythonConfigured() && !Plugin->IsPythonAvailable() && !Plugin->IsPythonInitialized()));
    HMODULE Modules[2048]{};
    DWORD Needed = 0;
    const bool Enumerated = K32EnumProcessModules(GetCurrentProcess(), Modules, sizeof(Modules), &Needed)
        && Needed <= sizeof(Modules) && Needed % sizeof(HMODULE) == 0;
    State->SetBoolField(TEXT("librariesEnumerated"), Enumerated);
    Valid &= Enumerated;
    bool RuntimeFound = false;
    TArray<TSharedPtr<FJsonValue>> Libraries;
    if (Enumerated)
    {
        for (DWORD Index = 0; Index < Needed / sizeof(HMODULE); ++Index)
        {
            WCHAR Buffer[32768]{};
            const DWORD Length = GetModuleFileNameW(Modules[Index], Buffer, UE_ARRAY_COUNT(Buffer));
            if (Length == 0 || Length >= UE_ARRAY_COUNT(Buffer)) { Valid = false; continue; }
            const FString Path(Buffer);
            const FString Name = FPaths::GetCleanFilename(Path).ToLower();
            if (!Name.StartsWith(TEXT("python")) || !Name.EndsWith(TEXT(".dll"))) continue;
            auto Library = MakeShared<FJsonObject>();
            Library->SetStringField(TEXT("path"), Path);
            const FString Expected = FPaths::ConvertRelativePathToFull(FPaths::Combine(
                FPaths::EngineDir(), TEXT("Binaries/ThirdParty/Python3/Win64"), Name));
            const bool AdmittedLibrary = (Name == TEXT("python3.dll") || Name == TEXT("python311.dll"))
                && FPaths::IsSamePath(Path, Expected);
            Valid &= AdmittedLibrary;
            bool QueryAvailable = false;
            int Initialized = -1;
            if (AdmittedLibrary && Name == TEXT("python311.dll"))
            {
                RuntimeFound = true;
                using FIsInitialized = int (__cdecl*)();
                const auto Query = reinterpret_cast<FIsInitialized>(GetProcAddress(Modules[Index], "Py_IsInitialized"));
                QueryAvailable = Query != nullptr;
                if (Query) Initialized = Query();
                Valid &= QueryAvailable && Initialized == 0;
            }
            Library->SetBoolField(TEXT("queryAvailable"), QueryAvailable);
            Library->SetNumberField(TEXT("interpreterInitialized"), Initialized);
            Libraries.Add(MakeShared<FJsonValueObject>(Library));
        }
    }
    Valid &= RuntimeFound || (!Plugin && Libraries.IsEmpty());
    State->SetBoolField(TEXT("runtimeLibraryLoaded"), RuntimeFound);
    State->SetArrayField(TEXT("libraries"), Libraries);
    State->SetBoolField(TEXT("valid"), Valid);
    return Valid;
}

void CaptureDdc(const FDerivedDataCacheStatsNode& Node, int32 Parent, TArray<TSharedPtr<FJsonValue>>& Stores)
{
    const int32 Index = Stores.Num();
    auto Store = MakeShared<FJsonObject>();
    Store->SetNumberField(TEXT("index"), Index);
    Store->SetNumberField(TEXT("parent"), Parent);
    Store->SetNumberField(TEXT("childCount"), Node.Children.Num());
    Store->SetStringField(TEXT("type"), Node.GetCacheType());
    Store->SetStringField(TEXT("name"), Node.GetCacheName());
    Store->SetBoolField(TEXT("local"), Node.IsLocal());
    Stores.Add(MakeShared<FJsonValueObject>(Store));
    for (const auto& Child : Node.Children) CaptureDdc(*Child, Index, Stores);
}
}

namespace
{
bool CookPlayableCandidate(const FString& Output)
{
    const FString Cooked = FPaths::Combine(Output, TEXT("Cooked"));
    FString Platform;
    FParse::Value(FCommandLine::Get(), TEXT("TargetPlatform="), Platform);
    if (!IsRunningCookCommandlet() || !GIsClient || !GIsServer || Platform != TEXT("Windows") || !GWarn
        || IFileManager::Get().DirectoryExists(*Cooked))
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Fresh Windows cook admission is missing."));
        return false;
    }
    UClass* CookClass = LoadClass<UCommandlet>(nullptr, TEXT("/Script/UnrealEd.CookCommandlet"));
    if (!CookClass)
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Installed standard CookCommandlet is unavailable."));
        return false;
    }
    TStrongObjectPtr<UCommandlet> Cook(NewObject<UCommandlet>(GetTransientPackage(), CookClass));
    const FString Args = FString::Printf(
        TEXT("-TargetPlatform=Windows -Map=/Game/SurvivalGame/Maps/Homestead -OutputDir=\"%s\" -CookProcessCount=1 -SkipZenStore -unattended -nop4"),
        *Cooked);
    const bool SkipZenStore = FParse::Param(*Args, TEXT("SkipZenStore"));
    auto Invocation = MakeShared<FJsonObject>();
    Invocation->SetStringField(TEXT("arguments"), Args);
    Invocation->SetBoolField(TEXT("skipZenStore"), SkipZenStore);
    if (!SkipZenStore || !SaveEvidence(FPaths::Combine(Output, TEXT("cook-invocation.json")), Invocation))
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Explicit loose-file cook invocation is missing."));
        return false;
    }
    UE_LOG(LogHomesteadAuthoringProbe, Display, TEXT("Candidate cook begin: %s"), *Cooked);
    const int32 Code = Cook->Main(Args);
    TArray<FString> Errors;
    GWarn->GetErrors(Errors);
    const bool Cancelled = IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("stop-probe.txt")))
        || IsEngineExitRequested();
    auto Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("exitCode"), Code);
    Result->SetStringField(TEXT("arguments"), Args);
    Result->SetBoolField(TEXT("skipZenStore"), SkipZenStore);
    Result->SetBoolField(TEXT("cancelled"), Cancelled);
    Result->SetStringField(TEXT("outputDirectory"), Cooked);
    Result->SetStringField(TEXT("targetPlatform"), Platform);
    Result->SetBoolField(TEXT("cookByTheBook"), true);
    Result->SetBoolField(TEXT("cookGlobalsInitialized"), IsRunningCookCommandlet());
    Result->SetBoolField(TEXT("isClient"), GIsClient);
    Result->SetBoolField(TEXT("isServer"), GIsServer);
    Result->SetNumberField(TEXT("cookProcessCount"), 1);
    TArray<TSharedPtr<FJsonValue>> ErrorValues;
    for (const FString& Error : Errors) ErrorValues.Add(MakeShared<FJsonValueString>(Error));
    Result->SetArrayField(TEXT("errors"), ErrorValues);
    const bool Passed = Code == 0 && Errors.IsEmpty() && !Cancelled;
    Result->SetBoolField(TEXT("passed"), Passed);
    UE_LOG(LogHomesteadAuthoringProbe, Display, TEXT("Candidate cook returned: code=%d errors=%d cancelled=%d"),
        Code, Errors.Num(), Cancelled);
    return SaveEvidence(FPaths::Combine(Output, TEXT("cook-result.json")), Result) && Passed;
}
}

UHomesteadAuthoringProbeCommandlet::UHomesteadAuthoringProbeCommandlet()
{
    FString FernMode;
    FParse::Value(FCommandLine::Get(), TEXT("FernMode="), FernMode);
    // AllocateScene otherwise creates a dummy scene even with a real RHI.
    IsClient = FernMode == TEXT("Cook") || (FernMode == TEXT("Render")
        && FParse::Param(FCommandLine::Get(), TEXT("AllowCommandletRendering"))
        && FParse::Param(FCommandLine::Get(), TEXT("RenderOffScreen")));
    IsServer = FernMode == TEXT("Cook");
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
    const FString CompletionPolicy = FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_PROBE_COMPLETION_POLICY"));
    const bool bCompletionDriven = CompletionPolicy == TEXT("until-complete");
    FString FernMode;
    FParse::Value(*Params, TEXT("FernMode="), FernMode);
    Require(CompletionPolicy.IsEmpty() || CompletionPolicy == TEXT("bounded") || bCompletionDriven,
        TEXT("Unknown native completion policy."));
    Require(!bCompletionDriven || ((FernMode == TEXT("Render") || FernMode == TEXT("Cook")
        || FernMode == TEXT("HairImport") || FernMode == TEXT("HairVerify")
        || FernMode == TEXT("WardrobeImport") || FernMode == TEXT("WardrobeVerify")
        || FernMode == TEXT("TreeImport") || FernMode == TEXT("TreeVerify"))
        && FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_PROBE_DEADLINE")).IsEmpty()),
        TEXT("Completion-driven mode must be render/cook/hair without a synthetic deadline."));
    Result->SetBoolField(TEXT("completionDriven"), bCompletionDriven);
    UE_LOG(LogHomesteadAuthoringProbe, Display, TEXT("Native Main entry: completionDriven=%s"),
        bCompletionDriven ? TEXT("true") : TEXT("false"));
    auto PythonEntry = MakeShared<FJsonObject>();
    Require(CapturePython(PythonEntry), TEXT("Python execution is not proved disabled at entry."));
    Result->SetObjectField(TEXT("pythonEntry"), PythonEntry);
    Result->SetNumberField(TEXT("pid"), GetCurrentProcessId());
    Result->SetStringField(TEXT("executable"), FPaths::ConvertRelativePathToFull(
        FPaths::Combine(FPlatformProcess::BaseDir(), FPlatformProcess::ExecutableName(false))));
    Result->SetStringField(TEXT("projectSavedDirectory"), FPaths::ProjectSavedDir());
    Result->SetStringField(TEXT("userDirectory"), FPlatformProcess::UserDir());
    Result->SetStringField(TEXT("engineIni"), GEngineIni);
    Result->SetStringField(TEXT("editorSettingsIni"), GEditorSettingsIni);
    Result->SetStringField(TEXT("editorPerProjectIni"), GEditorPerProjectIni);
    Result->SetStringField(TEXT("gameUserSettingsIni"), GGameUserSettingsIni);
    TArray<TSharedPtr<FJsonValue>> ConfigBranches;
    struct FConfigIdentity { const TCHAR* Name; FString Key; };
    for (const FConfigIdentity& Identity : {
        FConfigIdentity{TEXT("Engine"), GEngineIni}, {TEXT("Editor"), GEditorIni},
        {TEXT("EditorSettings"), GEditorSettingsIni}, {TEXT("EditorPerProjectUserSettings"), GEditorPerProjectIni},
        {TEXT("GameUserSettings"), GGameUserSettingsIni}, {TEXT("Game"), GGameIni}, {TEXT("Input"), GInputIni}})
    {
        auto Record = MakeShared<FJsonObject>();
        Record->SetStringField(TEXT("name"), Identity.Name);
        Record->SetStringField(TEXT("logicalKey"), Identity.Key);
        const FConfigBranch* Branch = GConfig->FindBranchWithNoReload(FName(Identity.Name), Identity.Key);
        Record->SetBoolField(TEXT("found"), Branch != nullptr);
        Require(Branch != nullptr, TEXT("An existing config branch is missing."));
        if (Branch)
        {
            Record->SetStringField(TEXT("destination"), Branch->IniPath);
            Require(!FPaths::IsRelative(Branch->IniPath) && FPaths::IsSamePath(Branch->IniPath,
                FPaths::Combine(Output, TEXT("Config"), FString(Identity.Name) + TEXT(".ini"))),
                TEXT("An actual config destination differs from the isolated file."));
            Record->SetStringField(TEXT("sourceEngineDirectory"), Branch->SourceEngineConfigDir);
            Record->SetStringField(TEXT("sourceProjectDirectory"), Branch->SourceProjectConfigDir);
            TArray<TSharedPtr<FJsonValue>> Hierarchy, Static, Dynamic;
            for (const auto& Layer : Branch->Hierarchy)
                Hierarchy.Add(MakeShared<FJsonValueString>(FString(Layer.Value)));
            for (const auto& Layer : Branch->StaticLayers)
                Static.Add(MakeShared<FJsonValueString>(Layer.Key));
            for (const FConfigCommandStream* Layer : Branch->DynamicLayers)
                Dynamic.Add(MakeShared<FJsonValueString>(Layer->Filename));
            Record->SetArrayField(TEXT("hierarchy"), Hierarchy);
            Record->SetArrayField(TEXT("staticLayers"), Static);
            Record->SetArrayField(TEXT("dynamicLayers"), Dynamic);
            Record->SetStringField(TEXT("savedLayer"), Branch->SavedLayer.Filename);
            Record->SetStringField(TEXT("commandLineLayer"), Branch->CommandLineOverrides.Filename);
            Record->SetNumberField(TEXT("commandLineSectionCount"), Branch->CommandLineOverrides.Num());
        }
        ConfigBranches.Add(MakeShared<FJsonValueObject>(Record));
    }
    Result->SetArrayField(TEXT("configBranches"), ConfigBranches);
    Require(FPlatformMisc::GetEnvironmentVariable(TEXT("UE_SKIP_UBT_SDK_SETUP")) == TEXT("1"),
        TEXT("Settings-only SDK validation suppression is missing."));

    const FString HandleText = FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_GUARD_HANDLE"));
    TCHAR* End = nullptr;
    const uint64 HandleNumber = FCString::Strtoui64(*HandleText, &End, 10);
    Require(!HandleText.IsEmpty() && End && *End == 0 && HandleNumber != 0, TEXT("Invalid inherited marker handle."));
    const HANDLE Marker = reinterpret_cast<HANDLE>(static_cast<UPTRINT>(HandleNumber));
    BY_HANDLE_FILE_INFORMATION Info{};
    Require(GetFileInformationByHandle(Marker, &Info) != 0, TEXT("Inherited marker identity unavailable."));
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
    BOOL InJob = 0;
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
    Require(GetProcessTimes(GetCurrentProcess(), &Created, &Exited, &Kernel, &User) != 0, TEXT("Process creation identity unavailable."));
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
    TArray<TSharedPtr<FJsonValue>> Plugins;
    for (const TSharedRef<IPlugin>& Plugin : IPluginManager::Get().GetEnabledPlugins())
    {
        Plugins.Add(MakeShared<FJsonValueString>(Plugin->GetName()));
        Require(Plugin->GetName() != TEXT("UdpMessaging")
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
    Result->SetStringField(TEXT("shaderEvidenceLimit"), TEXT("Entry settings snapshot only; any fern workload readiness is reported separately."));
    Result->SetStringField(TEXT("ddcGraph"), GetDerivedDataCacheRef().GetGraphName());
    TArray<TSharedPtr<FJsonValue>> Stores;
    // UE5.8 still uses this public legacy adapter for its own cache diagnostics.
    PRAGMA_DISABLE_DEPRECATION_WARNINGS
    CaptureDdc(*GetDerivedDataCacheRef().GatherUsageStats(), -1, Stores);
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
    const bool bHairMode = FernMode == TEXT("HairImport") || FernMode == TEXT("HairVerify");
    const bool bWardrobeMode = FernMode == TEXT("WardrobeImport") || FernMode == TEXT("WardrobeVerify");
    const bool bTreeMode = FernMode == TEXT("TreeImport") || FernMode == TEXT("TreeVerify");
    if ((bWardrobeMode || bTreeMode) && !bCompletionDriven) return 8;
    if (!FernMode.IsEmpty() && FernMode != TEXT("Import") && FernMode != TEXT("Render") && FernMode != TEXT("Cook") && !bHairMode && !bWardrobeMode && !bTreeMode) return 8;
    const double Deadline = FPlatformTime::Seconds() + (FernMode == TEXT("Render") ? 510
        : (FernMode == TEXT("Import") || FernMode == TEXT("HairImport")) ? 180 : 90);
    FDateTime RunDeadline;
    if (!bCompletionDriven && !FDateTime::ParseIso8601(*FPlatformMisc::GetEnvironmentVariable(TEXT("HOMESTEAD_PROBE_DEADLINE")), RunDeadline))
    {
        UE_LOG(LogHomesteadAuthoringProbe, Error, TEXT("Missing native deadline."));
        return 5;
    }
    const auto MayContinue = [&]()
    {
        return bCompletionDriven || (FPlatformTime::Seconds() < Deadline && FDateTime::UtcNow() < RunDeadline);
    };
    FString Stop;
    if (!FernMode.IsEmpty())
    {
        const FString AdmitPath = FPaths::Combine(Output, TEXT("operation-admitted.txt"));
        const FString StopPath = FPaths::Combine(Output, TEXT("stop-probe.txt"));
        while (!IFileManager::Get().FileExists(*AdmitPath) && !IFileManager::Get().FileExists(*StopPath)
            && MayContinue())
        {
            FPlatformProcess::SleepNoStats(0.02f);
        }
        FString Admission;
        if (!FFileHelper::LoadFileToString(Admission, *AdmitPath) || Admission != FernMode
            || IFileManager::Get().FileExists(*StopPath) || !MayContinue()
            || !(FernMode == TEXT("Cook") ? CookPlayableCandidate(Output)
                : bTreeMode ? RunTreeSpike(FernMode, Output, RunDeadline, bCompletionDriven)
                : bWardrobeMode ? RunWardrobeSpike(FernMode, Output, RunDeadline, bCompletionDriven)
                : bHairMode ? RunHairWaveSpike(FernMode, Output, RunDeadline, bCompletionDriven)
                : RunFernSpike(FernMode, Output, RunDeadline, bCompletionDriven)))
        {
            Valid = false;
            Stop = TEXT("fern-operation-failed");
        }
    }
    while (Stop.IsEmpty() && MayContinue())
    {
        if (IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("stop-probe.txt"))))
        {
            if (!FFileHelper::LoadFileToString(Stop, *FPaths::Combine(Output, TEXT("stop-probe.txt")))) return 6;
            break;
        }
        FPlatformProcess::SleepNoStats(0.02f);
    }
    auto Exit = MakeShared<FJsonObject>();
    auto PythonExit = MakeShared<FJsonObject>();
    Require(CapturePython(PythonExit), TEXT("Python execution is not proved disabled before exit."));
    Exit->SetObjectField(TEXT("pythonExit"), PythonExit);
    Exit->SetStringField(TEXT("stopReason"), Stop);
    Exit->SetBoolField(TEXT("cooperative"), true);
    Exit->SetBoolField(TEXT("passed"), Valid && Stop.TrimStartAndEnd() == TEXT("complete"));
    Require(SaveEvidence(FPaths::Combine(Output, TEXT("native-exit.json")), Exit), TEXT("Cannot persist stop evidence."));
    return Valid && Stop.TrimStartAndEnd() == TEXT("complete") ? 0 : 7;
}
