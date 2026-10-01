#include "../HomesteadSmokeTest.h"
#include "../HomesteadController.h"
#include "../HomesteadControllerConfig.h"
#include "../HomesteadControllerPreferences.h"
#include "../HomesteadCharacter.h"
#include "HomesteadMenuPortrait.h"
#include "../HomesteadSave.h"
#include "../HomesteadWorld.h"
#include "../HomesteadTestPaths.h"
#include "../Simulation/HomesteadPackRow.h"
#include "../Simulation/HomesteadBackpack.h"
#include "../Simulation/HomesteadFood.h"
#include "../Simulation/HomesteadItems.h"
#include "../Simulation/HomesteadShops.h"
#include "../Simulation/HomesteadEstatePublicRoad.h"
#include "../Simulation/HomesteadTravel.h"
#include "SHomesteadShop.h"
#include "SHomesteadMenu.h"
#include "SHomesteadMapView.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Framework/Application/SlateApplication.h"
#include "Materials/MaterialInterface.h"
#include "CoreGlobals.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformProcess.h"
#include "Engine/SkeletalMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#endif

namespace HomesteadNativeMenuProof
{
bool OrdinaryLocalPath(FString Path, bool Directory)
{
#if PLATFORM_WINDOWS
    FPaths::NormalizeFilename(Path);
    if (Path.Len() < 3 || !FChar::IsAlpha(Path[0]) || Path[1] != ':' || Path[2] != '/'
        || Path.Mid(2).Contains(TEXT(":"))) return false;
    const FString Root = Path.Left(3).Replace(TEXT("/"), TEXT("\\"));
    if (::GetDriveTypeW(*Root) != DRIVE_FIXED) return false;
    for (;;)
    {
        const FString Native = Path.Replace(TEXT("/"), TEXT("\\"));
        const DWORD Attributes = ::GetFileAttributesW(*Native);
        if (Attributes == INVALID_FILE_ATTRIBUTES || (Attributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE))
            || ((Attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) != Directory) return false;
        FString Parent = FPaths::GetPath(Path);
        if (Parent.Len() == 2 && Parent[1] == ':') Parent += TEXT("/");
        if (Parent.IsEmpty() || Parent == Path) return true;
        Path = Parent;
        Directory = true;
    }
#else
    return false;
#endif
}

bool SameLook(const FHomesteadAppearance& A, const FHomesteadAppearance& B)
{
    return A.BodyPreset == B.BodyPreset && A.HairStyle == B.HairStyle && A.HairColor == B.HairColor
        && A.SkinTone == B.SkinTone && A.EyeColor == B.EyeColor && A.TunicColor == B.TunicColor && A.Outfit == B.Outfit;
}

FString MeshPath(const FHomesteadAppearance& Look, const TCHAR* Suffix)
{
    const TCHAR* Bodies[] = {TEXT("Preferred"), TEXT("Willow"), TEXT("Hazel")};
    const FString Name = FString::Printf(TEXT("SK_Modular_%s_%s"), Bodies[Look.BodyPreset], Suffix);
    return FString::Printf(TEXT("/Game/SurvivalGame/Characters/ModularClothing/%s/%s.%s"),
        Bodies[Look.BodyPreset], *Name, *Name);
}

struct FSaveFixture
{
    FString Simulation;
    FString World;
    FHomesteadAppearance Look;
    FString Fingerprint;
    uint32 ProducerProcess = 0;
};

const TCHAR* LookKeys[] = {TEXT("body"), TEXT("hair"), TEXT("hairColor"), TEXT("skin"), TEXT("eyes"), TEXT("legacyDye"), TEXT("legacyOutfit")};

TSharedRef<FJsonObject> FixtureJson(const FSaveFixture& Fixture)
{
    auto Object = MakeShared<FJsonObject>();
    Object->SetStringField(TEXT("route"), TEXT("HomesteadNativeMenuTest"));
    Object->SetNumberField(TEXT("schema"), 1);
    Object->SetNumberField(TEXT("producerProcess"), Fixture.ProducerProcess);
    Object->SetStringField(TEXT("world"), Fixture.World);
    Object->SetStringField(TEXT("simulation"), Fixture.Simulation);
    // Byte fingerprint only; the real save envelope/ownership validators remain authoritative.
    Object->SetStringField(TEXT("saveMd5"), Fixture.Fingerprint);
    const int32 Values[] = {Fixture.Look.BodyPreset, Fixture.Look.HairStyle, Fixture.Look.HairColor,
        Fixture.Look.SkinTone, Fixture.Look.EyeColor, Fixture.Look.TunicColor, Fixture.Look.Outfit};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Values); ++Index) Object->SetNumberField(LookKeys[Index], Values[Index]);
    return Object;
}

bool ReadFixture(const FString& Directory, FSaveFixture& Fixture, TArray<uint8>& Bytes, FString& Error)
{
    FString Normalized = Directory;
    FPaths::NormalizeDirectoryName(Normalized);
    FPaths::CollapseRelativeDirectories(Normalized);
    FString Consumer = HomesteadTestOutputDirectory();
    FPaths::NormalizeDirectoryName(Consumer);
    const FString Segment(TEXT("/Saved/Automation/"));
    const int32 Automation = Consumer.Find(Segment, ESearchCase::IgnoreCase);
    const FString AllowedRoot = Automation >= 0 ? Consumer.Left(Automation + Segment.Len()) : FString();
    if (FPaths::IsRelative(Directory) || AllowedRoot.IsEmpty()
        || !FPaths::IsUnderDirectory(Normalized, AllowedRoot)
        || FPaths::IsSamePath(Normalized, Consumer) || !OrdinaryLocalPath(Normalized, true))
    { Error = TEXT("Resume requires a different absolute producer directory under Saved/Automation."); return false; }
    const FString ManifestPath = FPaths::Combine(Normalized, TEXT("native-wardrobe-fixture.json"));
    const FString SavePath = FPaths::Combine(Normalized, TEXT("native-wardrobe-fixture.sav"));
    if (!OrdinaryLocalPath(ManifestPath, false) || !OrdinaryLocalPath(SavePath, false))
    { Error = TEXT("Resume accepts only ordinary local fixture files with no reparse/device ancestors."); return false; }
    if (IFileManager::Get().FileSize(*ManifestPath) > 8 * 1024 * 1024
        || IFileManager::Get().FileSize(*SavePath) > 4 * 1024 * 1024)
    { Error = TEXT("The explicit resume fixture exceeds the existing save bounds."); return false; }
    FString Text;
    TSharedPtr<FJsonObject> Object;
    if (!FFileHelper::LoadFileToString(Text, *ManifestPath)
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Object) || !Object.IsValid()
        || !FFileHelper::LoadFileToArray(Bytes, *SavePath))
    { Error = TEXT("Could not read the producer's explicit wardrobe fixture and manifest."); return false; }
    FString Route;
    double Schema = 0, Process = 0;
    if (!Object->TryGetStringField(TEXT("route"), Route) || Route != TEXT("HomesteadNativeMenuTest")
        || !Object->TryGetNumberField(TEXT("schema"), Schema) || Schema != 1
        || !Object->TryGetNumberField(TEXT("producerProcess"), Process) || !FMath::IsFinite(Process)
        || Process < 1 || Process > MAX_uint32 || FMath::FloorToDouble(Process) != Process
        || static_cast<uint32>(Process) == FPlatformProcess::GetCurrentProcessId()
        || !Object->TryGetStringField(TEXT("world"), Fixture.World)
        || !Object->TryGetStringField(TEXT("simulation"), Fixture.Simulation)
        || !Object->TryGetStringField(TEXT("saveMd5"), Fixture.Fingerprint)
        || Fixture.Fingerprint != FMD5::HashBytes(Bytes.GetData(), Bytes.Num()))
    { Error = TEXT("Resume fixture provenance, distinct-process identity or saved bytes do not match."); return false; }
    Fixture.ProducerProcess = static_cast<uint32>(Process);
    int32* Fields[] = {&Fixture.Look.BodyPreset, &Fixture.Look.HairStyle, &Fixture.Look.HairColor,
        &Fixture.Look.SkinTone, &Fixture.Look.EyeColor, &Fixture.Look.TunicColor, &Fixture.Look.Outfit};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Fields); ++Index)
    {
        double Value = 0;
        if (!Object->TryGetNumberField(LookKeys[Index], Value) || !FMath::IsFinite(Value)
            || Value < 0 || Value > MAX_int32 || FMath::FloorToDouble(Value) != Value)
        { Error = TEXT("Resume fixture appearance fields are invalid."); return false; }
        *Fields[Index] = static_cast<int32>(Value);
    }
    Homestead::Simulation Parsed;
    FGuid World;
    if (!Fixture.Look.IsValid() || !FGuid::Parse(Fixture.World, World) || !World.IsValid()
        || !Parsed.Deserialize(TCHAR_TO_UTF8(*Fixture.Simulation)))
    { Error = TEXT("Resume fixture expected world/appearance/current simulation is invalid."); return false; }
    return true;
}

bool WriteEnvelope(UHomesteadSave& Save, const FString& Path)
{
    TArray<uint8> Data;
    if (!UGameplayStatics::SaveGameToMemory(&Save, Data)) return false;
    TArray<uint8> Envelope;
    Envelope.SetNumUninitialized(Data.Num() + 12);
    const char Magic[] = "HOMESAV1";
    const uint32 Checksum = FCrc::MemCrc32(Data.GetData(), Data.Num());
    FMemory::Memcpy(Envelope.GetData(), Magic, 8);
    FMemory::Memcpy(Envelope.GetData() + 8, &Checksum, sizeof(Checksum));
    FMemory::Memcpy(Envelope.GetData() + 12, Data.GetData(), Data.Num());
    return FFileHelper::SaveArrayToFile(Envelope, *Path);
}
}

bool AHomesteadSmokeTest::VerifyNativeMenuPresentation() const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    if (!Avatar) return false;
    const auto* Presentation = Avatar->GetEquipmentPresentation();
    const auto& Look = Controller->GetAppearance();
    if (!Presentation || !Presentation->Ready || !Look.IsValid()
        || !HomesteadNativeMenuProof::SameLook(Presentation->Appearance, Look)) return false;
    const TCHAR* BaseSuffixes[] = {TEXT("Base_LongWave"), TEXT("Base_Bob"), TEXT("Base_Ponytail")};
    if (!Presentation->Base.Mesh
        || Presentation->Base.Mesh->GetPathName() != HomesteadNativeMenuProof::MeshPath(Look, BaseSuffixes[Look.HairStyle])) return false;
    const auto* Body = Avatar->GetMesh();
    if (!Body || !Body->IsVisible() || Body->GetSkeletalMeshAsset() != Presentation->Base.Mesh) return false;
    const auto Matches = [](const USkeletalMeshComponent& Component, const FHomesteadEquipmentSurface& Surface)
    {
        if (Component.GetNumMaterials() != Surface.Materials.Num()) return false;
        for (int32 Index = 0; Index < Surface.Materials.Num(); ++Index)
            if (!Surface.Materials[Index] || Component.GetMaterial(Index) != Surface.Materials[Index]
                || !Surface.Materials[Index]->GetShadingModels().HasShadingModel(MSM_DefaultLit)) return false;
        return true;
    };
    if (!Matches(*Body, Presentation->Base)) return false;
    bool SkinFound = false, EyesFound = false, HairFound = false;
    const auto& BaseSlots = Presentation->Base.Mesh->GetMaterials();
    if (Presentation->Base.Materials.Num() != BaseSlots.Num()) return false;
    for (int32 Index = 0; Index < BaseSlots.Num(); ++Index)
    {
        const FName Name = BaseSlots[Index].MaterialSlotName;
        TOptional<FLinearColor> Wanted;
        FName Parameter(TEXT("ColorTint"));
        if (Name == TEXT("M_Heroine_Skin")) { Wanted = HomesteadLook::SkinTint(Look.SkinTone); SkinFound = true; }
        else if (Name == TEXT("M_Heroine_LightEyes"))
        {
            Wanted = HomesteadLook::IrisColor(Look.EyeColor); Parameter = TEXT("IrisColor"); EyesFound = true;
            float Mix = -1;
            if (!Presentation->Base.Materials[Index]->GetScalarParameterValue(FMaterialParameterInfo(TEXT("IrisMix")), Mix)
                || !FMath::IsNearlyEqual(Mix, Look.EyeColor == 0 ? 0.0f : 1.0f, 0.001f)) return false;
        }
        else if (Name.ToString().StartsWith(TEXT("M_Heroine_Hair_")))
        {
            const bool Neutral = Name == TEXT("M_Heroine_Hair_long01_Neutral") || Name == TEXT("M_Heroine_Hair_bob01_Neutral");
            Wanted = Neutral ? HomesteadLook::NeutralHairTint(Look.HairColor) : HomesteadLook::HairTint(Look.HairColor);
            HairFound = true;
        }
        else if (Name == TEXT("M_Heroine_Eyebrows")) Wanted = HomesteadLook::HairTint(Look.HairColor);
        if (Wanted.IsSet())
        {
            FLinearColor Actual;
            if (!Presentation->Base.Materials[Index]->GetVectorParameterValue(FMaterialParameterInfo(Parameter), Actual)
                || !Actual.Equals(Wanted.GetValue(), 0.001f)) return false;
        }
    }
    if (!SkinFound || !EyesFound || !HairFound) return false;
    TArray<USkeletalMeshComponent*> Components;
    Avatar->GetComponents(Components);
    int32 VisibleComponents = 0;
    for (const auto* Component : Components) if (Component->IsVisible() && Component->GetSkeletalMeshAsset()) ++VisibleComponents;
    if (VisibleComponents != Presentation->Garments.Num() + 1) return false;
    TSet<int32> RenderedIds;
    for (const auto& Surface : Presentation->Garments)
    {
        const auto* Owned = Controller->Simulation().GetWearable(Surface.WearableId);
        if (!Owned || Owned->owner != Homestead::WearableOwner::Equipped || RenderedIds.Contains(Owned->id)
            || Surface.Definition != static_cast<int32>(Owned->definition) || Surface.Dye != Owned->dye
            || Surface.Slot < 0 || Surface.Slot >= Homestead::EquipmentSlotCount
            || Controller->State().equipment[Surface.Slot] != Owned->id || !Surface.Mesh) return false;
        const TCHAR* Suffixes[] = {TEXT("Tunic"), TEXT("Apron"), TEXT("Shoes"), TEXT("Footwraps")};
        if (Surface.Definition < 0 || Surface.Definition >= UE_ARRAY_COUNT(Suffixes)
            || Surface.Mesh->GetPathName() != HomesteadNativeMenuProof::MeshPath(Look, Suffixes[Surface.Definition])) return false;
        RenderedIds.Add(Owned->id);
        bool Found = false;
        for (const auto* Component : Components)
            if (Component != Body && Component->IsVisible() && Component->GetSkeletalMeshAsset() == Surface.Mesh
                && Matches(*Component, Surface)) Found = true;
        if (!Found) return false;
        const auto& Slots = Surface.Mesh->GetMaterials();
        for (int32 Index = 0; Index < Slots.Num(); ++Index)
        {
            if (Slots[Index].MaterialSlotName != TEXT("M_Heroine_MossLinen")
                && Slots[Index].MaterialSlotName != TEXT("M_Heroine_ApronLinen")) continue;
            FLinearColor Actual;
            if (!Surface.Materials.IsValidIndex(Index) || !Surface.Materials[Index]
                || !Surface.Materials[Index]->GetVectorParameterValue(FMaterialParameterInfo(TEXT("ColorTint")), Actual)
                || !Actual.Equals(HomesteadLook::TunicTint(Owned->dye), 0.001f)) return false;
        }
    }
    for (const auto& Owned : Controller->State().wearables)
        if (Owned.owner == Homestead::WearableOwner::Equipped && !RenderedIds.Contains(Owned.id)) return false;
    return true;
}

void AHomesteadSmokeTest::PrepareNativeMenuChecks()
{
    const auto Before = MakeShared<std::string>();
    const auto PortraitWorld = MakeShared<std::string>();
    const auto PortraitPhase = MakeShared<float>(-1);
    const auto OriginalRoute = MakeShared<FString>();
    const auto Blocker = MakeShared<FString>(FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("native-menu-write-blocker")));
    Add(TEXT("Required modular content is actually prepared and rendered; no prototype fallback"),
        []() {}, [this]() { return VerifyNativeMenuPresentation(); });
    FString ProducerOutput;
    bool ResumeRequested = false;
    const TCHAR* Cursor = FCommandLine::Get();
    FString Token;
    const FString ResumeName(TEXT("-HomesteadNativeResumeFrom"));
    const FString ResumePrefix = ResumeName + TEXT("=");
    while (FParse::Token(Cursor, Token, false))
    {
        if (!Token.StartsWith(ResumeName, ESearchCase::IgnoreCase)) continue;
        if (ResumeRequested || !Token.StartsWith(ResumePrefix, ESearchCase::IgnoreCase) || Token.Len() == ResumePrefix.Len())
        { Finish(false, TEXT("Supply exactly one nonempty -HomesteadNativeResumeFrom=<producer output>.")); return; }
        ResumeRequested = true;
        ProducerOutput = Token.Mid(ResumePrefix.Len());
        // FParse::Token preserves quotes embedded after an option's equals sign.
        if (ProducerOutput.StartsWith(TEXT("\"")) && ProducerOutput.EndsWith(TEXT("\"")) && ProducerOutput.Len() >= 2)
            ProducerOutput = ProducerOutput.Mid(1, ProducerOutput.Len() - 2);
        if (ProducerOutput.IsEmpty() || ProducerOutput.Contains(TEXT("\"")))
        { Finish(false, TEXT("Resume requires one nonempty, correctly quoted producer directory.")); return; }
    }

    if (ResumeRequested)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadNativeQuitTest")))
        { Finish(false, TEXT("Resume and quit verification require separate fresh processes.")); return; }
        PrepareNativeResumeChecks(ProducerOutput);
        return;
    }
    Add(TEXT("Close initial guide through mapped Back"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    const auto HintLeft = MakeShared<double>(0.0);
    Add(TEXT("The first-minute controls strip counts down while it is on screen"),
        [this, HintLeft]() { *HintLeft = Controller->ControlsHintSecondsLeft(); },
        [this, HintLeft]() { return Controller->IsControlsHintOnScreen() && Controller->ControlsHintSecondsLeft() < *HintLeft - 0.5; }, 1.0f);
    Add(TEXT("First exit-path press opens Settings"),
        [this]() { Tap(EKeys::Escape); PausedHour = Controller->State().hour; },
        [this]() { return Controller->HasNativeMenu() && Controller->BookPage() == 4; });
    Add(TEXT("The controls strip's minute is frozen while the book is open"),
        [this, HintLeft]() { *HintLeft = Controller->ControlsHintSecondsLeft(); },
        [this, HintLeft]() { return !Controller->IsControlsHintOnScreen() && Controller->ControlsHintSecondsLeft() == *HintLeft; }, 1.0f);
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadNativeSaveRetryTest")))
    {
        Add(TEXT("Prepare an existing valid manual save before retry fixture"),
            [this]() { Tap(EKeys::F5); },
            [this]() { return Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual"))) != nullptr; });
        Add(TEXT("Open exit confirmation for save-failure retry"),
            [this]() { Controller->NativeMenu->FocusLegacySubject(9); Tap(EKeys::Enter); },
            [this]() { return Controller->NativeMenu->IsExitPrompt(); });
        Add(TEXT("Owned temporary-path failure keeps process open and paused"),
            [this, Before, OriginalRoute, Blocker]()
            {
                *Before = Controller->Simulation().Serialize();
                *OriginalRoute = Controller->SaveRoute.Directory;
                FFileHelper::SaveStringToFile(TEXT("owned retry blocker"), **Blocker);
                Controller->SaveRoute.Directory = *Blocker;
                Tap(EKeys::Enter);
            },
            [this, Before]() { return Controller->NativeMenu->IsExitPrompt() && Controller->ToastIsError()
                && Controller->Simulation().Serialize() == *Before && !IsEngineExitRequested(); });
        Add(TEXT("Explicit Retry succeeds, saves exact current state, then requests exit"),
            [this, Before, OriginalRoute, Blocker]()
            {
                Controller->SaveRoute.Directory = *OriginalRoute;
                if (!IFileManager::Get().Delete(**Blocker, false, true))
                { Finish(false, TEXT("Could not remove owned retry blocker.")); return; }
                Tap(EKeys::Enter);
                const auto* Saved = Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual")));
                const bool Passed = Saved && Saved->SimulationData == UTF8_TO_TCHAR(Before->c_str())
                    && IsEngineExitRequested();
                Finish(Passed, Passed ? TEXT("Save failure retry durably replaced the manual save before exit.")
                    : TEXT("Retry did not verify both durable save and exit request."));
            }, []() { return true; }, 0);
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadNativeQuitTest")))
    {
        Add(TEXT("Reach the real save-and-quit confirmation"),
            [this]() { Controller->NativeMenu->FocusLegacySubject(9); Tap(EKeys::Enter); },
            [this]() { return Controller->NativeMenu && Controller->NativeMenu->IsExitPrompt(); });
        Add(TEXT("Successful save precedes actual engine exit request"),
            [this, Before]()
            {
                *Before = Controller->Simulation().Serialize();
                Tap(EKeys::Enter);
                const auto* SavedGame = Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual")));
                const bool Verified = SavedGame && SavedGame->SimulationData == UTF8_TO_TCHAR(Before->c_str())
                    && !Controller->ToastIsError() && IsEngineExitRequested();
                Finish(Verified, Verified ? TEXT("Native save-and-quit saved current state and requested process exit.")
                    : TEXT("Native save-and-quit did not verify both saved state and the exit request."));
            },
            []() { return true; }, 0);
        return;
    }
    auto Capture = [this](const FString& Name)
    {
        Add(TEXT("Capture native Slate menu: ") + Name, [this, Name]() { Screenshot(Name); },
            [this]() { return Controller->NativeMenu.IsValid(); }, 0.8f);
    };
    Capture(TEXT("native-settings"));
    const auto DragAudioSlider = [this](int32 AudioId, float FromFraction, float ToFraction,
        bool ReleaseOutside, bool& StayedOpen)
    {
        const auto Widget = Controller->NativeMenu->GetAudioSliderWidget(AudioId);
        if (!Widget)
        {
            Finish(false, TEXT("Native Settings audio slider is missing."));
            return;
        }
        const FGeometry Geometry = Widget->GetCachedGeometry();
        if (Geometry.GetAbsoluteSize().X < 40 || Geometry.GetAbsoluteSize().Y < 1)
        {
            Finish(false, TEXT("Native Settings audio slider has no hit geometry."));
            return;
        }
        const FVector2D From = Geometry.GetAbsolutePosition()
            + FVector2D(Geometry.GetAbsoluteSize().X * FromFraction, Geometry.GetAbsoluteSize().Y * 0.5f);
        FVector2D To = Geometry.GetAbsolutePosition()
            + FVector2D(Geometry.GetAbsoluteSize().X * ToFraction, Geometry.GetAbsoluteSize().Y * 0.5f);
        if (ReleaseOutside) To.Y -= 80;
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        Slate.SetCursorPos(From);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, From, From, TSet<FKey>(),
            EKeys::Invalid, 0, FModifierKeysState()));
        TSet<FKey> Pressed;
        Pressed.Add(EKeys::LeftMouseButton);
        Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, From, From, Pressed,
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
        StayedOpen = Controller->HasNativeMenu() && Controller->BookPage() == 4
            && Controller->IsBookOpen();
        Slate.SetCursorPos(To);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, To, From, Pressed,
            EKeys::Invalid, 0, FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, To, To, TSet<FKey>(),
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
    };
    for (const int32 AudioId : {16, 5, 6, 7})
    {
        const auto Stayed = MakeShared<bool>(false);
        const auto Prior = MakeShared<float>(0);
        const auto Target = MakeShared<float>(0);
        Add(FString::Printf(TEXT("Open the Sound tab at audio slider %d"), AudioId),
            [this, AudioId]() { Controller->NativeMenu->FocusLegacySubject(AudioId); },
            [this, AudioId]()
            {
                const auto Widget = Controller->NativeMenu->GetAudioSliderWidget(AudioId);
                return Widget && Widget->GetCachedGeometry().GetAbsoluteSize().X > 40
                    && Controller->NativeMenu->IsFocusedControlVisible();
            }, 0.2f);
        Add(FString::Printf(TEXT("Pointer click/drag audio slider %d stays in Settings"), AudioId),
            [this, AudioId, Stayed, Prior, Target, DragAudioSlider]()
            {
                *Prior = Controller->MenuAudioVolume(AudioId);
                // Volumes persist in GameUserSettings between runs, so aim away from the saved level.
                const float Preferred = AudioId == 16 ? 0.62f : AudioId == 5 ? 0.26f : AudioId == 6 ? 0.42f : 0.58f;
                *Target = FMath::Abs(Preferred - *Prior) > 0.15f ? Preferred : Preferred + 0.3f;
                DragAudioSlider(AudioId, AudioId == 5 ? *Target : (*Target > 0.7f ? 0.1f : 0.8f),
                    *Target, AudioId != 5, *Stayed);
            },
            [this, AudioId, Stayed, Prior, Target]()
            {
                const bool Valid = *Stayed && Controller->HasNativeMenu()
                    && Controller->IsBookOpen() && Controller->BookPage() == 4
                    && !Controller->NativeMenu->HasActiveDialog()
                    && FMath::Abs(Controller->MenuAudioVolume(AudioId) - *Target) < 0.09f
                    && FMath::Abs(Controller->MenuAudioVolume(AudioId) - *Prior) > 0.09f;
                return Valid;
            }, 0.2f);
    }
    // Stepping a sound slider with the d-pad or keys: heard live, saved once when she moves on,
    // and Back puts it back unsaved.
    const auto StepStart = MakeShared<float>(0);
    const auto StepWrites = MakeShared<int32>(0);
    const auto StepDirection = MakeShared<int32>(1);
    Add(TEXT("Focus the Music slider for keyboard steps"),
        [this]() { Controller->NativeMenu->FocusLegacySubject(5); },
        [this]() { return Controller->NativeMenu->IsFocusedControlVisible(); }, 0.2f);
    Add(TEXT("Keyboard steps change Music live without saving each step"),
        [this, StepStart, StepWrites, StepDirection]()
        {
            *StepStart = Controller->MenuAudioVolume(5);
            *StepWrites = Controller->AudioPersistWrites;
            *StepDirection = *StepStart > 0.5f ? -1 : 1;
            for (int32 Step = 0; Step < 3; ++Step) Tap(*StepDirection > 0 ? EKeys::Right : EKeys::Left);
        },
        [this, StepStart, StepWrites, StepDirection]()
        {
            return FMath::IsNearlyEqual(Controller->MusicVolume, *StepStart + *StepDirection * 0.15f, 0.001f)
                && Controller->AudioPersistWrites == *StepWrites && Controller->NativeMenu->HasPendingAudioStep();
        });
    Add(TEXT("Moving off the slider saves the stepped level once"),
        [this]() { Tap(EKeys::Up); },
        [this, StepStart, StepWrites, StepDirection]()
        {
            float Saved = -1;
            GConfig->GetFloat(HomesteadControllerPreferences::AudioSettingsSection,
                HomesteadControllerConfig::AudioKeys[HomesteadControllerConfig::AudioKeyIndex(5)], Saved, GGameUserSettingsIni);
            return !Controller->NativeMenu->HasPendingAudioStep() && Controller->AudioPersistWrites == *StepWrites + 1
                && FMath::IsNearlyEqual(Saved, *StepStart + *StepDirection * 0.15f, 0.001f)
                && FMath::IsNearlyEqual(Controller->MusicVolume, *StepStart + *StepDirection * 0.15f, 0.001f);
        });
    Add(TEXT("Back after a step puts Music back unsaved and keeps Settings open"),
        [this, StepWrites]()
        {
            Controller->NativeMenu->FocusLegacySubject(5);
            *StepWrites = Controller->AudioPersistWrites;
            Tap(EKeys::Left);
            Tap(EKeys::Escape);
        },
        [this, StepStart, StepWrites, StepDirection]()
        {
            return !Controller->NativeMenu->HasPendingAudioStep() && Controller->AudioPersistWrites == *StepWrites
                && FMath::IsNearlyEqual(Controller->MusicVolume, *StepStart + *StepDirection * 0.15f, 0.001f)
                && Controller->IsBookOpen() && Controller->BookPage() == 4;
        });
    Add(TEXT("Put Music back where it started"),
        [this, StepStart]() { Controller->MenuCommitAudioVolume(5, *StepStart, Controller->MenuAudioVolume(5)); },
        [this, StepStart]() { return FMath::IsNearlyEqual(Controller->MusicVolume, *StepStart, 0.001f); });
    Add(TEXT("Quit game row opens one two-choice dialog"),
        [this]() { Controller->NativeMenu->FocusLegacySubject(9); Tap(EKeys::Enter); },
        [this]() { return Controller->NativeMenu && Controller->NativeMenu->IsExitPrompt(); });
    Capture(TEXT("native-exit-confirm"));
    Add(TEXT("Exit confirmation keeps simulation paused"),
        []() {},
        [this]() { return FMath::IsNearlyEqual(Controller->State().hour, PausedHour, 1e-8); }, 1.0f);
    Add(TEXT("Back cancels the single quit dialog"),
        [this]() { Tap(EKeys::Escape); },
        [this]() { return Controller->IsBookOpen() && !Controller->NativeMenu->HasActiveDialog(); });
    Add(TEXT("Current-schema F5 writes a readable sandbox save"),
        [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError() && Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual"))) != nullptr; });
    Add(TEXT("Save status shows the save's local date and time in words, not an ISO UTC stamp"),
        []() {},
        [this]()
        {
            // The save was written moments ago, so it shows today's local date (en test culture).
            static const TCHAR* Months[] = {TEXT("January"), TEXT("February"), TEXT("March"), TEXT("April"), TEXT("May"),
                TEXT("June"), TEXT("July"), TEXT("August"), TEXT("September"), TEXT("October"), TEXT("November"), TEXT("December")};
            const FDateTime Local = FDateTime::Now();
            const FString Status = Controller->MenuSaveStatus();
            Results.Add(TEXT("SAVE_STATUS ") + Status.Replace(TEXT("\n"), TEXT(" | ")));
            return !Status.Contains(TEXT("UTC")) && !Status.Contains(TEXT("not known"))
                && !Status.Contains(FString::Printf(TEXT("%04d-%02d-%02d"), Local.GetYear(), Local.GetMonth(), Local.GetDay()))
                && Status.Contains(FString::Printf(TEXT("%s %d, %d"), Months[Local.GetMonth() - 1], Local.GetDay(), Local.GetYear()))
                && (Status.Contains(TEXT("AM")) || Status.Contains(TEXT("PM")));
        });
    Add(TEXT("A real IO error does not quit or change inventory"),
        [this, Before, OriginalRoute, Blocker]()
        {
            *Before = Controller->Simulation().Serialize();
            *OriginalRoute = Controller->SaveRoute.Directory;
            if (!FFileHelper::SaveStringToFile(TEXT("owned synthetic file prevents directory creation"), **Blocker))
            { Finish(false, TEXT("Could not prepare the owned save-failure fixture.")); return; }
            Controller->SaveRoute.Directory = *Blocker;
            Controller->NativeMenu->FocusLegacySubject(9);
            Tap(EKeys::Enter); Tap(EKeys::Enter);
        },
        [this, Before]() { return Controller->NativeMenu->IsExitPrompt() && Controller->ToastIsError()
            && Controller->Simulation().Serialize() == *Before; });
    Capture(TEXT("native-save-error"));
    Add(TEXT("Save failure stays actionable beyond toast expiry"),
        []() {},
        [this]() { return Controller->NativeMenu->IsExitPrompt() && Controller->Toast().IsEmpty(); }, 8.3f);
    Add(TEXT("Return from failure restores Settings without discarding progress"),
        [this, OriginalRoute, Blocker]()
        {
            Controller->SaveRoute.Directory = *OriginalRoute;
            if (!IFileManager::Get().Delete(**Blocker, false, true))
            { Finish(false, TEXT("Could not remove the owned save-failure fixture.")); return; }
            Tap(EKeys::Escape);
        },
        [this, Before]() { return !Controller->NativeMenu->HasActiveDialog()
            && Controller->Simulation().Serialize() == *Before; });
    // Disclosed fixture: the hotbar is her pack's first row (Simulation/HomesteadPackRow.h) and new
    // stacks normally fill it first. This suite exercises the pack grid, so new stock lands below the
    // row here; the row itself is covered by the chest-view hotbar block and the Hotbar suite.
    Add(TEXT("Disclosed fixture: new stock lands in the pack grid, below the hotbar row"),
        [this]() { Controller->Sim.SetPackRowAutoFill(false); },
        []() { return true; });
    QueueGrant(Homestead::Item::Billhook, 1);
    Add(TEXT("Controller tabs reach real carried inventory"),
        [this]() { Tap(EKeys::Escape); Tap(EKeys::I); },
        [this]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Controller->BookPage() == 0 && !Controller->UsesGamepad() && Subject
                && Subject->Subject == EHomesteadMenuSubject::ItemGroup
                && Subject->Id == static_cast<int>(Homestead::Item::Billhook) && Subject->Quantity == 1
                && Controller->NativeMenu->GetDisplayedDetails().Contains(TEXT("Carried: 1"));
        });
    Add(TEXT("Inventory portrait animates on its own while the simulation is paused"),
        [this, PortraitWorld, PortraitPhase]()
        {
            *PortraitWorld = Controller->Simulation().Serialize();
            const auto Phase = Controller->MenuPortrait
                ? Controller->MenuPortrait->IdlePhase() : TOptional<float>();
            if (!Phase.IsSet())
            {
                Finish(false, TEXT("Inventory portrait has no independent idle instance."));
                return;
            }
            *PortraitPhase = Phase.GetValue();
        },
        [this, PortraitWorld, PortraitPhase]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            const auto Phase = Controller->MenuPortrait
                ? Controller->MenuPortrait->IdlePhase() : TOptional<float>();
            const UAnimSequence* Idle = Avatar ? Avatar->GetIdleAnimation() : nullptr;
            if (!Phase.IsSet() || !Idle || *PortraitPhase < 0) return false;
            const float Duration = Idle->GetPlayLength();
            const float Advance = FMath::Fmod(Phase.GetValue() - *PortraitPhase + Duration, Duration);
            const bool Passed = Controller->IsBookOpen() && Controller->BookPage() == 0
                && Controller->Simulation().Serialize() == *PortraitWorld
                && Advance > 0.65f && Advance < 1.65f;
            if (!Passed)
                Results.Add(FString::Printf(TEXT("PORTRAIT_IDLE_DIAG book=%d page=%d start=%.3f now=%.3f duration=%.3f advance=%.3f same_world=%d"),
                    Controller->IsBookOpen(), Controller->BookPage(), *PortraitPhase,
                    Phase.GetValue(), Duration, Advance,
                    Controller->Simulation().Serialize() == *PortraitWorld));
            return Passed;
        }, FParse::Param(FCommandLine::Get(), TEXT("HomesteadIdleExtended")) ? 6.1f : 1.1f);
    Capture(TEXT("native-inventory"));
    PrepareNativeInventoryTransactionChecks();
    Add(TEXT("Mouse noise does not steal controller hints"),
        [this]() { Axis(EKeys::MouseX, 0.01f); },
        [this]() { return Controller->UsesGamepad(); });
    Add(TEXT("Keyboard tab shortcut opens actual recipes"),
        [this]()
        {
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftControl, IE_Pressed, 1));
            Tap(EKeys::Tab);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftControl, IE_Released, 0));
        },
        [this]() { return Controller->BookPage() == 1 && !Controller->UsesGamepad(); });
    Add(TEXT("Unavailable quick presses are atomic and expose structured requirements"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Enter); Tap(EKeys::Enter); },
        [this, Before]() { const FString Details = Controller->NativeMenu->GetDisplayedDetails();
            // The press explains itself in the book's notice, with the recipe's own reason, and
            // nothing changes.
            return Controller->ToastIsError() && Controller->Toast().StartsWith(TEXT("Gather "))
                && Controller->NativeMenu->GetNoticeText() == Controller->Toast()
                && Controller->Simulation().Serialize() == *Before
                && Details.Contains(TEXT("Branch: Have")) && Details.Contains(TEXT("/ Need 2"))
                && Details.Contains(TEXT("Rusted axe head: Have"))
                && Details.Contains(TEXT("Salvage piles around the manor")); });
    Capture(TEXT("native-crafting"));
    Add(TEXT("Mapped tab opens purpose-specific building plans"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { const auto Rows = Controller->Rows();
            // The action says what activating a plan does: a placement preview, nothing spent yet.
            return Controller->BookPage() == 2 && Controller->BookTitle() == TEXT("Building plans")
                && Rows.Num() > 1 && Rows[0].Action == TEXT("Choose a spot to build")
                && Rows.Last().Action == TEXT("Choose what to take down"); });
    Capture(TEXT("native-build"));
    Add(TEXT("Real building plan enters placement without charging"),
        [this, Before]()
        {
            *Before = Controller->Simulation().Serialize();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Before]() { return Controller->IsPlanning() && !Controller->IsBookOpen()
            && Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("Cancel placement and return to Settings"),
        [this]() { Tap(EKeys::Escape); Tap(EKeys::Escape); },
        [this]() { return !Controller->IsPlanning() && Controller->IsBookOpen() && Controller->BookPage() == 4; });
    // The Guidebook (page 3) is retired: its keys open nothing, a request for it opens the pack, and
    // the tabs run Inventory, Craft, Build, Map, Appearance.
    // The shop: food shows its Energy (a Meal also until when she'd be Well fed) and prices read as whole coins (Homestead::Food::EffectLabel,
    // Homestead::FormatMoney). A disclosed store is placed ahead of her at a known open hour.
    Add(TEXT("Shop Buy lists food with its Energy and prices in whole coins"),
        [this]()
        {
            Controller->CloseBook();
            Controller->HomesteadMorning(10.0f);
            Controller->HomesteadOpenStore();
            if (Controller->State().shops.empty()) { Finish(false, TEXT("The disclosed store was not placed.")); return; }
            Controller->OpenShopScreen(Controller->State().shops.front().id, false);
            if (Controller->ShopScreen.IsValid()) Controller->ShopScreen->SetTab(1);
        },
        [this]()
        {
            if (!Controller->ShopScreen.IsValid() || Controller->ShopScreen->IsSellTab() || Controller->ShopScreen->RowCount() < 2) return false;
            const FString Purse = UTF8_TO_TCHAR(Homestead::FormatMoney(Controller->State().money).c_str());
            const FString Pasty = UTF8_TO_TCHAR(Homestead::Food::EffectLabel(Controller->State(), Homestead::Item::Pasty).c_str());
            const FString Price = UTF8_TO_TCHAR(Homestead::FormatMoney(Homestead::BuyPrice(Homestead::Item::Pasty)).c_str());
            Results.Add(FString::Printf(TEXT("SHOP_LABELS purse=%s pasty=%s price=%s"), *Purse, *Pasty, *Price));
            return Purse.EndsWith(TEXT(" coins")) && !Purse.Contains(TEXT("$")) && (Controller->State().fixedEstate ? Pasty.StartsWith(UTF8_TO_TCHAR("+40 Energy \xC2\xB7 Well fed until ")) : Pasty == TEXT("+40 Energy"))
                && Price == TEXT("100 coins");
        }, 0.8f);
    Add(TEXT("Capture the shop's Buy page"), [this]() { Screenshot(TEXT("native-shop-buy")); },
        [this]() { return Controller->ShopScreen.IsValid(); }, 0.8f);
    // The leather backpack: a one-time upgrade row heading Buy. Bought once it doubles her pack and
    // leaves the list; the whole simulation is put back afterwards so later checks see a plain pack.
    const auto BeforeBackpack = MakeShared<Homestead::Simulation>();
    Add(TEXT("The backpack upgrade heads Buy and is refused when she can't afford it"),
        [this, BeforeBackpack]()
        {
            // A purse short of the price, whatever the fixture started with.
            if (Controller->State().money >= Homestead::Backpack::Price)
                Controller->Sim.GrantMoney(Homestead::Backpack::Price - 1 - Controller->State().money);
            Controller->ShopScreen->Refresh();
            *BeforeBackpack = Controller->Sim;
            Controller->ShopScreen->Choose(Controller->ShopScreen->GetSelection());
        },
        [this, BeforeBackpack]()
        {
            const auto& Shop = Controller->ShopScreen;
            return Shop.IsValid() && Shop->IsUpgradeRow(Shop->GetSelection()) && Shop->RowLabel(Shop->GetSelection()) == TEXT("Leather backpack")
                && !Shop->IsChoosingQuantity() && Shop->GetStatus().StartsWith(TEXT("That's 1,500 coins;"))
                && Controller->Simulation().Serialize() == BeforeBackpack->Serialize();
        });
    const auto BeforeCounter = MakeShared<Homestead::Point>();
    Add(TEXT("With 1,500 coins she buys it once: her pack holds 240 and the row is gone"),
        [this, BeforeCounter]()
        {
            // Trading needs her at the counter; she goes back afterwards.
            *BeforeCounter = Controller->PlayerPoint();
            const auto& Shop = Controller->State().shops.front();
            Teleport({Shop.counterX + std::cos(FMath::DegreesToRadians(Shop.counterYaw)) * 150.0,
                Shop.counterY + std::sin(FMath::DegreesToRadians(Shop.counterYaw)) * 150.0});
            Controller->Sim.GrantMoney(Homestead::Backpack::Price);
            Controller->ShopScreen->Refresh();
            Controller->ShopScreen->Choose(Controller->ShopScreen->GetSelection());
            Controller->ShopScreen->Confirm();
        },
        [this, BeforeBackpack]()
        {
            const auto& Shop = Controller->ShopScreen;
            bool bRowGone = true;
            for (int32 Index = 0; Index < Shop->RowCount(); ++Index) bRowGone &= !Shop->IsUpgradeRow(Index);
            return Controller->State().leatherBackpack && Controller->Sim.PackCapacity() == Homestead::MaxPackCapacity
                && Controller->State().money == BeforeBackpack->GetState().money && bRowGone
                && Controller->MenuInventorySummary().Contains(TEXT("/ 240"));
        });
    Add(TEXT("Put the simulation back as it was before the backpack"),
        [this, BeforeBackpack, BeforeCounter]()
        {
            Controller->Sim = *BeforeBackpack;
            Controller->ShopScreen->Refresh();
            Teleport(*BeforeCounter);
        },
        [this]() { return !Controller->State().leatherBackpack && Controller->Sim.PackCapacity() == Homestead::InventoryCapacity; });
    Add(TEXT("Leave the shop and return to Settings, as before the shop check"),
        [this]() { Controller->CloseShopScreen(); Controller->OpenBook(4); },
        [this]() { return !Controller->ShopScreen.IsValid() && Controller->IsBookOpen() && Controller->BookPage() == 4; });
    // Road signs: a one-way sign she can't walk from here says why and changes nothing (no book, no
    // pause); the two-way Gateway sign opens the centred confirm over the Map page. The sign is focused
    // directly (disclosed); wherever the fixture stands her, one of the one-way signs is refused.
    // With the book shut the clock runs, so "changes nothing" is her place, pack, purse and energy,
    // and no walk's worth of time (a refused walk passes none).
    const auto BeforeSign = MakeShared<Homestead::State>();
    const auto SignSpot = MakeShared<Homestead::Point>();
    const auto SignRefusal = MakeShared<FString>();
    Add(TEXT("A road sign whose one way is refused says why and leaves the book shut"),
        [this, BeforeSign, SignRefusal, SignSpot]()
        {
            Controller->CloseBook();
            const auto& Signs = Homestead::EstatePublicRoad().signs;
            int32 Refused = INDEX_NONE;
            for (int32 Index = 0; Index < static_cast<int32>(Signs.size()) && Refused == INDEX_NONE; ++Index)
            {
                const auto Ways = Homestead::RoadSignDestinations(Signs[Index].name);
                if (Ways.size() != 1) continue;
                if (!Controller->CanSetOut()) { Refused = Index; *SignRefusal = TEXT("You can't set out just now."); }
                else if (const auto Plan = Controller->MenuPlanTravel(Ways[0]); !Plan.ok)
                { Refused = Index; *SignRefusal = UTF8_TO_TCHAR(Plan.error.c_str()); }
            }
            if (Refused == INDEX_NONE) { Finish(false, TEXT("No one-way road sign is refused from the fixture's spot.")); return; }
            Results.Add(TEXT("SIGN_REFUSAL ") + *SignRefusal);
            *BeforeSign = Controller->State();
            *SignSpot = Controller->PlayerPoint();
            Controller->Focus = AHomesteadController::EFocus::RoadSign;
            Controller->FocusId = Refused;
            Controller->InteractWithRoadSign();
        },
        [this, BeforeSign, SignRefusal, SignSpot]()
        {
            const auto& Now = Controller->State();
            return !Controller->IsBookOpen() && Controller->ToastIsError() && Controller->Toast() == *SignRefusal
                && Now.inventory == BeforeSign->inventory && Now.money == BeforeSign->money
                && Now.energy <= BeforeSign->energy && Now.hour - BeforeSign->hour < 0.05
                && std::hypot(Controller->PlayerPoint().x - SignSpot->x, Controller->PlayerPoint().y - SignSpot->y) < 50.0;
        });
    Add(TEXT("The two-way Gateway sign opens the centred confirm over the Map page"),
        [this]()
        {
            const auto& Signs = Homestead::EstatePublicRoad().signs;
            for (int32 Index = 0; Index < static_cast<int32>(Signs.size()); ++Index)
                if (Homestead::RoadSignDestinations(Signs[Index].name).size() > 1)
                {
                    Controller->Focus = AHomesteadController::EFocus::RoadSign;
                    Controller->FocusId = Index;
                    Controller->InteractWithRoadSign();
                    return;
                }
            Finish(false, TEXT("No two-way road sign."));
        },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 7 && Controller->NativeMenu->IsTravelPromptOpen(); });
    Add(TEXT("Stay here, and back to Settings as before"),
        [this]() { Controller->CloseBook(); Controller->OpenBook(4); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 4 && !Controller->NativeMenu->IsTravelPromptOpen(); });
    Add(TEXT("The retired Guidebook has no G / H shortcut"),
        [this]() { Tap(EKeys::Escape); Tap(EKeys::G); Tap(EKeys::H); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("A request for the retired Guidebook page opens the pack instead"),
        [this]() { Controller->OpenBook(3); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Credits has no fabricated Read action"),
        [this]() { Controller->OpenBook(5); },
        [this]() { return Controller->BookPage() == 5
            && Controller->NativeMenu->GetActionCount() == 0; });
    Add(TEXT("Map tab sits between Build and Appearance"),
        [this, Before]()
        {
            *Before = Controller->Simulation().Serialize();
            Controller->OpenBook(2);
            Tap(EKeys::Gamepad_RightShoulder);
        },
        [this]() { return Controller->BookPage() == 7 && Controller->NativeMenu->GetMapView().IsValid()
            && Controller->NativeMenu->GetMapView()->PixelsPerCm() > 0; });    Capture(TEXT("native-map"));
    Add(TEXT("Controller triggers zoom the map in"),
        [this]() { Axis(EKeys::Gamepad_RightTriggerAxis, 1.0f); },
        [this]()
        {
            const auto View = Controller->NativeMenu->GetMapView();
            return View && View->PixelsPerCm() > View->FitPixelsPerCm() * 1.05;
        });
    const TSharedRef<double> PanStart = MakeShared<double>(0.0);
    Add(TEXT("Controller left stick pans the map"),
        [this, PanStart]()
        {
            const auto View = Controller->NativeMenu->GetMapView();
            // Zoom in far enough that the sheet is wider than the page, then push the stick east.
            View->ZoomBy(4.0, View->GetCachedGeometry().GetLocalSize() * 0.5f);
            *PanStart = View->Center().y;
            Axis(EKeys::Gamepad_LeftX, 1.0f);
        },
        [this, PanStart]()
        {
            const auto View = Controller->NativeMenu->GetMapView();
            return View && View->Center().y > *PanStart + 100.0;
        });
    Add(TEXT("D-pad steps between named landmarks"),
        [this]() { Tap(EKeys::Gamepad_DPad_Right); },
        [this]() { return !Controller->NativeMenu->GetMapView()->SelectedName().IsEmpty(); });
    Add(TEXT("Map input stays in the book and RB goes on to Appearance (no Guidebook between)"),
        [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this, Before]() { return Controller->BookPage() == 6 && Controller->IsBookOpen()
            && Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("The tab bar lists exactly Inventory, Craft, Build, Map, Appearance"),
        []() {},
        [this]() { return Controller->NativeMenu->GetTabPageCount() == 5
            && !Controller->NativeMenu->HasTabForPage(3); });
    Capture(TEXT("native-appearance"));
    // One real wheel notch over the book zooms Appearance exactly once (the preprocessor route only).
    const auto ArmBefore = MakeShared<float>(0.0f);
    Add(TEXT("One wheel notch over the book zooms Appearance by exactly one step"),
        [this, ArmBefore]()
        {
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (!Avatar) { Finish(false, TEXT("No heroine for the Appearance zoom check.")); return; }
            *ArmBefore = Avatar->AppearanceArm;
            const auto Geometry = Controller->NativeMenu->GetCachedGeometry();
            const FVector2D Over = Geometry.GetAbsolutePosition() + FVector2D(Geometry.GetAbsoluteSize().X * 0.15f, Geometry.GetAbsoluteSize().Y * 0.5f);
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            Slate.SetCursorPos(Over);
            Slate.ProcessMouseWheelOrGestureEvent(FPointerEvent(0, Over, Over, TSet<FKey>(), EKeys::MouseWheelAxis, 1.0f, FModifierKeysState()), nullptr);
        },
        [this, ArmBefore]()
        {
            const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            return Avatar && FMath::IsNearlyEqual(Avatar->AppearanceArm,
                FMath::Clamp(*ArmBefore - AHomesteadCharacter::AppearanceZoomStep, AHomesteadCharacter::AppearanceArmMin, AHomesteadCharacter::AppearanceArmMax));
        });
    Add(TEXT("Return to Settings without changing simulation or camera"),
        [this, Before]()
        {
            *Before = Controller->Simulation().Serialize();
            Tap(EKeys::Gamepad_Special_Right); Tap(EKeys::Gamepad_Special_Right);
        },
        [this, Before]() { return Controller->BookPage() == 4
            && Controller->Simulation().Serialize() == *Before; });
    PrepareNativeWardrobeChecks();
    PrepareNativePresentationCoverageChecks();
    PrepareNativeResetChecks();
    Add(TEXT("Prepare disclosed survival-failure fixture"),
        [this]() { Controller->Sim.AdvanceGameHours(120, Controller->PlayerPoint()); },
        [this]() { return Controller->IsFailed() && Controller->HasNativeMenu() && !Controller->IsBookOpen(); }, 0.8f);
    Add(TEXT("Recovery has independent controller Settings access"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Top); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 4; });
    Add(TEXT("Recovery Quit game uses the same safe two-choice dialog"),
        [this]() { Controller->NativeMenu->FocusLegacySubject(9); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->NativeMenu->IsExitPrompt(); });
    Capture(TEXT("native-recovery-exit"));
    Add(TEXT("Cancel recovery exit returns to recovery without forcing retry"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return Controller->IsFailed() && !Controller->IsBookOpen() && Controller->HasNativeMenu(); });
}

void AHomesteadSmokeTest::PrepareCraftingChecks()
{
    Add(TEXT("CONTROLLED unmet rusted head reveals the salvage piles on directional focus without crafting"),
        [this]()
        {
            Controller->Sim = Homestead::Simulation();
            Controller->OpenBook(1);
            if (!Controller->NativeMenu
                || !Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe,
                    static_cast<int32>(Homestead::Recipe::HaftAxe), 0))
            {
                Finish(false, TEXT("Axe hafting details are unavailable for the unmet head focus test."));
                return;
            }
            Tap(EKeys::Tab);
            for (int32 Step = 0; Step < 3; ++Step) Tap(EKeys::Down);
        },
        [this]()
        {
            return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Details")
                && Controller->NativeMenu->GetFocusedRequirementHint() == TEXT("Salvage piles around the manor")
                && Controller->Simulation().Count(Homestead::Item::RustedAxeHead) == 0
                && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0;
        });
    const auto ResetCraftingStock = [this]()
    {
        Controller->Sim = Homestead::Simulation();
        const auto Gather = [this](Homestead::ResourceKind Kind, Homestead::Item Item, int32 Target)
        {
            const auto Nodes = Controller->State().resources;
            for (const auto& Node : Nodes)
                if (Node.kind == Kind && Controller->Sim.Count(Item) < Target)
                    Controller->Sim.Harvest(Node.id, Node.position);
            return Controller->Sim.Count(Item) >= Target;
        };
        // Two rusted axe heads (stand-ins for estate salvage) allow two complete hafting cycles.
        if (!Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 4)
            || !Controller->Sim.GrantItems(Homestead::Item::RustedAxeHead, 2))
        { Finish(false, TEXT("Could not gather isolated crafting stock.")); return; }
        Controller->OpenBook(1);
        if (!Controller->NativeMenu.IsValid()
            || !Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe,
                static_cast<int32>(Homestead::Recipe::HaftAxe), 0))
            Finish(false, TEXT("Axe hafting recipe tile was unavailable."));
    };
    Add(TEXT("Structured recipe rows expose exact counts and the salvage source"),
        ResetCraftingStock,
        [this]()
        {
            const auto* Row = Controller->NativeMenu->GetSelectedSubject();
            const FString Details = Controller->NativeMenu->GetDisplayedDetails();
            return Row && Row->Subject == EHomesteadMenuSubject::Recipe
                && Row->HasRecipeState && Row->RecipeState.craftable
                && Details.Contains(TEXT("Branch: Have")) && Details.Contains(TEXT("/ Need 2"))
                && Details.Contains(TEXT("Rusted axe head: Have"))
                && Details.Contains(TEXT("/ Need 1 (Salvage piles around the manor)"))
                && Controller->NativeMenu->GetActionCount() == 0;
        });
    Add(TEXT("Capture ready crafting requirements"),
        [this]() { Screenshot(TEXT("craft-requirements-ready")); }, []() { return true; }, 0.6f);
    Add(TEXT("Quick keyboard press selects without crafting"),
        [this]() { Tap(EKeys::Enter); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 0
            && FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress()); });
    Add(TEXT("Space uses the same select-without-crafting press behavior"),
        [this]() { Tap(EKeys::SpaceBar); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 0
            && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content"); });
    Add(TEXT("Partial keyboard hold shows progress without granting output"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Pressed, 1)); },
        [this]() { const float Progress = Controller->NativeMenu->GetCraftProgress();
            return Controller->Simulation().Count(Homestead::Item::Hatchet) == 0
                && Progress > 0.25f && Progress < 0.75f; }, 0.5f);
    Add(TEXT("Capture partial crafting progress"),
        [this]() { Screenshot(TEXT("craft-hold-progress")); }, []() { return true; }, 0.35f);
    Add(TEXT("Keyboard release cancels incomplete cycle"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Released, 0)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 0
            && FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress()); }, 0.1f);
    Add(TEXT("Complete keyboard hold crafts exactly one"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Pressed, 1)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1; }, 1.3f);
    Add(TEXT("Continuing keyboard hold crafts a second complete cycle"),
        []() {},
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 2; }, 1.2f);
    Add(TEXT("Release ends repeated keyboard crafting"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Released, 0)); },
        [this]() { return FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress()); }, 0.1f);
    Add(TEXT("Unavailable recipe hold cannot queue another craft"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Pressed, 1)); },
        [this]() { const auto* Row = Controller->NativeMenu->GetSelectedSubject();
            return Row && Row->HasRecipeState && !Row->RecipeState.craftable
                && Controller->Simulation().Count(Homestead::Item::Hatchet) == 2
                && FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress())
                && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content"); }, 0.4f);
    Add(TEXT("Unavailable recipe release remains inert"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::SpaceBar, IE_Released, 0)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 2; }, 0.1f);
    Add(TEXT("Capture unavailable crafting requirements"),
        [this]() { Screenshot(TEXT("craft-requirements-blocked")); }, []() { return true; }, 0.6f);

    Add(TEXT("Controller hold uses the same authoritative cycle"),
        [this, ResetCraftingStock]() { ResetCraftingStock(); Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, 1)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1; }, 1.3f);
    Add(TEXT("Controller release cancels continuation"),
        [this]() { Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::Gamepad_FaceButton_Bottom, IE_Released, 0)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1
            && FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress()); }, 0.2f);

    Add(TEXT("Pointer hold uses the same authoritative cycle"),
        [this, ResetCraftingStock]() { ResetCraftingStock();
            Controller->TestCraftBeatRequests = 0;
            Controller->TestAudibleCraftBeats = 0;
            Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Pressed, 1)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1
            && Controller->TestCraftBeatRequests == 3; }, 1.3f);
    Add(TEXT("Pointer release ends crafting and refreshes blockers"),
        [this]() { Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::LeftMouseButton, IE_Released, 0)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1
            && FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress()); }, 0.2f);
    Add(TEXT("Muted Effects still times three beats without audible playback"),
        [this, ResetCraftingStock]() { ResetCraftingStock();
            Controller->EffectsVolume = 0;
            Controller->TestCraftBeatRequests = 0;
            Controller->TestAudibleCraftBeats = 0;
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Pressed, 1)); },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1
            && Controller->TestCraftBeatRequests == 3
            && Controller->TestAudibleCraftBeats == 0; }, 1.3f);
    Add(TEXT("Release muted hold and restore Effects level"),
        [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Released, 0));
            Controller->EffectsVolume = 0.8f; },
        [this]() { return Controller->Simulation().Count(Homestead::Item::Hatchet) == 1; }, 0.1f);
    Add(TEXT("Crafting audio proof includes a stable recording tail"),
        []() {}, []() { return true; }, 2.5f);
    Add(TEXT("Begin a hold before changing recipe focus"),
        [this, ResetCraftingStock]() { ResetCraftingStock(); Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Pressed, 1)); },
        [this]() { return Controller->NativeMenu->GetCraftProgress() > 0.2f
            && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0; }, 0.4f);
    Add(TEXT("Changing recipe focus cancels incomplete progress"),
        [this]() { Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe,
            static_cast<int32>(Homestead::Recipe::HaftHoe), 0); },
        [this]() { return FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress())
            && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0; }, 0.2f);
    Add(TEXT("Begin a hold before changing page"),
        [this, ResetCraftingStock]() { ResetCraftingStock(); Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Pressed, 1)); },
        [this]() { return Controller->NativeMenu->GetCraftProgress() > 0.2f; }, 0.4f);
    Add(TEXT("Changing page cancels incomplete progress"),
        [this]() { Controller->NativeMenu->ChangePage(2); },
        [this]() { return Controller->BookPage() == 2
            && FMath::IsNearlyZero(Controller->NativeMenu->GetCraftProgress())
            && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0; }, 0.2f);
    Add(TEXT("Begin a hold before closing the menu"),
        [this, ResetCraftingStock]() { ResetCraftingStock(); Controller->InputKey(
            FInputKeyEventArgs::CreateSimulated(EKeys::Enter, IE_Pressed, 1)); },
        [this]() { return Controller->NativeMenu->GetCraftProgress() > 0.2f; }, 0.4f);
    Add(TEXT("Closing the menu destroys incomplete progress without queued output"),
        [this]() { Controller->CloseBook(); },
        [this]() { return !Controller->IsBookOpen()
            && Controller->Simulation().Count(Homestead::Item::Hatchet) == 0; }, 1.3f);

    const auto PrepareRecipe = [this](Homestead::Recipe Recipe)
    {
        Controller->Sim = Homestead::Simulation();
        const auto Gather = [this](Homestead::ResourceKind Kind, Homestead::Item Item, int32 Target)
        {
            const auto Nodes = Controller->State().resources;
            for (const auto& Node : Nodes)
                if (Node.kind == Kind && Controller->Sim.Count(Item) < Target)
                    Controller->Sim.Harvest(Node.id, Node.position);
            return Controller->Sim.Count(Item) >= Target;
        };
        bool Ready = true;
        if (Recipe == Homestead::Recipe::HaftHoe)
            Ready = Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 2)
                && Controller->Sim.GrantItems(Homestead::Item::RustedHoeBlade, 1).ok;
        else if (Recipe == Homestead::Recipe::HaftBillhook)
            Ready = Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 2)
                && Controller->Sim.GrantItems(Homestead::Item::RustedBillhookHead, 1).ok;
        else if (Recipe == Homestead::Recipe::RoastedRoots || Recipe == Homestead::Recipe::HerbedRoots)
        {
            Ready = Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 4)
                && Gather(Homestead::ResourceKind::Stones, Homestead::Item::Stone, 4)
                && Gather(Homestead::ResourceKind::Roots, Homestead::Item::Roots, 2);
            if (Recipe == Homestead::Recipe::HerbedRoots)
                Ready = Ready && Gather(Homestead::ResourceKind::Flowers, Homestead::Item::Flowers, 1);
            bool Placed = false;
            for (int32 X = -4; X <= -2 && !Placed; ++X)
                for (int32 Y = -1; Y <= 1 && !Placed; ++Y)
                    Placed = Controller->Sim.Place(Homestead::Piece::Fire, X, Y, 0,
                        Controller->PlayerPoint()).ok;
            Ready = Ready && Placed;
            if (Ready)
            {
                const int32 FireId = Controller->State().structures.back().id;
                Ready = Controller->Sim.AddFuel(FireId, Controller->PlayerPoint()).ok;
            }
        }
        else if (Recipe == Homestead::Recipe::SplitFirewood)
        {
            Ready = Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 2)
                && Controller->Sim.GrantItems(Homestead::Item::RustedAxeHead, 1).ok
                && Controller->Sim.Craft(Homestead::Recipe::HaftAxe, Controller->PlayerPoint()).ok;
            if (Ready)
            {
                const auto Nodes = Controller->State().resources;
                for (const auto& Node : Nodes)
                    if (Node.kind == Homestead::ResourceKind::ForestTree)
                    { Ready = Controller->Sim.Harvest(Node.id, Node.position).ok; break; }
            }
        }
        if (!Ready)
        {
            Finish(false, FString::Printf(TEXT("Could not prepare %s crafting inputs."),
                UTF8_TO_TCHAR(Homestead::RecipeName(Recipe))));
            return;
        }
        Controller->OpenBook(1);
        if (!Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe,
            static_cast<int32>(Recipe), 0))
            Finish(false, TEXT("Prepared recipe tile was unavailable."));
    };
    const Homestead::Recipe Recipes[] = {Homestead::Recipe::HaftHoe,
        Homestead::Recipe::HaftBillhook, Homestead::Recipe::RoastedRoots,
        Homestead::Recipe::HerbedRoots, Homestead::Recipe::SplitFirewood};
    const Homestead::Item Outputs[] = {Homestead::Item::DiggingStick,
        Homestead::Item::Billhook, Homestead::Item::RoastedRoots,
        Homestead::Item::HerbedRoots, Homestead::Item::Firewood};
    const int32 Counts[] = {1, 1, 1, 1, 4};
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Recipes); ++Index)
    {
        const auto Recipe = Recipes[Index];
        const auto Output = Outputs[Index];
        const int32 Count = Counts[Index];
        Add(FString::Printf(TEXT("Prepare %s assessment from ordinary gathered inputs"),
            UTF8_TO_TCHAR(Homestead::RecipeName(Recipe))),
            [PrepareRecipe, Recipe]() { PrepareRecipe(Recipe); },
            [this]() { const auto* Row = Controller->NativeMenu->GetSelectedSubject();
                return Row && Row->HasRecipeState && Row->RecipeState.craftable; });
        Add(FString::Printf(TEXT("Hold crafts exact %s output"),
            UTF8_TO_TCHAR(Homestead::RecipeName(Recipe))),
            [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::Enter, IE_Pressed, 1)); },
            [this, Output, Count]() { return Controller->Simulation().Count(Output) == Count; }, 1.3f);
        Add(TEXT("Release recipe hold before the next isolated recipe"),
            [this]() { Controller->InputKey(FInputKeyEventArgs::CreateSimulated(
                EKeys::Enter, IE_Released, 0)); },
            [this, Output, Count]() { return Controller->Simulation().Count(Output) == Count; }, 0.1f);
    }
    Add(TEXT("CONTROLLED full pack shows capacity only when it blocks Split firewood"),
        [this]()
        {
            Homestead::Simulation Full;
            auto& State = const_cast<Homestead::State&>(Full.GetState());
            State.inventory.fill(0);
            State.inventoryLayout.clear();
            for (const auto Pair : {TPair<Homestead::Item, int32>(Homestead::Item::Billhook, 1),
                TPair<Homestead::Item, int32>(Homestead::Item::Hatchet, 1),
                TPair<Homestead::Item, int32>(Homestead::Item::Timber, 1),
                TPair<Homestead::Item, int32>(Homestead::Item::Stone, 117)})
            {
                State.inventory[static_cast<int32>(Pair.Key)] = Pair.Value;
                State.inventoryLayout.push_back(
                    {State.nextGroupId++, Pair.Key, Pair.Value, 0});
            }
            const auto Loaded = Controller->Sim.Deserialize(Full.Serialize());
            if (!Loaded || Controller->Sim.UsedCapacity() != Homestead::InventoryCapacity)
            {
                Finish(false, TEXT("Controlled full-pack recipe authority is invalid."));
                return;
            }
            Controller->OpenBook(1);
            if (!Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Recipe,
                static_cast<int32>(Homestead::Recipe::SplitFirewood), 0))
                Finish(false, TEXT("Split firewood details are unavailable."));
        },
        [this]()
        {
            const auto* Row = Controller->NativeMenu->GetSelectedSubject();
            const FString Details = Controller->NativeMenu->GetDisplayedDetails();
            return Row && Row->HasRecipeState && !Row->RecipeState.capacityMet
                && Row->RecipeState.ingredients.size() == 1
                && Row->RecipeState.ingredients[0].met
                && Row->RecipeState.retainedToolMet
                && Controller->NativeMenu->GetActionCount() == 0
                && Details.Contains(TEXT("Pack space: Full"));
        });
    Add(TEXT("Capture the only blocking pack-capacity icon row"),
        [this]() { Screenshot(TEXT("craft-requirements-capacity")); },
        []() { return true; }, 0.6f);
}

void AHomesteadSmokeTest::PrepareNativeWardrobeChecks()
{
    const auto Expected = MakeShared<Homestead::Simulation>();
    const auto Tunic = MakeShared<int32>(0);
    const auto Saved = MakeShared<HomesteadNativeMenuProof::FSaveFixture>();
    const auto ExpectedLook = MakeShared<FHomesteadAppearance>();
    const auto SaveCalls = MakeShared<uint32>(0);
    const auto LoadCalls = MakeShared<uint32>(0);
    const auto OpenInventory = [this](int32 View)
    {
        Controller->CloseBook();
        Controller->MenuInventoryView(View);
        Controller->OpenBook(0);
    };
    Add(TEXT("Select the actually owned starter tunic in Wearing"),
        [this, Tunic, OpenInventory]()
        {
            *Tunic = Controller->State().equipment[static_cast<int32>(Homestead::EquipmentSlot::Torso)];
            OpenInventory(2);
        },
        [this, Tunic]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            return Owned && Owned->definition == Homestead::WearableDefinition::LinenTunic
                && Owned->owner == Homestead::WearableOwner::Equipped && Subject
                && Subject->Subject == EHomesteadMenuSubject::Wearable && Subject->SubjectId == *Tunic
                && VerifyNativeMenuPresentation();
        });
    Add(TEXT("Mapped UI unequip commits exact ownership and removes the rendered tunic"),
        [this, Tunic, Expected]()
        {
            *Expected = Controller->Simulation();
            if (!Expected->UnequipWearable(*Tunic, Expected->GetRevision()))
            { Finish(false, TEXT("The independent unequip expectation was invalid.")); return; }
            if (!Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Unequip))
            { Finish(false, TEXT("Owned tunic unequip action is unavailable.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Expected, Tunic]()
        {
            const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            return !Controller->ToastIsError() && Owned && Owned->owner == Homestead::WearableOwner::Carried
                && Controller->Simulation().Serialize() == Expected->Serialize()
                && VerifyNativeMenuPresentation();
        });
    Add(TEXT("Select that same nonduplicated garment from the carried grid"),
        [this, Tunic, OpenInventory]()
        {
            OpenInventory(0);
            if (!Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *Tunic, 0))
                Finish(false, TEXT("The same carried tunic identity is unavailable."));
        },
        [this, Tunic]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Subject && Subject->Subject == EHomesteadMenuSubject::Wearable && Subject->SubjectId == *Tunic
                && Subject->ContainerId == 0;
        });
    Add(TEXT("Move the owned tunic into the exact chest through virtual drag"),
        [this, Tunic]()
        {
            int32 Chest = -1;
            for (const auto& Piece : Controller->State().structures)
                if (Piece.kind == Homestead::Piece::Chest) { Chest = Piece.id; break; }
            FHomesteadRow BranchRow;
            for (const auto& Row : Controller->MenuRows())
                if (Row.Subject == EHomesteadMenuSubject::ItemGroup
                    && Row.Id == static_cast<int32>(Homestead::Item::Branch))
                { BranchRow = Row; break; }
            BranchRow.DestinationId = Chest;
            if (Chest < 0 || BranchRow.SubjectId <= 0 || !Controller->MenuItemAction(
                BranchRow, EHomesteadItemAction::Transfer, 1, Controller->Simulation().GetRevision())
                || !Controller->OpenChestStorage(Chest))
            { Finish(false, TEXT("Could not prepare exact-chest garment drag.")); return; }
            const int32 ChestTarget = Controller->Simulation().GetLayout(Chest)->front().groupId;
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *Tunic, 0);
            Tap(EKeys::Enter);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, ChestTarget, Chest);
            Tap(EKeys::Enter);
        },
        [this, Tunic]() { const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            return !Controller->ToastIsError() && Owned && Owned->owner == Homestead::WearableOwner::Chest; });
    Add(TEXT("Return the same owned tunic to Pack through virtual drag"),
        [this, Tunic]()
        {
            const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            if (!Owned || Owned->owner != Homestead::WearableOwner::Chest)
            { Finish(false, TEXT("Stored tunic identity is unavailable.")); return; }
            const int32 ChestId = Owned->chestId;
            int32 PackTarget = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId) { PackTarget = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *Tunic, ChestId);
            Tap(EKeys::Enter);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, PackTarget, 0);
            Tap(EKeys::Enter);
            const auto* ChestLayout = Controller->Simulation().GetLayout(ChestId);
            if (ChestLayout && !ChestLayout->empty())
            {
                FHomesteadRow Stored;
                Stored.Id = static_cast<int32>(ChestLayout->front().item);
                Stored.Subject = EHomesteadMenuSubject::ItemGroup;
                Stored.SubjectId = ChestLayout->front().groupId;
                Stored.ContainerId = ChestId;
                Stored.DestinationId = 0;
                Stored.Quantity = ChestLayout->front().quantity;
                Controller->MenuItemAction(Stored, EHomesteadItemAction::Transfer,
                    Stored.Quantity, Controller->Simulation().GetRevision());
            }
        },
        [this, Tunic]() { const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            return !Controller->ToastIsError() && Owned && Owned->owner == Homestead::WearableOwner::Carried; });
    Add(TEXT("Mapped UI equip restores the same ID and committed garment mesh"),
        [this, Tunic, Expected]()
        {
            Controller->CloseBook(); Controller->MenuInventoryView(0); Controller->OpenBook(0);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *Tunic, 0);
            Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Equip);
            *Expected = Controller->Simulation();
            if (!Expected->EquipWearable(*Tunic, Expected->GetRevision()))
            { Finish(false, TEXT("The independent equip expectation was invalid.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Expected, Tunic]()
        {
            return !Controller->ToastIsError()
                && Controller->State().equipment[static_cast<int32>(Homestead::EquipmentSlot::Torso)] == *Tunic
                && Controller->State().equipment[static_cast<int32>(Homestead::EquipmentSlot::Legs)] == *Tunic
                && Controller->Simulation().Serialize() == Expected->Serialize() && VerifyNativeMenuPresentation();
        });
    const auto DyeTarget = MakeShared<int32>(0);
    const auto DyeBefore = MakeShared<FString>();
    Add(TEXT("Mapped UI opens the dye chooser for the owned tunic"),
        [this, Tunic, Expected, OpenInventory, DyeTarget, DyeBefore]()
        {
            OpenInventory(2);
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            if (!Subject || Subject->SubjectId != *Tunic || !Owned)
            { Finish(false, TEXT("The intended owned tunic is not selected for dye.")); return; }
            if (Owned->dye >= 3) { Finish(false, TEXT("The dye fixture expects a tunic below the last dye.")); return; }
            *DyeTarget = Owned->dye + 1;
            *DyeBefore = FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str()));
            *Expected = Controller->Simulation();
            if (!Expected->RecolorWearable(*Tunic, *DyeTarget, Controller->PlayerPoint(), Expected->GetRevision()))
            { Finish(false, TEXT("The independent dye expectation was invalid.")); return; }
            if (!Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Dye))
            { Finish(false, TEXT("Owned tunic dye action is unavailable.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this]() { return Controller->NativeMenu->IsDyeChooserOpen(); });
    Add(TEXT("Moving to the next dye previews it on her without changing the save"),
        [this]() { Tap(EKeys::Gamepad_DPad_Down); },
        [this, DyeTarget, DyeBefore]() { return Controller->NativeMenu->GetDyePreview() == *DyeTarget
            && FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str())) == *DyeBefore; });
    Add(TEXT("Capture the dye chooser previewing the next dye"),
        [this]() { Screenshot(TEXT("native-dye-chooser")); },
        [this]() { return Controller->NativeMenu->IsDyeChooserOpen(); }, 0.8f);
    Add(TEXT("A chooses the previewed dye"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, DyeTarget]() { return Controller->NativeMenu->IsDyeChooserOpen() && Controller->NativeMenu->GetDyeChoice() == *DyeTarget; });
    Add(TEXT("Apply changes only the owned item and its actual material tint"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Expected]() { return !Controller->ToastIsError() && !Controller->NativeMenu->IsDyeChooserOpen()
            && Controller->Simulation().Serialize() == Expected->Serialize() && VerifyNativeMenuPresentation(); });
    Add(TEXT("Capture admitted dyed wardrobe"),
        [this]() { Screenshot(TEXT("native-wardrobe-dyed")); },
        [this]() { return VerifyNativeMenuPresentation(); }, 0.8f);
    // A garment in her pack previews too: she tries it on in the copy, and nothing is saved.
    const auto PackDyeBefore = MakeShared<FString>();
    const auto PackDyeTarget = MakeShared<int32>(0);
    const auto PackDyeRestore = MakeShared<Homestead::Simulation>();
    const auto PackDyeShown = MakeShared<int32>(INDEX_NONE);
    const auto PackDyeSaveKept = MakeShared<bool>(false);
    Add(TEXT("Previewing a dye on the tunic while it's in her pack dresses her in it without saving"),
        [this, Tunic, PackDyeBefore, PackDyeTarget, PackDyeRestore, PackDyeShown, PackDyeSaveKept]()
        {
            *PackDyeRestore = Controller->Sim;
            if (!Controller->Sim.UnequipWearable(*Tunic, Controller->Sim.GetRevision()).ok)
            { Finish(false, TEXT("Could not put the tunic in her pack for the preview check.")); return; }
            *PackDyeBefore = FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str()));
            *PackDyeTarget = (Controller->Simulation().GetWearable(*Tunic)->dye + 1) % 4;
            if (!Controller->MenuPreviewDye(*Tunic, *PackDyeTarget)) { Finish(false, TEXT("The pack-garment dye preview was refused.")); return; }
            // What the preview dressed her in, then her own clothes back in the same frame, so the
            // saved-look contract holds between steps.
            if (const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn())) *PackDyeShown = Avatar->PendingMetaHumanTunicDye;
            *PackDyeSaveKept = Controller->Simulation().GetWearable(*Tunic)->owner == Homestead::WearableOwner::Carried
                && FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str())) == *PackDyeBefore;
            Controller->Sim = *PackDyeRestore;
            Controller->MenuEndDyePreview();
            Controller->NativeMenu->Refresh();
        },
        [PackDyeTarget, PackDyeShown, PackDyeSaveKept]() { return *PackDyeShown == *PackDyeTarget && *PackDyeSaveKept; });
    Add(TEXT("Ending the preview puts her own clothes back, and she wears the tunic again"),
        []() {},
        [this, Tunic]() { return Controller->State().equipment[static_cast<int32>(Homestead::EquipmentSlot::Torso)] == *Tunic
            && VerifyNativeMenuPresentation(); });

    for (int32 Id : {0, 1, 1, 1, 1, 2, 3})
    {
        Add(FString::Printf(TEXT("Change real Appearance control %d before persistence"), Id),
            [this, Id, ExpectedLook, Expected]()
            {
                Controller->CloseBook();
                Controller->OpenBook(6);
                if (!Controller->NativeMenu->FocusLegacySubject(Id))
                { Finish(false, TEXT("The required Appearance control is unavailable.")); return; }
                *ExpectedLook = Controller->GetAppearance();
                *Expected = Controller->Simulation();
                switch (Id)
                {
                case 0: ExpectedLook->MetaHair = (ExpectedLook->MetaHair + 1) % HomesteadLook::MetaHairCount;
                    ExpectedLook->HairStyle = HomesteadLook::LegacyHairStyle(ExpectedLook->MetaHair); break;
                case 1: ExpectedLook->HairColor = (ExpectedLook->HairColor + 1) % HomesteadLook::HairColorCount; break;
                case 2: ExpectedLook->SkinTone = (ExpectedLook->SkinTone + 1) % 4; break;
                case 3: ExpectedLook->EyeColor = (ExpectedLook->EyeColor + 1) % 4; break;
                case 6: ExpectedLook->BodyPreset = (ExpectedLook->BodyPreset + 1) % 3; break;
                }
                Tap(EKeys::Enter);
            },
            [this, ExpectedLook, Expected]() { return !Controller->ToastIsError()
                && HomesteadNativeMenuProof::SameLook(Controller->GetAppearance(), *ExpectedLook)
                && Controller->Simulation().Serialize() == Expected->Serialize() && VerifyNativeMenuPresentation(); });
    }
    Add(TEXT("Real F5 saves nondefault owned equipment, dye and all appearance fields"),
        [this, Saved, SaveCalls]()
        {
            Saved->Simulation = UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str());
            Saved->World = Controller->WorldId;
            Saved->Look = Controller->GetAppearance();
            Saved->ProducerProcess = FPlatformProcess::GetCurrentProcessId();
            *SaveCalls = Controller->TestQuickSaves + 1;
            Tap(EKeys::F5);
        },
        [this, Saved, SaveCalls]()
        {
            const auto* Written = Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual")));
            return Controller->TestQuickSaves == *SaveCalls && !Controller->ToastIsError() && Written
                && Written->IsCurrentVersion() && Written->SimulationData == Saved->Simulation && Written->WorldId == Saved->World;
        });
    Add(TEXT("Mutate owned equipment through UI after saving"),
        [this, OpenInventory]() { OpenInventory(2);
            if (!Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Unequip))
            { Finish(false, TEXT("Saved tunic unequip action is unavailable.")); return; }
            Tap(EKeys::Enter); },
        [this, Saved, Tunic]()
        {
            const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            return !Controller->ToastIsError() && Owned && Owned->owner == Homestead::WearableOwner::Carried
                && FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str())) != Saved->Simulation;
        });
    Add(TEXT("Mutate hairstyle through UI after saving"),
        [this]()
        {
            Controller->CloseBook(); Controller->OpenBook(6);
            if (!Controller->NativeMenu->FocusLegacySubject(0))
            { Finish(false, TEXT("Hairstyle control unavailable for the load-difference fixture.")); return; }
            Tap(EKeys::Enter); Tap(EKeys::Enter);
        },
        [this, Saved]() { return !Controller->ToastIsError() && Controller->GetAppearance().HairStyle != Saved->Look.HairStyle; });
    Add(TEXT("Genuine F9 restores exact current-version quantities, IDs, slots, dye, look and render identities"),
        [this, LoadCalls]()
        {
            *LoadCalls = Controller->TestQuickLoads + 1;
            Tap(EKeys::F9);
            // Reopen in the same callback so no unpaused simulation tick changes the expected saved state.
            Controller->OpenBook(0);
        },
        [this, Saved, LoadCalls]()
        {
            return Controller->TestQuickLoads == *LoadCalls && !Controller->ToastIsError() && !Controller->MenuNeedsTestReset()
                && Controller->WorldId == Saved->World
                && FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str())) == Saved->Simulation
                && HomesteadNativeMenuProof::SameLook(Controller->GetAppearance(), Saved->Look)
                && VerifyNativeMenuPresentation();
        }, 0.8f);
    Add(TEXT("Export explicit producer fixture from the actual validated manual save"),
        [this, Saved]()
        {
            TArray<uint8> Bytes;
            const FString Output = HomesteadTestOutputDirectory();
            const FString FixturePath = FPaths::Combine(Output, TEXT("native-wardrobe-fixture.sav"));
            const FString ManifestPath = FPaths::Combine(Output, TEXT("native-wardrobe-fixture.json"));
            if (IFileManager::Get().FileExists(*FixturePath) || IFileManager::Get().FileExists(*ManifestPath)
                || !FFileHelper::LoadFileToArray(Bytes, *Controller->SavePath(TEXT("Homestead_Manual"))))
            { Finish(false, TEXT("A fresh explicit producer fixture could not be prepared.")); return; }
            Saved->Fingerprint = FMD5::HashBytes(Bytes.GetData(), Bytes.Num());
            FString Json;
            if (!FJsonSerializer::Serialize(HomesteadNativeMenuProof::FixtureJson(*Saved), TJsonWriterFactory<>::Create(&Json))
                || !FFileHelper::SaveArrayToFile(Bytes, *FixturePath) || !FFileHelper::SaveStringToFile(Json, *ManifestPath))
            { Finish(false, TEXT("Could not persist the producer's pinned save bytes and expectations.")); }
        },
        [this, Saved]()
        {
            TArray<uint8> Bytes;
            if (!FFileHelper::LoadFileToArray(Bytes, *FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("native-wardrobe-fixture.sav"))))
                return false;
            return Saved->Fingerprint == FMD5::HashBytes(Bytes.GetData(), Bytes.Num())
                && Controller->ReadSave(FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("native-wardrobe-fixture.sav"))) != nullptr
                && IFileManager::Get().FileExists(*FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("native-wardrobe-fixture.json")));
        });
    Add(TEXT("Capture restored admitted wardrobe"),
        [this]() { Screenshot(TEXT("native-wardrobe-restored")); },
        [this]() { return VerifyNativeMenuPresentation(); }, 0.8f);
}

void AHomesteadSmokeTest::PrepareNativeInventoryTransactionChecks()
{
    const auto Chest = MakeShared<int32>(-1);
    const auto OtherChest = MakeShared<int32>(-1);
    const auto Branches = MakeShared<int32>(0);
    const auto Snapshot = MakeShared<std::string>();
    const auto BranchTotal = MakeShared<int32>(0);
    const auto DragSource = MakeShared<FVector2D>();
    const auto DragTarget = MakeShared<FVector2D>();
    const auto FullSnapshot = MakeShared<std::string>();
    const auto FullPackGroup = MakeShared<int32>(0);
    const auto Group = [this](int32 Container, int32 Quantity = -1)
    {
        const auto* Layout = Controller->Simulation().GetLayout(Container);
        if (!Layout) return 0;
        for (const auto& Entry : *Layout)
            if (!Entry.wearableId && Entry.item == Homestead::Item::Branch
                && (Quantity < 0 || Entry.quantity == Quantity)) return Entry.groupId;
        return 0;
    };
    const auto PointerDrag = [this](FVector2D From, FVector2D To)
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        Slate.SetCursorPos(From);
        TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
        Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, From, From, Pressed,
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
        Slate.SetCursorPos(To);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, To, From, Pressed,
            EKeys::Invalid, 0, FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, To, To, TSet<FKey>(),
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
    };
    Add(TEXT("Gather real transaction stock and place one reachable chest through authority"),
        [this, Chest, OtherChest, Branches, BranchTotal, Group]()
        {
            auto Gather = [this](Homestead::ResourceKind Kind, Homestead::Item Item, int32 Target)
            {
                while (Controller->Simulation().Count(Item) < Target)
                {
                    bool Done = false;
                    for (const auto& Node : Controller->State().resources)
                        if (Node.kind == Kind && Controller->Simulation().CanHarvest(Node.id))
                        { Done = Controller->Sim.Harvest(Node.id, Node.position).ok; break; }
                    if (!Done) return false;
                }
                return true;
            };
            if (!Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 12)
                || !Controller->Sim.GrantItems(Homestead::Item::BrambleCanes, 2))
            { Finish(false, TEXT("Could not gather real transaction stock.")); return; }
            FIntPoint FirstCell(0, 0);
            for (const FIntPoint Cell : {FIntPoint(-4,0), FIntPoint(-3,0), FIntPoint(-4,-1), FIntPoint(-3,-1)})
            {
                const auto Center = Homestead::CellCenter(Cell.X, Cell.Y);
                if (Controller->Sim.Place(Homestead::Piece::Chest, Cell.X, Cell.Y, 0, Center))
                {
                    *Chest = Controller->Sim.FindNearestStructure(Center, Homestead::Piece::Chest, 1);
                    FirstCell = Cell;
                    break;
                }
            }
            if (*Chest <= 0 || !Gather(Homestead::ResourceKind::Branches, Homestead::Item::Branch, 20)
                || !Gather(Homestead::ResourceKind::Stones, Homestead::Item::Stone, 3)
                || !Controller->Sim.GrantItems(Homestead::Item::BrambleCanes, 7)
                || !Controller->Sim.GrantItems(Homestead::Item::RustedAxeHead, 1)
                || !Controller->Sim.Craft(Homestead::Recipe::HaftAxe, Controller->PlayerPoint()))
            { Finish(false, TEXT("Could not gather the second-chest fixture stock.")); return; }
            const FIntPoint Neighbors[] = {
                {FirstCell.X + 1, FirstCell.Y}, {FirstCell.X - 1, FirstCell.Y},
                {FirstCell.X, FirstCell.Y + 1}, {FirstCell.X, FirstCell.Y - 1},
                {FirstCell.X + 1, FirstCell.Y + 1}, {FirstCell.X - 1, FirstCell.Y - 1},
                {FirstCell.X + 1, FirstCell.Y - 1}, {FirstCell.X - 1, FirstCell.Y + 1}};
            FIntPoint SecondCell(0, 0);
            for (const FIntPoint Cell : Neighbors)
            {
                const auto Nodes = Controller->State().resources;
                const double Left = Cell.X * Homestead::CellSize;
                const double Bottom = Cell.Y * Homestead::CellSize;
                for (const auto& Node : Nodes)
                {
                    if (Node.cleared || (Node.kind != Homestead::ResourceKind::Sapling
                        && Node.kind != Homestead::ResourceKind::ForestTree)) continue;
                    const bool Blocks = Node.kind == Homestead::ResourceKind::Sapling
                        ? FMath::FloorToInt(Node.position.x / Homestead::CellSize) == Cell.X
                            && FMath::FloorToInt(Node.position.y / Homestead::CellSize) == Cell.Y
                        : FMath::Square(Node.position.x - FMath::Clamp(Node.position.x,
                            Left, Left + Homestead::CellSize))
                            + FMath::Square(Node.position.y - FMath::Clamp(Node.position.y,
                                Bottom, Bottom + Homestead::CellSize)) <= 50.0 * 50.0;
                    if (Blocks) Controller->Sim.Harvest(Node.id, Node.position);
                }
                const auto Center = Homestead::CellCenter(Cell.X, Cell.Y);
                if (Controller->Sim.Place(Homestead::Piece::Chest, Cell.X, Cell.Y, 0, Center))
                {
                    *OtherChest = Controller->Sim.FindNearestStructure(Center, Homestead::Piece::Chest, 1);
                    SecondCell = Cell;
                    break;
                }
            }
            if (*OtherChest <= 0)
            { Finish(false, TEXT("Could not place a second reachable chest.")); return; }
            const auto FirstCenter = Homestead::CellCenter(FirstCell.X, FirstCell.Y);
            const auto SecondCenter = Homestead::CellCenter(SecondCell.X, SecondCell.Y);
            Teleport({(FirstCenter.x + SecondCenter.x) * 0.5, (FirstCenter.y + SecondCenter.y) * 0.5});
            *Branches = Group(0);
            *BranchTotal = Controller->Simulation().Count(Homestead::Item::Branch);
            Controller->MenuInventoryView(0); Controller->OpenBook(0);
        },
        [this, Chest, OtherChest, Branches]() { return *Chest > 0 && *OtherChest > 0
            && *OtherChest != *Chest && *Branches > 0; });
    Add(TEXT("Focused chest interaction binds one exact storage session ID"),
        [this]() { Controller->CloseBook(); Tap(EKeys::RightMouseButton); },
        [this, Chest, OtherChest]() { const auto Rows = Controller->MenuRows();
            return Controller->IsBookOpen() && Controller->InventoryView() == 1
            && Controller->ActiveStorageChest().IsSet()
            && Controller->ActiveStorageChest().GetValue() == *Chest
            && Rows.IndexOfByPredicate([OtherChest](const FHomesteadRow& Row)
                { return Row.ContainerId == *OtherChest; }) == INDEX_NONE; });
    Add(TEXT("Capture exact Chest and Pack storage surface"),
        [this]() { Screenshot(TEXT("native-storage-two-grid")); },
        [this]() { return Controller->ActiveStorageChest().IsSet(); }, 0.8f);
    Add(TEXT("Save the current world while exact storage is open"),
        [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError()
            && Controller->ActiveStorageChest().IsSet(); });
    Add(TEXT("Real load closes storage and clears its retained chest ID"),
        [this]() { Tap(EKeys::F9); },
        [this]() { return !Controller->ToastIsError() && !Controller->IsBookOpen()
            && !Controller->ActiveStorageChest().IsSet(); }, 0.8f);
    Add(TEXT("Storage reopens only through the exact restored chest"),
        [this, Chest]() { Controller->OpenChestStorage(*Chest); },
        [this, Chest]() { return Controller->IsBookOpen()
            && Controller->ActiveStorageChest().IsSet()
            && Controller->ActiveStorageChest().GetValue() == *Chest; });
    Add(TEXT("She names the chest from the keyboard; the name titles storage and its prompt"),
        [this]() { Controller->NativeMenu->OpenRenameChest(); Controller->NativeMenu->TypeChestName(TEXT("Linen press")); },
        [this]() { return Controller->NativeMenu->IsRenamingChest()
            && Controller->NativeMenu->GetChestNameDraft() == TEXT("Linen press"); });
    Add(TEXT("Capture the chest naming dialog"),
        [this]() { Screenshot(TEXT("native-chest-naming")); },
        [this]() { return Controller->NativeMenu->IsRenamingChest(); }, 0.8f);
    Add(TEXT("Enter saves the chest's name"),
        [this]() { Tap(EKeys::Enter); },
        [this, Chest]() { return !Controller->NativeMenu->IsRenamingChest() && !Controller->ToastIsError()
            && Controller->ChestDisplayName(*Chest) == TEXT("Linen press")
            && Controller->MenuInventorySummary().StartsWith(TEXT("Linen press:")); });
    const auto StoreExpected = MakeShared<Homestead::Simulation>();
    const auto BeforeStore = MakeShared<Homestead::Simulation>();
    Add(TEXT("T stores carried Stone onto the chest's Stone stack and nothing else"),
        [this, Chest, StoreExpected, BeforeStore]()
        {
            auto& Sim = Controller->Sim;
            *BeforeStore = Sim;
            const Homestead::Point At = Controller->PlayerPoint();
            if (!Sim.GrantItems(Homestead::Item::Stone, 3).ok) { Finish(false, TEXT("Could not grant the Stone fixture.")); return; }
            int32 Stone = 0;
            for (const auto& Entry : *Sim.GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Stone) { Stone = Entry.groupId; break; }
            if (!Sim.TransferGroup(*Chest, Stone, 1, true, At, Sim.GetRevision()).ok)
            { Finish(false, TEXT("Could not seed the chest's Stone stack.")); return; }
            Controller->NativeMenu->Refresh();
            *StoreExpected = Sim;
            if (!StoreExpected->StoreMatching(*Chest, At, StoreExpected->GetRevision()).ok)
            { Finish(false, TEXT("The independent auto-store expectation was invalid.")); return; }
            Tap(EKeys::T);
        },
        [this, StoreExpected]() { return !Controller->ToastIsError()
            && Controller->Simulation().Serialize() == StoreExpected->Serialize()
            && Controller->Simulation().Count(Homestead::Item::Stone) == 0
            && Controller->Simulation().Count(Homestead::Item::Branch) > 0; });
    Add(TEXT("With nothing left to match, T explains and changes nothing"),
        [this, StoreExpected]() { Tap(EKeys::T); },
        [this, StoreExpected]() { return Controller->ToastIsError()
            && Controller->Simulation().Serialize() == StoreExpected->Serialize(); });
    Add(TEXT("Put the chest back as it was before the auto-store checks (keeping its name)"),
        [this, BeforeStore]() { Controller->Sim = *BeforeStore; Controller->NativeMenu->Refresh(); },
        [this, Chest]() { return Controller->Simulation().ChestUsedCapacity(*Chest) == 0
            && Controller->ChestDisplayName(*Chest) == TEXT("Linen press"); });
    Add(TEXT("Back clears the exact storage session before ordinary Inventory"),
        [this, Branches]()
        {
            Controller->CloseBook();
            Controller->MenuInventoryView(0);
            Controller->OpenBook(0);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
        },
        [this]() { return !Controller->ActiveStorageChest().IsSet()
            && Controller->InventoryView() == 0
            && Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Ordinary Inventory beside a chest exposes pack only"),
        []() {},
        [this, Chest]() { const auto Rows = Controller->MenuRows();
            return !Controller->ActiveStorageChest().IsSet()
                && Rows.IndexOfByPredicate([this, Chest](const FHomesteadRow& Row)
                    { return Row.ContainerId == *Chest || Row.CanStore || Row.CanTake; }) == INDEX_NONE
                && !Controller->MenuInventorySummary().Contains(TEXT("Chest")); });
    Add(TEXT("Record the compact Branch tile and focus Bramble canes as a pointer reorder target"),
        [this, Branches, DragSource]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Branch tile geometry is unavailable.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            *DragSource = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            int32 Canes = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::BrambleCanes) { Canes = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Canes, 0);
        },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Drag Branch onto the Bramble canes position to reorder without changing quantities"),
        [this, DragSource, DragTarget, PointerDrag]()
        {
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Bramble canes tile geometry is unavailable.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            *DragTarget = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            PointerDrag(*DragSource, *DragTarget);
        },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            if (!Layout || Controller->Simulation().Count(Homestead::Item::Branch) != *BranchTotal) return false;
            int BranchIndex = -1, CanesIndex = -1;
            for (int Index = 0; Index < static_cast<int>(Layout->size()); ++Index)
            {
                if ((*Layout)[Index].item == Homestead::Item::Branch) BranchIndex = Index;
                if ((*Layout)[Index].item == Homestead::Item::BrambleCanes) CanesIndex = Index;
            }
            return BranchIndex > CanesIndex && !Controller->NativeMenu->IsPointerDraggingItem(); });
    Add(TEXT("Sort restores deterministic order after pointer reorder"),
        [this]() { Tap(EKeys::S); },
        [this]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            if (!Layout) return false;
            int BranchIndex = -1, CanesIndex = -1;
            for (int Index = 0; Index < static_cast<int>(Layout->size()); ++Index)
            {
                if ((*Layout)[Index].item == Homestead::Item::Branch) BranchIndex = Index;
                if ((*Layout)[Index].item == Homestead::Item::BrambleCanes) CanesIndex = Index;
            }
            return BranchIndex >= 0 && BranchIndex < CanesIndex; });
    Add(TEXT("Enter picks up the focused Branch tile for virtual drag"),
        [this, Branches]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            Tap(EKeys::Enter);
        },
        [this]() { return Controller->NativeMenu->IsVirtualDraggingItem(); });
    Add(TEXT("Enter drops Branch at focused Bramble canes using the same reorder authority"),
        [this]()
        {
            int32 Canes = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::BrambleCanes) { Canes = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Canes, 0);
            Tap(EKeys::Enter);
        },
        [this]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            if (!Layout || Controller->NativeMenu->IsVirtualDraggingItem()) return false;
            int BranchIndex = -1, CanesIndex = -1;
            for (int Index = 0; Index < static_cast<int>(Layout->size()); ++Index)
            {
                if ((*Layout)[Index].item == Homestead::Item::Branch) BranchIndex = Index;
                if ((*Layout)[Index].item == Homestead::Item::BrambleCanes) CanesIndex = Index;
            }
            return BranchIndex > CanesIndex; });
    Add(TEXT("Sort restores order after virtual drag"),
        [this]() { Tap(EKeys::S); },
        [this]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            if (!Layout) return false;
            int BranchIndex = -1, CanesIndex = -1;
            for (int Index = 0; Index < static_cast<int>(Layout->size()); ++Index)
            {
                if ((*Layout)[Index].item == Homestead::Item::Branch) BranchIndex = Index;
                if ((*Layout)[Index].item == Homestead::Item::BrambleCanes) CanesIndex = Index;
            }
            return BranchIndex >= 0 && BranchIndex < CanesIndex; });
    Add(TEXT("Back cancels virtual drag without closing Inventory or mutating state"),
        [this, Branches, Snapshot]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            *Snapshot = Controller->Simulation().Serialize();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_FaceButton_Right);
        },
        [this, Snapshot]() { return Controller->IsBookOpen()
            && !Controller->NativeMenu->IsVirtualDraggingItem()
            && Controller->Simulation().Serialize() == *Snapshot; });
    Add(TEXT("Inventory revision change cancels virtual drag before any drop"),
        [this, Branches]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            Tap(EKeys::Enter);
            if (!Controller->Sim.ReorderEntry(0, 0, 1,
                Controller->PlayerPoint(), Controller->Sim.GetRevision()))
            { Finish(false, TEXT("Could not create the stale virtual-drag revision.")); }
        },
        [this]() { return !Controller->NativeMenu->IsVirtualDraggingItem(); }, 0.3f);
    Add(TEXT("Sort restores order after stale virtual-drag cancellation"),
        [this]() { Tap(EKeys::S); },
        [this]() { return !Controller->ToastIsError(); });
    Add(TEXT("Changing page cancels virtual drag without mutating inventory"),
        [this, Branches, Snapshot]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            *Snapshot = Controller->Simulation().Serialize();
            Tap(EKeys::Enter);
            Controller->NativeMenu->ChangePage(1);
        },
        [this, Snapshot]() { return Controller->BookPage() == 1
            && !Controller->NativeMenu->IsVirtualDraggingItem()
            && Controller->Simulation().Serialize() == *Snapshot; });
    Add(TEXT("Return to Pack after virtual-drag page cancellation"),
        [this, Branches]()
        {
            Controller->NativeMenu->ChangePage(0);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
        },
        [this]() { return Controller->BookPage() == 0
            && Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Ctrl Enter splits the focused odd stack in half beside its source"),
        [this]()
        {
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftControl, IE_Pressed, 1));
            Tap(EKeys::Enter);
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftControl, IE_Released, 0));
        },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            if (!Layout) return false;
            TArray<int32> Quantities;
            for (const auto& Entry : *Layout)
                if (!Entry.wearableId && Entry.item == Homestead::Item::Branch)
                    Quantities.Add(Entry.quantity);
            return Quantities.Num() == 2 && Quantities[0] == (*BranchTotal + 1) / 2
                && Quantities[1] == *BranchTotal / 2; });
    Add(TEXT("Pack Sort collapses split groups and preserves exact totals"),
        [this]() { Tap(EKeys::S); },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 1; });
    Add(TEXT("Controller X directly splits the focused stack without an action button"),
        [this, Branches]() { *Branches = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Branch) { *Branches = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            Tap(EKeys::Gamepad_FaceButton_Left); },
        [this]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 2; });
    Add(TEXT("Controller A picks up the split Branch for virtual merge"),
        [this, Branches]()
        {
            int32 Split = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Branch
                    && Entry.groupId != *Branches) { Split = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Split, 0);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this]() { return Controller->NativeMenu->IsVirtualDraggingItem(); });
    Add(TEXT("Controller A drops onto the original Branch and merges exact totals"),
        [this, Branches]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && !Controller->NativeMenu->IsVirtualDraggingItem()
                && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 1; });
    Add(TEXT("Sort restores one authoritative branch stack for legacy transaction regression"),
        [this]() { Tap(EKeys::S); },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 1; });
    Add(TEXT("Focus the restored branch tile for real Ctrl pointer split"),
        [this, Branches]() { *Branches = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Branch) { *Branches = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0); },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Ctrl pointer click opens the how-many popover on the actual focused tile"),
        [this]()
        {
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            const auto Widget = Slate.GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Focused inventory tile is unavailable for pointer split.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            const FVector2D Position = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            Slate.SetCursorPos(Position);
            // A real pointer hovers the tile before clicking; the button only clicks while hovered.
            Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
                EKeys::Invalid, 0, FModifierKeysState()));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftControl, IE_Pressed, 1));
            TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
            Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position, Pressed,
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
            Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
            Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftControl, IE_Released, 0));
        },
        [this]() { return Controller->NativeMenu->IsQuantityPrompt()
            && Controller->NativeMenu->GetPopupOptionLabel(0).StartsWith(TEXT("Split off")); });
    Add(TEXT("Pointer Split off in the popover splits without typing an amount"),
        [this]()
        {
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            const auto Widget = Controller->NativeMenu->GetDialogButton(0);
            if (!Widget) { Finish(false, TEXT("The popover has no Split off button.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            const FVector2D Position = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            Slate.SetCursorPos(Position);
            Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
                EKeys::Invalid, 0, FModifierKeysState()));
            TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
            Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position, Pressed,
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
            Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
        },
        [this]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && !Controller->NativeMenu->HasActiveDialog()
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 2; });
    Add(TEXT("Record the original Branch merge target and focus its split source"),
        [this, Branches, DragTarget]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            const auto TargetWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!TargetWidget) { Finish(false, TEXT("Original Branch merge target is unavailable.")); return; }
            const auto Geometry = TargetWidget->GetCachedGeometry();
            *DragTarget = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            int32 Split = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Branch
                    && Entry.groupId != *Branches) { Split = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Split, 0);
        },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Drag one Branch stack onto the other to merge exact quantities"),
        [this, DragSource, DragTarget, PointerDrag]()
        {
            const auto SourceWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!SourceWidget) { Finish(false, TEXT("Split Branch source is unavailable.")); return; }
            const auto Geometry = SourceWidget->GetCachedGeometry();
            *DragSource = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            PointerDrag(*DragSource, *DragTarget);
        },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 1; });
    Add(TEXT("Sort restores the pointer-split stack before transfer regression"),
        [this]() { Tap(EKeys::S); },
        [this, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 1; });
    Add(TEXT("Pointer drag released outside any tile cancels without mutation"),
        [this, Branches, Snapshot, DragSource, PointerDrag]()
        {
            *Snapshot = Controller->Simulation().Serialize();
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Branch source is unavailable for cancel drag.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            *DragSource = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            PointerDrag(*DragSource, FVector2D(2.0f, 2.0f));
        },
        [this, Snapshot]() { return Controller->Simulation().Serialize() == *Snapshot
            && !Controller->NativeMenu->IsPointerDraggingItem(); });
    Add(TEXT("Right click on a stack opens its context menu with drop and hotbar choices"),
        [this, Branches]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *Branches, 0);
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            const auto Widget = Slate.GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Branch tile is unavailable for the context menu.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            const FVector2D Position = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            Slate.SetCursorPos(Position);
            TSet<FKey> Pressed; Pressed.Add(EKeys::RightMouseButton);
            Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position, Pressed,
                EKeys::RightMouseButton, 0, FModifierKeysState()));
            Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
                EKeys::RightMouseButton, 0, FModifierKeysState()));
        },
        [this]()
        {
            if (!Controller->NativeMenu->IsItemContextMenu()) return false;
            bool DropOne = false, DropAll = false;
            for (int32 Index = 0; Index < Controller->NativeMenu->DialogCountForTest(); ++Index)
            {
                const FString Label = Controller->NativeMenu->GetPopupOptionLabel(Index);
                DropOne |= Label == TEXT("Drop 1");
                DropAll |= Label.StartsWith(TEXT("Drop all"));
            }
            return DropOne && DropAll;
        });
    Add(TEXT("Escape closes the context menu without mutation"),
        [this]() { Tap(EKeys::Escape); },
        [this, Snapshot]() { return !Controller->NativeMenu->HasActiveDialog() && Controller->IsBookOpen()
            && Controller->Simulation().Serialize() == *Snapshot; });
    Add(TEXT("Seed one exact-chest target through authority for direct transfer testing"),
        [this, Chest, Group]()
        {
            const int32 PackBranch = Group(0);
            if (!Controller->Sim.TransferGroup(*Chest, PackBranch, 1, true,
                Controller->PlayerPoint(), Controller->Sim.GetRevision()))
            { Finish(false, TEXT("Could not seed the exact chest transfer target.")); return; }
            Controller->CloseBook();
            Controller->OpenChestStorage(*Chest);
        },
        [this, Chest, BranchTotal]() { return Controller->ActiveStorageChest().IsSet()
            && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal - 1
            && Controller->Simulation().ChestUsedCapacity(*Chest) == 1; });
    Add(TEXT("Controller Right crosses from exact Chest grid to nearest Pack tile"),
        [this, Chest, Group]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Group(*Chest), *Chest);
            Tap(EKeys::Gamepad_DPad_Right);
        },
        [this]() { const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Subject && Subject->ContainerId == 0; });
    Add(TEXT("Controller Left reverses from Pack to exact Chest grid"),
        [this]() { Tap(EKeys::Gamepad_DPad_Left); },
        [this, Chest]() { const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Subject && Subject->ContainerId == *Chest; });
    Add(TEXT("Record Pack Branch source and focus exact Chest pointer target"),
        [this, Chest, Group, DragSource]()
        {
            const int32 PackBranch = Group(0);
            const int32 ChestBranch = Group(*Chest);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, PackBranch, 0);
            const auto SourceWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!SourceWidget) { Finish(false, TEXT("Pack Branch drag source is unavailable.")); return; }
            const auto Geometry = SourceWidget->GetCachedGeometry();
            *DragSource = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, ChestBranch, *Chest);
        },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Pointer drag transfers the whole Pack Branch stack into exact Chest"),
        [this, DragSource, DragTarget, PointerDrag]()
        {
            const auto TargetWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!TargetWidget) { Finish(false, TEXT("Chest drag target is unavailable.")); return; }
            const auto Geometry = TargetWidget->GetCachedGeometry();
            *DragTarget = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            PointerDrag(*DragSource, *DragTarget);
        },
        [this, Chest, BranchTotal]() { return Controller->Simulation().Count(Homestead::Item::Branch) == 0
            && Controller->Simulation().ChestUsedCapacity(*Chest) == *BranchTotal
            && !Controller->NativeMenu->IsPointerDraggingItem(); });
    Add(TEXT("Record exact Chest Branch source and focus any Pack pointer target"),
        [this, Chest, Group, DragSource]()
        {
            const int32 ChestBranch = Group(*Chest);
            int32 PackTarget = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId) { PackTarget = Entry.groupId; break; }
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, ChestBranch, *Chest);
            const auto SourceWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!SourceWidget) { Finish(false, TEXT("Chest Branch drag source is unavailable.")); return; }
            const auto Geometry = SourceWidget->GetCachedGeometry();
            *DragSource = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, PackTarget, 0);
        },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Pointer drag returns exact Chest Branch stack to Pack"),
        [this, DragSource, DragTarget, PointerDrag]()
        {
            const auto TargetWidget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!TargetWidget) { Finish(false, TEXT("Pack drag target is unavailable.")); return; }
            const auto Geometry = TargetWidget->GetCachedGeometry();
            *DragTarget = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            PointerDrag(*DragSource, *DragTarget);
        },
        [this, Chest, BranchTotal]() { return Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
            && Controller->Simulation().ChestUsedCapacity(*Chest) == 0
            && !Controller->NativeMenu->IsPointerDraggingItem(); });
    const auto ShiftClickFocused = [this]()
    {
        TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
        auto& Slate = FSlateApplication::Get();
        const auto Widget = Slate.GetKeyboardFocusedWidget();
        if (!Widget) { Finish(false, TEXT("Focused tile is unavailable for Shift+click.")); return; }
        const auto Geometry = Widget->GetCachedGeometry();
        const FVector2D Position = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
        Slate.SetCursorPos(Position);
        Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
            EKeys::Invalid, 0, FModifierKeysState()));
        Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Pressed, 1));
        TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
        Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position, Pressed,
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
        Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
            EKeys::LeftMouseButton, 0, FModifierKeysState()));
        Controller->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::LeftShift, IE_Released, 0));
    };
    Add(TEXT("Focus the Pack Branch stack for Shift+click"),
        [this, Group]() { Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Group(0), 0); },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Shift+click moves the whole Pack Branch stack into the open Chest"),
        [ShiftClickFocused]() { ShiftClickFocused(); },
        [this, Chest, BranchTotal]() { return Controller->Simulation().Count(Homestead::Item::Branch) == 0
            && Controller->Simulation().ChestUsedCapacity(*Chest) == *BranchTotal
            && !Controller->NativeMenu->HasActiveDialog(); });
    Add(TEXT("Focus the Chest Branch stack for Shift+click"),
        [this, Chest, Group]() { Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Group(*Chest), *Chest); },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
    Add(TEXT("Shift+click on the Chest Branch stack returns it to the Pack"),
        [ShiftClickFocused]() { ShiftClickFocused(); },
        [this, Chest, BranchTotal]() { return Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
            && Controller->Simulation().ChestUsedCapacity(*Chest) == 0
            && !Controller->NativeMenu->HasActiveDialog(); });
    Add(TEXT("Seed one exact-chest target then split Pack for partial direct transfer"),
        [this, Chest, Group]()
        {
            const int32 PackBranch = Group(0);
            if (!Controller->Sim.TransferGroup(*Chest, PackBranch, 1, true,
                Controller->PlayerPoint(), Controller->Sim.GetRevision()))
            { Finish(false, TEXT("Could not seed the partial transfer target.")); return; }
            Controller->CloseBook();
            Controller->MenuInventoryView(0);
            Controller->OpenBook(0);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, PackBranch, 0);
            Tap(EKeys::Gamepad_FaceButton_Left);
        },
        [this, Chest, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal - 1
                && Controller->Simulation().ChestUsedCapacity(*Chest) == 1
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 2; });
    Add(TEXT("Virtual drag transfers only the split Pack stack into exact Chest"),
        [this, Chest, BranchTotal, Group]()
        {
            int32 Split = 0;
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Branch
                    && Entry.quantity == (*BranchTotal - 1) / 2) { Split = Entry.groupId; break; }
            Controller->CloseBook();
            Controller->OpenChestStorage(*Chest);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Split, 0);
            Tap(EKeys::Enter);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Group(*Chest), *Chest);
            Tap(EKeys::Enter);
        },
        [this, Chest, BranchTotal]() {
            const int32 Split = (*BranchTotal - 1) / 2;
            return Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal - 1 - Split
                && Controller->Simulation().ChestUsedCapacity(*Chest) == 1 + Split; });
    Add(TEXT("Virtual drag returns partial Chest stack and auto-stacks in Pack"),
        [this, Chest, Group]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Group(*Chest), *Chest);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, Group(0), 0);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Chest, BranchTotal]() { const auto* Layout = Controller->Simulation().GetLayout(0);
            return Layout && Controller->Simulation().Count(Homestead::Item::Branch) == *BranchTotal
                && Controller->Simulation().ChestUsedCapacity(*Chest) == 0
                && std::count_if(Layout->begin(), Layout->end(), [](const Homestead::LayoutEntry& Entry)
                    { return !Entry.wearableId && Entry.item == Homestead::Item::Branch; }) == 1; });
    Add(TEXT("Capture real stored transaction result"),
        [this]() { Screenshot(TEXT("native-storage-transactions")); },
        [this]() { return Controller->BookPage() == 0 && Controller->InventoryView() == 1; }, 0.8f);
    // The hotbar row with a chest open: the first row of her pack, ten cells heading the pack column,
    // holding real stacks (Simulation/HomesteadPackRow.h). A chest stack goes straight into a cell in
    // one transaction; stacks move, merge or swap between cells and the rest of the pack; what she
    // owns in pack and chest together never changes. The stock is restored after.
    {
        const auto SavedSelected = MakeShared<int32>(0);
        const auto PreStrip = MakeShared<std::string>();
        const auto StripBindings = MakeShared<TArray<int32>>();
        const auto HoldingsBefore = MakeShared<TArray<int32>>();
        const auto ChestPasty = MakeShared<int32>(0);
        const auto PackPasty = MakeShared<int32>(0);
        const auto TargetCell = MakeShared<int32>(INDEX_NONE);
        const auto BelowGroup = MakeShared<int32>(0);
        const auto Bindings = [this]()
        {
            TArray<int32> Result;
            for (const auto& Slot : Controller->HotbarSnapshot()) Result.Add(Slot.Assigned ? static_cast<int32>(Slot.Tool) : -1);
            return Result;
        };
        // Everything she owns in her pack and this chest together, per item.
        const auto Holdings = [this, Chest]()
        {
            TArray<int32> Result;
            const auto& State = Controller->State();
            for (int32 Index = 0; Index < Homestead::ItemCount; ++Index)
            {
                int32 Count = State.inventory[Index];
                for (const auto& Piece : State.structures) if (Piece.id == *Chest) Count += Piece.storage[Index];
                Result.Add(Count);
            }
            return Result;
        };
        const auto PastyGroup = [this](int32 Container)
        {
            const auto* Layout = Controller->Simulation().GetLayout(Container);
            if (Layout) for (const auto& Entry : *Layout)
                if (!Entry.wearableId && Entry.item == Homestead::Item::Pasty) return Entry.groupId;
            return 0;
        };
        const auto PastyBelowRow = [this]()
        {
            for (const auto& Entry : Controller->State().inventoryLayout)
                if (!Entry.wearableId && Entry.item == Homestead::Item::Pasty
                    && Homestead::PackRowRules::RowCellOf(Controller->State(), Entry.groupId, 0) < 0) return true;
            return false;
        };
        const auto Center = [](const TSharedPtr<SWidget>& Widget)
        {
            const auto Geometry = Widget->GetCachedGeometry();
            return FVector2D(Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f);
        };
        const auto StripDrag = [this, Center](const TSharedPtr<SWidget>& FromWidget, const TSharedPtr<SWidget>& ToWidget)
        {
            if (!FromWidget || !ToWidget) { Finish(false, TEXT("A hotbar row drag end is unavailable.")); return; }
            const FVector2D From = Center(FromWidget), To = Center(ToWidget);
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            Slate.SetCursorPos(From);
            TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
            Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, From, From, Pressed,
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
            Slate.SetCursorPos(To);
            Slate.ProcessMouseMoveEvent(FPointerEvent(0, To, From, Pressed, EKeys::Invalid, 0, FModifierKeysState()));
            Controller->NativeMenu->PointerItemDragMove(To);
            Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, To, To, TSet<FKey>(),
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
        };
        const auto PastyCell = [this]()
        {
            for (int32 Cell = 0; Cell < Homestead::PackRowSize; ++Cell)
                if (Controller->HotbarItem(Cell) == Homestead::Item::Pasty) return Cell;
            return static_cast<int32>(INDEX_NONE);
        };
        Add(TEXT("Chest view shows the hotbar row as the pack's first row: inside the book, heading the pack column above the grids"),
            [this, SavedSelected, PreStrip]() { *SavedSelected = Controller->SelectedHotbarIndex(); *PreStrip = Controller->Simulation().Serialize(); },
            [this]()
            {
                const auto Menu = Controller->NativeMenu;
                if (!Menu || !Controller->ActiveStorageChest().IsSet() || Menu->GetBookHotbarSlotCount() != 10) return false;
                const FGeometry Book = Menu->GetCachedGeometry();
                const FVector2D BookMin = Book.GetAbsolutePosition(), BookMax = BookMin + Book.GetAbsoluteSize();
                for (int32 Slot = 0; Slot < 10; ++Slot)
                {
                    const auto Widget = Menu->GetBookHotbarSlot(Slot);
                    if (!Widget || !Widget->GetVisibility().IsVisible()) return false;
                    const FGeometry Geometry = Widget->GetCachedGeometry();
                    const FVector2D A = Geometry.GetAbsolutePosition(), B = A + Geometry.GetAbsoluteSize();
                    if (Geometry.GetLocalSize().X < 20 || A.X < BookMin.X || A.Y < BookMin.Y || B.X > BookMax.X || B.Y > BookMax.Y) return false;
                }
                const FGeometry First = Menu->GetBookHotbarSlot(0)->GetCachedGeometry();
                const FGeometry Last = Menu->GetBookHotbarSlot(9)->GetCachedGeometry();
                const float RowBottom = Last.GetAbsolutePosition().Y + Last.GetAbsoluteSize().Y;
                const float RowRight = Last.GetAbsolutePosition().X + Last.GetAbsoluteSize().X;
                Results.Add(FString::Printf(TEXT("GEOMETRY chest-row viewport=%.0fx%.0f row=(%.0f,%.0f)-(%.0f,%.0f) cell=%.0f grids_top=%.0f pack_left=%.0f"),
                    Book.GetAbsoluteSize().X, Book.GetAbsoluteSize().Y, First.GetAbsolutePosition().X, First.GetAbsolutePosition().Y,
                    RowRight, RowBottom, First.GetAbsoluteSize().X, Menu->GetContentScrollTop(), Menu->GetPackColumnLeft()));
                // The first row: above both grids, over the pack column (not the chest's), in one line.
                return RowBottom <= Menu->GetContentScrollTop() + 1.0f
                    && First.GetAbsolutePosition().X >= Menu->GetPackColumnLeft() - 12.0f
                    && FMath::IsNearlyEqual(First.GetAbsolutePosition().Y, Last.GetAbsolutePosition().Y, 1.0f)
                    && First.GetAbsoluteSize().X >= 30.0f * Book.GetAbsoluteSize().Y / 720.0f;
            }, 0.5f);
        Add(TEXT("Disclosed fixture: one pasty below the hotbar row and one in the chest"),
            [this, Chest, PastyGroup, ChestPasty, PackPasty, HoldingsBefore, Holdings]()
            {
                if (!Controller->Sim.GrantItems(Homestead::Item::Pasty, 2)) { Finish(false, TEXT("Could not grant the pasty fixture.")); return; }
                if (!Controller->Sim.TransferGroup(*Chest, PastyGroup(0), 1, true, Controller->PlayerPoint(), Controller->Sim.GetRevision()))
                { Finish(false, TEXT("Could not stow the chest pasty fixture.")); return; }
                // A new stack takes the first empty cell; this fixture wants the pack's pasty below the row.
                const int32 Cell = Homestead::PackRowRules::RowCellOf(Controller->State(), PastyGroup(0), 0);
                if (Cell >= 0 && !Controller->Sim.MoveFromPackRow(Cell, 0, 0, Controller->Sim.GetRevision()))
                { Finish(false, TEXT("Could not move the pack pasty below the row.")); return; }
                Controller->NativeMenu->Refresh();
                *ChestPasty = PastyGroup(*Chest);
                *PackPasty = PastyGroup(0);
                *HoldingsBefore = Holdings();
            },
            [this, ChestPasty, PackPasty, PastyBelowRow]() { return *ChestPasty && *PackPasty && PastyBelowRow()
                && Controller->Simulation().Count(Homestead::Item::Pasty) == 1; });
        Add(TEXT("Capture the chest view's hotbar row"),
            [this]() { Screenshot(TEXT("native-storage-hotbar-strip")); },
            [this]() { return Controller->ActiveStorageChest().IsSet(); }, 0.8f);
        Add(TEXT("Controller A picks up the chest pasty"),
            [this, Chest, ChestPasty]()
            {
                Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *ChestPasty, *Chest);
                Tap(EKeys::Gamepad_FaceButton_Bottom);
            },
            [this]() { return Controller->NativeMenu->IsVirtualDraggingItem(); });
        Add(TEXT("D-pad Up from the chest grid's top row reaches the hotbar row above it"),
            [this]() { Tap(EKeys::Gamepad_DPad_Up); },
            [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Hotbar")
                && Controller->NativeMenu->HasSynchronizedFocus(); });
        Steps.Last().Repeat = [this]()
        {
            if (Controller->NativeMenu && Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content")) Tap(EKeys::Gamepad_DPad_Up);
        };
        Add(TEXT("A on a cell takes the chest pasty straight into it in one step; nothing is lost or duplicated"),
            [this, TargetCell]() { *TargetCell = Controller->NativeMenu->GetFocusedHotbarSlot(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
            [this, TargetCell, Chest, Holdings, HoldingsBefore, SavedSelected]()
            {
                return !Controller->NativeMenu->IsVirtualDraggingItem() && *TargetCell != INDEX_NONE
                    && Controller->HotbarItem(*TargetCell) == Homestead::Item::Pasty
                    && Controller->Simulation().Count(Homestead::Item::Pasty) == 2
                    && Controller->Simulation().ChestUsedCapacity(*Chest) >= 0 && Holdings() == *HoldingsBefore
                    && Controller->SelectedHotbarIndex() == *SavedSelected;
            });
        Add(TEXT("Down returns to the grids"),
            [this]() { Tap(EKeys::Gamepad_DPad_Down); },
            [this]() { return Controller->NativeMenu->GetFocusedRegionName() == TEXT("Content"); });
        Add(TEXT("Move to a hotbar slot, then A on the chest pasty's cell merges the pack pasty into it"),
            [this, PackPasty, TargetCell]()
            {
                for (const auto& Row : Controller->MenuRows())
                    if (Row.Subject == EHomesteadMenuSubject::ItemGroup && Row.SubjectId == *PackPasty)
                    { Controller->NativeMenu->BeginPlacingOnHotbar(Row); break; }
                // Walk the focus to the pasty's cell and put it down there.
                const int32 From = Controller->NativeMenu->GetFocusedHotbarSlot();
                for (int32 Step = From; Step < *TargetCell; ++Step) Tap(EKeys::Gamepad_DPad_Right);
                for (int32 Step = From; Step > *TargetCell; --Step) Tap(EKeys::Gamepad_DPad_Left);
                Tap(EKeys::Gamepad_FaceButton_Bottom);
            },
            [this, TargetCell, Holdings, HoldingsBefore, SavedSelected, PastyBelowRow]()
            {
                const auto* Entry = Controller->HotbarEntry(*TargetCell);
                return !Controller->NativeMenu->IsPlacingOnHotbar() && Entry && Entry->item == Homestead::Item::Pasty
                    && Entry->quantity == 2 && !PastyBelowRow()
                    && Controller->SelectedHotbarIndex() == *SavedSelected && Holdings() == *HoldingsBefore;
            });
        Add(TEXT("A pasty's hover text gives its Energy and no internal stack number"),
            []() {},
            [this, TargetCell]()
            {
                FHomesteadRow Row;
                const bool Found = Controller->MenuHotbarRow(*TargetCell, Row);
                Results.Add(TEXT("FOOD_HOVER ") + Row.Detail.Replace(TEXT("\n"), TEXT(" | ")));
                return Found && Row.Detail.Contains(TEXT("+40 Energy")) && !Row.Detail.Contains(TEXT("Stack #"))
                    && Row.Detail.Contains(TEXT(": 2"));
            });
        Add(TEXT("Capture the chest view's hotbar row holding the pasties"),
            [this]() { Screenshot(TEXT("native-storage-hotbar-row-filled")); },
            [this]() { return Controller->ActiveStorageChest().IsSet(); }, 0.8f);
        Add(TEXT("Y on the cell offers the stack's own options, Move into the pack among them"),
            [this, Bindings, StripBindings]() { *StripBindings = Bindings(); Tap(EKeys::Gamepad_FaceButton_Top); },
            [this]()
            {
                if (!Controller->NativeMenu->IsItemContextMenu()) return false;
                for (int32 Option = 0; Option < 8; ++Option)
                    if (Controller->NativeMenu->GetPopupOptionLabel(Option) == TEXT("Move into the pack")) return true;
                return false;
            });
        Add(TEXT("B closes it with the row unchanged"),
            [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
            [this, Bindings, StripBindings]() { return !Controller->NativeMenu->HasActiveDialog() && Controller->IsBookOpen()
                && Bindings() == *StripBindings; });
        Add(TEXT("Focus a pack stack below the row for a mouse drag"),
            [this, BelowGroup]()
            {
                *BelowGroup = 0;
                for (const auto& Row : Controller->MenuRows())
                    if (Row.Subject == EHomesteadMenuSubject::ItemGroup && Row.ContainerId == 0 && Row.HotbarCell < 0
                        && Row.Id != static_cast<int32>(Homestead::Item::Pasty)) { *BelowGroup = Row.SubjectId; break; }
                if (!*BelowGroup) { Finish(false, TEXT("No pack stack below the row to drag against.")); return; }
                Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *BelowGroup, 0);
            },
            [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
        Add(TEXT("Mouse drags the pasty's cell onto that stack below the row; the two swap places"),
            [this, StripDrag, TargetCell]()
            {
                StripDrag(Controller->NativeMenu->GetBookHotbarSlot(*TargetCell), FSlateApplication::Get().GetKeyboardFocusedWidget());
            },
            [this, TargetCell, BelowGroup, Holdings, HoldingsBefore, PastyBelowRow]()
            {
                const auto* Entry = Controller->HotbarEntry(*TargetCell);
                return Entry && Entry->groupId == *BelowGroup && PastyBelowRow() && Holdings() == *HoldingsBefore
                    && Controller->NativeMenu->GetHeldHotbarSlot() == INDEX_NONE;
            });
        Add(TEXT("Focus the pasty below the row"),
            [this, PastyGroup]() { Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, PastyGroup(0), 0); },
            [this]() { return Controller->NativeMenu->HasSynchronizedFocus(); });
        Add(TEXT("Mouse drags the pasty from the pack grid onto cell 1; it takes that cell (moving or swapping)"),
            [this, StripDrag]() { StripDrag(FSlateApplication::Get().GetKeyboardFocusedWidget(), Controller->NativeMenu->GetBookHotbarSlot(0)); },
            [this, Holdings, HoldingsBefore, PastyBelowRow]()
            {
                return !Controller->NativeMenu->IsPointerDraggingItem() && Controller->HotbarItem(0) == Homestead::Item::Pasty
                    && !PastyBelowRow() && Holdings() == *HoldingsBefore;
            });
        Add(TEXT("Mouse drags cell 1 onto cell 10 (key 0); the two swap"),
            [this, StripDrag, Bindings, StripBindings]()
            {
                *StripBindings = Bindings();
                StripDrag(Controller->NativeMenu->GetBookHotbarSlot(0), Controller->NativeMenu->GetBookHotbarSlot(9));
            },
            [this, Bindings, StripBindings, Holdings, HoldingsBefore]()
            {
                const auto Now = Bindings();
                return Now[0] == (*StripBindings)[9] && Now[9] == (*StripBindings)[0]
                    && Controller->NativeMenu->GetHeldHotbarSlot() == INDEX_NONE && Holdings() == *HoldingsBefore;
            });
        Add(TEXT("The edited row reloads from its saved form unchanged"),
            [this, Bindings, StripBindings]()
            {
                *StripBindings = Bindings();
                if (!Controller->Sim.Deserialize(Controller->Sim.Serialize())) Finish(false, TEXT("The edited row did not reload."));
            },
            [this, Bindings, StripBindings]() { return Bindings() == *StripBindings; });
        Add(TEXT("Restore the fixture's stock and the row as it was"),
            [this, SavedSelected, PreStrip]()
            {
                if (!Controller->Sim.Deserialize(*PreStrip)) { Finish(false, TEXT("Could not restore the pre-strip stock.")); return; }
                Controller->SelectedHotbarSlot = *SavedSelected;
                Controller->ToastText.Reset(); Controller->bToastError = false; Controller->ToastRemaining = 0;
                Controller->NativeMenu->Refresh();
            },
            [this, PreStrip]() { return Controller->Simulation().Serialize() == *PreStrip; });
    }    Add(TEXT("Prepare valid full Pack and Chest scrolling fixture"),
        [this, Chest, FullSnapshot, FullPackGroup]()
        {
            *FullSnapshot = Controller->Simulation().Serialize();
            auto& State = const_cast<Homestead::State&>(Controller->Sim.GetState());
            Homestead::Structure* Storage = nullptr;
            for (auto& Piece : State.structures) if (Piece.id == *Chest) { Storage = &Piece; break; }
            if (!Storage) { Finish(false, TEXT("Exact chest disappeared before full-grid fixture.")); return; }
            State.inventory.fill(0);
            State.inventory[static_cast<int32>(Homestead::Item::Billhook)] = 1;
            State.inventory[static_cast<int32>(Homestead::Item::Branch)] = 119;
            State.inventoryLayout.clear();
            State.packRow = {};
            Storage->storage.fill(0);
            Storage->storage[static_cast<int32>(Homestead::Item::Stone)] = 120;
            Storage->layout.clear();
            int32 GroupId = State.nextGroupId;
            State.inventoryLayout.push_back({GroupId++, Homestead::Item::Billhook, 1, 0});
            for (int32 Index = 0; Index < 119; ++Index)
            {
                if (Index == 0) *FullPackGroup = GroupId;
                State.inventoryLayout.push_back({GroupId++, Homestead::Item::Branch, 1, 0});
            }
            for (int32 Index = 0; Index < 120; ++Index)
                Storage->layout.push_back({GroupId++, Homestead::Item::Stone, 1, 0});
            State.nextGroupId = GroupId;
            const auto Reloaded = Controller->Sim.Deserialize(Controller->Sim.Serialize());
            if (!Reloaded)
            { Finish(false, TEXT("Full-grid fixture did not satisfy current save validation.")); return; }
            Controller->NativeMenu->Refresh();
        },
        [this, Chest]() { return Controller->Simulation().UsedCapacity() == 120
            && Controller->Simulation().ChestUsedCapacity(*Chest) == 120
            && Controller->MenuRows().Num() == 240; });
    Add(TEXT("Capture full scrolling Chest and Pack grids"),
        [this]() { Screenshot(TEXT("native-storage-full")); },
        [this]() { return Controller->NativeMenu->IsFocusedControlVisible(); }, 0.8f);
    Add(TEXT("Pointer edge drag autoscrolls without committing an invalid drop"),
        [this, FullPackGroup]()
        {
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *FullPackGroup, 0);
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Full Pack drag source is unavailable.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            const FVector2D From = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            const FVector2D To(From.X, Controller->NativeMenu->GetContentScrollBottom() - 2.0f);
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            Slate.SetCursorPos(From);
            TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
            Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, From, From, Pressed,
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
            for (int32 Index = 0; Index < 12; ++Index)
            {
                Slate.ProcessMouseMoveEvent(FPointerEvent(0, To, From, Pressed,
                    EKeys::Invalid, 0, FModifierKeysState()));
                Controller->NativeMenu->PointerItemDragMove(To);
            }
            Slate.SetCursorPos(FVector2D(2, 2));
            Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, FVector2D(2, 2), To, TSet<FKey>(),
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
        },
        [this]() { return Controller->NativeMenu->GetContentScrollOffset() > 0
            && !Controller->NativeMenu->IsPointerDraggingItem()
            && Controller->Simulation().UsedCapacity() == 120; });
    Add(TEXT("Restore exact pre-fixture inventory and chest state"),
        [this, Chest, FullSnapshot]()
        {
            if (!Controller->Sim.Deserialize(*FullSnapshot))
            { Finish(false, TEXT("Could not restore pre-fixture inventory state.")); return; }
            Controller->CloseBook();
            Controller->OpenChestStorage(*Chest);
            Tap(EKeys::Gamepad_DPad_Left);
        },
        [this, Chest]() { return Controller->Simulation().UsedCapacity() < 120
            && Controller->Simulation().ChestUsedCapacity(*Chest) < 120; });
    Add(TEXT("Repeated menu open-close rebuilds one valid shell and preserves committed state"),
        [this, Snapshot]()
        {
            *Snapshot = Controller->Simulation().Serialize();
            for (int32 Index = 0; Index < 20; ++Index)
            {
                Controller->CloseBook();
                Controller->MenuInventoryView(Index % 3);
                Controller->OpenBook(0);
            }
        },
        [this, Snapshot]() { return Controller->NativeMenu.IsValid() && Controller->IsBookOpen()
            && Controller->NativeMenu->HasSynchronizedFocus()
            && Controller->Simulation().Serialize() == *Snapshot && VerifyNativeMenuPresentation(); }, 1.0f);
}

void AHomesteadSmokeTest::PrepareNativeResetChecks()
{
    const auto Before = MakeShared<std::string>();
    const auto World = MakeShared<FString>();
    const auto Incompatible = MakeShared<FString>();
    Add(TEXT("Invalid ownership candidate is rejected before live mutation"),
        [this, Before]()
        {
            *Before = Controller->Simulation().Serialize();
            Homestead::Simulation Invalid = Controller->Simulation();
            auto& State = const_cast<Homestead::State&>(Invalid.GetState());
            State.wearables[0].owner = static_cast<Homestead::WearableOwner>(99);
            auto* Save = Cast<UHomesteadSave>(UGameplayStatics::CreateSaveGameObject(UHomesteadSave::StaticClass()));
            Save->WorldId = Controller->WorldId;
            Save->SimulationData = UTF8_TO_TCHAR(Invalid.Serialize().c_str());
            Save->SavedAtUtc = FDateTime::UtcNow().ToUnixTimestamp();
            const FString Path = Controller->SavePath(TEXT("invalid-owner"));
            if (!HomesteadNativeMenuProof::WriteEnvelope(*Save, Path)
                || Controller->ReadSave(Path) || !IFileManager::Get().Delete(*Path))
                Finish(false, TEXT("Invalid ownership fixture was not rejected and cleaned."));
        },
        [this, Before]() { return Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("Only incompatible sandbox save triggers explicit reset without mutation"),
        [this, Before, World, Incompatible]()
        {
            *Before = Controller->Simulation().Serialize();
            *World = Controller->WorldId;
            for (const FString& Slot : {TEXT("Homestead_Manual"), TEXT("Homestead_Auto_0"),
                TEXT("Homestead_Auto_1"), TEXT("Homestead_Auto_2"), TEXT("Homestead_Recovery")})
                for (const FString& Suffix : {FString(), FString(TEXT(".bak"))})
                    IFileManager::Get().Delete(*(Controller->SavePath(Slot) + Suffix), false, true);
            auto* Save = Cast<UHomesteadSave>(UGameplayStatics::CreateSaveGameObject(UHomesteadSave::StaticClass()));
            Save->Version = UHomesteadSave::CurrentVersion - 1;
            Save->WorldId = *World; Save->SimulationData = UTF8_TO_TCHAR(Before->c_str());
            Save->SavedAtUtc = FDateTime::UtcNow().ToUnixTimestamp();
            *Incompatible = Controller->SavePath(TEXT("Homestead_Manual"));
            if (!HomesteadNativeMenuProof::WriteEnvelope(*Save, *Incompatible))
            { Finish(false, TEXT("Could not write incompatible fixture.")); return; }
            Controller->bHasPlayableSession = false;
            Controller->QuickLoad();
            Controller->NativeMenu->Refresh();
        },
        [this, Before]() { return Controller->MenuNeedsTestReset()
            && Controller->Simulation().Serialize() == *Before && Controller->NativeMenu->IsTestResetPrompt(); });
    Add(TEXT("Capture explicit incompatible test reset confirmation"),
        [this]() { Screenshot(TEXT("native-test-reset")); },
        [this]() { return Controller->NativeMenu->IsTestResetPrompt(); }, 0.8f);
    Add(TEXT("Reset cancel preserves state and incompatible test file"),
        [this]() { Tap(EKeys::Enter); },
        [this, Before, Incompatible]() { return Controller->MenuNeedsTestReset()
            && !Controller->NativeMenu->HasActiveDialog() && Controller->Simulation().Serialize() == *Before
            && IFileManager::Get().FileExists(**Incompatible); });
    Add(TEXT("Explicit reset is separately requested after cancel"),
        [this]() { Controller->QuickLoad(); Controller->NativeMenu->Refresh(); },
        [this]() { return Controller->NativeMenu->IsTestResetPrompt(); });
    Add(TEXT("Confirmed reset replaces, never merges, and keeps old test file"),
        [this]() { Tap(EKeys::Down); Tap(EKeys::Enter); },
        [this, Before, World, Incompatible]() { return !Controller->MenuNeedsTestReset()
            && Controller->WorldId != *World && Controller->Simulation().Serialize() != *Before
            && Controller->Simulation().UsedCapacity() == 0
            && Controller->State().wearables.size() == 1
            && IFileManager::Get().FileExists(**Incompatible); });
}

void AHomesteadSmokeTest::PrepareNativePresentationCoverageChecks()
{
    const auto Original = MakeShared<Homestead::Simulation>();
    const auto BaseOnly = MakeShared<Homestead::Simulation>();
    const auto OriginalLook = MakeShared<FHomesteadAppearance>();
    const auto Layered = MakeShared<Homestead::Simulation>();
    const auto CoverageReady = MakeShared<bool>(false);
    Add(TEXT("Validate all nine permanent modest bases and complete feet before live mutation"),
        [this, Original, BaseOnly, Layered, OriginalLook, CoverageReady]()
        {
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            if (!Avatar) { Finish(false, TEXT("Heroine unavailable for base coverage.")); return; }
            *Original = Controller->Simulation();
            *BaseOnly = *Original;
            *OriginalLook = Controller->GetAppearance();
            TSet<int32> Equipped;
            for (int32 Id : BaseOnly->GetState().equipment) if (Id) Equipped.Add(Id);
            for (int32 Id : Equipped)
                if (BaseOnly->GetWearable(Id)
                    && !BaseOnly->UnequipWearable(Id, BaseOnly->GetRevision()))
                { Finish(false, TEXT("Could not create base-only state through authority.")); return; }
            bool Ready = BaseOnly->GetState().equipment == (std::array<int, Homestead::EquipmentSlotCount>{});
            for (int32 Body = 0; Body < 3 && Ready; ++Body)
                for (int32 Hair = 0; Hair < 3 && Ready; ++Hair)
                {
                    FHomesteadAppearance Look = *OriginalLook;
                    Look.BodyPreset = Body; Look.HairStyle = Hair;
                    FString Error;
                    Ready &= Avatar->PrepareEquipment(BaseOnly->GetState(), Look, Error)
                        && Avatar->ApplyPreparedEquipment(Error);
                    const auto* Presentation = Avatar->GetEquipmentPresentation();
                    Ready &= Presentation && Presentation->Ready && Presentation->Garments.IsEmpty()
                        && Presentation->Base.Mesh
                        && (Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("foot_l")) >= 0
                            || Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("foot.L")) >= 0
                            || Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("LeftFoot")) >= 0)
                        && (Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("ball_l")) >= 0
                            || Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("toe_l")) >= 0
                            || Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("toe.L")) >= 0
                            || Presentation->Base.Mesh->GetRefSkeleton().FindBoneIndex(TEXT("LeftToeBase")) >= 0);
                    bool Bra = false, Briefs = false;
                    if (Presentation && Presentation->Base.Mesh)
                        for (const auto& Slot : Presentation->Base.Mesh->GetMaterials())
                        {
                            Bra |= Slot.MaterialSlotName == TEXT("M_Modular_BaseBra");
                            Briefs |= Slot.MaterialSlotName == TEXT("M_Modular_BaseBriefs");
                        }
                    Ready &= Bra && Briefs;
                    if (!Ready)
                    {
                        Finish(false, FString::Printf(TEXT("Base coverage failed for body %d hair %d: %s"),
                            Body, Hair, *Error));
                        return;
                    }
                }
            *Layered = *Original;
            // Reed fibre and the knife left new games (garments become the dressmaker's, round 3);
            // garment coverage keeps the old stock and tool as stand-in grants.
            if (Layered->Count(Homestead::Item::Knife) == 0 && !Layered->GrantItems(Homestead::Item::Knife, 1))
            {
                Finish(false, TEXT("Layered coverage could not hold the stand-in knife."));
                return;
            }
            if (Layered->Count(Homestead::Item::Fiber) < 14
                && !Layered->GrantItems(Homestead::Item::Fiber, 14 - Layered->Count(Homestead::Item::Fiber)))
            {
                Finish(false, TEXT("Layered coverage could not hold 14 stand-in Fiber."));
                return;
            }
            if (Ready)
            {
                const auto ApronCraft = Layered->CraftGarment(Homestead::WearableDefinition::LinenApron,
                    Controller->PlayerPoint(), Layered->GetRevision());
                const auto FootwrapCraft = Layered->CraftGarment(Homestead::WearableDefinition::WovenFootwraps,
                    Controller->PlayerPoint(), Layered->GetRevision());
                if (!ApronCraft.ok || !FootwrapCraft.ok)
                {
                    Finish(false, FString::Printf(TEXT("Layered authority craft failed: %s / %s"),
                        UTF8_TO_TCHAR(ApronCraft.message.c_str()), UTF8_TO_TCHAR(FootwrapCraft.message.c_str())));
                    return;
                }
                int32 Apron = 0, Footwraps = 0;
                for (const auto& Item : Layered->GetState().wearables)
                {
                    if (Item.definition == Homestead::WearableDefinition::LinenApron) Apron = Item.id;
                    if (Item.definition == Homestead::WearableDefinition::WovenFootwraps) Footwraps = Item.id;
                }
                Ready &= Apron > 0 && Footwraps > 0
                    && Layered->EquipWearable(Apron, Layered->GetRevision()).ok
                    && Layered->EquipWearable(Footwraps, Layered->GetRevision()).ok
                    && Layered->RecolorWearable(Apron, 2, Controller->PlayerPoint(), Layered->GetRevision()).ok;
                FHomesteadAppearance Look = *OriginalLook;
                Look.BodyPreset = 1; Look.HairStyle = 1;
                FString Error;
                Ready &= Avatar->PrepareEquipment(Layered->GetState(), Look, Error)
                    && Avatar->ApplyPreparedEquipment(Error);
                const auto* Presentation = Avatar->GetEquipmentPresentation();
                Ready &= Presentation && Presentation->Garments.Num() == 3;
                if (Presentation)
                    for (const auto& Surface : Presentation->Garments)
                    {
                        const auto* Owned = Layered->GetWearable(Surface.WearableId);
                        Ready &= Owned && Owned->owner == Homestead::WearableOwner::Equipped
                            && Surface.Definition == static_cast<int32>(Owned->definition)
                            && Surface.Dye == Owned->dye;
                    }
                if (!Ready)
                {
                    Finish(false, TEXT("Layered tunic/apron/footwrap identity, slot, dye, or renderable parity failed."));
                    return;
                }
            }
            Homestead::Simulation Invalid = *Original;
            auto& InvalidState = const_cast<Homestead::State&>(Invalid.GetState());
            InvalidState.wearables[0].definition = static_cast<Homestead::WearableDefinition>(99);
            const std::string Before = Controller->Simulation().Serialize();
            FString Error;
            Ready &= !Avatar->PrepareEquipment(Invalid.GetState(), *OriginalLook, Error)
                && Controller->Simulation().Serialize() == Before;
            if (!Ready)
            {
                Finish(false, TEXT("Invalid wearable candidate was not rejected atomically."));
                return;
            }
            Error.Reset();
            if (!Avatar->PrepareEquipment(Original->GetState(), *OriginalLook, Error)
                || !Avatar->ApplyPreparedEquipment(Error))
            {
                Finish(false, TEXT("Could not restore live presentation after isolated coverage: ") + Error);
                return;
            }
            *CoverageReady = Ready;
        },
        [CoverageReady]() { return *CoverageReady; });
    for (int32 Body = 0; Body < 3; ++Body)
    {
        Add(FString::Printf(TEXT("Apply base-only body preset %d through prepared presentation"), Body),
            [this, BaseOnly, OriginalLook, Body]()
            {
                Controller->Sim = *BaseOnly;
                Controller->Appearance = *OriginalLook;
                Controller->Appearance.BodyPreset = Body;
                Controller->Appearance.HairStyle = Body;
                auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                FString Error;
                if (!Avatar || !Avatar->PrepareEquipment(Controller->State(), Controller->Appearance, Error)
                    || !Avatar->ApplyPreparedEquipment(Error))
                { Finish(false, TEXT("Base-only preset could not be displayed: ") + Error); return; }
                Controller->CloseBook(); Controller->MenuInventoryView(2); Controller->OpenBook(0);
                Controller->RefreshMenuPortrait();
            },
            [this]() { const auto* Presentation = Cast<AHomesteadCharacter>(
                Controller->GetPawn())->GetEquipmentPresentation();
                return Presentation && Presentation->Ready && Presentation->Garments.IsEmpty()
                    && Controller->MenuPortraitBrush() != nullptr; }, 0.8f);
        Add(FString::Printf(TEXT("Capture base-only supported preset %d"), Body),
            [this, Body]() { Screenshot(FString::Printf(TEXT("native-base-only-%d"), Body)); },
            [this]() { return Controller->MenuPortraitBrush() != nullptr; }, 0.8f);
    }
    const TCHAR* HairLabels[] = {TEXT("wave"), TEXT("bob-blonde")};
    const TCHAR* ViewLabels[] = {TEXT("back"), TEXT("three-quarter"), TEXT("side")};
    const float ViewAngles[] = {180.0f, 135.0f, 90.0f};
    for (int32 Hair = 0; Hair < 2; ++Hair)
        for (int32 Body = 0; Body < 3; ++Body)
            for (int32 View = 0; View < 3; ++View)
            {
                const FString Name = FString::Printf(TEXT("native-hair-%s-body%d-%s"),
                    HairLabels[Hair], Body, ViewLabels[View]);
                Add(TEXT("Apply real close hairstyle review: ") + Name,
                    [this, Original, OriginalLook, Hair, Body, ViewAngles, View]()
                    {
                        Controller->Sim = *Original;
                        Controller->Appearance = *OriginalLook;
                        Controller->Appearance.BodyPreset = Body;
                        Controller->Appearance.HairStyle = Hair;
                        Controller->Appearance.HairColor = Hair == 1 ? 4 : 0;
                        auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
                        FString Error;
                        if (!Avatar || !Avatar->PrepareEquipment(Controller->State(), Controller->Appearance, Error)
                            || !Avatar->ApplyPreparedEquipment(Error))
                        { Finish(false, TEXT("Close hairstyle review could not apply production presentation: ") + Error); return; }
                        Controller->CloseBook(); Controller->MenuInventoryView(2); Controller->OpenBook(0);
                        Controller->RefreshMenuPortrait();
                        Controller->ZoomMenuPortrait();
                        Controller->OrbitMenuPortrait(ViewAngles[View]);
                    },
                    [this, Hair, Body]()
                    {
                        const auto* Presentation = Cast<AHomesteadCharacter>(
                            Controller->GetPawn())->GetEquipmentPresentation();
                        const TCHAR* Bodies[] = {TEXT("Preferred"), TEXT("Willow"), TEXT("Hazel")};
                        const TCHAR* Styles[] = {TEXT("LongWave"), TEXT("Bob")};
                        const FString Expected = FString::Printf(TEXT("SK_Modular_%s_Base_%s"), Bodies[Body], Styles[Hair]);
                        return Presentation && Presentation->Base.Mesh
                            && Presentation->Base.Mesh->GetName() == Expected
                            && Presentation->Garments.Num() == 1
                            && Controller->MenuPortraitBrush() != nullptr
                            && VerifyNativeMenuPresentation();
                    }, 0.8f);
                Add(TEXT("Capture real close hairstyle review: ") + Name,
                    [this, Name]() { Screenshot(Name); },
                    [this]() { return Controller->MenuPortraitBrush() != nullptr; }, 0.8f);
            }
    Add(TEXT("Apply representative layered equipment to shared gameplay and portrait presentation"),
        [this, Layered, OriginalLook]()
        {
            Controller->Sim = *Layered;
            Controller->Appearance = *OriginalLook;
            Controller->Appearance.BodyPreset = 1; Controller->Appearance.HairStyle = 1;
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            FString Error;
            if (!Avatar || !Avatar->PrepareEquipment(Controller->State(), Controller->Appearance, Error)
                || !Avatar->ApplyPreparedEquipment(Error))
            { Finish(false, TEXT("Layered equipment could not be applied: ") + Error); return; }
            Controller->MenuInventoryView(2); Controller->OpenBook(0);
            Controller->RefreshMenuPortrait();
        },
        [this]()
        {
            const auto* Presentation = Cast<AHomesteadCharacter>(
                Controller->GetPawn())->GetEquipmentPresentation();
            return Presentation && Presentation->Garments.Num() == 3
                && Controller->MenuPortraitBrush() != nullptr && VerifyNativeMenuPresentation();
        }, 0.8f);
    Add(TEXT("Capture real tunic apron and footwrap presentation"),
        [this]() { Screenshot(TEXT("native-wardrobe-layered")); },
        [this]() { return Controller->MenuPortraitBrush() != nullptr
            && VerifyNativeMenuPresentation(); }, 0.8f);
    Add(TEXT("Representative layered equipment stays bound through walk"),
        [this]()
        {
            Controller->CloseBook();
            Axis(EKeys::Gamepad_LeftY, 1);
        },
        [this]() { return Controller->GetPawn()->GetVelocity().Size2D() > 20
            && VerifyNativeMenuPresentation(); }, 0.6f);
    Steps.Last().Repeat = [this]() { Axis(EKeys::Gamepad_LeftY, 1); };
    Add(TEXT("Layered equipment remains exact during gather/water/weed/clear presentation"),
        [this]()
        {
            Axis(EKeys::Gamepad_LeftY, 0);
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            Avatar->PlayGather(); Avatar->PlayWater(); Avatar->PlayClear();
        },
        [this]() { return VerifyNativeMenuPresentation(); }, 0.6f);
    Add(TEXT("Restore exact committed equipment after base-only coverage and clear portrait resources"),
        [this, Original, OriginalLook]()
        {
            Controller->Sim = *Original;
            Controller->Appearance = *OriginalLook;
            auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
            FString Error;
            if (!Avatar || !Avatar->PrepareEquipment(Controller->State(), Controller->Appearance, Error)
                || !Avatar->ApplyPreparedEquipment(Error))
            { Finish(false, TEXT("Could not restore committed equipment: ") + Error); return; }
            Controller->CloseBook();
        },
        [this, Original]()
        {
            const auto& Current = Controller->State();
            const auto& Expected = Original->GetState();
            if (Current.equipment != Expected.equipment
                || Current.wearables.size() != Expected.wearables.size()) return false;
            for (const auto& Item : Expected.wearables)
            {
                const auto* Restored = Controller->Simulation().GetWearable(Item.id);
                if (!Restored || Restored->definition != Item.definition || Restored->owner != Item.owner
                    || Restored->chestId != Item.chestId || Restored->dye != Item.dye) return false;
            }
            return !Controller->NativeMenu.IsValid() && !Controller->MenuPortrait
                && VerifyNativeMenuPresentation();
        });
    Add(TEXT("Reopen one fresh native shell after presentation cleanup"),
        [this]() { Controller->OpenBook(0); },
        [this]() { return Controller->IsBookOpen() && Controller->NativeMenu.IsValid()
            && Controller->NativeMenu->HasSynchronizedFocus() && VerifyNativeMenuPresentation(); });
    const auto DropGroup = MakeShared<int32>(0);
    const auto DropItem = MakeShared<Homestead::Item>(Homestead::Item::Count);
    const auto DropCount = MakeShared<int32>(0);
    const auto DropId = MakeShared<int32>(0);
    const auto DropPlayerLocation = MakeShared<FVector>(FVector::ZeroVector);
    const auto DropPlayerRotation = MakeShared<FRotator>(FRotator::ZeroRotator);
    const auto DropWindowLifecycle = MakeShared<bool>(false);
    Add(TEXT("Select a carried stack with contextual Drop"),
        [this, DropGroup, DropItem, DropCount]()
        {
            Controller->MenuInventoryView(0);
            Controller->CloseBook();
            Controller->OpenBook(0);
            for (const auto& Entry : *Controller->Simulation().GetLayout(0))
                if (!Entry.wearableId && Entry.item == Homestead::Item::Billhook && Entry.quantity == 1)
                {
                    *DropGroup = Entry.groupId;
                    *DropItem = Entry.item;
                    *DropCount = Controller->Simulation().Count(Entry.item);
                    break;
                }
            if (!*DropGroup
                || !Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::ItemGroup, *DropGroup, 0)
                || !Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Drop))
                Finish(false, TEXT("No carried stack exposes the contextual Drop action."));
        },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus()
            && Controller->NativeMenu->IsItemContextMenu(); });
    Add(TEXT("Controller cancel leaves exact carried and world totals unchanged"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, DropItem, DropCount]() { return !Controller->NativeMenu->HasActiveDialog()
            && Controller->Simulation().Count(*DropItem) == *DropCount
            && Controller->State().worldDrops.empty(); });
    Add(TEXT("Reopen the item menu on Drop for a real pointer choice"),
        [this]() { Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Drop); },
        [this]() { return Controller->NativeMenu->HasSynchronizedFocus()
            && Controller->NativeMenu->IsItemContextMenu(); });
    Add(TEXT("Pointer Drop in the item menu drops exactly one item"),
        [this]()
        {
            const auto Widget = FSlateApplication::Get().GetKeyboardFocusedWidget();
            if (!Widget) { Finish(false, TEXT("Focused Drop option has no pointer target.")); return; }
            const auto Geometry = Widget->GetCachedGeometry();
            const FVector2D Position = Geometry.GetAbsolutePosition() + Geometry.GetAbsoluteSize() * 0.5f;
            TGuardValue<bool> Admission(Controller->bSimulatedMenuEvent, true);
            auto& Slate = FSlateApplication::Get();
            const FVector2D Previous = Slate.GetCursorPos();
            Slate.SetCursorPos(Position);
            Slate.ProcessMouseMoveEvent(FPointerEvent(0, Position, Previous, TSet<FKey>(),
                EKeys::Invalid, 0, FModifierKeysState()));
            TSet<FKey> Pressed; Pressed.Add(EKeys::LeftMouseButton);
            Slate.ProcessMouseButtonDownEvent(nullptr, FPointerEvent(0, Position, Position, Pressed,
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
            Slate.ProcessMouseButtonUpEvent(FPointerEvent(0, Position, Position, TSet<FKey>(),
                EKeys::LeftMouseButton, 0, FModifierKeysState()));
        },
        [this, DropItem, DropCount, DropId]()
        {
            if (Controller->Simulation().Count(*DropItem) != *DropCount - 1
                || Controller->State().worldDrops.size() != 1) return false;
            // The hotbar holds real stacks: nothing is kept in the row for a dropped tool.
            if (Controller->HotbarCellOf(Homestead::Item::Billhook) != INDEX_NONE) return false;
            *DropId = Controller->State().worldDrops.front().id;
            return *DropId > 0;
        });
    Add(TEXT("Dropped item creates one low nonblocking camera-safe token"),
        [this, DropPlayerLocation, DropPlayerRotation]()
        {
            Controller->CloseBook();
            if (!Controller->State().worldDrops.empty())
            {
                const auto& Drop = Controller->State().worldDrops.front();
                *DropPlayerLocation = Controller->GetPawn()->GetActorLocation();
                *DropPlayerRotation = Controller->GetPawn()->GetActorRotation();
                bool Positioned = false;
                for (int32 Index = 0; Index < 16 && !Positioned; ++Index)
                {
                    const float Angle = Index * 360.0f / 16.0f;
                    const FVector Away = FRotator(0, Angle, 0).Vector();
                    const float X = Drop.position.x + Away.X * 80;
                    const float Y = Drop.position.y + Away.Y * 80;
                    const float Ground = Controller->GroundHeight(X, Y);
                    FCollisionQueryParams Params(SCENE_QUERY_STAT(HomesteadDropProofView), false,
                        Controller->GetPawn());
                    const FVector Body(X, Y, Ground + 90);
                    const FVector Camera = Body + Away * 340 + FVector(0, 0, 90);
                    const FVector Target(Drop.position.x, Drop.position.y,
                        Controller->GroundHeight(Drop.position.x, Drop.position.y) + 32);
                    bool StructureBlocksView = false;
                    for (const auto& Structure : Controller->State().structures)
                    {
                        const auto Center = Homestead::StructureCenter(Controller->State(), Structure);
                        if (FMath::PointDistToSegment(
                            FVector(Center.x, Center.y, 0),
                            FVector(Camera.X, Camera.Y, 0),
                            FVector(Target.X, Target.Y, 0)) < 150)
                        { StructureBlocksView = true; break; }
                    }
                    if (Controller->GetWorld()->OverlapAnyTestByChannel(Body, FQuat::Identity,
                            ECC_WorldStatic, FCollisionShape::MakeSphere(38), Params)
                        || Controller->GetWorld()->LineTraceTestByChannel(Camera, Target,
                            ECC_WorldStatic, Params) || StructureBlocksView)
                        continue;
                    Controller->GetPawn()->SetActorLocation(FVector(X, Y, Ground + 100),
                        false, nullptr, ETeleportType::TeleportPhysics);
                    Controller->GetPawn()->SetActorRotation(
                        FRotator(0, FMath::RadiansToDegrees(FMath::Atan2(-Away.Y, -Away.X)), 0));
                    Positioned = true;
                }
                if (!Positioned)
                { Finish(false, TEXT("No camera-clear view of the dropped item was available.")); return; }
            }
        },
        [this, DropId, DropItem]()
        {
            if (!Controller->Landscape) return false;
            const auto* Visual = Controller->Landscape->DropVisuals.Find(*DropId);
            if (!Visual || Visual->Components.Num() < 3) return false;
            for (const TObjectPtr<USceneComponent>& Component : Visual->Components)
            {
                const auto* Primitive = Cast<UPrimitiveComponent>(Component.Get());
                if (!Primitive || Primitive->GetCollisionEnabled() != ECollisionEnabled::NoCollision
                    || Primitive->CanEverAffectNavigation()) return false;
            }
            return Controller->FocusTitle().Contains(UTF8_TO_TCHAR(Homestead::ItemName(*DropItem)))
                && Controller->FocusActions().Contains(TEXT("Pick up"));
        }, 0.8f);
    Add(TEXT("Capture focused nonblocking world drop"),
        [this]() { Screenshot(TEXT("native-world-drop")); },
        [this, DropId]() { return Controller->Landscape
            && Controller->Landscape->DropVisuals.Contains(*DropId)
            && Controller->FocusActions().Contains(TEXT("Pick up")); }, 0.8f);
    Add(TEXT("Drop state survives distant active-window churn and exact schema round trip"),
        [this, DropId, DropWindowLifecycle]()
        {
            const std::string Saved = Controller->Simulation().Serialize();
            Homestead::Simulation Loaded;
            const bool Persisted = Loaded.Deserialize(Saved).ok
                && Loaded.GetState().worldDrops.size() == 1
                && Loaded.GetState().worldDrops.front().id == *DropId;
            Homestead::Simulation Distant = Controller->Simulation();
            const auto& Drop = Distant.GetState().worldDrops.front();
            const Homestead::Point Far{Drop.position.x + Homestead::Generation::ChunkSizeCm * 4,
                Drop.position.y};
            const bool Shifted = Distant.SetActiveWorldRegion(Far).ok
                && Controller->Landscape->Refresh(Distant)
                && Distant.GetState().worldDrops.size() == 1
                && !Controller->Landscape->DropVisuals.Contains(*DropId);
            const bool Returned = Controller->Landscape->Refresh(Controller->Simulation())
                && Controller->Landscape->DropVisuals.Contains(*DropId);
            *DropWindowLifecycle = Persisted && Shifted && Returned;
        },
        [DropWindowLifecycle]() { return *DropWindowLifecycle; }, 0.8f);
    Add(TEXT("Focused pickup restores the exact item once and removes its token"),
        [this, DropPlayerLocation, DropPlayerRotation]()
        {
            Controller->Interact();
            Controller->GetPawn()->SetActorLocation(*DropPlayerLocation,
                false, nullptr, ETeleportType::TeleportPhysics);
            Controller->GetPawn()->SetActorRotation(*DropPlayerRotation);
            Controller->OpenBook(0);
        },
        [this, DropItem, DropCount, DropId]() { return Controller->Simulation().Count(*DropItem) == *DropCount
            && Controller->State().worldDrops.empty() && Controller->Landscape
            && !Controller->Landscape->DropVisuals.Contains(*DropId); }, 0.8f);
    const auto DropWearable = MakeShared<int32>(0);
    const auto DropWearableDye = MakeShared<int32>(0);
    const auto WearablePlayerLocation = MakeShared<FVector>(FVector::ZeroVector);
    Add(TEXT("Equipped garments exclude Drop until unequipped"),
        [this, DropWearable, DropWearableDye]()
        {
            Controller->CloseBook();
            Controller->OpenBook(0);
            Controller->MenuInventoryView(2);
            Controller->NativeMenu->Refresh();
            for (const auto& Item : Controller->State().wearables)
                if (Item.owner == Homestead::WearableOwner::Equipped)
                {
                    *DropWearable = Item.id;
                    *DropWearableDye = Item.dye;
                    break;
                }
            if (!*DropWearable
                || !Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *DropWearable, -1)
                || Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Drop)
                || !Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Unequip))
            { Finish(false, TEXT("Equipped-garment Drop exclusion or Unequip action is unavailable.")); return; }
            Tap(EKeys::Enter);
        },
        [this, DropWearable]() { const auto* Item = Controller->Simulation().GetWearable(*DropWearable);
            return Item && Item->owner == Homestead::WearableOwner::Carried; });
    Add(TEXT("Carried garment Drop uses a one-item controller confirmation"),
        [this, DropWearable]()
        {
            Controller->MenuInventoryView(0);
            Controller->CloseBook();
            Controller->OpenBook(0);
            if (!Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *DropWearable, 0)
                || !Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Drop))
            { Finish(false, TEXT("Carried garment does not expose Drop.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this]() { return Controller->NativeMenu->HasActiveDialog()
            && !Controller->NativeMenu->IsEditingQuantity(); });
    Add(TEXT("Controller cancel preserves the exact carried garment"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this, DropWearable, DropWearableDye]() { const auto* Item = Controller->Simulation().GetWearable(*DropWearable);
            return Item && Item->owner == Homestead::WearableOwner::Carried
                && Item->dye == *DropWearableDye && Controller->State().worldDrops.empty(); });
    Add(TEXT("Controller confirms the same garment ID and dye into the world"),
        [this, DropWearable]()
        {
            Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Drop);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_DPad_Down);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, DropWearable, DropWearableDye]() { const auto* Item = Controller->Simulation().GetWearable(*DropWearable);
            return Item && Item->owner == Homestead::WearableOwner::World && Item->dye == *DropWearableDye
                && Controller->State().worldDrops.size() == 1
                && Controller->State().worldDrops.front().wearableId == *DropWearable; });
    Add(TEXT("Garment pickup preserves identity and dye then restores equip"),
        [this, DropWearable, WearablePlayerLocation]()
        {
            Controller->CloseBook();
            const auto& Drop = Controller->State().worldDrops.front();
            *WearablePlayerLocation = Controller->GetPawn()->GetActorLocation();
            Controller->GetPawn()->SetActorLocation(FVector(Drop.position.x - 70, Drop.position.y,
                Controller->GroundHeight(Drop.position.x - 70, Drop.position.y) + 100),
                false, nullptr, ETeleportType::TeleportPhysics);
            Controller->UpdateFocus();
            Controller->Interact();
            Controller->GetPawn()->SetActorLocation(*WearablePlayerLocation,
                false, nullptr, ETeleportType::TeleportPhysics);
            Controller->OpenBook(0);
            Controller->NativeMenu->FocusSubject(EHomesteadMenuSubject::Wearable, *DropWearable, 0);
            Controller->NativeMenu->FocusItemAction(EHomesteadItemAction::Equip);
            Tap(EKeys::Enter);
        },
        [this, DropWearable, DropWearableDye]() { const auto* Item = Controller->Simulation().GetWearable(*DropWearable);
            return Item && Item->owner == Homestead::WearableOwner::Equipped
                && Item->dye == *DropWearableDye && Controller->State().worldDrops.empty(); }, 0.8f);
}

void AHomesteadSmokeTest::PrepareNativeResumeChecks(const FString& ProducerOutput)
{
    const auto Expected = MakeShared<HomesteadNativeMenuProof::FSaveFixture>();
    const auto ExpectedLoads = MakeShared<uint32>(0);
    Add(TEXT("Admit pinned producer bytes into a fresh test sandbox after Shipping QA admission"),
        [this, ProducerOutput, Expected]()
        {
            TArray<uint8> Bytes;
            FString Error;
            if (!HomesteadNativeMenuProof::ReadFixture(ProducerOutput, *Expected, Bytes, Error))
            { Finish(false, Error); return; }
            const FString Destination = Controller->SavePath(TEXT("Homestead_Manual"));
            if (Controller->SaveRoute.Mode != TEXT("test-sandbox")
                || !FPaths::IsSamePath(Controller->SaveRoute.Directory, FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("SmokeSave")))
                || IFileManager::Get().FileExists(*Destination)
                || !IFileManager::Get().MakeDirectory(*Controller->SaveRoute.Directory, true)
                || !HomesteadNativeMenuProof::OrdinaryLocalPath(Controller->SaveRoute.Directory, true)
                || !FFileHelper::SaveArrayToFile(Bytes, *Destination)
                || !HomesteadNativeMenuProof::OrdinaryLocalPath(Destination, false))
            { Finish(false, TEXT("The owned fresh resume destination could not be safely populated.")); return; }
            TArray<uint8> Copied;
            if (!FFileHelper::LoadFileToArray(Copied, *Destination)
                || FMD5::HashBytes(Copied.GetData(), Copied.Num()) != Expected->Fingerprint)
            { Finish(false, TEXT("The consumer's copied fixture bytes differ from the producer fingerprint.")); return; }
            const auto* Actual = Controller->ReadSave(Destination);
            if (!Actual || !Actual->IsCurrentVersion() || Actual->WorldId != Expected->World
                || Actual->SimulationData != Expected->Simulation)
            { Finish(false, TEXT("Actual save decoding differs from the producer's expected current state.")); }
        },
        [this, Expected]() { return Expected->ProducerProcess != FPlatformProcess::GetCurrentProcessId()
            && Controller->WorldId != Expected->World && !Expected->Fingerprint.IsEmpty(); });
    Add(TEXT("Separate process genuinely resumes producer ownership, quantities, equipment, look and rendered identities via F9"),
        [this, ExpectedLoads]()
        {
            *ExpectedLoads = Controller->TestQuickLoads + 1;
            Tap(EKeys::F9);
            Controller->OpenBook(0);
        },
        [this, Expected, ExpectedLoads]()
        {
            const bool Matched = Controller->TestQuickLoads == *ExpectedLoads && !Controller->ToastIsError()
                && !Controller->MenuNeedsTestReset() && Controller->WorldId == Expected->World
                && FString(UTF8_TO_TCHAR(Controller->Simulation().Serialize().c_str())) == Expected->Simulation
                && HomesteadNativeMenuProof::SameLook(Controller->GetAppearance(), Expected->Look)
                && VerifyNativeMenuPresentation();
            if (Matched) Results.Add(FString::Printf(TEXT("NATIVE_RESUME producer_pid=%u consumer_pid=%u saved_bytes_md5=%s"),
                Expected->ProducerProcess, FPlatformProcess::GetCurrentProcessId(), *Expected->Fingerprint));
            return Matched;
        }, 0.8f);
    Add(TEXT("Separate process holds a screenshot-free cadence window after exact reload"),
        []() {}, []() { return true; }, 5.0f);
    Add(TEXT("Resume leaves the external producer fixture byte-identical"),
        [this]() { Screenshot(TEXT("native-wardrobe-resumed")); },
        [Expected, ProducerOutput]()
        {
            TArray<uint8> Bytes;
            return FFileHelper::LoadFileToArray(Bytes, *FPaths::Combine(ProducerOutput, TEXT("native-wardrobe-fixture.sav")))
                && Expected->Fingerprint == FMD5::HashBytes(Bytes.GetData(), Bytes.Num());
        }, 0.8f);
}
