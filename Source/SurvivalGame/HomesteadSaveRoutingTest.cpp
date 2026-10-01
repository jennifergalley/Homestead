#include "HomesteadController.h"

#if !UE_BUILD_SHIPPING
#include "HomesteadSave.h"
#include "HomesteadTestPaths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformProcess.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/SecureHash.h"

void AHomesteadController::RunSaveRoutingChecks()
{
    const FString Output = HomesteadTestOutputDirectory();
    const bool ReadOnly = FParse::Param(FCommandLine::Get(), TEXT("HomesteadRoutingReadOnly"));
    const FString FixtureSaved = FPaths::Combine(Output, TEXT("SaveRoutingFixtures"), TEXT("Default"));
    const FString UserRoot = FPlatformProcess::UserSettingsDir();
    const FString Token = FMD5::HashAnsiString(*Output).Left(12).ToLower();
    FString Report = TEXT("Synthetic fixtures only. Production default path is resolved read-only, never opened.\n");
    int32 Checks = 0, Failures = 0;
    const auto Check = [&](bool Passed, const FString& Label)
    {
        ++Checks;
        Report += (Passed ? TEXT("PASS ") : TEXT("FAIL ")) + Label + TEXT("\n");
        if (!Passed) ++Failures;
    };
    const auto Resolve = [&](const FString& Command, FHomesteadSaveRoute& Route, const FString& SavedRoot)
    {
        FString Error;
        return ResolveHomesteadSaveRoute(*Command, SavedRoot, UserRoot, Output, Route, Error);
    };
    FHomesteadSaveRoute ActualDefault;
    Check(Resolve(TEXT(""), ActualDefault, FPaths::ProjectSavedDir()), TEXT("Production default resolves without accessing files"));
    Report += TEXT("DEFAULT_READ_ONLY=") + ActualDefault.Directory + TEXT("\n");
    FHomesteadSaveRoute Route;
    const TArray<FString> Invalid = {TEXT("-HomesteadPreviewProfile"), TEXT("-HomesteadPreviewProfile="),
        TEXT("-HomesteadPreviewProfile=\"\""), TEXT("-HomesteadPreviewProfile=../escape"),
        TEXT("-HomesteadPreviewProfile=C:\\escape"), TEXT("-HomesteadPreviewProfile=Upper"),
        TEXT("-HomesteadPreviewProfile=bad/name"), TEXT("-HomesteadPreviewProfile=bad\\name"),
        TEXT("-HomesteadPreviewProfile=\"has space\""), TEXT("-HomesteadPreviewProfile=-option"),
        TEXT("-HomesteadPreviewProfile=abcdefghijklmnopqrstuvwxyz1234567"),
        TEXT("-HomesteadPreviewProfile=a -HomesteadPreviewProfile=b"), TEXT("-HomesteadPreviewProfileExtra=a")};
    for (const FString& Command : Invalid)
    {
        Check(!Resolve(Command, Route, FixtureSaved) && Route.Directory.IsEmpty(), TEXT("Reject without fallback: ") + Command);
        Check(!Resolve(Command + TEXT(" -HomesteadSmokeTest"), Route, FixtureSaved) && Route.Directory.IsEmpty(),
            TEXT("Invalid preview also fails before sandbox IO: ") + Command);
    }
    for (const FString& Profile : {FString(TEXT("a")), FString(TEXT("jenny-review")), FString(TEXT("con")), FString(TEXT("abcdefghijklmnopqrstuvwxyz123456"))})
        Check(Resolve(TEXT("-HomesteadPreviewProfile=") + Profile, Route, FixtureSaved)
            && Route.Mode == TEXT("preview") && Route.Profile == Profile
            && Route.Directory.Contains(TEXT("/profile-") + Profile + TEXT("/SaveGames")),
            TEXT("Limited profile resolves under fixed prefixed directory: ") + Profile);
    for (const FString& Flag : {FString(TEXT("HomesteadSmokeTest")), FString(TEXT("HomesteadVisualPlaytest"))})
        Check(Resolve(TEXT("-HomesteadPreviewProfile=valid -") + Flag, Route, FixtureSaved)
            && Route.Mode == TEXT("test-sandbox") && Route.Profile.IsEmpty()
            && Route.Directory == FPaths::ConvertRelativePathToFull(FPaths::Combine(Output, TEXT("SmokeSave"))),
            TEXT("Sandbox precedence: ") + Flag);

    struct FFixture
    {
        FHomesteadSaveRoute Route;
        FString Name, World, Before, After;
    };
    TArray<FFixture> Fixtures;
    const TArray<FString> Commands = {TEXT(""), TEXT("-HomesteadPreviewProfile=verification-a-") + Token,
        TEXT("-HomesteadPreviewProfile=verification-b-") + Token,
        TEXT("-HomesteadPreviewProfile=ignored -HomesteadSmokeTest")};
    bool Safe = true;
    for (int32 Index = 0; Index < Commands.Num(); ++Index)
    {
        FFixture Fixture;
        Fixture.Name = FString::Printf(TEXT("route-%d"), Index);
        Fixture.World = FMD5::HashAnsiString(*(Output + Fixture.Name));
        Safe &= Resolve(Commands[Index], Fixture.Route, FixtureSaved);
        Safe &= !Fixture.Route.Directory.IsEmpty() && Fixture.Route.Directory != ActualDefault.Directory;
        const bool Exists = IFileManager::Get().DirectoryExists(*Fixture.Route.Directory);
        Safe &= ReadOnly ? Exists : !Exists;
        Report += FString::Printf(TEXT("FIXTURE_%d=%s\n"), Index, *Fixture.Route.Directory);
        Sim.NewGame();
        int32 Harvests = 0;
        for (const auto Node : Sim.GetState().resources)
        {
            if (Node.kind != Homestead::ResourceKind::Branches) continue;
            Safe &= Sim.Harvest(Node.id, Node.position).ok;
            if (++Harvests == Index + 1) break;
        }
        Fixture.Before = UTF8_TO_TCHAR(Sim.Serialize().c_str());
        bool Berry = false;
        for (const auto Node : Sim.GetState().resources)
        {
            if (Node.kind != Homestead::ResourceKind::BerryBush) continue;
            Berry = Sim.Harvest(Node.id, Node.position).ok;
            break;
        }
        Safe &= Berry && Harvests == Index + 1;
        Fixture.After = UTF8_TO_TCHAR(Sim.Serialize().c_str());
        Fixtures.Add(MoveTemp(Fixture));
    }
    Check(Safe, ReadOnly ? TEXT("Only existing isolated fixture namespaces may be read")
        : TEXT("All four fixture namespaces are new; no production default IO or existing-profile overwrite"));
    const TArray<FString> Slots = {TEXT("Homestead_Manual"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"),
        TEXT("Homestead_Auto_2"), TEXT("Homestead_Recovery")};
    if (Safe)
    {
        for (const FFixture& Fixture : Fixtures)
        {
            SaveRoute = Fixture.Route;
            WorldId = Fixture.World;
            for (const FString& Slot : Slots)
            {
                const FString Label = Fixture.Name + TEXT(" ") + Slot;
                if (!ReadOnly)
                {
                    Check(Sim.Deserialize(TCHAR_TO_UTF8(*Fixture.Before)).ok && SaveSlot(Slot, true), Label + TEXT(" first atomic write"));
                    Check(Sim.Deserialize(TCHAR_TO_UTF8(*Fixture.After)).ok && SaveSlot(Slot, true), Label + TEXT(" replacement and backup"));
                }
                const UHomesteadSave* Current = ReadSave(SavePath(Slot));
                const UHomesteadSave* Backup = ReadSave(SavePath(Slot) + TEXT(".bak"));
                Check(Current && Current->WorldId == Fixture.World && Current->SimulationData == Fixture.After,
                    Label + TEXT(" current payload belongs only to its profile"));
                Check(Backup && Backup->WorldId == Fixture.World && Backup->SimulationData == Fixture.Before,
                    Label + TEXT(" prior payload remains in same-namespace backup"));
                Check(!IFileManager::Get().FileExists(*(SavePath(Slot) + TEXT(".tmp"))), Label + TEXT(" atomic temporary file consumed"));
                if (Backup)
                {
                    ApplySave(*Backup);
                    Check(WorldId == Fixture.World && UTF8_TO_TCHAR(Sim.Serialize().c_str()) == Fixture.Before,
                        Label + TEXT(" backup loads through actual ApplySave"));
                }
            }
            TArray<FString> Files;
            IFileManager::Get().FindFiles(Files, *(SaveRoute.Directory / TEXT("*")), true, false);
            Check(Files.Num() == 10, Fixture.Name + TEXT(" exactly five slots and five backups"));
        }
        // Revisit every profile after all writes, including across a second process.
        for (int32 Index = Fixtures.Num() - 1; Index >= 0; --Index)
        {
            const FFixture& Fixture = Fixtures[Index];
            SaveRoute = Fixture.Route;
            WorldId = Fixture.World;
            Sim.NewGame();
            Check(LoadLatest() && WorldId == Fixture.World && UTF8_TO_TCHAR(Sim.Serialize().c_str()) == Fixture.After,
                Fixture.Name + TEXT(" latest load never crosses profile"));
            Sim.NewGame();
            Check(LoadLatest(true) && WorldId == Fixture.World && UTF8_TO_TCHAR(Sim.Serialize().c_str()) == Fixture.After,
                Fixture.Name + TEXT(" recovery load never crosses profile"));
        }
        const FFixture& RecoveryFixture = Fixtures[3];
        const bool MapWasEstate = bEstateMap;
        Check(MapWasEstate, TEXT("Recovery preference fixture runs on the Estate map"));
        SaveRoute = RecoveryFixture.Route;
        SaveRoute.Directory = FPaths::Combine(Output, TEXT("RecoveryChoice"));
        WorldId = RecoveryFixture.World;
        if (!ReadOnly)
        {
            Check(!IFileManager::Get().DirectoryExists(*SaveRoute.Directory),
                TEXT("Recovery preference uses a fresh isolated test directory"));
            Check(Sim.Deserialize(TCHAR_TO_UTF8(*RecoveryFixture.Before)).ok
                && SaveSlot(TEXT("Homestead_Recovery"), true), TEXT("Older recovery fixture saved"));
            Check(Sim.Deserialize(TCHAR_TO_UTF8(*RecoveryFixture.After)).ok
                && SaveSlot(TEXT("Homestead_Auto_1"), true), TEXT("Newer autosave fixture saved"));
            Sim.NewGame();
            Check(LoadLatest(true) && UTF8_TO_TCHAR(Sim.Serialize().c_str()) == RecoveryFixture.After,
                TEXT("Estate recovery chooses the newer auto instead of the older recovery slot"));
            bEstateMap = false;
            Sim.NewGame();
            Check(LoadLatest(true) && UTF8_TO_TCHAR(Sim.Serialize().c_str()) == RecoveryFixture.Before
                && Toast().Contains(TEXT("sheltered recovery checkpoint")),
                TEXT("Woodland recovery prefers its safe sheltered checkpoint to a newer auto"));
            bEstateMap = MapWasEstate;
            Check(FFileHelper::SaveStringToFile(TEXT("corrupt newest save"), *SavePath(TEXT("Homestead_Auto_1"))),
                TEXT("Corrupt only the isolated newest autosave fixture"));
        }
        Sim.NewGame();
        Check(LoadLatest(true) && UTF8_TO_TCHAR(Sim.Serialize().c_str()) == RecoveryFixture.Before,
            TEXT("Estate corrupt newest auto falls back to the last valid recovery slot"));
        SaveRoute.Directory = FPaths::Combine(Output, TEXT("UnsafeRecovery"));
        if (!ReadOnly)
        {
            Sim.NewGame();
            Check(Sim.SetEnergy(50).ok && SaveSlot(TEXT("Homestead_Auto_0"), true),
                TEXT("Woodland fallback has a safe autosave"));
            Check(Sim.SetEnergy(10).ok && SaveSlot(TEXT("Homestead_Recovery"), true),
                TEXT("Woodland fixture has a newer but unsafe sheltered recovery"));
        }
        bEstateMap = false;
        Sim.NewGame();
        Check(LoadLatest(true) && FMath::IsNearlyEqual(Sim.GetState().energy, 50.0),
            TEXT("Woodland skips unsafe recovery and chooses the safe autosave"));
        bEstateMap = MapWasEstate;
        SaveRoute = Fixtures[1].Route;
        Check(PreviewLabel().Contains(Fixtures[1].Route.Profile), TEXT("Preview identification includes active isolated profile"));
        const uint32 IgnoredBefore = IgnoredExternalInputs;
        bAutomatedInputOnly = false;
        for (const FKey Key : {EKeys::Gamepad_Special_Right, EKeys::Escape})
        {
            InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, IE_Pressed, 1, false, FPlatformTime::Cycles64()));
            Check(bGamepad == Key.IsGamepadKey() && IgnoredExternalInputs == IgnoredBefore,
                TEXT("Normal input handler accepts physical-source-style ") + Key.ToString());
            InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, Key, IE_Released, 0, false, FPlatformTime::Cycles64()));
        }
        const auto RawAxis = [this, &Check, IgnoredBefore](FKey Key, float Value, bool Gamepad, const FString& Label)
        {
            const FInputKeyEventArgs Event(nullptr, INPUTDEVICEID_NONE, Key, Value, 1.0f / 60.0f, 1, FPlatformTime::Cycles64());
            InputKey(Event);
            Check(!Event.IsSimulatedInput() && !bAutomatedInputOnly && SaveRoute.Mode == TEXT("preview")
                && bGamepad == Gamepad && IgnoredExternalInputs == IgnoredBefore, Label);
        };
        RawAxis(EKeys::Gamepad_LeftY, 0.8f, true, TEXT("Normal preview accepts physical-source-style stick intent"));
        RawAxis(EKeys::MouseX, 0.01f, true, TEXT("Normal preview noise does not steal prompts; input is not rejected"));
        RawAxis(EKeys::MouseX, 4, false, TEXT("Normal preview deliberate mouse switches prompts without automation"));
        RawAxis(EKeys::Gamepad_LeftY, 0.8f, false, TEXT("Normal preview held stick respects deliberate mouse grace"));
        bAutomatedInputOnly = true;
        Check(!FParse::Param(FCommandLine::Get(), TEXT("HomesteadSmokeTest"))
            && !FParse::Param(FCommandLine::Get(), TEXT("HomesteadVisualPlaytest")),
            TEXT("Routing harness does not spawn smoke or visual actors"));
    }
    Report += FString::Printf(TEXT("%s %d checks; %d failures; phase=%s\n"),
        Failures ? TEXT("FAILURE") : TEXT("SUCCESS"), Checks, Failures, ReadOnly ? TEXT("relaunch-read") : TEXT("write"));
    const FString ReportPath = FPaths::Combine(Output, ReadOnly ? TEXT("routing-read.txt") : TEXT("routing-write.txt"));
    const bool Written = FFileHelper::SaveStringToFile(Report, *ReportPath);
    UE_LOG(LogTemp, Display, TEXT("%s"), *Report);
    if (!Written) UE_LOG(LogTemp, Error, TEXT("Could not write routing test report: %s"), *ReportPath);
    bSaveRoutingReady = false;
    FPlatformMisc::RequestExitWithStatus(false, Failures || !Written ? 1 : 0);
}
#endif
