# Design

## Context

See `proposal.md` for motivation. The Canvas HUD currently renders a 105-unit context panel for every gameplay frame, even when focus is `None`; controller then supplies `Woodland` plus a default till/field-book hint. A separate top-right shortcut strip is also always visible. Successful gathers already use the transient toast system, but Simulation emits prose such as `Gathered Fallen branches. This patch will regrow.` rather than exact inventory deltas.

Generated fallen branches currently share `Harvest` renewal behavior with forage: the patch receives a future `readyAtHour` and a sparse resource edit. The same edit structure already stores permanent `cleared` state, so no new save field is needed.

Locomotion uses one Enhanced Input movement action, a 180 cm/s movement component, and an animation proxy blending idle and one authored walk cycle before the action overlay. Energy is already authoritative, saved, restored by sleep, and shown on the HUD. The retained heroine skeleton, analytical Blender scripts, trial import workflow, and serialized Editor/package process can produce one original sprint clip without an external asset search.

## Goals / Non-Goals

**Goals:**

- Remove guidance that is not tied to an actual nearby interaction.
- Turn successful gathers into concise, exact inventory feedback while keeping errors useful.
- Let ordinary world-action rejection guidance clear sooner than critical system failures.
- Replace the wide labeled survival-meter row with a compact lower-left icon stack.
- Make fallen branches finite and durably absent after pickup.
- Add responsive Shift/L3 sprint with distinct animation, faster movement, and safe existing-Energy cost.
- Preserve simulation authority, current saves, collision, prompt-device switching, HUD readability, and the one-action presentation contract.

**Non-Goals:**

- A general HUD/theme redesign or removal/rebalancing of survival meters.
- Hiding actionable missing-tool or other rejection guidance when the player is actually near a target.
- A new regenerating stamina stat, save schema, meter, consumable, perk, encumbrance, or exhaustion animation.
- Root-motion locomotion, motion matching, parkour, crouch, dodge, climb, particles, or new sprint audio.

## Decisions

### 1. Render interaction UI only for real focus

Controller will expose whether focus is anything other than `None`. The HUD will create the context panel only in that state and will stop using fallback `Woodland`/till/menu copy. The general top-right field-book/camera shortcut strip will be removed entirely. Planning retains its dedicated panel, and menus, failure, toasts, meters, and calendar remain. Preview-label placement is owned by `refine-settings-and-wildflower-groundcover`.

Renewing/depleted resource patches will not remain actionable focus targets merely to explain renewal. Missing-tool targets may remain focused because the nearby requirement is useful and actionable.

**Alternative considered:** fade the always-on panels. Rejected because the request is absence outside context, not lower opacity.

### 2. Build pickup text from authoritative inventory deltas

Simulation will format successful gather results from the exact yield applied to the candidate inventory, producing concise `Added to pack` text for one or multiple item kinds. Controller continues using the existing toast lifecycle and sound/animation dispatch only after `Result.ok`. Error results keep their existing messages and longer error lifetime.

The renderer and feedback-layout fixture are reused. No second pickup widget, notification queue, icon framework, or generalized message bus is introduced.

**Alternative considered:** infer gains in HUD from focus/resource kind. Rejected because capacity, yield changes, multi-item forage, and future balance updates belong to simulation authority.

### 3. Classify toast lifetime by failure severity

`Notify` will accept or derive a feedback category instead of treating every `Error` identically. Ordinary world-action rejections use six seconds, versus the current eight; critical save, recovery, startup, and preference-persistence failures retain eight seconds. Successful pickup/action toasts retain their existing shorter lifetime. Tests assert category and expiry rather than matching arbitrary text fragments in HUD code.

**Alternative considered:** shorten every error globally. Rejected because save/recovery failures need enough time to read and act on.

### 4. Draw compact original survival icons with existing Canvas primitives

The HUD retains authoritative numeric values, fill ratios, colors, low-value threshold, and protected-region measurement. `Meter` changes to a compact horizontal bar prefixed by a small material-free icon, and the three meters are positioned as a fixed vertical lower-left stack. Food uses a simple bowl/food silhouette, Energy a lightning/leaf-like mark, and Warmth a flame/sun mark drawn from Canvas rectangles/lines so no asset, font glyph, dependency, or license is added. Measurement metadata keeps semantic names even though persistent label text is not rendered.

**Alternative considered:** import icon textures or use emoji/font glyphs. Rejected because existing Canvas primitives are deterministic, package-safe, themeable, and avoid platform-font or asset-provenance variance.

### 5. Fallen branches use existing permanent resource edits

`Harvest` special-cases only the fallen-branch resource kind after all current range, readiness, tool, capacity, and edit-limit validation. Its candidate resource is marked cleared with zero renewal deadline, then the normal atomic inventory commit stores one yield. Other resource kinds retain their current renewal deadlines. Existing visual/focus rebuild code already removes cleared generated keys across active-window churn and reload.

**Alternative considered:** set an extremely long renewal timer. Rejected because it is not permanent, communicates the wrong model, and complicates tests.

### 6. Sprint spends existing Energy with a reserve floor

Sprint speed will be 300 cm/s versus the retained 180 cm/s walk. While sprint is actually active, grounded, and moving, Simulation will spend 0.35 Energy per real second. Sprint is admitted only above 10 Energy and clamps/stops at that reserve, so sprint alone cannot create the `energy == 0` failed-state invariant. Walking and sleep remain available; existing saves already persist the resulting Energy.

The character owns held-input and physical sprint state; Simulation owns Energy mutation. No drain occurs for a held key while stationary, blocked, airborne, in menus/planning/failure, or during the hand-action overlay.

**Alternative considered:** a new fast-regenerating stamina pool. Deferred because it duplicates the visible Energy meter and adds tuning/save/UI scope before playtesting the smaller mechanic.

### 7. Add a dedicated sprint clip and locomotion blend

An original sprint cycle will be authored on the retained skeleton with forward lean, longer stride, stronger arm drive, planted contact phases, zero root travel, scale one, and zero gameplay notifies. Runtime loads the clip explicitly and extends locomotion to idle -> walk/sprint blending beneath the existing hand-action overlay. Movement velocity and explicit sprint state select the animation; clip geometry never moves the pawn or drains Energy.

The existing walk remains unchanged below sprint conditions. Sprint release, low Energy, menu/planning/failure, load, loss of ground, and action cancellation blend back without a queued replay or foot/root displacement.

### 8. Parallelize independent source and animation lanes

The HUD/simulation lane owns Simulation harvest/energy APIs, HUD context/toast policy, and portable/source tests. The animation lane owns sprint Blender source, verification, FBX, provenance, and fresh trial import. Character/controller/anim-proxy integration plus Editor, cook, package, and promotion are serialized after both contracts are stable. Compatible DDC/shader caches and the selected `work-actions-v13` rollback remain.

## Risks / Trade-offs

- **[Exact pickup toast becomes too long for multi-yield forage]** -> Reuse existing wrapping, retain concise item names, and verify real maximum yields at 720p/4K.
- **[Removing default hints harms discoverability]** -> Keep interaction hints when a target is actually focused; guidebook/menu onboarding remains available without permanent HUD chrome.
- **[Icon-only meters become ambiguous]** -> Use three visibly distinct silhouettes, stable order/color, retained numeric values, semantic measurement IDs, and ordinary first-look acceptance at 720p/4K.
- **[Shorter errors disappear before they are read]** -> Reduce only ordinary action rejection from eight to six seconds; keep critical failures at eight and verify maximum real copy length.
- **[Branch gather bypasses an atomic capacity check]** -> Apply cleared state only to the candidate after the existing full-yield capacity validation and commit once.
- **[Energy mutation every frame churns revisions or saves]** -> Use one bounded simulation API, consume only while active movement is confirmed, and test transaction/save behavior after exertion.
- **[Sprint animation slides or looks like a fast walk]** -> Author against 300 cm/s travel cadence, inspect side/three-quarter/gameplay views, and reject simple walk-speedup as completion.
- **[Shift conflicts with menu quantity modifiers]** -> Sprint input is ignored while menus are open; existing native-menu Shift handling remains local to the menu.
- **[L3 click conflicts with future controls]** -> Left-stick click is currently unused; retain right-stick click for camera distance and cover both bindings in prompt/input tests.

## Migration Plan

1. Record ordinary no-focus/focus/gather HUD and walk/Energy baselines in the selected build.
2. Deliver the smallest HUD, six-second world-action feedback, icon-meter stack, and pickup-message correction with portable/source tests.
3. Make fallen branches permanent and verify atomic save/revisit behavior.
4. Author, import, and integrate sprint input, Energy authority, speed, and dedicated animation.
5. Run controller/keyboard, HUD layout, lifecycle, persistence, full-loop, and ordinary visual routes in Editor.
6. Build one immutable Shipping candidate, run separate-process replay and comparable cadence checks, and promote only if HUD quietness and sprint readability are materially improved.
7. Retain `work-animation-complete-02-shipping / work-actions-v13` as rollback until promotion passes.
