# Proposal

## Why

Jenny, playtesting: "Why does that river just appear from nowhere?" The estate river started as a
flat, square-ended slab of water 6.6 m wide, tilted on a grassy slope with no source. Along its
course it floated over the grass wherever the valley side fell away, because reshape.py's cut only
lowered ground (a 4.4 m flat bed with low banks), so side-hills had no downhill bank. It also
stopped short of the sea on the cove beach, the ribbon was spatially loaded (the manor saw no river
and the pail probe found nothing there), and the pail dip stopped about a metre short of the water.

## What Changes

- **A real channel all the way down** (`Scripts/Terrain/river_channel.py`): the heightfield is
  graded to a designed cross-section along the river. That's a flat bed, banks rising 0.6 m per
  metre to 0.25-0.5 m above the water, and a raised bank wherever the ground falls away. The stream
  grows from a 1.2 m spring rill to a 3.8 m river over its first 300 m. There's a shallow ford with
  low, gentle banks where the drive crosses, and a shallow run over the cove beach that reaches the
  shore wash. It writes EstateHeightfield.r16, the heightmap PNG and the work npy, and records the
  water surface and waterline half-width per point in estate_layout.json. reshape.py runs it last.
- **A spring**: the river rises in a small pool at the head of its valley, cut into the head
  wall, with granite boulders and cobbles round it. The water froths where it wells up.
- **The Landscape follows the heightfield**: `UHomesteadEstateAuthoringLibrary::ApplyEstateHeightfield`
  writes only the tiles that differ into the base edit layer. This time that was 29 tiles in 9
  landscape proxies.
- **The ribbon**: it's drawn from the graded surface at every layout point. It runs 45 cm under each
  bank, so the ground draws the waterline. Both ends round off: a semicircle round the spring pool,
  and a 6 m taper at the sea. White water (vertex colour B, `M_EstateRiver`) shows on riffles
  steeper than about 4 % and at the spring. V restarts every 10 m so half-precision UVs don't smear
  the ripples into stripes at the far end. It's not spatially loaded.
- **The pail**: she aims 25 cm inside the waterline and steps down the bank (up to 1.1 m, feet
  kept on the ground) until the pail's reach lands in the water.

## Capabilities

### New Capabilities

- `estate-river`

### Modified Capabilities

None.

## Impact

- New: `Scripts/Terrain/river_channel.py`, `M_EstateRiver`, the spring stones (`EstateSpringStone1-8`).
- Changed: `HomesteadWaterRibbon.{h,cpp}`, `HomesteadController` (`FreshWaterDipPoint`),
  `HomesteadCharacter` (`PlayFillPail`, `SettleOnGround`), the editor library (`ApplyEstateHeightfield`,
  and the Foliage module dependency its landscape header needs), `place_water.py`, `reshape.py`,
  `bootstrap_unreal.py` (the creek graph gains the white-water layer and a name),
  `EstateHeightfield.r16`, `Estate_Heightmap_4033.png`, `estate_layout.json`, `EstateGround.bin`, 9
  landscape proxies, `EstateRiver`.
- No save change. No placement moved.
