import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile, readFile, readdir } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import test from "node:test";
import {
    upsertMarkedBlock,
    renderBacklogInboxBlock,
    sanitizeBacklogTitle,
    sanitizeBacklogDescription,
    decodeBacklogImage,
    addBacklogEntry,
    updateBacklogEntry,
    backlogEntryRevision,
    backlogInboxPath,
    loadBacklogInbox,
    loadPlanner,
    toBacklogClientEntry,
    backlogMdPath,
    backlogAttachmentsDir,
    BACKLOG_MAX_IMAGE_BYTES,
} from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";

// A minimal, well-known 1x1 transparent PNG.
const TINY_PNG_BASE64 = "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=";
const TINY_PNG_DATA_URL = `data:image/png;base64,${TINY_PNG_BASE64}`;

async function fixture(t) {
    const root = await mkdtemp(join(tmpdir(), "homestead-planner-backlog-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    await mkdir(join(root, "docs", "handoff"), { recursive: true });
    await writeFile(backlogMdPath(root),
        "# Jenny's backlog\n\nIntro text.\n\n## Next two builds\n\n1. Existing item.\n\n## Later\n\n- Later item.\n");
    return root;
}

test("sanitizeBacklogTitle rejects empty and over-long titles, trims whitespace", () => {
    assert.equal(sanitizeBacklogTitle("  ").error, "Title is required.");
    assert.equal(sanitizeBacklogTitle(undefined).error, "Title is required.");
    assert.deepEqual(sanitizeBacklogTitle("  Sell crops  "), { value: "Sell crops" });
    assert.match(sanitizeBacklogTitle("x".repeat(201)).error, /too long/);
});

test("sanitizeBacklogDescription allows empty, trims, rejects over-long", () => {
    assert.deepEqual(sanitizeBacklogDescription(undefined), { value: "" });
    assert.deepEqual(sanitizeBacklogDescription("  hello  "), { value: "hello" });
    assert.match(sanitizeBacklogDescription("x".repeat(4001)).error, /too long/);
});

test("decodeBacklogImage accepts a valid PNG data URL and rejects bad input", () => {
    const ok = decodeBacklogImage({ dataUrl: TINY_PNG_DATA_URL });
    assert.ok(ok.value);
    assert.equal(ok.value.mime, "image/png");
    assert.equal(ok.value.extension, "png");
    assert.ok(Buffer.isBuffer(ok.value.buffer));

    assert.deepEqual(decodeBacklogImage(null), { value: null });
    assert.deepEqual(decodeBacklogImage(undefined), { value: null });

    assert.match(decodeBacklogImage({ dataUrl: "not-a-data-url" }).error, /PNG, JPEG, WebP, or GIF/);
    assert.match(decodeBacklogImage({ dataUrl: "data:text/plain;base64,aGk=" }).error, /PNG, JPEG, WebP, or GIF/);
    assert.match(decodeBacklogImage({}).error, /invalid/);

    const oversized = "A".repeat(Math.ceil((BACKLOG_MAX_IMAGE_BYTES + 1024) / 3) * 4);
    assert.match(decodeBacklogImage({ dataUrl: `data:image/png;base64,${oversized}` }).error, /too large/);
});

test("upsertMarkedBlock inserts under the existing ## Later heading, replaces idempotently, and removes cleanly", () => {
    const markdown = "# Title\n\nIntro.\n\n## Next two builds\n\n1. Item.\n\n## Later\n\n- Older later item.\n";
    const block = renderBacklogInboxBlock([
        { id: "a", title: "First", description: "desc", imageFile: null, createdUtc: "2026-10-01T00:00:00.000Z" },
    ]);

    const inserted = upsertMarkedBlock(markdown, block);
    assert.ok(inserted.includes("<!-- jenny-inbox:start -->"));
    assert.ok(inserted.includes("First"));
    // Lands right under "## Later", ahead of the pre-existing bullets, and
    // after "## Next two builds" (not before the first heading in the file).
    assert.ok(inserted.indexOf("## Next two builds") < inserted.indexOf("<!-- jenny-inbox:start -->"));
    assert.ok(inserted.indexOf("## Later") < inserted.indexOf("<!-- jenny-inbox:start -->"));
    assert.ok(inserted.indexOf("<!-- jenny-inbox:start -->") < inserted.indexOf("Older later item."));

    const block2 = renderBacklogInboxBlock([
        { id: "a", title: "First", description: "desc", imageFile: null, createdUtc: "2026-10-01T00:00:00.000Z" },
        { id: "b", title: "Second", description: "", imageFile: "b.png", createdUtc: "2026-10-01T01:00:00.000Z" },
    ]);
    const replaced = upsertMarkedBlock(inserted, block2);
    assert.ok(replaced.includes("Second"));
    assert.equal((replaced.match(/jenny-inbox:start/g) ?? []).length, 1);

    const removed = upsertMarkedBlock(replaced, "");
    assert.ok(!removed.includes("jenny-inbox"));
    assert.ok(!removed.includes("Second"));
    assert.ok(removed.includes("## Later"));
    assert.ok(removed.includes("Older later item."));
});

test("upsertMarkedBlock creates a ## Later section when the file doesn't have one", () => {
    const markdown = "# Title\n\nIntro.\n\n## Next two builds\n\n1. Item.\n";
    const block = renderBacklogInboxBlock([
        { id: "a", title: "First", description: "", imageFile: null, createdUtc: "2026-10-01T00:00:00.000Z" },
    ]);
    const inserted = upsertMarkedBlock(markdown, block);
    assert.ok(inserted.includes("## Later"));
    assert.ok(inserted.indexOf("## Later") < inserted.indexOf("<!-- jenny-inbox:start -->"));
    assert.ok(inserted.includes("First"));
});

test("addBacklogEntry persists inbox JSON, attachment bytes, and syncs backlog.md under ## Later", async (t) => {
    const root = await fixture(t);
    const result = await addBacklogEntry(root, {
        title: "  Sell crops at the General Store  ",
        description: "Tried the town loop and the till didn't accept turnips.",
        image: { dataUrl: TINY_PNG_DATA_URL },
    }, Date.parse("2026-10-01T22:00:00Z"));

    assert.ok(result.value);
    const entry = result.value;
    assert.equal(entry.title, "Sell crops at the General Store");
    assert.ok(entry.imageFile.endsWith(".png"));

    const inbox = await loadBacklogInbox(root);
    assert.equal(inbox.length, 1);
    assert.equal(inbox[0].id, entry.id);

    const imageBytes = await readFile(join(backlogAttachmentsDir(root), entry.imageFile));
    assert.equal(imageBytes.length, Buffer.from(TINY_PNG_BASE64, "base64").length);

    const markdown = await readFile(backlogMdPath(root), "utf8");
    assert.ok(markdown.includes("Sell crops at the General Store"));
    assert.ok(!markdown.includes("## New from Jenny"));
    assert.ok(markdown.indexOf("## Later") < markdown.indexOf("Sell crops at the General Store"));
    assert.ok(markdown.indexOf("Sell crops at the General Store") < markdown.indexOf("Later item."));

    const client = toBacklogClientEntry(entry);
    assert.equal(client.imageUrl, `/api/backlog-image/${entry.id}`);
    assert.equal(client.description, entry.description);
});

test("addBacklogEntry rejects an invalid title without writing any files", async (t) => {
    const root = await fixture(t);
    const result = await addBacklogEntry(root, { title: "   " });
    assert.equal(result.error, "Title is required.");
    assert.deepEqual(await loadBacklogInbox(root), []);
});

test("loadPlanner surfaces backlog entries as unscheduled pseudo-features, newest first", async (t) => {
    const root = await fixture(t);
    await mkdir(join(root, "openspec", "changes", "existing-feature"), { recursive: true });
    await writeFile(join(root, "openspec", "changes", "existing-feature", "tasks.md"), "## Work\n- [ ] 1.1 Step\n");

    const older = await addBacklogEntry(root, { title: "Older backlog item" }, Date.parse("2026-10-01T10:00:00Z"));
    const newer = await addBacklogEntry(root, { title: "Newer backlog item" }, Date.parse("2026-10-01T12:00:00Z"));
    assert.ok(older.value && newer.value);

    const planner = await loadPlanner(root, Date.parse("2026-10-01T23:00:00Z"));
    const titles = planner.features.map((feature) => feature.title);
    assert.equal(titles[0], "Newer backlog item");
    assert.equal(titles[1], "Older backlog item");
    const newerFeature = planner.features.find((feature) => feature.title === "Newer backlog item");
    assert.equal(newerFeature.fromBacklog, true);
    assert.equal(newerFeature.status, "proposed");
    assert.equal(newerFeature.slot, null);
    assert.ok(newerFeature.id.startsWith("backlog:"));
});

test("delivery registry marks shipped feedback without closing player acceptance or rewriting priority", async (t) => {
    const root = await fixture(t);
    const entry = (await addBacklogEntry(root, { title: "Delivered feedback" })).value;
    const priorityPath = join(root, "docs", "handoff", "priority.json");
    const priority = {
        order: [`backlog:${entry.id}`],
        slots: { [`backlog:${entry.id}`]: "2026-10-04 07:30" },
    };
    await writeFile(priorityPath, JSON.stringify(priority, null, 2) + "\n");
    const priorityBefore = await readFile(priorityPath, "utf8");
    await writeFile(join(root, "docs", "handoff", "measured-build-02.json"), JSON.stringify({
        plannerDelivery: {
            buildId: "20261003-measured-02",
            status: "shipped",
            playerAcceptance: "pending",
            selectedIds: [`backlog:${entry.id}`],
        },
    }, null, 2) + "\n");

    const planner = await loadPlanner(root, Date.parse("2026-10-04T12:00:00Z"));
    const feature = planner.features.find((value) => value.id === `backlog:${entry.id}`);
    assert.equal(feature.status, "proposed");
    assert.equal(feature.deliveryStatus, "shipped");
    assert.equal(feature.deliveryBuildId, "20261003-measured-02");
    assert.equal(feature.playerAcceptance, "pending");
    assert.equal(feature.slot, null);
    assert.equal(feature.carriedFrom, null);
    assert.equal(feature.nextBuild, false);
    assert.equal(await readFile(priorityPath, "utf8"), priorityBefore);
});

test("editing preserves identity, screenshot, other entries, document metadata and priority bytes", async (t) => {
    const root = await fixture(t);
    const first = (await addBacklogEntry(root, { title: "First", image: { dataUrl: TINY_PNG_DATA_URL } })).value;
    const second = (await addBacklogEntry(root, { title: "Second" })).value;
    const document = { schemaVersion: 1, custom: "preserved", entries: [first, { ...second, custom: { note: "keep" } }] };
    await writeFile(backlogInboxPath(root), JSON.stringify(document));
    const priorityPath = join(root, "docs", "handoff", "priority.json");
    const priority = JSON.stringify({ order: [`backlog:${first.id}`, `backlog:${second.id}`],
        slots: { [`backlog:${first.id}`]: "2026-10-03 21:00" }, removed: [`backlog:${second.id}`] });
    await writeFile(priorityPath, priority);
    const originalImage = await readFile(join(backlogAttachmentsDir(root), first.imageFile));
    const result = await updateBacklogEntry(root, first.id, {
        title: "  Changed <title> & text  ", description: "New description", revision: backlogEntryRevision(first),
    });
    assert.ok(result.value);
    assert.equal(result.value.id, first.id);
    assert.equal(result.value.createdUtc, first.createdUtc);
    assert.equal(result.value.imageFile, first.imageFile);
    assert.equal(result.value.title, "Changed <title> & text");
    const saved = JSON.parse(await readFile(backlogInboxPath(root), "utf8"));
    assert.equal(saved.custom, "preserved");
    assert.deepEqual(saved.entries[1], document.entries[1]);
    assert.deepEqual(await readFile(join(backlogAttachmentsDir(root), first.imageFile)), originalImage);
    assert.equal(await readFile(priorityPath, "utf8"), priority);
    assert.match(await readFile(backlogMdPath(root), "utf8"), /New description/);
    const planner = await loadPlanner(root, new Date(2026, 9, 3, 18).getTime());
    assert.equal(planner.features[0].id, `backlog:${first.id}`);
    assert.equal(planner.features[0].slot, "2026-10-03 21:00");
    assert.equal(planner.features.length, 1);
});

test("replacement, removal and addition preserve original screenshot bytes", async (t) => {
    const root = await fixture(t);
    let entry = (await addBacklogEntry(root, { title: "Picture", image: { dataUrl: TINY_PNG_DATA_URL } })).value;
    const original = entry.imageFile;
    const replacementBytes = Buffer.from("GIF89a-test-image");
    const replace = await updateBacklogEntry(root, entry.id, { title: "Replaced", revision: backlogEntryRevision(entry),
        image: { dataUrl: `data:image/gif;base64,${replacementBytes.toString("base64")}` } });
    entry = replace.value;
    assert.ok(entry.imageFile.endsWith(".gif"));
    assert.notEqual(entry.imageFile, original);
    assert.deepEqual(await readFile(join(backlogAttachmentsDir(root), entry.imageFile)), replacementBytes);
    const replacement = entry.imageFile;
    entry = (await updateBacklogEntry(root, entry.id, {
        title: "Removed", revision: backlogEntryRevision(entry), image: null,
    })).value;
    assert.equal(entry.imageFile, null);
    assert.equal(entry.imageMime, null);
    assert.equal(toBacklogClientEntry(entry).imageUrl, null);
    entry = (await updateBacklogEntry(root, entry.id, { title: "Added", revision: backlogEntryRevision(entry),
        image: { dataUrl: TINY_PNG_DATA_URL } })).value;
    assert.notEqual(entry.imageFile, original);
    assert.deepEqual(await readFile(join(backlogAttachmentsDir(root), original)), Buffer.from(TINY_PNG_BASE64, "base64"));
    assert.deepEqual(await readFile(join(backlogAttachmentsDir(root), replacement)), replacementBytes);
});

test("invalid, missing and stale edits do not write or lose feedback", async (t) => {
    const root = await fixture(t);
    const entry = (await addBacklogEntry(root, { title: "Original" })).value;
    const before = await readFile(backlogInboxPath(root), "utf8");
    const markdown = await readFile(backlogMdPath(root), "utf8");
    const valid = { title: "Draft", revision: backlogEntryRevision(entry) };
    for (const [id, content, pattern, status] of [
        [entry.id, { ...valid, title: " " }, /required/, undefined],
        [entry.id, { ...valid, title: "x".repeat(201) }, /too long/, undefined],
        [entry.id, { ...valid, description: "x".repeat(4001) }, /too long/, undefined],
        [entry.id, { ...valid, description: {} }, /must be text/, undefined],
        [entry.id, { ...valid, image: {} }, /invalid/, undefined],
        [entry.id, { ...valid, image: { dataUrl: "data:image/png;base64,aGk=" } }, /content/, undefined],
        [entry.id, { ...valid, image: { dataUrl: "data:image/png;base64,A" } }, /decoded/, undefined],
        [entry.id, { title: "Draft" }, /Reopen/, undefined],
        ["missing-id", valid, /no longer exists/, 404],
        ["../escape", valid, /Invalid feedback ID/, undefined],
        [entry.id, { ...valid, revision: "a".repeat(64) }, /changed elsewhere/, 409],
    ]) {
        const result = await updateBacklogEntry(root, id, content);
        assert.match(result.error, pattern);
        assert.equal(result.status, status);
        assert.equal(await readFile(backlogInboxPath(root), "utf8"), before);
        assert.equal(await readFile(backlogMdPath(root), "utf8"), markdown);
    }
    assert.deepEqual((await readdir(join(root, "docs", "handoff"))).sort(), ["backlog-inbox.json", "backlog.md"]);
});

test("corrupt inbox schemas and duplicate IDs cannot be replaced by an edit or add", async (t) => {
    const root = await fixture(t);
    for (const corrupt of ["{invalid", JSON.stringify({ entries: {} }), JSON.stringify({ entries: [
        { id: "same", title: "A" }, { id: "same", title: "B" },
    ] })]) {
        await writeFile(backlogInboxPath(root), corrupt);
        await assert.rejects(addBacklogEntry(root, { title: "New" }));
        await assert.rejects(updateBacklogEntry(root, "same", { title: "New", revision: "a".repeat(64) }));
        assert.equal(await readFile(backlogInboxPath(root), "utf8"), corrupt);
    }
});

test("exclusive inbox lock fails promptly and simultaneous writers cannot overwrite each other", async (t) => {
    const root = await fixture(t);
    const entry = (await addBacklogEntry(root, { title: "Original" })).value;
    const lock = `${backlogInboxPath(root)}.lock`;
    await writeFile(lock, "external-writer");
    const busy = await updateBacklogEntry(root, entry.id, { title: "Draft", revision: backlogEntryRevision(entry) });
    assert.equal(busy.status, 409);
    assert.match(busy.error, /being saved elsewhere/);
    assert.equal(await readFile(lock, "utf8"), "external-writer");
    await rm(lock);
    const outcomes = await Promise.all([
        updateBacklogEntry(root, entry.id, { title: "Edited", revision: backlogEntryRevision(entry) }),
        addBacklogEntry(root, { title: "New" }),
    ]);
    assert.equal(outcomes.filter((value) => value.value).length, 1);
    assert.equal(outcomes.filter((value) => value.status === 409).length, 1);
    const inbox = await loadBacklogInbox(root);
    assert.ok(inbox.some((value) => value.id === entry.id));
    assert.equal(inbox.length, outcomes[1].value ? 2 : 1);
});

test("markdown failure reports a warning while preserving the committed inbox edit", async (t) => {
    const root = await fixture(t);
    const entry = (await addBacklogEntry(root, { title: "Original" })).value;
    await rm(backlogMdPath(root));
    await mkdir(backlogMdPath(root));
    const result = await updateBacklogEntry(root, entry.id, { title: "Saved", revision: backlogEntryRevision(entry) });
    assert.equal(result.value.title, "Saved");
    assert.match(result.warning, /Feedback saved, but backlog.md/);
    assert.equal((await loadBacklogInbox(root))[0].title, "Saved");
    assert.ok(!(await readdir(join(root, "docs", "handoff"))).some((name) => name.endsWith(".tmp") || name.endsWith(".lock")));
});

test("isolated HTTP editing and scheduling use no chat calls and return useful error statuses", async (t) => {
    const root = await fixture(t);
    const chatCalls = [];
    const session = new Proxy({}, { get(_, name) { chatCalls.push(name); throw new Error("Unexpected chat access"); } });
    const { server, url } = await startPlannerServer(root, "editing-test", session);
    t.after(() => new Promise((resolve) => server.close(resolve)));
    const request = (path, method, body) => fetch(new URL(path, url), {
        method, headers: { "Content-Type": "application/json" }, body: JSON.stringify(body),
    });
    assert.equal((await (await fetch(new URL("health", url))).json()).status, "ready");
    const page = await fetch(url);
    assert.match(page.headers.get("content-security-policy"), /img-src.*blob:/);
    const created = await request("api/backlog", "POST", { title: "HTTP original", image: { dataUrl: TINY_PNG_DATA_URL } });
    assert.equal(created.status, 201);
    const original = (await created.json()).entry;
    const tasks = await (await fetch(new URL("api/tasks", url))).json();
    const id = `backlog:${original.id}`;
    const priority = await request("api/priority", "POST", { assign: { id, slot: tasks.slots[0].key }, order: [id] });
    assert.equal(priority.status, 200);
    const priorityBefore = await readFile(join(root, "docs", "handoff", "priority.json"), "utf8");
    const edited = await request(`api/backlog/${original.id}`, "PATCH", {
        title: "HTTP edited", description: "Changed", revision: original.revision,
    });
    assert.equal(edited.status, 200);
    const saved = (await edited.json()).entry;
    assert.equal(saved.id, original.id);
    assert.equal(saved.imageUrl, original.imageUrl);
    assert.notEqual(saved.revision, original.revision);
    assert.equal(await readFile(join(root, "docs", "handoff", "priority.json"), "utf8"), priorityBefore);
    const image = await fetch(new URL(saved.imageUrl, url));
    assert.equal(image.headers.get("content-type"), "image/png");
    assert.deepEqual(Buffer.from(await image.arrayBuffer()), Buffer.from(TINY_PNG_BASE64, "base64"));
    assert.equal((await request(`api/backlog/${original.id}`, "PATCH", {
        title: "Stale", revision: original.revision,
    })).status, 409);
    assert.equal((await request("api/backlog/missing", "PATCH", { title: "Missing", revision: saved.revision })).status, 404);
    assert.equal((await request(`api/backlog/${original.id}`, "PATCH", { title: "" })).status, 400);
    const malformed = await fetch(new URL(`api/backlog/${original.id}`, url), { method: "PATCH", body: "{ invalid" });
    assert.equal(malformed.status, 400);
    assert.match((await malformed.json()).error, /Invalid feedback JSON/);
    assert.equal((await request("api/backlog", "POST", null)).status, 400);
    assert.deepEqual(chatCalls, []);
});
