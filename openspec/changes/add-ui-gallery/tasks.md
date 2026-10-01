# Tasks

- [x] 1.1 Gallery registry with an isolated fixture, entries for every surface and a coverage check (HomesteadUIGallery.*)
- [x] 1.2 Capture run on the smoke route (HomesteadUIGalleryTest.cpp, Test-Game.ps1 -UIGallery)
- [x] 1.3 Capture-UiGallery.ps1 and ui_gallery_sheet.py: per-resolution and per-input folders, viewable copies, contact sheets, index.md
- [x] 1.4 Live PIE: homestead.UIGallery, editor_mcp.py gallery, McpHelpers gallery
- [x] 1.5 Focus card and world notices in the book's EB Garamond on parchment
- [x] 1.5a Seed-outline states (plant, occupied, select, untilled), marked pending until Water's seed outline (jennifergalley-seed-outline @6408cdd7) is on this line; the Names step; night and rain HUD and notices
- [x] 1.5b Review fixes: runs only on the sandbox or a preview save route, with a check that the normal Estate saves are untouched; coverage from SHomesteadMenu::TabPages, SettingsTabCount and HomesteadNoticeStyle::ESurface; carrot seed (in season); setup-new-game shown despite -HomesteadSkipNewGameSetup; Test-Game runs the game in a kill-on-close job, matched by its image
- [x] 1.5c Plain backdrop (default) with the heroine kept, -NoHeroine, -Backdrop World; world kept for garden outlines and night/rain
- [x] 1.6 Compile, then a first full pass at 720p and 4K. Look at the captures, fix what looks wrong, re-capture once
- [x] 1.6a Theme trial: parchment and EB Garamond across the UI (homestead.UITheme), full-width book, AA contrast; captured 1080p classic + parchment (plain and World day/night/rain) and 720p + 4K parchment, viewed and fixed once, recaptured
- [x] 1.6b Commit the downscaled 1080p plain set to docs/ui-gallery/<date>/ (index.md, contact sheet, view JPEGs, under 50 MB) as the reference for a cohesive period-themed UI
- [x] 1.7 Docs agent: a "Seeing every UI" section in the unreal-editor-mcp skill (main c3ca184b)
- [x] 1.6c Jenny approved parchment (2026-09-30 21:54): it is the default; classic is kept for comparison only. New UI surfaces use the theme and get a gallery entry
- [ ] 1.8 720p legibility: HUD serif words at least 15 font units, popovers captured in the gallery (source a37d693d; verify in the re-run slot)
