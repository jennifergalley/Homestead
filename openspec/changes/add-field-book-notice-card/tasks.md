# Tasks

## 1. Notice card

- [x] 1.1 Remove the toast banner from the field book's layout (`SHomesteadMenu`)
- [x] 1.2 Add the notice card: parchment slip, double-ruled frame, EB Garamond (`DisplayFont`), centred wrapping text, rust ink for errors, `HitTestInvisible`
- [x] 1.3 Timing: fade in, 2.6 s (errors 3.8 s), fade out; the latest message replaces the one showing; stale world toasts don't reappear when the book opens
- [x] 1.4 Placement: bottom centre, below the tabs when the focused control would lie under it
- [x] 1.5 Build SurvivalGameEditor

## 2. Verification

- [ ] 2.1 PIE at 1080p and 4K: a transfer, a held craft (fill not covered), and rapid successive notices; before/after captures show no page shift, focus or selection change
- [ ] 2.2 A long error wraps within the card; a confirmation dialog still looks and behaves modal
