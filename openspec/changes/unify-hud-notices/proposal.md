# Proposal

## Why

Playtest feedback (docs/handoff/round-2.md, "Unified hints and toasts"): the HUD had four unrelated
channels for guidance, each with its own look - a permanent top-left controls strip, the focus cue
floating over her head, pine world toasts, and the field book's parchment NoticeCard.

## What Changes

- **One notice style.** `UI/HomesteadNoticeStyle.h` holds the parchment notice (paper, double-ruled
  frame, iron-gall ink, rust for errors). The field book's NoticeCard and the HUD's world notices both
  use it.
- **Top-centre notices.** The focus actions (target name, key/pad stamps and verbs) are a parchment
  card at the top centre, under the compass band. The toast is a parchment slip sized to its text,
  stacked under the focus card when one shows. Errors keep their priority (an error isn't replaced by a
  success until it expires) and read in rust. Device-specific glyphs are unchanged; nothing takes focus.
- **First-minute controls.** The top-left controls strip shows only for its first 60 seconds of real
  time on screen after boot, a new game or "Reset action hints". Time in the book, shop, setup or
  failure screens doesn't count. Keyed focus hints still retire after three successes
  (`GameUserSettings` `Homestead.ActionHints`). The compass waits until the strip retires, so they
  never share the top row.

## Impact

- New `UI/HomesteadHudTiming.h` (portable `ControlsHintWindow`) with native tests
  (`HomesteadHudTimingTests`), new `UI/HomesteadNoticeStyle.h`.
- `HomesteadHUD.*` (NoticeCard, top-centre cue and stacked toast, controls strip gated),
  `HomesteadController.*` / `HomesteadControllerFocus.cpp` / `HomesteadControllerSaves.cpp` (the window,
  restarted by a new game and a hint reset), `HomesteadMapComponent.cpp` (compass waits),
  `UI/SHomesteadMenuPrivate.h` (NoticeCard reads the shared style), `HomesteadFeedbackTest.cpp`
  (ink colours), `UI/HomesteadNativeMenuTest.cpp` (the strip counts down on screen and freezes in the book).
- Behaviour change to disclose: the focus cue no longer floats over her head.
