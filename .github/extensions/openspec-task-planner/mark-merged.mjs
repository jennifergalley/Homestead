// Usage: node .github/extensions/openspec-task-planner/mark-merged.mjs --cards <ids...> --sha <sha>
//          [--note <text>] [--merged-at <ISO time>] [--clear] [--root <dir>] [--dry-run]
// The planner half of Scripts\Mark-Merged.ps1. Records the "On main, awaiting build" state for cards whose work is done and
// merged to main but not yet in a delivered build: priority.json `merged` maps a card ID to { sha, mergedAt, note }. The
// planner shows these cards in their own group and takes them out of the build slots and out of prioritisation.
// Idempotent: the same card with the same SHA changes nothing. Close-Delivery clears the state when the card ships.
import { readFile, readdir } from "node:fs/promises";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { normalizeMerged, shortSha } from "./build-slots.mjs";
import { atomicBacklogWrite, loadBacklogInbox, loadBuilds, loadPriority, priorityPath } from "./planner-data.mjs";

const defaultRoot = fileURLToPath(new URL("../../../", import.meta.url));
const backlogPrefix = "backlog:";

async function openSpecChangeNames(root) {
    try {
        const entries = await readdir(join(root, "openspec", "changes"), { withFileTypes: true });
        return entries.filter((entry) => entry.isDirectory() && entry.name !== "archive" && !entry.name.startsWith(".")).map((entry) => entry.name);
    } catch (error) {
        if (error?.code === "ENOENT") return [];
        throw error;
    }
}

export async function markMerged(projectRoot, options) {
    const { cards, sha = "", note, mergedAt, clear = false, dryRun = false, now = new Date() } = options;
    if (!Array.isArray(cards) || !cards.length || cards.some((id) => typeof id !== "string" || !id.trim())) {
        throw new Error("Give at least one card ID.");
    }
    if (!clear && !/^[0-9a-f]{7,64}$/i.test(sha)) throw new Error("Give the merge SHA (7 to 64 hex digits).");
    let at = now.toISOString();
    if (mergedAt) {
        const parsed = new Date(mergedAt);
        if (!Number.isFinite(parsed.getTime())) throw new Error("MergedAt is not a valid date/time.");
        at = parsed.toISOString();
    }
    const ids = [...new Set(cards.map((id) => id.trim()))];

    const builds = await loadBuilds(projectRoot);
    const priority = await loadPriority(projectRoot, builds, { strict: true });
    const merged = normalizeMerged(priority.merged);
    const result = { dryRun, sha: clear ? null : sha.toLowerCase(), marked: [], unchanged: [], cleared: [], unscheduled: [], notes: [] };

    if (clear) {
        for (const id of ids) {
            if (merged[id]) { delete merged[id]; result.cleared.push(id); }
            else result.unchanged.push(id);
        }
    } else {
        // Typos must not mark nothing: the card has to be on the board (an inbox entry or an OpenSpec change).
        const board = new Set([...(await loadBacklogInbox(projectRoot)).map((entry) => `${backlogPrefix}${entry.id}`), ...await openSpecChangeNames(projectRoot)]);
        const removed = new Set(priority.removed);
        const unknown = ids.filter((id) => !board.has(id) && !merged[id]);
        if (unknown.length) throw new Error(`Unknown card ID(s): ${unknown.join(", ")}. Use the IDs the planner shows (backlog:<inbox id> or an OpenSpec change name).`);
        const gone = ids.filter((id) => removed.has(id) && !merged[id]);
        if (gone.length) throw new Error(`Already removed or shipped, nothing to mark: ${gone.join(", ")}.`);
        for (const id of ids) {
            const existing = merged[id];
            const same = existing?.sha.toLowerCase() === result.sha;
            const nextNote = note ?? existing?.note ?? "";
            if (same && existing.note === nextNote) result.unchanged.push(id);
            else {
                merged[id] = { sha: result.sha, mergedAt: same ? existing.mergedAt : at, note: nextNote };
                result.marked.push(id);
            }
            if (id in priority.slots) { delete priority.slots[id]; result.unscheduled.push(id); }
        }
    }
    const changed = result.marked.length || result.cleared.length || result.unscheduled.length;
    result.changed = Boolean(changed);
    result.mergedNow = Object.keys(merged).length;
    if (!changed || dryRun) return result;
    const exists = await readFile(priorityPath(projectRoot), "utf8").then(() => true, (error) => {
        if (error?.code === "ENOENT") return false;
        throw error;
    });
    if (!exists) result.notes.push("No priority.json yet; created one.");
    await atomicBacklogWrite(priorityPath(projectRoot),
        JSON.stringify({ ...priority, merged, updated: new Date().toISOString() }, null, 2) + "\n");
    return result;
}

function parseArguments(argv) {
    const options = { cards: [], root: defaultRoot, dryRun: false, clear: false };
    for (let index = 0; index < argv.length; index++) {
        const key = argv[index];
        const take = () => {
            const value = argv[++index];
            if (value === undefined) throw new Error(`${key} needs a value.`);
            return value;
        };
        if (key === "--sha") options.sha = take();
        else if (key === "--note") options.note = take();
        else if (key === "--merged-at") options.mergedAt = take();
        else if (key === "--root") options.root = take();
        else if (key === "--dry-run") options.dryRun = true;
        else if (key === "--clear") options.clear = true;
        else if (key === "--cards") {
            while (index + 1 < argv.length && !argv[index + 1].startsWith("--")) options.cards.push(argv[++index]);
        } else throw new Error(`Unknown option ${key}.`);
    }
    return options;
}

if (process.argv[1] && fileURLToPath(import.meta.url) === process.argv[1]) {
    try {
        const { root, ...options } = parseArguments(process.argv.slice(2));
        const result = await markMerged(root, options);
        const tag = result.dryRun ? "[dry run, nothing written] " : "";
        if (options.clear) console.log(`${tag}Reopened ${result.cleared.length} card(s)${result.unchanged.length ? `; not merged: ${result.unchanged.join(", ")}` : ""}.`);
        else {
            console.log(`${tag}On main, awaiting build @ ${shortSha(result.sha)}: ${result.marked.length} marked, ${result.unchanged.length} already marked.`);
            if (result.unscheduled.length) console.log(`  taken out of the build slots: ${result.unscheduled.join(", ")}`);
        }
        for (const note of result.notes) console.log(`  note: ${note}`);
        console.log(`  ${result.mergedNow} card(s) now wait for the next build.`);
    } catch (error) {
        console.error(`mark-merged: ${error.message}`);
        console.error("Usage: node mark-merged.mjs --cards <ids...> --sha <sha> [--note <text>] [--merged-at <ISO time>] [--clear] [--root <dir>] [--dry-run]");
        process.exitCode = 1;
    }
}
