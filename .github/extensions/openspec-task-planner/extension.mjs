import { createServer } from "node:http";
import { readFile, writeFile, mkdir } from "node:fs/promises";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";
import { createCanvas, joinSession } from "@github/copilot-sdk/extension";
import { loadPlanner } from "./planner-data.mjs";
import { renderPlannerHtml } from "./planner-html.mjs";

const projectRoot = fileURLToPath(new URL("../../../", import.meta.url));
const priorityPath = join(projectRoot, "docs", "handoff", "priority.json");
const servers = new Map();
let session = null;
let notifyTimer = null;
let pendingNotes = [];

async function readPriority() {
    try {
        const value = JSON.parse(await readFile(priorityPath, "utf8"));
        return { order: Array.isArray(value.order) ? value.order : [], nextBuild: Array.isArray(value.nextBuild) ? value.nextBuild : [], removed: Array.isArray(value.removed) ? value.removed : [] };
    } catch {
        return { order: [], nextBuild: [], removed: [] };
    }
}

async function writePriority(priority) {
    await mkdir(dirname(priorityPath), { recursive: true });
    await writeFile(priorityPath, JSON.stringify({ ...priority, updated: new Date().toISOString() }, null, 2) + "\n", "utf8");
}

async function readBody(req) {
    let body = "";
    for await (const chunk of req) {
        body += chunk;
        if (body.length > 200000) throw new Error("Request too large");
    }
    return body ? JSON.parse(body) : {};
}

function notifyOrchestrator(note) {
    pendingNotes.push(note);
    clearTimeout(notifyTimer);
    notifyTimer = setTimeout(async () => {
        const notes = pendingNotes;
        pendingNotes = [];
        if (!session) return;
        const planner = await loadPlanner(projectRoot);
        const titles = new Map(planner.features.map((feature) => [feature.id, feature.title]));
        const priority = await readPriority();
        const next = priority.nextBuild.map((id) => titles.get(id) ?? id);
        const top = priority.order.slice(0, 8).map((id, index) => `${index + 1}. ${titles.get(id) ?? id}`);
        await session.send({
            prompt: [
                "[Task planner] Jenny updated her backlog priorities in the canvas (" + notes.join("; ") + ").",
                "Flagged for the next build: " + (next.length ? next.join(", ") : "none") + ".",
                "Priority order (top 8):\n" + top.join("\n"),
                "docs/handoff/priority.json was updated in the Orchestrator worktree: commit and push it to main, assign the next-build items to their lanes, and add them to the next planned entry in docs/handoff/builds.md.",
            ].join("\n"),
        });
    }, 15000);
}

function sendJson(res, value, status = 200) {
    res.writeHead(status, {
        "Content-Type": "application/json; charset=utf-8",
        "Cache-Control": "no-store",
        "X-Content-Type-Options": "nosniff",
    });
    res.end(JSON.stringify(value));
}

async function startServer(instanceId) {
    const server = createServer(async (req, res) => {
        try {
            const url = new URL(req.url ?? "/", "http://127.0.0.1");
            if (req.method === "GET" && url.pathname === "/") {
                res.writeHead(200, {
                    "Content-Type": "text/html; charset=utf-8",
                    "Cache-Control": "no-store",
                    "Content-Security-Policy": "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'; connect-src 'self'; img-src 'self' data:",
                    "X-Content-Type-Options": "nosniff",
                });
                res.end(renderPlannerHtml());
                return;
            }
            if ((req.method === "GET" && url.pathname === "/api/tasks")
                || (req.method === "POST" && url.pathname === "/refresh")) {
                sendJson(res, await loadPlanner(projectRoot));
                return;
            }
            if (req.method === "GET" && url.pathname === "/health") {
                sendJson(res, { status: "ready", instanceId });
                return;
            }
            if (req.method === "POST" && url.pathname === "/api/priority") {
                const body = await readBody(req);
                const priority = await readPriority();
                if (Array.isArray(body.order)) {
                    priority.order = body.order.filter((id) => typeof id === "string").slice(0, 500);
                    notifyOrchestrator("reordered");
                }
                if (typeof body.toggleNext === "string") {
                    const id = body.toggleNext;
                    const on = !priority.nextBuild.includes(id);
                    priority.nextBuild = on ? [...priority.nextBuild, id] : priority.nextBuild.filter((value) => value !== id);
                    notifyOrchestrator((on ? "flagged " : "unflagged ") + id);
                }
                if (typeof body.remove === "string") {
                    const id = body.remove;
                    priority.removed = [...new Set([...(priority.removed ?? []), id])];
                    priority.nextBuild = priority.nextBuild.filter((value) => value !== id);
                    priority.order = priority.order.filter((value) => value !== id);
                    notifyOrchestrator("removed " + id + " (archive its OpenSpec change)");
                }
                await writePriority(priority);
                sendJson(res, priority);
                return;
            }
            if (req.method === "POST" && url.pathname === "/api/quote") {
                const body = await readBody(req);
                const planner = await loadPlanner(projectRoot);
                const feature = planner.features.find((item) => item.id === body.id);
                if (!feature) {
                    sendJson(res, { error: "Unknown item" }, 404);
                    return;
                }
                if (!session) throw new Error("Not connected to the chat yet");
                const tasks = feature.sections.flatMap((section) => section.tasks.map((task) => (task.done ? "[x] " : "[ ] ") + task.text));
                await session.rpc.extensions.sendAttachmentsToMessage({
                    instanceId,
                    attachments: [{
                        type: "extension_context",
                        title: feature.title,
                        payload: { kind: "planned-improvement", id: feature.id, path: feature.path, title: feature.title, tasks, nextBuild: !!feature.nextBuild },
                    }],
                });
                sendJson(res, { ok: true });
                return;
            }
            sendJson(res, { error: "Not found" }, 404);
        } catch (error) {
            sendJson(res, {
                error: error instanceof Error ? error.message : String(error),
            }, 500);
        }
    });
    await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
    const address = server.address();
    const port = typeof address === "object" && address ? address.port : 0;
    return { server, url: `http://127.0.0.1:${port}/` };
}

session = await joinSession({
    canvases: [
        createCanvas({
            id: "openspec-task-planner",
            displayName: "Homestead task planner",
            description: "Interactive progress board for every Homestead OpenSpec feature and task.",
            actions: [
                {
                    name: "refresh",
                    description: "Rescan all OpenSpec task files and return current progress totals.",
                    handler: async () => {
                        const planner = await loadPlanner(projectRoot);
                        return {
                            featureCount: planner.features.length,
                            completedTasks: planner.summary.completedTasks,
                            totalTasks: planner.summary.totalTasks,
                            activeFeatures: planner.summary.activeFeatures,
                            proposedFeatures: planner.summary.proposedFeatures,
                            pausedFeatures: planner.summary.pausedFeatures,
                            completedFeatures: planner.summary.completedFeatures,
                        };
                    },
                },
            ],
            open: async (ctx) => {
                let entry = servers.get(ctx.instanceId);
                if (!entry) {
                    entry = await startServer(ctx.instanceId);
                    servers.set(ctx.instanceId, entry);
                }
                const planner = await loadPlanner(projectRoot);
                return {
                    title: "Homestead OpenSpec tasks",
                    status: `${planner.summary.completedTasks}/${planner.summary.totalTasks} tasks complete`,
                    url: entry.url,
                };
            },
            onClose: async (ctx) => {
                const entry = servers.get(ctx.instanceId);
                if (!entry) return;
                servers.delete(ctx.instanceId);
                await new Promise((resolve) => entry.server.close(resolve));
            },
        }),
    ],
});
