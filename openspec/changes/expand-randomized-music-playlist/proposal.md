# Proposal

## Why

Playtests always hear the same song because Homestead currently imports and loads exactly one music asset, `Evening Fall (Harp)`; save behavior is not the cause and shuffle logic cannot create variety from a one-track catalog. The game needs a small legally admitted playlist plus launch-independent playback ordering.

## What Changes

- Admit three to five additional calm instrumental tracks that fit grounded woodland/farm play, using the incumbent Kevin MacLeod/Incompetech catalog when exact official downloads, CC BY 4.0 terms, source identity, duration, and attribution all verify.
- Use `Evening Fall (Harp)` as the retained anchor. Candidate additions for exact-source audition are `Valse Gymnopedie`, `That Zen Moment`, `Canon in D for Two Renaissance Harps`, and `The Britons`; implementation MUST reject any candidate whose official title/source/license/download or in-game mood cannot be verified.
- Import each accepted source into its own Unreal music asset, preserve source receipts/checksums, cook it, and add exact attribution to packaged `asset-credits.md`.
- Replace single-track playback with a per-session Fisher-Yates shuffle bag: play every valid track once before reshuffling and prevent the same track from playing twice across a bag boundary.
- Persist only the most recently started track ID in user-level audio preferences so a fresh app launch can avoid immediately starting with the previous launch's track even when no world is saved. Playlist order remains session-only and world saves never control it.
- Choose a fresh randomized first track after the normal opening gap, preserve 4-second fades and randomized silence between pieces, and continue to honor Music volume/mute immediately.
- Degrade explicitly: skip missing/invalid tracks with diagnostics; if exactly one valid track remains, play it without claiming shuffle; if none remain, continue ambience and report music unavailable.
- Add deterministic seeded playlist tests plus ordinary multi-launch evidence without making production order deterministic.

Focused reuse research found the existing track and all candidate additions in Kevin MacLeod's Incompetech royalty-free catalog, whose official license/FAQ permits commercial redistribution under CC BY 4.0 with attribution ([music catalog](https://incompetech.com/music/royalty-free/music.html), [license FAQ](https://incompetech.com/music/royalty-free/faq.html)). Final admission remains exact-file and exact-page gated; third-party mirrors are not accepted as source authority. No plugin or playback framework is needed.

The smallest useful in-game result imports at least three verified tracks and starts two fresh no-save launches on different tracks when the catalog allows. The first playable demonstration records a complete session shuffle with no repeat before every track has played, then relaunches and proves the persisted last-track guard works independently of homestead saves. Full acceptance covers missing assets, one/zero-track fallback, volume/mute, fades/gaps, credits/package identity, separate-process restart, Settings integration, and immutable Shipping replay.

Deferred scope includes adaptive season/weather/daypart scoring, combat/event stingers, crossfading overlapping tracks, user playlists, streaming audio, per-track volume normalization beyond import checks, a now-playing HUD, and music preference categories.

## Capabilities

### New Capabilities

- `randomized-music-playlist`: Multi-track admitted music catalog, shuffle-bag playback, cross-launch immediate-repeat avoidance, fallback behavior, and attribution.

### Modified Capabilities

None.

## Impact

- `HomesteadController` audio initialization/tick/finish state and user-level `Homestead.Audio` settings: playlist loading, entropy, shuffle bag, last-track persistence, and explicit fallbacks.
- `Content/SurvivalGame/Audio/Music`, source admission records, bootstrap/import scripts, cook manifests, and `docs/asset-credits.md`: additional exact music assets and attribution.
- Audio proof/settings/native/Shipping tests: deterministic shuffle properties, production entropy, multi-launch independence from world saves, fades/gaps, mute/volume, and package completeness.
- Current music timing and selected build remain rollback until the expanded playlist is admitted, packaged, and playtested.

