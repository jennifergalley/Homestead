import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { loadPlanner } from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { renderPlannerHtml } from "../.github/extensions/openspec-task-planner/planner-html.mjs";

const now = Date.parse("2026-09-23T23:00:00Z");

async function fixture(t) {
    const root = await mkdtemp(join(tmpdir(), "homestead-planner-status-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    const changes = join(root, "openspec", "changes");
    for (const [name, marks] of [
        ["ready-to-start", [" "]],
        ["work-underway", ["x", " "]],
        ["finished-work", ["x"]],
    ]) {
        const directory = join(changes, name);
        await mkdir(directory, { recursive: true });
        await writeFile(join(directory, "tasks.md"),
            `## Work\n${marks.map((mark, index) => `- [${mark}] 1.${index + 1} Step ${index + 1}`).join("\n")}\n`);
    }
    return root;
}

async function setActivity(root, {
    change = "work-underway",
    phase = "implementing",
    updatedUtc = new Date(now - 60_000).toISOString(),
    runId = "current-run",
    state = "running",
} = {}) {
    const directory = join(root, "Automation");
    await mkdir(directory, { recursive: true });
    await Promise.all([
        writeFile(join(directory, "run.json"),
            JSON.stringify({ id: "current-run", state, openSpecChange: change })),
        writeFile(join(directory, "status.json"),
            JSON.stringify({ runId, phase, updatedUtc })),
    ]);
}

function statuses(planner) {
    return Object.fromEntries(planner.features.map(({ name, status }) => [name, status]));
}

test("planning-only changes are Proposed; interrupted implementation is Paused", async (t) => {
    const root = await fixture(t);
    const planner = await loadPlanner(root, now);
    assert.deepEqual(statuses(planner), {
        "work-underway": "paused",
        "ready-to-start": "proposed",
        "finished-work": "complete",
    });
    assert.deepEqual([
        planner.summary.activeFeatures,
        planner.summary.pausedFeatures,
        planner.summary.proposedFeatures,
        planner.summary.completedFeatures,
    ], [0, 1, 1, 1]);
});

test("only a fresh matching implementation report makes its change Active", async (t) => {
    const root = await fixture(t);
    await setActivity(root);
    const planner = await loadPlanner(root, now);
    assert.equal(statuses(planner)["work-underway"], "active");
    assert.equal(planner.summary.activeFeatures, 1);
    assert.equal(statuses(planner)["ready-to-start"], "proposed");

    await setActivity(root, { change: "ready-to-start" });
    assert.equal(statuses(await loadPlanner(root, now))["ready-to-start"], "active");
    assert.equal(statuses(await loadPlanner(root, now))["finished-work"], "complete");
});

test("stale, mismatched, or non-implementing reports cannot claim Active", async (t) => {
    const root = await fixture(t);
    for (const activity of [
        { updatedUtc: new Date(now - 16 * 60_000).toISOString() },
        { updatedUtc: new Date(now + 60_000).toISOString() },
        { runId: "older-run" },
        { state: "paused" },
        { phase: "promoted" },
        { change: "unknown-change" },
    ]) {
        await setActivity(root, activity);
        const planner = await loadPlanner(root, now);
        assert.equal(planner.summary.activeFeatures, 0);
        assert.equal(statuses(planner)["work-underway"], "paused");
    }
});

test("invalid activity data surfaces an error rather than inventing a status", async (t) => {
    const root = await fixture(t);
    await setActivity(root);
    await writeFile(join(root, "Automation", "status.json"), "{ invalid");
    await assert.rejects(loadPlanner(root, now), SyntaxError);
});

test("the canvas exposes all four status filters and completed-task disclosure", () => {
    const html = renderPlannerHtml();
    for (const status of ["active", "paused", "proposed", "complete"]) {
        assert.match(html, new RegExp(`data-filter="${status}"`));
    }
    assert.match(html, /feature\.status === "active" \|\| feature\.status === "paused"/);
    assert.match(html, /const completed = el\("details", "completed-group"\)/);
    assert.match(html, /completedExpanded\.get\(feature\.id\) \?\? false/);
    assert.match(html, /setInterval\(\(\) => load\(true\), 60_000\)/);
});
