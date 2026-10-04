import assert from "node:assert/strict";
import { mkdtemp, mkdir, writeFile, readFile, rm } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import test from "node:test";
import { buildCostView, groupBuildCosts } from "../.github/extensions/openspec-task-planner/cost-view.mjs";
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
