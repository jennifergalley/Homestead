# Save/load hotkeys and inherited debug bindings

## Confirmed conflict, not a graphics fix

During `feedback-layout-01`, mapped F5 both saved the synthetic world and logged
`Set new viewmode: ShaderComplexity`. The same transition occurred in its
unchanged-layout baseline. This explains the flat shading/black sky in those
feedback frames; it is not evidence of a lighting regression from toast layout.

Read-only inspection of UE5.8's `Engine\Config\BaseInput.ini` found:

```ini
+DebugExecBindings=(Key=F5,Command="viewmode shadercomplexity")
+DebugExecBindings=(Key=F9,Command="shot showui")
```

The game's `HomesteadController.cpp` binds F5 to QuickSave and F9 to QuickLoad.
Its input override classifies intent then forwards to `Super::InputKey`.
Engine `UPlayerInput::InputKey` records key state/events and, in non-Shipping
builds, resolves `GetBind` and executes inherited debug commands. The inherited
debug-command path can therefore run as well as the mapped game action.
`UGameViewportClient::HandleViewModeCommand` sets the requested mode and logs
the observed transition. Packages through `feedback-layout-01` are affected. The separate
`hotkey-safety-01` correction is documented below.

Current direct proof:
`Saved\Automation\20260920-050723-5cc6c8a5\feedback-layout-final\720\engine.log`,
first transition at2026-09-20T15:48:30.082Z immediately after `INPUT F5`;
the actual save and active feedback checks passed. The4K log agrees.

F9's conflicting inherited screenshot command is established in engine config
and its dispatch path. Exact before/after viewport ShowFlags and screenshot
request-state instrumentation is **deferred to the separately authorized
hotkey-collision slice**. This UI slice does not claim those measurements.
It does not force Lit, disable debug bindings, change graphics defaults, edit
the installed engine, or remove historical F9 screenshots.

## Appended historical qualification, 2026-09-20

This qualifies earlier evidence without changing its sealed files or erasing
its original observations. `Scripts\Audit-PresentationEvidence.py` read existing
logs and original capture timestamps; the final scoped report covers112 relevant
logs. It records log hashes and separates capture-request ordering from weaker
file-write correlations. Multiple-process folders are filtered to each log's
actual timestamp range. No logged transition is not universal renderer proof.

| Evidence | What the existing logs actually establish |
| --- | --- |
| Dedicated face comparison | `presentation-before`, `presentation-before-settled`, and `presentation-01-portraits`: no logged ShaderComplexity transition in their five-frame comparison runs. The separate148-frame ordinary presentation route also has none. Do not retroactively blame the rejected face experiment on this hotkey. |
| Dedicated hair comparison | `hair-length-before` and `hair-length-01-portraits`: no transition in either12-frame set. The144-frame ordinary hair route has none. Separate packaged smoke/recovery runs switch later; only `first-foundation.png` follows the transition in those ten-image smoke sets. |
| Ordinary motion and tearing diagnostics | Older290-frame route, setup302-frame route, presentation diagnostics17/105frames and271-frame ordinary regression: no logged ShaderComplexity transition. Their existing sampling/scanout limits remain, not blanket invalidation. |
| Book clarity and VSync focused UI | Both ten-frame book-clarity sets and both five-frame VSync sets have no logged transition. Their separate retained full-loop runs do switch later. |
|45-minute endurance | First transition13:39:44.977Z, about35seconds after the first milestone. `milestone-00` precedes it; milestones01-04 follow by565-2365seconds. All post60-second-warmup timing/memory samples occurred after the transition. They remain measurements of that actual process, **not normal-Lit gameplay performance**. |
| Wild renewal | Write process first switches at15:05:15.150Z. Early branch/flower images precede it; early berry image was written0.286seconds before it and the driver requests all three early images before saving. Images03-09 follow by70-107seconds. The seven later images cannot attribute their flat shading solely to changed day/rain. |
| Full-loop family | In the audited full loops, the initial clearing/book/heroine captures precede F5. `first-foundation`, `shelter-night`, `berry-garden`, `garden`, and `failure-retry` follow a logged ShaderComplexity transition. Timing spans mixed rendering conditions and is not a normal-Lit performance benchmark. |
| Current toast baseline/fixed | Real F5 occurs before every focused capture; both sets share the disclosed ShaderComplexity condition. Native Canvas geometry, colors, full text and lifecycle checks remain useful. These are not world-lighting acceptance images. |

The renewal produce-component counts, visibility/collision checks, cooldowns,
single rewards and persistence assertions remain evidence of those behaviors.
Endurance wall time, simulation progression, actions, save integrity, bounded
stability and measured resource counts likewise remain observed outcomes.
ShaderComplexity changes their rendering/performance interpretation, not their
recorded game-state results.

No causal claim is made about earlier resource-focus failures, berry maturation
failures, rejected art, physical tearing, or the endurance working-set reduction.
The new finding is a specific inherited hotkey collision, not a universal
explanation for prior defects.

Audit proof:
`Saved\Automation\20260920-050723-5cc6c8a5\feedback-layout-binding-audit\historical-log-audit-scoped.json`.
The earlier unscoped exploratory report is not the final frame-association proof.

## Subsequent hotkey-safety runtime confirmation

The separate `hotkey-safety-01` baseline now observes the exact immediate and
delayed state: F5 changes actual mode3 to8 and three ShowFlags while saving once;
F9 requests and writes a screenshot while loading once, even on missing-save
error. The fixed project removes only the two conflicting inherited entries.
Actual bindings F2/F3 remain unchanged, F5/F9 debug bindings become empty, and
repeated keyboard/controller save/load preserve full flags with zero screenshots.

The first fixed fixture deliberately requested Unlit, which this engine build
rejects without `AllowDebugViewmodes()`. The strict assertion failed and remains
preserved. `HandleViewModeCommand` logs its request before `SetViewMode` applies
policy, so a requested-mode log alone is not final-mode proof. The corrected
witness explicitly chooses supported ShaderComplexity and requires that mode
and full flags survive saves/loads; no graphics policy is changed.

Fresh-process saved-profile loading and the fresh4K homestead start and remain
Lit as observed directly. See `hotkey-safety-playtesting.md` for reproduction and
limits. This new evidence does not alter any historical capture, sealed receipt,
timing result or qualification above, and is not a new endurance run.
