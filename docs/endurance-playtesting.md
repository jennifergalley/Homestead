# Continuous endurance diagnostic

This is opt-in stability evidence, not a new gameplay feature or approval of rendering,
physical presentation, character art, indefinite memory stability, or controller comfort.
The historical `endurance-01` below is diagnostic-only. `Preview.json` identifies
the current human selection; diagnostic routes never change it.

## Current fresh-world Shipping route

`Test-Endurance.ps1 -FreshWorld -ShippingQA` reuses the existing observer with a
fresh disposable world, no copied save, no teleports and no time/lighting edits.
Pass an explicit Shipping package and fresh output directory. `-Seconds 180`
checks mapped gathering, current native-menu eating and exact save/load.
`2700` retains the45-minute observation; `4200` provides70minutes to observe
actual24-hour branch renewal at the unchanged60-minute day length.

The70-minute route additionally requires at least24 natural game hours and
one resource it actually harvested to regain its original registered, visible
produce components while retaining its base. Harvest removal is checked too.
There is no claim that a70-minute run tests168-hour sapling renewal. A separate
controlled clearing fixture covers canopy removal and checkpoint restoration.

Fresh Shipping requires live `until-complete` authority. It uses the existing
root-only, detached `Invoke-ShippingQA.ps1` route and its existing zero-timer
`RenderCompletionDriven` lifetime profile; no external duration-based kill is
added. Live stop/pause, identity changes, endpoint/helper violations and owned-job
cancellation still apply. The finite observer duration is a gameplay observation
criterion, not a wall-clock supervisor cutoff. Ordinary short QA retains100/110s.

Sparse original HUD-inclusive frames record actual game hour, night/rain and Lit
presentation every600seconds plus a final pre-exit milestone. Renderer, save,
action and participation checks remain; unavailable Shipping debug-binding queries
are explicitly distinguished from the historical Development binding evidence.
An absent Shipping engine log is recorded as unavailable, not treated as a clean
engine log; the native process stream, guard outcome and explicit gameplay evidence
are checked. Results for the new route belong to their own output/receipts.

### Fresh-world natural-play result, September21

`readability-endurance-long-01` passed4200.263seconds, with4191.968 unpaused,
3346.975 moving,27.94646 natural game hours,8 gathers,4 meals,628 waypoints,
15 manual saves, one exact paused reload and17 distinct rotating autosave writes.
Eight current envelopes/backups passed independent CRC checks. Navigation and
recovery failures:0. Four previously harvested branch/flower nodes actually
renewed their visible component inventories; branches8/15 were harvested again.
No sleep, clock edit, teleport or synthetic starting fixture was used.

Eight original1920x1080 milestones cover natural dawn, daylight, night and the
next morning's Rain state. Daylight controls/central view are readable. Night
has a readable HUD and distinguishable silhouettes but very dark heroine/hair
and near ground. Pale bare outer terrain and primitive resources remain visible
art gaps. These are observations, not finished lighting or human-play approval.

Post-warmup actor intervals averaged16.668ms, p95=16.9ms, p99=17.0ms,
maximum41.852ms under a60 cap. Sampled physical memory ranged340344832 to
1050480640bytes; virtual1936986112 to2043858944bytes, with98 resources and no
constructed structures/plots. The working-set decline has no established cause;
it is not leak-freedom evidence. Screenshot exclusion and observer/concurrent-load
qualifications above apply; GPU/Present/internal temporal resolution remain unmeasured.

Actual guard exit0, active0/total1, observed death before unchanged marker release,
no hard termination or cleanup errors. Largest finite sampling gap4136.622ms;
no continuous-history claim. Sealed receipt:
`research\environment-assets\readability-endurance-01\receipt.json`
(`31B7CB81F9FC07A6F93B41D8E392342AC24783FAB134F0A8E621808093FCDF56`).

## Historical fixture setup and criteria

One 180-second harness sanity, a short graceful-cancellation probe, then one 2700-second
(45-minute) run. No automatic long-run retry. The timer starts after loading and settling.
1920x1080 offscreen D3D12; game-only sparse frames, no audio/desktop/input capture.
Graphics preferences are copied from project defaults to the output's explicit
`Graphics\GameUserSettings.ini`, with a synthetic `EngineUser` directory.
No production/selected-preview graphics or personal-save writes are permitted.

The disclosed starting fixture is
`Saved\Automation\20260920-050723-5cc6c8a5\video-sync-01-full-loop\SmokeSave\Homestead_Manual.sav`.
The existing fixture helper copies/hash-verifies it into a fresh output `SmokeSave`.
`fixture.json` records the exact hash and provenance. This is a prepared test homestead,
not a fresh start: prior smoke setup used teleports and ordinary sleep. It has a cabin,
fire, chest and two planted plots, starts at hour64.03679574 (day3 about16:02), with a
60-minute day, hunger68.52, energy99.87 and warmth100.
Frozen fixture SHA256:
`D07D8406AC875A9212E9C7F37E502DFC6E86EE501397A18C54BB3D3EAA5214C3`.

After that setup, the observer sends only normal mapped stick/button inputs: a small
clearing circuit, available eastern forage, actual carried food when needed, and
normal book/manual-save/load controls. It never grants supplies, refills needs,
teleports along the route, edits clock/resources, changes rules or directly ticks
the simulation. No sleep is planned. Natural survival failure ends the run without
recovery (cap0). Up to3 documented navigation misses may be skipped; the fourth fails.

Fixed long-run pass requirements:

- At least2700wall seconds after settling, at least90% of target duration unpaused,
  and at least30% moving. Setup is excluded.
- At least15 natural game hours, excluding measured immediate action costs.
  No load rewind is allowed: the one mapped F5/F9 roundtrip must restore an exact
  paused same-world serialized state. Later manual saves do not reload.
- At least2 successful mapped gathers,1 actual food consumption and40 waypoints.
- At least8 distinct successful rotating autosave writes across3 CRC/schema-valid
  same-world slots. Count changed per-slot saved UTC stamps, never repeated reads.
  The actual cadence is240unpaused engine seconds: the90% target supports10 writes.
  Thirty-second inspection cannot conflate normal240-second writes to the same slot.
- No crash/unhandled error, corrupt/cross-world save, hung input or stuck action/prop.
  A presented action lasting more than5seconds or incorrect tool arbitration fails.

Sanity uses the same participation, integrity and roundtrip rules, but180seconds,
at least1gather/1eat/4waypoints; it has no long autosave/natural-hour minimum.
Cancellation, pause, stop or deadline is always **cancelled/incomplete**, never a
full pass. Failure reasons and unsuccessful attempts are retained, not overwritten.
Criteria are not reduced after failures.

## Invocation and evidence

Use `Scripts\Test-Endurance.ps1` with explicit `-PackageDirectory`, `-FixtureSave`,
fresh `-OutputDirectory`, and `-Seconds 180` or `2700`. `-CancelProbe` is only for180.
The driver requires both `-HomesteadVisualPlaytest -HomesteadEndurance`; normal
preview receives neither and retains unrestricted human input and normal lifetime.
It rejects overlapping test modes and a non-isolated graphics destination.

The wrapper checks active run permission and remaining duration plus180seconds.
The actor checks the same run identity/state/deadline once per second and freezes
the original deadline so a later extension cannot extend this exercise. An output
`stop-endurance.txt` marker requests graceful cancellation. Wrapper timeout is
duration+90seconds, followed by15seconds to exit gracefully; only its owned PID may
then be stopped. The process stays attached to the tool/session.

`launch.json` records PID, command, working directory and executable hash.
Atomic `progress.json`, `endurance-events.txt` and `endurance-samples.csv` update
every30seconds. Samples include wall/engine/game time, needs, distance, process
physical/virtual memory, live UObject slots, resource/structure/plot counts and
successful actions/saves. Existing save-envelope validation checks current slots
and backups. At most five milestone frames are requested during45minutes.
No continuous framebuffer readback is used.

Actor wall-tick timing is sampled after60seconds, excluding2seconds after each
screenshot request. Its bounded histogram has0.1ms bins and a1000ms overflow bin;
exact maximum is also recorded. This is not GPU time, Present/scanout timing,
physical tearing evidence or a clean-FPS certification. Controller simulation
delta and wall duration are separately accumulated; menu time is reported.
Telemetry/save-validation creates transient observer objects and performs disk IO;
memory/object trends include that overhead. Concurrent machine load is uncontrolled.

## Outcome

The single45-minute run passed the frozen criteria; no long-run retry occurred.
Output: `Saved\VisualPlaytests\20260920-050723-5cc6c8a5\endurance-01`.
The attached owned PID35364 exited normally. The final evidence includes the
analysis JSON, complete event/sample logs, five1920x1080 frames and a milestone sheet.
The actual frames were inspected: the HUD progresses through afternoon, evening,
midnight, predawn and morning; needs and unattended garden growth/weeds change.
This is not a smooth video or aesthetic/presentation acceptance.

| Measure | Actual result |
| --- | --- |
| Wall time after fixture load/settle | 2700.287seconds |
| Unpaused / paused wall time | 2690.112 /10.170seconds (99.62% unpaused) |
| Unpaused engine delta | 2690.004seconds |
| Moving time / distance | 2146.080seconds (79.48%) /236075cm |
| Natural game progression | 17.93336hours; day3~16:02 to day4~09:58 |
| Immediate action costs / rewind | 0hours /0; engine-delta conversion independently agrees |
| Successful actions | 5gathers,4food consumptions,402waypoints |
| Manual saves / exact roundtrip | 10 /1 paused same-world F5/F9, in addition to initial fixture load |
| Autosave writes / rotating slots | 11distinct timestamp transitions /3 |
| Final valid envelopes and backups | 8, independently CRC-checked and SHA256-indexed |
| Navigation failures / recoveries | 0 /0 |
| Observed resource availability renewals | 0 |

The 180-second sanity also passed:177.678unpaused seconds,3gathers/1eat/19waypoints,
one exact roundtrip,0navigation misses. The separate marker probe returned
`cancelled`, not `passed`, and the analysis helper rejected it as a full pass.
The fresh diagnostic binary retained308save-routing/input checks, including an
actual no-automation human-preview startup;40launcher guards passed.
The older914gameplay,70prompt and106clarity suites were not rerun or newly claimed
for this observer-only diagnostic. All124character/source-art files and selected
preview/original hashes matched; five owned bootstrap world resaves were restored.

After the60-second warmup, sampled physical working set ranged434470912-1152995328
bytes; first1109962752, last435126272. There was a sharp working-set reduction near
2010seconds (996716544 to507342848), while committed virtual usage stayed near
1.95GB. Its cause was not established; do not attribute it to game cleanup or infer
leak freedom. Virtual usage ranged1950519296-2085879808bytes, net-58970112.
Live UObject slots ranged36343-36411, net-17. World counts stayed98resources,
9structures and2plots. Observer transient objects, garbage collection and OS
residency policy affect these values; concurrent machine activity was uncontrolled.

The157935included wall-tick intervals averaged16.667ms, histogram p95=16.9ms,
p99=17.0ms, exact maximum28.432ms. These are instrumented actor intervals under a
60fps cap, not GPU execution or scanout/Present timings and not clean-performance
certification. Start/end renderer settings agreed: offscreen D3D12/TSR,
1920x1080 windowed, VSyncOff, cap60, dynamic resolutionOff, automatic primary scale.

No crash, unhandled exception, invalid save, navigation failure or stuck action/prop
was detected. Seven startup warnings remain: six pre-mesh `hand_r` socket queries
and one render-thread-safety warning for `r.MotionVectorSimulation`. The same
warnings occur in the prior accepted video-sync full-loop log; they were not
introduced or fixed by this test.

Resource renewal was not reached: new branch/flower depletion lasts24game hours
and berries36, beyond this17.93-hour observation. No time manipulation was used to
force coverage. Last sampled hunger81.05, energy57.30 and warmth46.33 remained
viable; minimum sampled warmth35.09. No sleep, fuel tending, construction, failure
recovery or additional UI behavior was exercised. Longer/other-route behavior,
physical input comfort, leaks over indefinite play and horizontal tearing remain
outside this evidence. No game rules, production graphics or art were changed.
The diagnostic package is not eligible for preview selection; accepted video-sync
and the persistent `jenny-review` profile remain selected.

**Appended qualification,2026-09-20:** the first F5 also selected inherited
`ShaderComplexity`, logged about35seconds after milestone00. Milestones01-04
and all postwarmup timing/memory samples follow that transition. The retained
measurements describe this actual process, not normal-Lit gameplay performance;
wall time, simulation/actions/save integrity and bounded stability remain
observed. Original sealed proofs are unchanged. See `debug-hotkey-evidence.md`.

**Subsequent confirmation:** after accepting and explicitly selecting
`hotkey-safety-01` in `af0c907`, the coordinator authorized one normal-Lit run
using this exact fixture/criteria. See `endurance-lit-playtesting.md`. It is new
separate evidence, not an overwrite or reinterpretation of these sealed results.
