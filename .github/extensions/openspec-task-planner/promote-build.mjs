// Usage: node .github/extensions/openspec-task-planner/promote-build.mjs <shipped ids...> [--root <dir>]
// Integration's final step after a delivery: removes the shipped IDs from
// docs/handoff/priority.json, keeps unfinished "Next build" items there, and
// moves "Build after next" items up to "Next build". The write is atomic.
import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import { atomicBacklogWrite, deliveredBuildKeys, loadBuilds, loadPriority, priorityPath } from "./planner-data.mjs";
import { promoteAfterDelivery } from "./build-slots.mjs";

const defaultRoot = fileURLToPath(new URL("../../../", import.meta.url));

export async function promoteBuild(projectRoot, shippedIds) {
    const path = priorityPath(projectRoot);
    try {
        await readFile(path, "utf8");
    } catch (error) {
        if (error?.code === "ENOENT") return { written: false, before: {}, after: {} };
        throw error;
    }
    const builds = await loadBuilds(projectRoot);
    const before = await loadPriority(projectRoot, builds, { strict: true });
    const after = promoteAfterDelivery(before, shippedIds, deliveredBuildKeys(builds));
    await atomicBacklogWrite(path, JSON.stringify({ ...after, updated: new Date().toISOString() }, null, 2) + "\n");
    return { written: true, before: before.slots, after: after.slots };
}

function parseArguments(argv) {
    const ids = [];
    let root = defaultRoot;
    for (let index = 0; index < argv.length; index++) {
        if (argv[index] === "--root") {
            root = argv[++index];
            if (!root) throw new Error("--root needs a directory.");
        } else if (argv[index].startsWith("--")) {
            throw new Error(`Unknown option ${argv[index]}.`);
        } else {
            ids.push(argv[index]);
        }
    }
    if (!ids.length) throw new Error("Give at least one shipped item ID.");
    return { root, ids };
}

if (process.argv[1] && fileURLToPath(import.meta.url) === process.argv[1]) {
    try {
        const { root, ids } = parseArguments(process.argv.slice(2));
        const result = await promoteBuild(root, ids);
        if (!result.written) {
            console.log("No docs/handoff/priority.json; nothing to promote.");
        } else {
            const next = Object.keys(result.after).filter((id) => result.after[id] === "next");
            console.log(`Promoted after delivery of ${ids.join(", ")}. Next build now holds ${next.length} item(s): ${next.join(", ") || "none"}.`);
        }
    } catch (error) {
        console.error(`promote-build: ${error.message}`);
        console.error("Usage: node .github/extensions/openspec-task-planner/promote-build.mjs <shipped ids...> [--root <dir>]");
        process.exitCode = 1;
    }
}
