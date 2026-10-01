# Add a UI gallery

## Why
Jenny (2026-09-30 17:32): agents designing her menus, settings, pop-ups, hints and HUD must be able to see each one as she does, at her resolutions, to understand what she means. `hshot` drops Slate and `shot` misses floating PIE windows, so agents worked from logs.

## What changes
- `FHomesteadUIGallery` (Source/SurvivalGame/HomesteadUIGallery.*, Development builds only): a registry of named states. Each state starts from one isolated fixture: 10:00, a known pack, 1,000 coins, Energy 80, nothing open, no toasts. It opens exactly one surface. Covered: every field-book page and settings tab, the dialogs, the shop, the road-sign confirm and refusal, world and book notices, focus hints and garden outlines, the HUD (Energy, Well fed, season warning, hotbar, pickups, controls strip) and new-game setup. Keyboard or controller hints on request.
- A coverage check fails the capture run if any book tab, settings tab or notice style has no entry.
- Capture run: `-HomesteadSmokeTest -HomesteadUIGallery[=ids] [-HomesteadUIGalleryInput=Pad]` on the Estate map (sandboxed saves). It captures the game's own window, Slate included, as `<id>.png` with `gallery-index.tsv`. `Test-Game.ps1 -UIGallery` runs it.
- `Scripts\Capture-UiGallery.ps1 -Ids -Res 720p,1080p,4K -Input KBM,Pad` runs it at each resolution and input under `E:\CopilotScratch\<session>\ui-gallery\<stamp>\`. `ui_gallery_sheet.py` adds viewable copies, a contact sheet per run and `index.md`.
- Safety: the gallery runs only on the smoke sandbox or a `-HomesteadPreviewProfile` save route (Start-EditorMcp.ps1 -PreviewProfile gallery), never a real save. The capture run proves the normal Estate saves are unchanged. Test-Game runs the game in a kill-on-close job.
- Live PIE: `homestead.UIGallery list | <id> | next | prev | all [Pad|KBM]`, `editor_mcp.py gallery <id>` and the `gallery` helper in McpHelpers.ps1.
- World notices and the focus card set their words in the field book's EB Garamond on the parchment card; key glyphs stay in the crisp sans.

## Impact
Development-only code, compiled out of Shipping. No save or simulation change.
