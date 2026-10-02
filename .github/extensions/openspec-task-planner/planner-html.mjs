export function renderPlannerHtml() {
    return `<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Homestead OpenSpec tasks</title>
  <style>
    :root { color-scheme: light dark; }
    * { box-sizing: border-box; }
    body {
      margin: 0;
      min-height: 100vh;
      background: var(--background-color-default, #0d1117);
      color: var(--text-color-default, #e6edf3);
      font-family: var(--font-sans, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif);
      font-size: var(--text-body-medium, 14px);
      line-height: var(--leading-body-medium, 20px);
    }
    button, input { font: inherit; }
    .shell { max-width: 1400px; margin: 0 auto; padding: 24px; }
    .hero {
      display: flex;
      justify-content: space-between;
      gap: 24px;
      align-items: flex-start;
      margin-bottom: 20px;
    }
    h1 {
      margin: 0 0 6px;
      font-size: var(--text-title-large, 26px);
      line-height: var(--leading-title-large, 32px);
      font-weight: var(--font-weight-semibold, 650);
    }
    .subtitle, .muted { color: var(--text-color-muted, #8b949e); }
    .refresh {
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 8px;
      padding: 8px 14px;
      background: var(--button-default-bgColor-rest, #21262d);
      color: var(--text-color-default, #e6edf3);
      cursor: pointer;
    }
    .refresh:hover { border-color: var(--color-focus-outline, #58a6ff); }
    .stats {
      display: grid;
      grid-template-columns: repeat(3, minmax(120px, 1fr));
      gap: 12px;
      margin-bottom: 18px;
    }
    .stat, .feature {
      border: 1px solid var(--border-color-default, #30363d);
      background: var(--background-color-muted, #161b22);
      border-radius: 12px;
    }
    .stat { padding: 15px; }
    .stat strong { display: block; font-size: 22px; line-height: 28px; }
    .builds {
      display: grid;
      gap: 10px;
      margin-bottom: 18px;
    }
    .builds-head {
      display: flex;
      justify-content: space-between;
      align-items: baseline;
      gap: 12px;
    }
    .builds-head h2 { margin: 0; font-size: 17px; line-height: 24px; }
    .build-card {
      border: 1px solid var(--border-color-default, #30363d);
      background: var(--background-color-muted, #161b22);
      border-radius: 10px;
      padding: 12px 14px;
    }
    .build-card.current { border-color: var(--true-color-blue, #58a6ff); }
    .build-title { display: flex; flex-wrap: wrap; gap: 8px; align-items: center; }
    .build-title strong { font-size: 14px; }
    .build-meta { color: var(--text-color-muted, #8b949e); font-family: var(--font-mono, Consolas, monospace); font-size: 12px; }
    .build-card ul { margin: 8px 0 0; padding-left: 18px; }
    .build-card li { margin: 2px 0; }
    .build-card details { margin-top: 8px; }
    .build-card summary { cursor: pointer; color: var(--text-color-muted, #8b949e); font-size: 12px; }
    .toolbar {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
      margin-bottom: 18px;
    }
    .search {
      min-width: 260px;
      flex: 1;
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 8px;
      padding: 9px 11px;
      background: var(--background-color-default, #0d1117);
      color: var(--text-color-default, #e6edf3);
      outline: none;
    }
    .search:focus { border-color: var(--color-focus-outline, #58a6ff); }
    .filters { display: flex; gap: 6px; }
    .filter {
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 999px;
      padding: 7px 12px;
      background: transparent;
      color: var(--text-color-muted, #8b949e);
      cursor: pointer;
    }
    .filter.active {
      background: var(--true-color-blue-muted, #1f6feb33);
      border-color: var(--true-color-blue, #58a6ff);
      color: var(--text-color-default, #e6edf3);
    }
    .board { display: grid; gap: 14px; }
    .feature { overflow: hidden; }
    .feature > summary {
      list-style: none;
      cursor: pointer;
      padding: 16px 18px;
    }
    .feature > summary::-webkit-details-marker { display: none; }
    .feature-head {
      display: grid;
      grid-template-columns: minmax(220px, 1fr) auto;
      gap: 16px;
      align-items: center;
    }
    .feature-title { display: flex; gap: 9px; align-items: center; }
    .feature-title h2 { margin: 0; font-size: 17px; line-height: 24px; }
    .badge {
      border-radius: 999px;
      padding: 2px 8px;
      font-size: 11px;
      font-weight: var(--font-weight-semibold, 600);
      text-transform: uppercase;
      letter-spacing: .04em;
    }
    .badge.active { background: var(--true-color-blue-muted, #1f6feb33); color: var(--true-color-blue, #58a6ff); }
    .badge.proposed { background: var(--background-color-default, #0d1117); color: var(--text-color-muted, #8b949e); border: 1px solid var(--border-color-default, #30363d); }
    .badge.paused { background: var(--true-color-yellow-muted, #9e6a032e); color: var(--true-color-yellow, #d29922); }
    .badge.complete { background: var(--true-color-green-muted, #23863633); color: var(--true-color-green, #3fb950); }
    .count { white-space: nowrap; font-variant-numeric: tabular-nums; }
    .bar { height: 6px; margin-top: 12px; border-radius: 999px; overflow: hidden; background: var(--border-color-default, #30363d); }
    .bar span { display: block; height: 100%; background: var(--true-color-blue, #58a6ff); }
    .feature.complete .bar span { background: var(--true-color-green, #3fb950); }
    .feature.paused .bar span { background: var(--true-color-yellow, #d29922); }
    .feature-body { border-top: 1px solid var(--border-color-default, #30363d); padding: 4px 18px 18px; }
    .section { margin-top: 16px; }
    .section h3 { margin: 0 0 8px; font-size: 13px; color: var(--text-color-muted, #8b949e); }
    .task {
      display: grid;
      grid-template-columns: 24px minmax(0, 1fr);
      gap: 8px;
      padding: 7px 0;
      border-bottom: 1px solid color-mix(in srgb, var(--border-color-default, #30363d) 55%, transparent);
    }
    .task:last-child { border-bottom: 0; }
    .task-id { color: var(--text-color-muted, #8b949e); font-family: var(--font-mono, Consolas, monospace); font-size: 12px; }
    .task.done .task-text { color: var(--text-color-muted, #8b949e); text-decoration: line-through; text-decoration-color: color-mix(in srgb, currentColor 55%, transparent); }
    .completed-group {
      margin-top: 16px;
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 8px;
      background: color-mix(in srgb, var(--background-color-default, #0d1117) 55%, transparent);
      overflow: hidden;
    }
    .completed-group > summary {
      list-style: none;
      cursor: pointer;
      display: flex;
      align-items: center;
      justify-content: space-between;
      gap: 12px;
      padding: 9px 11px;
      color: var(--text-color-muted, #8b949e);
      font-size: 12px;
      font-weight: var(--font-weight-semibold, 600);
    }
    .completed-group > summary::-webkit-details-marker { display: none; }
    .completed-group > summary::after { content: "Show"; font-weight: 400; }
    .completed-group[open] > summary::after { content: "Hide"; }
    .completed-content {
      border-top: 1px solid var(--border-color-default, #30363d);
      padding: 0 11px 11px;
    }
    .completed-content .section { margin-top: 12px; }
    .meta { margin-top: 13px; font-size: 12px; color: var(--text-color-muted, #8b949e); }
    .checklist {
      border: 1px solid var(--border-color-default, #30363d);
      background: var(--background-color-muted, #161b22);
      border-radius: 12px;
      overflow: hidden;
    }
    .check-row {
      display: grid;
      grid-template-columns: 22px minmax(0, 1fr) auto;
      gap: 10px;
      align-items: start;
      padding: 10px 16px;
      border-bottom: 1px solid color-mix(in srgb, var(--border-color-default, #30363d) 55%, transparent);
    }
    .check-row:last-child { border-bottom: 0; }
    .check-box {
      width: 16px; height: 16px; margin-top: 3px;
      border: 1.5px solid var(--text-color-muted, #8b949e);
      border-radius: 4px;
      display: grid; place-items: center;
      font-size: 12px; line-height: 1;
    }
    .check-row.done .check-box {
      border-color: var(--true-color-green, #3fb950);
      background: var(--true-color-green-muted, #23863633);
      color: var(--true-color-green, #3fb950);
    }
    .check-title { font-weight: var(--font-weight-semibold, 600); }
    .check-text { color: var(--text-color-muted, #8b949e); font-size: 13px; }
    .check-row.done .check-title, .check-row.done .check-text { text-decoration: line-through; text-decoration-color: color-mix(in srgb, currentColor 55%, transparent); }
    .empty { padding: 48px 20px; text-align: center; color: var(--text-color-muted, #8b949e); }
    .error {
      border: 1px solid var(--true-color-red, #f85149);
      background: var(--true-color-red-muted, #f8514922);
      color: var(--text-color-default, #e6edf3);
      border-radius: 10px;
      padding: 14px;
    }
    @media (max-width: 760px) {
      .shell { padding: 16px; }
      .hero { display: block; }
      .refresh { margin-top: 12px; }
      .stats { grid-template-columns: repeat(2, 1fr); }
      .feature-head { grid-template-columns: 1fr; gap: 7px; }
    }
    .backlog-head { margin: 4px 0 10px; }
    .check-row { grid-template-columns: 26px 18px minmax(0, 1fr) auto; cursor: grab; }
    .check-row.dragging { opacity: .45; }
    .check-row.next { background: color-mix(in srgb, var(--true-color-blue, #58a6ff) 9%, transparent); }
    .rank { color: var(--text-color-muted, #8b949e); font-variant-numeric: tabular-nums; text-align: right; padding-top: 1px; }
    .drag { color: var(--text-color-muted, #8b949e); letter-spacing: -3px; padding-top: 1px; user-select: none; }
    .next-button {
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 999px;
      padding: 4px 10px;
      background: transparent;
      color: var(--text-color-muted, #8b949e);
      cursor: pointer;
      white-space: nowrap;
      font-size: 12px;
    }
    .next-button:hover { border-color: var(--true-color-blue, #58a6ff); color: var(--text-color-default, #e6edf3); }
    .next-button.on {
      border-color: var(--true-color-blue, #58a6ff);
      background: var(--true-color-blue-muted, #1f6feb33);
      color: var(--text-color-default, #e6edf3);
    }
    .row-actions { display: flex; gap: 6px; align-items: center; }
    .slot-select {
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 999px;
      padding: 4px 8px;
      background: var(--background-color-default, #0d1117);
      color: var(--text-color-muted, #8b949e);
      font: inherit;
      font-size: 12px;
      cursor: pointer;
    }
    .slot-select.on {
      border-color: var(--true-color-blue, #58a6ff);
      background: var(--true-color-blue-muted, #1f6feb33);
      color: var(--text-color-default, #e6edf3);
    }
    .carried-note { color: var(--true-color-orange, #d29922); font-size: 12px; margin-top: 2px; }
    .chips { display: flex; flex-wrap: wrap; gap: 6px; margin-top: 10px; }
    .chip {
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 999px;
      padding: 2px 9px;
      font-size: 12px;
      color: var(--text-color-default, #e6edf3);
    }
    .chip.carried { border-color: var(--true-color-orange, #d29922); }
    .chip.carried::after { content: " ↻"; color: var(--true-color-orange, #d29922); }
    .icon-button {
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 999px;
      padding: 4px 9px;
      background: transparent;
      color: var(--text-color-muted, #8b949e);
      cursor: pointer;
      white-space: nowrap;
      font-size: 12px;
    }
    .icon-button:disabled { opacity: .35; cursor: default; }
    .icon-button:hover { border-color: var(--true-color-blue, #58a6ff); color: var(--text-color-default, #e6edf3); }
    .icon-button.danger:hover, .icon-button.armed { border-color: var(--true-color-red, #f85149); color: var(--true-color-red, #f85149); }
    .backlog-form {
      border: 1px solid var(--border-color-default, #30363d);
      background: var(--background-color-muted, #161b22);
      border-radius: 12px;
      padding: 16px;
      margin-bottom: 18px;
    }
    .backlog-form h2 { margin: 0; font-size: 16px; }
    .field { display: block; margin: 10px 0; }
    .field > span { display: block; margin-bottom: 4px; color: var(--text-color-muted, #8b949e); font-size: 12px; }
    .field input[type="text"], .field textarea {
      width: 100%;
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 8px;
      padding: 8px 10px;
      background: var(--background-color-default, #0d1117);
      color: var(--text-color-default, #e6edf3);
      resize: vertical;
    }
    .field input[type="file"] { color: var(--text-color-default, #e6edf3); }
    .backlog-form-actions { display: flex; align-items: center; gap: 12px; margin-top: 10px; }
    .backlog-recent { display: flex; flex-wrap: wrap; gap: 10px; margin-top: 14px; }
    .backlog-recent-item {
      display: flex;
      align-items: center;
      gap: 8px;
      border: 1px solid var(--border-color-default, #30363d);
      border-radius: 8px;
      padding: 6px 10px;
      max-width: 260px;
    }
    .backlog-recent-item img { width: 32px; height: 32px; object-fit: cover; border-radius: 4px; }
    .backlog-recent-item .backlog-recent-title { font-size: 12px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
    .error-text { color: var(--true-color-red, #f85149); }
  </style>
</head>
<body>
  <main class="shell">
    <header class="hero">
      <div>
        <h1>Homestead task planner</h1>
        <div class="subtitle">What shipped, what's building, and what's next. Drag to set priority; pick a release for each item.</div>
      </div>
      <button id="refresh" class="refresh" type="button">Refresh</button>
    </header>
    <section id="backlog-form-card" class="backlog-form" aria-label="Quick backlog entry">
      <div class="builds-head">
        <h2>Add to Jenny's backlog</h2>
        <span id="backlog-form-note" class="muted">No chat tokens spent — this writes straight to docs/handoff/backlog.md.</span>
      </div>
      <form id="backlog-form" novalidate>
        <label class="field">
          <span>Title</span>
          <input id="backlog-title" type="text" maxlength="200" required placeholder="e.g. Sell crops at the General Store">
        </label>
        <label class="field">
          <span>Description (optional)</span>
          <textarea id="backlog-description" maxlength="4000" rows="3" placeholder="What should it do? What did you see?"></textarea>
        </label>
        <label class="field">
          <span>Screenshot (optional, PNG/JPEG/WebP/GIF, max 8 MB)</span>
          <input id="backlog-image" type="file" accept="image/png,image/jpeg,image/webp,image/gif">
        </label>
        <div class="backlog-form-actions">
          <button id="backlog-submit" class="refresh" type="submit">Add to backlog</button>
          <span id="backlog-form-status" class="muted" role="status" aria-live="polite"></span>
        </div>
      </form>
      <div id="backlog-recent" class="backlog-recent"></div>
    </section>
    <section id="builds" class="builds" aria-label="Build changelist"></section>
    <div class="builds-head backlog-head">
      <h2>Planned improvements</h2>
      <span id="backlog-note" class="muted"></span>
    </div>
    <section id="board" class="board" aria-live="polite"></section>
  </main>
  <script>
    const state = { planner: null, signature: null, dragging: null };
    const buildsNode = document.getElementById("builds");
    const board = document.getElementById("board");
    const refresh = document.getElementById("refresh");
    const note = document.getElementById("backlog-note");
    const BACKLOG_MAX_TITLE_LENGTH = 200;
    const BACKLOG_MAX_DESCRIPTION_LENGTH = 4000;
    const BACKLOG_MAX_IMAGE_BYTES = 8 * 1024 * 1024;
    const BACKLOG_IMAGE_TYPES = new Set(["image/png", "image/jpeg", "image/webp", "image/gif"]);

    const el = (tag, className, text) => {
      const node = document.createElement(tag);
      if (className) node.className = className;
      if (text !== undefined) node.textContent = text;
      return node;
    };

    function renderBuildCard(build, current = false, assigned = []) {
      const card = el("article", "build-card" + (current ? " current" : ""));
      const title = el("div", "build-title");
      title.append(el("strong", "", build.label ?? [build.date, build.slot].filter(Boolean).join(" — ")),
        el("span", "badge " + (build.status === "delivered" ? "complete" : "active"), build.status));
      card.append(title);
      if (build.ships?.length) {
        const list = el("ul");
        for (const item of build.ships) list.append(el("li", "", item));
        card.append(list);
      }
      if (assigned.length) {
        const chips = el("div", "chips");
        for (const feature of assigned) {
          const chip = el("span", "chip" + (feature.carriedFrom ? " carried" : ""), feature.title);
          if (feature.carriedFrom) chip.title = "Carried over from " + feature.carriedFrom;
          chips.append(chip);
        }
        card.append(chips);
      }
      if (!build.ships?.length && !assigned.length) card.append(el("div", "build-meta", "Nothing scheduled yet"));
      return card;
    }

    function renderBuilds(builds) {
      buildsNode.replaceChildren();
      const slots = state.planner.slots ?? [];
      const features = state.planner.features;
      const head = el("div", "builds-head");
      head.append(el("h2", "", "Builds"), el("span", "muted", "Upcoming releases and recent deliveries"));
      buildsNode.append(head);
      const shown = new Set();
      slots.forEach((slot, index) => {
        const entry = builds?.entries?.find((build) => build.key === slot.key && build.status !== "delivered");
        const assigned = features.filter((feature) => feature.slot === slot.key);
        if (entry) shown.add(entry);
        if (!entry && !assigned.length && index > 0) return;
        buildsNode.append(renderBuildCard({ ...(entry ?? { status: "planned", ships: [] }), label: slot.label, status: entry?.status ?? "planned" }, index === 0, assigned));
      });
      const byTime = (a, b) => (Number.isFinite(a.time) ? a.time : 0) - (Number.isFinite(b.time) ? b.time : 0);
      for (const build of [...(builds?.entries ?? [])].sort(byTime)) {
        if (build.status === "delivered" || shown.has(build)) continue;
        buildsNode.append(renderBuildCard(build));
      }
      const delivered = (builds?.entries ?? []).filter((build) => build.status === "delivered").sort((a, b) => byTime(b, a));
      for (const build of delivered.slice(0, 2)) buildsNode.append(renderBuildCard(build));
      if (delivered.length > 2) {
        const collapsed = el("details", "build-card");
        collapsed.append(el("summary", "", "Older delivered builds (" + (delivered.length - 2) + ")"));
        for (const build of delivered.slice(2)) collapsed.append(renderBuildCard(build));
        buildsNode.append(collapsed);
      }
    }

    async function savePriority(payload) {
      const response = await fetch("/api/priority", {
        method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify(payload),
      });
      if (!response.ok) throw new Error("Couldn't save priority: " + response.status);
      return response.json();
    }

    function currentOrder() {
      return [...board.querySelectorAll(".check-row")].map((row) => row.dataset.id);
    }

    function renderBoard() {
      board.replaceChildren();
      const features = state.planner.features.filter((feature) => feature.status !== "complete");
      const flagged = features.filter((feature) => feature.slot).length;
      note.textContent = features.length + " items" + (flagged ? " · " + flagged + " scheduled" : "");
      if (!features.length) {
        board.append(el("div", "empty", "Nothing planned. Playtest feedback will land here."));
        return;
      }
      const list = el("div", "checklist");
      features.forEach((feature, index) => {
        const open = feature.sections.flatMap((section) => section.tasks).filter((task) => !task.done);
        const row = el("div", "check-row" + (feature.slot ? " next" : ""));
        row.dataset.id = feature.id;
        row.draggable = true;
        row.title = feature.path;
        const text = el("div");
        text.append(el("div", "check-title", feature.title));
        if (feature.carriedFrom) text.append(el("div", "carried-note", "Carried over from " + feature.carriedFrom));
        for (const task of open.slice(0, 3)) text.append(el("div", "check-text", task.text));
        const button = el("select", "slot-select" + (feature.slot ? " on" : ""));
        button.title = "Schedule for a release";
        button.append(new Option("Unscheduled", ""));
        for (const slot of state.planner.slots ?? []) button.append(new Option(slot.label, slot.key));
        button.value = feature.slot ?? "";
        for (const eventName of ["mousedown", "click", "dragstart"]) button.addEventListener(eventName, (event) => event.stopPropagation());
        button.addEventListener("change", async () => {
          button.disabled = true;
          try { await savePriority({ assign: { id: feature.id, slot: button.value || null } }); await load(true, true); }
          catch (error) { note.textContent = error.message; }
          finally { button.disabled = false; }
        });
        const actions = el("div", "row-actions");
        const quote = el("button", "icon-button", "❝ Quote");
        quote.type = "button";
        quote.title = "Attach this item to the chat";
        quote.addEventListener("click", async (event) => {
          event.stopPropagation();
          quote.disabled = true;
          try {
            const response = await fetch("/api/quote", {
              method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ id: feature.id }),
            });
            if (!response.ok) throw new Error((await response.json()).error ?? "Couldn't attach");
            note.textContent = "Attached “" + feature.title + "” to the chat";
          } catch (error) { note.textContent = error.message; }
          finally { quote.disabled = false; }
        });
        const remove = el("button", "icon-button danger", "✕");
        remove.type = "button";
        remove.title = "Remove from the list";
        remove.addEventListener("click", async (event) => {
          event.stopPropagation();
          if (remove.dataset.armed !== "1") {
            remove.dataset.armed = "1";
            remove.textContent = "Remove?";
            remove.classList.add("armed");
            setTimeout(() => { remove.dataset.armed = ""; remove.textContent = "✕"; remove.classList.remove("armed"); }, 3000);
            return;
          }
          remove.disabled = true;
          try { await savePriority({ remove: feature.id }); await load(true, true); note.textContent = "Removed “" + feature.title + "”"; }
          catch (error) { note.textContent = error.message; remove.disabled = false; }
        });
        const top = el("button", "icon-button", "⤒ Top");
        top.type = "button";
        top.title = "Move to top";
        top.disabled = index === 0;
        top.addEventListener("click", async (event) => {
          event.stopPropagation();
          top.disabled = true;
          const order = [feature.id, ...currentOrder().filter((id) => id !== feature.id)];
          try { await savePriority({ order }); await load(true, true); note.textContent = "Moved “" + feature.title + "” to the top"; }
          catch (error) { note.textContent = error.message; top.disabled = false; }
        });
        actions.append(top, button, quote, remove);
        row.append(el("div", "rank", String(index + 1)), el("div", "drag", "⋮⋮"), text, actions);
        row.addEventListener("dragstart", (event) => {
          state.dragging = row;
          row.classList.add("dragging");
          event.dataTransfer.effectAllowed = "move";
          event.dataTransfer.setData("text/plain", feature.id);
        });
        row.addEventListener("dragend", async () => {
          row.classList.remove("dragging");
          state.dragging = null;
          [...list.querySelectorAll(".rank")].forEach((rank, i) => { rank.textContent = String(i + 1); });
          try { await savePriority({ order: currentOrder() }); note.textContent = "Order saved"; }
          catch (error) { note.textContent = error.message; }
        });
        row.addEventListener("dragover", (event) => {
          event.preventDefault();
          const dragging = state.dragging;
          if (!dragging || dragging === row) return;
          const box = row.getBoundingClientRect();
          const after = event.clientY > box.top + box.height / 2;
          list.insertBefore(dragging, after ? row.nextSibling : row);
        });
        list.append(row);
      });
      board.append(list);
    }

    function render() {
      renderBuilds(state.planner.builds);
      renderBoard();
    }

    function fileToDataUrl(file) {
      return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = () => resolve(reader.result);
        reader.onerror = () => reject(new Error("Couldn't read the screenshot file"));
        reader.readAsDataURL(file);
      });
    }

    function renderBacklogRecent(entries) {
      const host = document.getElementById("backlog-recent");
      host.replaceChildren();
      for (const entry of entries.slice(-6).reverse()) {
        const item = el("div", "backlog-recent-item");
        if (entry.imageUrl) {
          const img = document.createElement("img");
          img.src = entry.imageUrl;
          img.alt = "";
          item.append(img);
        }
        item.append(el("span", "backlog-recent-title", entry.title));
        item.title = entry.title + (entry.description ? " — " + entry.description : "");
        host.append(item);
      }
    }

    async function loadBacklogRecent() {
      try {
        const response = await fetch("/api/backlog", { cache: "no-store" });
        if (!response.ok) return;
        const data = await response.json();
        renderBacklogRecent(Array.isArray(data.entries) ? data.entries : []);
      } catch {
        // Non-fatal: the recent-submissions strip is a convenience, not required reading.
      }
    }

    const backlogForm = document.getElementById("backlog-form");
    const backlogTitle = document.getElementById("backlog-title");
    const backlogDescription = document.getElementById("backlog-description");
    const backlogImage = document.getElementById("backlog-image");
    const backlogSubmit = document.getElementById("backlog-submit");
    const backlogStatus = document.getElementById("backlog-form-status");

    backlogForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      backlogStatus.className = "muted";
      const title = backlogTitle.value.trim();
      const description = backlogDescription.value.trim();
      if (!title) {
        backlogStatus.className = "error-text";
        backlogStatus.textContent = "Title is required.";
        backlogTitle.focus();
        return;
      }
      if (title.length > BACKLOG_MAX_TITLE_LENGTH) {
        backlogStatus.className = "error-text";
        backlogStatus.textContent = "Title is too long (max " + BACKLOG_MAX_TITLE_LENGTH + " characters).";
        return;
      }
      if (description.length > BACKLOG_MAX_DESCRIPTION_LENGTH) {
        backlogStatus.className = "error-text";
        backlogStatus.textContent = "Description is too long (max " + BACKLOG_MAX_DESCRIPTION_LENGTH + " characters).";
        return;
      }
      const file = backlogImage.files?.[0] ?? null;
      let image;
      if (file) {
        if (!BACKLOG_IMAGE_TYPES.has(file.type)) {
          backlogStatus.className = "error-text";
          backlogStatus.textContent = "Screenshot must be a PNG, JPEG, WebP, or GIF image.";
          return;
        }
        if (file.size > BACKLOG_MAX_IMAGE_BYTES) {
          backlogStatus.className = "error-text";
          backlogStatus.textContent = "Screenshot is too large (max " + Math.floor(BACKLOG_MAX_IMAGE_BYTES / (1024 * 1024)) + " MB).";
          return;
        }
        try {
          image = { dataUrl: await fileToDataUrl(file) };
        } catch (error) {
          backlogStatus.className = "error-text";
          backlogStatus.textContent = error.message;
          return;
        }
      }
      backlogSubmit.disabled = true;
      backlogStatus.textContent = "Adding...";
      try {
        const response = await fetch("/api/backlog", {
          method: "POST", headers: { "Content-Type": "application/json" }, body: JSON.stringify({ title, description, image }),
        });
        const data = await response.json().catch(() => ({}));
        if (!response.ok) throw new Error(data.error ?? "Couldn't add to the backlog");
        backlogForm.reset();
        backlogStatus.className = "muted";
        backlogStatus.textContent = "Added “" + title + "” to docs/handoff/backlog.md.";
        await loadBacklogRecent();
      } catch (error) {
        backlogStatus.className = "error-text";
        backlogStatus.textContent = error.message;
      } finally {
        backlogSubmit.disabled = false;
      }
    });

    async function load(quiet = false, force = false) {
      if (!quiet) {
        refresh.disabled = true;
        refresh.textContent = "Refreshing...";
      }
      try {
        const response = await fetch("/api/tasks", { cache: "no-store" });
        if (!response.ok) throw new Error("Planner request failed: " + response.status);
        const planner = await response.json();
        const signature = JSON.stringify({ features: planner.features, builds: planner.builds });
        if (state.dragging) return;
        if (force || !quiet || state.signature !== signature) {
          state.planner = planner;
          state.signature = signature;
          render();
        }
      } catch (error) {
        state.signature = null;
        board.replaceChildren(el("div", "error", error.message));
      } finally {
        if (!quiet) {
          refresh.disabled = false;
          refresh.textContent = "Refresh";
        }
      }
    }

    refresh.addEventListener("click", () => load(false));
    load(false);
    loadBacklogRecent();
    setInterval(() => { load(true); loadBacklogRecent(); }, 10000);
  </script>
</body>
</html>`;
}
