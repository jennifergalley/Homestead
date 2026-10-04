import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile, readFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import test from "node:test";
import { addBacklogEntry, loadBacklogInbox, loadPlanner, backlogInboxPath, backlogMdPath, backlogAttachmentsDir } from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";

// Optional browser proof uses an E-drive helper install, never Jenny's browser profile.
const browserModule = process.env.PLANNER_BROWSER_MODULE;
const png = Buffer.from("iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAQAAAC1HAwCAAAAC0lEQVR42mNk+A8AAQUBAScY42YAAAAASUVORK5CYII=", "base64");
const image = { dataUrl: `data:image/png;base64,${png.toString("base64")}` };

test("feedback editing in an isolated headless browser", {
    skip: !browserModule && "Set PLANNER_BROWSER_MODULE to a playwright-core module on E: to run browser proof.",
}, async (t) => {
    assert.match(tmpdir(), /^E:[\\/]/i, "Browser fixtures and profile must stay on E:");
    const { chromium } = await import(pathToFileURL(browserModule).href);
    const root = await mkdtemp(join(tmpdir(), "planner-editing-browser-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    await mkdir(join(root, "docs", "handoff"), { recursive: true });
    await writeFile(backlogMdPath(root), "# Feedback\n\n## Later\n\n- Existing text\n");
    await mkdir(join(root, "openspec", "changes", "real-feature"), { recursive: true });
    await writeFile(join(root, "openspec", "changes", "real-feature", "tasks.md"), "## Work\n- [ ] 1.1 Real feature\n");
    const first = (await addBacklogEntry(root, { title: "Original feedback", description: "Original description", image })).value;
    const second = (await addBacklogEntry(root, { title: "No screenshot" })).value;
    const planner = await loadPlanner(root);
    const priorityPath = join(root, "docs", "handoff", "priority.json");
    await writeFile(priorityPath, JSON.stringify({
        order: [`backlog:${first.id}`, `backlog:${second.id}`], removed: [],
        slots: { [`backlog:${first.id}`]: planner.slots[0].key },
    }));
    const priorityBefore = await readFile(priorityPath, "utf8");
    const chatCalls = [];
    const session = new Proxy({}, { get(_, key) { chatCalls.push(key); throw new Error("No chat permitted"); } });
    const { server, url } = await startPlannerServer(root, "browser-proof", session);
    t.after(() => new Promise((resolve) => server.close(resolve)));
    const browser = await chromium.launch({
        headless: true,
        executablePath: process.env.PLANNER_BROWSER_EXECUTABLE ?? "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
    });
    t.after(() => browser.close());
    const page = await browser.newPage({ viewport: { width: 1440, height: 1000 } });
    page.setDefaultTimeout(10_000);
    const errors = [];
    page.on("pageerror", (error) => errors.push(error.message));
    const mutations = [];
    page.on("request", (request) => {
        if (!["GET", "HEAD"].includes(request.method())) mutations.push(request);
    });
    await page.goto(url);
    await page.getByRole("button", { name: "Edit feedback: Original feedback", exact: true }).waitFor();
    assert.equal(await page.locator(".feedback-edit").count(), 2);
    assert.equal(await page.locator('[data-id="real-feature"] .feedback-edit').count(), 0);

    const status = page.locator("#backlog-form-status");
    const title = page.locator("#backlog-title");
    const description = page.locator("#backlog-description");
    const preview = page.locator("#backlog-current-image");
    const pickPng = () => page.locator("#backlog-image").setInputFiles({ name: "replacement.png", mimeType: "image/png", buffer: png });
    const waitStatus = (pattern) => page.waitForFunction((source) =>
        new RegExp(source).test(document.getElementById("backlog-form-status").textContent)
        && !document.getElementById("backlog-submit").disabled, pattern.source);
    const open = async (name) => {
        await page.getByRole("button", { name: "Edit feedback: " + name, exact: true }).click();
        await page.getByRole("button", { name: "Save changes", exact: true }).waitFor();
        await page.waitForFunction(() => {
            const img = document.getElementById("backlog-current-image");
            return img.hidden || (img.complete && img.naturalWidth > 0);
        });
    };

    await t.test("pencil prefills the form; cancel discards text, replacement and removal without a write", async () => {
        const before = await readFile(backlogInboxPath(root), "utf8");
        const mdBefore = await readFile(backlogMdPath(root), "utf8");
        await open("Original feedback");
        assert.equal(await title.inputValue(), first.title);
        assert.equal(await description.inputValue(), first.description);
        assert.equal(await preview.isVisible(), true);
        await title.fill("Uncommitted draft");
        await pickPng();
        assert.match(await preview.getAttribute("src"), /^blob:/);
        await page.getByRole("button", { name: "Remove screenshot", exact: true }).click();
        await page.getByRole("button", { name: "Cancel", exact: true }).click();
        assert.equal(await readFile(backlogInboxPath(root), "utf8"), before);
        assert.equal(await readFile(backlogMdPath(root), "utf8"), mdBefore);
        assert.equal(await readFile(priorityPath, "utf8"), priorityBefore);
        assert.equal(mutations.length, 0);
        assert.equal(await title.inputValue(), "");
    });

    await t.test("save edits text in place and retains the screenshot, release and row order", async () => {
        await open("Original feedback");
        await title.fill("Edited <feedback> & text");
        await description.fill("New description\nSecond line");
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/^Saved/);
        const entry = (await loadBacklogInbox(root))[0];
        assert.equal(entry.id, first.id);
        assert.equal(entry.createdUtc, first.createdUtc);
        assert.equal(entry.imageFile, first.imageFile);
        assert.equal(entry.title, "Edited <feedback> & text");
        const row = page.locator(`[data-id="backlog:${first.id}"]`);
        assert.equal(await row.locator(".check-title").textContent(), entry.title);
        assert.equal(await row.locator("select").inputValue(), planner.slots[0].key);
        assert.equal(await page.locator(".check-row").first().getAttribute("data-id"), `backlog:${first.id}`);
        assert.equal(await readFile(priorityPath, "utf8"), priorityBefore);
        assert.ok(!("image" in mutations.at(-1).postDataJSON()), "Keeping an image must omit image from PATCH.");
    });

    await t.test("replace, remove and add screenshot; reload preserves the saved content", async () => {
        await open("Edited <feedback> & text");
        await pickPng();
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/^Saved/);
        let entry = (await loadBacklogInbox(root))[0];
        assert.notEqual(entry.imageFile, first.imageFile);
        assert.deepEqual(await readFile(join(backlogAttachmentsDir(root), first.imageFile)), png);
        await open(entry.title);
        await page.getByRole("button", { name: "Remove screenshot", exact: true }).click();
        assert.equal(await preview.isVisible(), false);
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/^Saved/);
        entry = (await loadBacklogInbox(root))[0];
        assert.equal(entry.imageFile, null);
        assert.equal(mutations.at(-1).postDataJSON().image, null);
        await open("No screenshot");
        assert.equal(await preview.isVisible(), false);
        await pickPng();
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/^Saved/);
        assert.ok((await loadBacklogInbox(root))[1].imageFile);
        await page.reload();
        await page.getByRole("button", { name: "Edit feedback: No screenshot", exact: true }).waitFor();
        await open("No screenshot");
        assert.equal(await preview.isVisible(), true);
        await page.getByRole("button", { name: "Cancel", exact: true }).click();
    });

    await t.test("invalid, network-error, stale and missing entries retain drafts and expose errors", async () => {
        await open("No screenshot");
        const before = await readFile(backlogInboxPath(root), "utf8");
        await title.fill(" ");
        const count = mutations.length;
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/Title is required/);
        assert.equal(mutations.length, count);
        await title.fill("Still a draft");
        await page.route("**/api/backlog/" + second.id, (route) => route.abort());
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/fetch|network/i);
        assert.equal(await title.inputValue(), "Still a draft");
        assert.equal(await page.getByRole("button", { name: "Save changes" }).isEnabled(), true);
        assert.equal(await readFile(backlogInboxPath(root), "utf8"), before);
        await page.unroute("**/api/backlog/" + second.id);
        const document = JSON.parse(before);
        document.entries[1].description = "Concurrent correction";
        await writeFile(backlogInboxPath(root), JSON.stringify(document));
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/changed elsewhere/);
        assert.equal(await title.inputValue(), "Still a draft");
        assert.equal((await loadBacklogInbox(root))[1].description, "Concurrent correction");
        document.entries.pop();
        await writeFile(backlogInboxPath(root), JSON.stringify(document));
        await page.getByRole("button", { name: "Save changes", exact: true }).click();
        await waitStatus(/no longer exists/);
        assert.equal(await title.inputValue(), "Still a draft");
        await page.getByRole("button", { name: "Cancel", exact: true }).click();
        await page.getByRole("button", { name: "Refresh", exact: true }).click();
        await page.locator(".feedback-edit").first().waitFor();
    });

    await t.test("desktop and narrow layout keep pencil, form and screenshot controls accessible", async () => {
        await open("Edited <feedback> & text");
        assert.equal(await page.getByRole("button", { name: "Cancel" }).isVisible(), true);
        for (const width of [1440, 390]) {
            await page.setViewportSize({ width, height: 1000 });
            assert.equal(await page.evaluate(() => document.documentElement.scrollWidth <= window.innerWidth), true);
            for (const selector of ["#backlog-title", "#backlog-description", "#backlog-submit", "#backlog-cancel"]) {
                const box = await page.locator(selector).boundingBox();
                assert.ok(box.x >= 0 && box.x + box.width <= width, selector);
            }
            if (process.env.PLANNER_EVIDENCE_DIR) {
                await page.screenshot({ path: join(process.env.PLANNER_EVIDENCE_DIR, `planner-edit-${width}.png`), fullPage: true });
            }
        }
        await page.getByRole("button", { name: "Cancel", exact: true }).click();
        assert.equal(await status.textContent(), "Changes cancelled.");
    });
    assert.deepEqual(errors, []);
    assert.deepEqual(chatCalls, []);
    assert.equal(await readFile(priorityPath, "utf8"), priorityBefore);
});
