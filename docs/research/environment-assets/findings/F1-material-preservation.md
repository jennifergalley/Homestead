# F1 - Acquiring better meshes is not sufficient

Status: directly supported local integration finding; visual result untested.

The current `AddDecoration` overwrites all mesh slots with a tint-derived
material, and the importer disables material/texture import. A tree FBX swap
alone therefore does not establish preservation of separate bark and leaf PBR.
The planned explicit authored-material path addresses the cause instead of
changing global exposure or merely increasing mesh complexity.

Evidence: `integration-evidence.md`; S01-S03 record distinct source materials/
alpha channels. This is a local code-path observation, not a broad claim that
every natural asset needs the same material graph.
