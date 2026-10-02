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
        <div class="subtitle">Active work, paused work, and plans at a glance. Completed tasks stay tucked away.</div>
      </div>
      <button id="refresh" class="refresh" type="button">Refresh tasks</button>
    </header>
    <section id="builds" class="builds" aria-label="Build changelist"></section>
    <section id="stats" class="stats" aria-label="Progress summary"></section>
    <section class="toolbar" aria-label="Planner controls">
      <input id="search" class="search" type="search" placeholder="Search features and tasks..." autocomplete="off">
      <div class="filters" role="group" aria-label="Status filter">
        <button class="filter active" data-filter="all" type="button">All</button>
        <button class="filter" data-filter="active" type="button">Active</button>
        <button class="filter" data-filter="paused" type="button">Paused</button>
        <button class="filter" data-filter="proposed" type="button">Proposed</button>
        <button class="filter" data-filter="complete" type="button">Complete</button>
      </div>
    </section>
    <section id="board" class="board" aria-live="polite"></section>
  </main>
  <script>
    const state = {
      planner: null, signature: null, filter: "all", query: "",
      expanded: new Map(), completedExpanded: new Map(),
    };
    const stats = document.getElementById("stats");
    const buildsNode = document.getElementById("builds");
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
        ["Paused features", String(summary.pausedFeatures)],
        ["Proposed features", String(summary.proposedFeatures)],
        ["Completed features", String(summary.completedFeatures)],
      ];
      for (const [label, value] of values) {
        const card = el("div", "stat");
        card.append(el("strong", "", value), el("span", "muted", label));
        stats.append(card);
      }

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
        head.append(el("h2", "", "Builds"), el("span", "muted", "Current changelist and deferred work"));
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
        if (builds.later?.length) {
          const later = el("details", "build-card");
          later.append(el("summary", "", "Later (" + builds.later.length + ")"));
          const list = el("ul");
          for (const item of builds.later) list.append(el("li", "", item));
          later.append(list);
          buildsNode.append(later);
        }
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
        details.open = state.expanded.get(feature.id)
          ?? (feature.status === "active" || feature.status === "paused");
        details.addEventListener("toggle", () => state.expanded.set(feature.id, details.open));
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
        const appendSections = (parent, sections, done) => {
          for (const section of sections) {
            const tasks = section.tasks.filter((task) => task.done === done);
            if (!tasks.length) continue;
            const sectionNode = el("section", "section");
            sectionNode.append(el("h3", "", section.title));
            for (const task of tasks) {
              const taskNode = el("div", "task" + (task.done ? " done" : ""));
              taskNode.append(el("div", "task-id", task.id), el("div", "task-text", task.text));
              sectionNode.append(taskNode);
            }
            parent.append(sectionNode);
          }
        };
        appendSections(body, feature.sections, false);

        if (feature.completed > 0) {
          const completed = el("details", "completed-group");
          completed.open = state.completedExpanded.get(feature.id) ?? false;
          completed.addEventListener("toggle", () =>
            state.completedExpanded.set(feature.id, completed.open));
          completed.append(el("summary", "", "Completed (" + feature.completed + ")"));
          const completedContent = el("div", "completed-content");
          appendSections(completedContent, feature.sections, true);
          completed.append(completedContent);
          body.append(completed);
        }
        body.append(el("div", "meta", feature.path + " | Updated " + new Date(feature.modifiedAt).toLocaleString()));
        details.append(body);
        board.append(details);
      }
    }

    function render() {
      renderBuilds(state.planner.builds);
      renderStats(state.planner.summary);
      renderBoard();
    }

    async function load(quiet = false) {
      if (!quiet) {
        refresh.disabled = true;
        refresh.textContent = "Refreshing...";
      }
      try {
        const response = await fetch("/api/tasks", { cache: "no-store" });
        if (!response.ok) throw new Error("Planner request failed: " + response.status);
        const planner = await response.json();
        const signature = JSON.stringify({ summary: planner.summary, features: planner.features, builds: planner.builds });
        if (!quiet || state.signature !== signature) {
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
          refresh.textContent = "Refresh tasks";
        }
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
    refresh.addEventListener("click", () => load());
    load();
    setInterval(() => load(true), 60_000);
  </script>
</body>
</html>`;
}
