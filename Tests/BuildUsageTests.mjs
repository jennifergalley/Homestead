import assert from "node:assert/strict";
import test from "node:test";
import { mkdtemp, mkdir, readFile, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { buildUsageReport, persistUsageReport, loadUsageReports } from "../.github/extensions/openspec-task-planner/accounting-data.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";
import { renderPlannerHtml } from "../.github/extensions/openspec-task-planner/planner-html.mjs";
import { buildCostView } from "../.github/extensions/openspec-task-planner/cost-view.mjs";

const since = "2026-10-03T04:32:10.763Z";
const allocation = {
    buildId: "measured-test", authorizedAt: since,
    segments: [{ sessionId: "s", task: "task", category: "implementation", since, afterEventId: 10 }],
};
const details = [
    { tokenType: "input", tokenCount: 3, costPerBatch: 200000000000, batchSize: 1000000 },
    { tokenType: "cache_read", tokenCount: 80241, costPerBatch: 10000000000, batchSize: 1000000 },
    { tokenType: "cache_write", tokenCount: 10001, costPerBatch: 250000000000, batchSize: 1000000 },
    { tokenType: "output", tokenCount: 441, costPerBatch: 1000000000000, batchSize: 1000000 },
];
const event = (id, extra = {}) => ({ id, session_id: "s", agent_id: null, model: "gpt-6.1-sol",
    reasoning_effort: "medium", created_at: "2026-10-03T04:38:30.310Z",
    input_tokens: 90245, output_tokens: 441, reasoning_tokens: 32,
    total_nano_aiu: 3744260000, token_details_json: JSON.stringify(details), ...extra });
const snapshot = (events) => ({ schemaVersion: 1, sourceId: "local-assistant-usage-v1",
    capturedAt: "2026-10-03T05:00:00Z", throughEventId: 100, sessionIds: ["s"], since, events });

test("supplied rates reproduce recorded cost without cache or reasoning overlap", () => {
    const report = buildUsageReport(allocation, [snapshot([event(11)])]);
    assert.equal(report.totals.recordedNanoAiu, "3744260000");
    assert.equal(report.totals.recordedMinusRatedNanoAiu, "0");
    assert.deepEqual(report.totals.tokenCostsNanoAiu, {
        input: "600000", cache_read: "802410000", cache_write: "2500250000", output: "441000000",
    });
});

test("overlapping snapshots deduplicate calls, not inclusive parent totals; mixed models stay separate", () => {
    const report = buildUsageReport({ ...allocation, segments: [{ ...allocation.segments[0], contextTier: "default" }] }, [snapshot([event(10), event(11)]),
        snapshot([event(11), event(12, { model: "other", agent_id: "helper", reasoning_effort: "high" })])]);
    assert.equal(report.totals.calls, 2);
    assert.equal(report.totals.recordedNanoAiu, "7488520000");
    assert.equal(report.segments.length, 2);
    assert.equal(report.segments[1].agentId, "helper");
    assert.equal(report.segments[1].contextTier, null);
    assert.equal(report.coverage.excludedCalls, 1);
    assert.throws(() => buildUsageReport(allocation, [snapshot([event(11)]),
        snapshot([event(11, { total_nano_aiu: 1 })])]), /Conflicting/);
    assert.throws(() => buildUsageReport(allocation, [{ schemaVersion: 1, sourceId: "shutdown" }]), /inclusive/);
});

test("unknown coverage and legacy costs remain explicit; estimates never become recorded totals", () => {
    const report = buildUsageReport({ ...allocation, segments: [...allocation.segments,
        { sessionId: "missing", task: "integration", category: "overhead", since }] },
    [snapshot([event(11, { total_nano_aiu: null }), event(12, { token_details_json: null })])]);
    assert.equal(report.status, "incomplete");
    assert.equal(report.legacy.recordedNanoAiu, null);
    assert.equal(report.totals.estimatedNanoAiu, "3744260000");
    assert.equal(report.totals.recordedNanoAiu, "3744260000");
    assert.deepEqual(report.coverage.missingSessions, ["missing"]);
    assert.equal(report.totals.ratedCalls, 1);
});

test("boundaries classify rework and preserve configuration changes without overlapping events", () => {
    const segments = [
        { ...allocation.segments[0], throughEventId: 11, contextTier: "default" },
        { ...allocation.segments[0], afterEventId: 11, task: "retry", category: "rework", retryClassification: "retry", contextTier: "long_context" },
    ];
    const report = buildUsageReport({ ...allocation, segments }, [snapshot([event(11), event(12)])]);
    assert.deepEqual(report.segments.map((segment) => segment.contextTier), ["default", "long_context"]);
    assert.equal(report.segments[1].category, "rework");
    assert.throws(() => buildUsageReport({ ...allocation, segments: [segments[0], allocation.segments[0]] },
        [snapshot([event(11)])]), /Overlapping/);
});

test("report persists across reads and malformed reports fail visibly", async (t) => {
    const root = await mkdtemp(join(tmpdir(), "homestead-costs-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    const path = join(root, "docs", "handoff", "accounting", "reports", "measured-test.json");
    const report = buildUsageReport(allocation, [snapshot([event(11)])]);
    await persistUsageReport(path, report);
    await persistUsageReport(path, report);
    assert.deepEqual(JSON.parse(await readFile(path, "utf8")), report);
    const loaded = await loadUsageReports(root);
    assert.equal(loaded[0].records, undefined);
    assert.equal(loaded[0].totals.calls, 1);
    await writeFile(path, "{bad");
    await assert.rejects(loadUsageReports(root), SyntaxError);
});

test("HTTP report reads, backlog/screenshots, schedule/reorder/remove and quote remain agent-free", async (t) => {
    const root = await mkdtemp(join(tmpdir(), "homestead-cost-http-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    await mkdir(join(root, "docs", "handoff"), { recursive: true });
    await writeFile(join(root, "docs", "handoff", "backlog.md"), "# Backlog\n\n## Later\n");
    const report = buildUsageReport(allocation, [snapshot([event(11)])]);
    await persistUsageReport(join(root, "docs", "handoff", "accounting", "reports", "measured-test.json"), report);
    const attachments = [];
    const session = { send: () => { throw new Error("Agent call forbidden"); },
        rpc: { extensions: { sendAttachmentsToMessage: async (value) => attachments.push(value) } } };
    const { server, url } = await startPlannerServer(root, "test-panel", session);
    t.after(() => new Promise((resolve) => { server.closeAllConnections(); server.close(resolve); }));
    const post = async (path, body, expected = 200) => {
        const response = await fetch(url + path, { method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(body) });
        assert.equal(response.status, expected);
        return response.json();
    };
    assert.equal((await (await fetch(url + "health")).json()).status, "ready");
    const entry = (await post("api/backlog", { title: "Feedback", image: {
        dataUrl: "data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=",
    } }, 201)).entry;
    const planner = await (await fetch(url + "api/tasks")).json();
    assert.equal(planner.accounting[0].totals.recordedNanoAiu, "3744260000");
    assert.equal(planner.accounting[0].records, undefined);
    assert.equal((await fetch(new URL(entry.imageUrl, url))).status, 200);
    const id = planner.features[0].id;
    await post("api/priority", { order: [id], assign: { id, slot: planner.slots[0].key } });
    assert.equal((await (await fetch(url + "api/tasks")).json()).features[0].slot, planner.slots[0].key);
    await post("api/quote", { id });
    assert.equal(attachments.length, 1);
    await post("api/priority", { remove: id });
    assert.equal((await (await fetch(url + "api/tasks")).json()).features.length, 0);
    const html = await (await fetch(url)).text();
    assert.match(html, /Observed AIU, not billing-reconciled AI credits/);
    assert.doesNotMatch(html, /cost-table/);
    assert.match(html, /accounting: planner.accounting/);
});

test("rendered browser script parses without executing it", () => {
    const script = renderPlannerHtml().match(/<script>([\s\S]*?)<\/script>/)[1];
    assert.doesNotThrow(() => new Function(script));
});

test("browser accounting renderer is in page scope and renders actual costs and unknown coverage", () => {
    class Element {
        children = [];
        style = {};
        dataset = {};
        append(...children) { this.children.push(...children); }
        prepend(...children) { this.children.unshift(...children); }
        replaceChildren(...children) { this.children = children; }
        addEventListener() {}
        setAttribute() {}
    }
    const nodes = new Map();
    const document = {
        getElementById: (id) => {
            if (!nodes.has(id)) nodes.set(id, new Element());
            return nodes.get(id);
        },
        createElement: () => new Element(),
    };
    const script = renderPlannerHtml().match(/<script>([\s\S]*?)<\/script>/)[1];
    const beforeStartup = script.slice(0, script.indexOf('    refresh.addEventListener'));
    const renderAccounting = new Function("document", beforeStartup + "\nreturn renderAccounting;")(document);
    renderAccounting(null);
    const text = (node) => [node.textContent ?? "", ...node.children.map(text)].join(" ");
    assert.match(text(nodes.get("accounting")), /unknown, not zero/);
    const report = buildUsageReport(allocation, [snapshot([event(11)])]);
    renderAccounting(buildCostView({ reports: [report], builds: [{ buildId: report.buildId,
        status: "delivered", date: "2026-10-03", slot: "9 PM", shippedFeatures: ["A shipped feature"] }] }));
    assert.match(text(nodes.get("accounting")), /3.744 recorded AIU/);
    assert.match(text(nodes.get("accounting")), /A shipped feature/);
    assert.match(text(nodes.get("accounting")), /2026-10-03 — 9:00 PM/);
    assert.match(text(nodes.get("accounting")), /inherited costs.*incomplete/);
    assert.doesNotMatch(text(nodes.get("accounting")), /unknown runtime context|gpt-6.1-sol|cache read/);
});
