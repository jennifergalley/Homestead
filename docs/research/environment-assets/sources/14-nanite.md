# S14 - Epic Nanite Virtualized Geometry

URL: https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine
Accessed: 2026-09-20 by the counterevidence worker.
Type: primary engine reference.

## Short verbatim evidence

- "Nanite supports materials that have their **Blend Mode** set to **Opaque** and **Masked**."
- "While deformation with **World Position Offset** (WPO) in a material is supported, it is limited."

## Interpretation and limits

Masked foliage and WPO are supported, not automatically cheap. ISM/HISM support
does not certify a particular imported tree's Nanite settings, bounds, shading
or simplification. Compare the actual representative mesh with conventional LODs
before adopting a path. Do not extrapolate showcase geometry savings to total
texture/shadow/game VRAM.

## Assessment

Credibility: high for documented behavior. Recency: rolling documentation,
not verified 5.8.2 execution. Bias: vendor/showcase, older cross-links present.
