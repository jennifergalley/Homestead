# Design

## Context

See `proposal.md` and `specs/randomized-music-playlist/spec.md`. Current audio loads exactly one `USoundBase`, binds one finish callback, waits an initial 18 seconds, fades for four seconds, and inserts a random 55-110 second gap. Only `EveningHarp.uasset` exists and `docs/asset-credits.md` credits only `Evening Fall (Harp)`. World saves currently carry volume, while the pending audio-controls change moves volume authority to user settings.

The incumbent artist/source already provides a broad CC BY 4.0 catalog and an established attribution pattern. Candidate titles are an audition list, not admitted assets until official source/license/download identity and in-game mood pass.

## Goals / Non-Goals

**Goals:**

- Add enough stylistically coherent music for audible variety.
- Guarantee fair no-repeat ordering within sessions and avoid immediate cross-launch repetition without requiring world saves.
- Preserve current fades, silence, separate volume, offline packaging, and explicit failure behavior.

**Non-Goals:**

- Dynamic/adaptive scoring, streaming, user playlists, per-season sets, stingers, overlapping crossfades, a now-playing HUD, or replacing ambience.

## Decisions

### 1. Admit a small exact-source catalog before coding shuffle

Audition the incumbent plus candidate Incompetech tracks from official pages only. For each accepted track, retain title, composer, official URL/ISRC where published, license URL/text, source file SHA-256, duration, import settings, Unreal package path, cooked identity, and exact `asset-credits.md` entry. Reject third-party mirror-only downloads and tracks that feel overly comedic, modern, dramatic, or repetitive beside ordinary woodland play.

At least three accepted tracks are required; four or five is preferred if the mood remains coherent. This is a quality gate, not a quota that forces weak music into the game.

### 2. Use a Fisher-Yates bag over loaded track descriptors

Build a list of successfully loaded descriptors, shuffle indices with session entropy, and pop sequentially. At exhaustion, reshuffle and swap the first entry if it equals the prior track and another option exists. The audio component sets the next sound only when starting it; existing fade/gap lifecycle remains.

Tests inject a seeded random source. Production seeds from process/session entropy such as platform cycles plus UTC/GUID mixing, never world seed/save data.

**Alternative considered:** choose a random index after every song. Rejected because it can repeat frequently and starve tracks.

### 3. Persist only last-started identity at user scope

Reuse the exact-property user-settings pattern planned for `Homestead.Audio`. `LastMusicTrack` is a stable catalog key, validated independently. On startup, the first bag position avoids it when possible. Persist when a track successfully begins, not when it finishes, so abnormal exits still protect the next launch.

World saves cease to influence music order. The value is a playback preference/history hint, not simulation state.

### 4. Separate catalog failure from audio subsystem failure

Load every descriptor once at initialization, log one precise diagnostic per missing/invalid asset, and construct the bag from valid entries. One entry uses the existing single-track path; zero entries disable only music and leave ambience/effects untouched. Do not insert nulls or success-shaped fallback tracks.

### 5. Coordinate with audio slider work

Playlist work owns catalog/order/finish transitions and last-track persistence. `refine-settings-and-wildflower-groundcover` owns Music/Ambience/Effects slider UI and volume persistence. Both share the `Homestead.Audio` config helper contract before integration. Asset admission can run independently; Editor import/cook/package and final audio proof remain serialized.

## Risks / Trade-offs

- **[Candidate license/title is misidentified]** -> Official source page, exact file hash, CC BY text and packaged credit are mandatory before import.
- **[Tracks vary too much in loudness or mood]** -> Audition in ordinary gameplay, inspect peaks/import settings, and reject rather than hide weak fit with code.
- **[Cross-launch value outlives a removed track]** -> Unknown key is ignored and replaced on next successful start.
- **[Fade callback advances twice]** -> Keep one transition owner and assert one bag pop per successful start/finish lifecycle.
- **[Muted music churns silently]** -> Pause start countdown while volume is zero without discarding bag position.

## Migration Plan

1. Verify/admit at least three exact official tracks and update source/package credits.
2. Import into fresh music packages and verify duration/cook/load identity.
3. Add deterministic bag logic and portable tests, then bind to existing audio lifecycle.
4. Add user-level last-track exact-property persistence and separate-process no-save restart proof.
5. Exercise volume/mute, one/zero-track failures, fades/gaps, full-loop and ordinary listening.
6. Build one immutable Shipping candidate and retain the current selected build as rollback until explicit promotion.

