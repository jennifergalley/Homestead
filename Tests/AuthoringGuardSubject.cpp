#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <string>

static std::wstring Env(const wchar_t* key)
{
    wchar_t value[32768];
    DWORD size = GetEnvironmentVariableW(key, value, 32768);
    if (!size || size >= 32768) ExitProcess(70);
    return std::wstring(value, size);
}
static void Check(bool okay, const char* operation)
{
    if (!okay) { std::fprintf(stderr, "FAIL %s error=%lu\n", operation, GetLastError()); ExitProcess(71); }
}
static HANDLE NumericHandle(const wchar_t* key)
{
    std::wstring value = Env(key);
    wchar_t* end = nullptr;
    unsigned long long number = wcstoull(value.c_str(), &end, 10);
    Check(end && *end == 0 && number != 0, "handle encoding");
    return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(number));
}
static void WriteNew(const std::wstring& path, const std::string& text)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    Check(file != INVALID_HANDLE_VALUE, "new fixture evidence");
    DWORD written = 0;
    Check(WriteFile(file, text.data(), static_cast<DWORD>(text.size()), &written, nullptr) &&
        written == text.size(), "fixture evidence write");
    Check(CloseHandle(file) != FALSE, "evidence close");
}
static bool ForbiddenMode()
{
    int count = 0;
    wchar_t** args = CommandLineToArgvW(GetCommandLineW(), &count);
    Check(args != nullptr, "arguments");
    bool child = count == 3 && wcscmp(args[1], L"--forbidden") == 0;
    if (child) WriteNew(args[2], "forbidden child initializer executed");
    LocalFree(args);
    return child;
}
static bool IsForbiddenChild = false;
struct EarlyCheck
{
    EarlyCheck()
    {
        IsForbiddenChild = ForbiddenMode();
        if (IsForbiddenChild) return;
        HANDLE marker = NumericHandle(L"HOMESTEAD_GUARD_HANDLE");
        BY_HANDLE_FILE_INFORMATION information{};
        Check(GetFileInformationByHandle(marker, &information) && !information.nFileSizeHigh &&
            !information.nFileSizeLow, "inherited empty marker identity");
        char data = 0; DWORD count = 0;
        Check(ReadFile(marker, &data, 1, &count, nullptr) && count == 0, "inherited readable marker");
        Check(!WriteFile(marker, "X", 1, &count, nullptr) && GetLastError() == ERROR_ACCESS_DENIED,
            "inherited handle has no write access");
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION job{};
        Check(!QueryInformationJobObject(NumericHandle(L"HOMESTEAD_GUARD_JOB_HANDLE"),
            JobObjectExtendedLimitInformation, &job, sizeof(job), nullptr), "job handle not inherited");
        BY_HANDLE_FILE_INFORMATION canary{};
        HANDLE forbidden = NumericHandle(L"HOMESTEAD_TEST_CANARY_HANDLE");
        bool opened = GetFileInformationByHandle(forbidden, &canary) != FALSE;
        unsigned long high = wcstoul(Env(L"HOMESTEAD_TEST_CANARY_HIGH").c_str(), nullptr, 10);
        unsigned long low = wcstoul(Env(L"HOMESTEAD_TEST_CANARY_LOW").c_str(), nullptr, 10);
        Check(!opened || canary.nFileIndexHigh != high || canary.nFileIndexLow != low, "ambient inheritable canary excluded");
        Check(QueryInformationJobObject(nullptr, JobObjectExtendedLimitInformation, &job, sizeof(job), nullptr) &&
            job.BasicLimitInformation.LimitFlags == 8200 && job.BasicLimitInformation.ActiveProcessLimit == 1,
            "actual subject job limits");
        wchar_t executable[32768];
        Check(GetModuleFileNameW(nullptr, executable, 32768) != 0, "own image");
        const DWORD flags[] = {0, DETACHED_PROCESS, CREATE_BREAKAWAY_FROM_JOB,
            DETACHED_PROCESS | CREATE_BREAKAWAY_FROM_JOB};
        for (int i = 0; i < 4; ++i)
        {
            std::wstring witness = Env(L"HOMESTEAD_GUARD_TEST_ROOT") + L"\\forbidden-" + std::to_wstring(i);
            std::wstring command = L"\"" + std::wstring(executable) + L"\" --forbidden \"" + witness + L"\"";
            STARTUPINFOW startup{}; startup.cb = sizeof(startup);
            PROCESS_INFORMATION process{};
            BOOL created = CreateProcessW(executable, command.data(), nullptr, nullptr, TRUE,
                flags[i] | CREATE_NEW_PROCESS_GROUP, nullptr, nullptr, &startup, &process);
            DWORD error = GetLastError();
            if (created)
            {
                TerminateProcess(process.hProcess, 72);
                WaitForSingleObject(process.hProcess, 5000);
                CloseHandle(process.hThread); CloseHandle(process.hProcess);
            }
            Check(!created && error == static_cast<DWORD>(i < 2 ? ERROR_NOT_ENOUGH_QUOTA : ERROR_ACCESS_DENIED) &&
                GetFileAttributesW(witness.c_str()) == INVALID_FILE_ATTRIBUTES, "pre-main inherited-handle child denial");
        }
        char json[512];
        sprintf_s(json, "{\"pid\":%lu,\"volume\":%lu,\"indexHigh\":%lu,\"indexLow\":%lu,"
            "\"readOnlyGuard\":true,\"jobHandleExcluded\":true,\"canaryExcluded\":true,\"deniedChildren\":4}",
            GetCurrentProcessId(), information.dwVolumeSerialNumber, information.nFileIndexHigh, information.nFileIndexLow);
        WriteNew(Env(L"HOMESTEAD_GUARD_TEST_ROOT") + L"\\ready.json", json);
    }
};
static EarlyCheck BeforeMain;

int wmain()
{
    if (IsForbiddenChild) return 73;
    const std::wstring root = Env(L"HOMESTEAD_GUARD_TEST_ROOT");
    const bool cooperative = Env(L"HOMESTEAD_GUARD_TEST_CASE") == L"normal";
    const ULONGLONG deadline = GetTickCount64() + 20000;
    while (GetTickCount64() < deadline)
    {
        if (cooperative && GetFileAttributesW((root + L"\\stop.txt").c_str()) != INVALID_FILE_ATTRIBUTES)
        {
            WriteNew(root + L"\\cooperative.json", "{\"stopMarkerObserved\":true}");
            return 0;
        }
        Sleep(5);
    }
    return 74;
}
