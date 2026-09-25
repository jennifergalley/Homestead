# Wild forage renewal through normal rest and save/load

**Historical fixture:** this documented manual save contains `HOMESTEAD 3`;
the current game requires save version 7. Do not pass it to a new Shipping build
or imply this old 24-hour rest route passed against current code. The
`HomesteadForageRenewal` source assertions now expect quiet, nontechnical
player feedback while retaining renewal authority. The current-version
`-Creek` route exercises ordinary mapped Reed gathering, the exact 24-hour
deadline, anti-duplicate attempt, stubble, F5/F9 and 720p/4K in-game frames.
Natural three-rest restoration still needs a newly prepared version-7 fixture
before it can be rerun.

Targeted diagnostic-only coverage following endurance-01, not another long soak,
gameplay rebalance, resource grant or preview promotion. The selected human build
stays accepted video-sync-01 with the persistent jenny-review profile.

## Frozen fixture and route

Source: final manual save from
`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\endurance-01\SmokeSave`.
SHA256: `0B3300D2A92A160EDE8E178251F6FAFDFB5ADE5855B63C5250260E2467EEC0CA`.
This is a disclosed prepared test homestead, not a fresh player start. Its original
smoke setup used teleports/rest; the subsequent45-minute endurance used ordinary
mapped controls without time/needs/inventory edits. Each new renewal fixture is an
exact hash-verified copy. No personal or selected-preview saves/configs are used.

It starts at hour81.97015855 (day4 about09:58), hunger80.66, energy56.83,
warmth46.93, with15berries and working knife/hatchet.

| Actual ID | Resource / state | Existing readiness hour | Normal renewal |
| --- | --- | --- | --- |
| 8 | Depleted branches at(-600,-400) | 88.11285109 | 24game hours |
| 12 | Depleted flowers at(-500,-300) | 88.49711500 | 24game hours |
| 10 | Depleted berries at(-400,-400) | 100.99135901 | 36game hours |
| 14 | Permanently cleared sapling at(-700,-600) | 0, cleared | Never renews |

The bounded observer reuses existing mapped input, walking/camera navigation and
save validation. It first approaches each depleted ID, tries a real gather and
asserts no reward, deadline change or gather animation. An exact paused mapped
F5/F9 roundtrip preserves the entire simulation, world identity and cooldowns.
It then walks around the cabin to the actual west doorway and sheltered bed105.
There are three ordinary mapped8-hour rests:24sleep hours, the minimum needed
from this fixture to cross the chosen berry deadline. Food is consumed normally
if hunger calls for it; no replenishment or direct simulation advancement occurs.

After each rest, actual resource scene components must update automatically:
branches/flowers become ready after rest1, berries stay depleted through rest2,
then become ready after rest3. No test calls the world refresh function.
The route walks back to the same IDs, checks actual focused HUD labels and
registered visible mesh/render states, captures ready frames, takes exactly one
normal reward, then verifies depletion and another rejected gather.

The cleared control must retain zero base/produce components, no focus and no
blocking overlap at its former trunk position. A final mapped manual save is
loaded by a second actual process; serialized state, deadlines and actual depleted
components must match, and that read-only relaunch must not change any save bytes.

## Execution and limits

`Scripts\Test-ForageRenewal.ps1` takes an explicit package, fixture and fresh output.
`-GuardCase graphics` checks rejection of the wrong synthetic config destination;
`-GuardCase overlap` rejects simultaneous endurance/renewal on the same observer.
`-CancelProbe` writes the stop marker after20wall seconds and requires cancelled,
never passed. Normal invocation performs write-route then separate-process reload.
Only test flags activate the observer; normal preview keeps human input/no actor.

The actor has a900-second ceiling and45-second waypoint/12-second progress limits;
no navigation retries/teleports. The attached wrapper waits at most960seconds then
requests graceful cancellation before stopping only its owned PID if necessary.
`stop-renewal.txt`, run pause/stop/identity/deadline are checked once per second.
Atomic phase result/events/visual-component records update every5seconds.
Each process has fresh or explicitly synthetic graphics/UserDir destinations.
Sleep hours, real wall/paused/unpaused/engine time and natural time are separate.
This is **normal sleep advancement**, not45minutes of unpaused clock progression.

One coherent before/ready/reharvest frame sequence is planned at1920x1080,
offscreen/no audio capture, with changing actual day/weather/light. Mesh component
checks alone do not prove rendered appearance; inspect the actual game frames too.
No physical scanout, comfort, art approval or clean-performance claim is made.

## Result

**Existing renewal behavior passed; no production gameplay/rendering fix was
needed.** The successful output is
`Saved\VisualPlaytests\20260920-050723-5cc6c8a5\forage-renewal-01-audited`.
The write process completed457 native assertion evaluations and the separate
reload39. These counts include repeated control/renderer checks, not496 distinct
gameplay scenarios.

| Measure | Observed result |
| --- | --- |
| Route wall time after observer preparation | 140.381seconds, including1.051seconds of initial load/settle |
| Unpaused / paused wall time | 133.439 /5.892seconds |
| Unpaused engine delta | 133.352seconds |
| Ordinary bed sleep | 3actual mapped rests,24hours |
| Additional natural progression | 0.889013hours, independently matching engine delta |
| Food / walking | 2actual carried berries eaten;17reached waypoints including doorway/bed |
| Rejected gathers | 3early plus3after reharvest; no reward/action/deadline change |
| Successful harvests | Node8:+5branches, node12:+3flowers, node10:+5berries, once each |
| Save/load | 2manual saves,1exact paused in-process roundtrip, then another real process |
| Relaunch integrity | 7CRC-valid saves/backups, all byte-unchanged by read-only relaunch |
| Cleared control14 | Zero base/produce meshes, no resource focus, no blocking trunk-area overlap |

The first rest ended near hour90.373: branch8 and flower12 produce appeared
automatically, berry10 remained depleted. Rest2 ended near98.379 with berries
still depleted. Rest3 ended near106.386 and berry produce appeared. Original
deadlines remained exactly88.11285109,88.49711500 and100.99135901 through rest.
After actual reharvest, new deadlines were130.61427582,130.68757002 and142.76028119.
Those exact values and the entire paused final state survived separate-process
load at hour106.85917178.

Actual scene checks observed3branch,15flower and8berry produce components when
ready, zero when depleted, with registered visible meshes and created render state.
The ten game frames, nine-panel full sheet and native-pixel detail sheet were
inspected as one coherent comparison: the same branches, blossoms and fruit
appear and disappear, while actual HUD titles gain/lose `(renewing)`.
The permanently cleared location's separate frame remains empty.
Base flower foliage and berry-bush geometry appropriately remain while depleted.

Conditions were not frozen: the early frames are clear day4 around10:04-10:10;
later frames are rainy day5 around10:37-10:47. The existing deterministic weather
cycle and production lighting lower direct sunlight/increase fog during that rain.
The visibly flatter later shading is retained as captured, not an art/lighting
acceptance or a reason to reopen cosmetic/presentation work.

**Appended qualification,2026-09-20:** subsequent read-only log review found
F5 also selected inherited `ShaderComplexity` before images03-09. Their flatter
shading must **not** be attributed solely to weather/production lighting.
Actual day/rain changes and component/state/persistence checks remain recorded;
sealed images/proofs are unchanged. See `debug-hotkey-evidence.md`.

### Harness corrections and dispatch evidence

Two unsuccessful short attempts are retained, not hidden:

- `forage-renewal-01-initial`: the camera's proportional stick request entered the
  existing mapped deadzones before its2-degree target; the route failed after22s,
  before any rest/harvest. Only the test driver now uses a deliberate0.65stick
  until alignment, following existing observational-route practice.
- `forage-renewal-01-dispatch-initial`: the driver checked the previous deliberate
  cooldown-error toast immediately after queuing F5, before normal input dispatch.
  It stopped after30s; no actual save failure was logged or proven.

The corrected driver explicitly waits/verifies pack opening, saving, closing and
food actions. Gather/reject assertions occur0.7seconds after queueing; load/rest
checks wait0.8seconds. Every rejected gather additionally proves a **fresh**
error response by comparing its reset toast timer against independently measured
decay (for example0.662→7.300seconds), rather than passing on an old error or a
button that did nothing. Errors are never cleared to force a pass.
Queued rest accounting subtracts separately accumulated natural engine delta
before asserting each8-hour jump. Load and pause are queued together, then
verified after dispatch against exact state and the real load-response message;
this avoids an intervening unpaused simulation frame.

One already-running intermediate build was preserved as
`forage-renewal-01-pre-dispatch-audit`, **unrun/superseded**, when the coordinator
requested the complete dispatch audit. The audited route then passed once;
no gameplay timing/input deadzones, rewards, cooldowns, inventory or health were
changed to make it pass. No navigation obstacle was bypassed or save reset hidden.

The final executable also passed isolated wrong-graphics and overlapping-mode
rejection, explicit cancelled-not-passed stop behavior,308save-routing/input checks
including ordinary human-preview startup, and40launcher guards. Six fixture-copy
tests cover both allowed synthetic roots, unchanged copies, occupied-destination
rejection, non-test-source rejection and malformed envelopes. The analyzer checks
same-ID deadlines/components/prompts, exact reload state, saved hashes/CRCs and
frame dimensions. The broader914/70/106suites were not rerun or newly claimed for
this test-only change. No other actor/framework/dependency or production setting
was added. All124character/source-art hashes and9protected original/selected
preview/default hashes match; five owned bootstrap resaves were restored.

This covers selected24h/36h wild renewals through **normal sleep**, not additional
wall-clock endurance, every resource type, all bed layouts, indefinite stability,
physical input comfort or scanout. It does not resolve tearing or broader UI/art
feedback. The receipt is diagnostic-only and the launcher must reject it; accepted
video-sync-01/jenny-review remains selected.
