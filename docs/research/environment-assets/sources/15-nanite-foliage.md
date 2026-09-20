# S15 - Epic specialized Nanite Foliage

URL: https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-foliage
Accessed: 2026-09-20 by the counterevidence worker.
Type: primary engine technical documentation.

## Short verbatim evidence

- "introduces a lot of overdraw"
- "This plugin is experimental."

## Interpretation and limits

The first excerpt concerns alpha masking. The second concerns Dynamic Wind.
This specialized pipeline uses assemblies, voxels and skinning; it is not the
same as enabling Nanite on a free FBX. Full triangle leaves also have storage
and distant-simplification tradeoffs. The documented wind path requires setup
and has limitations including global direction and no player/object collision.
Adopting this pipeline is outside the small asset-replacement scope.

## Assessment

Credibility: high for documented architecture, not a universal performance
comparison. Recency: live route, patch-specific availability untested.
Bias: vendor demonstration. No independent target-project replication.
