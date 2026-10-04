# Travel Rest / measured build 02

- Build `20261003-measured-02`, authorized checkpoint `8b67ebc0`; clean/complete checkout confirmed. Target Oct 3 21:00; fallback Oct 4 07:30, no overnight work.
- App session `f84c58de-15cd-457f-bdc3-450ac4ecd859`; runtime folder `1b0e10a4-09b5-458e-b9de-098cce831b07`; branch `jennifergalley-travel-rest-agent`; worktree `E:\Repos\copilot-worktrees\SurvivalGame\jennifergalley-miniature-fishstick`.
- Source/launch: GPT-6.1 Sol (`gpt-6.1-sol`), high/default requested. Actual model matches runtime identity; actual reasoning/context and credit telemetry not yet verified. No helpers/subagents; task costs include planning/Notion identity reads, native checks and eventual compile. Usage incomplete, not billing-reconciled.
- Feedback mapping: `jenny-mut5v5yo-9f8fpx` -> `sleep-evening-until-dawn`; `jenny-mut5c51h-orhi39` -> `unlock-travel-by-visiting`; `jenny-mus1kfmm-ayz6jx` -> `wait-sunday-store-until-monday`.
- State: proposal/design/spec/tasks authored; implementation not started. OpenSpec propose requires a subsequent explicit apply message before source edits. Coordinator must send that message, then work resumes within slot 2.
- Reuse: `BedSleepOption`/`Sleep`/`Step`; `PlanTravel`/`WalkRoad`/ground-snap rollback; `NextShopOpening`/`WaitForShop` and two-press shop wait. Current lighting is fixed 06:00/18:00; no seasonal solar helper exists.
- Shared edits coordinated with Farming Fishing `5be207bc-49b1-4a1b-811e-088ae565dc1b`: only minimal State/discovery declarations/save hooks and sleep policy in Simulation; Shops wait functions only. Peer owns Item/Recipe/HarvestCrop/shop goods/trade. Integration/docs `e528fd4a-5aed-4c95-9463-a37941afc00b`.
- Proposed save addition (not implemented): optional counted `travel` section; exact final shape must be sent to coordinator before ready. No version/bake changes. Older readers reject unknown tags; preserve Jenny's live saves.
- Player checks live in each change's tasks; Jenny alone accepts in-game. No editor/PIE/UAT/packages/main pushes/shortcut/game interference. Compile requires coordinator slot.
- Owned scratch: none. No live processes/ports/releases/automations. Planning metadata created by installed OpenSpec CLI; source unmodified.
