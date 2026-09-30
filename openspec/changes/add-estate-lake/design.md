# Design

1. **Up-left, not north-east.** The field-book map is north-up (`HomesteadMapGeometry::WorldToUV`), so
   her "up and to the left" is north-west. The orchestrator agreed to match the map.
2. **A dammed shelf pool, not a hollow.** There's no natural hollow within reach of the farm; the ground
   rises 5% to the north-west. So the lake is cut into the slope with a low pond bay on the downhill
   side, the way estate ponds were made. The section is designed (shelf, lip, cut, bay) rather than
   carved by minimum/maximum, because the downhill shore needs fill.
3. **Grade once.** The path and the fade at the reach blend by weight, so grading twice isn't a no-op.
   Once graded, `lake_basin.py` only reapplies the scenery. To regrade, restore the heightfield and
   delete "lake" from the layout.
4. **The shoreline is the waterline.** The spline's scale Y is 0 (the river uses it as a half-width), and
   the controller treats a closed spline as a polygon: inside is in the water, the pail aims 25 cm inside.
5. **A wading wall.** The bed reaches 1.8 m, deeper than she is tall. An invisible pawn-only wall 2.2 m in
   from the shore (about knee deep over the 1 in 4 shelf) stops her, and doesn't catch the camera or
   traces.
6. **Still water from the creek graph.** The creek material's drift is authored into its panners, so
   `M_EstatePond` is the same graph built with 8% of the drift. `MI_EstatePond` darkens it to a peaty
   pool: it absorbs blue fastest.
