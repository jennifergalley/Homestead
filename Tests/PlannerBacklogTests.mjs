import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile, readFile } from "node:fs/promises";
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
    loadBacklogInbox,
    loadPlanner,
    toBacklogClientEntry,
    backlogMdPath,
    backlogAttachmentsDir,
    BACKLOG_MAX_IMAGE_BYTES,
} from "../.github/extensions/openspec-task-planner/planner-data.mjs";

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
