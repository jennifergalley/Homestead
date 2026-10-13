import assert from "node:assert/strict";
import { execFile } from "node:child_process";
import { createHash } from "node:crypto";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import { fileURLToPath } from "node:url";
import { promisify } from "node:util";
import { buildUsageReport, persistUsageReport } from "../.github/extensions/openspec-task-planner/accounting-data.mjs";
import {
    buildCardLines, closeDelivery, formatBuildHeading, insertBuildCard, nanoToAiuText, parsePromotedAt, verifyPlanner,
} from "../.github/extensions/openspec-task-planner/close-delivery.mjs";
import { loadPlanner, parseBuilds } from "../.github/extensions/openspec-task-planner/planner-data.mjs";

const run = promisify(execFile);
const closeScript = fileURLToPath(new URL("../Scripts/Close-Delivery.ps1", import.meta.url));
const BUILD = "20261012-test-01";
const OLD_BUILD = "20261011-next-01";

const buildsMarkdown = (newline = "\n") => [
    "# Build changelist", "", "Intro prose.", "",
    "## 2026-10-12 — 1:58 AM", "",
    `- Build ID: \`${OLD_BUILD}\``, "- Package source SHA: `6c7b1f8e6136`", "- Status: delivered", "- Configuration: Development",
    "- Ships:", "- Existing saves load in this build.", "",
    "## Next build", "", "- Currants easier to spot.", "",
    "## Build after next", "", "- Unassigned.", "",
    "## 2026-10-04 — 3:13 PM", "", "- Build ID: `20261004-evening-03`", "- Status: delivered", "- Ships:", "- Old thing.", "",
    "## Later", "", "- Deferred idea.", "",
].join(newline);

const inboxEntry = (id, title) => ({ id, title, description: `${title} description`, imageFile: null, imageMime: null, createdUtc: "2026-10-09T22:39:31.22Z" });
const backlogMarkdown = (entries) => [
    "# Backlog", "", "<!-- jenny-inbox:start -->",
    ...[...entries].reverse().map((entry) => `- **${entry.title}** — Jenny, 2026-10-09 (added from the planner canvas): ${entry.description} `.trimEnd()),
    "<!-- jenny-inbox:end -->", "", "## Later", "", "- Hand-written idea.", "",
].join("\n");

async function usageReport(root, buildId) {
    const allocation = { buildId, authorizedAt: "2026-10-11T20:14:00Z", segments: [{ sessionId: "s1", task: "mine", category: "implementation", since: "2026-10-11T20:14:00Z" }] };
    const events = [1, 2].map((id) => ({ id, session_id: "s1", created_at: "2026-10-11T21:00:00Z", model: "m", total_nano_aiu: 1_500_000_000 * id,
        input_tokens: 10, agent_id: null, token_details_json: null }));
    const snapshot = { schemaVersion: 1, sourceId: "local-assistant-usage-v1", capturedAt: "2026-10-12T00:00:00Z", throughEventId: 2, sessionIds: ["s1"], since: allocation.authorizedAt, events };
    await persistUsageReport(join(root, "docs", "handoff", "accounting", "reports", `${buildId}.json`), buildUsageReport(allocation, [snapshot]));
}

async function fixture(t, { newline = "\n", withReport = true } = {}) {
    const root = await mkdtemp(join(tmpdir(), "homestead-close-delivery-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    const handoff = join(root, "docs", "handoff");
    await mkdir(handoff, { recursive: true });
    await mkdir(join(root, "openspec", "changes", "add-swimming"), { recursive: true });
    await mkdir(join(root, "openspec", "changes", "add-fishing-boat"), { recursive: true });
    await writeFile(join(root, "openspec", "changes", "add-swimming", "tasks.md"), "## 1. Swim\n- [ ] 1.1 Swim\n");
    await writeFile(join(root, "openspec", "changes", "add-fishing-boat", "tasks.md"), "## 1. Boat\n- [ ] 1.1 Boat\n");
    const entries = [inboxEntry("jenny-a", "Alpha"), inboxEntry("jenny-b", "Bravo"), inboxEntry("jenny-c", "Charlie"), inboxEntry("jenny-d", "Delta")];
    await writeFile(join(handoff, "backlog-inbox.json"), JSON.stringify({ entries }, null, 2));
    await writeFile(join(handoff, "backlog.md"), backlogMarkdown(entries));
    await writeFile(join(handoff, "priority.json"), JSON.stringify({
        order: ["backlog:jenny-a", "add-swimming", "backlog:jenny-b", "backlog:jenny-c", "backlog:jenny-d", "add-fishing-boat"],
        slots: { "backlog:jenny-a": "next", "add-swimming": "next", "backlog:jenny-b": "next", "backlog:jenny-c": "after-next", "add-fishing-boat": "after-next" },
        removed: ["old-card"], updated: "2026-10-12T00:00:00.000Z",
    }, null, 2));
    await writeFile(join(handoff, "builds.md"), buildsMarkdown(newline));
    await writeFile(join(handoff, "backlog-archive.json"), JSON.stringify({ schemaVersion: 1, archives: [{ buildId: OLD_BUILD, inboxEntries: ["x"] }] }, null, 2));
    if (withReport) await usageReport(root, BUILD);
    return root;
}

const handoffFile = (root, name) => join(root, "docs", "handoff", name);
const readJson = async (root, name) => JSON.parse(await readFile(handoffFile(root, name), "utf8"));
const hashOf = async (root, name) => createHash("sha256").update(await readFile(handoffFile(root, name))).digest("hex");
const options = (overrides = {}) => ({
    buildId: BUILD, shippedCards: ["backlog:jenny-a", "add-swimming", "backlog:jenny-b"],
    packageSha: "0123456789abcdef", promotedAt: "2026-10-13 9:05 PM", ...overrides,
});

test("headings use h:mm AM/PM from the local promotion time", () => {
    assert.equal(formatBuildHeading(new Date(2026, 9, 12, 0, 5)), "2026-10-12 — 12:05 AM");
    assert.equal(formatBuildHeading(new Date(2026, 9, 12, 12, 0)), "2026-10-12 — 12:00 PM");
    assert.equal(formatBuildHeading(new Date(2026, 9, 12, 13, 58)), "2026-10-12 — 1:58 PM");
    for (const text of ["2026-10-12 1:58 PM", "2026-10-12 13:58", "2026-10-12T13:58", "2026-10-12T13:58:30"]) {
        assert.equal(formatBuildHeading(parsePromotedAt(text)), "2026-10-12 — 1:58 PM", text);
    }
    assert.equal(formatBuildHeading(parsePromotedAt("2026-10-12 12:00 AM")), "2026-10-12 — 12:00 AM");
    assert.equal(formatBuildHeading(parsePromotedAt("4:30 PM", new Date(2026, 0, 2))), "2026-01-02 — 4:30 PM");
    const instant = new Date("2026-10-12T20:58:00Z");
    assert.equal(parsePromotedAt("2026-10-12T20:58:00Z").getTime(), instant.getTime());
    for (const bad of ["tomorrow", "2026-02-30 1:00 AM", "2026-10-12 13:00 PM", "25:00"]) assert.throws(() => parsePromotedAt(bad), /PromotedAt/, bad);
    assert.equal(parsePromotedAt("", new Date(2026, 5, 1)).getFullYear(), 2026);
});

test("nano-AIU becomes exact AIU text", () => {
    assert.equal(nanoToAiuText("41025373140000"), "41025.37314");
    assert.equal(nanoToAiuText(2_000_000_000), "2");
    assert.equal(nanoToAiuText("1"), "0.000000001");
});

test("the skeleton card parses as a delivered build and keeps the file's line endings", () => {
    for (const newline of ["\n", "\r\n"]) {
        const markdown = insertBuildCard(buildsMarkdown(newline), { heading: "2026-10-13 — 9:05 PM", buildId: BUILD, sha: "abc1234" });
        assert.equal(markdown.includes("\r\n"), newline === "\r\n");
        assert.equal(markdown.split(newline === "\r\n" ? /(?<!\r)\n/ : /\r\n/).length, 1, "mixed line endings");
        const { entries } = parseBuilds(markdown);
        assert.equal(entries[0].buildId, BUILD, "newest card first");
        assert.equal(entries[0].status, "delivered");
        assert.equal(entries[0].heading, "2026-10-13 — 9:05 PM");
        assert.equal(entries[0].shippedFeatures.length, 3);
        assert.equal(entries.filter((entry) => entry.buildId === BUILD).length, 1);
    }
    const lines = buildCardLines({ heading: "2026-10-13 — 9:05 PM", buildId: BUILD, sha: "" });
    assert.ok(lines.includes("- Package source SHA: pending"));
    assert.ok(lines.includes("- Configuration: Development"));
});

test("a card older than the newest shipped one goes above the first card older than it", () => {
    const markdown = insertBuildCard(buildsMarkdown(), { heading: "2026-10-11 — 9:00 AM", buildId: BUILD, sha: "" });
    const headings = markdown.split("\n").filter((line) => line.startsWith("## "));
    assert.deepEqual(headings, ["## 2026-10-12 — 1:58 AM", "## Next build", "## Build after next", "## 2026-10-11 — 9:00 AM", "## 2026-10-04 — 3:13 PM", "## Later"]);
    assert.equal(insertBuildCard("# Only a title\n", { heading: "2026-10-13 — 9:05 PM", buildId: BUILD, sha: "" }).endsWith("\n"), true);
});

test("closing a delivery retires the cards, promotes after-next, archives the receipt and adds the card", async (t) => {
    const root = await fixture(t);
    const result = await closeDelivery(root, options());
    assert.equal(result.cardAction, "inserted");
    assert.equal(result.planner.ok, true, result.planner.problems?.join("; "));

    const priority = await readJson(root, "priority.json");
    assert.deepEqual(priority.order, ["backlog:jenny-c", "backlog:jenny-d", "add-fishing-boat"]);
    assert.deepEqual(priority.removed, ["old-card", "backlog:jenny-a", "add-swimming", "backlog:jenny-b"]);
    assert.deepEqual(priority.slots, { "backlog:jenny-c": "next", "add-fishing-boat": "next" });

    const inbox = await readJson(root, "backlog-inbox.json");
    assert.deepEqual(inbox.entries.map((entry) => entry.id), ["jenny-c", "jenny-d"]);
    const markdown = await readFile(handoffFile(root, "backlog.md"), "utf8");
    assert.ok(!markdown.includes("Alpha") && !markdown.includes("Bravo"), "shipped entries leave the mirror");
    assert.ok(markdown.includes("Charlie") && markdown.includes("Hand-written idea."));

    const archive = await readJson(root, "backlog-archive.json");
    assert.equal(archive.archives.length, 2);
    const entry = archive.archives.find((value) => value.buildId === BUILD);
    assert.deepEqual(entry.inboxEntries, ["jenny-a", "jenny-b"]);
    assert.deepEqual(entry.inboxEntryRecords.map((record) => record.title), ["Alpha", "Bravo"]);
    assert.deepEqual(entry.plannerOnlyItems, ["add-swimming"]);
    assert.equal(entry.status, "shipped-pending-player-acceptance");
    assert.deepEqual(entry.receipt, {
        buildRecord: "builds.md", changelist: "builds.md", observedUsageReport: `accounting/reports/${BUILD}.json`,
        recordedCalls: 2, recordedAiu: "4.5", sourceCommit: "0123456789abcdef",
    });

    const planner = await loadPlanner(root);
    const cost = planner.costView.shipped.find((value) => value.buildId === BUILD);
    assert.equal(cost.label, "2026-10-13 — 9:05 PM");
    assert.equal(cost.reportPath, `docs/handoff/accounting/reports/${BUILD}.json`);
    assert.equal(cost.recordedNanoAiu, "4500000000");
    assert.equal(planner.builds.entries.filter((build) => build.buildId === BUILD).length, 1);
    assert.ok(planner.features.every((feature) => !result.shipped.includes(feature.id)));
    const verified = await verifyPlanner(root, BUILD, result.shipped);
    assert.equal(verified.ok, true);
});

test("running it twice, even with another promotion time, adds no second card or archive entry", async (t) => {
    const root = await fixture(t);
    await closeDelivery(root, options());
    const builds = await hashOf(root, "builds.md");
    const again = await closeDelivery(root, options({ promotedAt: "2026-10-13 11:30 PM" }));
    assert.equal(again.cardAction, "exists");
    assert.equal(await hashOf(root, "builds.md"), builds);
    const archive = await readJson(root, "backlog-archive.json");
    assert.equal(archive.archives.filter((value) => value.buildId === BUILD).length, 1);
    assert.deepEqual(archive.archives.find((value) => value.buildId === BUILD).inboxEntries, ["jenny-a", "jenny-b"]);
    assert.equal((await readJson(root, "priority.json")).removed.filter((id) => id === "add-swimming").length, 1);
    const more = await closeDelivery(root, options({ shippedCards: ["backlog:jenny-c"] }));
    assert.ok(more.planner.ok);
    const merged = (await readJson(root, "backlog-archive.json")).archives.find((value) => value.buildId === BUILD);
    assert.deepEqual(merged.inboxEntries, ["jenny-a", "jenny-b", "jenny-c"]);
});

test("a dry run reports the plan and writes nothing", async (t) => {
    const root = await fixture(t);
    const names = ["priority.json", "backlog-inbox.json", "backlog.md", "backlog-archive.json", "builds.md"];
    const before = await Promise.all(names.map((name) => hashOf(root, name)));
    const result = await closeDelivery(root, options({ dryRun: true }));
    assert.equal(result.dryRun, true);
    assert.equal(result.cardAction, "inserted");
    assert.deepEqual(result.inboxArchived, ["backlog:jenny-a", "backlog:jenny-b"]);
    assert.equal(result.movedUpFromAfterNext, 2);
    assert.deepEqual(await Promise.all(names.map((name) => hashOf(root, name))), before);
});

test("an unknown card, a taken heading or a bad SHA stops before anything is written", async (t) => {
    const root = await fixture(t);
    const before = await hashOf(root, "priority.json");
    await assert.rejects(closeDelivery(root, options({ shippedCards: ["backlog:typo"] })), /Unknown card ID/);
    await assert.rejects(closeDelivery(root, options({ promotedAt: "2026-10-12 1:58 AM" })), /already has a card headed/);
    await assert.rejects(closeDelivery(root, options({ packageSha: "nothex" })), /Package SHA/);
    await assert.rejects(closeDelivery(root, options({ buildId: "bad id" })), /Build ID/);
    assert.equal(await hashOf(root, "priority.json"), before);
});

test("without a usage report the build still ships and the note says why", async (t) => {
    const root = await fixture(t, { withReport: false });
    const result = await closeDelivery(root, options());
    assert.ok(result.notes.some((note) => /No accounting\/reports/.test(note)));
    assert.equal(result.planner.ok, true);
    assert.equal(result.receipt.observedUsageReport, undefined);
});

test("CRLF builds.md stays CRLF", async (t) => {
    const root = await fixture(t, { newline: "\r\n" });
    await closeDelivery(root, options());
    const text = await readFile(handoffFile(root, "builds.md"), "utf8");
    assert.ok(text.includes("\r\n") && !/(?<!\r)\n/.test(text));
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

test("Close-Delivery.ps1 -WhatIf changes nothing and the full run exports usage, reports and closes", { skip: process.platform !== "win32" }, async (t) => {
    const root = await fixture(t, { withReport: false });
    const database = join(root, "usage.db");
    const python = `import sqlite3,sys
c=sqlite3.connect(sys.argv[1])
c.execute("create table assistant_usage_events(id integer primary key,session_id,turn_index,agent_id,parent_tool_call_id,model,reasoning_effort,input_tokens,output_tokens,cache_read_tokens,cache_write_tokens,reasoning_tokens,total_nano_aiu,token_details_json,created_at)")
for i in (1,2): c.execute("insert into assistant_usage_events values(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",(i,'s1',0,None,None,'m','high',10,5,0,0,0,1500000000*i,None,'2026-10-11T21:00:00Z'))
c.commit()`;
    try {
        await run("python", ["-c", python, database]);
    } catch (error) {
        if (error.code === "ENOENT") return t.skip("python is not installed");
        throw error;
    }
    const allocation = { buildId: BUILD, authorizedAt: "2026-10-11T20:14:00Z",
        segments: [{ sessionId: "s1", task: "mine", category: "implementation", since: "2026-10-11T20:14:00Z" }] };
    await mkdir(join(root, "docs", "handoff", "accounting"), { recursive: true });
    await writeFile(join(root, "docs", "handoff", "accounting", `${BUILD}-allocation.json`), JSON.stringify(allocation));
    const common = ["-File", closeScript, "-BuildId", BUILD, "-ShippedCards", "backlog:jenny-a,add-swimming", "-PackageSha", "0123456789abcdef",
        "-PromotedAt", "2026-10-13 9:05 PM", "-Root", root, "-UsageDatabase", database];

    const before = await hashOf(root, "priority.json");
    const dry = await powershell([...common, "-WhatIf"]);
    if (!dry) return t.skip("no PowerShell on PATH");
    assert.match(dry.stdout, /dry run, nothing written/);
    assert.equal(await hashOf(root, "priority.json"), before);
    await assert.rejects(readFile(handoffFile(root, join("accounting", "reports", `${BUILD}.json`))), { code: "ENOENT" });

    const real = await powershell(common);
    assert.match(real.stdout, /planner check: ok/);
    const report = await readJson(root, join("accounting", "reports", `${BUILD}.json`));
    assert.equal(report.buildId, BUILD);
    assert.equal(report.totals.recordedNanoAiu, "4500000000");
    assert.equal((await readJson(root, "priority.json")).removed.includes("backlog:jenny-a"), true);
    const planner = await loadPlanner(root);
    assert.ok(planner.costView.shipped.some((value) => value.buildId === BUILD && value.reportPath));
});
