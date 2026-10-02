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
//   - Live PIE: homestead.UIGallery list | <id> | next | prev | all [Pad|KBM], backdrop plain|world (editor_mcp.py gallery),
//     in an editor started with -HomesteadPreviewProfile=<id> (Start-EditorMcp.ps1 -PreviewProfile gallery).
// It runs only on an isolated save route (RefusalFor): never on a real save. Add an entry for every
// new surface; the run fails when a book tab (SHomesteadMenu::TabPages), settings tab
// (SHomesteadMenu::SettingsTabCount) or notice surface (HomesteadNoticeStyle::ESurface) has none.
#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING
#include "Simulation/HomesteadSimulation.h"
#include "UI/HomesteadNoticeStyle.h"

class AHomesteadController;

struct FHomesteadUIGallery
{
    // What an entry stands for in the coverage check.
    enum class ECover : uint8 { BookPage, SettingsTab, Notice, Focus, Hud, Dialog, Shop, Setup };
    // Every notice surface the game draws (UI/HomesteadNoticeStyle.h); each needs an entry.
    using ENotice = HomesteadNoticeStyle::ESurface;
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
        // Shown over the world even on the plain backdrop (the garden outlines are world geometry).
        bool bKeepWorld = false;
    };

    static const TArray<FEntry>& Entries();
    static const FEntry* Find(const FString& Id);
    // "all", or ids separated by commas; unknown ids are an error.
    static TArray<FString> Resolve(const FString& Spec, FString& Error);
    // Book tabs, settings tabs and notice styles with no entry ("book-page-3", "notice-5").
    static TArray<FString> MissingCoverage(const AHomesteadController& Controller);
    // Only on an isolated save route (the smoke sandbox, or a -HomesteadPreviewProfile editor): the
    // gallery changes the game it runs in (clock, stock, plots, names). Empty when allowed, else why not.
    static FString RefusalFor(const AHomesteadController& Controller);
    // Puts `Id` on screen in this game; OnReady(true) once it has settled, or (false, why).
    static void Show(AHomesteadController& Controller, const FString& Id, bool bPad,
        TFunction<void(bool, const FString&)> OnReady);
    // Forgets the fixture taken from the current game (the next Show takes a fresh one).
    static void ResetFixture();
    // Plain backdrop (Jenny, 2026-09-30): the UI in its real screen positions with the game out of the
    // way. Every world actor is hidden from her camera and the sky, fog and particles are off; a flat
    // warm-grey unlit plane fills the view behind the heroine, who stays (bHeroine) so the hints and
    // outlines still read against her. Entries with bKeepWorld (garden outlines) keep the world.
    // Call before each capture (the camera and streamed actors change); bPlain false restores it.
    static void SetBackdrop(AHomesteadController& Controller, bool bPlain, bool bHeroine);

private:
    static void Prepare(AHomesteadController& Controller, bool bPad);
    static void Face(AHomesteadController& Controller, Homestead::Point At);
};
#endif
