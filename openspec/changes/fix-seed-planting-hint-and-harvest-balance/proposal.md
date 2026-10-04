# Proposal

## Why
Jenny's selected feedback (`jenny-mut56ggp-uijcta`, `jenny-mut57myz-9tfjci`) reports disappearing planting hints and excessive harvested seeds. Fix both for build `20261003-measured-02` without changing crop growth, tools or saves.

## What Changes
- Keep the keyed Plant Seeds hint visible on eligible tilled ground, including after repeated successful planting.
- Reduce cultivated root seed returns from two guaranteed seeds to at most one occasional seed; bought crop seeds remain purchase-dependent.

## Capabilities
### New Capabilities
None.
### Modified Capabilities
- `crop-growing`: persistent planting cues and reduced cultivated seed returns.

## Impact
Garden targeting/sow cues, controller focus, HUD retirement, crop yields and focused native tests. No placement, save-version, animation or growth changes.

## Reuse research
`CheckSow` already validates eligibility; `DescribeSow` and garden targeting supply cues. `DrawInteractCue` currently removes a lone keyed hint after three successful uses, explaining the reported disappearance. Reuse these paths, existing crop data and deterministic hash conventions. No external assets, libraries or licensing dependencies.

## Smallest useful result and first playable demonstration
Select seeds, aim at eligible tilled ground and plant repeatedly: [E] Plant Seeds remains visible. Harvest cultivated roots across a representative garden and receive far fewer seeds. Fishing and new recipes are separate checkpoints; Jenny performs integrated player acceptance.
