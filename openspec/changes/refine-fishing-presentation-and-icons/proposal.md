# Proposal

## Why
Jenny's selected feedback `jenny-muu6k1g1-qr5dx2`: the fishing pole and six fish icons look unlike the other tools and finished crops. Fishing also doesn't look like fishing: there's a fleeting text/progress bar instead of a visible cast, bite and catch.

## What Changes
- Original fishing pole and six fish species glyphs, drawn in the same style as the tool and crop icons. Gameplay switches them to glyph-first for the 9 PM build.
- An original baked MetaHuman clip (`AN_HeroineMH_Fishing`, authored by `fish_cast.py`) covering cast, wait, bite, fight, strike, catch and miss. Its contact beats are shared with gameplay through `HomesteadFishingPresentation.h`.
- A visible line, float and caught fish attached to the pole tip, plus an on-screen bobber/ring bite cue in place of the band meter.

## Capabilities
### New Capabilities
- `fishing-presentation`: readable cast/bite/catch animation, tackle and icons driven by gameplay's fishing state.
### Modified Capabilities
None.

## Impact
Presentation only: anim instance, character fishing tackle, `SHomesteadFishing`, `SHomesteadIcon`, one clip and its authoring script. Gameplay owns the randomized bite and hook-window rules, rewards, input and saves. No save, enum or placement changes.

## Smallest useful result and first playable demonstration
Equip the pole at water and cast. The heroine casts, the float lands, it bobs on a bite, she strikes and lifts the fish. The pole and fish show proper icons in the hotbar and pack.
