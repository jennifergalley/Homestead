import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { fileURLToPath, pathToFileURL } from "node:url";
import test from "node:test";
import { promisify } from "node:util";
import { migrateSlots, normalizeMerged, promoteAfterDelivery, shortSha } from "../.github/extensions/openspec-task-planner/build-slots.mjs";
import { closeDelivery } from "../.github/extensions/openspec-task-planner/close-delivery.mjs";
import { markMerged } from "../.github/extensions/openspec-task-planner/mark-merged.mjs";
import { renderPlannerHtml } from "../.github/extensions/openspec-task-planner/planner-html.mjs";
import { loadPlanner } from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";

const run = promisify(execFile);
const markScript = fileURLToPath(new URL("../Scripts/Mark-Merged.ps1", import.meta.url));
const closeScript = fileURLToPath(new URL("../Scripts/Close-Delivery.ps1", import.meta.url));
const browserModule = process.env.PLANNER_BROWSER_MODULE;
const SHA = "049a7282f0123456789abcdef0123456789abcde";
const SHA2 = "1c41feb4d0123456789abcdef0123456789abcde";
const BUILD = "20261013-merged-01";

const buildsMarkdown = [
    "# Build changelist", "",
    "## 2026-10-12 — 1:58 AM", "", "- Build ID: `20261011-next-01`", "- Status: delivered", "- Ships:", "- Old.", "",
    "## Next build", "", "- Planned.", "", "## Build after next", "", "- Unassigned.", "",
].join("\n");

const entry = (id, title) => ({ id, title, description: `${title} description`, imageFile: null, imageMime: null, createdUtc: "2026-10-09T22:39:31.22Z" });

async function fixture(t) {
    const root = await mkdtemp(join(tmpdir(), "homestead-merged-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    const handoff = join(root, "docs", "handoff");
    await mkdir(handoff, { recursive: true });
    await mkdir(join(root, "openspec", "changes", "add-swimming"), { recursive: true });
    await writeFile(join(root, "openspec", "changes", "add-swimming", "tasks.md"), "## 1. Swim\n- [ ] 1.1 Swim\n");
    await writeFile(join(handoff, "backlog-inbox.json"), JSON.stringify({ entries: [entry("jenny-a", "Alpha search"), entry("jenny-b", "Bravo"), entry("jenny-c", "Charlie"), entry("jenny-d", "Delta")] }, null, 2));
    await writeFile(join(handoff, "backlog.md"), "# Backlog\n\n## Later\n\n- Idea.\n");
    await writeFile(join(handoff, "priority.json"), JSON.stringify({
        order: ["backlog:jenny-a", "backlog:jenny-b", "add-swimming", "backlog:jenny-c", "backlog:jenny-d"],
        slots: { "backlog:jenny-a": "next", "backlog:jenny-b": "next", "backlog:jenny-c": "after-next" },
        removed: ["old-card"], updated: "2026-10-12T00:00:00.000Z",
    }, null, 2));
    await writeFile(join(handoff, "builds.md"), buildsMarkdown);
    await writeFile(join(handoff, "backlog-archive.json"), JSON.stringify({ schemaVersion: 1, archives: [] }, null, 2));
    return root;
}

const file = (root, name) => join(root, "docs", "handoff", name);
const readJson = async (root, name) => JSON.parse(await readFile(file(root, name), "utf8"));
const hashOf = async (root, name) => createHash("sha256").update(await readFile(file(root, name))).digest("hex");

test("the merged map survives slot migration and leaves with the shipped cards", () => {
    assert.deepEqual(normalizeMerged({ a: { sha: "abc1234", mergedAt: "2026-10-12T00:00:00Z", note: "n" }, b: { sha: "" }, c: "x", d: { sha: "def5678" } }),
        { a: { sha: "abc1234", mergedAt: "2026-10-12T00:00:00Z", note: "n" }, d: { sha: "def5678", mergedAt: "", note: "" } });
    assert.equal(shortSha("049a7282f0123"), "049a7282f");
    assert.deepEqual(migrateSlots({ order: [], slots: {} }), { order: [], removed: [], slots: {} }, "no merged key unless there was one");
    const migrated = migrateSlots({ slots: { a: "next" }, merged: { b: { sha: "abc1234" }, c: { sha: "def5678" } } });
    assert.deepEqual(Object.keys(migrated.merged), ["b", "c"]);
    const promoted = promoteAfterDelivery({ slots: { a: "after-next" }, merged: { b: { sha: "abc1234" }, c: { sha: "def5678" } } }, ["b"]);
    assert.deepEqual(promoted.slots, { a: "next" });
    assert.deepEqual(Object.keys(promoted.merged), ["c"]);
});

test("marking cards merged records the SHA, takes them out of the slots and is idempotent", async (t) => {
    const root = await fixture(t);
    const result = await markMerged(root, { cards: ["backlog:jenny-a", "add-swimming"], sha: SHA, note: "search box", now: new Date("2026-10-13T01:00:00Z") });
    assert.deepEqual(result.marked, ["backlog:jenny-a", "add-swimming"]);
    assert.deepEqual(result.unscheduled, ["backlog:jenny-a"]);
    const priority = await readJson(root, "priority.json");
    assert.deepEqual(priority.merged["backlog:jenny-a"], { sha: SHA, mergedAt: "2026-10-13T01:00:00.000Z", note: "search box" });
    assert.equal(priority.slots["backlog:jenny-a"], undefined);
    assert.equal(priority.slots["backlog:jenny-b"], "next", "other cards keep their slots");
    assert.deepEqual(priority.order, ["backlog:jenny-a", "backlog:jenny-b", "add-swimming", "backlog:jenny-c", "backlog:jenny-d"]);

    const before = await hashOf(root, "priority.json");
    const again = await markMerged(root, { cards: ["backlog:jenny-a", "add-swimming"], sha: SHA, now: new Date("2026-10-14T00:00:00Z") });
    assert.equal(again.changed, false);
    assert.deepEqual(again.unchanged, ["backlog:jenny-a", "add-swimming"]);
    assert.equal(await hashOf(root, "priority.json"), before, "a repeat run writes nothing, not even `updated`");

    const renoted = await markMerged(root, { cards: ["backlog:jenny-a"], sha: SHA, note: "new note", now: new Date("2026-10-14T00:00:00Z") });
    assert.deepEqual(renoted.marked, ["backlog:jenny-a"]);
    const kept = (await readJson(root, "priority.json")).merged["backlog:jenny-a"];
    assert.equal(kept.note, "new note");
    assert.equal(kept.mergedAt, "2026-10-13T01:00:00.000Z", "same SHA keeps the original merge time");
    const remerged = await markMerged(root, { cards: ["backlog:jenny-a"], sha: SHA2, mergedAt: "2026-10-14T05:00:00Z" });
    assert.equal(remerged.marked.length, 1);
    const moved = (await readJson(root, "priority.json")).merged["backlog:jenny-a"];
    assert.deepEqual([moved.sha, moved.mergedAt, moved.note], [SHA2, "2026-10-14T05:00:00.000Z", "new note"]);
});

test("a dry run, unknown cards, removed cards and bad input change nothing", async (t) => {
    const root = await fixture(t);
    const before = await hashOf(root, "priority.json");
    const dry = await markMerged(root, { cards: ["backlog:jenny-a"], sha: SHA, dryRun: true });
    assert.equal(dry.dryRun, true);
    assert.deepEqual(dry.marked, ["backlog:jenny-a"]);
    await assert.rejects(markMerged(root, { cards: ["backlog:typo"], sha: SHA }), /Unknown card ID/);
    await assert.rejects(markMerged(root, { cards: ["old-card"], sha: SHA }), /Unknown card ID/);
    await assert.rejects(markMerged(root, { cards: ["backlog:jenny-a"], sha: "nothex" }), /merge SHA/);
    await assert.rejects(markMerged(root, { cards: ["backlog:jenny-a"], sha: SHA, mergedAt: "never" }), /MergedAt/);
    await assert.rejects(markMerged(root, { cards: [], sha: SHA }), /at least one card/);
    assert.equal(await hashOf(root, "priority.json"), before);
});

test("reopening clears the merged state", async (t) => {
    const root = await fixture(t);
    await markMerged(root, { cards: ["backlog:jenny-a", "backlog:jenny-b"], sha: SHA });
    const reopened = await markMerged(root, { cards: ["backlog:jenny-a", "backlog:jenny-c"], clear: true });
    assert.deepEqual(reopened.cleared, ["backlog:jenny-a"]);
    assert.deepEqual(reopened.unchanged, ["backlog:jenny-c"]);
    assert.deepEqual(Object.keys((await readJson(root, "priority.json")).merged), ["backlog:jenny-b"]);
});

test("the planner shows merged cards out of the slots with their SHA, and search still sees them", async (t) => {
    const root = await fixture(t);
    await markMerged(root, { cards: ["backlog:jenny-a"], sha: SHA, note: "planner search", mergedAt: "2026-10-13T01:00:00Z" });
    const planner = await loadPlanner(root);
    const merged = planner.features.find((feature) => feature.id === "backlog:jenny-a");
    assert.ok(merged, "still a feature, so the Backlog search can find it");
    assert.equal(merged.slot, null);
    assert.equal(merged.nextBuild, false);
    assert.equal(merged.merged.shortSha, "049a7282f");
    assert.equal(merged.merged.note, "planner search");
    assert.equal(planner.features.find((feature) => feature.id === "backlog:jenny-b").slot, "next");
    assert.equal(planner.features.find((feature) => feature.id === "backlog:jenny-b").merged, null);
    // A stale slot entry (written by an older planner) cannot put a merged card back into a build.
    const priority = await readJson(root, "priority.json");
    priority.slots["backlog:jenny-a"] = "next";
    await writeFile(file(root, "priority.json"), JSON.stringify(priority));
    assert.equal((await loadPlanner(root)).features.find((feature) => feature.id === "backlog:jenny-a").slot, null);
});

async function serve(t, root) {
    const session = new Proxy({}, { get() { throw new Error("No chat permitted"); } });
    const { server, url } = await startPlannerServer(root, "merged-proof", session);
    t.after(() => new Promise((resolve) => server.close(resolve)));
    return url;
}
const post = (url, payload) => fetch(`${url}api/priority`, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(payload) });

test("the planner server refuses to schedule a merged card, can reopen it, and removal forgets it", async (t) => {
    const root = await fixture(t);
    await markMerged(root, { cards: ["backlog:jenny-a", "backlog:jenny-d"], sha: SHA });
    const url = await serve(t, root);
    const refused = await post(url, { assign: { id: "backlog:jenny-a", slot: "next" } });
    assert.equal(refused.status, 400);
    assert.match((await refused.json()).error, /on main/);
    assert.equal((await post(url, { assign: { id: "backlog:jenny-a", slot: null } })).status, 200, "clearing a slot is harmless");
    assert.equal((await post(url, { order: ["backlog:jenny-b"] })).status, 200);
    assert.deepEqual(Object.keys((await readJson(root, "priority.json")).merged).sort(), ["backlog:jenny-a", "backlog:jenny-d"], "reordering keeps the merged map");
    assert.equal((await post(url, { unmerge: "backlog:jenny-a" })).status, 200);
    assert.deepEqual(Object.keys((await readJson(root, "priority.json")).merged), ["backlog:jenny-d"]);
    assert.equal((await post(url, { assign: { id: "backlog:jenny-a", slot: "after-next" } })).status, 200, "reopened cards schedule again");
    assert.equal((await post(url, { remove: "backlog:jenny-d" })).status, 200);
    const priority = await readJson(root, "priority.json");
    assert.deepEqual(priority.merged, {});
    assert.ok(priority.removed.includes("backlog:jenny-d"));
    const tasks = await (await fetch(`${url}api/tasks`)).json();
    assert.equal(tasks.features.find((feature) => feature.id === "backlog:jenny-a").merged, null);
});

test("the page has the Done group, badge and no slot or drag controls for merged rows", () => {
    const html = renderPlannerHtml();
    assert.match(html, /Done, ships in the next build/);
    assert.match(html, /"merged-row"/);
    assert.match(html, /merged-badge/);
    assert.match(html, /unmerge: feature\.id/);
    assert.match(html, /row\.draggable = !isMerged/);
});

test("closing a delivery without -ShippedCards ships every merged card and clears the state", async (t) => {
    const root = await fixture(t);
    await markMerged(root, { cards: ["backlog:jenny-a", "add-swimming"], sha: SHA });
    const result = await closeDelivery(root, { buildId: BUILD, promotedAt: "2026-10-13 9:05 PM", packageSha: "0123456789abcdef" });
    assert.deepEqual(result.shipped.sort(), ["add-swimming", "backlog:jenny-a"]);
    assert.deepEqual(result.mergedCleared.sort(), ["add-swimming", "backlog:jenny-a"]);
    assert.equal(result.planner.ok, true, result.planner.problems?.join("; "));
    const priority = await readJson(root, "priority.json");
    assert.deepEqual(priority.merged, {});
    assert.ok(priority.removed.includes("backlog:jenny-a") && priority.removed.includes("add-swimming"));
    assert.deepEqual(priority.slots, { "backlog:jenny-b": "next", "backlog:jenny-c": "next" }, "after-next promoted once");
    assert.deepEqual((await readJson(root, "backlog-inbox.json")).entries.map((value) => value.id), ["jenny-b", "jenny-c", "jenny-d"]);
    const archived = (await readJson(root, "backlog-archive.json")).archives[0];
    assert.deepEqual(archived.inboxEntries, ["jenny-a"]);
    assert.deepEqual(archived.plannerOnlyItems, ["add-swimming"]);

    // Jenny schedules something into Build after next, then the closure is re-run: nothing is promoted again.
    priority.slots["backlog:jenny-d"] = "after-next";
    await writeFile(file(root, "priority.json"), JSON.stringify(priority, null, 2));
    const again = await closeDelivery(root, { buildId: BUILD, promotedAt: "2026-10-13 9:05 PM" });
    assert.equal(again.cardAction, "exists");
    assert.equal(again.movedUpFromAfterNext, 0);
    assert.equal((await readJson(root, "priority.json")).slots["backlog:jenny-d"], "after-next");
});

test("an explicit list leaves other merged cards waiting; no list and nothing merged stops", async (t) => {
    const root = await fixture(t);
    await assert.rejects(closeDelivery(root, { buildId: BUILD, promotedAt: "2026-10-13 9:05 PM" }), /none are marked merged/);
    await markMerged(root, { cards: ["backlog:jenny-a", "backlog:jenny-b"], sha: SHA });
    const result = await closeDelivery(root, { buildId: BUILD, shippedCards: ["backlog:jenny-a"], promotedAt: "2026-10-13 9:05 PM" });
    assert.deepEqual(result.mergedCleared, ["backlog:jenny-a"]);
    assert.ok(result.notes.some((note) => /stay "On main, awaiting build".*jenny-b/.test(note)));
    assert.deepEqual(Object.keys((await readJson(root, "priority.json")).merged), ["backlog:jenny-b"]);
    const planner = await loadPlanner(root);
    assert.equal(planner.features.find((feature) => feature.id === "backlog:jenny-b").merged.shortSha, "049a7282f");
});

async function powershell(args) {
    for (const exe of ["pwsh", "powershell"]) {
        try {
            return await run(exe, ["-NoProfile", "-ExecutionPolicy", "Bypass", ...args]);
        } catch (error) {
            if (error.code !== "ENOENT") throw error;
        }
    }
    return null;
}

test("Mark-Merged.ps1: -WhatIf writes nothing, a real run marks, a repeat is a no-op, -Reopen clears, and Close-Delivery.ps1 ships the lot", { skip: process.platform !== "win32" }, async (t) => {
    const root = await fixture(t);
    const base = ["-File", markScript, "-Cards", "backlog:jenny-a,add-swimming", "-Sha", "049a7282f", "-Root", root];
    const before = await hashOf(root, "priority.json");
    const dry = await powershell([...base, "-WhatIf"]);
    if (!dry) return t.skip("no PowerShell on PATH");
    assert.match(dry.stdout, /dry run, nothing written/);
    assert.equal(await hashOf(root, "priority.json"), before);

    const real = await powershell([...base, "-Note", "dev-only"]);
    assert.match(real.stdout, /2 marked/);
    const marked = await hashOf(root, "priority.json");
    assert.match((await powershell(base)).stdout, /2 already marked/);
    assert.equal(await hashOf(root, "priority.json"), marked);
    assert.equal((await readJson(root, "priority.json")).merged["backlog:jenny-a"].note, "dev-only");

    const reopen = await powershell(["-File", markScript, "-Cards", "add-swimming", "-Reopen", "-Root", root]);
    assert.match(reopen.stdout, /Reopened 1/);
    await assert.rejects(powershell(["-File", markScript, "-Cards", "backlog:jenny-a", "-Sha", "zzz", "-Root", root]), /neither a commit|hex digits/);

    const closed = await powershell(["-File", closeScript, "-BuildId", BUILD, "-PromotedAt", "2026-10-13 9:05 PM", "-Root", root, "-SkipUsage"]);
    assert.match(closed.stdout, /No -ShippedCards given: shipping all 1 card/);
    assert.match(closed.stdout, /planner check: ok/);
    assert.deepEqual((await readJson(root, "priority.json")).merged, {});
});

test("in a real browser: merged cards sit in their own group, search finds them, and there are no drag or slot controls", {
    skip: !browserModule && "Set PLANNER_BROWSER_MODULE to a playwright-core module on E: to run browser proof.",
}, async (t) => {
    assert.match(tmpdir(), /^E:[\\/]/i, "Browser fixtures and profile must stay on E:");
    const { chromium } = await import(pathToFileURL(browserModule).href);
    const root = await fixture(t);
    await markMerged(root, { cards: ["backlog:jenny-a"], sha: SHA, note: "planner search", mergedAt: "2026-10-13T01:00:00Z" });
    const url = await serve(t, root);
    const browser = await chromium.launch({
        headless: true,
        executablePath: process.env.PLANNER_BROWSER_EXECUTABLE ?? "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
    });
    t.after(() => browser.close());
    const page = await browser.newPage({ viewport: { width: 1440, height: 1100 } });
    page.setDefaultTimeout(10_000);
    const errors = [];
    page.on("pageerror", (error) => errors.push(error.message));
    await page.goto(url);
    await page.locator(".merged-row").first().waitFor();

    assert.equal(await page.locator(".check-row").count(), 4, "the other four stay on the board");
    assert.equal(await page.locator(".merged-row").count(), 1);
    const row = page.locator(".merged-row");
    assert.match(await row.innerText(), /Alpha search/);
    assert.match(await row.locator(".merged-badge").innerText(), /On main · 049a7282f · /);
    assert.equal(await row.locator("select.slot-select").count(), 0);
    assert.equal(await row.locator(".drag, .rank").count(), 0);
    assert.equal(await row.getAttribute("draggable"), "false");
    assert.equal(await row.locator("button.feedback-edit").count(), 1, "editing stays available");
    assert.match(await page.locator(".merged-group > summary").innerText(), /Done, ships in the next build/);
    assert.match(await page.locator("#backlog-note").innerText(), /1 done, awaiting build/);
    assert.equal(await page.locator(".check-row select.slot-select").count(), 4);

    // The planner's slot lists no longer include it.
    const search = page.locator("#backlog-search");
    await search.fill("alpha");
    assert.equal(await page.locator(".merged-row:not([hidden])").count(), 1, "search still finds the merged card");
    assert.equal(await page.locator(".check-row:not([hidden])").count(), 0);
    await search.fill("bravo");
    assert.equal(await page.locator(".merged-row:not([hidden])").count(), 0);
    await search.fill("");

    await row.locator("button", { hasText: "Reopen" }).click();
    await page.locator(".check-row").nth(4).waitFor();
    assert.equal(await page.locator(".merged-row").count(), 0);
    assert.equal(await page.locator(".merged-group").count(), 0);
    assert.deepEqual(errors, []);
});
