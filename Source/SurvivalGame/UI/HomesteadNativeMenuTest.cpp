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

bool AHomesteadSmokeTest::VerifyNativeMenuPresentation() const
{
    const auto* Avatar = Cast<AHomesteadCharacter>(Controller->GetPawn());
    if (!Avatar) return false;
    const auto* Presentation = Avatar->GetEquipmentPresentation();
    if (!Presentation)
    {
        const auto* Prototype = Avatar->GetMesh();
        if (!Avatar->HasHeroine() || !Prototype || !Prototype->GetSkeletalMeshAsset()) return false;
        for (int32 Index = 0; Index < Prototype->GetNumMaterials(); ++Index)
        {
            const auto* Surface = Prototype->GetMaterial(Index);
            if (!Surface || !Surface->GetShadingModels().HasShadingModel(MSM_DefaultLit)) return false;
        }
        return true;
    }
    const auto* Body = Avatar->GetMesh();
    if (!Body || !Body->IsVisible() || Body->GetSkeletalMeshAsset() != Presentation->Base.Mesh) return false;
    const auto Matches = [](const USkeletalMeshComponent& Component, const FHomesteadEquipmentSurface& Surface)
    {
        if (Component.GetNumMaterials() != Surface.Materials.Num()) return false;
        for (int32 Index = 0; Index < Surface.Materials.Num(); ++Index)
            if (Component.GetMaterial(Index) != Surface.Materials[Index]) return false;
        return true;
    };
    if (!Matches(*Body, Presentation->Base)) return false;
    TArray<USkeletalMeshComponent*> Components;
    Avatar->GetComponents(Components);
    for (const auto& Surface : Presentation->Garments)
    {
        bool Found = false;
        for (const auto* Component : Components)
            if (Component != Body && Component->IsVisible() && Component->GetSkeletalMeshAsset() == Surface.Mesh
                && Matches(*Component, Surface)) Found = true;
        if (!Found) return false;
    }
    return true;
}

void AHomesteadSmokeTest::PrepareNativeMenuChecks()
{
    const auto Before = MakeShared<std::string>();
    const auto OriginalRoute = MakeShared<FString>();
    const auto Blocker = MakeShared<FString>(FPaths::Combine(HomesteadTestOutputDirectory(), TEXT("native-menu-write-blocker")));
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
