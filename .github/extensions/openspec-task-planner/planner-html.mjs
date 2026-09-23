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
      grid-template-columns: repeat(4, minmax(120px, 1fr));
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
    .feature summary {
      list-style: none;
      cursor: pointer;
      padding: 16px 18px;
    }
    .feature summary::-webkit-details-marker { display: none; }
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
    .badge.complete { background: var(--true-color-green-muted, #23863633); color: var(--true-color-green, #3fb950); }
    .count { white-space: nowrap; font-variant-numeric: tabular-nums; }
    .bar { height: 6px; margin-top: 12px; border-radius: 999px; overflow: hidden; background: var(--border-color-default, #30363d); }
    .bar span { display: block; height: 100%; background: var(--true-color-blue, #58a6ff); }
    .feature.complete .bar span { background: var(--true-color-green, #3fb950); }
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
    .meta { margin-top: 13px; font-size: 12px; color: var(--text-color-muted, #8b949e); }
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
  </style>
</head>
<body>
  <main class="shell">
    <header class="hero">
      <div>
        <h1>Homestead task planner</h1>
        <div class="subtitle">Live OpenSpec progress across every planned feature.</div>
      </div>
      <button id="refresh" class="refresh" type="button">Refresh tasks</button>
    </header>
    <section id="stats" class="stats" aria-label="Progress summary"></section>
    <section class="toolbar" aria-label="Planner controls">
      <input id="search" class="search" type="search" placeholder="Search features and tasks..." autocomplete="off">
      <div class="filters" role="group" aria-label="Status filter">
        <button class="filter active" data-filter="all" type="button">All</button>
        <button class="filter" data-filter="active" type="button">Active</button>
        <button class="filter" data-filter="complete" type="button">Complete</button>
      </div>
    </section>
    <section id="board" class="board" aria-live="polite"></section>
  </main>
  <script>
    const state = { planner: null, filter: "all", query: "" };
    const stats = document.getElementById("stats");
    const board = document.getElementById("board");
    const search = document.getElementById("search");
    const refresh = document.getElementById("refresh");

    const el = (tag, className, text) => {
      const node = document.createElement(tag);
      if (className) node.className = className;
      if (text !== undefined) node.textContent = text;
      return node;
    };

    function renderStats(summary) {
      stats.replaceChildren();
      const values = [
        ["Tasks complete", summary.completedTasks + " / " + summary.totalTasks],
        ["Overall progress", summary.totalTasks ? Math.round(summary.completedTasks * 100 / summary.totalTasks) + "%" : "0%"],
        ["Active features", String(summary.activeFeatures)],
        ["Completed features", String(summary.completedFeatures)],
      ];
      for (const [label, value] of values) {
        const card = el("div", "stat");
        card.append(el("strong", "", value), el("span", "muted", label));
        stats.append(card);
      }
    }

    function featureMatches(feature) {
      if (state.filter !== "all" && feature.status !== state.filter) return false;
      if (!state.query) return true;
      const haystack = [
        feature.title,
        feature.name,
        ...feature.sections.flatMap((section) => [section.title, ...section.tasks.map((task) => task.text)]),
      ].join(" ").toLowerCase();
      return haystack.includes(state.query);
    }

    function renderBoard() {
      board.replaceChildren();
      const features = state.planner.features.filter(featureMatches);
      if (!features.length) {
        board.append(el("div", "empty", "No OpenSpec work matches this view."));
        return;
      }
      for (const feature of features) {
        const details = el("details", "feature " + feature.status);
        if (feature.status === "active") details.open = true;
        const summary = el("summary");
        const head = el("div", "feature-head");
        const title = el("div", "feature-title");
        title.append(el("h2", "", feature.title), el("span", "badge " + feature.status, feature.status));
        head.append(title, el("div", "count", feature.completed + " / " + feature.total));
        const bar = el("div", "bar");
        const fill = el("span");
        fill.style.width = (feature.total ? feature.completed * 100 / feature.total : 0) + "%";
        bar.append(fill);
        summary.append(head, bar);
        details.append(summary);

        const body = el("div", "feature-body");
        for (const section of feature.sections) {
          const sectionNode = el("section", "section");
          sectionNode.append(el("h3", "", section.title));
          for (const task of section.tasks) {
            const taskNode = el("div", "task" + (task.done ? " done" : ""));
            taskNode.append(el("div", "task-id", task.id), el("div", "task-text", task.text));
            sectionNode.append(taskNode);
          }
          body.append(sectionNode);
        }
        body.append(el("div", "meta", feature.path + " | Updated " + new Date(feature.modifiedAt).toLocaleString()));
        details.append(body);
        board.append(details);
      }
    }

    function render() {
      renderStats(state.planner.summary);
      renderBoard();
    }

    async function load() {
      refresh.disabled = true;
      refresh.textContent = "Refreshing...";
      try {
        const response = await fetch("/api/tasks", { cache: "no-store" });
        if (!response.ok) throw new Error("Planner request failed: " + response.status);
        state.planner = await response.json();
        render();
      } catch (error) {
        board.replaceChildren(el("div", "error", error.message));
      } finally {
        refresh.disabled = false;
        refresh.textContent = "Refresh tasks";
      }
    }

    search.addEventListener("input", () => {
      state.query = search.value.trim().toLowerCase();
      if (state.planner) renderBoard();
    });
    for (const button of document.querySelectorAll(".filter")) {
      button.addEventListener("click", () => {
        state.filter = button.dataset.filter;
        document.querySelectorAll(".filter").forEach((item) => item.classList.toggle("active", item === button));
        if (state.planner) renderBoard();
      });
    }
    refresh.addEventListener("click", load);
    load();
  </script>
</body>
</html>`;
}
