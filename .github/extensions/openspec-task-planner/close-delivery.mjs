// Usage: node .github/extensions/openspec-task-planner/close-delivery.mjs --build-id <id> --shipped <card ids...>
//          [--package-sha <sha>] [--promoted-at "<local time>"] [--root <dir>] [--dry-run]
// The planner half of Scripts\Close-Delivery.ps1 (README "Playtest builds" step 4): in one run it
//   1. removes the shipped cards from priority.json `order`/`slots` and adds them to `removed`,
//   2. moves their inbox entries to backlog-archive.json with the delivery receipt, then refreshes the
//      backlog.md mirror exactly as the planner server does (syncBacklogMarkdown),
//   3. promotes `after-next` slots to `next` (promoteAfterDelivery, as promote-build.mjs does), and
//   4. inserts a delivered-card skeleton in builds.md in the format the planner parses.
// The planner's own parsers stay the source of truth: the card is parsed back before anything is written, and the
// finished tree is loaded with loadPlanner to prove the build shows on the Measured build cost tab.
import { readFile, readdir } from "node:fs/promises";
import { join } from "node:path";
import { fileURLToPath } from "node:url";
import { promoteAfterDelivery, slotKeyForHeading } from "./build-slots.mjs";
import { shipmentTiming } from "./build-time.mjs";
import {
    atomicBacklogWrite, backlogInboxPath, backlogMdPath, deliveredBuildKeys, loadBacklogInbox, loadBuilds, loadPlanner,
    loadPriority, parseBuilds, priorityPath, renderBacklogInboxBlock, syncBacklogMarkdown, upsertMarkedBlock, withBacklogLock,
} from "./planner-data.mjs";

const defaultRoot = fileURLToPath(new URL("../../../", import.meta.url));
const backlogPrefix = "backlog:";
const ARCHIVE_STATUS = "shipped-pending-player-acceptance";
const ACCEPTANCE = "Jenny's player acceptance remains pending; archival removes shipped work from the active backlog and does not claim acceptance.";
// None of these placeholder lines may contain a colon after their leading words: the planner reads "Word words:" in a
// Ships list as a field label and stops collecting features.
const PLACEHOLDER_SHIPS = [
    "Replace this line with the first player-facing change Jenny will notice",
    "Replace this line with the next player-facing change",
    "Delete any unused placeholder lines, then add the admitted work and verification notes",
];

const pad = (value) => String(value).padStart(2, "0");
const handoffPath = (root, ...parts) => join(root, "docs", "handoff", ...parts);

// "YYYY-MM-DD — h:mm AM/PM", the heading every shipped card uses.
export function formatBuildHeading(date) {
    const hour = date.getHours();
    return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())} — ${hour % 12 || 12}:${pad(date.getMinutes())} ${hour < 12 ? "AM" : "PM"}`;
}

function localDate(year, month, day, hour, minute) {
    const date = new Date(year, month - 1, day, hour, minute);
    if (date.getFullYear() !== year || date.getMonth() !== month - 1 || date.getDate() !== day
        || date.getHours() !== hour || date.getMinutes() !== minute) throw new Error("PromotedAt is not a real date/time.");
    return date;
}

function hourOf(text, meridiem) {
    const hour = Number(text);
    if (!meridiem) return hour;
    if (hour < 1 || hour > 12) throw new Error("PromotedAt 12-hour times run 1 to 12.");
    return (hour % 12) + (/^pm$/i.test(meridiem) ? 12 : 0);
}

// Local machine time, as the planner's cards use it. Accepts "2026-10-12 1:58 AM", "2026-10-12 13:58",
// "2026-10-12T13:58", "1:58 AM" (today) or a full ISO timestamp with Z/offset.
export function parsePromotedAt(text, now = new Date()) {
    const value = (text ?? "").trim();
    if (!value) return now;
    if (/^\d{4}-\d{2}-\d{2}T.*(?:Z|[+-]\d{2}:?\d{2})$/i.test(value)) {
        const date = new Date(value);
        if (!Number.isFinite(date.getTime())) throw new Error("PromotedAt is not a valid timestamp.");
        return date;
    }
    let match = /^(\d{4})-(\d{2})-(\d{2})[ T](\d{1,2}):(\d{2})(?::\d{2})?\s*(AM|PM)?$/i.exec(value);
    if (match) return localDate(+match[1], +match[2], +match[3], hourOf(match[4], match[6]), +match[5]);
    match = /^(\d{1,2}):(\d{2})\s*(AM|PM)$/i.exec(value);
    if (match) return localDate(now.getFullYear(), now.getMonth() + 1, now.getDate(), hourOf(match[1], match[3]), +match[2]);
    throw new Error(`PromotedAt "${value}" is not a local time like "2026-10-12 1:58 AM".`);
}

// 41025373140000 nano-AIU -> "41025.37314" (exact, no floating point).
export function nanoToAiuText(nano) {
    const value = BigInt(nano);
    const whole = value / 1_000_000_000n;
    const fraction = String(value % 1_000_000_000n).padStart(9, "0").replace(/0+$/, "");
    return fraction ? `${whole}.${fraction}` : String(whole);
}

export function buildCardLines({ heading, buildId, sha, configuration = "Development" }) {
    return [
        `## ${heading}`,
        "",
        `- Build ID: \`${buildId}\``,
        `- Package source SHA: ${sha ? `\`${sha}\`` : "pending"}`,
        "- Status: delivered",
        `- Configuration: ${configuration}`,
        "- Ships:",
        ...PLACEHOLDER_SHIPS.map((line) => `- ${line}`),
    ];
}

// Newest first: the card goes above the first shipped card that is older than it, else above the first planned
// ("Next build" / "Build after next") or Later section, else at the end. The file's line endings are kept.
export function insertBuildCard(markdown, card) {
    const newline = markdown.includes("\r\n") ? "\r\n" : "\n";
    const lines = markdown.split(/\r?\n/);
    const [date, slot] = card.heading.split(/\s+—\s+/, 2);
    const newTime = shipmentTiming({ date, slot }).time;
    let at = -1;
    let fallback = -1;
    lines.forEach((line, index) => {
        const heading = line.match(/^##\s+(.+?)\s*$/);
        if (!heading) return;
        const text = heading[1].trim();
        if (text.toLowerCase() === "later" || slotKeyForHeading(text)) {
            if (fallback < 0) fallback = index;
            return;
        }
        const [existingDate, existingSlot] = text.split(/\s+—\s+/, 2);
        const time = shipmentTiming({ date: existingDate?.trim(), slot: existingSlot?.trim() }).time;
        if (at < 0 && Number.isFinite(time) && time < newTime) at = index;
    });
    const block = [...buildCardLines(card), ""];
    if (at < 0) at = fallback;
    if (at < 0) {
        while (lines.length && lines[lines.length - 1] === "") lines.pop();
        return [...lines, "", ...block].join(newline);
    }
    lines.splice(at, 0, ...block);
    return lines.join(newline);
}

async function readJson(path, fallback) {
    try {
        return JSON.parse((await readFile(path, "utf8")).replace(/^\uFEFF/, ""));
    } catch (error) {
        if (error?.code === "ENOENT") return fallback;
        throw error;
    }
}

async function openSpecChangeNames(root) {
    try {
        const entries = await readdir(join(root, "openspec", "changes"), { withFileTypes: true });
        return entries.filter((entry) => entry.isDirectory() && entry.name !== "archive" && !entry.name.startsWith(".")).map((entry) => entry.name);
    } catch (error) {
        if (error?.code === "ENOENT") return [];
        throw error;
    }
}

const unique = (values) => [...new Set(values)];

// Union of two arrays of inbox records by id, later wins.
function mergeRecords(previous = [], added = []) {
    const byId = new Map();
    for (const record of [...previous, ...added]) if (record && typeof record === "object" && record.id) byId.set(record.id, record);
    return [...byId.values()];
}

function receiptFor(report, buildId, sha) {
    return {
        buildRecord: "builds.md",
        changelist: "builds.md",
        ...(report ? {
            observedUsageReport: `accounting/reports/${buildId}.json`,
            recordedCalls: report.totals?.recordedCalls ?? report.totals?.calls ?? null,
            recordedAiu: report.totals?.recordedNanoAiu != null ? nanoToAiuText(report.totals.recordedNanoAiu) : null,
        } : {}),
        ...(sha ? { sourceCommit: sha } : {}),
    };
}

export async function closeDelivery(projectRoot, options) {
    const { buildId, shippedCards, packageSha = "", promotedAt = "", configuration = "Development", dryRun = false, now = new Date() } = options;
    if (typeof buildId !== "string" || !/^[\w-]+$/.test(buildId)) throw new Error("Build ID must be letters, digits, dashes and underscores.");
    if (!Array.isArray(shippedCards) || !shippedCards.length || shippedCards.some((id) => typeof id !== "string" || !id.trim())) {
        throw new Error("Give at least one shipped card ID.");
    }
    if (packageSha && !/^[0-9a-f]{7,64}$/i.test(packageSha)) throw new Error("Package SHA must be 7 to 64 hex digits.");
    const shipped = unique(shippedCards.map((id) => id.trim()));
    const shippedSet = new Set(shipped);
    const notes = [];

    const builds = await loadBuilds(projectRoot);
    const heading = formatBuildHeading(parsePromotedAt(promotedAt, now));
    const [headingDate, headingSlot] = heading.split(/\s+—\s+/, 2);
    const existingCard = builds.entries.find((entry) => entry.buildId === buildId);
    const sameHeading = builds.entries.find((entry) => entry.heading === heading && entry.buildId !== buildId);
    if (sameHeading) {
        throw new Error(`builds.md already has a card headed "${heading}" for ${sameHeading.buildId ?? "another build"}; pass a different -PromotedAt.`);
    }

    const priorityExists = await readFile(priorityPath(projectRoot), "utf8").then(() => true, (error) => {
        if (error?.code === "ENOENT") return false;
        throw error;
    });
    const priority = await loadPriority(projectRoot, builds, { strict: true });
    const entries = await loadBacklogInbox(projectRoot);
    const inboxDocument = await readJson(backlogInboxPath(projectRoot), { entries: [] });
    const archivePath = handoffPath(projectRoot, "backlog-archive.json");
    const archive = await readJson(archivePath, { schemaVersion: 1, archives: [] });
    if (!archive || !Array.isArray(archive.archives)) throw new Error("backlog-archive.json must hold an archives array.");
    const prior = archive.archives.find((entry) => entry.buildId === buildId);
    const alreadyArchived = new Set([...(prior?.inboxEntries ?? []).map((id) => `${backlogPrefix}${id}`), ...(prior?.plannerOnlyItems ?? [])]);

    // A typo must not silently archive nothing: every card has to be on the board, in the inbox, an OpenSpec
    // change, or already closed out under this build.
    const inboxById = new Map(entries.map((entry) => [`${backlogPrefix}${entry.id}`, entry]));
    const known = new Set([...priority.order, ...priority.removed, ...Object.keys(priority.slots), ...inboxById.keys(),
        ...await openSpecChangeNames(projectRoot), ...alreadyArchived]);
    const unknown = shipped.filter((id) => !known.has(id));
    if (unknown.length) throw new Error(`Unknown card ID(s): ${unknown.join(", ")}. Use the IDs the planner shows (backlog:<inbox id> or an OpenSpec change name).`);

    const archivedEntries = shipped.filter((id) => inboxById.has(id)).map((id) => inboxById.get(id));
    const plannerOnly = shipped.filter((id) => !id.startsWith(backlogPrefix));
    const alreadyClosed = shipped.filter((id) => alreadyArchived.has(id) || (priority.removed.includes(id) && !inboxById.has(id)));
    if (alreadyClosed.length) notes.push(`Already closed out earlier, left as they are: ${alreadyClosed.join(", ")}.`);
    const keptEntries = entries.filter((entry) => !shippedSet.has(`${backlogPrefix}${entry.id}`));

    // (1)+(3): the planner's own promotion, then retire the shipped cards.
    const promoted = promoteAfterDelivery(priority, shipped, deliveredBuildKeys(builds));
    const nextPriority = {
        ...promoted,
        order: promoted.order.filter((id) => !shippedSet.has(id)),
        removed: unique([...promoted.removed, ...shipped]),
    };
    const nextIds = Object.keys(nextPriority.slots).filter((id) => nextPriority.slots[id] === "next");
    const movedUp = Object.keys(priority.slots).filter((id) => priority.slots[id] === "after-next" && !shippedSet.has(id));

    // (2): the archive entry, keeping the receipt and the full inbox records.
    const report = await readJson(handoffPath(projectRoot, "accounting", "reports", `${buildId}.json`), null);
    if (!report) notes.push(`No accounting/reports/${buildId}.json yet; the receipt has no usage numbers and the Measured build cost tab shows no cost.`);
    const receipt = receiptFor(report, buildId, packageSha);
    const entry = {
        buildId,
        archivedAt: now.toISOString(),
        status: ARCHIVE_STATUS,
        receipt,
        inboxEntries: archivedEntries.map((value) => value.id),
        inboxEntryRecords: archivedEntries,
        plannerOnlyItems: plannerOnly,
        markdownOnlyEntries: [],
        acceptance: ACCEPTANCE,
    };
    const archiveEntry = prior ? {
        ...prior,
        receipt: { ...prior.receipt, ...receipt },
        inboxEntries: unique([...(prior.inboxEntries ?? []), ...entry.inboxEntries]),
        inboxEntryRecords: mergeRecords(prior.inboxEntryRecords, entry.inboxEntryRecords),
        plannerOnlyItems: unique([...(prior.plannerOnlyItems ?? []), ...plannerOnly]),
    } : entry;
    const nextArchive = { ...archive, archives: prior
        ? archive.archives.map((value) => value === prior ? archiveEntry : value) : [...archive.archives, archiveEntry] };

    // (4): the build card, parsed back with the planner's own parser before anything is written.
    let cardAction = "exists";
    let nextBuilds = null;
    const buildsPath = handoffPath(projectRoot, "builds.md");
    if (existingCard) {
        notes.push(`builds.md already has a card for ${buildId} ("${existingCard.heading}"); not adding another.`);
    } else {
        const markdown = await readFile(buildsPath, "utf8").catch((error) => {
            if (error?.code === "ENOENT") return "# Build changelist\n";
            throw error;
        });
        nextBuilds = insertBuildCard(markdown, { heading, buildId, sha: packageSha, configuration });
        const parsed = parseBuilds(nextBuilds).entries.filter((value) => value.buildId === buildId);
        const card = parsed[0];
        const timing = shipmentTiming({ date: headingDate, slot: headingSlot });
        if (parsed.length !== 1 || card.status !== "delivered" || card.heading !== heading || timing.dataError
            || card.shippedFeatures.length !== PLACEHOLDER_SHIPS.length) {
            throw new Error("The generated builds.md card did not parse as a delivered build; nothing was written.");
        }
        cardAction = "inserted";
    }

    // (2) cont.: the markdown mirror, refreshed the way the planner server refreshes it after an inbox save.
    const markdownBefore = await readFile(backlogMdPath(projectRoot), "utf8").catch((error) => {
        if (error?.code === "ENOENT") return "";
        throw error;
    });
    const backlogMdChanged = upsertMarkedBlock(markdownBefore, renderBacklogInboxBlock(keptEntries)) !== markdownBefore;

    const summary = {
        dryRun, buildId, heading, cardAction, packageSha: packageSha || null,
        shipped, inboxArchived: archivedEntries.map((value) => `${backlogPrefix}${value.id}`), plannerOnly,
        inboxRemaining: keptEntries.length, backlogMdChanged,
        nextBuildSlots: nextIds.length, movedUpFromAfterNext: movedUp.length,
        archive: prior ? "updated existing archive entry" : "added archive entry", receipt, notes,
        priorityFile: priorityExists ? "rewritten" : "not present; created",
    };
    if (dryRun) return summary;

    // Archive first so the receipt exists before the active surfaces drop the cards; every step is safe to re-run.
    await atomicBacklogWrite(archivePath, JSON.stringify(nextArchive, null, 2) + "\n");
    if (archivedEntries.length) {
        const locked = await withBacklogLock(projectRoot, async () => {
            await atomicBacklogWrite(backlogInboxPath(projectRoot), JSON.stringify({ ...inboxDocument, entries: keptEntries }, null, 2) + "\n");
            await syncBacklogMarkdown(projectRoot, keptEntries);
        });
        if (locked?.error) throw new Error(locked.error);
    } else if (backlogMdChanged) {
        await syncBacklogMarkdown(projectRoot, keptEntries);
    }
    await atomicBacklogWrite(priorityPath(projectRoot), JSON.stringify({ ...nextPriority, updated: new Date().toISOString() }, null, 2) + "\n");
    if (nextBuilds !== null) await atomicBacklogWrite(buildsPath, nextBuilds);
    summary.planner = await verifyPlanner(projectRoot, buildId, shipped, Boolean(report));
    return summary;
}

// Loads the finished tree with the planner's own loader: the build must show on the Measured build cost tab and the
// shipped cards must be gone from the board.
export async function verifyPlanner(projectRoot, buildId, shipped, expectReport = true) {
    const problems = [];
    let planner;
    try {
        planner = await loadPlanner(projectRoot, Date.now());
    } catch (error) {
        return { ok: false, problems: [`The planner could not load the result: ${error.message}`] };
    }
    const card = planner.builds.entries.find((value) => value.buildId === buildId);
    if (!card) problems.push(`The planner shows no build card for ${buildId}.`);
    else if (card.status !== "delivered") problems.push(`The ${buildId} card is "${card.status}", not delivered.`);
    else if (card.dataError) problems.push(card.dataError);
    const cost = planner.costView.shipped.find((value) => value.buildId === buildId);
    if (!cost) problems.push(`The Measured build cost tab does not list ${buildId}.`);
    else {
        if (cost.dataError) problems.push(cost.dataError);
        if (expectReport && !cost.reportPath) problems.push(`The cost tab lists ${buildId} without its usage report.`);
    }
    const leftover = planner.features.filter((value) => shipped.includes(value.id)).map((value) => value.id);
    if (leftover.length) problems.push(`Still on the active board: ${leftover.join(", ")}.`);
    return { ok: problems.length === 0, problems, costLabel: cost?.label ?? null, recordedNanoAiu: cost?.recordedNanoAiu ?? null, reportPath: cost?.reportPath ?? null };
}

function parseArguments(argv) {
    const options = { shippedCards: [], root: defaultRoot, dryRun: false };
    for (let index = 0; index < argv.length; index++) {
        const key = argv[index];
        const take = () => {
            const value = argv[++index];
            if (value === undefined) throw new Error(`${key} needs a value.`);
            return value;
        };
        if (key === "--build-id") options.buildId = take();
        else if (key === "--package-sha") options.packageSha = take();
        else if (key === "--promoted-at") options.promotedAt = take();
        else if (key === "--configuration") options.configuration = take();
        else if (key === "--root") options.root = take();
        else if (key === "--dry-run") options.dryRun = true;
        else if (key === "--shipped") {
            while (index + 1 < argv.length && !argv[index + 1].startsWith("--")) options.shippedCards.push(argv[++index]);
        } else throw new Error(`Unknown option ${key}.`);
    }
    return options;
}

if (process.argv[1] && fileURLToPath(import.meta.url) === process.argv[1]) {
    try {
        const { root, ...options } = parseArguments(process.argv.slice(2));
        const result = await closeDelivery(root, options);
        const tag = result.dryRun ? "[dry run, nothing written] " : "";
        console.log(`${tag}Build ${result.buildId} (${result.heading})`);
        console.log(`  builds.md card: ${result.cardAction}${result.cardAction === "inserted" ? " (fill in the Ships bullets)" : ""}`);
        console.log(`  priority.json: ${result.shipped.length} shipped card(s) -> removed; ${result.movedUpFromAfterNext} moved up from Build after next; Next build now holds ${result.nextBuildSlots}`);
        console.log(`  inbox: ${result.inboxArchived.length} entr${result.inboxArchived.length === 1 ? "y" : "ies"} -> backlog-archive.json (${result.archive}); ${result.inboxRemaining} remain; backlog.md ${result.backlogMdChanged ? "refreshed" : "unchanged"}`);
        if (result.plannerOnly.length) console.log(`  planner-only cards archived: ${result.plannerOnly.join(", ")}`);
        for (const note of result.notes) console.log(`  note: ${note}`);
        if (result.planner) {
            if (result.planner.ok) console.log(`  planner check: ok; cost tab lists ${result.buildId} (${result.planner.costLabel}${result.planner.reportPath ? ", with usage report" : ""})`);
            else {
                for (const problem of result.planner.problems) console.error(`  planner check FAILED: ${problem}`);
                process.exitCode = 2;
            }
        }
    } catch (error) {
        console.error(`close-delivery: ${error.message}`);
        console.error("Usage: node close-delivery.mjs --build-id <id> --shipped <card ids...> [--package-sha <sha>] [--promoted-at \"<local time>\"] [--root <dir>] [--dry-run]");
        process.exitCode = 1;
    }
}
