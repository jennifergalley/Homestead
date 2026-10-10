# Design

## Decisions
- **Timing contract** (`HomesteadFishingPresentation.h`, 30 fps, clip seconds): Cast 0-1.6 (line release 0.62, splash 1.1); Wait 1.6-3.6 (loops); Bite 3.6-4.4 (loops); Fight 4.4-5.6; Strike 5.6-6.0; Catch 6.0-7.4 (lift 6.4); Miss 7.4-8.0. Gameplay reads these constants for its contact beats and its Miss completion.
- Gameplay owns randomness, state, input and rewards. Presentation reads the state each frame and never decides an outcome.
- Comfortable anatomy: two-handed grip, overhead-to-forward cast and a lift through the elbows. Authored with the existing MetaHuman Control Rig; no copied game art.
- Icons are vector glyphs alongside the existing renders. Generic Fish keeps the old routing.
