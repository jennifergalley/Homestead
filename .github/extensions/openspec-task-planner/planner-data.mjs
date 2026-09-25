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
        if (entry.name.startsWith(".")) continue;
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
        const task = line.match(/^- \[([ xX])\]\s+([0-9]+(?:\.[0-9]+)?)\s*(.+?)\s*$/);
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
    const features = await Promise.all(taskFiles.map((path) => loadFeature(projectRoot, path, activeChange)));
    features.sort((a, b) => {
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
    };
}
