# Updated waves: qualified interchange candidate

This snapshot supersedes the wave bytes in `a3a6dee` for candidate inspection.
It is **not accepted art**: broad chunky locks and a conspicuous cut edge remain;
OpenSpec task 1.1 stays open. Prefer actual normal-game camera review before
further sculpting and preserve the original alternative if this looks worse.

The only hair-shaping difference from the early checkpoint is remapping the
end-band UVs below 120 cm to the admitted atlas's feathered tips. Source/export
texture references were also made strictly local. The neutral texture's bytes,
alpha, palette, rig, retained upper crown and nonhair geometry are unchanged.
All six FBXs and packed scenes have new hashes; never mix them with the early
manifest.

Frozen `LongWave-manifest.json` SHA-256:
`cb6e61cd7624cc5535053ba2d503d3655a064bfa48d936aa61fed6bc8880cac8`.
Neutral texture SHA-256:
`df985d44b9b708e32c478b86f0717c775c15825971d1fb2980f600dc3a0d3a55`.
Run `python Scripts\Characters\check_hairstyle_waves.py` for the independent
six-wave/hash/original-input check. Existing import paths, hair slot 7 and
neutral material setup remain as documented in `EARLY-WAVES.md`.

Runtime palette compatibility remains `013c70b`; no new Appearance change,
wardrobe authority or modular asset dependency is introduced by this snapshot.
Main owns import/cook and ordinary gameplay acceptance at its safe boundary.
