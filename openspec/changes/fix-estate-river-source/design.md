# Design

## Decisions

1. **Grade the heightfield, not just the ribbon.** On 16 % of the layout points one bank was missing
   (the least-cost path runs along a valley side), so no ribbon width could sit in the old channel.
   The grade is `min(max(z, bank), cut)`: the bank raises ground to the bank top out to a 0.8 m
   berm, then falls at 0.5 per metre. The cut holds the channel shape and rises at 0.6 per metre
   beyond it. Inside the channel the ground is exactly the designed section. The largest raise is
   1.9 m, where a parallel hollow runs beside the stream.
2. **The profile is stored.** The beach bed follows the sand down (0.3 m below it), which would dig
   deeper on every run. river_channel.py computes the profile once and keeps it in
   estate_layout.json (`riverChannel`, `riverSurface`, `riverHalfWidth`, `riverEnd`), so running it
   again is a no-op. reshape.py rewrites the layout without them, so a full rebuild recomputes the
   profile from fresh terrain.
3. **The r16 is the source of truth**, and the Landscape gets a diff. A whole-landscape import
   would dirty all 230 proxies. `ApplyEstateHeightfield` compares tile by tile and writes only the
   tiles that differ. Only the proxies it touched are saved, because loading the region leaves the
   others dirty with no real change. An early dry run seemed to find thousands of other
   mismatches, but they were all on the landscape's outermost row and column, which the data
   interface misreads. The rendered landscape matches the r16 there (within 1.1 cm), so the tool
   now skips that edge.
4. **Spline scale Y is the waterline**, as the controller already assumed. The mesh adds
   `BankOverlap`, and vertex colour is computed from the distance to the waterline, so the foam and
   shoreline highlight sit at the real edge.
5. **A separate material.** Re-authoring M_CreekWater in place crashed the editor (`!IsRooted` in
   the material editor) while the woodland creek used it. M_EstateRiver is built fresh from the same
   graph, with a white-water layer on vertex colour B. B is 0 on the woodland creek.
