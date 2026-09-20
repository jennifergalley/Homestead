# Jenny's playtest feedback

## Current priority

Character appearance and robotic movement need an observed gameplay diagnosis
first. MetaHuman is not a settled next step. Use the current build's normal
movement and interaction recordings as a basis for improvements.

## Safe review entry point (2026-09-20)

`Preview.cmd` exposes the separately verified movement/action candidate with
persistent isolated preview saves, rather than replacing `Play.cmd` or copying
Jenny's original world. `Preview.json` explicitly selects the reviewed package.
This is launch/save-safety integration, not another visual redesign or approval
of the heroine. The rejected face/hair trials remain excluded; mid-back waves,
the blonde-bob request and the UI feedback below remain unmet/deferred.

## Wild gathering - first motion slice (2026-09-20)

The separately packaged `gathering-01` adds a short restrained reach/pick/recover
after successful wild gathering, with movement interruption and clean recovery.
Actual ordinary berry gathering now visibly changes pose instead of remaining
static. This is technical/action-presentation progress, not Jenny's approval:
the gesture is generic, exact fingertip contact is not proven, and rigid hair,
fixed face and close-fitting clothes remain. See `docs\visual-playtesting.md`
for the two-pass review and packaged lifecycle/full-loop evidence.

No further cosmetic experiment was folded into this slice. Both requested
mid-back wavy length and the blonde-bob redesign remain unmet/deferred, as do
the UI/rendering feedback entries below.

## Watering - first motion slice (2026-09-20)

The separately scheduled `watering-01` slice adds one contextual can lift/tilt/
recover after a successful watering transaction. Its fresh-start ordinary route
and packaged checks are documented in `docs\visual-playtesting.md`. The can is
original wood/fiber presentation matching the existing recipe, not a new tool
or inventory rule. This does not complete the other interaction gestures or
resolve any deferred cosmetic/UI feedback.

## Weeding - reused motion feedback (2026-09-20)

The separately scheduled `weeding-01` slice reuses the accepted picking clip
after successful planted-plot X/F weeding. Actual walking and interaction were
recorded from a clearly disclosed copy of an existing test-world save; the
prior setup used functional teleports and ordinary sleep to grow weeds.
The final footage shows a gentle dip/pull and return to idle rather than a
static transaction. No new clip, prop, appearance edit or gardening rule was
added. This is generic feedback above the plot, not hand-to-ground contact:
weeds disappear at the existing immediate transaction. It is not Jenny's
approval or a fix for the remaining cosmetic/UI requests.

## Sapling clearing - contextual hatchet feedback (2026-09-20)

The separate `clearing-01` slice adds one restrained lift/swing/recover and a
small original wood/stone/fiber hatchet only after successful permanent sapling
clearing. Fresh ordinary controls gathered supplies, crafted the existing
hatchet and approached/cleared an actual sapling. The final full-body recording
shows the tool arc and recovery without gross new clipping. This remains a
generic gesture after immediate sapling disappearance, not synchronized impact,
realistic tree felling, combat or exact hand/tool contact. Other clearing/input
contexts and all gameplay costs/rewards remain unchanged. Existing cosmetic/UI
feedback is still unresolved; this is not Jenny's aesthetic approval.

## Wavy length - attempted and reverted (2026-09-20)

Jenny's wavy-hair length feedback was scheduled as its own bounded slice:
"the wavy hairstyle is too long - I'm thinking more the middle of her back."
Only the existing long-wave length across all three bodies and both outfits is
in scope. The packaged trial shortened the tips by about 22cm, but compressed
the lower waves into obvious accordion-like ridges and a blunt/frayed shelf.
It failed the final visual gate despite passing technical/functional checks.
The coordinator independently inspected the comparison and agreed to recovery.

**Not completed: the original overlong hair is restored, and the mid-back
request remains unmet.** No third sculpt/review pass was attempted. Only useful
back/three-quarter diagnostics are retained; brunette color, crown/framing,
other styles, face, materials, rig and locomotion remain at the accepted baseline.
The blonde-bob redesign and other remaining deferred entries below remain deferred.
See `docs\visual-playtesting.md` for rejection and recovery evidence.

## Controller prompt switching - implemented and technically verified (2026-09-20)

The coordinator has explicitly scheduled only this later-feedback item as
`prompts-01`: interaction labels revert to keyboard/mouse while using a controller.
Investigate accepted engine events and shared device-intent state, preserving
actual input, existing layout/text/mappings and the working `review-01` launcher
selection. This does not schedule tearing, book/recipe/inventory redesign or
further cosmetic work.

An isolated engine fixture reproduced the bug on the unchanged runtime:
accepted mouse-X input of `0.01` changed the paused Notes footer from controller
to keyboard labels. `MouseLook` wrote the device flag before checking whether
that screen allowed camera motion; movement and stick-look callbacks also
overwrote the same state. This is distinct from test-only physical-input isolation.

The fix uses one typed accepted-event classifier for the existing shared flag.
Buttons and fresh controller gestures switch immediately; releases/repeats and
zero/noise axes do not. Stick intent honors the inherited per-axis shaping and
existing radial modifier. Mouse intent uses one raw input unit of signed travel
within 120ms, allowing fine fractional motion without accumulating idle noise
forever. Already-held sticks cannot undo deliberate keyboard/mouse intent for
200ms. These rules affect hints only: input dispatch, movement/camera scales,
control deadzones, mappings, layout and glyph text are unchanged.

Both editor and fresh `prompts-01` package pass 70 focused checks, including the
same previously failing sequence and actual fine camera response. The final
rendered comparison was inspected: Notes/context/Settings/Look/planning hints
agree with deliberate use of both devices. All 914 previous action/full-loop
assertions, 308 save/normal-input checks and 40 launcher guards pass. `Preview.json`
was kept on `review-01` during that work; the coordinator subsequently accepted
and explicitly selected `prompts-01` in separate checkpoint `455c906`.
This reproduces and fixes a concrete cause, not every possible hardware/driver/
large-cursor-warp case or Jenny's physical-controller review.

## Inventory versus recipe clarity - implemented and technically verified (2026-09-20)

Jenny reported that it is difficult to tell what is actually carried versus what
belongs to the field book. The coordinator scheduled only this distinction as
`book-clarity-01`, not gradual recipe learning or a navigation/inventory redesign.
Original native Pack/Craft/Build captures at 1280x720 and 3840x2160 show identical
item-style rows, a generic "Field book" title and "use" action even for an inert
carried knife. The bounded correction labels carried/chest counts, required
materials, page purpose and actual eat/take/craft/plan actions in existing panels.
All recipes remain available; no learning/unlock claim or economy change is made.
The accepted `prompts-01` preview selection stays unchanged during this work.

The editor and fresh candidate pass 106 focused checks; the packaged fixture
passes independently at both 1280x720 and 3840x2160/render100. It verifies actual
counts, complete authoritative requirements, failed/successful crafting, menu
pause, keyboard/controller navigation and food/storage actions, and a genuinely
empty pack after transferring everything into a crafted chest. Functional setup
uses disclosed test teleports, not injected inventory or ordinary-play claims.
All 70 prompt, 914 action/full-loop, 308 save-routing/input, 40 launcher and 21
run/path guards remain intact and pass. Two visual-review sets only: original
baseline and final native book states; 20 final frames fit measured Canvas text
bounds without observed clipping. The existing transient toast can briefly cover
the heading; stable captures wait for its expiry while the world stays paused.
This does not resolve every source of field-book overwhelm, introduce gradual
recipe knowledge, or claim Jenny's comprehension/comfort approval.

## Movement tearing/flicker - investigated, unresolved (2026-09-20)

The coordinator explicitly scheduled investigation only as
`presentation-diagnostics-01`, distinct from the rejected facial-presentation
experiment. Two offscreen game-only motion batches record live renderer/settings,
screenshot-free tick timing, and sampled frames. They do not observe physical
scanout, actual DXGI Present calls, DWM composition or VRR engagement.

Read-only Windows queries independently report 3840x2160 at 30Hz; the diagnostic
runtime reports D3D12, TSR, VSync off and a 60fps cap. These are facts to investigate,
not a proven tearing cause. No repeatable whole-frame horizontal discontinuity
was isolated in the inspected sampled frames; sparse captures cannot rule out
brief GPU-rendered flicker. No graphics, exposure, shadow, AA or OS setting was
changed. Full evidence and the bounded human follow-up are documented in
`presentation-diagnostics.md`. Accepted `book-clarity-01` remains selected.

## Optional vertical sync - option added, tearing unresolved (2026-09-20)

The coordinator separately authorized `video-sync-01`: one controller/keyboard
toggle at the end of Settings. Existing Off preferences/defaults stay Off.
The copy explains that sync may reduce tearing but can add input delay and does
not fix every flicker. It reports an engine override instead of pretending the
requested preference is active. This is a reversible comparison aid, not a
rendering fix, display-mode change, or human scanout acceptance.

The coordinator subsequently accepted and explicitly selected `video-sync-01`
in `d7d0a02`, keeping `jenny-review` and Off. This accepts the optional control,
not a tearing cure.
Graphics settings belong to the game's Unreal configuration, not individual
preview save profiles. See `setup.md` for operation and isolated verification.

## Sustained autonomous testing - bounded evidence (2026-09-20)

The explicitly scheduled `endurance-01` diagnostic completed one45-minute
ordinary-control exercise after short sanity/cancellation checks. It was99.62%
unpaused, advanced17.93natural game hours through night/morning, completed
5gathers/4eats/402waypoints and verified manual/load/autosave integrity without
navigation failures. It used a disclosed prepared test-world copy, not a fresh
start or personal save. No time/needs/inventory resets or compressed simulation
were used. See `endurance-playtesting.md` for fixed criteria, actual memory/timing
ranges and limits. Resource regrowth, indefinite stability and physical tearing
were not proven; no gameplay/art/graphics changes or preview promotion followed.

The subsequent explicitly scheduled `forage-renewal-01` closes the selected wild
forage coverage gap through three normal8-hour bedrests, not hidden time edits.
Actual branch8/flower12/berry10 produce and cooldown hints renew, give one correct
reward and deplete again; exact save/load and another process retain cooldowns.
Cleared sapling14 remains absent/nonblocking. Existing game behavior passed;
only diagnostic-driver issues were corrected. See `forage-renewal-playtesting.md`
for the retained failed attempts, actual images and limits. The selected human
preview remains video-sync-01; broader feedback is not implicitly resolved.

## Active toast versus book heading - bounded correction (2026-09-20)

The coordinator scheduled the specific transient overlap documented during
book/VSync verification. Real F5 success and craft rejection reproduced covered
headings at720p/4K before editing. `feedback-layout-01` moves the existing feedback
affordance into the free upper-right band while the book/Look is open, without
moving tabs/rows/footer or changing fonts, messages, colors, input or lifetime.
World/planning placement remains unchanged; feedback draws complete wrapped lines.

The fresh candidate passes49 focused checks at each size with26 actual active
frames, including real graphics-write failure, actual backup-recovery feedback,
replacement and normal expiry while menus pause simulation.
This is not broad field-book-overwhelm resolution or progressive recipe learning.
See `feedback-layout-playtesting.md`; preview selection awaits coordinator review.

These baseline/final captures also expose a pre-existing F5/debug-render collision.
`debug-hotkey-evidence.md` appends precise qualifications to prior endurance,
renewal and full-loop rendering/performance evidence without changing sealed
proofs or invalidating unrelated state/geometry results. Dedicated face/hair and
ordinary-motion runs without logged transitions are explicitly distinguished.
The binding fix is separate; no renderer state or installed engine was changed.

## Save/load changes rendering - concrete hotkey correction (2026-09-20)

The separately authorized `hotkey-safety-01` removes only inherited F5
ShaderComplexity and F9 screenshot debug commands at the project-config layer.
Actual before/after instrumentation confirms the baseline's duplicate meanings
and the corrected save/load-only behavior, including errors, repeat actions,
exact saved-world/appearance reload and normal human-preview startup. Supported
deliberate non-Lit choices remain intact; no forced Lit conceals the collision.
Fresh4K full-homestead verification stays Lit throughout13,210 observed ticks.
See `hotkey-safety-playtesting.md`. The older selected video-sync package remains
unchanged until review; no tearing, appearance or broad performance fix is claimed.

## Deferred feedback - 2026-09-19

Jenny explicitly marked the following as feedback for later, not an instruction
to change these systems immediately.

| Area | Report | Follow-up when scheduled |
| --- | --- | --- |
| Music and ambience | "Great." | Preserve this as a successful baseline; avoid unnecessary replacement or remixing during unrelated work. |
| Field book / recipes | The book feels overwhelming; learning recipes gradually might be better than exposing all recipes immediately. | Explore progressive disclosure or a learn/unlock flow. This is a proposal, not a confirmed progression design; essential opening-survival actions must remain attainable. |
| Bob hairstyle | The intended straight blonde bob is closer to Melinoe's haircut in Hades II, not the current "karen hairstyle." | Deferred visual direction for that alternative only; preserve the long brown-haired default. Inspect the reference before specifying cut details. Author an original interpretation, not a copy or import of game assets. |

The remaining table entries are still deferred; the attempted wavy-length
correction does not mean that this or all other feedback has been fixed.
