# Agent lifecycle and build credits

Jenny's policy, 2026-10-02. Quality, especially visuals and performance, remains a shipping gate;
AI credits are an additional optimization metric, not permission to lower that gate.
The role/model table in README.md is authoritative. This policy does not retune an existing session.

## Compact, durable handoffs

Every agent, including the orchestrator, updates its task handoff at meaningful checkpoints, before
parking or replacement, and before eligible archival. Use the existing OpenSpec task/design files for
feature status; link them from the round registry rather than duplicating them. For coordination or
tooling work without a change, use a small named handoff under docs/handoff/.

A handoff contains:

- Task/change and target build; session ID, branch, worktree, latest SHA and pushed/integrated status.
- Actual model, reasoning effort and context tier, with changes recorded as separate segments.
- Done, incomplete, blocker and the exact next action; preserve Jenny's direct decisions.
- The few relevant files/functions/assets and reusable helpers; links to canonical docs, not copied docs.
- Commands already run, results and evidence paths; failed approaches worth avoiding.
- Any live process, editor port, release/shortcut dependency, owned scratch or save-safety concern.
- Usage snapshot/export reference, coverage and task/build allocation; unknown fields stay unknown.

Keep it sufficient to resume without rereading an entire conversation or exploring the whole repo.
Update current state in place; historical narratives and large logs are not mandatory startup reading.
Commit handoff updates with coherent work checkpoints. Do not generate agent chatter just to report a write.

## Reuse versus replacement

Bias toward a fresh session for a new objective, unrelated subsystem or noisy/obsolete history.
Reuse for an immediate fix, review response or next step in the same investigation when the agent
already has useful context. Do not restart halfway through a coherent task just to satisfy a timer.
At a natural task/build boundary, checkpoint and replace if reconstruction from the handoff is cheaper
than carrying the old history. Compare observed credits and rework; there is no universal turn limit.

Before eligible archival: persist work and the handoff, capture usage including handoff-writing costs,
and verify no ongoing work, attached automation, live shortcut release or other protected dependency.
Jenny's old agents remain protected until she chooses to archive them. An orchestrator cannot archive
itself; it leaves a replacement handoff. No work or automations run overnight.

## Right-sized delegation

Agents may delegate bounded support work to cheaper, task-scoped sub-agents when expected savings
exceed startup, briefing and review costs. The owning agent remains accountable for correctness.
Prefer direct execution for tiny tasks; use a separate project session only when isolation or an
independent workstream warrants it. Supply an objective, relevant files or excerpts, constraints,
expected output and acceptance criteria, not the entire parent conversation.

Keep delegation one level deep by default. The three-hands-on-implementer cap applies across the
whole tree, including helpers while they edit, build or use Blender/Unreal. Request a slot before
starting a hands-on helper if all slots are occupied. All descendant usage belongs in task/build costs.
Cheap helpers can document verified facts or execute existing tests. The high-effort owning agent
defines gameplay test behavior and edge cases and reviews helper-written cases; substantive gameplay,
animation or rendering diagnosis remains GPT-6.1 Sol / high. Jenny's direct instructions still win.

## Accounting contract for future builds

This is the requested reporting contract, not an implemented collector or planner feature.

- Each accounting segment identifies the build, task/change, session/agent, actual model,
  reasoning effort, configured context tier, timestamps and usage-event range.
- Export per-call token classes and recorded nano-AI units where available. Store integer units and
  source identifiers; avoid rounded intermediate sums. Validate unit conversion against runtime/billing
  evidence before labeling the display "AI credits". Never substitute premium-request multipliers.
- Input totals may already include cached/read/write tokens. Use the token-type breakdown and its
  supplied rates; do not charge aggregate input again or add reasoning tokens twice.
- Include implementers, nested sub-agents, review, documentation, orchestration, integration, failed
  attempts and retries. Count each usage event once; do not add inclusive parent totals to children.
- Use snapshot deltas or event ranges for reused sessions, not their lifetime total for every build.
  Record model/effort changes and actual prompt size separately from the configured context tier.
- Work deferred to a later build carries its incurred cost with it. Shared coordination/tooling goes
  in an explicit overhead category with a documented allocation; never silently omit or double-count it.
- Label totals as recorded, estimated, reconciled or incomplete. Missing coverage is visible, not zero.
  Reconcile against GitHub's AI usage report without attributing Jenny's unrelated project usage here.
- The planner should show build total and task/session/model/configuration breakdowns, including
  input, cache read/write and output cost, overhead/rework and coverage. Reading the report must not
  invoke an agent; keep existing agent-free feedback and scheduling behavior unchanged.

Optimize credits per accepted player-visible improvement alongside build totals, defects/rework,
performance and visual acceptance. Compare similar task types rather than concluding that the more
expensive build was inefficient merely because it contained harder work.

## Verified sources and telemetry limits

Checked 2026-10-02:

- [GitHub pricing](https://docs.github.com/en/copilot/reference/copilot-billing/models-and-pricing):
  token-type rates, AI credit units and long-context thresholds differ by model.
- [GitHub billing reports](https://docs.github.com/en/billing/reference/billing-reports):
  AI exports group by date, model and user, not task or session.
- Local session-store assistant_usage_events contains model, reasoning_effort, input/output/cache
  tokens, token_details_json and total_nano_aiu. This makes retrospective usage export plausible,
  but task/build linkage and context-tier capture still need implementation and coverage checks.
- The installed SDK exposes usage metrics and subagent configuration events. Persist these before
  archival rather than depending on retention of runtime history.
- [OpenAI caching](https://developers.openai.com/api/docs/guides/prompt-caching) documents discounted
  matching prefixes; keeping a session does not guarantee a hit, and compaction can change the prefix.
- [Microsoft's Copilot workload study](https://www.microsoft.com/en-us/research/publication/agentic-coding-in-the-wild-characterizing-github-copilot-at-production-scale/)
  observes cache-hit variation across turns and invalidation after compaction/model switches. It does
  not establish that always restarting, or always retaining sessions, minimizes Homestead's cost.
