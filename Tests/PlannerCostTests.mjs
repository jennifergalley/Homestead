import assert from "node:assert/strict";
import { mkdtemp, mkdir, writeFile, readFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import test from "node:test";
import { buildCostView, groupBuildCosts } from "../.github/extensions/openspec-task-planner/cost-view.mjs";
import { loadAccountUsage, summarizeUsageRows } from "../.github/extensions/openspec-task-planner/account-usage.mjs";
import { loadPlanner } from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { persistUsageReport } from "../.github/extensions/openspec-task-planner/accounting-data.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";

const now = Date.parse("2026-10-04T19:00:00Z");
const cycle = { start: "2026-09-30", end: "2026-10-30", reset: "2026-10-31", timeZone: "America/Los_Angeles" };
const segment = (task, category, recordedNanoAiu, extra = {}) => ({
    task, category, recordedNanoAiu, estimatedNanoAiu: "0", recordedCalls: 1, calls: 1, ...extra,
});
function report(buildId, segments) {
    return { schemaVersion: 1, buildId, status: "incomplete", legacy: { recordedNanoAiu: null },
        segments, totals: { calls: segments.reduce((sum, value) => sum + value.calls, 0),
            recordedCalls: segments.reduce((sum, value) => sum + value.recordedCalls, 0),
            recordedNanoAiu: String(segments.reduce((sum, value) => sum + BigInt(value.recordedNanoAiu), 0n)),
            estimatedNanoAiu: String(segments.reduce((sum, value) => sum + BigInt(value.estimatedNanoAiu), 0n)) } };
}
const measured = report("20261003-measured-02", [
    segment("farming-fishing-and-provisional-food", "implementation", "1000000000000"),
    segment("farming-fishing-and-provisional-food", "implementation", "123450000000"),
    segment("first-overhead", "overhead", "100000000000"),
    segment("other-overhead", "overhead", "200000000000"),
    segment("reviewer", "review", "70000000000"),
]);
const shipment = { date: "2026-10-04", slot: "11:29 AM", status: "delivered",
    buildId: measured.buildId, shippedFeatures: ["Six fish and eleven provisional meals", "Plant Seeds hint"] };
const buildView = (extra = {}) => buildCostView({ reports: [measured], builds: [shipment], cycle, now, ...extra });

test("billing chart fills every current-cycle day and uses actual shipment date, not build ID", () => {
    const view = buildView();
    assert.equal(view.period.status, "current");
    assert.equal(view.period.days.length, 31);
    assert.equal(view.period.days[0].date, "2026-09-30");
    assert.equal(view.period.days.at(-1).date, "2026-10-30");
    const day = (date) => view.period.days.find((value) => value.date === date);
    assert.equal(day("2026-10-03").recordedNanoAiu, "0");
    assert.equal(day("2026-10-03").shipments, 0);
    assert.equal(day("2026-10-04").recordedNanoAiu, measured.totals.recordedNanoAiu);
    assert.equal(day("2026-10-05").future, true);
    assert.equal(view.shipped[0].label, "2026-10-04 — 11:29 AM");
    assert.deepEqual(view.shipped[0].features, shipment.shippedFeatures);
});

test("multiple shipments aggregate exactly; missing costs are unknown, never zero shipments", () => {
    const second = report("second", [segment("task", "implementation", "1111111111")]);
    const view = buildView({ reports: [measured, second], builds: [
        shipment, { ...shipment, buildId: "second", slot: "4 PM" },
        { date: "2026-10-01", slot: "7:30 AM", status: "delivered", shippedFeatures: ["Older work"] },
    ] });
    const day = view.period.days.find((value) => value.date === "2026-10-04");
    assert.equal(day.shipments, 2);
    assert.equal(day.recordedNanoAiu, String(BigInt(measured.totals.recordedNanoAiu) + 1111111111n));
    const unknown = view.period.days.find((value) => value.date === "2026-10-01");
    assert.equal(unknown.unknown, true);
    assert.equal(unknown.shipments, 1);
    assert.equal(view.shipped.at(-1).recordedNanoAiu, null);
    assert.equal(view.period.unknown, true);
});

test("credit share tracks used, remaining and an exact weekly allotment of the 500k", () => {
    const share = buildView().period.share;
    assert.equal(share.shareCredits, 500000);
    assert.equal(share.totalNanoAiu, "500000000000000");
    assert.equal(share.recordedNanoAiu, measured.totals.recordedNanoAiu);
    assert.equal(BigInt(share.recordedNanoAiu) + BigInt(share.remainingNanoAiu), 500000000000000n);
    assert.equal(share.overNanoAiu, "0");
    assert.equal(share.percentUsed, 0.29);
    assert.equal(share.weeks.length, 5);
    assert.deepEqual(share.weeks.map((week) => week.days), [7, 7, 7, 7, 3]);
    assert.equal(share.weeks[0].start, "2026-09-30");
    assert.equal(share.weeks.at(-1).end, "2026-10-30");
    assert.equal(share.weeks.reduce((sum, week) => sum + BigInt(week.allotmentNanoAiu), 0n), 500000000000000n);
    assert.equal(share.weeks[0].allotmentNanoAiu, String(500000000000000n * 7n / 31n));
    assert.equal(share.weeks.reduce((sum, week) => sum + BigInt(week.recordedNanoAiu), 0n), BigInt(share.recordedNanoAiu));
    assert.equal(share.currentWeek, 1);
    assert.deepEqual(share.weeks.map((week) => week.status), ["current", "upcoming", "upcoming", "upcoming", "upcoming"]);
    assert.equal(share.paceAllowedNanoAiu, String(500000000000000n * 5n / 31n));
});

test("credit share goes over cleanly, honours a configured share and skips non-current periods", () => {
    const big = report("big", [segment("task", "implementation", "600000000000000")]);
    const over = buildView({ reports: [big], builds: [{ ...shipment, buildId: "big" }] }).period.share;
    assert.equal(over.remainingNanoAiu, "0");
    assert.equal(over.overNanoAiu, "100000000000000");
    assert.equal(over.percentUsed, 120);
    const small = buildView({ cycle: { ...cycle, shareCredits: 1000 } }).period.share;
    assert.equal(small.totalNanoAiu, "1000000000000");
    assert.equal(small.weeks.reduce((sum, week) => sum + BigInt(week.allotmentNanoAiu), 0n), 1000000000000n);
    assert.throws(() => buildView({ cycle: { ...cycle, shareCredits: 0 } }), /Invalid credit share/);
    assert.equal(buildView({ cycle: { ...cycle, start: "2026-08-01", end: "2026-08-31", reset: "2026-09-01" } }).period.share, undefined);
    assert.equal(buildView({ cycle: null }).period.share, undefined);
});

test("credit share marks weeks with unknown costs", () => {
    const view = buildView({ builds: [shipment, { date: "2026-10-01", slot: "7:30 AM", status: "delivered", shippedFeatures: ["Older"] }] });
    assert.equal(view.period.share.unknown, true);
    assert.equal(view.period.share.weeks[0].unknown, true);
});

const octCycle = { start: "2026-10-01", end: "2026-10-31", reset: "2026-11-01", timeZone: "America/Los_Angeles" };

test("account usage buckets by billing time zone, mixes timestamp formats and groups by project", () => {
    const rows = [
        { session_id: "a", created_at: "2026-10-01T06:59:59.000Z", nano: 9_000_000_000n, repository: "o/one" },
        { session_id: "a", created_at: "2026-10-01T07:00:00.000Z", nano: 2_000_000_000n, repository: "o/one" },
        { session_id: "b", created_at: "2026-10-02 12:00:00", nano: 3_000_000_000n, repository: null },
        { session_id: "c", created_at: "2026-11-01T06:59:00Z", nano: 4_000_000_000n, repository: "o/two" },
        { session_id: "c", created_at: "2026-11-01T07:00:00Z", nano: 5_000_000_000n, repository: "o/two" },
    ];
    const summary = summarizeUsageRows(rows, octCycle);
    assert.deepEqual(summary.days, { "2026-10-01": "2000000000", "2026-10-02": "3000000000", "2026-10-31": "4000000000" });
    assert.equal(summary.totalNanoAiu, "9000000000");
    assert.equal(summary.calls, 3);
    assert.equal(summary.sessions, 3);
    assert.deepEqual(summary.projects.map((project) => project.repository), ["o/two", "No repository (chats)", "o/one"]);
});

test("credit share uses account-wide usage when available and falls back to shipped builds when not", () => {
    const usage = { status: "ok", calls: 3, sessions: 2, projects: [{ repository: "o/one", nanoAiu: "1", calls: 1, sessions: 1 }],
        days: { "2026-09-30": "50000000000000", "2026-10-04": "25000000000000", "2026-10-20": "1" } };
    const share = buildView({ usage }).period.share;
    assert.equal(share.basis, "all-projects");
    assert.equal(share.recordedNanoAiu, "75000000000001");
    assert.equal(share.percentUsed, 15);
    assert.equal(share.weeks[0].recordedNanoAiu, "75000000000000");
    assert.equal(share.weeks[2].recordedNanoAiu, "1");
    assert.equal(share.unknown, false);
    assert.equal(share.projects.length, 1);
    const fallback = buildView({ usage: { status: "unavailable", message: "no store" } }).period.share;
    assert.equal(fallback.basis, "shipped-builds");
    assert.equal(fallback.usageMessage, "no store");
    assert.equal(fallback.recordedNanoAiu, measured.totals.recordedNanoAiu);
    assert.equal(buildView().period.share.basis, "shipped-builds");
});

test("loadAccountUsage reads a session store read-only and fails softly when it is missing", async (t) => {
    const dir = await mkdtemp(join(tmpdir(), "planner-usage-"));
    t.after(() => rm(dir, { recursive: true, force: true }));
    const dbPath = join(dir, "store.db");
    const { DatabaseSync } = await import("node:sqlite");
    const db = new DatabaseSync(dbPath);
    db.exec("CREATE TABLE sessions (id TEXT, repository TEXT); CREATE TABLE assistant_usage_events (session_id TEXT, created_at TEXT, total_nano_aiu INTEGER);");
    db.exec("INSERT INTO sessions VALUES ('s1','o/one'); INSERT INTO assistant_usage_events VALUES ('s1','2026-10-03T20:00:00Z',1500000000), ('s1','2026-09-30T20:00:00Z',7), ('zz','2026-10-04T01:00:00Z',500000000);");
    db.close();
    const usage = await loadAccountUsage({ ...octCycle, dbPath });
    assert.equal(usage.status, "ok");
    assert.equal(usage.totalNanoAiu, "2000000000");
    assert.deepEqual(usage.days, { "2026-10-03": "2000000000" });
    assert.deepEqual(usage.projects.map((project) => project.repository), ["o/one", "No repository (chats)"]);
    const missing = await loadAccountUsage({ ...octCycle, dbPath: join(dir, "nope.db") });
    assert.equal(missing.status, "unavailable");
});

test("groups combine models/sessions/categories without losing integer cost or duplicating bundles", () => {
    const groups = groupBuildCosts(measured);
    assert.equal(groups.length, 3);
    assert.equal(groups[0].label, "Farming, fishing and meals");
    assert.equal(groups[0].recordedNanoAiu, "1123450000000");
    assert.equal(groups[1].recordedNanoAiu, "300000000000");
    assert.equal(groups.reduce((sum, value) => sum + BigInt(value.recordedNanoAiu), 0n), BigInt(measured.totals.recordedNanoAiu));
    const wide = report("many", Array.from({ length: 20 }, (_, index) =>
        segment("feature-" + index, "implementation", "1234567891")));
    const compact = groupBuildCosts(wide);
    assert.equal(compact.length, 8);
    assert.equal(compact.at(-1).label, "Other features and shared work");
    assert.equal(compact.reduce((sum, value) => sum + BigInt(value.recordedNanoAiu), 0n), BigInt(wide.totals.recordedNanoAiu));
    assert.throws(() => groupBuildCosts({ ...measured, totals: { ...measured.totals, recordedNanoAiu: "1" } }), /does not match/);
});

test("estimated and missing call costs stay separate and unshipped reports do not enter the chart", () => {
    const partial = report("partial", [
        segment("task", "implementation", "1000000000"),
        segment("task", "implementation", "0", { recordedCalls: 0, estimatedNanoAiu: "2000000000" }),
    ]);
    const view = buildView({ reports: [measured, partial], builds: [{ ...shipment, buildId: "partial" },
        { ...shipment, status: "planned" }, { ...shipment, status: "historical" }, { ...shipment, status: "deferred" }] });
    assert.equal(view.shipped.length, 1);
    assert.equal(view.shipped[0].recordedNanoAiu, "1000000000");
    assert.equal(view.shipped[0].estimatedNanoAiu, "2000000000");
    assert.equal(view.period.recordedNanoAiu, "1000000000");
    assert.equal(view.period.unknown, true);
    assert.equal(view.unshippedReports, 1);
});

test("expired/missing/invalid cycles never masquerade as a current billing period", () => {
    const expired = buildView({ now: Date.parse("2026-10-31T07:00:00Z") });
    assert.equal(expired.period.status, "expired");
    assert.deepEqual(expired.period.days, []);
    assert.match(expired.period.message, /need updating/);
    assert.equal(buildView({ cycle: null }).period.status, "missing");
    assert.throws(() => buildView({ cycle: { ...cycle, start: "2026-09-31" } }), /Invalid/);
    assert.throws(() => buildView({ cycle: { ...cycle, reset: "2026-11-01" } }), /Invalid billing period/);
    assert.throws(() => buildView({ reports: [measured, measured] }), /Duplicate accounting/);
    assert.throws(() => buildView({ builds: [shipment, shipment] }), /Duplicate shipment/);
});

test("receipt timezone is explicit and shipped facts do not change with editable or removed feedback", () => {
    const view = buildView({ builds: [{ ...shipment, date: "2026-10-03", slot: "9 PM" }],
        manifests: [{ buildId: measured.buildId, deliveredAt: "2026-10-04T06:59:00Z",
            plannerDelivery: { status: "shipped", selectedIds: ["backlog:stable"] } }],
        features: [{ id: "backlog:stable", title: "Player-selected feature" }] });
    assert.equal(view.shipped[0].label, "2026-10-03 — 11:59 PM");
    assert.deepEqual(view.shipped[0].features, shipment.shippedFeatures);
    assert.equal(view.period.days.find((day) => day.date === "2026-10-03").shipments, 1);
});

async function fixture(t) {
    const root = await mkdtemp(join(tmpdir(), "planner-costs-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    const handoff = join(root, "docs", "handoff");
    await mkdir(handoff, { recursive: true });
    const builds = [
        "# Builds", "",
        "## 2026-10-03 — 9 PM", "- Status: deferred", "- Ships:", "- Unshipped old scope",
        "## 2026-10-04 — 11:29 AM", "- Build ID: " + measured.buildId, "- Status: historical",
        "- Ships: Superseded admission", "- Status: delivered", "- Ships:",
        "- Six fish and eleven provisional meals", "- Plant Seeds hint", "- Verification: internal",
        "- Not a feature after metadata",
        "## 2026-10-02 — 11:46 PM", "- Build ID: older", "- Status: delivered", "- Ships:", "- Pack layout",
        "## 2026-10-01 — 7:30 AM", "- Status: delivered", "- Ships:", "- Unmeasured old feature",
        "## 2026-10-01 — early evening", "- Status: delivered", "- Ships:", "- Incomplete receipt",
        "## Later", "- Unselected future work",
    ].join("\n");
    const paths = [join(handoff, "builds.md"), join(handoff, "priority.json"), join(handoff, "backlog-inbox.json")];
    await writeFile(paths[0], builds);
    await writeFile(paths[1], '{"order":[],"slots":{},"removed":[]}');
    await writeFile(paths[2], '{"entries":[]}');
    const older = report("older", [segment("place-items-in-exact-slots", "implementation", "200000000000")]);
    for (const value of [measured, older]) {
        const path = join(handoff, "accounting", "reports", value.buildId + ".json");
        await persistUsageReport(path, value); paths.push(path);
    }
    return { root, paths };
}

test("HTTP projection preserves raw reports and all fixture data; malformed metadata fails visibly", async (t) => {
    const { root, paths } = await fixture(t);
    const before = await Promise.all(paths.map((path) => readFile(path, "utf8")));
    const session = new Proxy({}, { get() { throw new Error("Agent access forbidden"); } });
    const { server, url } = await startPlannerServer(root, "cost-http", session);
    t.after(() => new Promise((resolve) => { server.closeAllConnections(); server.close(resolve); }));
    const response = await fetch(url + "api/tasks");
    assert.equal(response.status, 200);
    const planner = await response.json();
    assert.deepEqual(planner.costView.shipped[0].features, shipment.shippedFeatures);
    assert.equal(planner.costView.shipped[0].label, "2026-10-04 — 11:29 AM");
    assert.equal(planner.accounting[0].records, undefined);
    assert.deepEqual(await Promise.all(paths.map((path) => readFile(path, "utf8"))), before);
    await writeFile(join(root, "docs", "handoff", "measured-build-bad.json"), "{bad");
    const bad = await fetch(url + "api/tasks");
    assert.equal(bad.status, 500);
    assert.ok((await bad.json()).error);
});

test("cost tab in an isolated browser shows the chart, summaries, strict headings and responsive states", {
    skip: !process.env.PLANNER_BROWSER_MODULE && "Set PLANNER_BROWSER_MODULE for isolated headless proof.",
}, async (t) => {
    assert.match(tmpdir(), /^E:[\\/]/i);
    const { chromium } = await import(pathToFileURL(process.env.PLANNER_BROWSER_MODULE).href);
    const { root, paths } = await fixture(t);
    const before = await Promise.all(paths.map((path) => readFile(path, "utf8")));
    const planner = await loadPlanner(root, now);
    const session = new Proxy({}, { get() { throw new Error("Agent access forbidden"); } });
    const { server, url } = await startPlannerServer(root, "cost-browser", session);
    t.after(() => new Promise((resolve) => { server.closeAllConnections(); server.close(resolve); }));
    const browser = await chromium.launch({ headless: true, executablePath: process.env.PLANNER_BROWSER_EXECUTABLE
        ?? "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe" });
    t.after(() => browser.close());
    const page = await browser.newPage({ viewport: { width: 1440, height: 1100 } });
    const errors = [];
    const writes = [];
    page.on("pageerror", (error) => errors.push(error.message));
    page.on("request", (request) => { if (!["GET", "HEAD"].includes(request.method())) writes.push(request.method()); });
    await page.route("**/api/tasks", (route) => route.fulfill({ json: planner }));
    await page.goto(url);
    await page.locator("#builds .build-title").first().waitFor();
    assert.doesNotMatch(await page.locator("#builds").innerText(), /Superseded admission|Unshipped old scope|deferred|early evening/);
    const deliveredLabels = await page.locator("#builds .build-card .build-title strong").allTextContents();
    assert.ok(deliveredLabels.includes("2026-10-04 — 11:29 AM"));
    assert.ok(deliveredLabels.includes("Invalid shipment date/time"));
    await page.getByRole("tab", { name: "Build cost", exact: true }).click();
    const cost = page.locator("#accounting");
    assert.equal(await cost.locator(".cost-day").count(), 31);
    assert.equal(await cost.locator("table").count(), 0);
    assert.equal(await cost.locator("h3").first().textContent(), "2026-10-04 — 11:29 AM");
    assert.doesNotMatch(await cost.innerText(), /Superseded admission|internal|Not a feature|Unshipped old scope/);
    await cost.locator('.cost-day[data-date="2026-10-03"]').click();
    assert.match(await cost.locator(".cost-chart-readout").innerText(), /0 recorded AIU.*0 shipped builds/);
    await page.keyboard.press("ArrowRight");
    assert.match(await cost.locator(".cost-chart-readout").innerText(), /2026-10-04.*1,493.45 recorded AIU/);
    assert.match(await cost.locator('.cost-day[data-date="2026-10-01"]').getAttribute("aria-label"), /Unknown cost/);
    if (process.env.PLANNER_COST_EVIDENCE) {
        await mkdir(process.env.PLANNER_COST_EVIDENCE, { recursive: true });
        await page.screenshot({ path: join(process.env.PLANNER_COST_EVIDENCE, "cost-1440.png"), fullPage: true });
    }
    await page.setViewportSize({ width: 390, height: 1100 });
    assert.ok(await page.evaluate(() => document.documentElement.scrollWidth <= innerWidth));
    assert.ok(await cost.locator(".cost-chart-scroll").evaluate((node) => node.scrollWidth > node.clientWidth));
    if (process.env.PLANNER_COST_EVIDENCE) {
        await page.screenshot({ path: join(process.env.PLANNER_COST_EVIDENCE, "cost-390.png"), fullPage: true });
    }
    const expired = { ...planner, costView: buildView({ now: Date.parse("2026-10-31T07:00:00Z") }) };
    await page.unroute("**/api/tasks");
    await page.route("**/api/tasks", (route) => route.fulfill({ json: expired }));
    await page.locator("#refresh").click();
    await page.getByText("Billing dates need updating.", { exact: false }).waitFor();
    assert.equal(await cost.locator(".cost-day").count(), 0);
    assert.deepEqual(errors, []);
    assert.deepEqual(writes, []);
    assert.deepEqual(await Promise.all(paths.map((path) => readFile(path, "utf8"))), before);
});
