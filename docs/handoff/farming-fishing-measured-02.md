# Farming Fishing measured build 02

Target: `20261003-measured-02`, Oct 3 9 PM local; fallback Oct 4 7:30 AM.
Authorization: `2026-10-04T01:55:51.513Z`, scope source `8b67ebc0`; coordinator's
registry update `5e00cc7f` observed on origin/main. Checkout was complete/clean
at `8b67ebc0`; no reset/restore was performed.

App session `d4518710-b508-4196-8598-1cf91d0edf7d`; runtime
`5be207bc-49b1-4a1b-811e-088ae565dc1b`; branch
`jennifergalley-farming-fishing-agent`; worktree `jennifergalley-cautious-robot`.
Actual model/effort `gpt-6.1-sol` / `high`, confirmed by local usage events
69327-69329. Launch context default; actual runtime context tier unknown.
No helpers, editor, live process, automation or release/save dependency.

## Boundaries and selected feedback
| Feedback | Change | State |
| --- | --- | --- |
| `jenny-mut56ggp-uijcta`, `jenny-mut57myz-9tfjci` | `fix-seed-planting-hint-and-harvest-balance` | source implemented; compile/acceptance pending |
| `jenny-mut5gsd4-oebvov` | `add-basic-crop-cookfire-recipes` | source implemented; compile/acceptance pending |
| `jenny-mut592z8-2od0d5`, `jenny-mut5legr-oba2rd` | existing `add-shore-and-river-fishing` | pending coherent fishing checkpoint; do not duplicate |

First delivery: seed hint/balance and eight crop meals. Then fishing pole at exactly
1500 coins beside backpack upgrade, inventory/hotbar, original active fishing,
river/lake/ocean catches, General Store sale and fish preparations.
No carry/grip/hair/foliage/rucksack/manor relocation or speculative scope.

Travel Rest (`1b0e10a4-09b5-458e-b9de-098cce831b07`) owns bed/time/wait/discovery/travel.
Its Simulation State/declarations and optional travel save hook must survive integration.
Our shared edits are localized Recipe/crafting/HarvestCrop and shop goods/trade only;
no wait edits. Report later fishing save additions before [ready]. Integration/docs liaison:
`e528fd4a-5aed-4c95-9463-a37941afc00b`; coordinator:
`146ed534-2f78-48ba-b0fc-98436c1f3223`.

## Findings and reusable implementation context
- `HomesteadHUD.cpp::DrawInteractCue` retires a lone keyed cue after three uses.
  Fix the actual retirement policy, not only FocusActions' already-present plant string.
- `CheckSow` / `DescribeSow` share eligibility; input E plants, tool click works.
- `CropTable`: only cultivated legacy roots yield separate seeds (two guaranteed);
  period crops already yield no seed packets. Preserve wild forage and regrowing fruit.
- Implemented tuning: cultivated roots one seed at 25%, deterministic plot/day; eight new
  Meals use one kindling and existing fire/hearth checks. Recipe design lists dishes
  and round(12 + 0.6 * consumed sale coins) nominal Energy.
- `SellPrice` equals BasePrice, not a presumed discount. Item/Recipe append-only;
  counted stocks already load narrower saves with new items zero.
- `CraftChange`, `Cooking`, `RecipeName`, cookbook metadata and native fixtures
  iterating every Recipe need coherent wiring. Prefer a small shared recipe helper.
- Existing fishing proposal is a corrupted System.Object[] stub; repair it before
  fishing planning/apply. Liaison notified; no external assets/dependencies needed
  for the first checkpoint.

## Checkpoint and next action
Coordinator supplied the separate apply instruction after the short plans; both
changes are implemented. Eligible selected seeds use the authoritative DescribeSow
cue and Plant Seeds alone bypasses three-use retirement. Other hints still retire.
Eight append-only Meals share ingredient/output data with crafting and assessment;
the cookbook shows output icons and nominal Energy. No serializer/tag/version/bake
changes; inventories widen through the existing counted-stock format.

The first native pass compiled all selected targets and passed Economy/Manor; its
simulation run exposed a directly coupled old full-pack fixture assuming six root
harvest items. The fixture now tests the actual four-root plus zero/one-seed yield,
including a carried seed for the later replant. The corrected Release native pass
passed Simulation, Economy and Manor (3/3, 210.16 seconds).
Focused new coverage exercises retirement, both seed outcomes/reload, a 1024-sample
20-30% frequency bound, unchanged wild seeds/bought crops, all eight meal ingredients,
kindling/station refusal atomicity, Energy/eating and current/narrower stock saves.
Both OpenSpec changes pass strict validation; risky source wiring reviewed directly.

Jenny's newest decision (19:12 local, Oct 3) supersedes the earlier broad pause:
Editor/Blender and scheduled fishing source work may proceed in slot1, but NO game
compile/UBT/Live Coding/UAT/package while she plays. No editor needed or launched.
This checkpoint is NOT compile-verified or release-ready until Integration's editor
compile and Jenny's manual checks. No [ready] claim while that gate is blocked.
Never touch Jenny's game/saves/shortcut, no overnight automation.

Native command: CMake Release targets HomesteadSimulationTests,
HomesteadEconomyTests, HomesteadManorTests, then CTest selecting those same suites.
Both strict OpenSpec validations and git diff --check pass. The native Manor target
reported an existing C4456 shadowed-local warning at Tests/HomesteadManorTests.cpp:309;
that unrelated source was not changed. No editor compile has been attempted.

Player checks after Integration clears compilation: select seeds and plant at least
four eligible tilled plots without losing [E] Plant Seeds; harvest roots and see
occasional singles, not guaranteed pairs. Inspect the eight new recipes and their
Energy, cook a herb-seasoned dish beside the fueled fire/hearth, then eat it; each
batch uses one kindling. Jenny's acceptance tasks remain open.

Commands already run: git status/log (clean checkout), openspec list/context/spec
inventory, full relevant crop/crafting specs, new change/status/instructions.
Notion identity and Ledger read in full; no technical-project Notion writes.
Canonical startup sources: README.md, round-3.md, agent-lifecycle.md,
measured-build-02.json and full selected backlog feedback.

Usage reference: `accounting/farming-fishing-measured-02-usage.json`, exported from
metadata-only local usage via the adjacent lane allocation. Startup/planning is a
bounded overhead segment, implementation begins at 2026-10-04T02:04:27.787Z.
Interim coverage includes source work; terminal response and later calls need recapture.
The adjacent `farming-fishing-measured-02-report.json` is the provisional attributed
aggregate; recorded nano-AIU is not a reconciled billing claim.
All usage belongs to build 02, never the closed build 01 allocation. No actual-context
or reconciled-credit claim. TEMP/TMP for heavy tools must use
`E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\tmp`.
