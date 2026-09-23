# Proposal

## Why

Jenny captured foliage forcing or occupying the third-person camera until the view was almost completely blocked. Current young sapling visual meshes block the camera channel as one whole leafy mesh, while other no-collision foliage can render directly between the heroine and camera; collision alone therefore either compresses the spring arm into the character or leaves opaque vegetation across the view.

## What Changes

- Make leaves, grass, ferns, flowers, shrubs, reeds, and other non-solid foliage ignore camera collision so they cannot push the spring arm into the heroine.
- Keep solid terrain, trunks, rocks, structures, and intentionally solid obstacles camera-blocking so the camera still avoids physical geometry.
- Add a project-owned camera-safe foliage visibility treatment: foliage close to the camera or inside a bounded capsule between camera and heroine fades/dithers out smoothly rather than filling the frame.
- Apply the same policy to resource meshes, generated tree leaf/twig material slots, HISM groundcover, creek-bank cover, future decorative wildflowers, and near/far visual rings without hiding trunks or changing world collision/navigation.
- Update camera/target positions for the material policy every frame from the real spring-arm result. Missing fade-capable materials MUST log explicitly and fall back to camera-ignore rather than restoring whole-mesh camera blocking.
- Preserve visual density outside the camera corridor, shadows beyond the fade zone where practical, resource focus/rewards, deterministic generation, save state, LOD/culling, walkability, performance, and all existing material licensing/provenance.
- Add ordinary close-canopy/sapling/orbit/zoom/slope evidence proving the heroine and target remain readable with no full-screen foliage or clothing clip caused by foliage compression.

The private playtest screenshot is evidence only and will not be copied into the repository. Existing Unreal camera channels, spring arm, material parameter collections, masked-material dither, world-position math, authored foliage textures/materials, and current camera lifecycle tests provide the implementation path; no plugin or external asset is required.

The smallest useful in-game result removes the sapling whole-mesh camera block that reproduced the failure and prevents camera compression into the heroine. The first playable demonstration orbits and zooms through dense sapling/fern/grass/tree-canopy positions while near-camera and camera-to-hero foliage clears smoothly, trunks still constrain the camera, and the target remains visible. Full acceptance covers every admitted foliage family, generated/resource/HISM ownership, active-window rebuilds, rain/day/night, 720p/4K, save/reload, LOD/view-distance compatibility, cadence, and immutable Shipping replay.

Deferred scope includes X-ray outlines, making trunks/walls transparent, indoor room-cutaway systems, over-the-shoulder camera redesign, manual first-person view, cinematic cameras, or hiding the heroine when a genuinely solid obstacle compresses the camera.

## Capabilities

### New Capabilities

- `camera-safe-foliage`: Camera-channel and visibility policy that prevents decorative/leafy vegetation from compressing or obscuring the gameplay camera while preserving solid obstacles and world behavior.

### Modified Capabilities

None.

## Impact

- `HomesteadWorld` resource/tree/cover component and material assignment: foliage camera-ignore classification and fade-capable project materials for admitted textures.
- `HomesteadCharacter`/camera update path plus a project material-parameter collection: real camera/heroine corridor data.
- Existing imported CC0 meshes/textures remain unchanged; project-owned material wrappers may reuse them under their current license/attribution.
- Camera lifecycle, clearing, generated-world, visual, material, performance and Shipping tests: no full-screen foliage, retained solid collision, fade coverage, rebuild cleanup and cadence.
- Coordinate with `upgrade-woodland-environment-assets`, `polish-locomotion-view-distance-and-time-hud`, and `refine-settings-and-wildflower-groundcover` so future outer rings and decorative flowers inherit the same camera-safe classification.

