#pragma once

// UI Gallery (Development builds only; compiled out of Shipping): named, deterministic states that
// each put one UI surface on screen exactly as Jenny sees it, so agents can capture and look at every
// menu, dialog, notice, hint and HUD element they design.
//
// Each entry starts from one isolated fixture (10:00 on the day the gallery began, a known pack,
// purse and Energy, no toasts or pickups, nothing open) and then opens a single surface. Routes:
//   - Capture run: -HomesteadSmokeTest -HomesteadUIGallery[=<id>,<id>|all] [-HomesteadUIGalleryInput=Pad]
//     on the Estate map, through Scripts\Capture-UiGallery.ps1 / Test-Game.ps1 -UIGallery. The
//     smoke route keeps saves in its sandbox; a gallery run never touches a real save.
//   - Live PIE: homestead.UIGallery list | <id> | next | prev | all [Pad|KBM] (editor_mcp.py gallery).
// Add an entry for every new surface; the run fails when a book tab, settings tab or notice style
// has none (MissingCoverage).
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include "Simulation/HomesteadSimulation.h"

class AHomesteadController;

struct FHomesteadUIGallery
{
    // What an entry stands for in the coverage check.
    enum class ECover : uint8 { BookPage, SettingsTab, Notice, Focus, Hud, Dialog, Shop, Setup };
    // Every notice style on screen; each needs an entry.
    enum class ENotice : uint8
    {
        WorldToast, WorldToastError, WorldToastLong, BookNotice, BookNoticeError, FocusCard, Pickup,
        ControlsStrip, ShopStatus, Count
    };
    // Where she stands for an entry (world hints need something in front of her).
    struct FStage
    {
        bool bMove = false;
        Homestead::Point Stand;
        Homestead::Point Face;
    };
    struct FEntry
    {
        FString Id;
        FString Description; // What she should see.
        ECover Cover = ECover::Hud;
        int32 Key = 0;
        TFunction<bool(AHomesteadController&, FStage&, FString&)> Stage;
        TFunction<void(AHomesteadController&)> Apply;
        float Settle = 0.9f; // Seconds on screen before it counts as shown.
        // Set while the entry shows another lane's work not yet on this line (what it needs); the run
        // still captures it and reports it as pending.
        FString Pending;
    };

    static const TArray<FEntry>& Entries();
    static const FEntry* Find(const FString& Id);
    // "all", or ids separated by commas; unknown ids are an error.
    static TArray<FString> Resolve(const FString& Spec, FString& Error);
    // Book tabs, settings tabs and notice styles with no entry ("book-page-3", "notice-5").
    static TArray<FString> MissingCoverage(const AHomesteadController& Controller);
    // Puts `Id` on screen in this game; OnReady(true) once it has settled, or (false, why).
    static void Show(AHomesteadController& Controller, const FString& Id, bool bPad,
        TFunction<void(bool, const FString&)> OnReady);
    // Forgets the fixture taken from the current game (the next Show takes a fresh one).
    static void ResetFixture();

private:
    static void Prepare(AHomesteadController& Controller, bool bPad);
    static void Face(AHomesteadController& Controller, Homestead::Point At);
};
#endif
