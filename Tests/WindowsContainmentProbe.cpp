#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <string>
#include <vector>

struct Handle
{
    HANDLE value = nullptr;
    explicit Handle(HANDLE input = nullptr) : value(input) {}
    ~Handle() { if (valid()) CloseHandle(value); }
    bool valid() const { return value && value != INVALID_HANDLE_VALUE; }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
};

static bool EarlyPassed = true;

static void Require(bool condition, const char* name)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL %s win32=%lu\n", name, GetLastError());
        std::fflush(stdout);
        ExitProcess(2);
    }
}

static bool Exists(const std::wstring& path)
{
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) return true;
    Require(GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_PATH_NOT_FOUND,
        "marker existence query");
    return false;
}

static void WriteNew(const std::wstring& path, const char* contents)
{
    Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
    Require(file.valid(), "create fresh fixture");
    DWORD length = static_cast<DWORD>(std::char_traits<char>::length(contents));
    DWORD written = 0;
    Require(WriteFile(file.value, contents, length, &written, nullptr) && written == length,
        "write fixture");
}

static std::wstring Executable()
{
    wchar_t path[32768];
    DWORD count = GetModuleFileNameW(nullptr, path, 32768);
    Require(count > 0 && count < 32768, "own executable identity");
    return std::wstring(path, count);
}

static void Append(const std::wstring& root, const char* line)
{
    Handle file(CreateFileW((root + L"\\attempts.tsv").c_str(), FILE_APPEND_DATA, FILE_SHARE_READ,
        nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
    Require(file.valid(), "open attempt evidence");
    DWORD written = 0;
    DWORD length = static_cast<DWORD>(std::char_traits<char>::length(line));
    Require(WriteFile(file.value, line, length, &written, nullptr) && written == length,
        "persist attempt evidence");
}

static bool Attempts(const std::wstring& root, const wchar_t* phase, bool contained)
{
    const DWORD flags[] = {0, DETACHED_PROCESS, CREATE_BREAKAWAY_FROM_JOB,
        DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB};
    const wchar_t* names[] = {L"normal", L"detached", L"breakaway", L"detached-breakaway"};
    BOOL inheritedJob = FALSE;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION inheritedLimits{};
    Require(IsProcessInJob(GetCurrentProcess(), nullptr, &inheritedJob) != FALSE, "subject job membership");
    if (inheritedJob)
        Require(QueryInformationJobObject(nullptr, JobObjectExtendedLimitInformation, &inheritedLimits,
            sizeof(inheritedLimits), nullptr) != FALSE, "inherited job limits");
    bool passed = true;
    for (int i = 0; i < 4; ++i)
    {
        std::wstring marker = root + L"\\" + phase + L"-" + names[i] + L".marker";
        Require(!Exists(marker), "fresh child marker");
        std::wstring command = L"\"" + Executable() + L"\" --child \"" + marker + L"\"";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION process{};
        SetLastError(ERROR_SUCCESS);
        BOOL created = CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
            flags[i] | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &startup, &process);
        DWORD error = created ? ERROR_SUCCESS : GetLastError();
        DWORD exitCode = STILL_ACTIVE;
        if (created)
        {
            Handle child(process.hProcess);
            Handle thread(process.hThread);
            DWORD waited = WaitForSingleObject(child.value, 5000);
            if (waited != WAIT_OBJECT_0)
            {
                TerminateProcess(child.value, 98);
                WaitForSingleObject(child.value, 5000);
                Require(false, "synthetic child timeout (owned-handle hard stop)");
            }
            Require(GetExitCodeProcess(child.value, &exitCode) != FALSE, "child exit code");
        }
        bool markerPresent = Exists(marker);
        bool inheritedBreakawayDenial = !contained && (flags[i] & CREATE_BREAKAWAY_FROM_JOB) &&
            inheritedJob && !(inheritedLimits.BasicLimitInformation.LimitFlags &
                (JOB_OBJECT_LIMIT_BREAKAWAY_OK | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK)) &&
            !created && error == ERROR_ACCESS_DENIED && !markerPresent;
        bool success = contained ? (!created && error != 0 && !markerPresent)
                                 : ((created && exitCode == 0 && markerPresent) || inheritedBreakawayDenial);
        char line[400];
        sprintf_s(line, "%ls\t%ls\tcreated=%d\terror=%lu\texit=%lu\tmarker=%d\tinheritedJob=%d\tjobFlags=%lu\tbaselineBreakawayUnavailable=%d\tpassed=%d\n",
            phase, names[i], created, error, exitCode, markerPresent, inheritedJob,
            inheritedLimits.BasicLimitInformation.LimitFlags, inheritedBreakawayDenial, success);
        Append(root, line);
        passed = passed && success;
    }
    return passed;
}

// The subject tries to create children before wmain; each child writes before its own wmain.
struct StaticProbe
{
    StaticProbe()
    {
        int count = 0;
        wchar_t** arguments = CommandLineToArgvW(GetCommandLineW(), &count);
        Require(arguments != nullptr, "static arguments");
        if (count == 3 && wcscmp(arguments[1], L"--child") == 0)
            WriteNew(arguments[2], "child static initializer executed\n");
        if (count == 4 && wcscmp(arguments[1], L"--subject") == 0)
        {
            WriteNew(std::wstring(arguments[2]) + L"\\subject-static.marker", "subject static initializer\n");
            EarlyPassed = Attempts(arguments[2], L"static", wcscmp(arguments[3], L"contained") == 0);
        }
        LocalFree(arguments);
    }
};
static StaticProbe EarlyProbe;

static void Subject(const std::wstring& root, bool contained)
{
    Require(CreateDirectoryW(root.c_str(), nullptr) != FALSE, "fresh subject directory");
    Handle job(contained ? CreateJobObjectW(nullptr, nullptr) : nullptr);
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_ACTIVE_PROCESS | JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    limits.BasicLimitInformation.ActiveProcessLimit = 1;
    if (contained)
    {
        Require(job.valid(), "create private job");
        Require(SetInformationJobObject(job.value, JobObjectExtendedLimitInformation, &limits, sizeof(limits)) != FALSE,
            "set one-process/no-breakaway limits");
    }
    std::wstring command = L"\"" + Executable() + L"\" --subject \"" + root +
        (contained ? L"\" contained" : L"\" control");
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION info{};
    Require(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED,
        nullptr, nullptr, &startup, &info) != FALSE, "create suspended subject");
    Handle process(info.hProcess);
    Handle thread(info.hThread);
    auto failSuspended = [&](const char* reason)
    {
        TerminateProcess(process.value, 97);
        WaitForSingleObject(process.value, 5000);
        Require(false, reason);
    };
    if (contained && !AssignProcessToJobObject(job.value, process.value))
        failSuspended("assignment failed; subject was never resumed");
    if (Exists(root + L"\\subject-static.marker"))
        failSuspended("static initializer ran while suspended");
    if (contained)
    {
        BOOL member = FALSE;
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION actual{};
        JOBOBJECT_BASIC_ACCOUNTING_INFORMATION accounting{};
        if (!IsProcessInJob(process.value, job.value, &member) || !member ||
            !QueryInformationJobObject(job.value, JobObjectExtendedLimitInformation, &actual, sizeof(actual), nullptr) ||
            !QueryInformationJobObject(job.value, JobObjectBasicAccountingInformation, &accounting, sizeof(accounting), nullptr) ||
            actual.BasicLimitInformation.LimitFlags != limits.BasicLimitInformation.LimitFlags ||
            actual.BasicLimitInformation.ActiveProcessLimit != 1 || accounting.ActiveProcesses != 1)
            failSuspended("effective pre-resume job policy");
        std::printf("job pid=%lu flags=%lu activeLimit=1 activeBeforeResume=%lu noEarlyMarker=1\n",
            info.dwProcessId, actual.BasicLimitInformation.LimitFlags, accounting.ActiveProcesses);
    }
    if (ResumeThread(thread.value) != 1) failSuspended("resume primary thread");
    DWORD waited = WaitForSingleObject(process.value, 60000);
    if (waited != WAIT_OBJECT_0) failSuspended("subject timeout (owned-handle hard stop)");
    DWORD code = STILL_ACTIVE;
    Require(GetExitCodeProcess(process.value, &code) != FALSE && code == 0, "subject assertions");
    Require(Exists(root + L"\\subject-static.marker"), "subject static initializer positive witness");
    std::printf("%s static/runtime assertions passed (baseline breakaway eligibility recorded separately)\n",
        contained ? "contained" : "baseline-control");
}

static std::vector<BYTE> Security(const std::wstring& path)
{
    DWORD needed = 0;
    GetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION, nullptr, 0, &needed);
    Require(needed > 0, "fixture security size");
    std::vector<BYTE> result(needed);
    Require(GetFileSecurityW(path.c_str(), DACL_SECURITY_INFORMATION, result.data(), needed, &needed) != FALSE,
        "read fixture DACL");
    return result;
}

static void FileSharing(const std::wstring& path, const char* original)
{
    WriteNew(path, original);
    auto securityBefore = Security(path);
    {
        Handle reader(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        Require(reader.valid(), "read-only share-read fixture handle");
        BY_HANDLE_FILE_INFORMATION before{}, after{};
        Require(GetFileInformationByHandle(reader.value, &before) != FALSE, "fixture metadata before");
        Handle secondReader(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        Require(secondReader.valid(), "concurrent reader permitted");
        std::wstring command = L"\"" + Executable() + L"\" --file-writer \"" + path + L"\"";
        STARTUPINFOW startup{};
        startup.cb = sizeof(startup);
        PROCESS_INFORMATION info{};
        Require(CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
            nullptr, nullptr, &startup, &info) != FALSE, "start disposable writer");
        {
            Handle process(info.hProcess);
            Handle thread(info.hThread);
            if (WaitForSingleObject(process.value, 5000) != WAIT_OBJECT_0)
            {
                TerminateProcess(process.value, 96);
                WaitForSingleObject(process.value, 5000);
                Require(false, "disposable writer timeout");
            }
            DWORD code = STILL_ACTIVE;
            Require(GetExitCodeProcess(process.value, &code) && code == 0,
                "other-process Core-style OpenWrite denied");
        }
        for (DWORD disposition : {CREATE_ALWAYS, OPEN_ALWAYS})
        {
            for (DWORD share : {DWORD(0), DWORD(FILE_SHARE_READ | FILE_SHARE_DELETE)})
            {
                Handle writer(CreateFileW(path.c_str(), GENERIC_WRITE, share, nullptr,
                    disposition, FILE_ATTRIBUTE_NORMAL, nullptr));
                DWORD error = GetLastError();
                Require(!writer.valid() && error == ERROR_SHARING_VIOLATION, "write/append denied before truncation");
            }
        }
        Require(!DeleteFileW(path.c_str()) && GetLastError() == ERROR_SHARING_VIOLATION, "delete denied");
        Require(!MoveFileW(path.c_str(), (path + L".renamed").c_str()) &&
            GetLastError() == ERROR_SHARING_VIOLATION, "rename denied");
        Require(GetFileInformationByHandle(reader.value, &after) != FALSE &&
            before.nFileSizeHigh == after.nFileSizeHigh && before.nFileSizeLow == after.nFileSizeLow &&
            CompareFileTime(&before.ftLastWriteTime, &after.ftLastWriteTime) == 0, "size/write time preserved");
        char contents[64]{};
        DWORD read = 0;
        Require(ReadFile(reader.value, contents, sizeof(contents), &read, nullptr) &&
            std::string(contents, read) == original, "fixture bytes preserved");
        Require(Security(path) == securityBefore, "fixture DACL preserved");
    }
    {
        Handle writer(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        Require(writer.valid(), "write access restored after lock release");
        Handle conflictingReader(CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        Require(!conflictingReader.valid() && GetLastError() == ERROR_SHARING_VIOLATION,
            "existing writer makes lock acquisition fail closed");
    }
    std::printf("dummy-file bytes=%zu readers=allowed otherProcessOpenWrite=denied write/append/delete/rename=denied bytes/time/DACL=preserved release=restored conflict=denied\n",
        std::char_traits<char>::length(original));
}

int wmain(int count, wchar_t** arguments)
{
    if (count == 3 && wcscmp(arguments[1], L"--child") == 0) return 0;
    if (count == 3 && wcscmp(arguments[1], L"--file-writer") == 0)
    {
        Handle writer(CreateFileW(arguments[2], GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr));
        return !writer.valid() && GetLastError() == ERROR_SHARING_VIOLATION ? 0 : 3;
    }
    if (count == 4 && wcscmp(arguments[1], L"--subject") == 0)
        return Attempts(arguments[2], L"runtime", wcscmp(arguments[3], L"contained") == 0) && EarlyPassed ? 0 : 1;
    Require(count == 2, "usage: WindowsContainmentProbe fresh-output-directory");
    std::wstring root(arguments[1]);
    Require(CreateDirectoryW(root.c_str(), nullptr) != FALSE, "fresh probe output directory");
    BOOL existingJob = FALSE;
    Require(IsProcessInJob(GetCurrentProcess(), nullptr, &existingJob) != FALSE, "controller job observation");
    std::printf("controller pid=%lu inheritedJob=%d\n", GetCurrentProcessId(), existingJob);
    Subject(root + L"\\control", false);
    Subject(root + L"\\contained", true);
    FileSharing(root + L"\\dummy-empty", "");
    FileSharing(root + L"\\dummy-sentinel", "unchanged");
    std::puts("PASS synthetic containment and disposable-file sharing only; no Unreal/global marker/network used");
    return 0;
}
