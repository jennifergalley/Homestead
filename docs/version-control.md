# Version control

The project uses Git, with Git LFS for Unreal assets, FBX exports, Blender sources,
textures and audio. Keep LFS installed when cloning or switching revisions.

Tracked content includes source, configuration, scripts, design documents,
licensed authored/imported assets, and provenance. Build outputs, saved games,
recorded playtests, derived-data caches, vendor asset downloads and tool bundles
are not repository content.

The supplied private character reference is explicitly excluded. Do not force-add
it, derived composites containing it, authentication files, or personal saves.
Generated runtime status is local, not a reason to make a commit every heartbeat.

## Restore

```powershell
git lfs install --local
git lfs pull
.\Scripts\Fetch-Assets.ps1
.\Scripts\Test-Native.ps1
```

The game still requires the documented Unreal/toolchain setup to build. Optional
character authoring uses the separately pinned Blender/MPFB workflow.
Downloaded vendor sources are reproducible through the acquisition scripts and
retained source/license manifests.

## Autonomous work

Commit a known-good baseline before changes, then use a focused worker branch
and small, verified commits. A Git repository does not itself make folder-backed
Copilot sessions isolated: until a separate worktree is explicitly verified, keep
only one coding writer in this folder. Preserve unrelated user edits.

Candidate packages belong under a versioned `Build\Releases` directory. Never
overwrite the running player's known-good `Build\Windows` package merely because
new source compiled. No force-pushes, destructive resets, or automatic public
visibility changes are part of this workflow.
