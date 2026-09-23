# Design

## Context

See `proposal.md` and `specs/camera-safe-foliage/spec.md`. The spring arm uses a 12 cm probe on the camera channel. Solid generated tree collision proxies block that channel appropriately. However, authored young sapling resource components currently set the entire multi-material visual mesh to camera-blocking, so leaves can collapse the spring arm toward/inside the heroine. Other grass/fern/flower/shrub/HISM components use no camera collision, so their masked pixels can still render directly across the view.

The captured failure is consistent with these two incomplete strategies. Existing source already notes continuous camera-foliage handling as deferred. Unreal's material parameter collections, world-position material math, temporal dithered opacity masks, camera channels, imported texture identities, and camera lifecycle tests provide a bounded solution.

## Goals / Non-Goals

**Goals:**

- Remove foliage from camera collision and clear interfering foliage visually.
- Preserve solid obstacles, world density away from the camera corridor, authored textures, simulation authority, and deterministic generation.
- Establish one classification inherited by current and future foliage batches/rings.

**Non-Goals:**

- Fading trunks/walls, room cutaways, X-ray outlines, first-person camera, general camera redesign, or hiding the heroine under genuine solid compression.

## Decisions

### 1. Separate solid camera collision from foliage visibility

Remove `ECC_Camera` blocking from whole young-tree/resource visuals and assert all camera-sensitive foliage components ignore that channel. Retain camera-blocking hidden proxies for mature trunks/rocks and solid structure/terrain collision. Mixed meshes rely on solid proxies, not their leaf-bearing render component.

**Alternative considered:** make all vegetation camera-blocking. Rejected because dense leaves compress the spring arm into the heroine and cause the captured failure.

### 2. Use one project-owned foliage camera-fade material function

Create a material function and parameter collection containing actual camera position, heroine visibility target, near-camera radius, corridor radius, and fade-band width. In world space, compute distance to the finite camera-target segment plus a near-camera sphere. Multiply the existing authored alpha mask by a temporally dithered visibility value: clear inside the inner region, blend through a bounded band, remain unchanged outside.

Create project-owned camera-safe wrapper materials/material instances for each admitted foliage family, preserving their exact CC0 diffuse/normal/roughness/AO/alpha textures and two-sided/masked behavior. Apply the function only to leaf/twig/grass/fern/flower/shrub/reed slots; bark/trunk/rock/soil slots keep incumbent materials.

**Alternative considered:** hide an entire component/HISM batch when any part intersects the view. Rejected because one nearby instance would remove large woodland areas and pop during movement.

### 3. Feed real post-camera positions once per frame

After the gameplay camera resolves, update the collection from `PlayerCameraManager`'s actual camera location and a target near the heroine upper torso/interaction center. Freeze or disable updates while no gameplay camera exists; menu portrait capture uses separate show-only components/material policy and must not overwrite gameplay parameters.

Use centimeters consistently and clamp degenerate segment math when camera and target nearly coincide.

### 4. Make material classification explicit and testable

Centralize known foliage material-slot mappings/tags at creation rather than guessing from color or mesh name during every tick. Resource components, generated tree batches, groundcover, creek-bank cover, decorative wildflowers, and outer visual rings call the same material assignment helper. Creation fails explicitly for required foliage slots without an admitted wrapper.

Keep one diagnostic command/fixture to disable the fade for matched A/B frames, but expose no player setting that can reintroduce the defect.

### 5. Verify visual coverage and bounded cost

Source tests assert collision responses and material paths per family/slot. A native camera route measures desired/resolved arm, camera-to-hero corridor, faded material coverage, component lifecycle, and no state mutation. Ordinary Editor/Shipping routes orbit/zoom through focused saplings, shrubs, reeds, groundcover, creek banks, mature canopies and slopes in day/night/rain at 720p/4K.

Material/camera work owns no Simulation/save data. The foliage-material lane and camera-parameter lane can proceed independently behind the collection schema; Editor/cook/package remain serialized.

## Risks / Trade-offs

- **[Dither appears noisy/ghosted]** -> tune narrow bands under the project's TSR path and inspect motion/day/night at native resolution.
- **[Wrong material slot fades a trunk]** -> explicit per-mesh slot contracts and visual/source tests; never infer from broad component tags alone.
- **[Alpha wrapper changes authored appearance]** -> preserve exact texture inputs/two-sided/mask thresholds outside a fade region and compare matched frames.
- **[Material ALU/overdraw regresses dense woods]** -> one collection update, no per-instance CPU search, shader complexity/cadence evidence, bounded fade math.
- **[Camera updates lag one frame]** -> source parameters from final camera manager position and verify fast orbit/zoom without trailing occlusion.
- **[Unsupported foliage remains opaque]** -> explicit admission failure blocks promotion rather than falling back silently.

## Migration Plan

1. Reproduce the private screenshot route and inventory every camera-blocking/occluding foliage family/material slot.
2. Remove foliage camera blocking while retaining solid proxies; verify the camera no longer compresses into the heroine.
3. Build/assign camera-safe wrapper materials and update the actual camera/target collection.
4. Cover resource/generated/HISM/bank/decorative/far-ring creation/rebuild paths and run lifecycle/material tests.
5. Run ordinary visual, camera, clearing, save/reload, day/night/rain and matched cadence acceptance.
6. Build one immutable Shipping candidate and retain the current selected build as rollback until explicit promotion.

