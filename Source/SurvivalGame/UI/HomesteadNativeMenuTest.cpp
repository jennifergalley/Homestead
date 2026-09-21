#include "../HomesteadSmokeTest.h"
#include "../HomesteadController.h"
#include "../HomesteadCharacter.h"
#include "../HomesteadSave.h"
#include "../HomesteadTestPaths.h"
#include "SHomesteadMenu.h"
#include "Components/SkeletalMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "Materials/MaterialInterface.h"
#include "CoreGlobals.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/SecureHash.h"
#include "HAL/PlatformProcess.h"
#include "Engine/SkeletalMesh.h"
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
    }
    if (ResumeRequested)
    {
        if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadNativeQuitTest")))
        { Finish(false, TEXT("Resume and quit verification require separate fresh processes.")); return; }
        PrepareNativeResumeChecks(ProducerOutput);
        return;
    }
    if (FParse::Param(FCommandLine::Get(), TEXT("HomesteadNativeQuitTest")))
    {
        Add(TEXT("Reach the real save-and-quit confirmation"),
            [this]()
            {
                Tap(EKeys::Escape);
                Tap(EKeys::Escape);
                Tap(EKeys::Right);
                Tap(EKeys::Enter);
            },
            [this]() { return Controller->NativeMenu && Controller->NativeMenu->IsExitPrompt(); });
        Add(TEXT("Successful save precedes actual engine exit request"),
            [this, Before]()
            {
                *Before = Controller->Simulation().Serialize();
                Tap(EKeys::Down);
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
    Add(TEXT("Close initial guide through mapped Back"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("First exit-path press opens Settings"),
        [this]() { Tap(EKeys::Escape); PausedHour = Controller->State().hour; },
        [this]() { return Controller->HasNativeMenu() && Controller->BookPage() == 4; });
    Capture(TEXT("native-settings"));
    Add(TEXT("Second and third exit-path presses reach confirmation"),
        [this]() { Tap(EKeys::Right); Tap(EKeys::Enter); },
        [this]() { return Controller->NativeMenu && Controller->NativeMenu->IsExitPrompt(); });
    Capture(TEXT("native-exit-confirm"));
    Add(TEXT("Exit confirmation keeps simulation paused"),
        []() {},
        [this]() { return FMath::IsNearlyEqual(Controller->State().hour, PausedHour, 1e-8); }, 1.0f);
    Add(TEXT("Exit confirmation defaults to staying"),
        [this]() { Tap(EKeys::Enter); },
        [this]() { return Controller->IsBookOpen() && !Controller->NativeMenu->HasActiveDialog(); });
    Add(TEXT("Current-schema F5 writes a readable sandbox save"),
        [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError() && Controller->ReadSave(Controller->SavePath(TEXT("Homestead_Manual"))) != nullptr; });
    Add(TEXT("A real IO error does not quit or change inventory"),
        [this, Before, OriginalRoute, Blocker]()
        {
            *Before = Controller->Simulation().Serialize();
            *OriginalRoute = Controller->SaveRoute.Directory;
            if (!FFileHelper::SaveStringToFile(TEXT("owned synthetic file prevents directory creation"), **Blocker))
            { Finish(false, TEXT("Could not prepare the owned save-failure fixture.")); return; }
            Controller->SaveRoute.Directory = *Blocker;
            Tap(EKeys::Enter);
            Tap(EKeys::Down);
            Tap(EKeys::Enter);
        },
        [this, Before]() { return Controller->NativeMenu->IsSaveError()
            && Controller->Simulation().Serialize() == *Before; });
    Capture(TEXT("native-save-error"));
    Add(TEXT("Save failure stays actionable beyond toast expiry"),
        []() {},
        [this]() { return Controller->NativeMenu->IsSaveError() && Controller->Toast().IsEmpty(); }, 8.3f);
    Add(TEXT("Return from failure restores Settings without discarding progress"),
        [this, OriginalRoute, Blocker]()
        {
            Controller->SaveRoute.Directory = *OriginalRoute;
            if (!IFileManager::Get().Delete(**Blocker, false, true))
            { Finish(false, TEXT("Could not remove the owned save-failure fixture.")); return; }
            Tap(EKeys::Enter);
        },
        [this, Before]() { return !Controller->NativeMenu->HasActiveDialog()
            && Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("Controller tabs reach real carried inventory"),
        [this]()
        {
            for (int Index = 0; Index < 4; ++Index) Tap(EKeys::Gamepad_LeftShoulder);
        },
        [this]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Controller->BookPage() == 0 && Controller->UsesGamepad() && Subject
                && Subject->Subject == EHomesteadMenuSubject::ItemGroup
                && Subject->Id == static_cast<int>(Homestead::Item::Knife) && Subject->Quantity == 1
                && Controller->NativeMenu->GetDisplayedDetails().Contains(TEXT("Carried: 1"));
        });
    Capture(TEXT("native-inventory"));
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
    Add(TEXT("Rejected UI crafting is atomic and describes real requirements"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Enter); Tap(EKeys::Enter); },
        [this, Before]() { return Controller->ToastIsError() && Controller->Simulation().Serialize() == *Before
            && Controller->NativeMenu->GetDisplayedDetails().Contains(TEXT("4 Branch + 3 Stone + 2 Fiber")); });
    Capture(TEXT("native-crafting"));
    Add(TEXT("Real building plan enters placement without charging"),
        [this, Before]()
        {
            Tap(EKeys::Gamepad_RightShoulder);
            *Before = Controller->Simulation().Serialize();
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Before]() { return Controller->IsPlanning() && !Controller->IsBookOpen()
            && Controller->Simulation().Serialize() == *Before; });
    Add(TEXT("Cancel placement and return to Settings"),
        [this]() { Tap(EKeys::Escape); Tap(EKeys::Escape); },
        [this]() { return !Controller->IsPlanning() && Controller->IsBookOpen() && Controller->BookPage() == 4; });
    PrepareNativeWardrobeChecks();
    Add(TEXT("Prepare disclosed survival-failure fixture"),
        [this]() { Controller->Sim.AdvanceGameHours(120, Controller->PlayerPoint()); },
        [this]() { return Controller->IsFailed() && Controller->HasNativeMenu() && !Controller->IsBookOpen(); }, 0.8f);
    Add(TEXT("Recovery has independent controller Settings access"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Top); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 4; });
    Add(TEXT("Recovery exit warns about unsaved progress instead of overwriting checkpoint"),
        [this]() { Tap(EKeys::Gamepad_DPad_Right); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->NativeMenu->IsUnsavedPrompt(); });
    Capture(TEXT("native-recovery-exit"));
    Add(TEXT("Cancel recovery exit returns to recovery without forcing retry"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Right); Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return Controller->IsFailed() && !Controller->IsBookOpen() && Controller->HasNativeMenu(); });
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
            Tap(EKeys::Gamepad_FaceButton_Bottom);
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
        [this, OpenInventory]() { OpenInventory(0); Tap(EKeys::Gamepad_DPad_Right); },
        [this, Tunic]()
        {
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            return Subject && Subject->Subject == EHomesteadMenuSubject::Wearable && Subject->SubjectId == *Tunic
                && Subject->ContainerId == 0;
        });
    Add(TEXT("Mapped UI equip restores the same ID and committed garment mesh"),
        [this, Tunic, Expected]()
        {
            *Expected = Controller->Simulation();
            if (!Expected->EquipWearable(*Tunic, Expected->GetRevision()))
            { Finish(false, TEXT("The independent equip expectation was invalid.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Expected, Tunic]()
        {
            return !Controller->ToastIsError()
                && Controller->State().equipment[static_cast<int32>(Homestead::EquipmentSlot::Torso)] == *Tunic
                && Controller->State().equipment[static_cast<int32>(Homestead::EquipmentSlot::Legs)] == *Tunic
                && Controller->Simulation().Serialize() == Expected->Serialize() && VerifyNativeMenuPresentation();
        });
    Add(TEXT("Mapped UI dye changes only the owned item and its actual material tint"),
        [this, Tunic, Expected, OpenInventory]()
        {
            OpenInventory(2);
            const auto* Subject = Controller->NativeMenu->GetSelectedSubject();
            const auto* Owned = Controller->Simulation().GetWearable(*Tunic);
            if (!Subject || Subject->SubjectId != *Tunic || !Owned)
            { Finish(false, TEXT("The intended owned tunic is not selected for dye.")); return; }
            *Expected = Controller->Simulation();
            if (!Expected->RecolorWearable(*Tunic, (Owned->dye + 1) % 4, Controller->PlayerPoint(), Expected->GetRevision()))
            { Finish(false, TEXT("The independent dye expectation was invalid.")); return; }
            Tap(EKeys::Gamepad_FaceButton_Bottom);
            Tap(EKeys::Gamepad_DPad_Right);
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Expected]() { return !Controller->ToastIsError()
            && Controller->Simulation().Serialize() == Expected->Serialize() && VerifyNativeMenuPresentation(); });
    Add(TEXT("Capture admitted dyed wardrobe"),
        [this]() { Screenshot(TEXT("native-wardrobe-dyed")); },
        [this]() { return VerifyNativeMenuPresentation(); }, 0.8f);

    for (int32 Id : {0, 1, 2, 3, 6})
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
                case 0: ExpectedLook->HairStyle = (ExpectedLook->HairStyle + 1) % 3; break;
                case 1: ExpectedLook->HairColor = (ExpectedLook->HairColor + 1) % HomesteadLook::HairColorCount; break;
                case 2: ExpectedLook->SkinTone = (ExpectedLook->SkinTone + 1) % 4; break;
                case 3: ExpectedLook->EyeColor = (ExpectedLook->EyeColor + 1) % 4; break;
                case 6: ExpectedLook->BodyPreset = (ExpectedLook->BodyPreset + 1) % 3; break;
                }
                Tap(EKeys::Enter);
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
        [this, OpenInventory]() { OpenInventory(2); Tap(EKeys::Enter); Tap(EKeys::Enter); },
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
    Add(TEXT("Resume leaves the external producer fixture byte-identical"),
        [this]() { Screenshot(TEXT("native-wardrobe-resumed")); },
        [Expected, ProducerOutput]()
        {
            TArray<uint8> Bytes;
            return FFileHelper::LoadFileToArray(Bytes, *FPaths::Combine(ProducerOutput, TEXT("native-wardrobe-fixture.sav")))
                && Expected->Fingerprint == FMD5::HashBytes(Bytes.GetData(), Bytes.Num());
        }, 0.8f);
}
