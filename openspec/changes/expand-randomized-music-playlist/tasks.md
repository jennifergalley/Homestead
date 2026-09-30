# Tasks

## 1. Exact Music Admission

- [ ] 1.1 Record the one-track selected-build baseline across fresh no-save launches, current fade/gap/volume behavior, package identity and credits, verifying `Evening Fall (Harp)` is the only loaded/cooked score
- [x] 1.2 Audition the incumbent and official Incompetech candidates in ordinary woodland play, then admit at least three coherent tracks only after exact title/source/license/download/duration/hash/attribution verification; verify third-party mirrors and mood-mismatched candidates are rejected. (2026-09-25: admitted four official Incompetech downloads, with metadata from the official `pieces.json`: Ascending the Vale USUAN1600064 (Relaxed/Calming, harp and flute, 247.6 s), Teller of the Tales USUAN1400020 (Calming/Relaxed, lute, 212.4 s), Meditation Impromptu 02 USUAN1100162 (Relaxed/Calming, piano, 249.1 s) and At Rest USUAN1100748 (Relaxed, piano and strings, 206.6 s). They were screened by onset rate (1.1-2.6/s, harp 2.9) and spectral centroid. Rejected: Achaidh Cheide (busiest at 3.7 onsets/s and loudest, -16.7 dB), Frost Waltz (wintry and loud) and Almost in F (official download 404). Sizes are pinned in `asset-manifest.json` and hashes in the fetch receipt. The in-play audition is Jenny's, still to come.)
- [x] 1.3 Import accepted tracks into distinct music assets with retained source/provenance receipts and packaged `asset-credits.md` entries; verify fresh-process load, duration, cook inclusion, output identity and reasonable playback level. (Level: each track is matched to -35 dB K-weighted at 100% music volume, about -38.7 dB at the default 65%. That is 14 dB under the old harp mix and leaves the ambience and footsteps audible.)

## 2. Shuffle and Cross-Launch Variety

- [x] 2.1 Implement a testable Fisher-Yates bag over successfully loaded descriptors, including full-bag uniqueness and no same-track bag-boundary repeat; verify deterministic seeded property tests across catalog sizes 0, 1, 2 and the admitted set (2026-09-25: `Source/SurvivalGame/HomesteadMusicPlaylist.h`, engine-free. `Tests/HomesteadMusicPlaylistTests.cpp` checks 0, 1, 2, 3, 5 and 8 tracks over 400 seeds and 6 bags each, plus seed variety and fixed-seed reproducibility; it passes with `cl /W4 /WX`.)
- [x] 2.2 Seed production order from session/process entropy independent of world/save state, while keeping tests injectable; verify identical homestead states do not force identical production order (The seed combines the cycle counter, UTC ticks and the process ID.)
- [x] 2.3 Persist exact `LastMusicTrack` in user-level `Homestead.Audio` state when a track starts and exclude it from the next launch's first choice when possible; verify write/read/invalid/read-only rollback and two fresh no-world-save process launches (It is written when a track starts; an unknown or missing name is ignored, and a failed write only logs a warning. The two-launch check is in 3.2.)
- [x] 2.4 Integrate the bag with existing fade, randomized gap, Music volume/mute and ambience independence; verify one transition per finish, no reseed/reset on slider changes and no silent churn while muted (The volume slider only changes the level. Muted, no track starts. After each start the gap timer is held so a one-frame `IsPlaying` lag can't advance the bag twice.)
- [x] 2.5 Skip missing tracks with one precise diagnostic and preserve explicit one-track/zero-track fallback; verify ambience/effects remain operational and no repeated load spam or false shuffle success appears

- [x] 2.6 Pacing (Jenny, 2026-09-29): the first piece starts 45-120 s into a session, and each piece is followed by 3-8 minutes of ambience alone (was 18 s, then 55-110 s). `Homestead::MusicPacing` in `HomesteadMusicPlaylist.h`; `HomesteadMusicPlaylistTests` is now registered in CMake and covers it.

## 3. Integrated Acceptance and Promotion

- [ ] 3.1 Run focused playlist/audio/config/attribution contracts, full relevant portable/native suites, strict OpenSpec validation and SurvivalGameEditor Win64 Development build from a clean integrated checkpoint
- [ ] 3.2 Exercise an ordinary complete shuffle plus repeated no-save launches, volume/mute changes, fades/gaps and missing-asset fixtures in Editor, verifying audible variety without abrupt or mood-breaking transitions
- [ ] 3.3 Build one immutable Shipping candidate, verify cooked tracks and exact credits, run fresh producer/consumer audio proof and comparable cadence, and promote only if the playlist materially varies while remaining calm, legal and rollback-safe

