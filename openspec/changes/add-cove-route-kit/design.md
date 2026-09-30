# Design

## Route constraints (Water, 2026-09-30, provisional until the route data lands)

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
  flight gets its own export or a small set of standard pitches (26, 28, 30 degrees).
- **Fingerpost.** A 210 cm oak post, 12 × 12 cm, with a capped finial. One arm, 60 × 12 × 3 cm, at 170 cm,
  reading "To the Cove": incised letters painted white on the weathered oak. The arm points along the
  post's +X, so Water sets the yaw.

## Pivots and orientation

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
- **Fingerpost.** Pivot at the foot of the post; the arm points along +X.

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
