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
#include "Serialization/JsonSerializer.h"
#include "UI/SHomesteadMenu.h"

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
    // With the field book open, feedback is the book's own notice card (UI/SHomesteadMenu), not the
    // Canvas toast (the HUD draws nothing over the native book): the same checks, measured on it.
    const auto NativeNotice = [this]() -> TSharedPtr<FJsonObject>
    {
        if (!Controller->NativeMenu.IsValid() || !Controller->IsBookOpen()) return nullptr;
        const auto Layout = Controller->NativeMenu->GetNoticeLayout();
        const auto BoxJson = [](const FBox2D& Box)
        {
            auto Result = MakeShared<FJsonObject>();
            Result->SetNumberField(TEXT("left"), Box.Min.X); Result->SetNumberField(TEXT("top"), Box.Min.Y);
            Result->SetNumberField(TEXT("right"), Box.Max.X); Result->SetNumberField(TEXT("bottom"), Box.Max.Y);
            return Result;
        };
        const auto Overlaps = [](const FBox2D& A, const FBox2D& B)
            { return A.Min.X < B.Max.X && A.Max.X > B.Min.X && A.Min.Y < B.Max.Y && A.Max.Y > B.Min.Y; };
        const FString Text = Controller->NativeMenu->GetNoticeText();
        bool bOverlap = false;
        TArray<TSharedPtr<FJsonValue>> Protected;
        for (const auto& Region : Layout.Protected)
        {
            auto Entry = BoxJson(Region.Value);
            Entry->SetStringField(TEXT("role"), Region.Key);
            Entry->SetBoolField(TEXT("overlap"), Layout.bShowing && Overlaps(Layout.Card, Region.Value));
            bOverlap |= Layout.bShowing && Overlaps(Layout.Card, Region.Value);
            Protected.Add(MakeShared<FJsonValueObject>(Entry));
        }
        const FVector2D Drawn = Layout.Card.GetSize();
        auto Object = MakeShared<FJsonObject>();
        Object->SetStringField(TEXT("surface"), TEXT("native-notice-card"));
        Object->SetStringField(TEXT("source"), Text);
        // The whole wrapped text fits: the card got at least the size it asked for.
        Object->SetBoolField(TEXT("fullTextRendered"), Layout.bShowing && Text == Controller->Toast()
            && Drawn.X + 1.0 >= Layout.CardDesired.X && Drawn.Y + 1.0 >= Layout.CardDesired.Y);
        Object->SetBoolField(TEXT("overlap"), bOverlap);
        Object->SetBoolField(TEXT("insideViewport"), Layout.bShowing && Layout.Card.Min.X >= Layout.Book.Min.X - 1
            && Layout.Card.Min.Y >= Layout.Book.Min.Y - 1 && Layout.Card.Max.X <= Layout.Book.Max.X + 1 && Layout.Card.Max.Y <= Layout.Book.Max.Y + 1);
        Object->SetBoolField(TEXT("error"), Controller->NativeMenu->IsNoticeError());
        Object->SetNumberField(TEXT("viewportWidth"), Layout.Book.GetSize().X);
        Object->SetNumberField(TEXT("viewportHeight"), Layout.Book.GetSize().Y);
        Object->SetObjectField(TEXT("toastPanel"), BoxJson(Layout.Card));
        TArray<TSharedPtr<FJsonValue>> Lines;
        auto Line = BoxJson(Layout.Card); Line->SetStringField(TEXT("text"), Text);
        Lines.Add(MakeShared<FJsonValueObject>(Line));
        Object->SetArrayField(TEXT("drawnToastLines"), Lines);
        Object->SetArrayField(TEXT("protected"), Protected);
        return Object;
    };
    const auto Capture = [this, Output, Baseline, NativeNotice](const FString& Name)
    {
        Add(TEXT("Capture while actual feedback is active: ") + Name,
            [this, Output, Name, NativeNotice]()
            {
                FString Measurements;
                if (const auto Native = NativeNotice()) FJsonSerializer::Serialize(Native.ToSharedRef(), TJsonWriterFactory<>::Create(&Measurements));
                else if (const auto* HUD = Controller->GetHUD<AHomesteadHUD>()) Measurements = HUD->FeedbackMeasurements();
                if (!FFileHelper::SaveStringToFile(Measurements, *FPaths::Combine(Output, Name + TEXT(".layout.json"))))
                    UE_LOG(LogTemp, Error, TEXT("Could not write feedback measurements."));
                Screenshot(Name);
            },
            [this, Baseline, NativeNotice]()
            {
                if (const auto Native = NativeNotice())
                {
                    Results.Add(TEXT("NOTICE ") + Native->GetStringField(TEXT("source")));
                    return !Controller->Toast().IsEmpty() && Native->GetStringField(TEXT("source")) == Controller->Toast()
                        && Native->GetBoolField(TEXT("fullTextRendered")) && Native->GetBoolField(TEXT("insideViewport"))
                        && !Native->GetBoolField(TEXT("overlap")) && Native->GetBoolField(TEXT("error")) == Controller->ToastIsError();
                }
                const auto* HUD = Controller->GetHUD<AHomesteadHUD>();
                const bool ExpectedOverlap = Baseline && Controller->IsBookOpen() && Controller->BookPage() != 6;
                const FLinearColor ExpectedColor = Controller->ToastIsError()
                    ? HomesteadNoticeStyle::CardRust() : HomesteadNoticeStyle::CardInk();
                return HUD && !Controller->Toast().IsEmpty() && HUD->FeedbackSource() == Controller->Toast()
                    && HUD->FeedbackFullText() && HUD->FeedbackInsideViewport()
                    && HUD->FeedbackOverlaps() == ExpectedOverlap && HUD->FeedbackColor().Equals(ExpectedColor);
            });
    };
    Add(TEXT("Close initial Notes with existing toggle"), [this]() { Tap(EKeys::I); },
        [this]() { return !Controller->IsBookOpen(); });
    // Synthetic presentation regression: a quiet action must not clear/restart an explicit notice
    // or prevent hint learning just because the retained notice is a refusal.
    for (const bool bError : {false, true})
    {
        const FString Message = bError ? TEXT("Too tired") : TEXT("Fast travel unlocked: the lake");
        const auto NoticeSerial = MakeShared<uint32>(0);
        const auto HintCount = MakeShared<int32>(0);
        const FString Hint = bError ? TEXT("HudQuietErrorFixture") : TEXT("HudQuietSuccessFixture");
        Add(TEXT("Quiet resource success and hotbar selection preserve notice text, priority, clock and hint learning"),
            [this, Message, bError, NoticeSerial, HintCount, Hint]()
            {
                Controller->Notify(Message, bError);
                *NoticeSerial = Controller->NoticeCount();
                *HintCount = Controller->HintUses.FindRef(Hint);
                AHomesteadController::FHintUse Use;
                Use.Id = Hint;
                Use.Serial = *NoticeSerial;
                Use.QuietSerial = Controller->QuietActionSerial;
                Use.bHackPending = Controller->bHackPending;
                Controller->NotifyResourceAction({true, "", Homestead::ResultCode::None, Controller->Sim.GetRevision()}, nullptr);
                Controller->EndHintUse(Use);
                const int32 Cell = Controller->SelectedHotbarIndex();
                Controller->SelectHotbarSlot((Cell + 1) % 10);
                Controller->SelectHotbarSlot(Cell);
            },
            [this, Message, bError, NoticeSerial, HintCount, Hint]()
            {
                const float Life = bError ? 8.0f : 5.0f;
                return Controller->Toast() == Message && Controller->ToastIsError() == bError
                    && Controller->NoticeCount() == *NoticeSerial
                    && Controller->ToastSecondsLeft() <= Life && Controller->ToastSecondsLeft() > Life - 0.5f
                    && Controller->HintUses.FindRef(Hint) == FMath::Min(*HintCount + 1, AHomesteadController::HintRetireUses);
            }, 0.1f);
    }
    // Low Energy warns once per crossing (Jenny 2026-09-30: she starved "with zero warning"): a short
    // notice as it falls past 25 and past 10, never again while it hovers, never for a loaded/slept jump.
    // A marker notice in between proves nothing new was said.
    const auto Marker = TEXT("energy-warning-marker");
    const auto EnergyStep = [this, Marker](const TCHAR* Name, double Energy, bool bMark, const TCHAR* Expected, bool bError)
    {
        Add(Name, [this, Marker, Energy, bMark]()
            {
                if (bMark) Controller->Notify(Marker, false);
                Controller->Sim.SetEnergy(Energy);
            },
            [this, Expected, bError]() { return Controller->Toast() == Expected && Controller->ToastIsError() == bError; }, 0.3f);
    };
    EnergyStep(TEXT("A jump down to 40 (as from a load) says nothing"), 40.0, true, Marker, false);
    EnergyStep(TEXT("Wearing down to 30 says nothing"), 30.0, true, Marker, false);
    EnergyStep(TEXT("Crossing 25 says 'Getting tired' once"), 23.0, true, TEXT("Getting tired"), false);
    EnergyStep(TEXT("Hovering below 25 says nothing more"), 22.0, true, Marker, false);
    EnergyStep(TEXT("Back to 27 (inside the re-arm margin) and down to 24 again says nothing"), 27.0, true, Marker, false);
    EnergyStep(TEXT("Down again to 24 still says nothing"), 24.0, true, Marker, false);
    EnergyStep(TEXT("Crossing 10 says 'Exhausted' as a warning"), 9.0, true, TEXT("Exhausted"), true);
    EnergyStep(TEXT("Hovering below 10 says nothing more"), 8.0, true, Marker, false);
    EnergyStep(TEXT("Restoring her Energy in one jump says nothing"), 100.0, true, Marker, false);
    Add(TEXT("Clear the marker notice before the feedback checks"),
        [this]() { Controller->ToastText.Reset(); Controller->bToastError = false; Controller->ToastRemaining = 0; },
        [this]() { return Controller->Toast().IsEmpty(); });
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
    // The native book: A on a recipe she can't make yet shows the reason in its notice card ("Gather
    // ... first."), keeps the same recipe focused and changes nothing. (The legacy footer's "A: craft"
    // is gone: no recipe row carries a footer action any more.)
    const auto RecipeFocus = MakeShared<int32>(INDEX_NONE);
    Add(TEXT("Real unaffordable craft replaces success with error without simulation changes"),
        [this, Before, RecipeFocus]()
        {
            *Before = Controller->Simulation().Serialize();
            *RecipeFocus = Controller->NativeMenu.IsValid() ? Controller->NativeMenu->GetSelectedContentIndex() : INDEX_NONE;
            Tap(EKeys::Gamepad_FaceButton_Bottom);
        },
        [this, Before, RecipeFocus]()
        {
            const bool bOk = Controller->ToastIsError() && Controller->Toast().StartsWith(TEXT("Gather "))
                && Controller->Simulation().Serialize() == *Before && Controller->NativeMenu.IsValid()
                && Controller->NativeMenu->GetSelectedContentIndex() == *RecipeFocus
                && Controller->NativeMenu->GetNoticeText() == Controller->Toast();
            if (!bOk) Results.Add(FString::Printf(TEXT("CRAFT_REFUSAL toast='%s' error=%d focus=%d/%d notice='%s'"), *Controller->Toast(),
                Controller->ToastIsError(), Controller->NativeMenu.IsValid() ? Controller->NativeMenu->GetSelectedContentIndex() : -2,
                *RecipeFocus, Controller->NativeMenu.IsValid() ? *Controller->NativeMenu->GetNoticeText() : TEXT("")));
            return bOk;
        });
    Capture(TEXT("feedback-craft-error"));
    if (Baseline) return;

    Add(TEXT("Repeated keyboard rejection retains row and produces keyboard action hint"),
        [this]() { Tap(EKeys::Enter); },
        [this, Before, RecipeFocus]() { return Controller->ToastIsError() && Controller->Simulation().Serialize() == *Before
            && !Controller->UsesGamepad() && Controller->NativeMenu.IsValid()
            && Controller->NativeMenu->GetSelectedContentIndex() == *RecipeFocus; });
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
            const bool bToast = Controller->Toast().IsEmpty(), bSource = HUD->FeedbackSource().IsEmpty();
            const bool bGeometry = HUD->FeedbackCriticalGeometry() == *Geometry, bSim = Controller->Simulation().Serialize() == *Before;
            if (StepElapsed > 0.3f && !(bToast && bSource && bGeometry && bSim))
                Results.AddUnique(FString::Printf(TEXT("TOAST_EXPIRY toast_gone=%d source_gone=%d geometry_same=%d sim_same=%d toast='%s' source='%s'"),
                    bToast, bSource, bGeometry, bSim, *Controller->Toast(), *HUD->FeedbackSource()));
            return bToast && bSource && bGeometry && bSim;
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
    // Whatever the build's default vertical sync, the checks below compare against how it started.
    const auto VSyncAtStart = MakeShared<bool>(false);
    Add(TEXT("Make only synthetic graphics file read-only"),
        [Config, VSyncAtStart]() { *VSyncAtStart = GEngine->GetGameUserSettings()->IsVSyncEnabled(); FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Config, true); },
        [Config]() { return IFileManager::Get().IsReadOnly(*Config); });
    Add(TEXT("Real rejected VSync save displays complete recovery instructions"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before, VSyncAtStart]()
        {
            const bool bText = Controller->Toast() == TEXT("Could not save vertical sync. Your previous preference was restored.");
            const bool bVSync = GEngine->GetGameUserSettings()->IsVSyncEnabled() == *VSyncAtStart;
            const bool bSim = Controller->Simulation().Serialize() == *Before;
            const bool bRow = Controller->Rows().IsValidIndex(Controller->SelectedRow()) && Controller->Rows()[Controller->SelectedRow()].Id == 11;
            if (StepElapsed > 0.3f && !(bText && Controller->ToastIsError() && bVSync && bSim && bRow))
                Results.AddUnique(FString::Printf(TEXT("VSYNC_REJECT text=%d error=%d vsync_off=%d sim_same=%d row11=%d selected=%d id=%d"),
                    bText, Controller->ToastIsError(), bVSync, bSim, bRow, Controller->SelectedRow(),
                    Controller->Rows().IsValidIndex(Controller->SelectedRow()) ? Controller->Rows()[Controller->SelectedRow()].Id : -1));
            return bText && Controller->ToastIsError() && bVSync && bSim && bRow;
        });
    Capture(TEXT("feedback-settings-error"));
    Add(TEXT("Restore synthetic graphics file write permission"),
        [Config]() { FPlatformFileManager::Get().GetPlatformFile().SetReadOnly(*Config, false); },
        [Config]() { return !IFileManager::Get().IsReadOnly(*Config); });
    Add(TEXT("Controller success replaces error on same Settings row"),
        [this]() { Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this, Before, VSyncAtStart]() { return !Controller->ToastIsError() && GEngine->GetGameUserSettings()->IsVSyncEnabled() != *VSyncAtStart
            && (Controller->Rows().IsValidIndex(Controller->SelectedRow()) && Controller->Rows()[Controller->SelectedRow()].Id == 11) && Controller->Simulation().Serialize() == *Before; });
    Capture(TEXT("feedback-settings-success"));
    Add(TEXT("Keyboard restores the starting setting in synthetic config with correct input hint"),
        [this]() { Tap(EKeys::Enter); },
        [this, VSyncAtStart]() { return !Controller->ToastIsError() && GEngine->GetGameUserSettings()->IsVSyncEnabled() == *VSyncAtStart
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
        [this, Before]()
        {
            // Recovery now takes the newest valid save of any slot (Manual, Auto, Recovery), which may be
            // a moment older than the corrupted manual one: the book stays open and the notice says so.
            const bool bOk = Controller->IsBookOpen()
                && Controller->Toast() == TEXT("Recovered your latest valid save. An unreadable save was skipped; backups are retained.");
            if (!bOk && StepElapsed > 0.6f)
                Results.AddUnique(FString::Printf(TEXT("RECOVERY book=%d page=%d sim_same=%d"), Controller->IsBookOpen(),
                    Controller->BookPage(), Controller->Simulation().Serialize() == *Before));
            return bOk;
        }, 0.8f);
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
    Add(TEXT("Select actual plan with controller"), [this]() { AffordPlan(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
        [this]() { return Controller->IsPlanning() && !Controller->IsBookOpen(); });
    Add(TEXT("Real rejected placement leaves simulation and planning controls intact"),
        [this, Before]() { UndoAffordPlan(); *Before = Controller->Simulation().Serialize(); Tap(EKeys::Gamepad_FaceButton_Bottom); },
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
        [this, Before]()
        {
            const bool bToast = Controller->Toast().IsEmpty();
            const bool bSource = Controller->GetHUD<AHomesteadHUD>()->FeedbackSource().IsEmpty();
            const bool bSim = Controller->Simulation().Serialize() == *Before;
            if (StepElapsed > 1.0f && !(bToast && bSource && bSim))
                Results.AddUnique(FString::Printf(TEXT("SUCCESS_EXPIRY toast_gone=%d source_gone=%d sim_same=%d source='%s'"),
                    bToast, bSource, bSim, *Controller->GetHUD<AHomesteadHUD>()->FeedbackSource()));
            return bToast && bSource && bSim;
        }, 1.2f);
}
