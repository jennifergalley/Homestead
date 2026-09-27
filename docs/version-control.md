# Version control

The project uses Git, with Git LFS for Unreal assets, FBX exports, Blender sources,
textures and audio. Keep LFS installed when cloning or switching revisions.
The private remote is `https://github.com/jennifergalley/SurvivalGame`.

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
and small, verified commits. Preserve unrelated user edits.

Candidate packages belong under a versioned `Build\Releases` directory. Never
overwrite the running player's known-good `Build\Windows` package merely because
new source compiled. No force-pushes, destructive resets, or automatic public
visibility changes are part of this workflow.

## Parallel worktrees

Each agent session works in its own git worktree under
`E:\Repos\copilot-worktrees\SurvivalGame\<name>` (never on C:), on its own branch. Machine-wide
rules for editors, ports and builds are in section 0 of `.github\skills\unreal-editor-mcp\SKILL.md`.

- **Delivering to `main`:** `git pull --rebase origin main` (add `--autostash` if you have
  uncommitted work), rerun the relevant check, then `git push origin HEAD:main`. Keep commits small
  and push often; long-lived lane branches conflict in shared files (`HomesteadItems.cpp`,
  `SHomesteadMenu`, saves).
- **Close your editor before `pull`/`rebase`.** It holds `.uasset`/`.umap` files open, and git fails
  with `unable to unlink ... Invalid argument`.
- **Don't `git stash -u`** while a sub-agent may be writing files in the worktree; it sweeps up
  their new files. Use `--autostash`, which only stashes tracked changes.
- **Only commit your own assets.** The editor dirties shared maps (`Estate.umap`,
  `__ExternalObjects__`) and probe content (`Content/Trials/Probe/`); leave those out unless your
  lane owns them.
- **Line endings:** `core.autocrlf=true` with `* text=auto`, so working copies are CRLF while files
  an agent just wrote may be LF. Detect the newline before a PowerShell string `.Replace()` with
  `` `n ``, or use the edit tool.
- **`mvp-survival`** is a separate long-lived product line. Never merge it with `main` in either
  direction.
