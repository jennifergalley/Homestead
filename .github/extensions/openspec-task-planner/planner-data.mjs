import { readFile, readdir, stat } from "node:fs/promises";
import { basename, dirname, join, relative } from "node:path";

const activeWindowMs = 15 * 60 * 1000;
const statusOrder = { active: 0, paused: 1, proposed: 2, complete: 3 };

function titleCase(value) {
    return value
        .replace(/[-_]+/g, " ")
        .replace(/\b\w/g, (letter) => letter.toUpperCase());
}

async function findTaskFiles(directory) {
    const found = [];
    let entries = [];
    try {
        entries = await readdir(directory, { withFileTypes: true });
    } catch (error) {
        if (error?.code === "ENOENT") return found;
        throw error;
    }
    for (const entry of entries) {
        if (entry.name.startsWith(".") || entry.name === "archive") continue;
        const path = join(directory, entry.name);
        if (entry.isDirectory()) {
            found.push(...await findTaskFiles(path));
        } else if (entry.isFile() && entry.name === "tasks.md") {
            found.push(path);
        }
    }
    return found;
}

function parseTasks(markdown) {
    const sections = [];
    let section = { title: "Tasks", tasks: [] };
    let currentTask = null;
    for (const line of markdown.split(/\r?\n/)) {
        const heading = line.match(/^##\s+(.+?)\s*$/);
        if (heading) {
            if (section.tasks.length) sections.push(section);
            section = { title: heading[1].trim(), tasks: [] };
            currentTask = null;
            continue;
        }
        const task = line.match(/^- \[([ xX])\]\s+([0-9]+(?:\.[0-9]+)?)\.?\s*(.+?)\s*$/);
        if (task) {
            currentTask = {
                id: task[2],
                text: task[3],
                done: task[1].toLowerCase() === "x",
            };
            section.tasks.push(currentTask);
            continue;
        }
        if (currentTask && /^\s{2,}\S/.test(line)) {
            currentTask.text += ` ${line.trim()}`;
        } else if (line.trim()) {
            currentTask = null;
        }
    }
    if (section.tasks.length) sections.push(section);
    return sections;
}

function parseBuilds(markdown) {
    const entries = [];
    const later = [];
    let current = null;
    let inLater = false;
    for (const line of markdown.split(/\r?\n/)) {
        const heading = line.match(/^##\s+(.+?)\s*$/);
        if (heading) {
            inLater = heading[1].trim().toLowerCase() === "later";
            current = null;
            if (!inLater) {
                const [date, slot] = heading[1].split(/\s+—\s+/, 2);
                current = { date: date?.trim() ?? heading[1].trim(), slot: slot?.trim() ?? "", sha: "pending", status: "planned", ships: [] };
                entries.push(current);
            }
            continue;
        }
        const property = line.match(/^\s*-\s+(SHA|Status):\s*(.+?)\s*$/i);
        if (property && current) {
            current[property[1].toLowerCase()] = property[2].replace(/`/g, "");
            continue;
        }
        const bullet = line.match(/^\s*-\s+(.+?)\s*$/);
        if (!bullet) continue;
        const text = bullet[1].replace(/`/g, "").replace(/\[([^\]]+)\]\([^)]*\)/g, "$1");
        if (inLater) {
            later.push(text);
        } else if (current && text !== "Ships:") {
            current.ships.push(text);
        }
    }
    return { entries, later };
}

async function loadBuilds(projectRoot) {
    try {
        return parseBuilds(await readFile(join(projectRoot, "docs", "handoff", "builds.md"), "utf8"));
    } catch (error) {
        if (error?.code === "ENOENT") return { entries: [], later: [] };
        throw error;
    }
}

const slotMinutes = [7 * 60 + 30, 16 * 60, 21 * 60];
const pad = (value) => String(value).padStart(2, "0");
const dateKey = (date) => `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}`;

function slotTimeText(minutes) {
    const hour = Math.floor(minutes / 60), minute = minutes % 60;
    return `${hour % 12 || 12}${minute ? ":" + pad(minute) : ""} ${hour < 12 ? "AM" : "PM"}`;
}

function parseSlotMinutes(text) {
    const match = /^(\d{1,2})(?::(\d{2}))?\s*(AM|PM)$/i.exec((text ?? "").trim());
    if (!match) return null;
    return (Number(match[1]) % 12 + (match[3].toUpperCase() === "PM" ? 12 : 0)) * 60 + Number(match[2] ?? 0);
}

export function buildKey(build) {
    const minutes = parseSlotMinutes(build.slot);
    return minutes === null || !/^\d{4}-\d{2}-\d{2}$/.test(build.date) ? null
        : `${build.date} ${pad(Math.floor(minutes / 60))}:${pad(minutes % 60)}`;
}

function keyTime(key) {
    const match = /^(\d{4})-(\d{2})-(\d{2}) (\d{2}):(\d{2})$/.exec(key ?? "");
    return match ? new Date(+match[1], +match[2] - 1, +match[3], +match[4], +match[5]).getTime() : NaN;
}

const namedSlotMinutes = { morning: 9 * 60, midday: 12 * 60, noon: 12 * 60, afternoon: 15 * 60, "early evening": 18 * 60, evening: 20 * 60, night: 22 * 60, overnight: 23 * 60 };

function buildTime(build) {
    const exact = keyTime(buildKey(build));
    if (Number.isFinite(exact)) return exact;
    const day = /^(\d{4})-(\d{2})-(\d{2})$/.exec(build.date ?? "");
    if (!day) return NaN;
    const minutes = namedSlotMinutes[(build.slot ?? "").trim().toLowerCase()] ?? 12 * 60;
    return new Date(+day[1], +day[2] - 1, +day[3], Math.floor(minutes / 60), minutes % 60).getTime();
}

function slotLabel(time, now) {
    const date = new Date(time);
    const today = new Date(now);
    const tomorrow = new Date(today.getFullYear(), today.getMonth(), today.getDate() + 1);
    const minutes = date.getHours() * 60 + date.getMinutes();
    const day = dateKey(date) === dateKey(today) ? (minutes >= 18 * 60 ? "Tonight" : "Today")
        : dateKey(date) === dateKey(tomorrow) ? "Tomorrow"
        : date.toLocaleDateString("en-US", { weekday: "short", month: "short", day: "numeric" });
    return `${day} ${slotTimeText(minutes)}`;
}

export function upcomingSlots(builds, now = Date.now(), count = 3) {
    const delivered = new Set(builds.entries.filter((build) => build.status === "delivered").map(buildKey).filter(Boolean));
    const slots = [];
    const start = new Date(now);
    for (let day = 0; slots.length < count && day < 14; day++) {
        for (const minutes of slotMinutes) {
            const time = new Date(start.getFullYear(), start.getMonth(), start.getDate() + day, Math.floor(minutes / 60), minutes % 60).getTime();
            const key = `${dateKey(new Date(time))} ${pad(Math.floor(minutes / 60))}:${pad(minutes % 60)}`;
            if (time <= now || delivered.has(key)) continue;
            slots.push({ key, label: slotLabel(time, now), date: dateKey(new Date(time)), slot: slotTimeText(minutes) });
            if (slots.length === count) break;
        }
    }
    return slots;
}

export function scheduleFeatures(features, priority, builds, now = Date.now()) {
    const slots = upcomingSlots(builds, now, 3);
    const open = new Set(slots.map((slot) => slot.key));
    const labels = new Map(slots.map((slot) => [slot.key, slot.label]));
    const assigned = priority.slots && typeof priority.slots === "object" ? priority.slots : {};
    const legacyNext = new Set(Array.isArray(priority.nextBuild) ? priority.nextBuild : []);
    for (const feature of features) {
        let key = typeof assigned[feature.id] === "string" ? assigned[feature.id]
            : legacyNext.has(feature.id) && slots[0] ? slots[0].key : null;
        feature.carriedFrom = null;
        if (key && !open.has(key) && feature.status !== "complete") {
            const time = keyTime(key);
            feature.carriedFrom = Number.isFinite(time)
                ? new Date(time).toLocaleDateString("en-US", { weekday: "short" }) + " " + slotTimeText(new Date(time).getHours() * 60 + new Date(time).getMinutes())
                : key;
            key = slots[0]?.key ?? null;
        }
        feature.slot = key;
        feature.slotLabel = key ? labels.get(key) ?? key : null;
        feature.nextBuild = !!key;
    }
    return slots;
}

async function readActiveChange(projectRoot, now) {
    let run;
    let status;
    try {
        [run, status] = await Promise.all([
            readFile(join(projectRoot, "Automation", "run.json"), "utf8").then(JSON.parse),
            readFile(join(projectRoot, "Automation", "status.json"), "utf8").then(JSON.parse),
        ]);
    } catch (error) {
        if (error?.code === "ENOENT") return null;
        throw error;
    }
    const updated = Date.parse(status.updatedUtc);
    if (run.state !== "running" || run.id !== status.runId || status.phase !== "implementing"
        || typeof run.openSpecChange !== "string" || !Number.isFinite(updated)
        || updated > now || now - updated > activeWindowMs) return null;
    return run.openSpecChange;
}

async function loadFeature(projectRoot, tasksPath, activeChange) {
    const markdown = await readFile(tasksPath, "utf8");
    const sections = parseTasks(markdown);
    const tasks = sections.flatMap((section) => section.tasks);
    const completed = tasks.filter((task) => task.done).length;
    const changeName = basename(dirname(tasksPath));
    const metadata = await stat(tasksPath);
    return {
        id: relative(join(projectRoot, "openspec", "changes"), dirname(tasksPath))
            .replaceAll("\\", "/"),
        name: changeName,
        title: titleCase(changeName),
        path: relative(projectRoot, tasksPath).replaceAll("\\", "/"),
        modifiedAt: metadata.mtime.toISOString(),
        completed,
        total: tasks.length,
        status: tasks.length > 0 && completed === tasks.length ? "complete"
            : changeName === activeChange ? "active"
            : completed === 0 ? "proposed" : "paused",
        sections,
    };
}

export async function loadPlanner(projectRoot, now = Date.now()) {
    const taskFiles = await findTaskFiles(join(projectRoot, "openspec", "changes"));
    const activeChange = await readActiveChange(projectRoot, now);
    const [features, builds, priority] = await Promise.all([
        Promise.all(taskFiles.map((path) => loadFeature(projectRoot, path, activeChange))),
        loadBuilds(projectRoot),
        readFile(join(projectRoot, "docs", "handoff", "priority.json"), "utf8")
            .then((text) => JSON.parse(text)).catch(() => ({})),
    ]);
    const order = new Map((Array.isArray(priority.order) ? priority.order : []).map((id, index) => [id, index]));
    const removed = new Set(Array.isArray(priority.removed) ? priority.removed : []);
    for (let i = features.length - 1; i >= 0; i--) if (removed.has(features[i].id)) features.splice(i, 1);
    const slots = scheduleFeatures(features, priority, builds, now);
    for (const build of builds.entries) {
        build.key = buildKey(build);
        build.time = buildTime(build);
    }
    features.sort((a, b) => {
        const ao = order.has(a.id) ? order.get(a.id) : Infinity;
        const bo = order.has(b.id) ? order.get(b.id) : Infinity;
        if (ao !== bo) return ao - bo;
        if (a.status !== b.status) return statusOrder[a.status] - statusOrder[b.status];
        return b.modifiedAt.localeCompare(a.modifiedAt);
    });
    const completedTasks = features.reduce((sum, feature) => sum + feature.completed, 0);
    const totalTasks = features.reduce((sum, feature) => sum + feature.total, 0);
    return {
        generatedAt: new Date().toISOString(),
        project: "Homestead",
        summary: {
            completedTasks,
            totalTasks,
            activeFeatures: features.filter((feature) => feature.status === "active").length,
            proposedFeatures: features.filter((feature) => feature.status === "proposed").length,
            pausedFeatures: features.filter((feature) => feature.status === "paused").length,
            completedFeatures: features.filter((feature) => feature.status === "complete").length,
        },
        features,
        builds,
        slots,
    };
}
