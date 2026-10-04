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
No helpers, editor, owned live process or automation. No game/save/shortcut touched.
Source still needs Integration's editor compile and release-save-isolation admission.

## Boundaries and selected feedback
| Feedback | Change | State |
| --- | --- | --- |
| `jenny-mut56ggp-uijcta`, `jenny-mut57myz-9tfjci` | `fix-seed-planting-hint-and-harvest-balance` | source implemented; compile/acceptance pending |
| `jenny-mut5gsd4-oebvov` | `add-basic-crop-cookfire-recipes` | source implemented; compile/acceptance pending |
| `jenny-mut592z8-2od0d5`, `jenny-mut5legr-oba2rd` | existing `add-shore-and-river-fishing` | source/native verified; editor compile and acceptance pending |

First delivery: seed hint/balance and eight crop meals. Then fishing pole at exactly
1500 coins beside backpack upgrade, inventory/hotbar, original active fishing,
river/lake/ocean catches, General Store sale and fish preparations.
No carry/grip/hair/foliage/rucksack/manor relocation or speculative scope.

Travel Rest (`1b0e10a4-09b5-458e-b9de-098cce831b07`) owns bed/time/wait/discovery/travel.
Its Simulation State/declarations and optional travel save hook must survive integration.
Our shared edits are localized Recipe/crafting/HarvestCrop, ephemeral fishing reset/probe,
and shop goods/trade only; no wait edits. Fishing adds no save section. Integration/docs liaison:
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
- Existing fishing proposal was a corrupted System.Object[] stub; repaired the same
  change and completed short design/spec/tasks before apply. No duplicate change,
  external assets, dependencies, new animations or editor launch.

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

## Fishing source checkpoint
The commit containing this handoff follows pushed seed/crop checkpoint
`249229b0d5c3f072d4fb6384e9ded0cb0f503d9c`. It adds the pole to ordinary General Store
goods at exactly 1500 coins regardless of markup, before or after the backpack upgrade.
Pack layout, hotbar selection, held-tool presentation and the generic cookbook are wired.
The held pole reuses the original wooden digging-stick mesh/pose with a unique component;
its attachment and UI presentation remain engine-unverified.

Native fishing owns pole/Energy/space/water eligibility, the 1.5 Energy cast charge,
2-4 second wait, 0.9 second hook window and two 1.8 second landing passes (55-85% band).
E/LMB or A/RT casts/hooks/lands; Esc/B, walking away or changing tool/menu cancels.
Only the final successful beat awards one fish. River trout/salmon, lake perch/carp
and sea mackerel/bass are distinct pools. All sell to the General Store; raw catches
are prepared before eating. Eight fish preparations cover raw sushi-style slices,
three grills, salmon soup, fish/ember potatoes, herbed carp and mackerel/bean chowder.
Seven cooked dishes consume one kindling at the existing fire/hearth; raw mackerel
slices need no station/fuel. Sale-opportunity Energy uses the shared crop-meal curve.

The new scaled Slate card is 520x200 logical units, centred 180 units above the bottom.
Its timing target has contrasting boundary marks and a two-tone marker, not colour
alone. Habitat/readiness are cached on focus refresh, never probed in HUD paint; only
constant-size session state animates per frame. Gallery adds staged bite/landing and
pole-purchase entries. The gallery restores the real water probe immediately after
setting up its staged cast; these entries are not evidence of live-water verification.

Release native Simulation/Fishing/Economy passed 3/3 (177.87 seconds). The final extended
Fishing executable passed 1036 checks, including 1499-coin refusal, exactly-1500 purchase,
pack-row/save retention, all habitats, timing boundaries, bad/late/cancelled casts,
full pack, changed water, load/new-game resets, all eight preparations and actual
uncapped Energy gain. Initial fixture failures were corrected: the all-recipes stock
needed room for Split Firewood after adding six fish; estate load fixtures need the
matching placement context. Gameplay/serializer behavior was not changed for those fixes.
All three selected OpenSpec changes pass strict validation; git diff --check passes.
State-machine/input/recipe/save wiring was reviewed directly; no UBT/Live Coding attempted.

Integration must preserve HUD's removal of obsolete pickup mounting/root cleanup and
SelectHotbarSlot's redundant one-second name toast while retaining FishingRoot and
active fishing's selected-tool presentation. Preserve HUD's QuietActionSerial changes
in BeginHintUse/EndHintUse/NotifyResourceAction: fishing uses that helper for intermediate
success; its card owns progress, while terminal catch/cancel/refusal uses normal notices.
Travel Rest's State/discoveredTravel and independent optional travel save section remain
separate; preserve those localized hunks. Item/Recipe append-only, counted stocks,
SimulationSaveVersion 13 and placement bake unchanged; in-flight fishing is not saved.

This is a source checkpoint, NOT [ready]. Remaining gates: coordinated editor compile,
actual river/lake/sea reach and pole/card inspection, release-specific save isolation,
then Jenny's integrated manual acceptance. Older packages can reject wider stock saves
and fall back to a backup; append-only widening alone is not release rollback safety.
No gameplay admission/promotion until Integration verifies isolated save routing.
Player check: buy/select the pole for 1500, catch at each habitat using the timing card,
sell a catch, prepare/eat raw slices and a kindling-fired dish. No packaging, main merge,
shortcut retarget, live-game/save access, polling or overnight automation by this lane.

Commands already run: git status/log (clean checkout), openspec list/context/spec
inventory, full relevant crop/crafting specs, new change/status/instructions.
Notion identity and Ledger read in full; no technical-project Notion writes.
Canonical startup sources: README.md, round-3.md, agent-lifecycle.md,
measured-build-02.json and full selected backlog feedback.

Usage reference: `accounting/farming-fishing-measured-02-usage.json`, exported from
metadata-only local usage via the adjacent lane allocation. Startup/planning is a
bounded overhead segment, implementation begins at 2026-10-04T02:04:27.787Z.
Fishing implementation begins at 2026-10-04T02:17:28.345Z. Interim coverage includes
source work; terminal response and later calls need recapture.
The adjacent `farming-fishing-measured-02-report.json` is the provisional attributed
aggregate; recorded nano-AIU is not a reconciled billing claim.
All usage belongs to build 02, never the closed build 01 allocation. No actual-context
or reconciled-credit claim. TEMP/TMP for heavy tools must use
`E:\CopilotScratch\5be207bc-49b1-4a1b-811e-088ae565dc1b\tmp`.
