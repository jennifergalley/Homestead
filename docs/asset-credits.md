# Asset credits

This project uses original prototype geometry and the following licensed sources.
`Assets\asset-manifest.json` records the exact download URLs and license references.
`Assets\download-receipt.json` records file sizes and SHA-256 hashes after fetching.

## Music

**Evening Fall (Harp)** by **Kevin MacLeod** (incompetech.com).
Licensed under [Creative Commons Attribution 4.0](https://creativecommons.org/licenses/by/4.0/).
[Official track page](https://www.incompetech.com/music/royalty-free/index.html?isrc=USUAN1100236).
Converted for game playback; playback fades applied. The in-game field book
includes this credit. Include this document in any distributed build.

## Public-domain assets

| Asset | Creator | Source | License |
| --- | --- | --- | --- |
| Brown Mud Leaves 01 | Rob Tuytel | https://polyhaven.com/a/brown_mud_leaves_01 | CC0 |
| Rock Moss Set 02 | Kless Gyzen | https://polyhaven.com/a/rock_moss_set_02 | CC0 |
| Fern 02 | Rob Tuytel (scanning), Rico Cilliers (modeling) | https://polyhaven.com/a/fern_02 | CC0 |
| Forest Ambience | TinyWorlds | https://opengameart.org/content/forest-ambience | CC0 |
| Impact Sounds | Kenney | https://kenney.nl/assets/impact-sounds | CC0 |
| Interface Sounds | Kenney | https://kenney.nl/assets/interface-sounds | CC0 |

The Fern 02 clearing candidate uses four separately imported meshes and the
publisher's 1K diffuse, DirectX normal, roughness, ambient-occlusion and alpha
maps. Source-axis/unit conversion was baked once; the project-authored masked,
two-sided material connects those maps. Exact source files, licenses and hashes
are recorded in `Assets\Environment\woodland-preparation-01`; import and preview
evidence is in `docs\research\environment-assets`. This does not imply that the
other acquired woodland assets are included in the playable candidate.

## Character prototype

The clothed heroine prototype uses MakeHuman Community / MPFB CC0 graphical
assets and an original procedural tunic and idle/walk clips. The relaxed idle
and grounded walk revision is also project-authored on that same rig; no
motion-capture service or replacement character asset was used. Full provenance,
tool-license distinctions, source URLs, and modifications are recorded in
`Assets\Characters\provenance.json` and `docs\character-pipeline.md`. The GPL
authoring tools are separate from the exported CC0 graphical assets.

The character is a provisional interpretation of the brief, not final appearance
approval or the complete character creator. Older technical packages may still
contain the labeled geometric stand-in; see their build status before testing.
The supplied portrait is local reference only, not a licensed game texture or a
file to publish. No Skyrim, Hades, Coral Island, or Dreamlight Valley assets are used.
