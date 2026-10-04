import { readFile, readdir } from "node:fs/promises";
import { join } from "node:path";
import { shipmentTiming } from "./build-time.mjs";

const dayMs = 86_400_000;
const featureLabels = {
    "apply-dark-theme-everywhere": "Book and HUD theme",
    "fix-map-click-travel": "Map travel",
    "place-items-in-exact-slots": "Pack layout",
    "add-dollars-and-general-store": "Crop selling",
    "move-town-signpost": "Town signpost",
    "planner-feedback-editing": "Planner feedback editing",
    "travel-rest-feedback-bundle": "Travel, sleep and shop waiting",
    "farming-fishing-and-provisional-food": "Farming, fishing and meals",
    "hud-feedback-bundle": "Chest labels and HUD notices",
};
const categoryLabels = {
    overhead: "Coordination and shared tooling",
    integration: "Integration and delivery",
    review: "Review and verification",
    rework: "Rework",
};

function nano(value) {
    if (typeof value !== "string" || !/^\d+$/.test(value)) throw new Error("Invalid recorded cost.");
    return BigInt(value);
}

function dateTime(value) {
    if (!/^\d{4}-\d{2}-\d{2}$/.test(value ?? "")) throw new Error("Invalid billing or shipment date.");
    const time = Date.parse(value + "T00:00:00Z");
    if (!Number.isFinite(time) || new Date(time).toISOString().slice(0, 10) !== value) {
        throw new Error("Invalid billing or shipment date.");
    }
    return time;
}

function localDate(value, timeZone) {
    const date = new Date(value);
    if (!Number.isFinite(date.getTime())) throw new Error("Invalid shipment timestamp.");
    const parts = new Intl.DateTimeFormat("en-US", { timeZone, year: "numeric", month: "2-digit", day: "2-digit" })
        .formatToParts(date);
    return ["year", "month", "day"].map((type) => parts.find((part) => part.type === type).value).join("-");
}

export function groupBuildCosts(report) {
    const grouped = new Map();
    for (const segment of report.segments) {
        const label = segment.category === "implementation"
            ? featureLabels[segment.task] ?? segment.task.replaceAll("-", " ")
            : categoryLabels[segment.category] ?? "Other shared work";
        const group = grouped.get(label) ?? { label, recorded: 0n, estimated: 0n, recordedCalls: 0, calls: 0 };
        group.recorded += nano(segment.recordedNanoAiu);
        group.estimated += nano(segment.estimatedNanoAiu);
        group.recordedCalls += segment.recordedCalls;
        group.calls += segment.calls;
        grouped.set(label, group);
    }
    const groups = [...grouped.values()].sort((a, b) => a.recorded > b.recorded ? -1 : a.recorded < b.recorded ? 1 : 0);
    if (groups.reduce((sum, group) => sum + group.recorded, 0n) !== nano(report.totals.recordedNanoAiu)
        || groups.reduce((sum, group) => sum + group.estimated, 0n) !== nano(report.totals.estimatedNanoAiu)) {
        throw new Error(`Cost breakdown does not match build total: ${report.buildId}`);
    }
    if (groups.length > 8) {
        const other = groups.splice(7);
        groups.push({ label: "Other features and shared work",
            recorded: other.reduce((sum, group) => sum + group.recorded, 0n),
            estimated: other.reduce((sum, group) => sum + group.estimated, 0n),
            recordedCalls: other.reduce((sum, group) => sum + group.recordedCalls, 0),
            calls: other.reduce((sum, group) => sum + group.calls, 0) });
    }
    return groups.map(({ recorded, estimated, ...group }) => ({ ...group,
        recordedNanoAiu: group.recordedCalls ? String(recorded) : null,
        estimatedNanoAiu: String(estimated) }));
}

export function buildCostView({ reports = [], builds = [], manifests = [], cycle = null,
    now = Date.now() }) {
    const timeZone = cycle?.timeZone ?? "America/Los_Angeles";
    const today = localDate(now, timeZone);
    const byId = new Map();
    for (const report of reports) {
        if (byId.has(report.buildId)) throw new Error(`Duplicate accounting build: ${report.buildId}`);
        byId.set(report.buildId, report);
    }
    const metadata = new Map();
    for (const manifest of manifests) {
        const id = manifest.buildId ?? manifest.plannerDelivery?.buildId;
        if (!id) throw new Error("Missing delivery manifest build ID.");
        if (metadata.has(id)) throw new Error(`Duplicate delivery manifest: ${id}`);
        metadata.set(id, manifest);
    }
    const used = new Set();
    const shipped = builds.filter((build) => build.status === "delivered").map((build) => {
        if (build.buildId && used.has(build.buildId)) throw new Error(`Duplicate shipment: ${build.buildId}`);
        if (build.buildId) used.add(build.buildId);
        const report = byId.get(build.buildId);
        const manifest = metadata.get(build.buildId);
        const deliveredAt = manifest?.deliveredAt ?? null;
        const timing = shipmentTiming(build, deliveredAt, timeZone);
        const date = timing.date;
        const shippedFeatures = build.shippedFeatures ?? build.ships ?? [];
        return {
            buildId: build.buildId ?? null, date, deliveredAt,
            label: timing.label, time: timing.time, dataError: timing.dataError,
            features: [...new Set(shippedFeatures)],
            recordedNanoAiu: report?.totals.recordedCalls ? String(nano(report.totals.recordedNanoAiu)) : null,
            estimatedNanoAiu: report ? String(nano(report.totals.estimatedNanoAiu)) : "0",
            groups: report ? groupBuildCosts(report) : [],
            partial: !report || report.status !== "reconciled" || report.totals.recordedCalls !== report.totals.calls,
            missingCosts: !report || report.totals.recordedCalls !== report.totals.calls
                || report.legacy?.recordedNanoAiu == null || !!report.coverage?.missingSessions?.length,
            inheritedUnknown: report?.legacy?.recordedNanoAiu == null,
            reportPath: report ? `docs/handoff/accounting/reports/${report.buildId}.json` : null,
        };
    }).sort((a, b) => (b.time ?? -Infinity) - (a.time ?? -Infinity));

    let period = { status: "missing", days: [], message: "Confirm the current billing dates to show daily shipped-build cost." };
    if (cycle) {
        const start = dateTime(cycle.start), end = dateTime(cycle.end), reset = dateTime(cycle.reset);
        if (end < start || end - start > 366 * dayMs || reset !== end + dayMs) throw new Error("Invalid billing period.");
        period = { ...cycle, status: today < cycle.start || today > cycle.end ? "expired" : "current", days: [] };
        if (period.status === "current") {
            for (let day = start; day <= end; day += dayMs) {
                const date = new Date(day).toISOString().slice(0, 10);
                const shipments = shipped.filter((build) => build.date === date);
                period.days.push({ date, shipments: shipments.length, future: date > today,
                    recordedNanoAiu: String(shipments.reduce((sum, build) => sum + (build.recordedNanoAiu == null ? 0n : nano(build.recordedNanoAiu)), 0n)),
                    unknown: shipments.some((build) => build.recordedNanoAiu == null || build.missingCosts) });
            }
            period.recordedNanoAiu = String(period.days.reduce((sum, day) => sum + nano(day.recordedNanoAiu), 0n));
            period.unknown = period.days.some((day) => day.unknown);
        } else period.message = "Billing dates need updating. The last confirmed period is not current.";
    }
    return { period, shipped, unshippedReports: reports.filter((report) => !used.has(report.buildId)).length };
}

export async function loadBuildCostView(projectRoot, reports, builds, now = Date.now()) {
    const handoff = join(projectRoot, "docs", "handoff");
    let files;
    try { files = await readdir(handoff); }
    catch (error) { if (error.code !== "ENOENT") throw error; files = []; }
    const [cycle, manifests] = await Promise.all([
        readFile(new URL("./billing-cycle.json", import.meta.url), "utf8").then(JSON.parse),
        Promise.all(files.filter((file) => /^measured-build-[\w-]+\.json$/.test(file))
            .map(async (file) => JSON.parse(await readFile(join(handoff, file), "utf8")))),
    ]);
    return buildCostView({ reports, builds: builds.entries, manifests, cycle, now });
}
