# Proposal

## Why

The native menu still treats Settings like an inventory grid inside the field book, uses repeated button presses for continuous audio values, exposes a Credits tab Jenny does not want, and does not open Settings directly from Esc. The woodland floor would also benefit from restrained purely decorative flower color between interactive forage patches.

## What Changes

- Add sparse deterministic wildflower groundcover using the already imported and credited Flower Empodium meshes/material, with no new asset download or license.
- Keep decorative flowers noninteractive, noncolliding, nonoverlapping, nonnavigating, batched, and visually smaller/sparser than the gatherable flower resource.
- Respect home, path, stream, resource, plot, structure, building-site, and active-window lifecycle exclusions so decorative flowers never obscure gameplay or refill cleared resource sites.
- Separate Settings from the inventory/guidebook tab set. Esc on keyboard opens Settings directly from gameplay; Esc inside Settings resumes gameplay.
- Use controller Menu/Start for the separate Settings overlay and retain controller View/Back for the guidebook, while keeping `I`, `C`, and `B` purpose-specific field-book entry points and making `G` open Guidebook directly during gameplay.
- Replace the Settings two-column inventory-like grid with a vertically scrolling labeled list designed for preferences and session actions.
- Replace raw `Day length: 30/60/120 minutes` copy with a clear `Game speed` three-choice control: `Leisurely` (120-minute day), `Balanced` (60-minute default), and `Fast` (30-minute day), without displaying implementation minutes in the normal Settings UI.
- Replace Music, Ambience, and Effects volume cycling buttons with focusable continuous sliders, including pointer drag and keyboard/controller left/right adjustment.
- Persist audio volumes immediately as user-level settings independent from world saves, with exact-property write/readback and rollback behavior matching camera/video preferences.
- Remove the Credits tab, Credits page, navigation target, and in-game Credits icon entirely. The packaged `asset-credits.md` remains the durable attribution surface.
- Move preview profile/version metadata out of the permanent gameplay HUD and into Settings only; normal builds do not gain a gameplay build label.
- Preserve Settings pause, save/load, new woodland, quit/recovery, camera preferences, video preferences, accessibility, focus synchronization, mouse parity, and 720p/4K layout.

The smallest useful in-game result opens a separate one-column Settings list from Esc and removes Credits. The first playable demonstration also drags and keyboard-adjusts real volume sliders, relaunches without saving a world to prove persistence, and walks through a patch of decorative wildflowers that never takes focus. Full acceptance covers controller parity, recovery/exit modals, every settings row, layout, save isolation, flower exclusions/lifecycle, cadence, and immutable Shipping replay.

Deferred scope includes key rebinding, graphics preset dropdowns, advanced audio device selection, localization, a standalone legal/about screen, additional flower asset acquisition, seasonal flower swaps, pollinators, and making decorative flowers gatherable.

## Capabilities

### New Capabilities

- `settings-screen-navigation`: Separate Settings overlay, vertical preference list, friendly Game speed choices, direct Esc/Menu access, direct G-to-Guidebook routing, Settings-only preview metadata, and removal of the Credits tab.
- `audio-volume-controls`: Immediate persistent Music/Ambience/Effects sliders with pointer and controller/keyboard parity.
- `decorative-wildflower-groundcover`: Sparse deterministic noninteractive wildflower decoration using admitted project assets.

### Modified Capabilities

None.

## Impact

- `HomesteadController`, `SHomesteadMenu`, `HomesteadHUD`, menu icons, input/prompt/navigation/native-menu tests: separate Settings routing, six-or-fewer field-book pages, vertical list layout, and Credits removal.
- User-settings persistence: camera-style exact-property handling for three audio values; legacy world-save fields may remain compatible but cease to be authoritative.
- `HomesteadWorld` cover generation and environment tests: one or two Flower Empodium HISM batches with deterministic exclusions and bounded density.
- Existing Flower Empodium assets are Poly Haven CC0 and already imported, verified, cooked, and credited. No new license, account, purchase, download, or attribution is required.
- Packaged `asset-credits.md` remains available even though the in-game Credits tab is removed.
- `work-animation-complete-02-shipping / work-actions-v13` remains rollback until this change is implemented and promoted.
