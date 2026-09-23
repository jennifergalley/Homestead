import { readFile, readdir, stat } from "node:fs/promises";
import { basename, dirname, join, relative } from "node:path";

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

async function loadFeature(projectRoot, tasksPath) {
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
        status: tasks.length > 0 && completed === tasks.length ? "complete" : "active",
        sections,
    };
}

export async function loadPlanner(projectRoot) {
    const taskFiles = await findTaskFiles(join(projectRoot, "openspec", "changes"));
    const features = await Promise.all(taskFiles.map((path) => loadFeature(projectRoot, path)));
    features.sort((a, b) => {
        if (a.status !== b.status) return a.status === "active" ? -1 : 1;
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
            completedFeatures: features.filter((feature) => feature.status === "complete").length,
        },
        features,
    };
}
