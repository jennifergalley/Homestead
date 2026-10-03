# Round 3: scheduled builds with a lean team

Started 2026-10-01, 22:30. Jenny asked for a token-efficient team: a fresh orchestrator on GPT-6 Sol,
fresh implementers spawned only for scheduled work, and the rules in "Token budget" in
[README.md](README.md). Read README.md first, then this page. Older round pages are history only.

## Jenny's standing rules

- Lanes implement **only** what she schedules for a build slot in the planner canvas, then go idle.
  No autonomous pickup.
- She does all playtesting and verification. Lanes run native tests and a compile; they open the
  editor only when the work can't be checked otherwise (animation and art).
- Build slots: **7:30 AM, 4 PM, 9 PM**. Each build starts about an hour before its slot.
- Unfinished items roll to the next release; tell her when that happens.
- When a slot has nothing scheduled, tell her to schedule work.
- **No overnight work** (Jenny, 2026-10-01). She turns the computer off at night. No session schedules
  automations or runs work between her evening sign-off and her first message in the morning. Work
  starts when she turns the computer on and messages the orchestrator.
- Her packaged game is often running. Never touch it, and never start a third Unreal process.

## Registry

| Name | Session | Worktree (`E:\Repos\copilot-worktrees\SurvivalGame\...`) | Status |
| --- | --- | --- | --- |
| Orchestrator Agent | `3c4e743c-3c27-4732-adce-e651536c7e75` | `jennifergalley-expert-fiesta` | coordinates only; latest usage confirms GPT-6.1 Sol, high |
| UI Agent | `8ee8d5f3-2054-46d2-95df-1615bf4939d6` | `jennifergalley-turbo-carnival` | replacement `440d4de5` ready for re-review; parked, slot released; ui-measured-01.md handoff |
| Town Agent | `d1334c5c-5b56-4cc8-90aa-dfe669b2b2e2` | `jennifergalley-vigilant-broccoli` | `6e9c3a9a` reviewed/integrated at `9d7c117e`; parked, slot released; player checks pending; town-measured-01.md handoff |
| Accounting Agent | `02a5caf8-3fdf-4c96-be5c-192448d6fcdf` | `jennifergalley-laughing-lamp` | integrated on main at `08878e51`; parked, slot released; GPT-6.1 Sol/medium/default; accounting-measured-01.md handoff |
| Integration Agent | `3554b767-ebd7-436c-a691-13795fecad77` | `jennifergalley-redesigned-couscous` | measured build 01: merge accounting first, selected gameplay after ready/review; GPT-5.6 Terra/medium/default; integration-measured-01.md handoff |
| Disk Cleanup Agent | `9fc4e210-68e7-4bc1-acca-d52366894506` | `jennifergalley-congenial-engine` | **retained**; its own schedule is cleared, and the orchestrator asks it for one sweep a day |
| Old Integration Agent | `e251051b-8674-4ef0-a3ed-03830407f8b6` | `jennifergalley-literate-eureka` | retired, **do not archive**: its worktree holds the 9 PM Shipping release the shortcut targets |
| Old UI Menus, Props Animations, Terrain Weather, Documentation, Architecture | `5cf73757`, `ce241dd6`, `89914e30`, `a9f10974`, `a1648ae7` | various | retired; don't message them. Spawn fresh sessions instead |
| Old orchestrator | `92eac339` | `jennifergalley-cautious-pancake` | retired; hosts the old planner canvas instance |

**Do not archive any old agents**: old Orchestrator, UI Menus, Props Animations, Terrain Weather,
Documentation, Architecture, Integration, or Cleanup. Jenny will archive them herself once the new
system works. "Archive when done" applies only to sessions this orchestrator spawns. In particular,
the old Integration worktree holds the shortcut's current Shipping release; Cleanup checks that target
and its one rollback before deleting releases.

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

## Unscheduled bugs (in the planner, waiting for a slot)

- `fix-map-click-travel`: clicking a map destination no longer travels; only T works (UI Menus).
- `move-town-signpost`: the town signpost belongs beside the road past the derelict farm, pointing to
  town, not outside the manor (Terrain Weather, placement).
- `fit-rucksack-straps`: the rucksack straps sink inside her shoulders and chest (Props, hard).

## Planner canvas

Open it with `open_canvas` (canvas `openspec-task-planner`). Reordering, scheduling, quoting, and removing
items there is agent-free by design: it writes straight to `docs/handoff/priority.json` (no chat message is
sent). When you next read that file and see its `updated` timestamp has moved, commit and push it to `main`,
brief the lanes for any newly scheduled items, and keep each slot's entry in `docs/handoff/builds.md` in sync.

The canvas also has a quick backlog-entry form (title, optional description, optional screenshot) so Jenny
can drop new work items straight into `docs/handoff/backlog.md` without spending chat tokens or waiting on an
agent. Submissions land in a `## New from Jenny (not yet triaged)` block at the top of the file (screenshots
under `docs/handoff/attachments/backlog/`); fold each into the list above (or Later) and delete its line
the next time you touch `backlog.md`.

**Jenny's direct instructions stand** (2026-10-01): she chose to keep scheduling notifications off and gave
that instruction directly to the implementing session. The orchestrator must not reverse a choice Jenny made
directly with a lane, even on its own judgment about round-3 coordination needs, without her explicit
approval first. Jenny may direct any lane herself; lanes follow her direct instruction over a conflicting
orchestrator request.

## Latest coordination handoff (2026-10-02, 22:07)

- Active build: `20261002-measured-01`, authorized at 21:32 local. Scope and deferrals are committed
  in builds.md, backlog.md and priority.json; kickoff SHA `6390d37f`. No delivery ETA yet and no
  overnight work/automations. User-approved right-sized delegation is now in agent-lifecycle.md.
- UI, Town and Accounting are parked; Integration counts while merging/building.
  UI owns theme/map/inventory; Town owns shop/trade/signpost files; Integration
  owns merging/testing/packaging and the next accounting captures.
  Lanes push feature branches and report exact ready SHAs, not unverified changes to main.
- UI's original ready SHA is now blocked by independent review: legacy crafted-away pack-slot
  saves reject before reconciliation (high), and keyboard/controller cannot drop onto EmptySlot
  (medium). The same High owner delivered replacement `440d4de5` with both fixes, native 2/2
  and one editor compile passed, unchanged save format/version and retained corrupt-save rejection.
  Actual mapped A/Enter regression was authored/compiled but not run. Independent re-review is
  required before UI promotion or packaging; seven Jenny checks remain pending.
- Town ready SHA is in the registry and manifest: crop/sign native suites and editor compile passed.
  Production shop source path is checked, not in-game clicks. Crop-selling/sign visual checks remain
  Jenny's; no save/bake change. Town passed independent review and is integrated at `9d7c117e`.
- Fresh Integration runs on Terra/medium. Accounting tooling is integrated; the parent safely
  fast-forwarded and reopened the planner. Independent reviewer
  `be475939-47b6-4de1-85a4-220ba579d0b4` ran within Integration's runtime on observed
  GPT-6.1 Sol/high, actual context unknown. Integration holds packaging and gives UI the next UBT turn.
- Accounting collector/report/planner is **integrated**. The report at `09d542ec` captures 221 calls
  through `68903` and `741350550000` nano-AIU; this is not a final build invoice. Capture later
  responses and Integration/review calls before eligible archival, at meaningful build gates rather
  than after every forwarded usage note.
  Contributor identities/configuration and the coordinator's pre-kickoff usage cursor are recorded
  in measured-build-01.json. Include helpers, startup/retries, review, integration and overhead.
- UI verification allocation is `(68789,68824]`; delivery overhead is after `68824`, without
  overlap. Accounting reports UI snapshot through `68851`: 45 calls, `157423420000` nano-AIU,
  not final. Event `68772` has unknown effort; actual runtime context remains unknown.
- Town task splits use UTC evidence in town-measured-01.md, not sampled cursors. Its receipt
  through `68896` is 59 calls and `182170580000` nano-AIU; later handoff/messages are delivery overhead.
- UI review rework is `[68941,68965]`, validation `(68965,68974]`, then delivery after `68974`.
  Initial delivery ends `68859`; do not overlap these with the old open-ended delivery segment.
  Capture the final reply tail at the next integration gate, not after every message.
- Local session usage records expose per-call model, reasoning effort, token details and nano-AI
  units. Capture actual context configuration separately. GitHub's AI export aggregates by user/model/day,
  so it can reconcile totals but cannot by itself supply task attribution. See the accounting contract.
- Fresh worktrees initially appeared incomplete immediately after create_session; all three agents'
  startup rechecks then confirmed clean indexes and required project files at kickoff SHA. STOP was
  lifted, and no reset was needed. Jenny authorized repair only for still-incomplete fresh checkouts
  with no real edits; do not treat this as blanket reset permission.
- All old agents remain stopped/protected. Preserve the old
  Integration worktree and Jenny's current shortcut/save. The next build is not delivered.
- This worktree still contains Jenny's untracked backlog-inbox.json; its test entry is preserved
  in backlog.md and its removal flag in priority.json. Do not discard it during pulls or cleanup.
