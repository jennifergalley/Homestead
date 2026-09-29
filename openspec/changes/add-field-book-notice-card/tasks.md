# Tasks

## 1. Notice card

- [x] 1.1 Remove the toast banner from the field book's layout (`SHomesteadMenu`)
- [x] 1.2 Add the notice card: parchment slip, double-ruled frame, EB Garamond (`DisplayFont`), centred wrapping text, rust ink for errors, `HitTestInvisible`
- [x] 1.3 Timing: fade in, 2.6 s (errors 3.8 s), fade out; the latest message replaces the one showing; stale world toasts don't reappear when the book opens
- [x] 1.4 Placement: bottom centre, below the tabs when the focused control would lie under it
- [x] 1.5 Build SurvivalGameEditor

## 2. Verification

- [x] 2.1 PIE 1080p: a notice doesn't move the page (pixel diff of tabs, recipe row and details panel before versus during: 0 changed pixels); the latest replaces; the card moves under the tabs when the focused equipment slot is under it; a held craft's white fill stays visible and "Crafted a worn hoe." shows below it
- [ ] 2.2 4K: the HUD was checked in standalone 4K, but the book card wasn't captured at 4K (standalone key injection couldn't raise a notice with the book open). The card scales with the book's own ScaleBox.
- [x] 2.3 A long error: rust ink and frame. An unbroken 100-character word was clipped at first; per-character wrapping was added and compiles, but hasn't been re-captured.
