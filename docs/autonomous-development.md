# Autonomous development

One coding worker, one coordinator, bounded runs, and evidence from the real game.
This is an app-session workflow, not an unattended executable that can operate
after the Copilot app is closed or the machine is unavailable.

## Run contract

The default run is eight hours with a coordinator check-in every five minutes.
`Automation\config.json` records scope and priorities; `Automation\run.json` is
the local control record. The coding worker checks `allowWork` using
`Scripts\Development-Run.ps1 -Action Status` before each new phase, build or
delegation. The fixed deadline is never extended by resume or a scheduled tick.
Finish an in-flight safe operation when stopping; do not start another iteration.
The deadline is a cooperative stop, not a destructive mid-save process kill.

The coordinator owns run state and scheduling. The worker owns game changes,
candidate builds, and `Scripts\Update-DevelopmentStatus.ps1` reports. Use one
coding writer and one Unreal build/import/test process at a time, even when Git
worktrees exist. A branch-backed session shares the folder: do not mistake it for
an isolated checkout. Coordinator changes to tracked files must finish before
the worker starts. No automatic merge, force push or baseline package promotion.

## One iteration

1. Read the approved plan and feedback. Choose one bounded, reversible improvement.
2. Observe the actual problem in normal play; use the existing evidence when still
   applicable. State a concrete improvement target and failure/regression risks.
3. Implement it, using at most one bounded read-only specialist if genuinely useful.
   Do not create extra writers/editors for the same task.
4. Run the smallest relevant native/build checks, then actual-game tests and an
   ordinary-movement visual capture. Inspect the evidence, not just exit codes.
   Functional teleport-assisted smoke checks and sampled-motion footage prove
   different things; neither certifies physical controller comfort.
5. Fix concrete regressions. Limit subjective cosmetic work to two review passes
   per slice rather than endless resculpting. Preserve useful partial work honestly.
6. Package separately, exercise that executable, and commit a verified checkpoint.
   Report paths, evidence, shortcomings, and suggested next slice to the coordinator.
   Wait for the next bounded assignment; do not silently expand into deferred work.

The first priorities are current-pipeline movement/posture, one appealing heroine,
then gathering motion. MetaHuman, other recorded UI feedback, and larger roadmap
features remain deferred unless Jenny or a later explicit task changes the scope.
Liking the heroine remains Jenny's subjective gate; do not call it approved for her.

## Visibility and control

`docs\development-status.md` is a generated, local latest-status report. Its
timestamp and evidence distinguish fresh results from a long operation. It is
opened in an editor panel; reload if that panel has not refreshed the file.
`Automation\status.json` is the machine-readable equivalent.

The coordinator uses the app's live session status, not absence of commits, to
detect whether the worker is busy or awaiting input. Roughly every five minutes,
post a short chat update: current work, last actual result, and a real blocker or
uncertainty. A long tool call or closed app can delay a check-in. Do not invent
progress, repeatedly poll an unchanged blocker, or send the worker duplicate jobs.

Ask in chat to **pause**, **resume**, **stop**, or **run for N hours**. The coordinator
updates state and messages the worker immediately. Local controls also exist:

```powershell
.\Scripts\Development-Run.ps1 -Action Status
.\Scripts\Development-Run.ps1 -Action Pause -Reason 'Jenny requested a pause.'
.\Scripts\Development-Run.ps1 -Action Resume
.\Scripts\Development-Run.ps1 -Action Stop -Reason 'Stopping with a handoff.'
```

These file controls do not forcibly interrupt an in-flight tool call. Pause keeps
the original deadline; after stop or expiry, a new run needs new authorization.
The coordinator clears its session automation when stopped, records a concise
handoff and project-journal entry, and releases its temporary awake process.
`Hold-DevelopmentAwake.ps1` independently releases on stop/deadline within 15 seconds,
and while paused. It holds only system sleep, not the display, and changes no
persistent power settings. Closing its process also releases the OS thread request.

## Candidate builds

Keep `Play.cmd` and `Build\Windows` as the known-good player build. The worker uses
a fresh candidate directory and fresh evidence directories:

```powershell
$candidate = 'Build\Releases\<run-id>\movement-01'
.\Scripts\Build-Game.ps1 -Package -ArchiveDirectory $candidate
.\Scripts\Test-Game.ps1 -Packaged -FullLoop -PackageDirectory $candidate `
    -OutputDirectory 'Saved\Automation\<run-id>\movement-01'
.\Scripts\Playtest-Visual.ps1 -Packaged -PackageDirectory $candidate `
    -OutputDirectory 'Saved\VisualPlaytests\<run-id>\movement-01'
```

An older package cannot verify newer source. Keep licenses/provenance with assets,
private portrait and personal saves out of Git, and recordings local. No purchases,
credential work, account changes, or bypassing login/license/elevation prompts.
Free assets/tools and reversible art decisions are permitted. Stop on human-only
blockers rather than treating spare credits as a requirement to consume them.

Unreal may append a `Windows` platform folder to an archive directory. The build
receipt records the actual package directory. Both test scripts accept the archive
or exact platform directory and reject ambiguous old/new executable layouts.
Run `Tests\DevelopmentRunTests.ps1` to check lifecycle and package-path controls.

### Explicit human review without promotion

`Preview.cmd` / `Scripts\Start-Preview.ps1` is separate from `Play.cmd`. The tracked
`Preview.json` names one repo-relative reviewed candidate and persistent profile,
bound to an executable SHA-256. Selection is deliberate, never newest-folder
discovery. The resolver checks archive/platform ambiguity, rejection markers in
both platform and ancestors, reparse points, a passed schema-2 acceptance receipt,
preview-routing version, checkpoint, executable and proof-index hashes. Earlier
packages without the new save-routing capability must not be selected.

Human preview passes only `-HomesteadPreviewProfile=<limited-id>`; never reuse
smoke/visual flags to achieve save isolation. It keeps physical input, sound and
ordinary play, without synthetic actors or automatic exit. Saves persist under
the fixed user-settings `SurvivalGame\PreviewProfiles\profile-<id>\SaveGames`
namespace, including manual/auto/recovery/backup slots. Invalid IDs fail before
save IO. No copying, reset or migration of personal worlds is automatic.
`Play.cmd`, the original package and all prior candidates remain untouched.
See `setup.md` for exact profile rules and launch commands.

Preview acceptance needs focused launcher and actual save-backend isolation
checks, including a separate-process reload. Use only synthetic fixtures: inspect
the normal default path without opening it; create fresh uniquely named test profiles,
copy their proof and remove only their owned files/directories. A short
preview-only validation process has no automation flags and is stopped only by
its owned PID; do not detach it or affect Jenny's other games. Retain the existing
914 action/full-loop checks and the new candidate's native-4K/render100 evidence.
Sampled action images and concurrent-GPU timings are not clean-performance,
precise-contact, controller-comfort or aesthetic approval.

This entry point exposes accepted locomotion and contextual actions only.
Rejected face/hair trials stay rejected; requested mid-back waves, blonde bob,
book/UI changes and broader roadmap work are not silently marked complete.

The coordinator subsequently scheduled only controller prompt stability as
`prompts-01`. Its isolated accepted-event fixture and hint-only classifier do not
authorize the remaining UI/recipe/inventory/tearing work. The later explicit
`book-clarity-01` assignment schedules only carried-possession versus recipe/plan
labels, quantities and action copy, not learning/unlocks or navigation redesign.
Accepted `prompts-01` was separately selected for the persistent `jenny-review`
profile. Preserve `Preview.json`
on the reviewed candidate until the coordinator explicitly reviews and selects
a replacement; a newer source checkpoint is not automatic launcher promotion.

The coordinator subsequently selected accepted `book-clarity-01` in `7051fc3`
and scheduled `presentation-diagnostics-01` as investigation only. It is not the
rejected face experiment and must not be selected as a player improvement.
Keep the accepted book preview while recording game-only evidence. Separate
screenshot-free timing from readback-disturbed capture, distinguish output pixels
from internal render scale, and never call an offscreen framebuffer a scanout
measurement. The current finding is investigated/unresolved; no renderer/OS
settings changes were authorized by that diagnostic task. See `presentation-diagnostics.md`.

The subsequent explicit `video-sync-01` assignment permits one reversible Settings
toggle using the canonical Unreal VSync preference, not an automatic fix or a new
graphics default. Keep the accepted book preview selected pending coordinator
review. Validate requested/applied state and persistence in synthetic graphics
config roots; preview save profiles do not independently isolate graphics settings.
No OS/driver/display comparison or additional renderer option is authorized.

Accepted `video-sync-01` was explicitly selected in `d7d0a02`, preserving
`jenny-review` and the existing Off preference. The next bounded assignment is
diagnostic-only `endurance-01`: one45-minute ordinary-control exercise after short
sanity/cancellation checks. Its prepared test-world provenance, fixed pass criteria,
atomic progress, graceful cancellation and timing limits are in
`endurance-playtesting.md`. Do not promote this test package or start another task
after its handoff. The selected human preview remains video-sync-01.

## Starting a fresh run (coordinator)

Confirm no old worker/build is active; explicitly stop and hand off an old run
before replacing it. Create the run with `Development-Run.ps1 -Action Start -Hours N
-CoordinatorSessionId <id>`, open a single app-native worker session, and assign
its ID with `-Action AssignWorker -WorkerSessionId <id>`. Persist a five-minute
session automation here, launch the bounded awake helper, and verify all three.
The scheduler prompt must read this contract and run state, inspect the actual
worker, enforce stop conditions, send meaningful updates, and dispatch only one
bounded task at a time. Reuse that worker for sequential slices.

## Setup evidence, 2026-09-20

Lifecycle and package-path tests pass 21 checks. A separate Unreal package was
built at `Build\Releases\automation-setup\Windows`; its ordinary-control route
recorded 302 correctly sized frames and reached/gathered a plant. The original
`Build\Windows` package was not rebuilt or replaced.

Do not describe the new candidate as functionally clean. Its extended smoke run
failed when node 22 gathered stones instead of expected branches; a separate
basic run failed changing appearance color row 1. A comparison against the
untouched original package also failed, later at berry maturation after six
rests. These runs used 1920x1080, unlike the earlier successful 4K acceptance run.
The differing failures need diagnosis; neither a game regression nor test
nondeterminism has yet been proved. Do not weaken assertions or repeatedly rerun
until a lucky pass. The first worker must investigate this evidence alongside
the movement slice so autonomous quality gates become reliable.

Reports remain local under `Saved\Automation\automation-setup`,
`automation-setup-basic`, and `automation-original-comparison`.
The actual-motion recording is `Saved\VisualPlaytests\automation-setup`.

The first movement worker reproduced a menu failure with a diagnostic trace:
a non-simulated physical A-button press arrived during the scripted outfit
change, advancing the choice twice. Physical stick events also entered the
offscreen game. Jenny confirmed she was using that controller in another game.
Automation now rejects physical-source events only in development-only
smoke/visual/routing-test modes; a preview profile alone leaves ordinary player
input enabled. Tests must coexist with her
other applications, not ask her to stop playing. Performance captured while
another game is active is concurrent-load evidence, not clean performance
acceptance. Berry maturity must be established separately rather than assumed
to share this cause.
