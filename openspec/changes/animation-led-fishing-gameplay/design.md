# Design

## Context

See proposal.md. Current fishing waits a hashed 2-4 seconds, hooks within 0.9 seconds and immediately grants the fish after two fixed green-band presses. Its state is transient, outside serialization.

## Goals / Non-Goals

Keep authoritative rules in plain C++17 and presentation in actors/widgets. No generalized tension infrastructure, persistent cast save data, third-party content or extra habitats.

## Decisions

Extend FishingSession with cast/catch presentation phases and bounded randomized strike cues. Deterministic hashes include cast identity/revision so repeated attempts vary without hidden RNG save state. Timers advance reaction windows but cannot issue rewards.

The final cozy tuning uses a 2-4-second bite wait, 0.9-second hook window, and two strikes delayed 0.65-1.25 seconds with 0.7-second reaction windows. A 2.0-second catch-contact fail-safe tolerates frame hitches across the authored strike/lift; expiry or cancellation still yields nothing without the genuine lift contact. Player-facing names and instructions use fresh mackerel and closing-ring cues, not sushi or a retired green band.

Expose explicit cast-release and successful-catch contact commands returning Result. Catch contact requires the authorized successful phase, matching cast token and the authored minimum beat; repeated or stale contacts fail without mutation. A missing contact times out without reward.

Remove fishing dispatch from E/A interactions; tool input owns the sequence. Controller forwards Art's authored events to simulation and renders the phase/cue API without per-frame world scans.

## Lanes and ownership

Gameplay UI owns Simulation/HomesteadFishing.*, native tests and controller policy. Fishing Art owns original character clips, presentation widget, animation event wiring and agreed timing constants. Exact timing constants are supplied by the authored clips; changes remain within the bounded contact contract. Integration alone merges/packages.

Agreed Art interface: HomesteadFishingPresentation.h defines EHomesteadFishingPose (None/Cast/Wait/Bite/Fight/Catch/Miss). UHomesteadAnimInstance exposes SetFishingPose, PlayFishingStrike and authored FishCastSplashes/FishCatchLifts counters. Controller snapshots counters per cast and forwards fresh contacts only. Missing clips must not synthesize contact counters from timers.

Ship only at 9 PM, separate from the 4 PM seed/Back/quit/Harvest cut. Cast requests are keyed by the native cast token, including cancel/recast before an animation tick. Release Miss at the authored final sample (`MissEnd - ClipFrameSeconds`), then fade to None even when the native phase stays Idle. CancelAction clears pose, queued Cast/Strike and finishing-catch state. The portable presentation header lets native tests use the exact authored splash/lift/frame constants without engine types. Art's pole and six species glyphs activate only in this evening branch.

## Risks / Trade-offs

Art not ready -> no timer-only success fallback; defer fishing integration explicitly. Late/missing/duplicate contact -> token and phase checks plus bounded expiry. Pack fills during animation -> refuse reward without partial mutation. Input held across phases -> one press edge per action.

## Migration Plan

No persistent fishing state or save format change. Native tests cover timing boundaries, failure/cancel, no timer reward, exact single-contact reward and stale contacts; integrated animation timing remains Art's verification gate.
