import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { join } from "node:path";
import { isBuildSlotKey } from "./build-slots.mjs";
import { loadPlanner, loadBuilds, loadPriority, atomicBacklogWrite, priorityPath as priorityFile, loadBacklogInbox, addBacklogEntry, updateBacklogEntry, toBacklogClientEntry, backlogAttachmentsDir, BACKLOG_IMAGE_TYPES } from "./planner-data.mjs";
import { renderPlannerHtml } from "./planner-html.mjs";

export async function startPlannerServer(projectRoot, instanceId, session, options = {}) {
    const priorityPath = priorityFile(projectRoot);
    // Jenny's backlog-entry form, and reordering/scheduling in this canvas, are
    // deliberately agent-free: they write straight to disk (priority.json,
    // backlog-inbox.json, backlog.md) and the orchestrator picks up the change
    // next time it reads those files. Nothing here calls session.send().
    const BACKLOG_BODY_MAX_BYTES = 11 * 1024 * 1024; // ~8 MB image, base64-inflated, plus JSON overhead

    async function readPriority() {
        return loadPriority(projectRoot, await loadBuilds(projectRoot));
    }

    async function writePriority(priority) {
        await atomicBacklogWrite(priorityPath, JSON.stringify({ ...priority, updated: new Date().toISOString() }, null, 2) + "\n");
    }

    async function readBody(req, maxBytes = 200000) {
        const chunks = [];
        let bytes = 0;
        for await (const chunk of req) {
            bytes += chunk.length;
            if (bytes > maxBytes) throw new Error("Request too large");
            chunks.push(chunk);
        }
        const body = Buffer.concat(chunks).toString("utf8");
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
                    "Content-Security-Policy": "default-src 'self'; style-src 'self' 'unsafe-inline'; script-src 'self' 'unsafe-inline'; connect-src 'self'; img-src 'self' data: blob:",
                    "X-Content-Type-Options": "nosniff",
                });
                res.end(renderPlannerHtml());
                return;
            }
            if ((req.method === "GET" && url.pathname === "/api/tasks")
                || (req.method === "POST" && url.pathname === "/refresh")) {
                sendJson(res, await loadPlanner(projectRoot, Date.now(), options));
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
                    const { id, slot } = body.assign;
                    if (slot && !isBuildSlotKey(slot)) {
                        sendJson(res, { error: "Choose Next build or Build after next" }, 400);
                        return;
                    }
                    if (slot) priority.slots[id] = slot;
                    else delete priority.slots[id];
                }
                if (typeof body.remove === "string") {
                    const id = body.remove;
                    priority.removed = [...new Set([...(priority.removed), id])];
                    delete priority.slots[id];
                    priority.order = priority.order.filter((value) => value !== id);
                }
                await writePriority(priority);
                sendJson(res, priority);
                return;
            }
            if (req.method === "POST" && url.pathname === "/api/quote") {
                const body = await readBody(req);
                const planner = await loadPlanner(projectRoot, Date.now(), options);
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
            const editingBacklog = req.method === "PATCH" && url.pathname.startsWith("/api/backlog/");
            if ((req.method === "POST" && url.pathname === "/api/backlog") || editingBacklog) {
                let body;
                try {
                    body = await readBody(req, BACKLOG_BODY_MAX_BYTES);
                } catch (error) {
                    sendJson(res, { error: error instanceof SyntaxError ? "Invalid feedback JSON." : error.message },
                        error instanceof SyntaxError ? 400 : 413);
                    return;
                }
                if (!body || typeof body !== "object" || Array.isArray(body)) {
                    sendJson(res, { error: "Invalid feedback request." }, 400);
                    return;
                }
                const content = { title: body.title, description: body.description, image: body.image, revision: body.revision };
                const result = editingBacklog
                    ? await updateBacklogEntry(projectRoot, decodeURIComponent(url.pathname.slice("/api/backlog/".length)), content)
                    : await addBacklogEntry(projectRoot, content);
                if (result.error) {
                    sendJson(res, { error: result.error }, result.status ?? 400);
                    return;
                }
                sendJson(res, { entry: toBacklogClientEntry(result.value), warning: result.warning }, editingBacklog ? 200 : 201);
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
