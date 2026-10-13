import assert from "node:assert/strict";
import { mkdtemp, mkdir, rm, writeFile } from "node:fs/promises";
import { tmpdir } from "node:os";
import { join } from "node:path";
import { pathToFileURL } from "node:url";
import test from "node:test";
import { addBacklogEntry, backlogMdPath } from "../.github/extensions/openspec-task-planner/planner-data.mjs";
import { startPlannerServer } from "../.github/extensions/openspec-task-planner/planner-server.mjs";

// Optional browser proof uses an E-drive helper install, never Jenny's browser profile.
const browserModule = process.env.PLANNER_BROWSER_MODULE;

async function fixture(t) {
    const root = await mkdtemp(join(tmpdir(), "planner-search-"));
    t.after(() => rm(root, { recursive: true, force: true }));
    await mkdir(join(root, "docs", "handoff"), { recursive: true });
    await writeFile(backlogMdPath(root), "# Feedback\n\n## Later\n\n- Existing text\n");
    await mkdir(join(root, "openspec", "changes", "real-feature"), { recursive: true });
    await writeFile(join(root, "openspec", "changes", "real-feature", "tasks.md"), "## Work\n- [ ] 1.1 Gather the mussels\n");
    await addBacklogEntry(root, { title: "Roots are overpowered", description: "They grow in only 2 days" });
    await addBacklogEntry(root, { title: "Titles on interaction cards are too small", description: "Here, Sapling is far too small" });
    await addBacklogEntry(root, { title: "Remove this entirely" });
    const session = new Proxy({}, { get() { throw new Error("No chat permitted"); } });
    const { server, url } = await startPlannerServer(root, "search-proof", session);
    t.after(() => new Promise((resolve) => server.close(resolve)));
    return url;
}

test("the planner page offers a backlog search box", async (t) => {
    const url = await fixture(t);
    const html = await (await fetch(url)).text();
    assert.match(html, /id="backlog-search"/);
    assert.match(html, /id="backlog-search-clear"/);
});

test("backlog search filters, highlights and clears in an isolated headless browser", {
    skip: !browserModule && "Set PLANNER_BROWSER_MODULE to a playwright-core module on E: to run browser proof.",
}, async (t) => {
    assert.match(tmpdir(), /^E:[\\/]/i, "Browser fixtures and profile must stay on E:");
    const { chromium } = await import(pathToFileURL(browserModule).href);
    const url = await fixture(t);
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
        if (!["GET", "HEAD"].includes(request.method())) mutations.push(request.url());
    });
    await page.goto(url);
    await page.locator(".check-row").first().waitFor();

    const search = page.locator("#backlog-search");
    const clear = page.locator("#backlog-search-clear");
    const visible = page.locator(".check-row:not([hidden])");
    const total = await page.locator(".check-row").count();
    assert.equal(total, 4);
    assert.equal(await visible.count(), total);
    assert.equal(await clear.isHidden(), true);

    // Matches the description, ignores case, and highlights the hit.
    await search.fill("SAPLING");
    assert.equal(await visible.count(), 1);
    assert.match(await visible.first().innerText(), /Titles on interaction cards/);
    assert.equal(await visible.first().locator("mark.search-hit").first().innerText(), "Sapling");
    assert.match(await page.locator("#backlog-note").innerText(), /1 of 4 items match/);
    assert.equal(await clear.isVisible(), true);

    // Every word must match; titles and open task text count too.
    await search.fill("roots overpowered");
    assert.equal(await visible.count(), 1);
    assert.equal(await visible.first().locator("mark.search-hit").count(), 2);
    await search.fill("mussels");
    assert.equal(await visible.count(), 1);
    assert.equal(await visible.first().locator(".check-text mark.search-hit").count(), 1);
    await search.fill("roots mussels");
    assert.equal(await visible.count(), 0);
    assert.match(await page.locator("#board").innerText(), /No items match/);

    // The clear button and Escape both restore the full list.
    await clear.click();
    assert.equal(await search.inputValue(), "");
    assert.equal(await visible.count(), total);
    assert.equal(await page.locator("mark.search-hit").count(), 0);
    await search.fill("small");
    assert.equal(await visible.count(), 1);
    await search.press("Escape");
    assert.equal(await search.inputValue(), "");
    assert.equal(await visible.count(), total);

    assert.deepEqual(errors, []);
    assert.deepEqual(mutations, [], "Searching never writes");
});
