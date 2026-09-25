# Design

## Context

All five findings came from one Play-In-Editor session driven through the editor MCP play toolset
(`homestead_agent.toolset.HomesteadPlayTools`, see `.github/skills/unreal-editor-mcp/SKILL.md`).
That same harness reproduces each bug with real simulated controller input and editor captures,
so it is the default verification loop. Packaged Shipping captures are the acceptance check.

Observed code entry points:

- **Settings skipping:** `SHomesteadMenu::Columns()` returns 2 for page 4 (Settings), and
  `NavigateDirection` moves Content focus with `MoveWithin(..., Columns(), ...)`. Content cells are
  placed with `Grid->AddSlot(Index % Columns(), Index / Columns())`, yet the captured Settings page
  showed one stacked column. The logical grid and the drawn layout disagree.
- **Feedback reflow:** the book feedback banner is an `SVerticalBox` `AutoHeight` slot, a 62 px
  `SBox` toggled between `Visible` and `Collapsed` by `Controller->Toast()`, directly above the
  page content in `SHomesteadMenu`. Toggling it shifts everything below.
- **Competing lights:** `AHomesteadWorld` creates `MeadowSun` and `MeadowMoonlight` as
  `UDirectionalLightComponent`s, both with `bAtmosphereSunLight = true` (indices 0 and 1). Neither
  sets `ForwardShadingPriority`.
- **Felled tree:** mature-tree clearing is filtered out of the tree instances once `Node.cleared`
  is true (`HomesteadWorld.cpp`, the ForestTree rebuild loop). The chop animation
  (`AN_Heroine_Chop`, `HomesteadCharacter`) plays after the transaction commits, so the tree can
  vanish before the swing lands.
- **Creek water:** the water surface and its material readiness are handled in
  `HomesteadGeneratedVisual.cpp`. The session's first captures showed the editor still
  "Preparing Shaders", so a fallback material has not been ruled out.

## Goals / Non-Goals

**Goals:**
- Fix each finding in the smallest owning file set, using existing engine and game facilities.
- Keep every fix independently deliverable and verifiable.

**Non-Goals:**
- No save-format, simulation-rule, recipe, input-binding or menu-structure redesign.
- No new tree, water or lighting assets unless investigation shows the current ones can't meet the
  spec.
- The in-flight `improve-menu-directional-navigation` work is not absorbed here. This change only
  corrects the Settings step size.

## Decisions

1. **Settings: make logical columns match the drawn list.** Settings is specified as a single
  vertical list (`settings-screen-navigation`), so `Columns()` should return 1 for the Settings
  page, with the grid placement following it. Alternative considered: keep 2 columns and make the
  layout draw two columns. Rejected because it contradicts the existing vertical-list requirement.
  First confirm whether page 6 (the other page returning 2) intentionally uses two columns before
  changing that page.
2. **Feedback: reserve or overlay, never collapse.** Replace the `Collapsed` toggle with a
  fixed-height reserved row that is `Hidden` when empty, or move the banner into an overlay layer
  over the existing header band (the approach `feedback-layout-01` used for the HUD). Pick the one
  that keeps 720p content fully visible; overlay is preferred if reserving 62 px squeezes 720p.
3. **Lights: set explicit forward-shading priority and switch ownership with time of day.** Give
  the currently dominant light the higher `ForwardShadingPriority` (and atmosphere index 0) and
  swap at the existing day/night crossover, instead of deleting the moon. Alternative: a single
  directional light retinted for night. Rejected for now because it changes the approved sky look.
4. **Felled tree: delay the visual removal to the chop impact, then show a fall or stump.** Keep
  the simulation transaction immediate. Hide the instance at the animation's impact notify, and
  reuse existing mesh pieces for a short scripted topple or a stump instance. No physics simulation
  and no gameplay collision on debris.
5. **Creek: diagnose before changing materials.** Re-check in a packaged Shipping build and in PIE
  after shaders finish compiling. If it's a fallback, fix readiness gating. If not, adjust the
  existing water material (depth tint, fresnel/specular, edge fade) within the
  `woodland-creek-presentation` constraints.

Independent lanes (no shared files): A Settings columns (`SHomesteadMenu.cpp` navigation);
B feedback banner (`SHomesteadMenu.cpp` layout). A and B share a file, so sequence them or merge
carefully. C lights (`HomesteadWorld.cpp` lighting setup); D felled tree (`HomesteadWorld.cpp` tree
instances plus `HomesteadCharacter` notify), which also overlaps C's file, so sequence them;
E creek (`HomesteadGeneratedVisual.cpp` and the water material).

## Risks / Trade-offs

- [Changing `Columns()` affects every Settings navigation path] → re-run the existing native menu
  and directional-navigation tests plus an MCP walk down every Settings row.
- [Delayed tree removal could leave a frame where a cleared tree still blocks movement] → remove
  collision immediately with the transaction; only the visual lingers.
- [Light priority swap could pop at dusk] → swap at the existing crossover where both intensities
  are low, and capture the transition.
- [The creek finding may be an editor-only artifact] → task 5.1 decides whether any change is
  needed; closing it as not reproducible in Shipping is an acceptable outcome if recorded.
