# Tasks

Source work is authorized in this isolated lane after plan validation. Main owns
the live endurance run and all engine/compiler slots; no competing execution.

## 1. Directional browsing increment

- [x] 1.1 Extend existing grid/analog helpers with explicit edge results, desired-column and left-stick repeat behavior; verify focused portable tests through the existing approved slot.
- [ ] 1.2 Connect actual Slate focus, native spatial boundary navigation and semantic subject IDs; verify grid-to-equipment/reverse, sideways/upper controls, empty content and scrolled rows through real directional input.
- [ ] 1.3 Preserve native focus across rebuilds, removals and tab return, with explanatory unavailable controls and no navigation activation; verify selected IDs/highlights/focused widgets agree.

## 2. Modal and input consistency

- [ ] 2.1 Trap modal focus and add explicit quantity edit entry/exit; verify directions do not edit while browsing, Back leaves edit before cancel, and cancel/confirm preserve transaction semantics.
- [ ] 2.2 Route D-pad, left stick and keyboard arrows consistently without world movement or implicit portrait rotation; verify the existing intent classifier, mouse behavior, LB/RB and three-press quit path remain intact.
- [ ] 2.3 Update footer and guide guidance to directional-first navigation with optional trigger/Tab convenience; verify hints describe the actual active browsing/edit context.

## 3. Integrated acceptance

- [ ] 3.1 Extend existing native UI tests with actual D-pad and left-stick boundary sequences, real focus assertions and modal/empty/short/full cases; verify compiled tests rather than a direct region-shortcut proxy.
- [ ] 3.2 Integrate after main releases the shared engine, run targeted 720p/4K boundary playtests and record limits; verify current save/quit/input behavior and unchanged visual/asset/world scope before delivery.

## Current evidence

Source `6b96961` reached a real guarded /W4 /WX portable compile/link, then
failed `held stick repeat`: `2 + .28` exceeded literal `2.28` by one double ULP.
That failed evidence is retained. Correction `6f0bb37` keeps the 280/180 ms
repeat timing with a one-microsecond comparison tolerance; added checks reject
one-millisecond-early repeats and test both inclusive deadlines.

The exact corrected helper/test passed 58,099 checks through the unchanged
guarded adapter. Evidence is in source-owner session
`6e4e8162-6f64-459c-a766-ba613d11c82f`, `files\directional-tests-6f0bb37`
(`compile-navigation`, `link`, `tests` result.json/stdout.log). All three leaves
exited zero with one process and released their markers. This proves the portable
logic, not native spatial focus or user/controller comfort. Main owns the next
native build and actual D-pad/left-stick route; all corresponding tasks remain open.
