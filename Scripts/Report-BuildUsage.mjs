// Usage: node Scripts\Report-BuildUsage.mjs --allocation <json> --input <snapshot> [--input <snapshot>] --out <report>
import { readFile } from "node:fs/promises";
import { buildUsageReport, persistUsageReport } from "../.github/extensions/openspec-task-planner/accounting-data.mjs";

const options = { inputs: [] };
for (let i = 2; i < process.argv.length; i += 2) {
    const key = process.argv[i], value = process.argv[i + 1];
    if (!value || !["--allocation", "--input", "--out"].includes(key)) throw new Error("Expected --allocation, --input and --out paths");
    if (key === "--input") options.inputs.push(value);
    else options[key.slice(2)] = value;
}
if (!options.allocation || !options.inputs.length || !options.out) throw new Error("Allocation, input and output are required");
const parse = async (path) => JSON.parse((await readFile(path, "utf8")).replace(/^\uFEFF/, ""));
const [allocation, ...snapshots] = await Promise.all([options.allocation, ...options.inputs].map(parse));
const report = buildUsageReport(allocation, snapshots);
await persistUsageReport(options.out, report);
process.stdout.write(`${report.buildId}: ${report.totals.calls} calls, ${report.totals.recordedNanoAiu} recorded nano-AIU (${report.status})\n`);
