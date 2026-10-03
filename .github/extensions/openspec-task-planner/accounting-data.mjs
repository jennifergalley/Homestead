import { readFile, readdir, mkdir, writeFile, rename } from "node:fs/promises";
import { dirname, join } from "node:path";

const classes = ["input", "cache_read", "cache_write", "output"];
const emptyCosts = () => Object.fromEntries(classes.map((key) => [key, 0n]));

function integer(value, label) {
    if ((typeof value === "number" && Number.isSafeInteger(value) && value >= 0)
        || (typeof value === "string" && /^\d+$/.test(value))) return BigInt(value);
    throw new Error(`Invalid nonnegative integer: ${label}`);
}

function costsFor(event) {
    const details = typeof event.token_details_json === "string"
        ? JSON.parse(event.token_details_json) : event.token_details_json;
    const costs = emptyCosts();
    let other = 0n;
    if (details == null) return { costs, other, available: false };
    if (!Array.isArray(details)) throw new Error(`Invalid token details for ${event.id}`);
    for (const detail of details) {
        const batch = integer(detail.batchSize, "batchSize");
        if (!batch) throw new Error("Zero billing batch size");
        const cost = integer(detail.tokenCount, "tokenCount")
            * integer(detail.costPerBatch, "costPerBatch") / batch;
        if (classes.includes(detail.tokenType)) costs[detail.tokenType] += cost;
        else other += cost;
    }
    return { costs, other, available: details.length > 0 };
}

function matches(segment, event) {
    const time = Date.parse(event.created_at);
    return segment.sessionId === event.session_id
        && (segment.agentId === undefined || segment.agentId === (event.agent_id ?? "main"))
        && event.id > (segment.afterEventId ?? 0)
        && event.id <= (segment.throughEventId ?? Infinity)
        && time >= Date.parse(segment.since)
        && (!segment.until || time < Date.parse(segment.until));
}

function bucket(identity) {
    return { ...identity, calls: 0, recordedCalls: 0, estimatedCalls: 0, ratedCalls: 0, recorded: 0n,
        estimated: 0n, costs: emptyCosts(), other: 0n, firstEventId: null, lastEventId: null,
        since: null, until: null, maxPromptTokens: 0 };
}

function add(target, event, rating) {
    target.calls++;
    if (event.total_nano_aiu != null) {
        target.recordedCalls++;
        target.recorded += integer(event.total_nano_aiu, "total_nano_aiu");
    } else if (rating.available) {
        target.estimatedCalls++;
        target.estimated += Object.values(rating.costs).reduce((a, b) => a + b, rating.other);
    }
    if (rating.available) target.ratedCalls++;
    for (const key of classes) target.costs[key] += rating.costs[key];
    target.other += rating.other;
    target.firstEventId = Math.min(target.firstEventId ?? event.id, event.id);
    target.lastEventId = Math.max(target.lastEventId ?? event.id, event.id);
    if (!target.since || Date.parse(event.created_at) < Date.parse(target.since)) target.since = event.created_at;
    if (!target.until || Date.parse(event.created_at) > Date.parse(target.until)) target.until = event.created_at;
    target.maxPromptTokens = Math.max(target.maxPromptTokens, event.input_tokens ?? 0);
}

function serialize(target) {
    const { recorded, estimated, costs, other, ...rest } = target;
    const rated = Object.values(costs).reduce((a, b) => a + b, other);
    return { ...rest, unknownCostCalls: target.calls - target.recordedCalls - target.estimatedCalls,
        recordedNanoAiu: String(recorded), estimatedNanoAiu: String(estimated),
        tokenCostsNanoAiu: Object.fromEntries(Object.entries(costs).map(([key, value]) => [key, String(value)])),
        otherTokenCostsNanoAiu: String(other),
        recordedMinusRatedNanoAiu: target.recordedCalls === target.ratedCalls && target.recordedCalls === target.calls
            ? String(recorded - rated) : null };
}

export function buildUsageReport(allocation, snapshots, generatedAt = new Date().toISOString()) {
    if (!/^[\w-]+$/.test(allocation.buildId) || !Number.isFinite(Date.parse(allocation.authorizedAt))) {
        throw new Error("Invalid build ID or authorization timestamp");
    }
    for (const segment of allocation.segments) {
        if (!segment.sessionId || !segment.task || !segment.category || !Number.isFinite(Date.parse(segment.since))
            || (segment.until && (!Number.isFinite(Date.parse(segment.until)) || Date.parse(segment.until) <= Date.parse(segment.since)))) {
            throw new Error("Invalid allocation segment");
        }
    }
    const unique = new Map();
    const captures = [];
    for (const snapshot of snapshots) {
        if (snapshot.schemaVersion !== 1 || snapshot.sourceId !== "local-assistant-usage-v1") {
            throw new Error("Unsupported usage source; inclusive shutdown/snapshot totals are not call records");
        }
        captures.push({ sourceId: snapshot.sourceId, capturedAt: snapshot.capturedAt,
            throughEventId: snapshot.throughEventId, sessionIds: snapshot.sessionIds, since: snapshot.since });
        for (const event of snapshot.events) {
            if (!Number.isSafeInteger(event.id) || event.id < 0 || !Number.isFinite(Date.parse(event.created_at))) throw new Error("Invalid usage event identity/time");
            const key = `${snapshot.sourceId}:${event.id}`;
            const previous = unique.get(key);
            if (previous && JSON.stringify(previous) !== JSON.stringify(event)) throw new Error(`Conflicting usage record ${key}`);
            unique.set(key, event);
        }
    }
    const totals = bucket({});
    const segments = new Map();
    const records = [];
    let excludedCalls = 0;
    for (const [recordId, event] of unique) {
        if (Date.parse(event.created_at) < Date.parse(allocation.authorizedAt)) { excludedCalls++; continue; }
        const candidates = allocation.segments.filter((segment) => matches(segment, event));
        if (candidates.length > 1) throw new Error(`Overlapping allocations for ${recordId}`);
        const segment = candidates[0];
        if (!segment) { excludedCalls++; continue; }
        const helper = event.agent_id != null && event.agent_id !== "main";
        const contextTier = helper && segment.agentId === undefined ? null : segment.contextTier ?? null;
        const identity = { task: segment.task, tasks: segment.tasks ?? [segment.task], category: segment.category,
            sessionId: event.session_id, agentId: event.agent_id ?? "main",
            projectSessionId: segment.projectSessionId ?? null,
            model: event.model ?? null, reasoningEffort: event.reasoning_effort ?? null,
            contextTier, contextEvidence: contextTier ? segment.contextEvidence ?? "explicit allocation" : null,
            launchContextTier: segment.launchContextTier ?? null,
            retryClassification: segment.retryClassification ?? "unknown" };
        const key = JSON.stringify(identity);
        if (!segments.has(key)) segments.set(key, bucket(identity));
        const rating = costsFor(event);
        add(segments.get(key), event, rating);
        add(totals, event, rating);
        records.push({ recordId, ...event });
    }
    const missingSessions = [...new Set(allocation.segments.map((segment) => segment.sessionId))]
        .filter((id) => !records.some((event) => event.session_id === id));
    const limits = [...(allocation.limits ?? [])];
    if (missingSessions.length) limits.push(`No call records captured for: ${missingSessions.join(", ")}`);
    if (totals.recordedCalls !== totals.calls) limits.push("Some calls lack recorded nano-AIU; supplied-rate estimates are separate.");
    if (totals.ratedCalls !== totals.calls) limits.push("Some calls lack token-class billing detail.");
    if ([...segments.values()].some((segment) => !segment.contextTier)) limits.push("Some configured context tiers are unknown.");
    return { schemaVersion: 1, buildId: allocation.buildId, authorizedAt: allocation.authorizedAt, generatedAt,
        status: "incomplete", unit: "nano-AIU", displayUnit: "AIU", nanoPerAiu: "1000000000",
        unitEvidence: "Runtime assistant_usage_events.total_nano_aiu; AIU display is nano / 1e9. Not reconciled to billing credits.",
        allocationPolicy: allocation.allocationPolicy ?? null,
        legacy: allocation.legacy ?? { status: "unattributed", recordedNanoAiu: null },
        coverage: { limits, missingSessions, excludedCalls, retryClassification: "Manual event-range classification; unclassified calls remain unknown",
            billingReconciliation: "not supplied", finalCapture: allocation.finalCapture === true },
        totals: serialize(totals), segments: [...segments.values()].map(serialize), captures, records };
}

export async function persistUsageReport(path, report) {
    await mkdir(dirname(path), { recursive: true });
    const temporary = `${path}.${process.pid}.tmp`;
    await writeFile(temporary, JSON.stringify(report, null, 2) + "\n", "utf8");
    await rename(temporary, path);
}

export async function loadUsageReports(projectRoot) {
    const directory = join(projectRoot, "docs", "handoff", "accounting", "reports");
    let files;
    try { files = await readdir(directory); }
    catch (error) { if (error.code === "ENOENT") return []; throw error; }
    return Promise.all(files.filter((file) => file.endsWith(".json")).sort().map(async (file) => {
        const report = JSON.parse(await readFile(join(directory, file), "utf8"));
        if (report.schemaVersion !== 1 || !report.totals || !Array.isArray(report.segments)) {
            throw new Error(`Invalid accounting report ${file}`);
        }
        const { records, ...summary } = report;
        return summary;
    }));
}
