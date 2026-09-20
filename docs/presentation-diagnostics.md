# Movement presentation diagnostics

**Status: investigated, unresolved. No rendering fix or graphics-setting change.**
This is `presentation-diagnostics-01`, not the rejected face-presentation experiment.
The accepted human preview remains `book-clarity-01`, selected separately in
`7051fc3`, with the same persistent `jenny-review` profile.

## What was observed

Read-only `EnumDisplaySettings` and `Win32_VideoController` both reported the
primary desktop at **3840x2160, 30Hz** on the RTX 5080. Before/after queries agree.
This is a reported desktop mode, not a measurement of panel scanout or VRR.
The original player's persisted graphics file reports a 3756x2120 window size.
No display, driver, power, graphics configuration or player save was changed.

The fresh diagnostic executable reported identical start/end values:

| Runtime observation | Value |
| --- | --- |
| RHI / AA | D3D12 / `r.AntiAliasingMethod=4` (TSR) |
| Actual viewport mode | 2, windowed; wrapper explicitly forces windowed/offscreen |
| VSync / frame cap | `r.VSync=0`, `t.MaxFPS=60` |
| Dynamic resolution | User setting off, `r.DynamicRes.OperationMode=0` |
| Primary scale policy | `r.ScreenPercentage=0`, default 100, desktop mode 1 (automatic policy) |
| TSR history scale | 200; this is not the primary scene scale |
| Present-related settings | `rhi.SyncInterval=1`, `r.D3D12.UseAllowTearing=1` |
| Scene settings | Motion-blur default off; virtual shadows, Lumen GI/reflections retained |

`r.FullScreenMode=1` and `PreferredFullscreenMode=1` are preferences/policy, not
proof that the actual viewport is fullscreen. The observed viewport mode is 2.
Likewise scale 0 means automatic/default policy, **not 0% rendering**. Output
dimensions and user-setting scale do not reveal the internal per-view render
rectangle. That fraction was not instrumented; do not label these diagnostic
captures 100%-scale native rendering.

UE's D3D12 present path derives sync interval from its VSync-lock decision, allows
custom-present overrides, and conditionally adds the allow-tearing flag when
interval is zero, the swapchain is not fullscreen, and capability is supported.
Reading the two related CVars does not prove which branch/flags reached DXGI.
No actual Present events, compositor mode, VRR engagement or physical scanout
were recorded. These settings plus a 30Hz desktop suggest a useful human timing
check, not a confirmed cause of Jenny's report.

## Evidence and limits

Only two coherent diagnostic capture batches were made, on the same executable
and unmodified graphics settings. Each starts from a fresh isolated test world,
uses the existing mapped walk/turn/camera inputs without teleport/time/state
edits, and records a screenshot-free 20-second interval before capturing.

| Output | Screenshot-free actor-tick evidence | Subsequent frame capture |
| --- | --- | --- |
| 3840x2160 | About 60 ticks/s; segment p99 16.93-17.11ms; no >33.33ms samples | 17 frames, measured request rate 0.878Hz |
| 1280x720 | About 60 ticks/s; no >33.33ms samples | 105 frames, measured request rate 6.405Hz |

These are instrumented game-thread wall intervals, **not GPU durations, Present
timestamps or clean-performance certification**. Concurrent GPU use was not ruled
out. Capture visits different positions from timing, so it is not a controlled
performance A/B. The inherited observer's "8Hz" wording describes its nominal
request threshold, not delivered cadence. At 4K, many capture ticks exceeded its
legacy one-second route-clock cap: `telemetry.csv` route times are not an accurate
wall timeline there. `presentation-timings.csv` retains unclamped wall intervals;
the analyzer uses those to report real request cadence.

The inspected walk/turn/sweep samples did not isolate a repeatable whole-frame
horizontal discontinuity. They **cannot rule out** brief temporal/shader/geometry
flicker between samples, and cannot show or exclude display scanout tearing.
No surgical rendered-defect cause was established. No fix was attempted.

Evidence directories under `Saved\VisualPlaytests\20260920-050723-5cc6c8a5`:
`presentation-diagnostics-01` and `presentation-diagnostics-01-720`.
Each has original game-only PNGs, pose telemetry, unclamped timing CSV,
start/end runtime queries, read-only display modes, exact launch/executable hash,
analysis and a labeled contact sheet. A separate ordinary-forage run is a
functional observer regression, not a third diagnostic comparison.

## Earlier explicit 100% tests versus human preview

Existing full-loop logs for review, book clarity, prompts, clearing, weeding,
watering and gathering include **both** `-ExecCmds="r.ScreenPercentage 100"` and
an executed runtime `r.ScreenPercentage = "100"` confirmation. Their evidence is
not merely output dimensions; the new default-auto diagnostic does not invalidate
those explicit runs. They did not continuously instrument internal view/effect
buffer dimensions. The precise claim is requested and runtime-confirmed
**100% primary screen-percentage CVar**, with separately verified output size.
The evidence addendum records original log hashes/lines; prior sealed receipts
and candidates were not rewritten.

Normal `Preview.cmd` passes **only** `-HomesteadPreviewProfile=jenny-review`.
It does not force resolution, window mode, VSync, frame cap or render scale.
The selected book package's persisted graphics diff contains
`sg.ResolutionQuality=0` and no resolution/VSync/frame-cap override; defaults are
1920x1080, windowed mode 2, VSync off, frame cap 60, automatic scale policy.
Actual human window sizing/settings may subsequently change. Preview profiles
isolate game saves, not every Unreal graphics preference. This diagnostic did
not launch or alter Jenny's selected preview to inspect its live settings.

## Reproduce the diagnostic, not a fix

```powershell
.\Scripts\Playtest-Visual.ps1 -Packaged -PresentationDiagnostics `
    -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\presentation-diagnostics-01' `
    -OutputDirectory 'Saved\VisualPlaytests\presentation-fresh' `
    -Width 1280 -Height 720
```

Use a fresh output directory. The opt-in flag extends the existing observer,
not a second test actor. It retains test-sandbox saves and physical-input
isolation. Without visual-test flags, normal preview accepts human input and
does not activate diagnostics. No OS input injection, desktop capture,
microphone/system audio or player-process manipulation is used.

Seven synthetic analyzer tests cover cadence contamination, missing motion/
sweep, incomplete intervals, wrong capture phases and missing process isolation.
The fresh executable also completes the unchanged ordinary forage/gather route,
plus 308 save-routing/normal-input checks. Prior 914 action/full-loop results are
not claimed as new diagnostic-package verification.

## Next human check

1. In the accepted `Preview.cmd` build, record the current in-game/window settings
   and Windows Advanced Display refresh rate **without changing them first**.
   Confirm whether the reported 30Hz mode is intentional.
2. Reproduce a short, repeatable walking/camera pan. Note whether the symptom is
   a moving full-width seam with displaced top/bottom portions, or local
   shimmering/flickering foliage/shadow edges.
3. If it is visible on the monitor but absent from a contemporaneous game-only
   framebuffer capture, presentation/scanout becomes a stronger hypothesis,
   not automatic proof. Human observation or a tightly framed monitor recording
   would be needed; avoid unrelated desktop content.
4. Report those observations before any coordinator-scoped single-variable
   comparison or renderer change. Do not silently turn on VSync or change AA,
   exposure, shadows, driver settings or the display mode.
