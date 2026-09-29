# Tasks

## 1. Notice card

- [x] 1.1 Remove the toast banner from the field book's layout (`SHomesteadMenu`)
- [x] 1.2 Add the notice card: parchment slip, double-ruled frame, EB Garamond (`DisplayFont`), centred wrapping text, rust ink for errors, `HitTestInvisible`
- [x] 1.3 Timing: fade in, 2.6 s (errors 3.8 s), fade out; the latest message replaces the one showing; stale world toasts don't reappear when the book opens
- [x] 1.4 Placement: bottom centre, below the tabs when the focused control would lie under it
- [x] 1.5 Build SurvivalGameEditor

## 2. Verification

- [x] 2.1 PIE 1080p: a notice doesn't move the page (pixel diff of tabs, recipe row and details panel before versus during: 0 changed pixels); the latest replaces; the card moves under the tabs when the focused equipment slot is under it; a held craft's white fill stays visible and "Crafted a worn hoe." shows below it
- [x] 2.2 Standalone 4K: with the book open, holding Enter on the hoe recipe showed the white fill, then "Crafted a worn hoe." bottom centre; the tabs and recipe rows didn't change (0 changed pixels, before versus during)
- [x] 2.3 A long error: rust ink and frame; an unbroken 100-character word now wraps inside the card at 4K (per-character wrapping), with no clipping
