import { createServer } from "node:http";
import { readFile, writeFile, mkdir } from "node:fs/promises";
import { dirname, join } from "node:path";
import { loadPlanner, loadBacklogInbox, addBacklogEntry, toBacklogClientEntry, backlogAttachmentsDir, BACKLOG_IMAGE_TYPES } from "./planner-data.mjs";
import { renderPlannerHtml } from "./planner-html.mjs";

export async function startPlannerServer(projectRoot, instanceId, session) {
    const priorityPath = join(projectRoot, "docs", "handoff", "priority.json");
    // Jenny's backlog-entry form, and reordering/scheduling in this canvas, are
    // deliberately agent-free: they write straight to disk (priority.json,
    // backlog-inbox.json, backlog.md) and the orchestrator picks up the change
    // next time it reads those files. Nothing here calls session.send().
    const BACKLOG_BODY_MAX_BYTES = 11 * 1024 * 1024; // ~8 MB image, base64-inflated, plus JSON overhead

    async function readPriority() {
        try {
            const value = JSON.parse(await readFile(priorityPath, "utf8"));
            return {
                order: Array.isArray(value.order) ? value.order : [],
                nextBuild: Array.isArray(value.nextBuild) ? value.nextBuild : [],
                slots: value.slots && typeof value.slots === "object" ? value.slots : {},
                removed: Array.isArray(value.removed) ? value.removed : [],
            };
        } catch {
            return { order: [], nextBuild: [], slots: {}, removed: [] };
        }
    }

    async function migrateNextBuild(priority) {
        if (!priority.nextBuild.length) return;
        const planner = await loadPlanner(projectRoot);
        const first = planner.slots[0]?.key;
        for (const id of priority.nextBuild) if (first && !priority.slots[id]) priority.slots[id] = first;
        priority.nextBuild = [];
    }

    async function writePriority(priority) {
        await mkdir(dirname(priorityPath), { recursive: true });
        await writeFile(priorityPath, JSON.stringify({ ...priority, updated: new Date().toISOString() }, null, 2) + "\n", "utf8");
    }

    async function readBody(req, maxBytes = 200000) {
        let body = "";
        for await (const chunk of req) {
            body += chunk;
            if (body.length > maxBytes) throw new Error("Request too large");
        }
        return body ? JSON.parse(body) : {};
    }

    function sendJson(res, value, status = 200) {
        res.writeHead(status, {
            "Content-Type": "application/json; charset=utf-8",
            "Cache-Control": "no-store",
            "X-Content-Type-Options": "nosniff",
        });
        res.end(JSON.stringify(value));
    }

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
                }
                if (body.assign && typeof body.assign.id === "string") {
                    await migrateNextBuild(priority);
                    const { id, slot } = body.assign;
                    const planner = await loadPlanner(projectRoot);
                    const target = planner.slots.find((value) => value.key === slot);
                    if (slot && !target) {
                        sendJson(res, { error: "That release is no longer open" }, 400);
                        return;
                    }
                    if (target) priority.slots[id] = target.key;
                    else delete priority.slots[id];
                }
                if (typeof body.remove === "string") {
                    const id = body.remove;
                    priority.removed = [...new Set([...(priority.removed ?? []), id])];
                    priority.nextBuild = priority.nextBuild.filter((value) => value !== id);
                    delete priority.slots[id];
                    priority.order = priority.order.filter((value) => value !== id);
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
                        payload: { kind: "planned-improvement", id: feature.id, path: feature.path, title: feature.title, tasks, release: feature.slotLabel ?? "unscheduled" },
                    }],
                });
                sendJson(res, { ok: true });
                return;
            }
            if (req.method === "GET" && url.pathname === "/api/backlog") {
                const entries = await loadBacklogInbox(projectRoot);
                sendJson(res, { entries: entries.map(toBacklogClientEntry) });
                return;
            }
            if (req.method === "POST" && url.pathname === "/api/backlog") {
                let body;
                try {
                    body = await readBody(req, BACKLOG_BODY_MAX_BYTES);
                } catch {
                    sendJson(res, { error: "Request too large" }, 413);
                    return;
                }
                const result = await addBacklogEntry(projectRoot, {
                    title: body.title,
                    description: body.description,
                    image: body.image,
                });
                if (result.error) {
                    sendJson(res, { error: result.error }, 400);
                    return;
                }
                sendJson(res, { entry: toBacklogClientEntry(result.value) }, 201);
                return;
            }
            if (req.method === "GET" && url.pathname.startsWith("/api/backlog-image/")) {
                const id = decodeURIComponent(url.pathname.slice("/api/backlog-image/".length));
                const entries = await loadBacklogInbox(projectRoot);
                const entry = entries.find((value) => value.id === id);
                if (!entry || !entry.imageFile) {
                    sendJson(res, { error: "Not found" }, 404);
                    return;
                }
                let bytes;
                try {
                    bytes = await readFile(join(backlogAttachmentsDir(projectRoot), entry.imageFile));
                } catch {
                    sendJson(res, { error: "Not found" }, 404);
                    return;
                }
                const mime = [...BACKLOG_IMAGE_TYPES.keys()].find((key) => key === entry.imageMime) ?? "application/octet-stream";
                res.writeHead(200, {
                    "Content-Type": mime,
                    "Cache-Control": "no-store",
                    "X-Content-Type-Options": "nosniff",
                });
                res.end(bytes);
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
