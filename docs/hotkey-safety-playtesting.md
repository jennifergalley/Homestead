# Save/load hotkey safety

## Bounded root correction

`hotkey-safety-01` removes exactly two inherited `Engine.PlayerInput`
`DebugExecBindings` in project `Config\DefaultInput.ini`: F5's
`viewmode shadercomplexity` and F9's `shot showui`. Their normal QuickSave and
QuickLoad mappings remain. Other debug bindings, controller/menu actions,
physical input, prompt classification and graphics defaults are unchanged.
There is no engine edit, broad debug-command disable, forced Lit or ShowFlags
reset after saving. No art or save-format change is involved.

The accepted video-sync package remains the selected human preview until a
separate coordinator-reviewed selection. Its existing hotkey collision is not
changed in place. Settings save/load avoids those keyboard debug bindings.
Never remove unidentified historical screenshots or touch a running player.

## Actual before/after evidence

Run: `20260920-050723-5cc6c8a5`.
Candidate: `Build\Releases\20260920-050723-5cc6c8a5\hotkey-safety-01`.
Executable SHA256:
`747501167355103485FE91F35B274F70414CA85DDC8A4D3BC1128E22F8C5E394`.

The unchanged-config baseline starts in actual Lit (enum3). F5 dispatches one
save and changes to ShaderComplexity (enum8): `PostProcessing` and
`VisualizeMegaLights` turn off, `ShaderComplexity` turns on. Both the failed
empty-save F9 and successful F9 request `shot showui`, write one game screenshot
each, and dispatch one load each. Those two unsolicited PNGs remain in the
baseline's isolated screenshot directory.

The fixed write/reload fixture passes29+5 checks. Immediate and delayed JSONL
snapshots record actual viewport mode, complete ShowFlags, screenshot request/
filename/file count, effective bindings, dispatch counters, world identity,
serialized-simulation MD5, seven appearance values and visible feedback.
Repeated F5/F9 and controller Settings save/load each invoke exactly one action,
preserve complete renderer flags and state, and create no screenshot request
or screenshot file. The world includes a legitimately gathered/depleted branch
node and a real nondefault appearance selection. This is a disclosed functional
fixture using existing test teleports, not an ordinary walking route.

A blocked, owned temporary save file produces the real save-failure message
without replacing the previous envelope or changing rendering. A missing-save
F9 produces the real error without taking a screenshot. An explicitly chosen,
supported ShaderComplexity mode survives subsequent F5/F9 unchanged, proving
the fix does not force Lit. A fresh reload process starts Lit and restores the
exact serialized world/appearance/cooldowns.

A third process uses the same synthetic preview profile with **no automation
flag**. Read-only `-HomesteadSaveAudit` confirms exact auto-loaded state in Lit;
startup reports `automation_input=0 smoke_actor=0 visual_actor=0`, stays running
and does not alter the saved files. Only its owned PID is closed. This verifies
the normal input policy, not a physical-controller usability trial.

## Harness boundary and preserved failure

The first fixed attempt passed24 checks then failed its deliberate Unlit
setup. UE5.8 `UGameViewportClient::SetViewMode` rejects Unlit when
`AllowDebugViewmodes()` is false; ShaderComplexity is explicitly exempt. Its
preceding console log reports the requested mode before this policy is applied.
The strict actual-mode assertion caught that distinction. We did not enable
extra debug modes or weaken the non-Lit preservation requirement: the corrected
fixture deliberately chooses the already-supported ShaderComplexity mode.

That failed output remains at `hotkey-safety-final`, and its package at
`hotkey-safety-01-unlit-fixture`. Earlier baseline compile/API and script parser
errors are retained in build logs. Effective F2/F3 bindings remain unchanged;
this does not claim that packaged Unlit is allowed by engine policy.

## Isolation and reproduction

```powershell
.\Scripts\Test-HotkeySafety.ps1 `
  -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\hotkey-safety-01' `
  -OutputDirectory 'Saved\Automation\hotkey-new-fixture'
```

Use a fresh output path. The existing opt-in smoke actor requires an explicit
baseline/write/reload phase, actual isolated graphics branch and screenshot
root. The runner uses the production save-route resolver with a new
`hotkey-<output-hash>` synthetic preview namespace, refuses an existing write
namespace and proves the independent reload uses the same directory. It retains
and copies those test saves. Preview profiles isolate saves, not automatically
graphics preferences; `GameUserSettingsINI` and `UserDir` are separately isolated.
Each fixture has a five-minute timeout. No OS input or desktop capture is used.

`Test-Game.ps1 -RequireLit` and `Test-FeedbackLayout.ps1 -RequireLit` add a
read-only every-tick mode/ShaderComplexity guard and reject missing runtime
guard evidence. They never set the mode. Capture metadata also records actual
mode, full flags and render percentage. Do not combine this guard with the
focused fixture's deliberate non-Lit preservation experiment.

## Fresh normal-Lit full homestead

The fresh4K/render100 full-loop passes434 checks with13,210 actual guard samples
and final mode3/ShaderComplexity0. All14 native frames have Lit metadata,
including post-save cultivated garden, shelter/night and the failure/retry prompt.
Actor-cadence sampling reports59.82mean FPS,16.91ms p95 and17.08ms p99 across
13,188 samples, excluding startup/readback. These are not GPU/Present/scanout
measurements, a clean-machine benchmark or proof of long-run performance.
Jenny may independently play another package; no OS/driver/background settings
were changed. This is not a second45-minute endurance run.

Focused source evidence:
`Saved\Automation\20260920-050723-5cc6c8a5\hotkey-safety-baseline`,
`hotkey-safety-supported` and `hotkey-safety-01-full-loop`.
The acceptance receipt and indexed `Verification` directory bind final results,
source hashes and executable hash to the checkpoint.

## Retained gates and actual image review

The same executable passes914 action/full-loop checks (434full-loop,
188clearing,74gathering,181watering,37weeding),106book-clarity,70prompt-intent,
112VSync,308routing/input and40launcher checks. The seven gameplay/book/prompt
runs all pass the sustained Lit guard. No simulation rule changed, so no
unrelated portable-simulation rerun is claimed.

Active-feedback reruns pass49checks at720p and49at4K, with26 immediately active
success/error/recovery frames and1,761/1,579 Lit-guard samples. Full text, measured
Canvas bounds, replacement, expiry and pause assertions remain intact. Their
existing runner leaves the default world-scale policy alone (raw
`r.ScreenPercentage=0`); these are native-size UI/Lit proofs, **not** assertions
of100-percent underlying3D resolution. The separate full-loop above explicitly
requests and records100.

Two coherent review sets were inspected: the native-Lit world sheet and active
feedback sheets at both sizes. The world has ordinary dawn/day shadows and
dark night/firelight rather than the accidental debug presentation. Active
feedback remains clear of headings/tabs/selection/footer. This does not approve
the existing bright dawn face, hair, night readability or broader art direction.
Sheets reduce original frames for review; all native originals remain available:
`hotkey-safety-01-full-loop\lit-world-sheet.jpg` and
`hotkey-safety-feedback\feedback-720-sheet.png` / `feedback-4k-sheet.png`.

All124 character/source-art hashes and nine selected/original/default protection
hashes match. Only the five known owned bootstrap world resaves were restored.
No player process, personal save, graphics default or selected preview changed.

Historical non-Lit qualifications in `debug-hotkey-evidence.md` remain valid.
This fixes a specific accidental debug shortcut, not physical tearing, heroine
appearance, broad lighting, performance or Jenny's subjective approval.
