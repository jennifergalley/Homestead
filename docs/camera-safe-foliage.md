# Camera-safe foliage

Homestead foliage never blocks `ECC_Camera`. Solid terrain, rocks, structures,
and generated mature-tree capsule proxies retain their existing camera
collision. This separation prevents a leafy sapling render mesh from collapsing
the spring arm into the heroine while preserving real trunk avoidance.

Camera-sensitive material slots use project-owned assets under
`/Game/SurvivalGame/Environment/CameraSafeFoliage`:

- `MPC_CameraSafeFoliage` stores the actual post-spring-arm camera position,
  heroine upper-torso target, and bounded fade radii.
- `MF_CameraSafeFoliageVisibility` computes distance to the finite
  camera-to-hero segment plus a near-camera sphere.
- masked and opaque camera-safe masters use stable world-space dithering.
- exact instances preserve the admitted diffuse, normal, roughness, AO, and
  alpha textures for grass, ferns, flowers, shrubs, sapling surfaces, young-tree
  leaves, mature-fir twigs, jacaranda leaves, and intermediate-fir twigs.

`AHomesteadWorld::ApplyCameraSafeFoliageMaterials` is the single slot contract.
It assigns only classified foliage slots. Broadleaf branches/trunks,
mature-fir bark/trunk, jacaranda branches/trunk, rocks, soil, and structures
keep their incumbent materials. A missing wrapper or changed required slot
stops world preparation with an explicit error.

The heroine updates the collection once in `TG_PostUpdateWork` from the real
camera component and gameplay target. Appearance portrait mode freezes those
values so portrait capture cannot move the world foliage corridor.

## Verification

The focused clearing route proves sapling render components have no camera
query collision through gather, checkpoint reload, permanent clear, save, and
reload. The mature-tree route proves the invisible solid capsule still blocks
the pawn and camera while leaf slots dither independently.

The Shipping ordinary traversal validates 25 loaded chunks with exact grass,
fern, flower, active-tree, and outer-tree material contracts, then performs
mapped forage, orbit/zoom, trunk contact, and retreat at 720p, 1080p, and 4K.
Generated producer/consumer routes cover chunk churn, boundary traversal,
clear/build/till/water, and save/reload. Full-loop evidence covers night, rain,
gardening, shelter, failure, and recovery.

Stable world-space dither was selected instead of frame-varying noise to avoid
shimmering or a temporal ghost trail. Automated evidence does not replace
human camera-feel or aesthetic review.
