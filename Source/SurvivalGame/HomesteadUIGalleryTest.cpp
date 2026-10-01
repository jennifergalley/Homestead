// UI gallery capture run (Development builds): -HomesteadSmokeTest -HomesteadUIGallery[=<ids>|all]
// [-HomesteadUIGalleryInput=Pad] on the Estate map. Checks the gallery covers every book tab,
// settings tab and notice style, then puts each entry on screen (HomesteadUIGallery.h) and captures
// the window with Slate included as <id>.png, listing it in gallery-index.tsv (id, description).
// Scripts\Capture-UiGallery.ps1 runs this at each resolution and builds the contact sheet.
#include "HomesteadSmokeTest.h"
#include "HomesteadController.h"
#include "HomesteadTestPaths.h"
#include "HomesteadUIGallery.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace UIGalleryRun
{
// Seconds for the estate to stream in around her before the first entry.
constexpr float StreamInSeconds = 20.0f;
// Longest one entry may take to settle (a teleport across the estate plus its settle time).
constexpr float EntrySeconds = 75.0f;
// Time for the requested screenshot to be written before the next entry changes the screen.
constexpr float CaptureSeconds = 0.8f;
}

void AHomesteadSmokeTest::PrepareUIGalleryChecks()
{
#if UE_BUILD_SHIPPING
    Finish(false, TEXT("The UI gallery is a Development-build tool."));
#else
    FString Spec, Input;
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadUIGallery="), Spec, false);
    FParse::Value(FCommandLine::Get(), TEXT("HomesteadUIGalleryInput="), Input);
    const bool bPad = Input.Equals(TEXT("Pad"), ESearchCase::IgnoreCase);
    FString Error;
    const TArray<FString> Ids = FHomesteadUIGallery::Resolve(Spec, Error);
    if (!Error.IsEmpty() || Ids.IsEmpty())
    {
        Finish(false, Error.IsEmpty() ? FString(TEXT("No UI gallery ids to capture.")) : Error);
        return;
    }
    const FString Output = HomesteadTestOutputDirectory();
    const FString Index = FPaths::Combine(Output, TEXT("gallery-index.tsv"));
    IFileManager::Get().Delete(*Index, false, true, true);
    FHomesteadUIGallery::ResetFixture();
    Results.Add(FString::Printf(TEXT("UI_GALLERY ids=%d input=%s"), Ids.Num(), bPad ? TEXT("Pad") : TEXT("KBM")));

    Add(TEXT("Every book tab, settings tab and notice style has a gallery entry"), []() {},
        [this]()
        {
            const TArray<FString> Missing = FHomesteadUIGallery::MissingCoverage(*Controller);
            if (!Missing.IsEmpty()) Results.Add(TEXT("UI_GALLERY_MISSING ") + FString::Join(Missing, TEXT(" ")));
            return Missing.IsEmpty();
        });
    Add(TEXT("Let the estate stream in around her"), []() {},
        [this]()
        {
            const auto* Avatar = Cast<ACharacter>(Controller->GetPawn());
            return Avatar && Avatar->GetCharacterMovement() && Avatar->GetCharacterMovement()->IsMovingOnGround()
                && !Controller->bPendingGroundSnap && !Controller->bPendingSpawn;
        }, UIGalleryRun::StreamInSeconds);
    for (const FString& Id : Ids)
    {
        const auto Status = MakeShared<int32>(0);
        const auto Reason = MakeShared<FString>();
        FStep& Show = Steps.AddDefaulted_GetRef();
        Show.Name = TEXT("Show ") + Id;
        Show.Action = [this, Id, bPad, Status, Reason]()
        {
            *Status = 0;
            FHomesteadUIGallery::Show(*Controller, Id, bPad, [Status, Reason](bool bOk, const FString& Why)
            {
                *Status = bOk ? 1 : -1;
                *Reason = Why;
            });
        };
        Show.Check = [this, Id, Status, Reason]()
        {
            if (*Status < 0 && !Reason->StartsWith(TEXT("REPORTED ")))
            {
                Results.Add(FString::Printf(TEXT("UI_GALLERY_SKIPPED %s: %s"), *Id, **Reason));
                *Reason = TEXT("REPORTED ") + *Reason;
            }
            return *Status != 0;
        };
        Show.Wait = UIGalleryRun::EntrySeconds;
        Show.bCompleteWhenReady = true;
        FStep& Capture = Steps.AddDefaulted_GetRef();
        Capture.Name = TEXT("Capture ") + Id;
        Capture.Skip = [Status]() { return *Status <= 0; };
        Capture.Action = [this, Id, Output, Index]()
        {
            const FHomesteadUIGallery::FEntry* Entry = FHomesteadUIGallery::Find(Id);
            // The window as she sees it: the 3D view, the Canvas HUD and every Slate widget.
            FScreenshotRequest::RequestScreenshot(FPaths::Combine(Output, Id + TEXT(".png")), true, false);
            const FString Line = Id + TEXT("\t") + (Entry ? Entry->Description.Replace(TEXT("\t"), TEXT(" ")) : FString()) + TEXT("\n");
            FFileHelper::SaveStringToFile(Line, *Index, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
                &IFileManager::Get(), FILEWRITE_Append);
            Results.Add(TEXT("UI_GALLERY_CAPTURED ") + Id);
        };
        Capture.Check = []() { return true; };
        Capture.Wait = UIGalleryRun::CaptureSeconds;
    }
#endif
}
