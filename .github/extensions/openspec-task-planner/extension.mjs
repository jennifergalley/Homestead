import { fileURLToPath } from "node:url";
import { createCanvas, joinSession } from "@github/copilot-sdk/extension";
import { loadPlanner } from "./planner-data.mjs";
import { startPlannerServer } from "./planner-server.mjs";

const projectRoot = fileURLToPath(new URL("../../../", import.meta.url));
const plannerOptions = { accountUsage: true };
const servers = new Map();
let session;

session = await joinSession({
    canvases: [
        createCanvas({
            id: "openspec-task-planner",
            displayName: "Homestead task planner",
            description: "Interactive progress board for every Homestead OpenSpec feature and task.",
            actions: [{
                name: "refresh",
                description: "Rescan OpenSpec tasks and return progress and recorded build costs.",
                handler: async () => {
                    const planner = await loadPlanner(projectRoot, Date.now(), plannerOptions);
                    return { featureCount: planner.features.length, ...planner.summary, accounting: planner.accounting, costView: planner.costView };
                },
            }],
            open: async (ctx) => {
                let entry = servers.get(ctx.instanceId);
                if (!entry) {
                    entry = await startPlannerServer(projectRoot, ctx.instanceId, session, plannerOptions);
                    servers.set(ctx.instanceId, entry);
                }
                const planner = await loadPlanner(projectRoot, Date.now(), plannerOptions);
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
