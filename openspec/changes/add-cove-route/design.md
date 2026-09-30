# Design

## Route plan (manor to beach, on foot)

The route starts 1.5 m outside the fallen front door on the ruin's south front (EstateManorFrontDoor, where
the thin bramble chokes the gap) and ends on the sand at the head of the cove, just west of the river
mouth. It is **506 m long and falls 82.8 m** (86.5 m to 3.7 m).

1. **The meadow (0-349 m, 86.5 to 44.7 m).** A graded earth path, 1.4 m wide, down the open south slope in
   three switchbacks, never steeper than 1 in 7 (average 1 in 8.3). Kerbs stand on its downhill edge where
   the ground falls away beside it (the long traverses), and a level oak rail where it falls further.
2. **The cliff steps (349-414 m, 44.7 to 18.4 m).** The valley side falls at about 1 in 2.3 here, too steep
   for any path. Three straight stair legs go down the fall line, 155 risers in 14 flights with granite
   landings between, and square corner landings where the legs turn (18 and 21 degrees). A raked oak rail
   runs the whole way on one side. A fingerpost stands at the head.
3. **The bench (414-441 m, 18.4 to 14.7 m).** A path traverses back north-east along a bench above the
   valley floor at 1 in 7.2.
4. **The bench steps (441-448 m).** Two short flights (19 risers) drop to the valley floor, 7 m off the
   river. The path doubles back at their foot, so their rail stands on the far side.
5. **The valley floor (448-467 m).** A path runs back south-west at 1 in 7.2.
6. **The last steps (467-474 m, 9.0 to 6.7 m).** Two flights (14 risers) take the last steep bank above
   the beach, railed.
7. **Onto the sand (474-506 m, 6.7 to 3.7 m).** A gentle path at 1 in 10.8 to the sand.

Every drop steeper than 1 in 7 is on steps with a rail: the final descent is never an exposed dirt slope.

### By turning point

| # | Point | x, y (m) | Chainage (m) | Walk z (m) | Way on |
| --- | --- | --- | --- | --- | --- |
| 0 | Front door, the ruin's south front | (-260, -654) | 0 | 86.5 | path |
| 1 |  | (-280, -647) | 21 | 85.4 | path |
| 2 |  | (-290, -637) | 34 | 83.6 | path |
| 3 | First switchback | (-342, -611) | 86 | 77.3 | path |
| 4 |  | (-344, -651) | 119 | 73.2 | path |
| 5 |  | (-363, -673) | 147 | 70.1 | path |
| 6 | Second switchback | (-372, -676) | 156 | 69.0 | path |
| 7 |  | (-389, -662) | 177 | 66.4 | path |
| 8 |  | (-404, -632) | 210 | 62.3 | path |
| 9 |  | (-431, -607) | 247 | 58.8 | path |
| 10 |  | (-460, -580) | 285 | 53.5 | path |
| 11 | Third switchback, head of the valley side | (-461, -541) | 319 | 48.8 | path |
| 12 | Head of the cliff steps (fingerpost) | (-493, -557) | 349 | 44.7 | stairs |
| 13 | Corner landing | (-511, -548) | 369 | 36.6 | stairs |
| 14 | Corner landing | (-526, -533) | 390 | 28.3 | stairs |
| 15 | Bench above the valley floor | (-536, -511) | 414 | 18.6 | path |
| 16 | Head of the bench steps | (-524, -487) | 441 | 14.7 | stairs |
| 17 | Valley floor, 7 m off the river | (-521, -480) | 448 | 11.7 | path |
| 18 | Head of the last steps | (-538, -487) | 467 | 9.0 | stairs |
| 19 | Foot of the last steps | (-543, -492) | 474 | 6.8 | path |
| 20 | Sand west of the river mouth | (-556, -521) | 506 | 3.7 | end |

### By leg

| # | Way | From (x, y) m | Length m | Top z m | Foot z m | Fall m | Grade | Steps |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | path | (-260, -654) | 348.7 | 86.52 | 44.70 | 41.83 | 1 in 8.3 |  |
| 2 | stairs | (-493, -557) | 20.1 | 44.70 | 36.56 | 8.14 | 1 in 2.5 | 4 flights, 48 risers at 17.0 / 31.9 cm, 28 deg rail, landings 1.36 m |
| 3 | stairs | (-511, -548) | 21.2 | 36.56 | 28.28 | 8.28 | 1 in 2.6 | 5 flights, 49 risers at 16.9 / 30.0 cm, 30 deg rail, landings 1.25 m |
| 4 | stairs | (-526, -533) | 24.2 | 28.28 | 18.44 | 9.83 | 1 in 2.5 | 5 flights, 58 risers at 17.0 / 31.9 cm, 28 deg rail, landings 1.23 m |
| 5 | path | (-536, -511) | 26.8 | 18.44 | 14.72 | 3.72 | 1 in 7.2 |  |
| 6 | stairs | (-524, -487) | 7.6 | 14.72 | 11.53 | 3.20 | 1 in 2.4 | 2 flights, 19 risers at 16.8 / 31.7 cm, 28 deg rail, landings 1.60 m |
| 7 | path | (-521, -480) | 18.4 | 11.53 | 8.98 | 2.55 | 1 in 7.2 |  |
| 8 | stairs | (-538, -487) | 7.1 | 8.98 | 6.65 | 2.33 | 1 in 3.0 | 2 flights, 14 risers at 16.6 / 34.1 cm, 26 deg rail, landings 2.30 m |
| 9 | path | (-543, -492) | 31.8 | 6.65 | 3.70 | 2.95 | 1 in 10.8 |  |

### Flights

Tread pivots follow the kit: tread i of a flight is at the foot nosing + i x (going along yaw, rise). A
flight of n risers has n treads, and its top tread is level with the landing or path above it. Pitches
are the flights' own; the rail bay is the kit's nearest standard (26, 28 or 30 degrees, within a degree).

| Flight | Leg | Foot nosing (x, y) m | Yaw up | Risers | Rise / going (cm) | Pitch (rail bay) | Foot to top z (m) | Rail |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 1 | 2 | (-510.3, -548.3) | -26.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 36.56 to 38.59 | -Y (mirrored) |
| 2 | 2 | (-505.7, -550.7) | -26.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 38.59 to 40.63 | -Y (mirrored) |
| 3 | 2 | (-501.1, -553.0) | -26.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 40.63 to 42.66 | -Y (mirrored) |
| 4 | 2 | (-496.4, -555.3) | -26.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 42.66 to 44.69 | -Y (mirrored) |
| 5 | 3 | (-525.5, -533.5) | -45.0 | 10 | 16.9 / 30.0 | 29.4 (30) | 28.28 to 29.97 | -Y (mirrored) |
| 6 | 3 | (-522.5, -536.5) | -45.0 | 10 | 16.9 / 30.0 | 29.4 (30) | 29.97 to 31.66 | -Y (mirrored) |
| 7 | 3 | (-519.5, -539.5) | -45.0 | 10 | 16.9 / 30.0 | 29.4 (30) | 31.66 to 33.35 | -Y (mirrored) |
| 8 | 3 | (-516.4, -542.6) | -45.0 | 10 | 16.9 / 30.0 | 29.4 (30) | 33.35 to 35.04 | -Y (mirrored) |
| 9 | 3 | (-513.4, -545.6) | -45.0 | 9 | 16.9 / 30.0 | 29.4 (30) | 35.04 to 36.56 | -Y (mirrored) |
| 10 | 4 | (-536.0, -511.0) | -65.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 18.44 to 20.48 | -Y (mirrored) |
| 11 | 4 | (-533.9, -515.6) | -65.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 20.48 to 22.51 | -Y (mirrored) |
| 12 | 4 | (-531.8, -520.2) | -65.6 | 12 | 17.0 / 31.9 | 28.0 (28) | 22.51 to 24.55 | -Y (mirrored) |
| 13 | 4 | (-529.7, -524.8) | -65.6 | 11 | 17.0 / 31.9 | 28.0 (28) | 24.55 to 26.41 | -Y (mirrored) |
| 14 | 4 | (-527.8, -529.1) | -65.6 | 11 | 17.0 / 31.9 | 28.0 (28) | 26.41 to 28.28 | -Y (mirrored) |
| 15 | 6 | (-521.0, -480.0) | -113.2 | 10 | 16.8 / 31.7 | 28.0 (28) | 11.53 to 13.21 | +Y |
| 16 | 6 | (-522.9, -484.4) | -113.2 | 9 | 16.8 / 31.7 | 28.0 (28) | 13.21 to 14.72 | +Y |
| 17 | 8 | (-543.0, -492.0) | 45.0 | 7 | 16.6 / 34.1 | 26.0 (26) | 6.65 to 7.82 | +Y |
| 18 | 8 | (-539.7, -488.7) | 45.0 | 7 | 16.6 / 34.1 | 26.0 (26) | 7.82 to 8.98 | +Y |

Also 15 landings (1.23 to 2.30 m between flights; 1.5 m corner landings), 148 kerb pieces (1 m), 82 rail
bays (39 level, 43 raked, 35 of them mirrored) and 2 fingerposts: at (-266.8, -653.6) m, arm yaw 159
degrees (down the path from the front door), and at (-491.7, -557.9) m, arm yaw 153 degrees (down the cliff
steps).

## Decisions

1. **Designed from committed turning points, not searched each run.** `cove_route.py --search` runs a
   grade-aware least-cost search (paths at 1 in 7 or gentler, steps where steeper at a higher cost, a
   penalty on each turn so it makes long traverses and real switchbacks, 2.6 m clear of the interactive
   trees, large stumps and boulders, 7 m off the river). Its result was eased by hand into `CONTROL`. A
   route that moved with each re-grade of the ground could never be graded once and stay put.
2. **Paths at 1 in 7, not 1 in 8.** The meadow slope is 1 in 7.2 straight down it; at 1 in 8 the search
   zigzagged every 10 m. 1 in 7 is a steep coast path, still walked without steps, and gives three long
   switchbacks. The steps are unaffected.
3. **Steps straight down the fall line.** Below 45 m the side is about 1 in 2.3. Benching a 1 in 7 path
   across it would need 250 m of switchbacks with cut banks 2-3 m high; flights of 12 risers at 17 / 32 cm
   with 1.2 m landings average 1 in 2.5, close to the ground, so the cut stays under 2.5 m.
4. **Heights solved together.** The heights where legs meet are a least-squares fit to the ground, subject
   to each leg's limits (paths 1 in 7; stair legs 1 in 5 to 1 in 2.4, what the kit's flights and landings
   can make). The front door and the sand keep their levels. Path profiles between are the flattest-worst
   deviation within 1 in 7 (as `road_grade.py`), eased by 2 m vertical curves.
5. **The ground under the steps.** The heightfield is cut to the line 5 cm below each tread's back edge
   (the nosing line less one rise and 5 cm), and 5 cm under each landing, over 2.2 m either side (a
   tread edge's bilinear cell reaches 0.75 + 1.42 m), blending back to the ground over 2.5 m. Paths are
   cut to their bed over 1.2 m. The river channel (its half width plus 2 m) is never touched.
6. **Ends at the west sand.** CoveBeach (-650, -515) m is on the narrow strip under the east cliff, across
   the river mouth. The route ends on the sand at the head of the cove west of the mouth, which joins that
   strip across 0.3 m of water at the mouth; a crossing (stepping stones or a plank) can follow.
7. **One rail side per stair leg,** so it never swaps partway down: the side with the bigger drop, unless a
   rail there would stand in another leg's clear width (at the bench steps' hairpin). On landings the rail
   carries on the same side; at a corner landing it stops at the turn and a short level bay joins it to the
   next leg's rail, leaving no gap to slip through.

## Kit interface notes for Props

- **Raked rails on the other side are mirrored, not turned.** A raked bay turned 180 degrees about Z
  slopes the wrong way. Bays with `mirrored = 1` have the drop on their -Y: the builder scales them by -1
  in Y. Level bays are turned as the design says.
- **Corner landings** are 1.5 m along the lower leg and reach 0.75 m past the turning point; the upper
  leg's first riser is 0.75 m past it along its own heading (turns of 18-21 degrees). A small wedge at the
  outside of each turn shows cut ground 5 cm below the landing; a corner or wedge landing piece would
  close it.
- **Fingerpost yaws** are 159 and 153 degrees (the provisional 215 was a guess).
- **Landing lengths** vary (1.23-2.30 m); the builder will tile 1.2 m pieces or scale one along X.

## Open

- Runtime placement of the kit (flights, landings, kerbs, rails, pawn-only rail blockers, fingerposts)
  once Props' meshes are imported; until then the graded ground is walkable (1 in 2.3 at most, under her
  walkable floor angle).
- Paths through the forecourt pass the clear-out's stumps, rubble and saplings (clearable, as intended);
  two forage plants (a root patch at 96 m, a stand of flowers at 45 m) stand on or beside the path.
