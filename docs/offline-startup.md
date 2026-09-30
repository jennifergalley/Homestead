# Offline Shipping startup and human window default

`offline-startup-01` addresses the unwanted Unreal TraceControl listener and
Jenny's request for windowed fullscreen on human launch. It does not change
gameplay, art, VSync, quality, frame cap, monitor resolution/refresh or security
rules. The accepted hotkey package remains selected until coordinator review.

## Observed cause and supported correction

The interrupted endurance process PID37440 had TCP LISTEN `0.0.0.0:1985`, no
owned UDP endpoints, and `LogTrace: Control listening on port 1985`. This was
the in-process TraceControl server, not proof that mounted messaging plugins
were communicating. `-notraceserver` only prevents separate UnrealTraceServer
autolaunch; it does not disable this listener. TraceControl can also choose a
random32768-40959 fallback port.

UE5.8 `TargetRules.bEnableTrace` defaults false in Shipping. The fresh supported
Shipping target uses the installed Shipping engine objects, without engine edits,
global macro overrides or `bOverrideBuildEnvironment`. Its actual native probe
reports `shipping=true`, `traceCompiled=false`; the linked executable lacks the
TraceControl listening message present in the Development implementation.
Shipping also omits ordinary Development debug-console/logging facilities;
the limited native JSON probe, not missing log text, supplies runtime evidence.

Installed `UnrealPak` is a modular Development Program. Its actual
`UnrealPak-TraceLog.dll` contains that listener implementation, so it was **not**
launched. The bounded build path uses proper UAT Shipping staging with
`-skipbuild -skipcook -skippak`, preserving hash-identical existing cooked
containers while staging newly built Shipping binaries. No old executable/DLL
was copied across configurations. No editor, bootstrap, import, cooker,
UnrealPak or Zen runtime was launched by this build/stage.

```powershell
.\Scripts\Build-Game.ps1 -EngineRoot 'E:\Program Files\UE_5.8' `
    -Configuration Shipping -ReuseCooked -Package `
    -ArchiveDirectory 'Build\Releases\<run-id>\<fresh-candidate>' `
    -ReusePakDirectory 'Build\Releases\<accepted-candidate>\Windows\SurvivalGame\Content\Paks'
```

This is only for compatible, unchanged cooked content/configuration, not a way
to pick up new assets, shaders or edited cooked defaults. Omitting `-Package`
builds Shipping code only. Ordinary Development/editor workflows can still
open listeners and remain outside this fix; do not launch them under the
current runtime hold. `-SkipAssets` alone never skipped editor/cook work.

The executable is resolved explicitly as
`SurvivalGame\Binaries\Win64\SurvivalGame-Win64-Shipping.exe`.
Mixed Development/Shipping executables in one package are rejected.

## One isolated Shipping probe

Package: `Build\Releases\20260920-050723-5cc6c8a5\offline-startup-01`.
Executable SHA256:
`CE5C9A262394E0B63E1105D71D4EBFB82CB61D4041FDC994618665F6FF29E634`.
Output: `Saved\Automation\20260920-050723-5cc6c8a5\offline-startup-01`.

Owned PID45332 used1280x720 **offscreen windowed**, no sound capture, explicit
synthetic graphics/UserDir roots and profile `offline-7873e0a9d2f0`. The copied
prepared homestead is the prior full-loop fixture, not a fresh start:
SHA256 `D07D8406AC875A9212E9C7F37E502DFC6E86EE501397A18C54BB3D3EAA5214C3`.
No player world was read/copied. The profile uses the real production resolver.

- 38 owned-PID TCP/UDP snapshots: zero endpoints across21.237seconds; first
  sample0.819seconds after process start, maximum sample gap0.659seconds.
- 808 observational ticks remained actual Lit with lighting and without
  ShaderComplexity; the guard never resets the renderer. Heroine present.
- Mapped Tab opened the book; F5 saved exactly once and F9 loaded exactly once.
  Paused saved-state and immediately loaded-state MD5 both
  `1670fdc6498ca0100d063827d168b227`; the same world ID survived.
  Normal loading then closed the book and resumed simulation.
- No F9 screenshot request or screenshot files. Only this explicit probe
  suppresses physical input and exits automatically; normal preview has no
  probe flag, test actor, automatic action or forced quit.
- Actual window mode2, viewport1280x720; stored window2 and1920x1080 remained.
  VSync preference Off / `r.VSync=0`, frame limit / `t.MaxFPS=60`,
  `r.ScreenPercentage=0`, AA method4. No graphics setting was toggled.

The original wrapper **failed** its byte-identical INI assertion after native
checks passed. UE normalized the full-default fixture into its ordinary
`;METADATA=(Diff=true, UseCommands=true)` form: unchanged inherited keys were
removed and normal engine fields added. That failure is retained, not called a
byte-preservation pass. `graphics-comparison.json` checks all10 requested keys
against the exact hash-verified source defaults; effective values are unchanged.
`reviewed-result.json` analyzes the original raw evidence without another launch.
Eight comparator checks reject actual preference changes and changed defaults.

This is one small Shipping-specific startup/same-process save proof, not914
Shipping gameplay checks, a second-process save test, physical scanout evidence
or a claim that no brief socket can ever occur between observations. The compile
boundary plus actual sampling address the observed persistent TraceControl
listener. No firewall settings, permission dialogs, driver settings or other
processes were changed.

### Current probe-harness defects (2026-09-29)

The Shipping runtime evidence can be valid even when the automation wrapper reports a stale
application-specific failure; do not call such a run a clean harness pass until these are fixed:

- `Scripts\Test-OfflineStartup.ps1` prepares
  `PreviewProfiles\profile-X\SaveGames\Homestead_Manual.sav`, but Estate save routing now reads
  `SaveGames\Estate\`. The fixture needs the added `Estate` directory and its existence guard must
  inspect the profile directory two levels above.
- `HomesteadStartupProbe.cpp` case 4 requires legacy
  `AHomesteadCharacter::IsEquipmentPresentationReady`; the MetaHuman heroine never sets it. Replace
  that condition with a MetaHuman-aware rendered-heroine check (for example `HasHeroine()`).

The supported current Shipping recipe remains `Build-Game.ps1 -Configuration Shipping -ReuseCooked
-Package` with hash-identical cooked containers, isolated candidate `-UserDir` and owned-PID endpoint
sampling. On the late candidate it observed zero TCP/UDP endpoints in 301 samples across three runs,
`shipping=true` / `traceCompiled=false`, copied Estate save F5/F9 MD5 equality, Lit heroine ticks and
candidate-local writes. A bounded `Development-Run.ps1 -Action Start` prerequisite is still required
by the wrapper; do not treat a temporary test-run patch as a committed script fix.

## Human borderless launch and preferences

`Preview.cmd` now requests native **windowed fullscreen** with `-Res=0x0wf`.
UE's parser selects mode1 and resolves zero dimensions using monitor metrics;
it overrides a legacy saved Windowed2 value without a hardcoded resolution,
exclusive-mode request or persistent graphics-default edit.
`Start-Preview.ps1 -Windowed` replaces that argument with `-windowed`.

**Visible mode1/client-to-monitor bounds remain human-unverified.** Engine
`FWindowsWindow::SetWindowMode` calls `ShowWindow(SW_RESTORE)` for borderless
even before the offscreen show/hide branch. An unattended borderless probe
could therefore activate a window. It was deliberately not attempted while
Jenny might be playing. The actual windowed opt-out was exercised; the human
borderless argument/parser is statically checked, not mislabeled as visible proof.

Shipping normally moves generated config to AppData. The human launcher
explicitly supplies `-UserDir=<candidate>\Windows\SurvivalGame` to retain
candidate-local settings. The fresh candidate's only graphics seed is a
hash-verified read-only copy of the accepted hotkey package's
`GameUserSettings.ini`; `graphics-seed.json` records it. No source preferences,
original/book config or game saves were edited. The fixed `jenny-review` save
root is unchanged. Save profiles isolate worlds, **not** graphics preferences.

`PreviewLauncherTests.ps1` passed46 non-executable guards, including both
binary configurations, mixed-binary rejection, native borderless argument,
windowed opt-out and explicit Shipping UserDir. Receipt/hash checks remain.
Do not select this candidate until coordinator review. A later visible launch
is Jenny's choice; neither her current game nor desktop shortcut was changed.

## Cancelled endurance remains cancelled

The normal-Lit45-minute run was stopped after465.462seconds because of the
listener report. Short guard/sanity and partial evidence are preserved separately
in `endurance-lit-playtesting.md`. No45-minute pass, retry or diagnostic promotion
is claimed. Tearing, subjective art approval and future environment work remain
outside this correction.
