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
  </style>
</head>
<body>
  <main class="shell">
    <header class="hero">
      <div>
        <h1>Homestead task planner</h1>
        <div class="subtitle">What shipped, what's building, and what's next. Drag to set priority; flag items for the next build.</div>
      </div>
      <button id="refresh" class="refresh" type="button">Refresh</button>
    </header>
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

    const el = (tag, className, text) => {
      const node = document.createElement(tag);
      if (className) node.className = className;
      if (text !== undefined) node.textContent = text;
      return node;
    };

    function renderBuildCard(build, current = false) {
      const card = el("article", "build-card" + (current ? " current" : ""));
      const title = el("div", "build-title");
      title.append(el("strong", "", [build.date, build.slot].filter(Boolean).join(" — ")),
        el("span", "badge " + (build.status === "delivered" ? "complete" : "active"), build.status));
      card.append(title, el("div", "build-meta", build.sha));
      if (build.ships.length) {
        const list = el("ul");
        for (const item of build.ships) list.append(el("li", "", item));
        card.append(list);
      }
      return card;
    }

    function renderBuilds(builds) {
      buildsNode.replaceChildren();
      if (!builds?.entries?.length && !builds?.later?.length) return;
      const head = el("div", "builds-head");
      head.append(el("h2", "", "Builds"), el("span", "muted", "Current changelist and recent deliveries"));
      buildsNode.append(head);
      const current = builds.entries.find((build) => build.status === "building" || build.status === "planned")
        ?? builds.entries.find((build) => build.status === "delivered");
      if (current) buildsNode.append(renderBuildCard(current, true));
      const delivered = builds.entries.filter((build) => build.status === "delivered" && build !== current);
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
      const flagged = features.filter((feature) => feature.nextBuild).length;
      note.textContent = features.length + " items" + (flagged ? " · " + flagged + " flagged for the next build" : "");
      if (!features.length) {
        board.append(el("div", "empty", "Nothing planned. Playtest feedback will land here."));
        return;
      }
      const list = el("div", "checklist");
      features.forEach((feature, index) => {
        const open = feature.sections.flatMap((section) => section.tasks).filter((task) => !task.done);
        const row = el("div", "check-row" + (feature.nextBuild ? " next" : ""));
        row.dataset.id = feature.id;
        row.draggable = true;
        row.title = feature.path;
        const text = el("div");
        text.append(el("div", "check-title", feature.title));
        for (const task of open.slice(0, 3)) text.append(el("div", "check-text", task.text));
        const button = el("button", "next-button" + (feature.nextBuild ? " on" : ""),
          feature.nextBuild ? "★ Next build" : "☆ Next build");
        button.type = "button";
        button.addEventListener("click", async (event) => {
          event.stopPropagation();
          button.disabled = true;
          try { await savePriority({ toggleNext: feature.id }); await load(true, true); }
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
    setInterval(() => load(true), 10000);
  </script>
</body>
</html>`;
}
