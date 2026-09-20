# Development setup

## Installed prerequisites

- Epic Games Launcher: `C:\Program Files\Epic Games\Launcher`.
- Visual Studio Build Tools 2022 17.14.41: `E:\Tools\VSBuildTools`.
- Windows SDK: 10.0.26100.0.
- The Microsoft installer signature was verified before launch. The first
  Build Tools attempt returned 1618 while Epic's installer was active; the
  serialized retry completed with exit code 0, without a required reboot.
- Normal builds do not need an administrator shell.
- The build/start/test scripts set Unreal's documented `UE-LocalDataCachePath`
  override to this project's `DerivedDataCache` on E:, for their own process and
  children only. They do not change persistent user/system environment settings.

## Unreal installation status

**Unreal Engine 5.8.2 is installed at `E:\Program Files\UE_5.8`.**
Epic's install metadata reports `bIsIncompleteInstall=False`; the engine has
successfully compiled the project, executed its content bootstrap, run the
initial gameplay smoke scenario, and produced a Windows package.

### Earlier launcher issue

After sign-in, the Unreal Library had no engine tiles; Install Engine opened
the library, but the add-version control did not create an engine entry.
The news page loaded normally. Jenny later confirmed installation began working;
the exact cause was not established.

The launcher was restarted after its self-update, and the News/Library view was
refreshed. Neither restored engine availability. "Hide Game Library" is already
unchecked. No account restrictions, license agreement, or credential settings have
been bypassed, and no login caches have been deleted.

Epic's official installation flow is Unreal Engine > Library > Engine Versions
> + > choose a version > Install. The earlier failure occurred before a version tile
appeared; it was not a C++ compiler error. The documented pricing/EULA confirmations
also require the user's review when presented, but no such dialog has appeared
during the observed failure. The cause of the empty engine catalog is unconfirmed.
Avoid speculative permission rewrites, firewall changes, or deleting sign-in data.

- Installation guide: https://dev.epicgames.com/documentation/en-us/unreal-engine/install-unreal-engine
- Launcher updates: https://www.epicgames.com/help/c-32735058/c-36403860/a12304262
- Service status: https://status.epicgames.com/

The installation blocker is resolved. The earlier diagnostics above are retained
as history, not current setup steps. Do not repeat launcher repairs.

The selected installation is **Unreal Engine 5.8.2** at `E:\Program Files\UE_5.8`.
The resolver checks this path, Epic's installation manifest, and common `E:` locations.
An alternate path can be supplied explicitly:

```powershell
.\Scripts\Build-Game.ps1 -EngineRoot 'E:\Program Files\UE_5.8'
```

No MetaHuman data download is required just to compile the technical foundation.
The final heroine pipeline remains a separate feasibility/visual approval step.

## Repeatable workflow

1. Run `Scripts\Test-Native.ps1` to exercise game rules and persistence.
2. Run `Scripts\Build-Game.ps1`. It imports only source assets whose expected
   sizes are recorded in `Assets\asset-manifest.json`.
3. Run `Scripts\Test-Game.ps1` for actual engine input/gameplay/save integration
   checks and rendered captures. The harness uses `Saved\Automation\SmokeSave`
   rather than normal saves; previous reports/images are archived before a run.
4. Run `Scripts\Start-Game.ps1` for standalone gameplay through the engine.
5. Run `Scripts\Build-Game.ps1 -Package` for a standalone Windows distribution.
   For autonomous work, use `-ArchiveDirectory 'Build\Releases\<candidate>'` to
   preserve the known-good player build. Test it with `Test-Game.ps1 -Packaged
   -PackageDirectory 'Build\Releases\<candidate>' -OutputDirectory
   'Saved\Automation\<candidate>'`. See `autonomous-development.md` for run controls.
6. Review the packaged build, not editor FPS, against the 60 FPS target and
   controller-only playthrough in `game-plan.md`.

## Isolated human-play preview

`Preview.cmd` requires **PowerShell 7 (`pwsh`)** and launches the exact separate
candidate selected by the repo-relative `Preview.json`. Keep the entire candidate
archive, its `acceptance-receipt.json` and `Verification` directory together.
A build receipt alone is not acceptance. Missing, ambiguous, rejected, unsupported
or hash-mismatched candidates fail explicitly; no newest-folder fallback exists.
Neither this launcher nor selecting a candidate rebuilds or promotes `Build\Windows`.

```powershell
.\Scripts\Start-Preview.ps1 -ValidateOnly
.\Scripts\Start-Preview.ps1
.\Scripts\Start-Preview.ps1 -Profile second-review
```

The first command prints the resolved executable, working directory, checkpoint,
hash and argument list without launching. A human launch supplies only
`-HomesteadPreviewProfile=<id>`: normal controller, keyboard/mouse, sound and game
flow remain enabled. There is no smoke/visual actor, synthetic route, input
isolation or automatic quit. The existing prototype footer identifies the active
preview profile. Settings **Save and quit**, F5/F9, autosaves and recovery behave
normally within that profile.

Profile IDs are **1-32 ASCII lowercase letters, digits or hyphens, starting with
a letter**. Empty, duplicate, malformed, uppercase, absolute-path and traversal
arguments are rejected before save access, never converted to an ordinary save.
The runtime uses the fixed Windows user-settings root:
`%LOCALAPPDATA%\SurvivalGame\PreviewProfiles\profile-<id>\SaveGames`.
The `profile-` prefix also avoids Windows reserved device-directory names.
All manual/rotating-auto/recovery files and their `.bak`/atomic `.tmp` siblings
stay in that directory. The default selected profile is `jenny-review`.

The same profile persists across relaunches and explicitly selected compatible
preview packages. A different ID starts a separate preview world; it does not
copy, reset or migrate any existing world. This isolates game save slots, not
every Unreal graphics/config/cache file. No save schema was changed.
Unflagged ordinary launches retain `ProjectSavedDir\SaveGames`; smoke/visual
modes retain their explicit `SmokeSave` sandbox even with a valid preview flag.
An invalid preview flag still fails before either route is used.
The measured `review-01` Development executable resolves its unflagged default
to `Windows\SurvivalGame\Saved\SaveGames` inside that archive. This is observed
behavior for that package, not an assumption about every Unreal configuration;
the explicit user-root preview route is independent of archive location.

Use **`Preview.cmd`**, not a candidate's bare executable, for review. Return to the
original with **`Play.cmd`**; its executable and personal worlds are not replaced.
Do not manually copy personal saves into these profiles as part of verification.

The explicit selection binds the candidate and executable SHA-256. Its schema-2
acceptance receipt must say `verificationStatus: passed`, declare
`previewSaveRoutingVersion: 1`, identify a privately checkpointed commit, and
match the executable and proof-index hashes. Old verified packages without this
runtime capability are deliberately refused. To change selection, first verify
a separately built compatible candidate and its evidence, then deliberately
update `Preview.json`; do not point at a rejected trial or edit receipts to
bypass checks. Local receipts/hashes are integrity checks, not signed distribution.

Focused checks:

```powershell
.\Tests\PreviewLauncherTests.ps1
.\Scripts\Test-PreviewSaves.ps1 -Packaged `
    -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\review-01' `
    -OutputDirectory 'Saved\Automation\preview-routing-fresh'
```

Use a new output directory. The runtime suite resolves the real default path
**read-only** and substitutes a synthetic default root for IO; it never opens
personal saves. It creates two uniquely named synthetic profiles under the real
preview root plus a test sandbox, writes all five slots and backups, and reloads
them in a second process. Copies/hashes remain in the report folder; the two
owned user-root fixtures are removed after successful verification. A separate
preview-only startup proves the normal input gate/no automated actors and stays
alive until the test closes only its own PID. In-process physical-source-style
events are not a human hardware/controller-comfort test.

Accepted content is relaxed locomotion and generic gathering/watering/weeding/
hatchet presentation. No target-aware IK or exact contact is claimed; weeds and
saplings still disappear when the original transaction commits. Rejected face
shaders and shortened hair are absent; mid-back waves and the blonde-bob request
remain unmet/deferred. The consolidated candidate's own receipt records its
functional/native-4K evidence. Do not transfer the older clean-performance
numbers below to this concurrent-GPU review run.

### Review-01 evidence

The fresh standalone package passed **170 routing writes + 130 separate-process
reads**, with all **40 save/backup files byte-unchanged** on relaunch, four invalid
startup cases exiting with status 2, and an actual preview-only process retaining
normal input/no test actors. Launcher fixtures passed **40 checks**, and existing
run/path controls passed **21**. The editor also passed the 300 routing checks.
The real default directory was not opened; only the disclosed synthetic default,
two fresh actual preview namespaces and the test sandbox were exercised.

All **914** previous assertions passed: 188 clearing, 74 gathering, 181 watering,
37 weeding and 434 full-loop. The full loop produced actual **3840x2160** captures
at **render100**; its sampled mean was **59.10 FPS**, p95 **16.90 ms**, p99
**17.05 ms**, excluding startup/readback. GPU sharing was not ruled out, so these
are functional-run timings, not a new clean-performance certification.

One final ordinary mapped gather/craft/walk/clear recording contains **84
1280x720 frames over 40.7483 seconds**, with one clearing presentation, eight
held-tool samples and a hidden tool/zero action weight afterward. Its actual
before/swing/recovery sheet was inspected: the existing generic gesture remains,
without gross new tool/body clipping; the sapling still disappears before the
swing. No new cosmetic pass, exact-contact or human-feel approval is implied.
All **124 tracked character/source LFS files** (a broader set than earlier
72-file acceptance subsets) remained unchanged. Five incidental world-bootstrap
resaves were restored and hash-checked before checkpointing.

Evidence is indexed in the candidate's `Verification\proof-index.json`, with
source/executable hashes and the published checkpoint in
`acceptance-receipt.json`. Original reports remain under
`Saved\Automation\20260920-050723-5cc6c8a5\review-01-*` and the ordinary recording
under `Saved\VisualPlaytests\20260920-050723-5cc6c8a5\review-01-packaged`.

## Controller prompt stability candidate

`prompts-01` is separate from the currently selected `review-01`. Do not change
`Preview.json` until the coordinator has reviewed that candidate. This slice
schedules only the previously deferred input-prompt report, not book/recipe/
inventory redesign, tearing or more face/hair work.

The isolated baseline failed a concrete accepted-event sequence: a `0.01`
mouse-X sample changed the paused Notes footer to keyboard hints. Its
`MouseLook` callback wrote the device flag even when camera motion was blocked.
The corrected build removes callback-based writes and uses one typed classifier
before unchanged input dispatch. Every context/book/Look/Settings/planning hint
continues to read the same controller state; layout and text are unchanged.

The hint-only policy honors inherited gamepad axis shaping (including the
current 0.25 axial deadzone) before the existing 0.2 radial modifier. Mouse
intent is one raw input unit of signed travel within 120ms, so fractional motion
can accumulate while idle jitter cannot accumulate indefinitely. Digital
presses and newly engaged controller gestures switch immediately. Previously
held analog controls cannot undo deliberate keyboard/mouse intent for 200ms;
continuing actual controller input can take over afterward. Releases, repeats,
zero values and subthreshold noise do not select a new hint device.
No game input is dropped or delayed by this policy.

The camera-response fixture uses unchanged inherited mouse sensitivity 0.07:
`0.25` raw input produces a measured `0.0105`-degree yaw change even below the
hint threshold. An initial new fixture incorrectly demanded a greater rotation
from `0.01` input; its probe was calibrated, not the game's response or the old
regression assertions. Control deadzones, camera/movement scale and audio remain
unchanged. External-event rejection still applies only in test modes; its log
records the rejection count, not real key contents.

```powershell
.\Scripts\Test-Game.ps1 -Packaged -Prompts `
    -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\prompts-01' `
    -OutputDirectory 'Saved\Automation\prompts-focused-fresh'
```

The focused scenario exercises accepted mapped inputs, noise and mixed-device
sequences, real walking/fine camera movement, pause, and both devices' actual
rendered hints. Separate preview-routing checks additionally exercise constructed
physical-source-style analog events with the automation guard disabled, plus
a real preview-only process. No global OS input injection or recording of
Jenny's keystrokes is used. These fixtures do not establish every hardware/driver
or large cursor-warp case, or replace human-controller review.

The fresh package and editor each passed **70 focused checks** and **308
save/normal-input routing checks** (the previous 300 plus eight physical-source-
style prompt assertions across write/read runs). All **914** prior gameplay/
action checks passed unchanged, plus **40** launcher and **21** run/path guards.
The full loop and 11 final prompt images are actual **1920x1080/render100**
evidence; no new 4K or clean-performance certification is claimed. Full-loop
timings were 59.76 FPS mean, 16.89ms p95 and 17.07ms p99, excluding startup/readback,
with concurrent GPU use not ruled out.

The two inspected visual sets are the failing baseline and final packaged label
comparison. The latter visibly preserves controller hints after the original
noise sequence and shows intentional keyboard/controller hints across all five
tested surfaces. Original screenshots and the comparison sheet remain in
`Saved\Automation\20260920-050723-5cc6c8a5\prompts-01-focused`; copied source/exe/
test proof is indexed under the separate candidate's `Verification` directory.
Intermediate editor diagnostics were functional checks, not extra visual tuning
passes. All 124 character/source LFS files, the active `review-01` executable and
its launcher selection are unchanged; five incidental world resaves were restored.

## Preserved original verification status

**Preserved original packaged MVP:** `Build\Windows\SurvivalGame.exe`.
The final standalone native-4K run passed **432 mapped-input steps**, including
all 18 appearance combinations, both crops, cooking/storage/construction, the
enclosed doorway, overnight survival, exact persistence, and genuine failure
followed by same-world checkpoint recovery.
At **3840x2160 with 100% 3D screen percentage**, the bounded route averaged
**59.16 FPS**, with **16.90 ms p95 / 17.03 ms p99** frame times, excluding startup
and screenshot-readback windows. The game-only stereo audio capture was
**233.984 seconds at 48 kHz**, RMS **0.02690**, peak **0.35782**, with no
near-clipped samples. These measurements do not certify every possible scene,
physical-controller comfort, or listening quality.

- Portable simulation: MSVC Debug and Release builds passed. The final suite has
  16 scenarios and 986 explicit checks, including the default opening loop,
  ordinal wall placement, multi-cell enclosure, farming, atomic transactions,
  independent fires, failure recovery, both crop cycles, and strict version-3
  persistence with explicit version-2 root-plot migration.
- `Scripts\Test-Native.ps1` was rerun successfully through configuration,
  compilation, CTest, and the detailed test executable.
- All 18 downloaded/extracted environment/audio source files match recorded hashes.
  PowerShell scripts parse and the editor Python bootstrap passes syntax checking.
- Unreal editor-module build and Python material/map/audio bootstrap now pass.
  Targets use UE 5.8's V7 build settings and 5.8 include order. Compilation is
  local-only; no remote build service is used.
- The actual engine smoke scenario passes: synthetic gamepad movement/look,
  controller/keyboard field-book navigation, menu pausing, foraging/raw eating,
  checksummed save/load, crafting a hatchet, clearing a sapling, and placing a
  foundation. Fresh clearing, field-book, and foundation screenshots are produced.
  Test saves are isolated from normal game saves.
- Windows packaging succeeded at `Build\Windows\SurvivalGame.exe`. The package
  itself passed the final complete route through the standalone executable,
  without launching Unreal Editor.
- Editor `-FullLoop` passes the expanded mapped-input route covering crafting, an enclosed
  room and actual doorway traversal, cooking/eating, chest transfers, gardening,
  rain, six overnight sleeps, harvest/replant and exact saved-world restoration.
  Genuine outdoor failure and gamepad retry restore the same protected cabin,
  garden, stored resources, clearing and appearance. Both crop types persist.
- Actual 1920x1080 captures are now asserted; earlier offscreen runs had silently
  resized to 888x500. High-DPI awareness and `-ForceRes` corrected this.
- An editor route averaged 59.75 FPS, p95 16.90 ms/p99 17.08 ms, excluding startup
  and screenshot-readback windows. The extended route measured 59.83 FPS,
  p95 16.91 ms. These are bounded automated-route measurements, not all-scene/4K
  performance certification.
- Game-only master-submix capture produced 36.416 seconds of stereo 48 kHz audio,
  RMS 0.02084, peak 0.25064, with no near-clipped samples. No microphone or other
  application audio was captured. Silence/clipping checks do not establish sound
  quality or replace listening.
- Physical-controller feel and audio listening remain unverified. Jenny liked the running-scene
  screenshot; this is positive visual-direction feedback, not full MVP acceptance.
- Three clothed adult face/body presets with three hair choices and two cosmetic
  outfit choices share a 53-bone authoring rig
  (54 Unreal bones including its wrapper), and
  idle/walk clips was authored offline and successfully imported into Unreal.
  Runtime animation, color controls, saved appearance, and the Look sidebar
  passed editor and packaged smoke checks. Camera framing and UI corrections
  subsequently passed the 1080p editor route. All eighteen mesh combinations
  resolve correctly; fine-grained sculpt sliders and clothing physics are deferred.
- Material-slot import persistence is checked in a fresh engine process before
  accepting the character import receipt. Modified Python skeletal-material
  structs must be assigned back to their array elements; merely mutating an
  iteration wrapper can leave null references after reopening.
- The bounded visual review resolved framing, readable selection and hint
  backing, and the missing character evidence. Highlight rendition on sunlit
  faces remains a documented prototype limitation; no further polish loop was
  used to disguise it. Small apron overlap can occur in some poses.
- While installation was pending, comparison with the downloaded 5.8 headers
  exposed and corrected a controller override: use `FInputKeyEventArgs`, not the
  legacy final `FInputKeyParams` overload. An engine smoke harness now exercises
  synthetic gamepad/keyboard input, menu pause, foraging/eating, saving/loading,
  crafting, and construction. Its first placement attempt correctly refused an
  uncleared sapling; the scenario now clears the site and passes.

The bootstrap does not replace existing maps or material graphs. It reapplies
the documented source-texture compression/color-space settings, rock-material
assignments, and ambience looping flag. If those import conventions need to
change, update the script deliberately. Never delete the whole Content folder
to regenerate a map or revise an imported material.

## Persistence

The Unreal integration stores checksummed save records beneath `Saved\SaveGames`.
It writes and verifies a temporary record before replacement, retains the prior
file as `.bak`, and surfaces failed writes. Three automatic slots, a manual slot,
and a recovery checkpoint are kept separately. New clearings have distinct world
identifiers so a failed new game cannot silently jump to an unrelated old world.

Unreal-side save/restore and actual failure/retry pass editor and standalone
extended routes. Appearance save records are version 4 and accept earlier
versions 1-3 with default values for newly introduced choices. Simulation records
are version 3; version 2 plots migrate explicitly to roots. Version 1 degree-based
prototype simulation records remain rejected rather than misplacing structures.
