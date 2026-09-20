# Transient feedback layout

`feedback-layout-01` is the explicitly scheduled correction for transient
feedback covering the field-book heading. It is not a field-book redesign,
recipe-learning feature, character revision or tearing fix. The accepted human
preview remains `video-sync-01` / `jenny-review` until coordinator review.

## Reproduction and limited change

The unchanged layout was reproduced with real mapped F5 saves and rejected
hatchet crafting in a fresh synthetic world, before changing toast placement.
At1280x720 the existing toast backing occupied x340..940/y75.33..136.67;
the actual "Your pack" draw occupied x269.33..367.29/y106..131.33.
The title and carried/paused caption intersected the backing. Both save success
and craft rejection reproduced this at720p and native3840x2160.

While the book/Look is open, the same feedback panel now uses the unoccupied
upper-right HUD band beside the calendar. It retains the existing font, size,
colors, padding, messages, minimum height and lifetime. World/planning feedback
keeps its original centered position. Shared Canvas word wrapping returns all
lines for feedback instead of cutting off after two; the backing grows to fit.
Book/context line limits, panel geometry, tabs, row positions, footer, camera,
input bindings and simulation-pause rules are unchanged.

The current renderer expires success after five seconds and error after eight;
it does not animate a fade alpha. This change preserves those existing clocks,
including while the simulation is paused.

## Isolated native fixture

```powershell
.\Scripts\Test-FeedbackLayout.ps1 `
    -PackageDirectory 'Build\Releases\20260920-050723-5cc6c8a5\feedback-layout-01' `
    -OutputDirectory 'Saved\Automation\feedback-example-720' `
    -Width 1280 -Height 720
```

Use another fresh directory and3840/2160 for4K. The existing smoke actor is
selected only by the explicit smoke/feedback flags; no new actor or automatic
human-preview behavior is introduced. The wrapper bounds each process to five
minutes, reports its PID and only closes its own process.

All saves go to the fixture's `SmokeSave`. Graphics use its explicit
`Graphics\GameUserSettings.ini` and `EngineUser`; the native fixture checks the
actual Unreal config-branch write destination before toggling. The main smoke
and routing wrappers now use the same per-output graphics isolation. Preview
profiles themselves still isolate game saves, not graphics preferences.

Inputs are simulated engine events, not OS keyboard/mouse injection. Every
action waits for actual input dispatch before checking/capturing its result:

- Real saved-game success and unaffordable craft rejection, controller/keyboard.
- Actual read-only synthetic graphics-file rejection, then successful On/Off
  replacement on the same selected Settings row; the fixture finishes Off.
- A second actual save creates a backup. The fixture deliberately corrupts
  **only its own primary save**, then mapped F9 recovers the real backup and
  reports the full existing recovery message. This is not personal-save damage.
- Pack, Craft, Settings, Look, building plans, active planning and world-only
  feedback; cancellation and unchanged page/row identities.
- Error still visible near eight seconds, normal error/success expiry, stable
  protected book geometry and exact paused simulation state.

Each active-feedback capture has a `.layout.json` recording the text actually
sent to Canvas, measured line rectangles, panel bounds, color, viewport and
protected text/regions. Dimensions are cached during DrawHUD: Unreal detaches
the Canvas between draws, so inspecting its pointer later is invalid. The
analyzer independently recomputes intersections/containment and reconstructs
full text from draw calls; raw PNGs provide the rendered evidence.

```powershell
python .\Scripts\Review-FeedbackLayout.py `
    'Saved\Automation\20260920-050723-5cc6c8a5\feedback-layout-final'
```

## Retained development evidence and limits

Baseline instrumentation first had two compile errors (wrong file API and an
Actor member-name collision), corrected without a production behavior change.
The first runtime fixture incorrectly expected I to open Pack over initial
Notes; the actual binding toggles the book closed. The complete opening and
page-return sequence was corrected to use existing controls. A subsequent
strict containment check caught the detached Canvas dimensions; measured
viewport dimensions are now cached during the actual draw, not substituted
with the requested screenshot size. Failed reports/build logs remain retained.

Only two visual-review sets were used: original active baseline and final
active feedback at720p/4K. The current closed set of English messages is not
universal localization, arbitrarily long text or every-aspect-ratio acceptance.
No smaller fonts, hidden messages, shorter lifetimes, modal focus or moving
book rows are used. Offscreen captures do not prove physical scanout, tearing
improvement or subjective controller comfort.

## Verified focused outcome

The final fresh package passed49 focused checks at1280x720 and49 at3840x2160.
All26 actual active-feedback captures retain full drawn message text inside
the backing/viewport, without intersecting protected book/Look panels, their
title/tabs/selection/footer, calendar, meters or planning/context areas.
Their current English messages fit one line at the retained900-unit width;
this is not evidence for arbitrarily long translations. The complete-line
wrapping path is used without silently capping toast content.

The two visual-review sets are complete: four original active baseline frames
and26 final active-state frames. No spacing/cosmetic revision followed.
Actual frames disclose a pre-existing F5 debug-binding collision: saving also
selects ShaderComplexity, in both baseline and fixed layout. These are native
Canvas UI proofs, **not normal-Lit world-lighting or performance acceptance**.
See `debug-hotkey-evidence.md` for exact logs and historical qualifications.

A read-only renderer trace was briefly drafted after coordinator follow-up,
then removed unbuilt when that instrumentation was deferred to the next slice.
The three touched native files exactly match the final game's embedded
CodeView source checksums; file timestamps were not forged to hide the edit.
There was no extra build or visual batch for that deferred diagnostic.

Retained gates on this same executable passed106 clarity,70 prompt,112 VSync,
308 routing/input,40 launcher and914 gameplay checks (188clearing,74gathering,
181watering,37weeding,434full-loop). The full loop rendered its UI at native4K
with requested/executed primary100; after F5 its logged ShaderComplexity means
its timing is not normal-Lit performance evidence. All124 character/source-art
hashes, five restored owned world resaves and nine protected original/selected
preview/default files matched. No production input/graphics settings were changed.

Final executable SHA256:
`8A906D0DA8F56B65B52A58DCEB5DCB56771BBDCF2F5F3AA5BBAF437C69219E79`.
Proofs and checkpoint are sealed in the candidate's `Verification` directory and
`acceptance-receipt.json`. The known hotkey collision holds preview promotion
for a separately reviewed follow-up; the active preview was not changed.
