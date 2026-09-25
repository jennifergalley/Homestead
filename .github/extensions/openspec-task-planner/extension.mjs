import { createServer } from "node:http";
import { fileURLToPath } from "node:url";
import { createCanvas, joinSession } from "@github/copilot-sdk/extension";
import { loadPlanner } from "./planner-data.mjs";
import { renderPlannerHtml } from "./planner-html.mjs";

const projectRoot = fileURLToPath(new URL("../../../", import.meta.url));
const servers = new Map();

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

await joinSession({
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
