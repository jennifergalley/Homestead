# Design

## Route data (Water, final: `jennifergalley-cove-route` @010c7003)

The schedule is in `openspec/changes/add-cove-route/design.md` and the pivots in
`Source/SurvivalGame/Simulation/HomesteadEstateCoveRoute.inc` (cm, this kit's conventions). The route
runs 506 m from the manor's front door (-260.5, -654.5) m down to the sand west of the river mouth
(-556, -521) m, falling from 86.5 m to 3.7 m.

| Piece | Count | Sizes |
| --- | --- | --- |
| Flights | 18, 188 risers | rise 16.6-17.0 cm, going 30.0-34.1 cm, 7-12 risers each; pitches 26 (2 flights), 28 (11), 29.4 (5) |
| Landings | 15, plus 2 corner landings | 1.23-2.30 m long; corner landings 1.5 m square, turning 18 and 21 degrees |
| Kerbs | 148 | 1 m straights |
| Rail bays | 82 | 39 level, 43 raked; 35 are mirrored (scale Y = -1) |
| Fingerposts | 2 | front door, yaw 159; head of the cliff steps, yaw 153 |

The first-draft constraints below are superseded where they differ.

## First-draft constraints (Water, 2026-09-30)

| Item | Value |
| --- | --- |
| Start | the manor's south front, about (-265, -650) m, z about 86.5 m |
| End | CoveBeach (-650, -515) m, z about 1.8 m |
| Length and fall | about 450 m of path, about 85 m of fall |
| Ramps | 1 in 8 or gentler, following the contours with switchbacks |
| Stairs | 3 to 6 short flights across the cliff band (the last ~25 m of fall) |
| Clear width | 1.4 m |
| Drop side | seaward (south / west): kerb and rail there; no rail on a side cut into rock |
| Ground | Water cuts the heightfield a few cm under each tread and ramp |
| Signs | one fingerpost at the manor-south gate (yaw about 215 degrees), one at the top of the cliff stairs |

## Dimensions

- **Steps.** Rise 15 to 17 cm, going 30 to 35 cm, 2R + G of about 64 cm. At most 12 risers to a flight; a
  granite landing of at least 120 cm going between flights. Treads are 150 cm wide: the 140 cm clear
  width, plus 5 cm bedding under the kerb or cheek on each side. Each tread block is 20 cm deep, so it
  covers the next tread's back edge by 3 to 5 cm, and it has a 5 cm skirt below its lowest visible edge so
  no gap shows over the cut ground.
- **Kerbs.** 100 cm long straight pieces, 15 cm wide, standing 20 cm proud of the path with 10 cm buried. A
  15-degree curve piece for switchbacks, and a rounded end stone.
- **Rails.** Oak posts 12 × 12 cm at bays of 160 to 180 cm, set 10 cm outside the clear width. A rounded
  oak top rail 95 cm above the path, or above the step nosing line on a flight; a mid-rail at 50 cm. Iron
  shoe plates at the post feet. A raked bay is built for a given pitch (atan of rise over going), so each
  flight gets its own export or a small set of standard pitches (26, 28, 30 degrees): `SM_CoveRail_Level` and `SM_CoveRail_Rake26/28/30`, 1.70 m in plan. Water's builder scales a bay's X and Z together to its plan length, so the pitch holds.
- **Fingerpost.** A 210 cm oak post, 12 × 12 cm, with a capped finial. One arm, 60 × 12 × 3 cm, at 170 cm,
  reading "To the Cove": incised letters painted white on the weathered oak. The arm points along the
  post's +X, so Water sets the yaw.

## Agreed with Water (2026-09-30)

- **Treads.** A flight of n risers has n treads, i = 0..n-1, at foot nosing + i × (going along the yaw,
  rise). The top tread is level with the landing or path above it: its top face is that landing's
  surface.
- **Landings.** Pivot on the top face at the centre of the downhill edge, +X uphill. Lengths run
  1.23-2.30 m, so the kit provides a 0.6 m slab (`SM_CoveLandingSlab`) that Water tiles along X,
  scaling each run by at most ±10% so the stone never stretches visibly. The 1.2 m `SM_CoveLanding`
  stays for the stock lengths.
- **Corner wedges.** `SM_CoveLandingWedge`: a granite wedge that closes the outside of a corner
  landing's turn, where cut ground would otherwise show 5 cm down. It is authored for 21 degrees (pivot
  at the inner corner on the top face, +X along the incoming leg) and scaled along Y for 18 degrees.
- **Mirrored rails.** Water mirrors 35 raked bays with scale Y = -1 so their rake runs the right way on
  the far side. Every rail piece is therefore symmetric across its rail line (the XZ plane): posts,
  shoe plates, nails and chamfers all match on both faces, with no text, no one-sided wear and no
  directional grain decal. Unreal flips the winding under a negative scale, so shading stays correct.
- **Ground.** Water cuts the ground 5 cm under tread and landing tops for 2.2 m either side; the 20 cm
  tread block with its 5 cm skirt covers the cut.
- **Fingerposts.** Yaws of 159 degrees (front door) and 153 degrees (head of the cliff steps).

## Pivots and orientation

The FBX export mirrors Y, so the recipes build the pieces' drop side (kerbs) and the wedge toward -Y and
lay the fingerpost's letters mirror-image; they arrive in the engine as described here. The side cheek of
the first draft is dropped: Water cuts the ground to the flights, and kerbs and rails edge them.

Blender recipes are authored in metres, Z up, and the kit puts the pivot at the bottom centre. The
export mirrors Y, so the directions below are given in the engine's frame, which is what Water's
generator uses.

- **Step and landing.** Pivot on the top of the tread at the centre of its front nosing. +X runs up the
  flight (the going direction) and Y is the width. A flight is placed at pivot_i = start + i × (going,
  rise).
- **Kerb.** Pivot on the path-side top edge at the piece's centre. +X runs along the path and +Y points to
  the drop.
- **Rail.** Pivot at the foot of the downhill post, on the path surface. +X runs along the path (uphill on
  a raked bay) and +Y points to the drop. For the other side of the path, rotate the piece 180 degrees
  about Z at the other edge; the pieces are symmetric across their rail line, so one set serves both sides.
- **Fingerpost.** Pivot at the foot of the post; the arm points along +X. The arm's lettering reads on both faces, so either side of the post is fine.

## Collision

- Steps and landings: a simple box per tread in the mesh's collision, so she walks the flight as a ramp
  (her step height is 45 cm).
- Kerbs block with simple boxes and refuse step-up (`CanCharacterStepUpOn = ECB_No`); a 20 cm kerb is
  under her step height and would otherwise be walked onto.
- Rails: no collision on the mesh. Water's builder adds one pawn-only blocker along each railed edge
  (ignoring camera and visibility, `ECB_No`) so she can't pass between the posts.
- Fingerpost: one pawn-blocking box around the post, no step-up.

## Materials

- Granite: the existing `kit.mats.granite` with worn arrises, a dished centre on each tread, salt
  bloom and grey-green lichen toward the sea edge; about 0.25 linear albedo.
- Oak: `kit.mats.wood`, silvered and weathered, with iron-stain streaks below the shoe plates.
- Fingerpost letters: white paint, chipped and weathered, in incised grooves. The text is converted to
  mesh from a font whose licence is checked and recorded in `PROVENANCE` before commit; if the lettering
  is too heavy it becomes a baked text texture on the arm.

## Budgets

- Treads and landings: about 3k triangles each, Nanite, 2K texture sets shared across the three tread
  variants.
- Kerbs: about 1.5k triangles each, Nanite.
- Rails: about 2k triangles per bay, no Nanite (thin wood), with LODs.
- Fingerpost: about 4k triangles, with LODs.
