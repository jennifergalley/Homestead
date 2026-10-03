# Measured build 01 integration

Build `20261002-measured-01`; Integration runtime
`3554b767-ebd7-436c-a691-13795fecad77`, branch
`jennifergalley-integration-agent`, worktree `jennifergalley-redesigned-couscous`.
Actual call model/effort is GPT-5.6 Terra / medium; launch context is default and actual context
is unknown.

## Accounting checkpoint

- Merged Accounting's exact handoff `848e4b4841b5e306fdedf6fbc94336f409a7d65d` after the
  coordinator metadata update `2e66035ede230b44a4eaa58332f6db2cdc492397`.
- Targeted validation passed: 16 Node tests in `BuildUsageTests.mjs`/`PlannerBacklogTests.mjs`
  and 2 Python exporter tests.
- Added non-overlapping integration telemetry segments:
  accounting merge/validation through event `68853`, then refreshed usage capture afterward.
  The refreshed durable report records 180 calls and `621342150000` nano-AIU through event
  `68862`; it remains incomplete pending lane terminals, review, later integration and billing
  reconciliation.

## Admission review

- Independent read-only review ran as nested agent
  `be475939-47b6-4de1-85a4-220ba579d0b4` in this Integration runtime. Telemetry confirms
  GPT-6.1 Sol / high; its actual context is unknown. Review calls are separately allocated through
  event `68939`.
- Reviewed `80ec592828329b7b0d5171b4890cd934f47a8be0`, UI candidate
  `f881c28c9c112c78e199678ebf425671c3f8ff24`, Town implementation
  `964aecb37c6b387f2d194cf9938ee0aee8cdcf69`, and Town delivery
  `6e9c3a9aaac7e3c1ceb9a5ec0e07f8ef01895b38`.
- Town passed review: crop sales preserve atomic wallet/inventory/shop updates and sign identities,
  arrival, resources, save format and bake version are unchanged.
- UI promotion is blocked pending fixes for a legacy crafted-away pack-slot save rejected before
  reconciliation and keyboard/controller commit onto an empty pack square.
- UI fix `440d4de5bea60cddcb5fd5cb751e05dc1c8ca0e5` passed independent re-review. Its legacy
  migration preserves structural/duplicate refusal before retiring only valid issued-and-absent
  references; its input guard permits `EmptySlot` only for an existing drag. Native and editor
  compile evidence passed; the mapped A/Enter regression compiled but was not executed.

## Build and validation checkpoint

- Integrated Town at merge `07ec7235` and the cleared UI replacement at merge `6c206a73`
  (`440d4de5bea60cddcb5fd5cb751e05dc1c8ca0e5`); the replacement's independent re-review passed.
- Combined Release native validation passed all 19 suites. The Development `SurvivalGame` target
  compiled successfully.
- The first package bootstrap re-saved 900 tracked Content assets. Their actual bytes were backed
  up with matching hashes under
  `E:\CopilotScratch\3554b767-ebd7-436c-a691-13795fecad77\package-bootstrap-evidence`, then only
  that enumerated list was restored. A clean `-PackageOnly` recook completed with Content clean.
- Current Shipping candidate is
  `Build\Releases\20261002-measured-01\shipping-candidate`; it was staged from unchanged clean
  cooked containers, without editor/bootstrap/cooker/UnrealPak. Guarded Shipping EstateSmoke and
  ToolRepeat passed.
- Shipping FullLoop is explicitly excluded: it requests the Woodland map, while Shipping forces
  Estate and FullLoop has no Estate adaptation. Its Pack-start assertion failure is invalid
  Shipping coverage, not a UI regression; no source change was made.
- Isolated Development FullLoop passed on the current integrated source/package. It passed every
  engine assertion, including Pack startup, exact chest-grid transfer and save reload; its
  performance sample recorded mean 43.49 FPS, p95 29.10 ms and p99 30.14 ms over 11,052 samples.

## Next action

Retain the promoted candidate and the old rollback package; do not archive this release-holding
worktree while the shortcut, running game or retained release depends on it, even after acceptance.

## Delivery

- Promoted the Shipping candidate to `Homestead Estate.lnk` after the valid gates. The shortcut
  now targets
  `Build\Releases\20261002-measured-01\shipping-candidate\Windows\SurvivalGame\Binaries\Win64\JennysHomesteadGame.exe`;
  its executable SHA-256 is
  `272613AA2892C6B6A4A618A5277747A81ECA0AA72A997C14DCBB3CB5538E93D8`.
- Copied and hash-verified all 20 package-local SaveGames/Config files before retargeting. The
  old `20261001-9pm-shipping` package remains the single rollback, and the shortcut retained
  `Homestead.ico`. Promotion evidence is under
  `E:\CopilotScratch\3554b767-ebd7-436c-a691-13795fecad77\promotion-evidence`.
- The source/package receipt is `d2155333`; the candidate receipt records Shipping staging at
  `2026-10-03T06:34:05.7513339+00:00` from the clean reused cook and container hashes. This is
  not promotion time: the retained promotion receipt proves the shortcut was retargeted at
  `2026-10-03T06:46:31Z`, after the Development FullLoop pass.
- Refreshed measured accounting captured 419 calls and `1675964350000` nano-AIU through event
  `69106`. It remains incomplete: delivery responses after that cutoff, actual context tiers,
  legacy historical costs, and external billing reconciliation are unavailable.
- Jenny considers all shipped player checks complete as of 2026-10-03: dark book/HUD consistency,
  map-click fast travel, pack-square persistence after reload, crop sales, and the signpost's road
  placement/direction. This records her acceptance, not additional automated execution.
  Development FullLoop FPS remains route telemetry, not an Estate performance benchmark.
- Closure capture closed the stopped lane tails at Accounting `69075`, UI `69076`, and Town
  `68910`; final coordinator/integration delivery overhead is closed through `69151`/`69152`.
  Later activity is intentionally outside build01. Actual context tiers, legacy historical costs,
  and external billing reconciliation remain unavailable.
- The final pre-park export supersedes the earlier checkpoint: it allocates 454 calls and
  `1797874160000` nano-AIU through the final build01 cutoff at event `69152`, with no missing
  configured session. The exporter observed records through `69159`; eight pre-/post-cutoff
  records are excluded so later work is not charged to build01.
- `JennysHomesteadGame.exe` began from the promoted candidate at `2026-10-03T06:48:32Z`. It is
  Jenny's live playtest process and was not touched; this worktree is release-holding and must not
  be archived while the shortcut or process depends on it.
