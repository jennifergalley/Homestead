import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { mkdtemp, mkdir, readdir, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";
import {
    BUILD_SLOTS, migrateSlots, promoteAfterDelivery, slotKeyForHeading,
} from "../.github/extensions/openspec-task-planner/build-slots.mjs";
import { loadPlanner } from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { promoteBuild } from "../.github/extensions/openspec-task-planner/promote-build.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";

const run = promisify(execFile);
const promoteScript = fileURLToPath(new URL("../.github/extensions/openspec-task-planner/promote-build.mjs", import.meta.url));

async function fixture(t, { priority, builds } = {}) {
    const root = await mkdtemp(join(tmpdir(), "homestead-planner-slots-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    await mkdir(join(root, "docs", "handoff"), { recursive: true });
    if (priority !== undefined) await writeFile(join(root, "docs", "handoff", "priority.json"), JSON.stringify(priority, null, 2));
    if (builds !== undefined) await writeFile(join(root, "docs", "handoff", "builds.md"), builds);
    return root;
}

const priorityFile = (root) => join(root, "docs", "handoff", "priority.json");
const readPriority = async (root) => JSON.parse(await readFile(priorityFile(root), "utf8"));

test("exactly two named slots with no clock times", () => {
    assert.deepEqual(BUILD_SLOTS.map((slot) => [slot.key, slot.label]), [["next", "Next build"], ["after-next", "Build after next"]]);
});

test("migration maps date-times by order, keeps named keys and legacy nextBuild, drops nothing", () => {
    const migrated = migrateSlots({
        order: ["a", "b", "c", "d", "e", "f", "g"],
        nextBuild: ["f", "a"],
        slots: {
            a: "2026-10-04 16:00", b: "2026-10-04 07:30", c: "2026-10-04 21:00", d: "2026-10-04 16:00",
            e: "after-next", g: "2026-10-03 21:00",
        },
    }, new Set(["2026-10-03 21:00"]));
    assert.deepEqual(migrated.slots, {
        a: "after-next", b: "next", c: "after-next", d: "after-next", e: "after-next", f: "next", g: "next",
    });
    assert.equal("nextBuild" in migrated, false);
    assert.deepEqual(migrated.order, ["a", "b", "c", "d", "e", "f", "g"]);
});

test("migration is idempotent and tolerates missing or malformed documents", () => {
    const once = migrateSlots({ slots: { a: "2026-10-04 07:30", b: "2026-10-04 16:00" } });
    assert.deepEqual(migrateSlots(once), once);
    assert.deepEqual(migrateSlots(null), { order: [], removed: [], slots: {} });
    assert.deepEqual(migrateSlots({ slots: { a: 5, b: "" } }).slots, {});
});

test("planner migrates priority on read without rewriting the file", async (t) => {
    const priority = { order: ["x"], slots: { x: "2026-10-04 16:00", y: "2026-10-04 21:00" }, nextBuild: ["z"] };
    const root = await fixture(t, { priority });
    const before = await readFile(priorityFile(root), "utf8");
    const planner = await loadPlanner(root);
    assert.deepEqual(planner.slots.map((slot) => slot.key), ["next", "after-next"]);
    assert.equal(await readFile(priorityFile(root), "utf8"), before);
});

test("planned builds use named headings and sort Next build, Build after next, then shipped newest-first", async (t) => {
    const root = await fixture(t, {
        builds: [
            "# Builds", "",
            "## 2026-10-03 — 9 PM", "- Status: delivered", "- Ships:", "- Older",
            "## Build after next", "- Ships:", "- Second",
            "## 2026-10-04 — 11:29 AM", "- Status: delivered", "- Ships:", "- Newer",
            "## Next build", "- Ships:", "- First", "",
        ].join("\n"),
    });
    const { builds } = await loadPlanner(root, Date.parse("2026-10-04T20:00:00Z"));
    assert.deepEqual(builds.entries.map((build) => build.label ?? build.heading), [
        "Next build", "Build after next", "2026-10-04 — 11:29 AM", "2026-10-03 — 9:00 PM",
    ]);
    assert.deepEqual(builds.entries.slice(0, 2).map((build) => [build.key, build.status, build.dataError]), [
        ["next", "planned", undefined], ["after-next", "planned", undefined],
    ]);
    assert.equal(slotKeyForHeading("  next BUILD "), "next");
});

test("a planned card with any other heading or a duplicate is a visible data error", async (t) => {
    const root = await fixture(t, {
        builds: ["# Builds", "",
            "## 2026-10-05 — 7:30 AM", "- Ships:", "- Timed plan",
            "## Soon", "- Ships:", "- Free text",
            "## Next build", "- Ships:", "- One",
            "## Next build", "- Ships:", "- Two", "",
        ].join("\n"),
    });
    const { builds } = await loadPlanner(root);
    const planned = builds.entries.filter((build) => build.status === "planned");
    assert.equal(planned.length, 4);
    assert.equal(planned[0].key, "next");
    assert.equal(planned[0].dataError, undefined);
    for (const build of planned.slice(1)) {
        assert.equal(build.key, null);
        assert.match(build.dataError, /Next build|Correct the build changelist/);
    }
    assert.match(planned.find((build) => build.heading === "Soon").dataError, /Next build" or "Build after next/);
});

test("promoteAfterDelivery removes shipped, keeps next, moves after-next up", () => {
    const before = {
        order: ["a", "b", "c", "d"], removed: ["r"], updated: "t",
        slots: { a: "next", b: "next", c: "after-next", d: "after-next" },
    };
    const snapshot = JSON.stringify(before);
    const after = promoteAfterDelivery(before, ["a", "d", "unknown"]);
    assert.deepEqual(after.slots, { b: "next", c: "next" });
    assert.deepEqual(after.order, before.order);
    assert.deepEqual(after.removed, ["r"]);
    assert.equal(JSON.stringify(before), snapshot);
    assert.deepEqual(promoteAfterDelivery({ slots: { a: "2026-10-04 07:30", b: "2026-10-04 16:00" } }, []).slots, { a: "next", b: "next" });
    assert.throws(() => promoteAfterDelivery({}, "a"), TypeError);
});

test("promoteBuild rewrites priority.json atomically with the new keys and leaves no temp files", async (t) => {
    const root = await fixture(t, {
        priority: { order: ["a", "b", "c"], slots: { a: "2026-10-04 16:00", b: "2026-10-04 16:00", c: "2026-10-04 21:00" }, nextBuild: [] },
    });
    const result = await promoteBuild(root, ["a"]);
    assert.equal(result.written, true);
    const saved = await readPriority(root);
    assert.deepEqual(saved.slots, { b: "next", c: "next" });
    assert.equal("nextBuild" in saved, false);
    assert.deepEqual(saved.order, ["a", "b", "c"]);
    assert.deepEqual((await readdir(join(root, "docs", "handoff"))).filter((name) => name.endsWith(".tmp")), []);
});

test("promote-build CLI applies the promotion, rejects bad usage and corrupt files", async (t) => {
    const root = await fixture(t, { priority: { slots: { a: "next", b: "after-next", c: "after-next" } } });
    const ok = await run(process.execPath, [promoteScript, "--root", root, "a"]);
    assert.match(ok.stdout, /Promoted after delivery of a/);
    assert.deepEqual((await readPriority(root)).slots, { b: "next", c: "next" });

    const noIds = await run(process.execPath, [promoteScript, "--root", root]).catch((error) => error);
    assert.equal(noIds.code, 1);

    const corrupt = await fixture(t);
    await writeFile(priorityFile(corrupt), "{ not json");
    const failed = await run(process.execPath, [promoteScript, "--root", corrupt, "a"]).catch((error) => error);
    assert.equal(failed.code, 1);
    assert.equal(await readFile(priorityFile(corrupt), "utf8"), "{ not json");

    const missing = await fixture(t);
    await promoteBuild(missing, ["a"]);
    assert.deepEqual((await readdir(join(missing, "docs", "handoff"))), []);
});

test("assign API accepts only the two named slots and saves the new keys", async (t) => {
    const root = await fixture(t, { priority: { slots: { a: "2026-10-04 16:00" }, nextBuild: ["b"] } });
    const { server, url } = await startPlannerServer(root, "slots-test", null);
    t.after(() => new Promise((resolve) => server.close(resolve)));
    const post = (body) => fetch(new URL("api/priority", url), {
        method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body),
    });
    assert.equal((await post({ assign: { id: "c", slot: "2026-10-04 16:00" } })).status, 400);
    assert.equal((await post({ assign: { id: "c", slot: "after-next" } })).status, 200);
    const saved = await readPriority(root);
    assert.deepEqual(saved.slots, { a: "next", b: "next", c: "after-next" });
    assert.equal("nextBuild" in saved, false);
    assert.equal((await post({ assign: { id: "c", slot: null } })).status, 200);
    assert.equal("c" in (await readPriority(root)).slots, false);
});
