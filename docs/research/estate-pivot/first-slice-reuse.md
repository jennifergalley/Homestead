<!-- Research-agent report, 2026-09-27, for the first slice of openspec/changes/pivot-to-cozy-estate-life-sim. Unverified items are flagged inline and must be confirmed before shipping. -->

# Homestead — Reuse & Feasibility Research Report

## 1. UK open elevation data for Cornwall

| Dataset | Licence | Resolution | Format | Account needed? | Coastal coverage |
|---|---|---|---|---|---|
| **EA LIDAR Composite DTM (1m & 2m)** | Open Government Licence v3.0 (OGL) | 1 m or 2 m grid | GeoTIFF, 5 km tiles | No — direct download via map selection | LIDAR is airborne and **stops at the tideline / mean low water**; there is no seabed data in this product |
| **OS Terrain 50** | OGL v3.0, "Open Data" product | 50 m grid | ASCII Grid, GML 3.2.1 (contours also in Shapefile/GeoPackage/MBTiles) | No | Includes mean high/low water contour lines but is far too coarse for cliff/cove sculpting |
| **OS Terrain 5** | **Not fully open** — mid-resolution DTM (~2 m RMSE accuracy) sits in OS's Land & Terrain portfolio; the OS product page and OS Data Hub only offer **sample tiles** as open data, full national coverage requires a paid/Premium OS Data Hub plan | 5 m grid | ASCII Grid, Shapefile, GML | Yes, for full coverage (OS Data Hub account); samples only require registration | Same land-only limit as other DTMs |

Sources: Defra Data Services Platform download portal and metadata record (environment.data.gov.uk/DefraDataDownload/?Mode=survey; environment.data.gov.uk/dataset/13787b9a-26a4-4775-8523-806d13af58fc); OS Terrain 50 overview (docs.os.uk/os-downloads/products/land-and-terrain-portfolio/os-terrain-50/os-terrain-50-overview) confirms "It is an Open Data product... free to view, download and use for commercial, educational and personal purposes"; OS Terrain 5 product page (ordnancesurvey.co.uk/products/os-terrain-5) and sample-data page (osdatahub.os.uk/downloads/sample-data/OsTerrain5) indicate sample-only open access.

**Attribution wording.** For EA data under OGL, GOV.UK's Environment Agency Conditional Licence gives the standard form: *"Contains Environment Agency information © Environment Agency and/or database right [year]"*, and when combined with OGL the recommended combined form is *"Contains Environment Agency information © Environment Agency and/or database right [year], licensed under the Open Government Licence v3.0."* (gov.uk/government/publications/environment-agency-conditional-licence). For OS Open Data the standard OGL form is *"Contains OS data © Crown copyright and database right [year]"* — I could not re-verify the exact current OS wording on a docs.os.uk attribution page in this pass; treat this as the commonly-cited form and confirm against the live OS Terrain 50 licence page before shipping.

**Bathymetry (for the sea-floor near cliffs/cove).** EMODnet Bathymetry provides a pan-European DTM (GeoTIFF/NetCDF/ASCII) free to view and download via the Map Viewer (emodnet.ec.europa.eu/en/bathymetry), generally under an open/free-reuse policy attributing EMODnet. UKHO/ADMIRALTY publishes bathymetric survey data via the ADMIRALTY Marine Data Portal (data.admiralty.co.uk/portal/apps/sites/#/marine-data-portal) and a data.gov.uk-listed "Bathymetry... in UK EEZ" dataset described as OGL (data.gov.uk/dataset/7ee5a832-1e48-4e17-82e5-a4cf43227300). **Caveat:** I was not able to independently open the ADMIRALTY portal's licence terms page in this pass (only aggregator descriptions), so treat "OGL" for UKHO bathymetry as reported-but-unverified until you read the licence text on data.admiralty.co.uk directly.

**Recommendation for Homestead:** Use EA LIDAR Composite DTM 1 m as the primary source for the estate/valley/cliff area (free, no account, correct resolution); fall back to OS Terrain 50 only for far-field context blending. For the seabed drop-off in front of the cliffs (visual only, not gameplay-critical), pull a coarse EMODnet tile and hand-sculpt the transition — don't rely on it for precision since LIDAR and bathymetry won't tile seamlessly at the tideline.

## 2. Candidate 4×4 km Cornish stretches

1. **St Agnes–Trevaunance–Chapel Porth–Wheal Coates (north coast, ~SW700505)** — Best single match: Trevaunance Coombe is a wooded valley/stream running to a small cove with cliffs; Wheal Coates and other engine houses sit directly on the clifftop (real historic reference only); St Agnes village serves as the inland town, though it isn't at an estuary head. Would need to invent an estuary/harbour town elsewhere or fictionalize St Agnes's setting. (cornishmining.org.uk/areas/st-agnes-mining-district; downthecove.com/knowledge/st-agnes)
2. **Portreath (~SW655455)** — a real former mining harbour/port at a valley mouth with steep cliffs either side; good "small harbour town at valley head" template, though it's a true harbour rather than an estuary.
3. **Perranporth / Perran Sands (~SW755545)** — long sandy beach backed by extensive dunes, adjoining St Agnes mining coast; ideal template for the "towans/dunes" biome requirement, paired with candidate 1 to the west.
4. **Hayle Towans / Hayle Estuary (~SW564394 for the Towans; Hayle town at the estuary head)** — genuine estuary-head town with a historic harbour and the largest dune system on this coast (St Ives Bay). Good literal template for "inland town at the head of an estuary with a harbour," but it's flatter/more urban than a single small estate cove. (cornwall-beaches.co.uk/north-coast/hayle-towans-map.htm; Wikipedia "St Ives Bay")
5. **Helford River (south coast, Helford village ~SW757260)** — wooded valley, sheltered south-facing cove/creek system, very Cornish-cove aesthetic (Daphne du Maurier's Frenchman's Creek), but this coast lacks the exposed cliffs/engine-house/moorland-and-tor backdrop of the north coast, and has no dunes nearby. (wikishire.co.uk/wiki/Helford)

None of these real 4×4 km boxes contains *every* requested feature (cove+cliffs+valley+estuary town+moor/tors+dunes) simultaneously — the brief's composite is a designer's amalgam. Moorland/granite tors (e.g. around Carn Brea/St Agnes Beacon or further afield on Bodmin Moor) are not co-located with dunes in reality.

**Recommendation for Homestead:** Base terrain on **St Agnes/Trevaunance/Chapel Porth (candidate 1)** for the estate/cove/cliff/engine-house core (best real analogue with mining heritage), splice in dune texture/shape cues from **Perranporth (candidate 3)** for the "towans" stretch, and fictionalize the estuary town rather than importing Hayle's real DTM wholesale — stitching real LIDAR tiles from unrelated areas will create seams and requires reshaping regardless, so treat all of this as reference/inspiration, not a literal mosaic.

## 3. UE 5.x heightmap import facts

- Landscape heightmaps must satisfy the component/section math in Epic's Landscape Technical Guide (dev.epicgames.com/documentation/unreal-engine/landscape-technical-guide-in-unreal-engine): section size is a power-of-two value (max 256×256) minus 1 (1 section/component) or minus 2 (2×2 sections/component); Epic recommends **a maximum of 1024 Landscape Components** for the largest landscapes.
- A commonly-cited valid configuration for ~4 km at 1 m/quad is **4033×4033** vertices (4032 quads/side) = 63 quads/section, 2×2 sections/component, 32×32 components (1024 components total) — this matches the 1024-component ceiling exactly. (numivo.org/tools/unreal-landscape-size; community tutorial dev.epicgames.com/community/learning/tutorials/KJ7l/landscape-import-basics) — verify these exact numbers against the current 5.8 docs before committing, as the community sources are secondary, not the primary Epic page.
- Formats: 16-bit grayscale **PNG** or **RAW (.r16)**, imported via Landscape mode → Manage → **Import from File**.
- Default XY scale of 100 = 1 m/quad; **Z scale** maps the 16-bit range (0–65535, mid value 32768=0 elevation) to world Z in the same cm-based unit system — for a given real elevation range you must rescale your source data into that 16-bit span before import (see GDAL pipeline below) and set the Landscape's Z scale to match your chosen metre-per-unit mapping.
- **World Partition** landscapes are split into Landscape Streaming Proxies automatically per grid cell so far terrain can stream in/out; this is documented as the standard approach for large open worlds in 5.x's Open World template.
- No built-in "import raw GeoTIFF directly" tool ships with vanilla UE Landscape; Cesium for Unreal handles georeferenced terrain (a different production workflow, generally overkill for a single fixed 4×4 km map) — the practical route is **QGIS/GDAL → 16-bit PNG or .r16 → Landscape import**.

**GDAL pipeline** (concrete, adapt values to your DEM):
```bash
# 1. Clip and reproject the source LIDAR/OS Terrain 50 GeoTIFF to a local metre-based CRS (e.g. OSGB36 / EPSG:27700 is already metric for GB)
gdalwarp -t_srs EPSG:27700 -te <xmin> <ymin> <xmax> <ymax> -tr 1 1 -r bilinear src.tif clipped.tif

# 2. Inspect min/max elevation to build a scale mapping
gdalinfo -mm clipped.tif

# 3. Rescale into 16-bit range and resample to a valid landscape resolution (e.g. 4033x4033)
gdal_translate -ot UInt16 -scale <src_min> <src_max> 0 65535 -outsize 4033 4033 clipped.tif heightmap.png
```
(forums.unrealengine.com/t/attempting-to-import-elevation-data-geotiff-into-unreal-engine-5-5/2118619; gis.stackexchange.com/questions/206537)

**Recommendation for Homestead:** Clip/merge LIDAR tiles in QGIS, hand-sculpt the composite (cove/valley/road) in a raster editor or directly in-editor, then run the `gdalwarp`→`gdal_translate` pipeline once to produce a validated 4033×4033 (or similar power-of-two-derived) 16-bit PNG, and import via World Partition's Open World template.

## 4. Water / Landmass / PCG / Landscape splines status

- **Water & Landmass plugins**: forum reports (forums.unrealengine.com/t/ue5-big-landscape-world-partition-landmass-plugin-water-plugin/240148) and Epic's own Water docs (dev.epicgames.com/documentation/unreal-engine/water-system-in-unreal-engine) describe Water as a self-contained, still-evolving plugin; multiple community threads report severe **editor** slowdowns editing rivers (spline-heavy) on large, high-component-count World Partition landscapes with Landscape Edit Layers enabled, while oceans/lakes are comparatively stable. Runtime rendering uses tile-based LOD/frustum culling and is generally fine; the risk is editor iteration speed, not shipped performance. I could not confirm from a primary Epic source whether Water is still formally labelled "Experimental" in 5.8 vs. reclassified — treat as **unclear/likely still evolving** and validate directly in your target engine version.
- **Single Layer Water material**: this is a material shading model built into the base renderer and can be used without enabling the Water plugin (for a hand-placed static ocean/lake mesh), giving you a lower-risk fallback if the full Water system proves troublesome for your scale.
- **PCG framework**: Epic's official PCG framework doc (dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-in-unreal-engine) is a live, actively maintained page and secondary coverage (nhance-school.com/articles/ue5-5-release) describes PCG as advancing toward production maturity by 5.5 with a new **Procedural Vegetation Editor** for Nanite-ready foliage. However, Epic's own **PCG Biome Core and Sample Plugins** docs (dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-pcg-biome-core-and-sample-plugins-overview-guide-in-unreal-engine) explicitly label the biome sample content as **experimental** as of the current docs — so "PCG is production ready" is true for the core scattering/foliage graph system but **not** for the ready-made biome sample plugins, which should be treated as a reference to copy/adapt rather than a hardened dependency.
- **Landscape splines**: the standard, mature workflow (long-shipped, not new/experimental) for a dirt road — splines can deform the landscape (cut/raise) and place spline meshes (road segments, verges) along their length; this is well-trodden and low-risk.

**Recommendation for Homestead:** Use Landscape splines for the road (safe, mature). Use core PCG graphs for foliage/biome scattering but write your own biome graphs rather than depending on the experimental Biome Core sample plugins. Prototype the Water plugin's river+ocean combo early on a cut-down version of your actual heightmap to de-risk editor performance before committing; keep Single Layer Water material as a fallback plan for the ocean if Water/Landmass proves too costly in editor.

## 5. Free town/building/interior assets

- **Fab Standard License** (dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab; fab.com/eula): permits use, modification, and commercial distribution of assets **incorporated into a larger project** (i.e., a shipped game), across any compatible engine/tool, but **forbids reselling/redistributing the asset standalone**. Tiers are "Personal" vs "Professional" based on the buyer's trailing-12-month gross revenue ($100k threshold), not on the asset's usage rights — both grant the same in-game usage rights.
- **Quixel Megascans**: the fully-free-and-unlimited era for Megascans on Fab **ended after 31 December 2024**; anything already claimed by that date keeps its granted rights, but new access from 2025 onward is limited to a smaller free subset (~1,500+ assets) with the rest requiring purchase/subscription (forums.unrealengine.com/t/reminder-free-megascans-ends-soon/2203090; digitalproduction.com/2024/09/19/quixels-megascans-no-longer-free-after-2024). **This is a "claim before a deadline" situation your team likely already missed for the full library** — check what was claimed historically on the studio's Epic account rather than assuming full access today.
- **Free/CC0 medieval town packs outside Fab**: Quaternius's Medieval Village Pack (quaternius.com/packs/medievalvillage.html) is CC0, FBX/OBJ/BLEND, engine-agnostic, no attribution required — a strong option for blockout town geometry, though its aesthetic is stylized/low-poly rather than Victorian-Cornish, so it's best for temporary blockout, not final art.
- Fab's own "Free" browsing category and periodic "Free for the Month" promos (unrealengine.com/fabfreecontent) do become **permanently owned** once claimed to your Epic account within the promo window — this requires an account/login, which per your constraints should not be done in this research pass; flag it for a human/dev to action separately.
- I found no verified, publicly-documented free Epic sample pack specifically themed "18th/19th-century English town/shopfront" (no primary source located); "Medieval Village"-style free packs found are more high-fantasy/generic-medieval than Victorian English coastal town, so expect to need custom Blender-authored shopfronts, counters, shelving, barrels, and crates regardless — which aligns with the project's existing procedural-Blender-props pipeline.

**Recommendation for Homestead:** Don't rely on premade town packs matching the Victorian-Cornish aesthetic — use CC0 kits (Quaternius) only for early blockout massing, and hand-author the general store, shopfront, and interior props (counter, shelves, barrels, crates) in Blender as the team already does, checking Fab's free tab periodically for reusable set-dressing (barrels/crates are generic enough to reuse across eras).

## 6. Minimap / world map approach

For a **fixed, hand-authored** landscape, a live per-frame `SceneCapture2D` is unnecessary overhead — community and Epic tutorial consensus (dev.epicgames.com/community/learning/tutorials/alEo/unreal-engine-scrolling-mini-map-widget-using-slate; forums.unrealengine.com/t/performance-optimization-with-scenecapture2d/1287354) is to **bake the top-down orthographic capture once** (either in-editor via a temporary `SceneCapture2D` set to Orthographic projection with unneeded show-flags disabled, or via a manual high-res screenshot) and export it as a static texture. At runtime, draw that texture in Slate with a clip/rotate brush and overlay a player-position marker computed by a simple world-to-UV linear transform; this avoids ongoing render cost entirely. Only re-capture if the world state changes (unlikely for the "walk your estate" fixed-terrain slice).

**Recommendation for Homestead:** Bake one high-resolution orthographic texture per zoom level (minimap crop + full world map) at build/editor time; render both from the same static texture in Slate with different UV crops/rotation — no per-frame capture needed for a static estate.

## 7. MetaHuman NPCs

Multiple secondary sources (cgchannel.com/2025/06/you-can-now-sell-metahumans-or-use-them-in-unity-or-godot; metahuman.com/news/metahuman-leaves-early-access-with-a-feature-packed-new-release; Unreal Engine 5.6 release notes) describe a June 2025 change coinciding with UE 5.6: **MetaHuman Creator is now integrated directly inside the Unreal Editor** (no separate web app — the standalone web tool is reportedly being sunset), and the EULA was updated so MetaHuman characters/animation can be used in **other engines and DCC tools**, not just Unreal, without the standard Unreal Engine royalty applying outside Unreal projects. Usage remains governed by the general Unreal Engine EULA revenue threshold (free under the standard consumer terms; commercial seat/royalty terms apply above roughly $1M lifetime/trailing revenue) for in-Unreal use. **I was not able to load the primary metahuman.com/license page directly (it redirected through an Epic login wall)**, so treat the exact revenue thresholds and "MIT open-source libraries" claim from secondary aggregation as **unverified — confirm against the actual EULA text before shipping**, especially since Homestead is shipping fully in Unreal Engine 5.8 and the "other engines" clause is likely not the operative one for you anyway.

No primary Epic performance/LOD guidance for "several MetaHumans in one interior" was located in this pass; general Unreal practice (MetaHuman LOD system + Nanite-skinned-cloth/hair groom LODs, capped at fewer simultaneously-visible high-LOD grooms) applies, but this specific claim is **not verified** here and should be checked against dev.epicgames.com/documentation/metahuman performance pages directly.

**Recommendation for Homestead:** Since the game ships purely in Unreal, the "other engines" licence change is not load-bearing for you — treat the shopkeeper MetaHuman as a standard in-Unreal usage under the normal Unreal Engine EULA revenue threshold, but have a human confirm current EULA text (metahuman.com/license or unrealengine.com/eula/mhc) since this agent could not load it.

## Licence/attribution checklist

- [ ] EA LIDAR DTM: OGL v3.0 — credit *"Contains Environment Agency information © Environment Agency and/or database right [year], licensed under the Open Government Licence v3.0"*
- [ ] OS Terrain 50 (if used for context blending): OGL v3.0 — credit *"Contains OS data © Crown copyright and database right [year]"* (verify exact current wording on docs.os.uk before shipping)
- [ ] OS Terrain 5: do **not** assume full free coverage — confirm licensing tier on OS Data Hub before use beyond samples
- [ ] EMODnet / UKHO bathymetry: confirm licence text directly on emodnet.ec.europa.eu and data.admiralty.co.uk before use (not independently verified here)
- [ ] Fab assets: Standard License — in-game use OK, no standalone redistribution, no required credit
- [ ] Quixel Megascans: confirm what your studio's Epic account already claimed pre-2025-01-01; new claims are limited/paid now
- [ ] Quaternius CC0 packs: no attribution required, blockout-only recommended
- [ ] MetaHuman: confirm current EULA revenue threshold and terms directly (this report could not load metahuman.com/license)

## Sources cited
- https://environment.data.gov.uk/DefraDataDownload/?Mode=survey
- https://environment.data.gov.uk/dataset/13787b9a-26a4-4775-8523-806d13af58fc
- https://www.gov.uk/government/publications/environment-agency-conditional-licence/environment-agency-conditional-licence
- https://docs.os.uk/os-downloads/products/land-and-terrain-portfolio/os-terrain-50/os-terrain-50-overview
- https://www.ordnancesurvey.co.uk/products/os-terrain-5
- https://osdatahub.os.uk/downloads/sample-data/OsTerrain5
- https://emodnet.ec.europa.eu/en/bathymetry
- https://data.admiralty.co.uk/portal/apps/sites/#/marine-data-portal
- https://www.data.gov.uk/dataset/7ee5a832-1e48-4e17-82e5-a4cf43227300/bathymetry-maritime-limits-and-boundaries-and-ships-routeing-measures-in-uk-eez
- https://www.cornishmining.org.uk/areas/st-agnes-mining-district
- https://www.downthecove.com/knowledge/st-agnes
- https://www.cornwall-beaches.co.uk/north-coast/hayle-towans-map.htm
- https://en.wikipedia.org/wiki/St_Ives_Bay
- https://wikishire.co.uk/wiki/Helford
- https://dev.epicgames.com/documentation/unreal-engine/landscape-technical-guide-in-unreal-engine
- https://www.numivo.org/tools/unreal-landscape-size
- https://dev.epicgames.com/community/learning/tutorials/KJ7l/landscape-import-basics
- https://forums.unrealengine.com/t/attempting-to-import-elevation-data-geotiff-into-unreal-engine-5-5/2118619
- https://gis.stackexchange.com/questions/206537/geotiff-to-16-bit-tiff-png-or-bmp-image-for-heightmap
- https://forums.unrealengine.com/t/ue5-big-landscape-world-partition-landmass-plugin-water-plugin/240148
- https://dev.epicgames.com/documentation/unreal-engine/water-system-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-framework-in-unreal-engine
- https://dev.epicgames.com/documentation/unreal-engine/procedural-content-generation-pcg-biome-core-and-sample-plugins-overview-guide-in-unreal-engine
- https://dev.epicgames.com/documentation/fab/licenses-and-pricing-in-fab
- https://www.fab.com/eula
- https://forums.unrealengine.com/t/reminder-free-megascans-ends-soon/2203090
- https://digitalproduction.com/2024/09/19/quixels-megascans-no-longer-free-after-2024
- https://quaternius.com/packs/medievalvillage.html
- https://www.unrealengine.com/fabfreecontent
- https://dev.epicgames.com/community/learning/tutorials/alEo/unreal-engine-scrolling-mini-map-widget-using-slate
- https://forums.unrealengine.com/t/performance-optimization-with-scenecapture2d/1287354
- https://www.cgchannel.com/2025/06/you-can-now-sell-metahumans-or-use-them-in-unity-or-godot
- https://www.metahuman.com/news/metahuman-leaves-early-access-with-a-feature-packed-new-release
- https://www.unrealengine.com/news/unreal-engine-5-6-is-now-available

**Gaps/uncertainties flagged:** exact current OS attribution wording; UKHO ADMIRALTY licence terms (secondary-source only); Water/Landmass current experimental-vs-production label in 5.8; MetaHuman EULA text (page inaccessible, login-walled); no primary source found for a "free 19th-century English town" Epic sample pack; PCG "production ready" applies to core graphs, not the biome sample plugins (which remain documented as experimental).
