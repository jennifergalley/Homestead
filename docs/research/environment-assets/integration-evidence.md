# Observed integration contracts

Inspected source checkpoint: `af0c90707c4e868fa693572563f987812813c318`.
Read-only source inspection on 2026-09-20; no engine was launched.
The parent has newer offline/fullscreen work in flight. Re-read accepted source
before a separately authorized implementation; this document is not a merge.

| Surface | Observed contract | Consequence for next run |
| --- | --- | --- |
| `Scripts\bootstrap_unreal.py:13-44` | Existing assets short-circuit import. FBX path disables material/texture import, combines meshes and generates collision. | Do not feed a multi-part plant pack blindly into this rock-oriented path. Record individual meshes, materials and units; make targeted imports reproducible. |
| `bootstrap_unreal.py:92-124` | Three JPG maps feed diffuse/normal/roughness; normal and roughness use non-sRGB; existing materials short-circuit graph construction. | Leaf opacity/subsurface/wind and slot-preserving graphs need explicit environment-specific handling, not a filename-only asset swap. Avoid silently keeping an old graph. |
| `bootstrap_unreal.py:133-137` | Every imported rock slot gets `M_Rock`. | Valid incumbent rock policy, not a generic bark/leaf import policy. Keep existing asset behavior. |
| `HomesteadWorld.cpp:121-140` | `Material` caches Tint/Roughness/Glow and special-cases textured rock to white. | Natural assets require identity-aware authored-material preservation rather than reusing color as the material identity. |
| `HomesteadWorld.cpp:142-163` | `AddPart` overwrites slot zero and scales dimensions by 100. | Interactive imported plants need an explicit authored-bounds/pivot/material path; keep primitive helper behavior intact. |
| `HomesteadWorld.cpp:165-205` | `AddDecoration` batches by mesh name/tint/collision/visibility, overwrites all slots, bounds-normalizes each axis around the bounds center. | Preserve distinct material sets in batching keys and tree proportions/root pivots. Existing rock normalization cannot be assumed correct for trees. |
| `HomesteadWorld.cpp:207-285` | Terrain 320 cells at 25 cm, UVs XY/300, analytic normals, separate noncolliding stream/bank ribbons. | Material-only variation may use deterministic world masks; do not displace vertices or change stream/collision. No Landscape import is needed. |
| `HomesteadWorld.cpp:287-344,791-809` | One world owner creates exposure, sun/moon, realtime skylight, atmosphere and fog; state hour drives daylight and a three-day rain schedule. | Clouds must use this owner/state and preserve night/rain, with a clouds-disabled route. Do not install pack lighting actors. |
| `HomesteadWorld.cpp:346-504` | Seeded tree/rock/grass placement with home/resources/structures/plots exclusions; HISM rebuilt after batches. Trunks block, understory does not. | Keep seeded placement and authoritative exclusions; count actual accepted instances, not loop attempts. Verify render/shadow/collision bounds for larger assets. |
| `HomesteadWorld.cpp:519-637,856-940` | Base and produce visuals use distinct ID maps and ready/harvested signatures; cleared resources draw nothing. | Preserve harvest/regrowth/clear lifecycles. A decorative lookalike must not replace an interactive node or add a second invisible obstacle. |
| `HomesteadWorld.cpp:869-880` | Decoration layout signature includes resource IDs/positions and structure/plot cells, not cleared flags. | Preserve resource-site exclusion even after clearing; never fill a newly cleared build site with a non-removable decorative tree. |
| `HomesteadWorld.cpp:741-789` | Crops, weeds and moisture are staged state-driven visuals. | Decorative groundcover must stay out of plots and must not obscure ripe produce/weeds. Crop rules are out of scope. |
| `Scripts\Fetch-Assets.ps1` | Manifest bytes gate downloads; existing receipt hashes detect changed content; archive members are selected explicitly. | Extend established receipts later; never invent bytes/hashes during research, download entire unrelated packs or overwrite provenance on mismatch. |
| `HomesteadVisualPlaytest.cpp:48-89,115` | Logs viewport/target/settings/CVars; sampled readbacks explicitly are not a benchmark. | Supplement accepted parent diagnostics with actual Lit/Lighting/ShaderComplexity and runtime primary render fraction. Separate visual capture from timing runs. |
| `HomesteadForageRenewal.cpp:101-145` | Renewal checks hardcode base and produce component counts, then check registration, visibility, focus and cleared-site collision. | Retain semantic lifecycle/reward/collision checks; adapt affected representation-count assumptions without demanding the old sphere count from natural meshes. |

## Baseline observation

Viewed the explicitly authorized local
`environment-reference-1080.png` from the parent session. The frame shows warm
low-angle light, an open central meadow, surrounding primitive trees, sparse
pointed grass and existing ground detail. It is a downscaled still, not a
measurement of 4K primary rendering, display refresh, movement, GPU cost or
Jenny's final realism approval. No reference image is copied into the repo or
uploaded to an external service.

## Source authority

`docs\game-plan.md` governs scope, with `PRODUCT.md` and `DESIGN.md` as product/
descriptive context. Some prose is historically stale (for example "no git
repository yet"); the inspected checkout is a Git worktree. Do not perform
unrelated documentation cleanup in this planning change.
