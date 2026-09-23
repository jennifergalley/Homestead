# Design

## Context

The relevant UI/navigation/controller files are identical to committed main
`a8dee30a053c3bcbc34ee45441b1220b42f2d5ac`; do not replace newer camera, lifecycle
or common QA files. The current menu clamps `Step` at grid edges, cycles sections
only through `CycleRegion`, makes buttons nonfocusable, and rotates the portrait
with directional browsing input. That is the demonstrated gap.

The existing main working policy was read through Git objects: reuse first,
small playable increments, isolated ownership and one shared engine lane.
This round is authorized for conditional apply after strict plan validation.

## Goals / Non-Goals

Natural directional browsing and real synchronized focus, not a visual redesign.
No new plugin, CommonUI router, input-intent classifier, scene/asset changes,
inventory rule, save schema, supervisor or parallel engine process.

## Decisions

Reuse the installed Slate focus and navigation system: focusable buttons,
`FNavigationReply`/navigation metadata, native directional routing, and
`SetKeyboardFocus` plus `ScrollDescendantIntoView`. The native engine already
has spatial neighbor search; do not recreate a geometry-scoring framework.
Add only transparent focus anchors where informative/empty regions otherwise
have no focusable widget.

Keep logical grid movement in the existing portable helper so offscreen rows
remain reachable and desired columns survive short rows. When a true logical
edge is reached, let Slate navigate to adjacent visible controls within the
menu scope. Down from the carried grid leads to equipment, Up reverses toward
the grid/upper controls, Left/Right reach portrait/details according to layout.
Use explicit native boundary links only for a scroll container's adjacent
informational/actions regions when their clipping would hide the target.

Synchronize a semantic focus descriptor (region + stable subject/index) with the
real focused widget. Focus callbacks update selection but never execute it.
Capture menu key-downs at the parent preview stage so native button activation
cannot run alongside the existing controller handler. Preserve accepted-input
classification and physical-input isolation before routing. Analog events still
reach the same controller path; add a small reusable axis-to-direction/repeat
helper, not a second device detector.

Left-stick/D-pad/arrows only navigate. Portrait rotation stays on right stick
or explicitly activated portrait buttons. Rebuilds defer focus restoration to
valid new widgets after layout; item identity, not a stale row pointer, wins.
Empty content has an actual focus anchor. Outer scope and modal boundaries
stop focus escape into the world. Unavailable controls remain explanatory and
nonmutating; truly disabled/hidden widgets are skipped.

Quantity dialogs begin with the existing safe Cancel default and use a
conventional stepper composed of explicit minus, current value, and plus
affordances. When the stepper has focus, Left/Right changes only the draft;
Up/Down leaves it for Cancel/Confirm/One/All. Pointer minus/plus clicks perform
the same bounded adjustment. Confirm alone commits and Back cancels. This removes
the hidden edit-mode state and its explanatory paragraph rather than replacing
that paragraph with different instructions.

LB/RB keeps tab switching. Triggers and Tab can remain optional conveniences,
but no persistent footer advertises navigation, activation, tabs, or Back.
Remove the default `Choose a tab to manage your homestead` filler, the Guidebook
menu-controls lesson, and generic action-key prose. Keep only stateful content:
page identity, selected object details, values, Have/Need requirements, labeled
buttons/steppers, meaningful empty states, errors, confirmations, and concise
pause state. Contextual hotkey badges MAY remain inside an actual labeled action
button when they reduce ambiguity; they are not repeated in a global legend.
Preserve Settings initial Resume focus, Right to Quit, then Activate.

**Alternative considered:** shorten the existing footer and tutorial text.
Rejected because Jenny's direction is that intuitive navigation should make
generic onscreen explanation unnecessary; merely compressing it leaves the same
design failure and visual noise.

## Verification and integration

Use existing portable/native test facilities. Add real edge/short/empty-grid
and analog-repeat cases to the portable helper checks. Native tests must send
actual D-pad and left-stick events through the shared handler, inspect actual
Slate focus and selected IDs, and verify no inventory/pawn/portrait mutation
from navigation. Direct `CycleRegion` calls are not proof of directional flow.

Main owns native compilation and gameplay after its current endurance run has
exited and released resources. Review actual boundary transitions at 720p/4K,
including scroll, empty/full content, modal edit/cancel, mouse parity and the
quit path. Source/portable proof is not gameplay acceptance. Adapt old snake
test routes at true edges rather than preserving awkward UX for fixtures.

## Risks / Trade-offs

- Rebuilt widgets invalidate focus pointers -> stable semantic selection plus
  deferred native focus restoration.
- Native buttons might also handle Enter -> parent preview routing consumes
  accepted menu input once before child activation.
- Clipped scroll content can confuse spatial traversal -> logical grid movement
  first; scroll focused targets into view; scoped native boundary routing.
- Initial viewport geometry is not ready -> defer spatial focus work until
  valid layout, without inventing a hardcoded region-cycle substitute.
- Removing coaching exposes weak affordances -> verify first-look navigation
  through visible focus, grouping, controls and labels; repair the affordance
  rather than restoring instructions when a route is unclear.
