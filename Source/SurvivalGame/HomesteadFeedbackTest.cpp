#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadHUD.h"
#include "HomesteadTestPaths.h"
#include "UI/HomesteadNoticeStyle.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void AHomesteadSmokeTest::PrepareFeedbackChecks()
{
    const FString Output = HomesteadTestOutputDirectory();
    const FString Config = FPaths::ConvertRelativePathToFull(FPaths::Combine(Output, TEXT("Graphics/GameUserSettings.ini")));
    const auto* Branch = GConfig->FindBranch(TEXT("GameUserSettings"), {});
    if (!Branch || !FPaths::IsSamePath(FPaths::ConvertRelativePathToFull(Branch->IniPath), Config))
    {
        Finish(false, TEXT("Feedback fixture requires its explicit synthetic graphics destination."));
        return;
    }
    for (const TCHAR* Other : {TEXT("HomesteadVideoSyncTest"), TEXT("HomesteadBookClarityTest"),
        TEXT("HomesteadPromptTest"), TEXT("HomesteadFullLoop"), TEXT("HomesteadGatheringTest"),
        TEXT("HomesteadWateringTest"), TEXT("HomesteadWeedingTest"), TEXT("HomesteadClearingTest"),
        TEXT("HomesteadPresentationTest"), TEXT("HomesteadAudioProof")})
        if (FParse::Param(FCommandLine::Get(), Other))
        { Finish(false, TEXT("Feedback fixture cannot be combined with another smoke selector.")); return; }

    const bool Baseline = FParse::Param(FCommandLine::Get(), TEXT("HomesteadFeedbackBaseline"));
    const auto Before = MakeShared<std::string>();
    const auto Geometry = MakeShared<FString>();
    Results.Add(TEXT("GRAPHICS_CONFIG=") + FPaths::ConvertRelativePathToFull(Branch->IniPath));
    Results.Add(TEXT("DISCLOSURE synthetic fresh world; mapped UI actions. Final coverage deliberately corrupts only its sandbox manual save to exercise real backup-recovery feedback."));
    const auto Capture = [this, Output, Baseline](const FString& Name)
    {
        Add(TEXT("Capture while actual feedback is active: ") + Name,
            [this, Output, Name]()
            {
                if (const auto* HUD = Controller->GetHUD<AHomesteadHUD>())
                    if (!FFileHelper::SaveStringToFile(HUD->FeedbackMeasurements(), *FPaths::Combine(Output, Name + TEXT(".layout.json"))))
                        UE_LOG(LogTemp, Error, TEXT("Could not write feedback Canvas measurements."));
                Screenshot(Name);
            },
            [this, Baseline]()
            {
                const auto* HUD = Controller->GetHUD<AHomesteadHUD>();
                const bool ExpectedOverlap = Baseline && Controller->IsBookOpen() && Controller->BookPage() != 6;
                const FLinearColor ExpectedColor = Controller->ToastIsError()
                    ? HomesteadNoticeStyle::RustInk : HomesteadNoticeStyle::InkBrown;
                return HUD && !Controller->Toast().IsEmpty() && HUD->FeedbackSource() == Controller->Toast()
                    && HUD->FeedbackFullText() && HUD->FeedbackInsideViewport()
                    && HUD->FeedbackOverlaps() == ExpectedOverlap && HUD->FeedbackColor().Equals(ExpectedColor);
            });
    };
    Add(TEXT("Close initial Notes with existing toggle"), [this]() { Tap(EKeys::I); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Open pack with mapped keyboard I"), [this]() { Tap(EKeys::I); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 0; });
    Add(TEXT("Save success comes from actual sandbox F5"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::F5); },
        [this, Before, Output]() { return Controller->Toast() == TEXT("Your homestead is saved.") && !Controller->ToastIsError()
            && Controller->Simulation().Serialize() == *Before
            && IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("SmokeSave/Homestead_Manual.sav"))); });
    Capture(TEXT("feedback-pack-success"));
    Add(TEXT("Controller opens crafting recipes"), [this]() { Tap(EKeys::Gamepad_RightShoulder); },
        [this]() { return Controller->BookPage() == 1 && Controller->UsesGamepad(); });
    Add(TEXT("Real unaffordable craft replaces success with error without simulation changes"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->ToastIsError() && !Controller->Toast().IsEmpty()
            && Controller->Toast() != TEXT("Your homestead is saved.") && Controller->Simulation().Serialize() == *Before
            && Controller->SelectedRow() == 0 && Controller->BookFooter().Contains(TEXT("A: craft")); });
    Capture(TEXT("feedback-craft-error"));
    if (Baseline) return;

    Add(TEXT("Repeated keyboard rejection retains row and produces keyboard action hint"),
        [this]() { Tap(EKeys::Enter); },
        [this, Before]() { return Controller->ToastIsError() && Controller->Simulation().Serialize() == *Before
            && Controller->SelectedRow() == 0 && Controller->BookFooter().Contains(TEXT("Enter: craft")); });
    Capture(TEXT("feedback-craft-keyboard"));
    Add(TEXT("Record protected geometry while toast is present"),
        [this, Geometry]() { *Geometry = Controller->GetHUD<AHomesteadHUD>()->FeedbackCriticalGeometry(); },
        []() { return true; });
    Add(TEXT("Error remains visible near the end of its eight-second lifetime"),
        []() {}, [this]() { return !Controller->Toast().IsEmpty() && Controller->ToastIsError(); }, 6.1f);
    Add(TEXT("Error expires on its existing eight-second clock without moving book contents or simulation"),
        []() {},
        [this, Before, Geometry]()
        {
            const auto* HUD = Controller->GetHUD<AHomesteadHUD>();
            return Controller->Toast().IsEmpty() && HUD->FeedbackSource().IsEmpty()
                && HUD->FeedbackCriticalGeometry() == *Geometry && Controller->Simulation().Serialize() == *Before;
        }, 1.2f);
    // The five field-book tabs in order (Pack 0, Craft 1, Build 2, Map 7, Look 6; the Guidebook, 3, is
    // retired). Settings (4) isn't a tab: Start opens it from the world.
    for (const int32 Page : {2, 7, 6})
        Add(TEXT("Preserve controller page navigation"), [this]() { Tap(EKeys::Gamepad_RightShoulder); },
            [this, Page]() { return Controller->BookPage() == Page; });
    Add(TEXT("Controller B closes the book"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsBookOpen(); });
    Add(TEXT("Controller Start opens Settings"), [this]() { Tap(EKeys::Gamepad_Special_Right); },
        [this]() { return Controller->IsBookOpen() && Controller->BookPage() == 4; });
    // The world ran for a moment between closing the book and opening Settings.
    Add(TEXT("Record the paused simulation in Settings"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); }, []() { return true; });
    QueueSelectRow(11);
    Add(TEXT("Make only synthetic graphics file read-only"),
        [Config]() { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Config, true); },
        [Config]() { return IFileManager::Get().IsReadOnly(*Config); });
    Add(TEXT("Real rejected VSync save displays complete recovery instructions"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->Toast() == TEXT("Could not save vertical sync. Your previous preference was restored.")
            && Controller->ToastIsError() && !GEngine->GetGameUserSettings()->IsVSyncEnabled()
            && Controller->Simulation().Serialize() == *Before && Controller->SelectedRow() == 11; });
    Capture(TEXT("feedback-settings-error"));
    Add(TEXT("Restore synthetic graphics file write permission"),
        [Config]() { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Config, false); },
        [Config]() { return !IFileManager::Get().IsReadOnly(*Config); });
    Add(TEXT("Controller success replaces error on same Settings row"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return !Controller->ToastIsError() && GEngine->GetGameUserSettings()->IsVSyncEnabled()
            && Controller->SelectedRow() == 11 && Controller->Simulation().Serialize() == *Before; });
    Capture(TEXT("feedback-settings-success"));
    Add(TEXT("Keyboard restores Off in synthetic config with correct input hint"),
        [this]() { Tap(EKeys::Enter); },
        [this]() { return !Controller->ToastIsError() && !GEngine->GetGameUserSettings()->IsVSyncEnabled()
            && Controller->BookFooter().Contains(TEXT("Enter: toggle")); });
    Capture(TEXT("feedback-settings-keyboard"));
    // Page-left from Settings enters the tab cycle at its end (Look) and walks back to the pack.
    for (const int32 Page : {6, 7, 2, 1, 0})
        Add(TEXT("Return to pack without unpausing via controller page-left"),
            [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
            [this, Page]() { return Controller->IsBookOpen() && Controller->BookPage() == Page; });
    Add(TEXT("Create real backup through second mapped manual save"), [this]() { Tap(EKeys::F5); },
        [this, Output]() { return Controller->Toast() == TEXT("Your homestead is saved.")
            && IFileManager::Get().FileExists(*FPaths::Combine(Output, TEXT("SmokeSave/Homestead_Manual.sav.bak"))); });
    const FString Manual = FPaths::Combine(Output, TEXT("SmokeSave/Homestead_Manual.sav"));
    Add(TEXT("Deliberately corrupt only sandbox primary save; keep real backup"),
        [Manual]() { FFileHelper::SaveStringToFile(TEXT("feedback fixture: invalid primary envelope"), *Manual); },
        [Manual]() { return IFileManager::Get().FileSize(*Manual) < 100; });
    Add(TEXT("Mapped load recovers actual backup and reopens paused pack"),
        [this]() { Tap(EKeys::F9); Tap(EKeys::Gamepad_Special_Right); },
        [this, Before]() { return Controller->IsBookOpen() && Controller->ToastIsError()
            && Controller->Toast() == TEXT("Recovered a valid save. An unreadable save was skipped; backups are retained.")
            && Controller->Simulation().Serialize() == *Before; }, 0.8f);
    Capture(TEXT("feedback-pack-recovery"));
    Add(TEXT("Same recovery feedback remains readable in Look without changing appearance"),
        [this]() { Tap(EKeys::Gamepad_LeftShoulder); },
        [this]() { return Controller->BookPage() == 6 && Controller->UsesGamepad(); });
    Capture(TEXT("feedback-look-recovery"));
    Add(TEXT("Save from Look creates normal success feedback"), [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError() && Controller->BookPage() == 6; });
    Capture(TEXT("feedback-look-success"));
    Add(TEXT("Open building plans using existing keyboard binding"), [this]() { Tap(EKeys::B); },
        [this]() { return Controller->BookPage() == 2 && Controller->IsBookOpen(); });
    Add(TEXT("Real save feedback on building-plan page"), [this]() { Tap(EKeys::F5); },
        [this]() { return Controller->Toast() == TEXT("Your homestead is saved."); });
    Capture(TEXT("feedback-plans-success"));
    Add(TEXT("Select actual plan with controller"), [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsPlanning() && !Controller->IsBookOpen(); });
    Add(TEXT("Real rejected placement leaves simulation and planning controls intact"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before]() { return Controller->IsPlanning() && Controller->ToastIsError()
            && Controller->Simulation().Serialize() == *Before && !Controller->Toast().IsEmpty(); });
    Capture(TEXT("feedback-planning-error"));
    Add(TEXT("Cancel planning with existing B button"), [this]() { Tap(EKeys::Gamepad_FaceButton_Right); },
        [this]() { return !Controller->IsPlanning() && !Controller->IsBookOpen(); });
    Capture(TEXT("feedback-world-error"));
    Add(TEXT("Mapped world save replaces error without opening menu"), [this]() { Tap(EKeys::F5); },
        [this]() { return !Controller->ToastIsError() && !Controller->IsBookOpen(); });
    Capture(TEXT("feedback-world-success"));
    Add(TEXT("Pause pack for success lifetime check"), [this]() { Tap(EKeys::I); },
        [this]() { return Controller->IsBookOpen(); });
    Add(TEXT("Fresh success uses unchanged five-second lifetime"),
        [this, Before]() { *Before = Controller->Simulation().Serialize(); Tap(EKeys::F5); },
        [this]() { return !Controller->Toast().IsEmpty() && !Controller->ToastIsError(); }, 4.2f);
    Add(TEXT("Success expires without hiding errors early or unpausing world"), []() {},
        [this, Before]() { return Controller->Toast().IsEmpty()
            && Controller->GetHUD<AHomesteadHUD>()->FeedbackSource().IsEmpty()
            && Controller->Simulation().Serialize() == *Before; }, 1.2f);
}
