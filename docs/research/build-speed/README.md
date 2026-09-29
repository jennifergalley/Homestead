# Build speed on the shared PC (2026-09-28)

This note explains why lane builds took 4-16 minutes and queued for up to 50. Everything was measured on this machine: Ryzen 3700X (8 cores, 16 threads), 32 GB RAM, the UE 5.8 installed build and MSVC 14.44. The numbers come from UnrealBuildTool's own per-action log (`bShowPerActionCompilationTimes`) and from sampling processes every 5 seconds.

## What was slow

1. **Builds compiled one file at a time.**
   - At start-up, UBT caps parallel actions at free memory divided by 1.5 GB. With 30-40 agent sessions and one or two editors open, the log said "Requested 1.5 GB memory per action, 1.28 GB available: limiting max parallel actions to 1". Commit was at 91 of 95 GB.
   - UBA's own scheduler also stops starting processes above 85% commit and kills them above 95% (`UbaScheduler.h`: `memStartWaitPercent = 85`, `memStartKillPercent = 95`).
   - So even when UBT allowed 6 in parallel, only one `cl.exe` ran, because commit sat at 80-100%.
2. **Every compile loaded a PCH of 2 GB or more.**
   - The game module used the engine's shared UnrealEd PCH in editor builds (2.4 GB) and the Engine PCH in game builds (2.0 GB).
   - Each `cl.exe` peaked at 2.4-3.2 GB, twice UBT's 1.5 GB estimate. That peak is what drove the memory limit.
   - Wall time ran at about twice CPU time because of paging.
3. **The editor module had no PCH.** It was set to `PCHUsage = NoPCHs`, left over from the 09-20 authoring probe, so each of its 6 files parsed the UnrealEd headers from scratch.
4. **All worktrees shared one queue.**
   - The UBT mutex is keyed on the UBT install, not the project (`UnrealBuildTool.cs`: `GetUniqueMutexForPath("UnrealBuildTool_Mutex", <UBT dll>)`). Every worktree queues behind every other.
   - A build with nothing to compile still waits in that queue. In one measured run it waited 625 s before doing 689 s of work.
   - `Start-EditorMcp.ps1` built on every launch, and `Build-Game.ps1 -Package` queued twice: once for the editor, once for the game.
5. **Packaging itself is a small part.**
   - UAT BuildCookRun took 106 s: cook 84 s, stage 18 s, pak and compression 10 s.
   - The rest of a 20-40 minute package is the two compile queues plus the content bootstrap and imports.
   - `-SkipAssets` doesn't skip the bootstrap and imports; `-PackageOnly` does.

## Changes and their measured effect

| | Before | After |
| --- | --- | --- |
| Game-module PCH | shared UnrealEd, 2.4 GB | private `SurvivalGamePCH.h`, 1.2 GB (22 s to build) |
| Memory per unity blob | 2.6-3.2 GB | 1.2-2.6 GB |
| CPU per unity blob | 60-106 s | 47-90 s |
| Editor-module file (`HomesteadAgentPlayLibrary.cpp`) | 17 s CPU, 72 s wall, no PCH | 9 s CPU, 11 s wall |
| Full rebuild of the editor and game targets together (20 actions, both PCHs) | n/a | 178 s at 7 in parallel (about 750 CPU-s, so 4.2x) |
| The same rebuild after rebasing (22 actions) when memory was short (commit 96 of 98.5 GB) | | 804 s, one compile at a time |
| Partial editor-only rebuild (12 actions, run serially) | 689 s | |
| `Start-EditorMcp.ps1` / `Build-Game.ps1` when the targets are already built | full queue wait | 1.5 s (build skipped) |

**The private PCH.**
- `SurvivalGamePCH.h` holds the ~30 engine headers the module uses most.
- Every `.cpp` was syntax-checked on its own against it with `cl /Zs`, the same way adaptive unity compiles modified files. So each file still includes what it needs.
- The first check found 19 files that had leaned on the big shared PCH.
- When a new file needs a common engine type, add the header to `SurvivalGamePCH.h`. Don't pull in `UnrealEd`.

**`Scripts\Invoke-UnrealBuild.ps1`** is now the one way the scripts call UBT.
- **Skips UBT when nothing changed.** If this script already built the targets from the same sources, it doesn't call UBT at all. The stamp covers:
  - the git tree of `Source\` and the `.uproject`, including uncommitted and untracked files;
  - the engine version;
  - the target receipt's write time, so any other rebuild invalidates the stamp.
- **Builds several targets in one UBT run** (`SurvivalGameEditor+SurvivalGame`), so there's one queue wait instead of two.
- **Adds `-UBADisableRemote`.** Without it, every local build opened a UBA listener on 0.0.0.0:1345.
- **Keeps its own log** in `Saved\Logs\UnrealBuildTool-<targets>.log`. The default `%LOCALAPPDATA%\UnrealBuildTool\Log.txt` is shared by all worktrees and overwritten by each build.
- **Names the builds it's queued behind.**
- `-Force` builds anyway, and `-CheckOnly` reports whether the targets are current.

## Not changed, and why

- **MemoryPerActionBytes and ProcessorCountMultiplier.** The real per-compile peak is above UBT's 1.5 GB default, so lowering it would be unsafe. UBA's 85% commit gate would still serialize builds anyway. The lever is memory headroom, not UBT settings.
- **Parallel builds across worktrees (`-NoMutex`).**
  - It is technically feasible: project builds don't write the engine-level caches under `%LOCALAPPDATA%\UnrealEngine`.
  - Each build would need its own `-Log`, `TEMP` and `-UBARootDir`, plus `-UBADisableRemote`.
  - But while commit is above 85%, two builds would just share the one-process budget, so keep the mutex for now.
- **A compile cache shared across worktrees (UBA cache).** UBT can read and write a `UbaCacheService` (`-UBACache=host:port -UBAWriteCache`), and the service can store its data on E: (`-dir=`). But:
  - It needs detoured UBA, which means dropping `-NoUBA`. The detoured store defaults to `C:\ProgramData\Epic\UnrealBuildAccelerator` with 40 GB capacity; move it with `-UBARootDir`.
  - Without the experimental `bUseVFS`, PCH actions are cached only when the absolute paths match (`VCToolChain.cs`: `ArtifactMode.AbsolutePath`). Every worktree has its own path and every compile depends on its PCH, so hits across worktrees would be close to zero.
  - It is worth a trial in a quiet window, using `-VFS` plus a cache service on E:. That would be a separate change, and it needs Jenny's approval because it's a background service.
- **Smaller unity blobs** (`NumIncludedBytesPerUnityCPPOverride`).
  - While builds are memory-limited and run serially, smaller blobs only add total CPU time.
  - Revisit once builds regularly run 4 or more files in parallel.
  - `Module.SurvivalGame.1` (Controller, Character and 23 `.gen.cpp` files) is the longest pole at about 80 CPU-s.
- **Iterative cook.** The loose-file cook (`-SkipZenStore`) does a full cook every time ("FULL COOK: ... -legacyiterative was not specified"). But it only takes 84 s.

## Needs Jenny's decision (machine-wide)

- **Memory headroom is the biggest lever.**
  - The same full rebuild took 178 s with 7 compiles in parallel (commit at 61 of 79 GB) and 804 s one at a time (commit at 96 of 98.5 GB).
  - At measurement time, about 38 `copilot` processes held 26 GB of commit, and each open editor held 15-16 GB.
- **Headroom needed for N parallel compiles.** With the private PCH, a compile peaks at 1.2-2.6 GB (the editor module's, on the UnrealEd PCH, at about 2.5 GB). Two gates must both pass:
  - UBT's start-up check needs N x 1.5 GB of *available physical* memory: 6 GB for 4 compiles, 9 GB for 6.
  - UBA won't start a process while commit is above 85% of the limit. So free commit must cover N x ~2.6 GB plus 15% of the limit (about 12-15 GB at a 80-98 GB limit): about **23-26 GB free commit for 4 compiles, 28-31 GB for 6**.
- **Move the pagefile.** The system-managed pagefile is on C: (52 GB allocated). A fixed pagefile on E: or D: would free space on C: and raise the commit limit. It needs admin rights and a reboot.
- **More RAM.** Going from 32 GB to 64 GB would remove the memory limit for 3 or more lanes.
