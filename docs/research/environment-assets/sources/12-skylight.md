# S12 - Epic Sky Lights

URL: https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-lights-in-unreal-engine
Accessed: 2026-09-20 by the ground/sky worker.
Type: primary engine reference.

## Short verbatim evidence

- "Volumetric Fog is not supported."
- "Time slicing is enabled by default"

## Interpretation and limits

These statements concern realtime skylight capture; they do not prohibit
Exponential Height Fog in the scene. Prefer GPU realtime capture/time slicing
over repeated CPU RecaptureSky calls. The page's PS4 128-pixel cubemap example
is not a prediction for this RTX5080 game; no numbers are adopted as budgets.

## Assessment

Credibility: high for capture limitations. Recency: live route, unknown revision.
Bias: engine vendor; example hardware and scene differ from target.
