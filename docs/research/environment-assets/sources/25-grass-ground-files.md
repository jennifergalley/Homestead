# S25 - Grass Ground exact map choices

URL: https://api.polyhaven.com/files/grass_ground
Accessed: 2026-09-20 by the ground/sky worker.
Type: primary public file manifest, not downloaded binaries.

## Verbatim field values

Field-preserving subset; unrelated entries and MD5 fields omitted:

```json
{
  "Diffuse": {"2k": {"png": {
    "url": "https://dl.polyhaven.org/file/ph-assets/Textures/png/2k/grass_ground/grass_ground_diff_2k.png",
    "size": 23017857
  }}},
  "nor_dx": {"2k": {"png": {
    "url": "https://dl.polyhaven.org/file/ph-assets/Textures/png/2k/grass_ground/grass_ground_nor_dx_2k.png",
    "size": 24557309
  }}},
  "Rough": {"2k": {"png": {
    "url": "https://dl.polyhaven.org/file/ph-assets/Textures/png/2k/grass_ground/grass_ground_rough_2k.png",
    "size": 6773940
  }}}
}
```

These are publisher-declared URLs/sizes, not actual local receipts. Refresh before
acquisition and compute actual SHA-256 after download. PNG bit depth, binary
integrity, import behavior and runtime memory were not tested. The existing
three-JPG bootstrap will need explicit selected-file mapping, not an assumption
that these new maps use its old extension.

Credibility: high for offered inventory. Recency: live manifest, refresh on
acquisition. Bias: provider data; file sizes are not GPU-residency measurements.
