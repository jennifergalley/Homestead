#include "HomesteadController.h"
#include "HomesteadControllerText.h"
#include "HomesteadSavePreference.h"
#include "HomesteadCharacter.h"
#include "HomesteadWorld.h"
#include "HomesteadSave.h"
#include "HomesteadEstateTerrain.h"
#include "Simulation/HomesteadEstate.h"
#include "Simulation/HomesteadManor.h"
#include "UI/SHomesteadMenu.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Components/AudioComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Crc.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogHomesteadSaves, Log, All);

using HomesteadControllerText::Text;

UHomesteadSave* AHomesteadController::ReadSave(const FString& Filename) const
{
    bReadIncompatible = false;
    bReadNewer = false;
    TArray<uint8> Data;
    if (IFileManager::Get().FileSize(*Filename) > 20 * 1024 * 1024) return nullptr;
    if (!FFileHelper::LoadFileToArray(Data, *Filename)) return nullptr;
    const char Magic[] = "HOMESAV1";
    if (Data.Num() < 16 || FMemory::Memcmp(Data.GetData(), Magic, 8) != 0) return nullptr;
    uint32 ExpectedCrc = 0;
    FMemory::Memcpy(&ExpectedCrc, Data.GetData() + 8, sizeof(ExpectedCrc));
    if (FCrc::MemCrc32(Data.GetData() + 12, Data.Num() - 12) != ExpectedCrc) return nullptr;
    Data.RemoveAt(0, 12, EAllowShrinking::No);
    UHomesteadSave* Save = Cast<UHomesteadSave>(UGameplayStatics::LoadGameFromMemory(Data));
    FGuid ParsedWorld;
    if (Save && !Save->IsCurrentVersion()) { bReadIncompatible = true; return nullptr; }
    if (!Save || Save->SavedAtUtc < 0 || Save->SavedAtUtc > 253402300799LL || Save->SavedRevision < 0
        || Save->PlayerLocation.ContainsNaN() || Save->ViewRotation.ContainsNaN()
        || !FGuid::Parse(Save->WorldId, ParsedWorld) || !ParsedWorld.IsValid()
        || FMath::Abs(Save->PlayerLocation.X) > Homestead::MaxWorldCoordinate
        || FMath::Abs(Save->PlayerLocation.Y) > Homestead::MaxWorldCoordinate
        || FMath::Abs(Save->PlayerLocation.Z) > Homestead::MaxWorldCoordinate || !FMath::IsFinite(Save->CameraSensitivity)
        || Save->CameraSensitivity < 0.2 || Save->CameraSensitivity > 3
        || !FMath::IsFinite(Save->MusicVolume) || Save->MusicVolume < 0 || Save->MusicVolume > 1
        || !FMath::IsFinite(Save->AmbienceVolume) || Save->AmbienceVolume < 0 || Save->AmbienceVolume > 1
        || !FMath::IsFinite(Save->EffectsVolume) || Save->EffectsVolume < 0 || Save->EffectsVolume > 1)
        return nullptr;
    FHomesteadAppearance SavedLook;
    SavedLook.HairStyle = Save->HairStyle; SavedLook.MetaHair = Save->MetaHair >= 0 ? Save->MetaHair : HomesteadLook::MetaHairForLegacy(Save->HairStyle);
    SavedLook.HairColor = Save->HairColor;
    SavedLook.SkinTone = Save->SkinTone;
    SavedLook.EyeColor = Save->EyeColor;
    SavedLook.TunicColor = Save->TunicColor;
    SavedLook.Outfit = Save->Outfit;
    SavedLook.BodyPreset = Save->BodyPreset;
    if (!SavedLook.IsValid()) return nullptr;
    Homestead::Simulation Candidate;
    if (bEstateMap) PrepareEstateSimulation(Candidate);
    const auto Decoded = Candidate.Deserialize(TCHAR_TO_UTF8(*Save->SimulationData));
    if (!Decoded)
    {
        bReadIncompatible = Decoded.code == Homestead::ResultCode::UnsupportedVersion;
        bReadNewer = Decoded.code == Homestead::ResultCode::NewerBuild;
        return nullptr;
    }
    return Save;
}

bool AHomesteadController::SaveSlot(const FString& Slot, bool Quiet)
{
    if (bTestResetRequired) { Notify(TEXT("Choose an explicit test reset before saving a new woodland."), true); return false; }
    if (bPendingGroundSnap) { Notify(TEXT("Wait until she is safely on the ground before saving."), true); return false; }
    if (!bWorldReady) { Notify(TEXT("The world is not ready; no save files were changed."), true); return false; }
    if (!bSaveRoutingReady) { Notify(TEXT("Save routing is unavailable. No save files were accessed."), true); return false; }
    if (const EHomesteadSaveHold Hold = SaveHold(); Hold != EHomesteadSaveHold::None)
    {
        const TCHAR* Reason = Hold == EHomesteadSaveHold::AwaitingSpawn
            ? TEXT("She's still arriving. Wait until she has landed before saving.")
            : TEXT("Finish naming her and the estate before saving.");
        UE_LOG(LogHomesteadSaves, Display, TEXT("Save to %s held: %s"), *Slot, Reason);
        if (!Quiet) Notify(Reason, true);
        return false;
    }
    UHomesteadSave* Save = Cast<UHomesteadSave>(UGameplayStatics::CreateSaveGameObject(UHomesteadSave::StaticClass()));
    if (!Save) { Notify(TEXT("Could not create a save record."), true); return false; }
    Save->WorldId = WorldId;
    Save->HairStyle = Appearance.HairStyle; Save->MetaHair = Appearance.MetaHair;
    Save->HairColor = Appearance.HairColor;
    Save->SkinTone = Appearance.SkinTone;
    Save->EyeColor = Appearance.EyeColor;
    Save->TunicColor = Appearance.TunicColor;
    Save->Outfit = Appearance.Outfit;
    Save->BodyPreset = Appearance.BodyPreset;
    Save->SimulationData = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    if (Save->SimulationData.IsEmpty())
    { Notify(TEXT("World serialization failed; previous saves are untouched."), true); return false; }
    Save->PlayerLocation = GetPawn() ? GetPawn()->GetActorLocation() : PendingLocation;
    const auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    Save->ViewRotation = Avatar ? Avatar->GameplayViewRotation() : GetControlRotation();
    Save->SavedAtUtc = FDateTime::UtcNow().ToUnixTimestamp();
    Save->SavedRevision = static_cast<int64>(Sim.GetRevision());
    Save->CameraSensitivity = Sensitivity;
    Save->InvertCameraY = bInvertY;
    Save->MusicVolume = MusicVolume;
    Save->AmbienceVolume = AmbienceVolume;
    Save->EffectsVolume = EffectsVolume;
    // Written for reference only: the row itself is saved with the simulation (layout 4 on).
    Save->HotbarSlots.Init(-1, Homestead::PackRowSize);
    for (int32 Cell = 0; Cell < Homestead::PackRowSize; ++Cell)
        if (const auto Item = HotbarItem(Cell); Item != Homestead::Item::Count) Save->HotbarSlots[Cell] = static_cast<int32>(Item);
    Save->SelectedHotbarSlot = SelectedHotbarSlot;
    Save->HotbarLayout = UHomesteadSave::CurrentHotbarLayout;
    Save->SaveLabel = CurrentSaveLabel();
    TArray<uint8> Data;
    const FString Path = SavePath(Slot);
    const FString Temporary = Path + TEXT(".tmp");
    if (!UGameplayStatics::SaveGameToMemory(Save, Data))
    {
        Notify(TEXT("The game could not serialize this save. Previous saves are untouched."), true);
        return false;
    }
    TArray<uint8> Envelope;
    Envelope.SetNumUninitialized(Data.Num() + 12);
    const char Magic[] = "HOMESAV1";
    const uint32 Checksum = FCrc::MemCrc32(Data.GetData(), Data.Num());
    FMemory::Memcpy(Envelope.GetData(), Magic, 8);
    FMemory::Memcpy(Envelope.GetData() + 8, &Checksum, sizeof(Checksum));
    FMemory::Memcpy(Envelope.GetData() + 12, Data.GetData(), Data.Num());
    if (!IFileManager::Get().MakeDirectory(*FPaths::GetPath(Path), true)
        || !FFileHelper::SaveArrayToFile(Envelope, *Temporary)
        || !ReadSave(Temporary))
    {
        Notify(TEXT("Save failed. Existing saves were not replaced; check disk space and permissions."), true);
        return false;
    }
    if (IFileManager::Get().FileExists(*Path)
        && IFileManager::Get().Copy(*(Path + TEXT(".bak")), *Path, true, true) != COPY_OK)
    {
        Notify(TEXT("Could not back up the previous save. It has not been replaced."), true);
        return false;
    }
    if (!IFileManager::Get().Move(*Path, *Temporary, true, true, false, true))
    {
        Notify(TEXT("Could not finish saving. The previous backup is retained."), true);
        return false;
    }
    if (!Quiet) Notify(TEXT("Your homestead is saved."));
    LastSuccessfulSave = FDateTime::UtcNow();
    LatestSaveLabel = Save->SaveLabel;
    return true;
}

bool AHomesteadController::ApplySave(const UHomesteadSave& Save)
{
    Homestead::Simulation Candidate = Sim;
    const auto Result = Candidate.Deserialize(TCHAR_TO_UTF8(*Save.SimulationData));
    if (!Result) { Notify(Result); return false; }
    const auto Region = Candidate.SetActiveWorldRegion({Save.PlayerLocation.X, Save.PlayerLocation.Y});
    if (!Region) { Notify(Region); return false; }
    FHomesteadAppearance Look;
    Look.HairStyle = Save.HairStyle; Look.MetaHair = Save.MetaHair >= 0 ? Save.MetaHair : HomesteadLook::MetaHairForLegacy(Save.HairStyle); Look.HairColor = Save.HairColor;
    Look.SkinTone = Save.SkinTone; Look.EyeColor = Save.EyeColor;
    Look.TunicColor = Save.TunicColor; Look.Outfit = Save.Outfit; Look.BodyPreset = Save.BodyPreset;
    auto* Avatar = Cast<AHomesteadCharacter>(GetPawn());
    FString Error;
    if (Avatar && !Avatar->PrepareEquipment(Candidate.GetState(), Look, Error))
    {
        LoadProblem = TEXT("This save is valid, but its clothing content is unavailable. Nothing was loaded. ") + Error;
        bTestResetRequired = !bHasPlayableSession;
        Notify(LoadProblem, true);
        return false;
    }
    if (!Landscape || !Landscape->Refresh(Candidate))
    {
        bWorldReady = false;
        if (Avatar) Avatar->ClearPreparedEquipment();
        Notify(TEXT("Saved woodland terrain could not be prepared. The save was not applied."), true);
        return false;
    }
    const Homestead::Simulation Previous = Sim;
    Sim = MoveTemp(Candidate);
    ActiveChestId.Reset();
    if (Avatar && !Avatar->ApplyPreparedEquipment(Error))
    {
        Sim = Previous;
        bWorldReady = Landscape->Refresh(Sim);
        Avatar->ClearPreparedEquipment();
        LoadProblem = TEXT("Save loading was canceled because its prepared appearance could not be displayed. ") + Error;
        bTestResetRequired = !bHasPlayableSession;
        Notify(LoadProblem, true);
        return false;
    }
    bTestResetRequired = false;
    PendingTravelNotices.Reset();
    bWorldReady = true;
    bHasPlayableSession = true;
    LoadProblem.Reset();
    if (Avatar)
    {
        Avatar->CancelAction(true);
        Avatar->ResetSprint();
    }
    WorldId = Save.WorldId;
    LastSuccessfulSave = FDateTime::FromUnixTimestamp(Save.SavedAtUtc);
    LatestSaveLabel = Save.SaveLabel;
    Appearance.HairStyle = Save.HairStyle; Appearance.MetaHair = Save.MetaHair >= 0 ? Save.MetaHair : HomesteadLook::MetaHairForLegacy(Save.HairStyle);
    Appearance.HairColor = Save.HairColor;
    Appearance.SkinTone = Save.SkinTone;
    Appearance.EyeColor = Save.EyeColor;
    Appearance.TunicColor = Save.TunicColor;
    Appearance.Outfit = Save.Outfit;
    Appearance.BodyPreset = Save.BodyPreset;
    SanitizeHotbar(Save.HotbarSlots, Save.SelectedHotbarSlot, Save.HotbarLayout);
    EndGroundSnap();
    PendingLocation = Save.PlayerLocation;
    PendingRotation = Save.ViewRotation;
    bFreshTerrainSpawn = false;
    // A save from the old town site stands beyond the trimmed road's reach: wake her at the manor instead.
    if (Sim.GetState().fixedEstate && !Homestead::WithinTravelReachOfRoad({PendingLocation.X, PendingLocation.Y}))
        SetEstateSpawn();
    bPendingSpawn = true;
    bWasFailed = false;
    CaptureSessionCheckpoint(PendingLocation, PendingRotation);
    Music->SetVolumeMultiplier(MusicLevel());
    Ambience->SetVolumeMultiplier(AmbienceVolume);
    Creek->SetVolumeMultiplier(AmbienceVolume * CreekGain);
    EndPlacement();
    CloseBook();
    RefreshRemaining = 0;
    GrantPlaytestKit(false);
    return true;
}

void AHomesteadController::GrantPlaytestKit(bool bNewGame)
{
    const TCHAR* Command = FCommandLine::Get();
    for (const TCHAR* Automation : {TEXT("unattended"), TEXT("HomesteadSmokeTest"), TEXT("HomesteadVisualPlaytest"),
        TEXT("HomesteadShippingQA"), TEXT("HomesteadSaveAudit"), TEXT("HomesteadPreviewProfile")})
        if (FParse::Param(Command, Automation) || FString(Command).Contains(FString(TEXT("-")) + Automation + TEXT("=")))
            return;
    if (bSaveRoutingTestPending || !StartupProbeDirectory.IsEmpty()) return;
    // On the estate her first tools are hafted from salvage; handing them over would skip that.
    if (Sim.GetState().fixedEstate) return;
    const FVector Facing = PendingRotation.Vector();
    const auto Result = Sim.GrantStarterKit({PendingLocation.X, PendingLocation.Y}, {Facing.X, Facing.Y}, bNewGame);
    if (!Result)
    {
        UE_LOG(LogTemp, Warning, TEXT("Playtest kit was not granted: %s"), UTF8_TO_TCHAR(Result.message.c_str()));
        return;
    }
    // The tools arrive in the first empty hotbar cells (the row is the first row of her pack).
    if (Landscape) Landscape->Refresh(Sim);
    UE_LOG(LogTemp, Display, TEXT("Playtest kit granted (new game %d): %s"), bNewGame, UTF8_TO_TCHAR(Result.message.c_str()));
}

bool AHomesteadController::LoadLatest(bool RecoveryOnly)
{
    if (!bSaveRoutingReady) { Notify(TEXT("Save routing is unavailable. No save files were accessed."), true); return false; }
    const bool WoodlandRecovery = RecoveryOnly && !bEstateMap;
    const TArray<FString> Slots = WoodlandRecovery
        ? TArray<FString>{TEXT("Homestead_Recovery"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"),
            TEXT("Homestead_Auto_2"), TEXT("Homestead_Manual")}
        : TArray<FString>{TEXT("Homestead_Manual"), TEXT("Homestead_Auto_0"), TEXT("Homestead_Auto_1"),
            TEXT("Homestead_Auto_2"), TEXT("Homestead_Recovery")};
    UHomesteadSave* Best = nullptr;
    FHomesteadSavePreference BestPreference;
    bool Corrupt = false;
    bool Incompatible = false;
    bool Newer = false;
    TArray<FString> IncompatiblePaths;
    for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
    {
        const FString& Slot = Slots[SlotIndex];
        for (const FString& Suffix : { FString(), FString(TEXT(".bak")) })
        {
            const FString Path = SavePath(Slot) + Suffix;
            if (!IFileManager::Get().FileExists(*Path)) continue;
            UHomesteadSave* Save = ReadSave(Path);
            if (!Save)
            {
                Incompatible |= bReadIncompatible;
                // A newer build's save is treated like an unreadable one: kept in place, never autosaved over.
                Newer |= bReadNewer;
                Corrupt |= !bReadIncompatible;
                if (bReadIncompatible) IncompatiblePaths.Add(Path);
                UE_LOG(LogTemp, Warning, TEXT("Cannot read save: %s"), *Path);
                continue;
            }
            Homestead::Simulation Candidate;
            if (bEstateMap) PrepareEstateSimulation(Candidate);
            const auto Decoded = Candidate.Deserialize(TCHAR_TO_UTF8(*Save->SimulationData));
            if (!Decoded || Candidate.GetState().failed) continue;
            if (RecoveryOnly && Save->WorldId != WorldId) continue;
            if (WoodlandRecovery && (Candidate.GetState().hunger < 20 || Candidate.GetState().energy < 20)) continue;
            if (WoodlandRecovery && Slot == TEXT("Homestead_Recovery"))
            {
                if (!ApplySave(*Save)) return false;
                Notify(TEXT("Returned to your sheltered recovery checkpoint."));
                return true;
            }
            const FHomesteadSavePreference Preference{
                Save->SavedAtUtc, Save->SavedRevision, SlotIndex * 2 + (Suffix.IsEmpty() ? 0 : 1)};
            if (!Best || IsNewerHomesteadSave(Preference, BestPreference))
            {
                Best = Save;
                BestPreference = Preference;
            }
        }
    }
    if (Best)
    {
        if (!ApplySave(*Best)) return false;
        if (!StartupProbeDirectory.IsEmpty()) StartupProbeLoadedState = UTF8_TO_TCHAR(Sim.Serialize().c_str());
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadSaveAudit")) && GEngine && GEngine->GameViewport)
            UE_LOG(LogTemp, Display, TEXT("SAVE_LOAD_AUDIT world=%s simulation_md5=%s look=%d,%d,%d,%d,%d,%d,%d view_mode=%d shader_complexity=%d"),
                *WorldId, *FMD5::HashAnsiString(UTF8_TO_TCHAR(Sim.Serialize().c_str())),
                Appearance.HairStyle, Appearance.HairColor, Appearance.SkinTone, Appearance.EyeColor,
                Appearance.TunicColor, Appearance.Outfit, Appearance.BodyPreset,
                GEngine->GameViewport->ViewModeIndex, static_cast<int32>(GEngine->GameViewport->EngineShowFlags.ShaderComplexity));
        Notify(Corrupt ? TEXT("Recovered your latest valid save. An unreadable save was skipped; backups are retained.")
            : RecoveryOnly ? TEXT("Returned to your latest save.") : TEXT("Welcome back to your homestead."), Corrupt);
        return true;
    }
    if (bEstateMap && Incompatible && !Corrupt && !RecoveryOnly && !bHasPlayableSession)
    {
        // Saves from earlier test builds are set aside (never deleted) and a new game begins, rather
        // than holding her on a reset page.
        const FString Retired = FPaths::Combine(SaveRoute.Directory, TEXT("Retired"));
        for (const FString& Path : IncompatiblePaths)
            IFileManager::Get().Move(*FPaths::Combine(Retired, FPaths::GetCleanFilename(Path)), *Path, true, true);
        Notify(TEXT("Saves from earlier test builds can't be opened by this one, so a new game begins. The old files are kept in the Retired folder."), false);
        return false;
    }
    if (Corrupt || Incompatible)
    {
        LoadProblem = Incompatible && !Corrupt
            ? (bEstateMap
                ? TEXT("Saves from earlier test builds can't be opened by this one. Start a new game; the old files are kept.")
                : TEXT("These test saves use an incompatible version. Start a new seeded woodland to use this build; old files are retained."))
            : Newer
            ? TEXT("These saves come from a newer build of the game. Open them with that build; they're kept unchanged.")
            : TEXT("No usable save could be read. Data is corrupt or incompatible; nothing was loaded. You can retry loading or explicitly reset this test world.");
        Notify(LoadProblem, true);
        // Do not let a fresh startup silently autosave over an unsuccessful load.
        bTestResetRequired = !RecoveryOnly && !bHasPlayableSession;
    }
    return false;
}

void AHomesteadController::RetryCheckpoint()
{
    if (LoadLatest(true)) return;
    if (SessionWorld != WorldId)
    { Notify(TEXT("No checkpoint belongs to this world. You can start a new test woodland or quit from Settings."), true); return; }
    Homestead::Simulation Candidate = Sim;
    const auto Result = Candidate.Deserialize(TCHAR_TO_UTF8(*SessionCheckpoint));
    if (!Result) { Notify(Result); return; }
    const auto Region = Candidate.SetActiveWorldRegion({SessionLocation.X, SessionLocation.Y});
    if (!Region) { Notify(Region); return; }
    if (!Landscape->Refresh(Candidate)) { bWorldReady = false; Notify(TEXT("Checkpoint terrain could not be prepared."), true); return; }
    Sim = MoveTemp(Candidate);
    ActiveChestId.Reset();
    PendingTravelNotices.Reset();
    bWorldReady = true;
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn()))
    {
        Avatar->CancelAction(true);
        Avatar->ResetSprint();
    }
    Appearance = SessionAppearance;
    EndGroundSnap();
    PendingLocation = SessionLocation;
    PendingRotation = SessionRotation;
    bFreshTerrainSpawn = false;
    bPendingSpawn = true;
    bWasFailed = false;
    RefreshRemaining = 0;
    Notify(TEXT("Returned to this session's checkpoint. No usable recovery save was available."));
}

void AHomesteadController::CaptureSessionCheckpoint(FVector Location, FRotator Rotation)
{
    SessionCheckpoint = UTF8_TO_TCHAR(Sim.Serialize().c_str());
    SessionAppearance = Appearance;
    SessionWorld = WorldId;
    SessionLocation = Location;
    SessionRotation = Rotation;
}

void AHomesteadController::NewGame()
{
    Homestead::Simulation Candidate = Sim;
    const FGuid Seed = FGuid::NewGuid();
    const auto Result = bEstateMap
        ? Candidate.NewEstateGame(Homestead::ProvisionalEstateLayout(), Homestead::ProvisionalEstatePlacements())
        : Candidate.NewGame((static_cast<uint64>(Seed.A) << 32) | Seed.B);
    if (!Result) { Notify(Result); return; }
    if (!Landscape->Refresh(Candidate)) { bWorldReady = false; Notify(TEXT("The new woodland could not be prepared. Your current session is retained."), true); return; }
    Sim = MoveTemp(Candidate);
    ActiveChestId.Reset();
    PendingTravelNotices.Reset();
    bWorldReady = true;
    bTestResetRequired = false;
    bHasPlayableSession = true;
    LastSuccessfulSave = FDateTime();
    LoadProblem.Reset();
    Appearance = FHomesteadAppearance();
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadHeroineTrialVitruvian01")))
    {
        Appearance.HairStyle = 1; Appearance.MetaHair = HomesteadLook::MetaHairForLegacy(1);
    }
    ResetHotbar();
    ControlsHint.Restart();
    EndGroundSnap();
    if (auto* Avatar = Cast<AHomesteadCharacter>(GetPawn())) Avatar->ResetSprint();
    WorldId = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    PendingLocation = FVector(-1000, 0, 180);
    PendingRotation = FRotator(-15, 15, 0);
    bFreshTerrainSpawn = true;
    if (bEstateMap) SetEstateSpawn();
    CaptureSessionCheckpoint(PendingLocation, PendingRotation);
    bPendingSpawn = true;
    bWasFailed = false;
    RefreshRemaining = 0;
    AutosaveRemaining = AutosaveMinutes * 60.0f;
    EndPlacement();
    GrantPlaytestKit(true);
    if (bEstateMap) { BeginNewGameSetup(); return; }
    OpenBook(0);
    Notify(TEXT("A new seeded woodland. Choose where to build; previous save files are still available."));
}

void AHomesteadController::QuickSave()
{
    if (bAutomatedInputOnly) ++TestQuickSaves;
    if (!IsFailed()) SaveSlot(TEXT("Homestead_Manual"));
}

void AHomesteadController::QuickLoad()
{
    if (bAutomatedInputOnly) ++TestQuickLoads;
    if (!LoadLatest())
    {
        if (bTestResetRequired)
        {
            if (NativeMenu.IsValid()) NativeMenu->RequestTestResetPrompt();
            OpenBook(4);
        }
        else if (LoadProblem.IsEmpty()) Notify(TEXT("There is no usable save to load yet."), true);
    }
}

FString AHomesteadController::SavePath(const FString& Slot) const
{
    return FPaths::Combine(SaveRoute.Directory, Slot + TEXT(".sav"));
}

FString AHomesteadController::PreviewLabel() const
{
    return SaveRoute.Mode == TEXT("preview") ? TEXT("Preview: ") + SaveRoute.Profile + TEXT(" (isolated saves)") : FString();
}
