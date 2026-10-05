// Account-wide AI credit usage across every project, read from the Copilot CLI session store.
// Read-only. Returns per-day (billing time zone) and per-project nano-AIU sums for a date range.
import { existsSync } from "node:fs";
import { homedir } from "node:os";
import { join } from "node:path";

const cacheMs = 30_000;
const dayMs = 86_400_000;
const cache = new Map();

export function defaultUsageDb() {
    return process.env.PLANNER_USAGE_DB || join(homedir(), ".copilot", "session-store.db");
}

function localDate(date, timeZone) {
    const parts = new Intl.DateTimeFormat("en-US", { timeZone, year: "numeric", month: "2-digit", day: "2-digit" })
        .formatToParts(date);
    return ["year", "month", "day"].map((type) => parts.find((part) => part.type === type).value).join("-");
}

// created_at is either ISO ("2026-10-01T00:00:03.406Z") or SQLite ("2026-10-01 00:00:03", UTC).
function parseUtc(value) {
    const text = String(value).replace(" ", "T");
    return new Date(/[zZ]|[+-]\d\d:?\d\d$/.test(text) ? text : text + "Z");
}

export function summarizeUsageRows(rows, { start, end, timeZone }) {
    const days = new Map();
    const repos = new Map();
    const sessions = new Set();
    let calls = 0, total = 0n;
    for (const row of rows) {
        const when = parseUtc(row.created_at);
        if (!Number.isFinite(when.getTime())) continue;
        const date = localDate(when, timeZone);
        if (date < start || date > end) continue;
        const nano = BigInt(row.nano ?? 0);
        days.set(date, (days.get(date) ?? 0n) + nano);
        const repo = row.repository || "No repository (chats)";
        const entry = repos.get(repo) ?? { repository: repo, nano: 0n, calls: 0, sessions: new Set() };
        entry.nano += nano; entry.calls += 1; entry.sessions.add(row.session_id);
        repos.set(repo, entry);
        sessions.add(row.session_id);
        total += nano; calls += 1;
    }
    return {
        totalNanoAiu: String(total), calls, sessions: sessions.size,
        days: Object.fromEntries([...days].map(([date, nano]) => [date, String(nano)])),
        projects: [...repos.values()].sort((a, b) => (a.nano > b.nano ? -1 : a.nano < b.nano ? 1 : 0))
            .map((entry) => ({ repository: entry.repository, nanoAiu: String(entry.nano), calls: entry.calls, sessions: entry.sessions.size })),
    };
}

export async function loadAccountUsage({ start, end, timeZone, dbPath = defaultUsageDb() }) {
    const key = [dbPath, start, end, timeZone].join("|");
    const hit = cache.get(key);
    if (hit && Date.now() - hit.at < cacheMs) return hit.value;
    let value;
    try {
        if (!existsSync(dbPath)) throw new Error("Copilot session store not found.");
        const { DatabaseSync } = await import("node:sqlite");
        const db = new DatabaseSync(dbPath, { readOnly: true });
        try {
            // One day of slack each side keeps the SQL prefix filter timezone-safe; exact bucketing is done above.
            const from = new Date(Date.parse(start + "T00:00:00Z") - dayMs).toISOString().slice(0, 10);
            const to = new Date(Date.parse(end + "T00:00:00Z") + 2 * dayMs).toISOString().slice(0, 10);
            const statement = db.prepare(`SELECT e.session_id AS session_id, e.created_at AS created_at,
                    e.total_nano_aiu AS nano, s.repository AS repository
                FROM assistant_usage_events e LEFT JOIN sessions s ON s.id = e.session_id
                WHERE substr(e.created_at, 1, 10) >= ? AND substr(e.created_at, 1, 10) < ?`);
            statement.setReadBigInts(true);
            value = { status: "ok", source: "Copilot session store, all projects",
                ...summarizeUsageRows(statement.all(from, to), { start, end, timeZone }) };
        } finally { db.close(); }
    } catch (error) {
        value = { status: "unavailable", message: String(error.message ?? error) };
    }
    cache.set(key, { at: Date.now(), value });
    return value;
}
