# Round 3: work-driven builds with a lean team

Started 2026-10-01, 22:30. Jenny asked for a token-efficient team: a fresh orchestrator on GPT-6 Sol,
fresh implementers spawned only for planner-assigned work, and the rules in "Token budget" in
[README.md](README.md). Read README.md first, then this page. Older round pages are history only.

## Jenny's standing rules

- Lanes implement **only** what she assigns to the planner's Next build or Build after next slot, then go idle.
  No autonomous pickup.
- She does all playtesting and verification. Lanes run native tests and a compile; they open the
  editor only when the work can't be checked otherwise (animation and art).
- Slots are work-driven: Next build delivers as soon as its admitted work is verified, then Build after next becomes Next build.
- Unfinished items stay in their assigned planner slot until Jenny reprioritizes them.
- The three-concurrent-hands-on cap applies only to game development: gameplay code, Blender,
  Unreal, and game assets. Planner, backlog, and build-cost canvas-extension work is outside that
  cap and needs no slot.
- **No overnight or workday work** (Jenny, 2026-10-04). Work, automation, packaging, and promotion occur only while Jenny is around.
- Her packaged game is often running. Never touch it, and never start a third Unreal process.

## Registry

Active round: `20261004-afternoon-03`, authorized by Jenny's 2026-10-04 11:59:23 local message
for the 4 PM slot. Scope and launch boundaries are in
[afternoon-build-03.json](afternoon-build-03.json) and
[`accounting/afternoon-03-allocation.json`](accounting/afternoon-03-allocation.json). No overnight
work/automations, no unselected animation/carry, meals, strap, normal-map, mine, or tree work.

### Standing team

| Role | Session | Standing responsibility |
| --- | --- | --- |
| Orchestrator | `146ed534-2f78-48ba-b0fc-98436c1f3223` | Coordinates only. |
| Integration | `64540925-e1b6-4766-ac1f-f2dc42f8aa36` | Integrates, verifies, packages, and promotes releases. |
| Docs | Task-scoped | Maintains canonical shared documentation and handoffs. |
| Balance | `81a547cd-0a72-4415-a617-460b5ade5f3c` | Reviews balance and cohesiveness; no game-code edits or implementer slot. |
| Cleanup | `9fc4e210-68e7-4bc1-acca-d52366894506` | Performs the daily project-storage sweep. |

**Future-launch model clarification (2026-10-04):** launch new sessions and sub-agents with Claude
models only: Opus 5.5/high for Orchestrator, assets, and Balance; Sonnet 5.5/high for gameplay and
general Unreal; Opus 5/high for risky/code review; Sonnet 5/medium for Integration/build/test; and
Haiku 4.5/medium for docs, accounting, and simple status. Existing GPT sessions continue unchanged.

| Name | Session | Worktree (`E:\Repos\copilot-worktrees\SurvivalGame\...`) | Status |
| --- | --- | --- | --- |
| Orchestrator Agent | runtime `146ed534-2f78-48ba-b0fc-98436c1f3223`; app `3c4e743c-3c27-4732-adce-e651536c7e75` | `jennifergalley-expert-fiesta` | coordinates only; afternoon-03; GPT-6.1 Sol/high/default |
| Fishing Art Agent | `ac7339b4-84f7-49c0-8ec2-3d51d2b86730` | `jennifergalley-bookish-pancake` | afternoon-03; hands-on slot 1; Claude Opus 5.5/high/default; fishing asset making/integration only |
| Gameplay UI Agent | runtime `986d7db6-7ae4-4cee-a980-054664106056`; app `3d3426ed-7f12-4245-ab69-dc66e8698418` | `jennifergalley-miniature-invention` | afternoon-03; hands-on slot 2; GPT-6.1 Sol/high/default |
| Environment Agent | `ff61de39-9742-45a8-8c6d-03512cd76224` | `jennifergalley-probable-engine` | afternoon-03; hands-on slot 3; GPT-6.1 Sol/high/default |
| Integration Agent (afternoon-03) | `64540925-e1b6-4766-ac1f-f2dc42f8aa36` | `jennifergalley-integration-agent-8ba` | docs/accounting/preflight only until explicit slot grant; GPT-5.6 Terra/medium/default |
| Balance Agent | `81a547cd-0a72-4415-a617-460b5ade5f3c` | review-only session | Claude Opus 5.5/high/default; reviews energy, coins, yields, timers, pacing, design, flavor, and UI; no game-code edits or implementer slot |
| Orchestrator Agent | `3c4e743c-3c27-4732-adce-e651536c7e75` | `jennifergalley-expert-fiesta` | coordinates only; latest usage confirms GPT-6.1 Sol, high |
| Farming Fishing Agent | `5be207bc-49b1-4a1b-811e-088ae565dc1b` | `jennifergalley-cautious-robot` | measured-02; hands-on slot 1; GPT-6.1 Sol/high/default launch |
| Travel Rest Agent | `1b0e10a4-09b5-458e-b9de-098cce831b07` | `jennifergalley-miniature-fishstick` | measured-02; hands-on slot 2; GPT-6.1 Sol/high/default launch |
| Planner Editing Agent | `ac593377-587f-4319-a033-b17f309be13e` | `jennifergalley-cuddly-eureka` | measured-02; hands-on slot 3; GPT-6.1 Sol/high/default launch |
| HUD Agent | `2424d5bc-bc5a-4eec-a37f-a2f82cb995d0` | pending lane handoff | measured-02; hands-on slot 2; GPT-6.1 Sol/high/default launch; selected 2026-10-04 02:19:32Z |
| Integration Agent (build 02) | `e528fd4a-5aed-4c95-9463-a37941afc00b` | archived `jennifergalley-studious-goggles` | archived at 12:32 Oct 4; archival deleted measured02's Shipping output and package-local save. Do not contact; the 4 PM build is recovery. |
| UI Agent | `8ee8d5f3-2054-46d2-95df-1615bf4939d6` | archived | `440d4de5` reviewed/shipped; closure captured; Jenny accepted checks 2026-10-03; ui-measured-01.md handoff |
| Town Agent | `d1334c5c-5b56-4cc8-90aa-dfe669b2b2e2` | archived | `6e9c3a9a` reviewed/shipped; closure captured; Jenny accepted checks 2026-10-03; town-measured-01.md handoff |
| Accounting Agent | `02a5caf8-3fdf-4c96-be5c-192448d6fcdf` | archived | tooling integrated; closure captured; GPT-6.1 Sol/medium/default; accounting-measured-01.md handoff |
| Integration Agent | `3554b767-ebd7-436c-a691-13795fecad77` | archived `jennifergalley-redesigned-couscous` | archived; its measured01 package is gone. Do not infer a rollback or retarget from it. |
| Disk Cleanup Agent | `9fc4e210-68e7-4bc1-acca-d52366894506` | `jennifergalley-congenial-engine` | **retained**; its own schedule is cleared, and the orchestrator asks it for one sweep a day |
| Old Integration Agent | `e251051b-8674-4ef0-a3ed-03830407f8b6` | `jennifergalley-literate-eureka` | retired, **do not archive**: its worktree holds the 9 PM Shipping release the shortcut targets |
| Old UI Menus, Props Animations, Terrain Weather, Documentation, Architecture | `5cf73757`, `ce241dd6`, `89914e30`, `a9f10974`, `a1648ae7` | various | retired; don't message them. Spawn fresh sessions instead |
| Old orchestrator | `92eac339` | `jennifergalley-cautious-pancake` | retired; hosts the old planner canvas instance |

**Release-loss incident (2026-10-04, 12:32):** archiving a release-holding session deletes its
worktree `Build` output. Archived measured02 `jennifergalley-studious-goggles` lost the active
Shipping release and package-local save; archived measured01 `jennifergalley-redesigned-couscous`
is also gone. Leave the broken shortcut untouched—do not rebuild measured02 or retarget the Oct 1
rollback. `jennifergalley-literate-eureka` is the only remaining Oct 1 release root and is
protected. The 4 PM afternoon-03 Shipping build is the recovery: package normally under the
Integration worktree, then install/hash-verify its complete `Windows` package at
`E:\Repos\SurvivalGame\Build\Windows`, run release isolation plus F5/F9 on that installed copy, and
promote only the main-checkout build. The shortcut must never target a session worktree or an
external release root.

## Spawning lanes

Spawn with `create_session` only when Jenny schedules work, one session per area per build slot,
choosing the model tier from README.md's table. Brief each with all its items for that slot at once.
Archive it once its work is pushed and Integration has packaged it.

| Area | Typical work | Tier |
| --- | --- | --- |
| UI Menus | the book, HUD, map, hints, toasts, theme, controls | GPT-6.1 Sol, high |
| Props Animations | character, tools, animation, clothing, Blender assets | GPT-6.1 Sol, high |
| Terrain Weather | terrain, water, foliage, weather, object placement on the map | GPT-6.1 Sol, high |
| Integration | merge, Development-Run, package, scripted tests, shortcut | routine low/medium tier from README.md; escalate implementation bugs |

**2026-10-02 model and lifecycle update:** the earlier tiers in this round are superseded by
README.md's current model table. Use GPT-6.1 Sol / high for the orchestrator and all Blender, Unreal,
gameplay, visual and performance implementation; use cheaper models or lower effort for routine docs,
integration and test execution. Keep task-scoped handoffs and build-credit attribution per
[agent-lifecycle.md](agent-lifecycle.md). Do not infer a live session's settings from this registry.

**2026-10-04 model clarification:** Claude Opus 5.5 / high / long is the future-launch setting for
the Orchestrator Agent (Jenny requested 1.1M context). Claude Opus 5.5 / high / default is also
allowed for Blender/Unreal asset making and asset integration (long only when necessary there) and
the Balance Agent. It is not used for general gameplay/UI/environment code, reviews other than
Balance, docs, accounting, or packaging. GPT-6.1 Sol / high remains
the future-launch configuration for general gameplay/UI/environment code and risky review; routine
docs, builds, tests and accounting stay on a cheaper low/medium tier. Record the requested 1.1M,
configured `long`, and actual runtime context separately. This policy does not retune a running
session: the current afternoon-03 coordinator remains a GPT-6.1 Sol / high / default segment.

**Coordinator wake checklist (2026-10-04):** every scheduled coordinator wake reads fresh
`priority.json` and `backlog-inbox.json` before assigning the next scheduled build. It assigns only
explicitly newly selected work, preserves existing owners, and leaves Jenny's scheduling
notifications off.

Parked work (don't resume unless scheduled): `origin/park-sprint-chests` (`f0cc4527`, sprint and chest
edits) and Terrain's water-slot-walk.

## Next build: 7:30 AM, 2026-10-02

**Superseded at 21:32 on 2026-10-02:** Jenny selected the next measured build scope in builds.md and
priority.json. Do not package the old list below as the new requested scope. Animation/tool carrying,
rucksack, foliage flicker and hair fixes are deferred without reverting existing main work.

Everything scheduled is on `main` (at or before `caa06e5d`):

- `pitch-dark-parchment-theme` review fixes (`1b73feca`) and `apply-dark-theme-everywhere` (`34f0b126`)
- `place-items-in-exact-slots` (`80ec5928`). **Save change**: an optional trailing `packslots`
  section. This is the one diff worth a cheap code review.
- `fix-tool-grip-contact` (`5ff56338`, `9344029d`)
- `reduce-foliage-shadow-motion` (`63f48e7f`)
- `refine-playable-heroine-hairstyles` (`0be441cc`)

`docs/handoff/builds.md` already lists what ships. Nothing runs overnight, so this build starts as soon as
Jenny turns the computer on and gives you your first message on 2026-10-02. It will land later than 7:30;
tell her the expected time. Then:

0. Rename yourself "Orchestrator Agent", add your session ID to the registry above, and open the planner canvas.

1. Spawn a fresh Integration Agent using README.md's current routine-work model tier.
2. Start a Development-Run with your own session ID as coordinator, then assign the Integration worker:
   ```
   Scripts\Development-Run.ps1 -Action Start -Hours 6 -CompletionPolicy until-complete -CoordinatorSessionId <your id> -StateDirectory <Integration worktree>\Automation
   Scripts\Development-Run.ps1 -Action AssignWorker -WorkerSessionId <integration id> -StateDirectory <same>
   ```
   The previous run (`20261002-030140-17c82249`, in literate-eureka's `Automation`) should be stopped;
   stop it first if not.
3. Integration packages from `main`, runs native, Development smoke/FullLoop and guarded Shipping QA,
   retargets only the "Homestead Estate" shortcut, and copies her current saves. Then it marks the
   builds.md entry delivered.
4. Known blocker: an orphan `VCTIP.EXE` (MSVC telemetry) blocks guarded Shipping QA. If no `cl`, `link`
   or UBT process is running, stopping that exact PID is safe.
5. Tell Jenny what shipped and what to try, and remind her the 4 PM slot is empty unless she has
   scheduled work.
6. After delivery, ask the Disk Cleanup Agent for its daily sweep, and archive the shipped OpenSpec
   changes. When a MODIFIED delta's requirement isn't in the main spec, use `--skip-specs` and add it by
   hand. Delete `openspec/changes/archive/.openspec-archive.lock` after each archive.

## Player-feedback queue

- `fix-map-click-travel` and `move-town-signpost` shipped in measured build 01; Jenny considers their checks complete.
- `fit-rucksack-straps`: the rucksack straps sink inside her shoulders and chest (Props, hard).

## Planner canvas

Open it with `open_canvas` (canvas `openspec-task-planner`). Reordering, scheduling, quoting, and removing
items there is agent-free by design: it writes straight to `docs/handoff/priority.json` (no chat message is
sent). When you next read that file and see its `updated` timestamp has moved, commit and push it to `main`,
brief the lanes for any newly scheduled items, and keep each slot's entry in `docs/handoff/builds.md` in sync.

The canvas also has a quick backlog-entry form (title, optional description, optional screenshot) so Jenny
can drop new work items into durable `docs/handoff/backlog-inbox.json` without spending chat tokens or waiting
on an agent. `docs/handoff/backlog.md` is the Markdown mirror for triage and display; fold each into the list
above (or Later) and remove its mirror entry the next time you touch `backlog.md`.

## Measured build delivery closure

Every delivered build ends with Integration exporting all allocated local usage records and committing the
loader-compatible `docs/handoff/accounting/reports/<build-id>.json`. The planner must then show the delivered
build and observed AIU before closure. Keep exact nano-AIU/deduplication, state that AIU is not
billing-reconciled credits, preserve unknown context/post-capture tails and deferred work, and never turn
Jenny's unchecked playtest acceptance or planner slots/feedback edits into completion.

**Jenny's direct instructions stand** (2026-10-01): she chose to keep scheduling notifications off and gave
that instruction directly to the implementing session. The orchestrator must not reverse a choice Jenny made
directly with a lane, even on its own judgment about round-3 coordination needs, without her explicit
approval first. Jenny may direct any lane herself; lanes follow her direct instruction over a conflicting
orchestrator request.

## Latest coordination handoff (2026-10-02 delivery)

- `20261002-measured-01` is **delivered and accepted by Jenny** as of 2026-10-03. Authorized 21:32
  local; shortcut promoted 23:46:31 after valid gates. Source/package checkpoint `d2155333`;
  closure checkpoint `e657e70d`. Exact release/hash/rollback are in measured-build-01.json and
  integration-measured-01.md. The 23:34:05 candidate receipt is staging, not promotion.
- Ships the five selected items in builds.md: dark book/HUD, map-click travel, exact pack squares,
  crop selling and relocated town sign. Further animation/carrying, rucksack, foliage and hair
  work remains deferred. Do not resume it or schedule new work without Jenny.
- Town review and UI replacement `440d4de5` re-review passed. Native 19, current-source
  Development FullLoop, and guarded Shipping EstateSmoke/ToolRepeat passed. Shipping FullLoop
  is invalid unadapted Woodland coverage, not a gameplay failure. Route FPS is not Estate visual
  or performance acceptance; actual mapped A/Enter regression compiled but was not run.
  Jenny considers all shipped player-check tasks complete; their OpenSpec checkboxes are closed.
- Both Integration worktrees are protected: new `jennifergalley-redesigned-couscous` holds the
  shortcut release; old `jennifergalley-literate-eureka` holds the single 9 PM rollback. All 20
  SaveGames/Config files were copied/hash-verified and the shortcut icon preserved. Jenny's game
  was observed running from the new release; do not touch it or launch more development while she plays.
- UI, Town and Accounting task sessions were archived after clean/no-PR preflight, pushed source
  and handoffs, and stopped-lane usage capture. Use fresh task-scoped sessions for new scheduled work.
  No old agent was archived. Integration has no automation and cannot be archived while its release
  is referenced.
- Final bounded accounting: 454 calls through `69152`, `1797874160000` nano-AIU (1,797.87416 AIU).
  Parent ends at `69151`, Integration at `69152`; all windows are closed so future work cannot
  accrue to build 01. The export observed `69159` and excludes eight pre-/post-cutoff records.
  This is **not a billing-reconciled credit total**: actual runtime context, inherited implementation
  costs and billing reconciliation remain incomplete; later closing activity is intentionally outside scope.
  Task/model/effort breakdown and coverage are on the agent-free planner; detailed disjoint ranges
  are in accounting/measured-01-allocation.json. No per-message refresh/polling.
- Bootstrap recovery is documented in integration-measured-01.md: Jenny approved backing up
  900 actual asset files with verified hashes, restoring only those paths, and clean-recooking
  with `-PackageOnly`. Content was clean afterward. This is not blanket restore/reset permission;
  known bootstrap/Shipping-map failures are already in the editor skill's section 0.1.
- Cleanup's Oct 2 daily sweep is done (+31.56 GB on E:), with game, current release, rollback,
  saves and recovery evidence preserved. Manifest: `E:\CopilotScratch\cleanup-logs\20261002-2350.md`.
  Do not ask for another Oct 2 sweep. No overnight work/automations; wait for Jenny's next input.
- This worktree retains Jenny's untracked backlog-inbox.json and an unrelated uncommitted
  .github/copilot-instructions.md edit. Preserve both; scheduling/feedback stay agent-free and
  Jenny's direct lane instructions override conflicting coordinator requests.
