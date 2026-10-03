# Design

## Context
See proposal.md. Main already contains HUD palette work (34f0b126) and deferred
menu/HUD rebuilding (1b73feca); this pass verifies and repairs concrete gaps.

## Decisions
Reuse HomesteadUITheme, HomesteadNoticeStyle and the existing theme-change
delegate. Preserve parchment/EB Garamond and the approved dark tokens; no new
theme framework or layout redesign. Rebuild cached Slate colours after a palette
change without losing the active page, selected setting or keyboard focus.
UI owns HUD/book/naming surfaces; Town owns shop/trade changes and receives only
necessary palette-interface findings.

## Risks / Verification
Cached brushes and copied colours can retain the previous theme. Inspect those
surfaces and the deferred focus path; compile once with the other UI fixes.
The contrast reference is ../pitch-dark-parchment-theme/design.md.
Jenny performs the existing player-checkable tasks; source inspection is not
visual acceptance. No editor screenshots or packaged tests in this lane.
