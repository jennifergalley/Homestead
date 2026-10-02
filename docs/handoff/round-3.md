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
| Orchestrator Agent | `3c4e743c-3c27-4732-adce-e651536c7e75` | `jennifergalley-expert-fiesta` | coordinates only (GPT-6 Sol, medium) |
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
| UI Menus | the book, HUD, map, hints, toasts, theme, controls | small (Sonnet) unless it touches saves |
| Props Animations | character, tools, animation, clothing, Blender assets | hard (Opus) |
| Terrain Weather | terrain, water, foliage, weather, object placement on the map | small for placement, hard for rendering |
| Integration | merge, Development-Run, package, scripted tests, shortcut | GPT-5.4 mini |

Parked work (don't resume unless scheduled): `origin/park-sprint-chests` (`f0cc4527`, sprint and chest
edits) and Terrain's water-slot-walk.

## Next build: 7:30 AM, 2026-10-02

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

1. Spawn a fresh Integration Agent (GPT-5.4 mini).
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

Open it with `open_canvas` (canvas `openspec-task-planner`). Each time Jenny reorders or schedules work there,
you get a notification: commit and push `docs/handoff/priority.json` to `main`, brief the lanes for any
newly scheduled items, and keep each slot's entry in `docs/handoff/builds.md` in sync.
