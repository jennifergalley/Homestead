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
    .accounting { margin-bottom: 22px; }
    .accounting h2 { font-size: 17px; margin: 0 0 8px; }
    .accounting h3 { font-size: 15px; margin: 0; }
    .accounting p { margin: 8px 0; max-width: 75ch; }
    .accounting summary { cursor: pointer; font-weight: var(--font-weight-semibold, 600); }
    .cost-period { margin: 0 0 32px; }
    .cost-period figcaption { margin-bottom: 12px; }
    .cost-period-line, .cost-build-head { display: flex; flex-wrap: wrap; justify-content: space-between; align-items: baseline; gap: 8px 20px; }
    .cost-chart-scroll { overflow-x: auto; padding: 4px 4px 8px; }
    .cost-chart { display: grid; grid-auto-flow: column; grid-auto-columns: minmax(28px, 1fr); min-width: 930px; gap: 4px; }
    .cost-day { display: flex; flex-direction: column; padding: 4px 0; border: 0; border-radius: 4px; background: none; color: inherit; cursor: pointer; font-variant-numeric: tabular-nums; font-size: 11px; }
    .cost-day:hover, .cost-day:focus-visible { background: var(--background-color-muted, #161b22); }
    .cost-day:focus-visible { outline: 2px solid var(--color-focus-outline, #58a6ff); outline-offset: 1px; }
    .cost-day.future { color: var(--text-color-muted, #8b949e); }
    .cost-bar-area { height: 190px; width: 100%; display: flex; flex-direction: column; align-items: center; justify-content: flex-end; border-bottom: 1px solid var(--border-color-default, #30363d); }
    .cost-bar { display: block; width: 70%; max-width: 28px; min-height: 2px; background: var(--true-color-blue, #58a6ff); border-radius: 3px 3px 0 0; }
    .cost-day.zero .cost-bar { background: var(--border-color-default, #30363d); }
    .cost-day.unknown .cost-bar { background: repeating-linear-gradient(135deg, var(--text-color-muted, #8b949e) 0 2px, transparent 2px 5px); border: 1px solid var(--text-color-muted, #8b949e); }
    .cost-day-label { display: block; padding-top: 8px; }
    .cost-day-value { display: block; margin-bottom: 4px; }
    .cost-chart-readout { min-height: 20px; font-variant-numeric: tabular-nums; }
    .cost-build { padding: 24px 0; border-top: 1px solid var(--border-color-default, #30363d); }
    .cost-build-total { font-weight: var(--font-weight-semibold, 600); font-variant-numeric: tabular-nums; }
    .cost-build-content { display: grid; grid-template-columns: minmax(0, 2fr) minmax(240px, 1fr); gap: 20px 40px; margin-top: 12px; }
    .cost-features { margin: 0; padding-left: 20px; max-width: 75ch; }
    .cost-features li { margin-bottom: 6px; }
    .cost-groups { margin: 0; }
    .cost-group { display: flex; justify-content: space-between; gap: 16px; margin-bottom: 8px; }
    .cost-group dt { color: var(--text-color-muted, #8b949e); }
    .cost-group dd { margin: 0; white-space: nowrap; font-variant-numeric: tabular-nums; }
    .cost-older { margin-top: 16px; }
    .cost-error { color: var(--true-color-red, #ff7b72); }
    @media (max-width: 700px) {
      .cost-build-content { grid-template-columns: minmax(0, 1fr); gap: 16px; }
      .cost-period-line { flex-direction: column; }
    }
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
    .check-row.shipped { background: color-mix(in srgb, var(--true-color-green, #3fb950) 9%, transparent); }
    .delivery-note { color: var(--true-color-green, #3fb950); font-size: 12px; margin-top: 2px; }
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
    button:focus-visible, input:focus-visible, textarea:focus-visible, select:focus-visible {
      outline: 2px solid var(--color-focus-outline, #58a6ff);
      outline-offset: 2px;
    }
    .check-row > div, .check-title, .check-text { min-width: 0; overflow-wrap: anywhere; }
    .row-actions { flex-wrap: wrap; justify-content: flex-end; }
    @media (max-width: 760px) {
      .check-row { grid-template-columns: 26px 18px minmax(0, 1fr); }
      .row-actions { grid-column: 3; justify-content: flex-start; }
    }
    .backlog-form {
      border: 1px solid var(--border-color-default, #30363d);
      background: var(--background-color-muted, #161b22);
      border-radius: 12px;
      padding: 16px;
      margin-bottom: 18px;
    }
    .backlog-form h2 { margin: 0; font-size: 16px; }
    .backlog-form .builds-head { flex-wrap: wrap; }
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
    .backlog-form-actions { flex-wrap: wrap; }
    .screenshot-preview img { display: block; max-width: 100%; max-height: 180px; margin: 8px 0; object-fit: contain; }
    .screenshot-preview p { margin: 8px 0; }
    [hidden] { display: none !important; }
    .backlog-origin { font-style: italic; }
    .backlog-thumb { width: 28px; height: 28px; object-fit: cover; border-radius: 4px; margin-top: 4px; display: block; }
    .error-text { color: var(--true-color-red, #f85149); }
    .tabs { display: flex; gap: 4px; border-bottom: 1px solid var(--border-color-default, #30363d); margin-bottom: 18px; }
    .tab-button {
      border: 1px solid transparent;
      border-bottom: none;
      border-radius: 8px 8px 0 0;
      padding: 8px 16px;
      background: transparent;
      color: var(--text-color-muted, #8b949e);
      cursor: pointer;
      font-weight: var(--font-weight-semibold, 600);
    }
    .tab-button:hover { color: var(--text-color-default, #e6edf3); }
    .tab-button.active {
      color: var(--text-color-default, #e6edf3);
      border-color: var(--border-color-default, #30363d);
      background: var(--background-color-muted, #161b22);
    }
    .tab-panel.hidden { display: none; }
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
    <nav class="tabs" role="tablist" aria-label="Planner sections">
      <button id="tab-planning-btn" class="tab-button active" type="button" role="tab" aria-selected="true" aria-controls="tab-planning">Planning</button>
      <button id="tab-cost-btn" class="tab-button" type="button" role="tab" aria-selected="false" aria-controls="tab-cost">Build cost</button>
    </nav>
    <div id="tab-planning" class="tab-panel" role="tabpanel" aria-labelledby="tab-planning-btn">
      <section id="backlog-form-card" class="backlog-form" aria-label="Quick backlog entry">
        <div class="builds-head">
          <h2 id="backlog-form-heading">Add to Jenny's backlog</h2>
          <span id="backlog-form-note" class="muted">Saved locally. No agent notifications.</span>
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
          <div id="backlog-image-preview" class="screenshot-preview" hidden>
            <p id="backlog-image-note" class="muted"></p>
            <button id="backlog-image-remove" class="icon-button" type="button">Remove screenshot</button>
          </div>
          <div class="backlog-form-actions">
            <button id="backlog-submit" class="refresh" type="submit">Add to backlog</button>
            <button id="backlog-cancel" class="icon-button" type="button" hidden>Cancel</button>
            <span id="backlog-form-status" class="muted" role="status" aria-live="polite"></span>
          </div>
        </form>
      </section>
      <section id="builds" class="builds" aria-label="Build changelist"></section>
      <div class="builds-head backlog-head">
        <h2>Planned improvements</h2>
        <span id="backlog-note" class="muted"></span>
      </div>
      <section id="board" class="board" aria-live="polite"></section>
    </div>
    <div id="tab-cost" class="tab-panel hidden" role="tabpanel" aria-labelledby="tab-cost-btn">
      <section id="accounting" class="accounting" aria-label="Measured build costs"></section>
    </div>
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
      const items = build.status === "delivered" ? build.shippedFeatures ?? build.ships : build.ships;
      const title = el("div", "build-title");
      title.append(el("strong", "", build.label ?? [build.date, build.slot].filter(Boolean).join(" — ")),
        el("span", "badge " + (build.status === "delivered" ? "complete" : "active"), build.status));
      card.append(title);
      if (build.dataError) card.append(el("p", "cost-error", build.dataError));
      if (items?.length) {
        const list = el("ul");
        for (const item of items) list.append(el("li", "", item));
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
      if (!items?.length && !assigned.length) card.append(el("div", "build-meta", build.status === "delivered" ? "Shipped features not recorded" : "Nothing scheduled yet"));
      return card;
    }

    function renderBuilds(builds) {
      buildsNode.replaceChildren();
      const slots = state.planner.slots ?? [];
      const features = state.planner.features;
      const head = el("div", "builds-head");
      head.append(el("h2", "", "Builds"), el("span", "muted", "Upcoming releases and recent deliveries"));
      buildsNode.append(head);
      slots.forEach((slot, index) => {
        const entry = builds?.entries?.find((build) => build.key === slot.key && build.status === "planned");
        const assigned = features.filter((feature) => feature.slot === slot.key);
        if (!entry && !assigned.length && index > 0) return;
        buildsNode.append(renderBuildCard({ ...(entry ?? { status: "planned", ships: [] }), label: slot.label, status: entry?.status ?? "planned" }, index === 0, assigned));
      });
      const byTime = (a, b) => (Number.isFinite(a.time) ? a.time : 0) - (Number.isFinite(b.time) ? b.time : 0);
      const delivered = (builds?.entries ?? []).filter((build) => build.status === "delivered").sort((a, b) => byTime(b, a));
      for (const build of delivered.slice(0, 2)) buildsNode.append(renderBuildCard(build));
      if (delivered.length > 2) {
        const collapsed = el("details", "build-card");
        collapsed.append(el("summary", "", "Older delivered builds (" + (delivered.length - 2) + ")"));
        for (const build of delivered.slice(2)) collapsed.append(renderBuildCard(build));
        buildsNode.append(collapsed);
      }
    }
    function aiu(value) {
      if (value == null) return "unknown";
      const nano = BigInt(value);
      return (Number(nano) / 1e9).toLocaleString("en-US", { maximumFractionDigits: 3 });
    }

    function renderAccounting(view) {
      const node = document.getElementById("accounting");
      node.replaceChildren();
      if (!view) {
        node.append(el("p", "muted", "No shipment cost view yet. Missing costs are unknown, not zero."));
        return;
      }
      const period = view.period;
      const figure = el("figure", "cost-period");
      const caption = el("figcaption");
      caption.append(el("h2", "", "Daily shipped-build cost"));
      if (period.status === "current") {
        const line = el("div", "cost-period-line");
        line.append(el("span", "muted", period.start + " – " + period.end + " · Resets " + period.reset),
          el("span", "", aiu(period.recordedNanoAiu) + " recorded AIU" + (period.unknown ? " + unknown costs" : "")));
        caption.append(line);
        figure.append(caption);
        const scroll = el("div", "cost-chart-scroll");
        const chart = el("div", "cost-chart");
        chart.setAttribute("role", "group");
        chart.setAttribute("aria-label", "Daily shipped-build cost for the current billing period");
        const readout = el("p", "cost-chart-readout muted", "Select a day for its cost and shipment count. Zero means no shipment; ? means missing cost.");
        readout.setAttribute("aria-live", "polite");
        const max = Math.max(1, ...period.days.map((day) => Number(BigInt(day.recordedNanoAiu)) / 1e9));
        period.days.forEach((day, index) => {
          const value = Number(BigInt(day.recordedNanoAiu)) / 1e9;
          const amount = day.unknown && !value ? "Unknown cost" : aiu(day.recordedNanoAiu) + " recorded AIU" + (day.unknown ? " + unknown costs" : "");
          const description = day.date + " · " + amount + " · " + day.shipments + " shipped build" + (day.shipments === 1 ? "" : "s") + (day.future ? " · Upcoming day" : "");
          const button = el("button", "cost-day" + (day.unknown && !value ? " unknown" : !value ? " zero" : "") + (day.future ? " future" : ""));
          button.type = "button";
          button.dataset.date = day.date;
          button.setAttribute("aria-label", description);
          button.title = description;
          const area = el("span", "cost-bar-area");
          const shortValue = day.unknown && !value ? "?" : (value >= 1000 ? (value / 1000).toFixed(1) + "k" : value ? value.toFixed(1) : "0") + (day.unknown ? "+" : "");
          area.append(el("span", "cost-day-value", shortValue));
          const bar = el("span", "cost-bar");
          bar.style.height = Math.max(day.unknown && !value ? 12 : 2, value / max * 160) + "px";
          area.append(bar);
          button.append(area, el("span", "cost-day-label", index === 0 || day.date.endsWith("-01") ? day.date.slice(5).replace("-", "/") : day.date.slice(8)));
          button.addEventListener("focus", () => { readout.textContent = description; });
          button.addEventListener("click", () => { readout.textContent = description; });
          button.addEventListener("keydown", (event) => {
            const target = event.key === "ArrowRight" ? index + 1 : event.key === "ArrowLeft" ? index - 1 : null;
            if (target !== null && chart.children[target]) { event.preventDefault(); chart.children[target].focus(); }
          });
          chart.append(button);
        });
        scroll.append(chart); figure.append(scroll, readout);
      } else {
        figure.append(caption, el("p", "cost-error", period.message));
      }
      figure.append(el("p", "muted", "Observed AIU, not billing-reconciled AI credits. Homestead shipped-build costs are assigned to shipment day, not the day calls ran."));
      node.append(figure, el("h2", "", "Shipped builds"));
      const older = el("details", "cost-older");
      older.append(el("summary", "", "Earlier shipped builds (" + Math.max(0, view.shipped.length - 2) + ")"));
      view.shipped.forEach((build, index) => {
        const article = el("article", "cost-build");
        const head = el("div", "cost-build-head");
        head.append(el("h3", "", build.label),
          el("span", "cost-build-total", build.recordedNanoAiu == null ? "Cost unknown" : aiu(build.recordedNanoAiu) + " recorded AIU"));
        article.append(head);
        if (build.dataError) article.append(el("p", "cost-error", build.dataError));
        const content = el("div", "cost-build-content");
        const list = el("ul", "cost-features");
        for (const feature of build.features) list.append(el("li", "", feature));
        if (!build.features.length) list.append(el("li", "muted", "Shipped features not recorded."));
        content.append(list);
        const groups = el("dl", "cost-groups");
        for (const group of build.groups) {
          const row = el("div", "cost-group");
          row.append(el("dt", "", group.label), el("dd", "", aiu(group.recordedNanoAiu)));
          groups.append(row);
        }
        if (build.groups.length) content.append(groups);
        article.append(content);
        if (build.partial) article.append(el("p", "muted", build.recordedNanoAiu == null
          ? "No cost export for this shipment. Missing costs are unknown, not zero."
          : "Recorded subtotal; billing reconciliation" + (build.inheritedUnknown ? " and inherited costs" : "") + " remain incomplete."));
        if (build.estimatedNanoAiu !== "0") article.append(el("p", "muted", aiu(build.estimatedNanoAiu) + " estimated AIU, separate from the recorded subtotal."));
        if (build.reportPath) {
          const evidence = el("details");
          evidence.append(el("summary", "muted", "Source report"), el("p", "muted", build.reportPath));
          article.append(evidence);
        }
        (index < 2 ? node : older).append(article);
      });
      if (view.shipped.length > 2) node.append(older);
      if (!view.shipped.length) node.append(el("p", "muted", "No shipped builds recorded yet."));
      if (view.unshippedReports) {
        node.append(el("p", "muted", view.unshippedReports + " unshipped cost report(s) excluded from shipment totals."));
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
        const row = el("div", "check-row" + (feature.slot ? " next" : "") + (feature.deliveryStatus === "shipped" ? " shipped" : ""));
        row.dataset.id = feature.id;
        row.draggable = true;
        row.title = feature.path ?? feature.description ?? feature.title;
        const text = el("div");
        text.append(el("div", "check-title", feature.title));
        if (feature.deliveryStatus === "shipped") {
          text.append(el("div", "delivery-note", "Shipped in " + feature.deliveryBuildId + " · Player acceptance pending"));
        }
        if (feature.carriedFrom) text.append(el("div", "carried-note", "Carried over from " + feature.carriedFrom));
        if (feature.fromBacklog) {
          text.append(el("div", "check-text backlog-origin", "From Jenny's backlog form" + (feature.description ? " — " + feature.description : "")));
          if (feature.imageUrl) {
            const thumb = document.createElement("img");
            thumb.className = "backlog-thumb";
            thumb.src = feature.imageUrl;
            thumb.alt = "";
            text.append(thumb);
          }
        }
        for (const task of open.slice(0, 3)) text.append(el("div", "check-text", task.text));
        const button = el("select", "slot-select" + (feature.slot ? " on" : ""));
        button.title = "Schedule for a release";
        if (feature.deliveryStatus === "shipped") {
          button.append(new Option("Shipped", ""));
          button.title = "Delivered; player acceptance remains pending.";
          button.disabled = true;
        } else {
          button.append(new Option("Unscheduled", ""));
          for (const slot of state.planner.slots ?? []) button.append(new Option(slot.label, slot.key));
        }
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
        if (feature.fromBacklog) {
          const edit = el("button", "icon-button feedback-edit", "✎");
          edit.type = "button";
          edit.title = "Edit feedback";
          edit.setAttribute("aria-label", "Edit feedback: " + feature.title);
          edit.disabled = feedbackState.busy || !!feedbackState.entry;
          for (const eventName of ["mousedown", "dragstart"]) edit.addEventListener(eventName, (event) => event.stopPropagation());
          edit.addEventListener("click", (event) => {
            event.stopPropagation();
            beginFeedbackEdit(feature.name);
          });
          actions.append(edit);
        }
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
      renderAccounting(state.planner.costView);
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

    const backlogForm = document.getElementById("backlog-form");
    const backlogTitle = document.getElementById("backlog-title");
    const backlogDescription = document.getElementById("backlog-description");
    const backlogImage = document.getElementById("backlog-image");
    const backlogSubmit = document.getElementById("backlog-submit");
    const backlogStatus = document.getElementById("backlog-form-status");
    const backlogHeading = document.getElementById("backlog-form-heading");
    const backlogCancel = document.getElementById("backlog-cancel");
    const backlogPreview = document.getElementById("backlog-image-preview");
    const backlogCurrentImage = el("img");
    backlogCurrentImage.id = "backlog-current-image";
    backlogCurrentImage.alt = "Feedback screenshot";
    backlogCurrentImage.hidden = true;
    backlogPreview.prepend(backlogCurrentImage);
    const backlogImageNote = document.getElementById("backlog-image-note");
    const backlogImageRemove = document.getElementById("backlog-image-remove");
    const feedbackState = { entry: null, busy: false, imageRemoved: false, previewUrl: null };

    function feedbackStatus(message, error = false) {
      backlogStatus.className = error ? "error-text" : "muted";
      backlogStatus.textContent = message;
    }

    function hasFeedbackDraft() {
      return !!(backlogTitle.value || backlogDescription.value || backlogImage.files?.length || feedbackState.entry);
    }

    function updateFeedbackControls() {
      const editing = !!feedbackState.entry;
      backlogHeading.textContent = editing ? "Edit Jenny's feedback" : "Add to Jenny's backlog";
      backlogSubmit.textContent = feedbackState.busy ? (editing ? "Saving..." : "Working...") : (editing ? "Save changes" : "Add to backlog");
      backlogCancel.hidden = !hasFeedbackDraft();
      for (const control of [backlogTitle, backlogDescription, backlogImage, backlogSubmit, backlogCancel, backlogImageRemove]) {
        control.disabled = feedbackState.busy;
      }
      for (const edit of board.querySelectorAll(".feedback-edit")) edit.disabled = feedbackState.busy || editing;
    }

    function updateFeedbackScreenshot() {
      if (feedbackState.previewUrl) URL.revokeObjectURL(feedbackState.previewUrl);
      feedbackState.previewUrl = null;
      const file = backlogImage.files?.[0];
      const existing = !feedbackState.imageRemoved && feedbackState.entry?.imageUrl;
      if (file && BACKLOG_IMAGE_TYPES.has(file.type) && file.size <= BACKLOG_MAX_IMAGE_BYTES) {
        feedbackState.previewUrl = URL.createObjectURL(file);
      }
      const source = feedbackState.previewUrl || (existing ? existing + "?revision=" + feedbackState.entry.revision : null);
      backlogPreview.hidden = !source && !feedbackState.imageRemoved;
      backlogCurrentImage.hidden = !source;
      if (source) backlogCurrentImage.src = source;
      else backlogCurrentImage.removeAttribute("src");
      backlogImageRemove.hidden = !source;
      backlogImageNote.textContent = file ? "New screenshot" : feedbackState.imageRemoved ? "Screenshot will be removed when you save." : "Current screenshot";
    }

    function resetFeedbackForm() {
      backlogForm.reset();
      feedbackState.entry = null;
      feedbackState.imageRemoved = false;
      updateFeedbackScreenshot();
      updateFeedbackControls();
    }

    async function beginFeedbackEdit(id) {
      if (feedbackState.busy || feedbackState.entry) return;
      if (hasFeedbackDraft()) {
        feedbackStatus("Save or cancel your current draft first.", true);
        backlogTitle.focus();
        return;
      }
      feedbackState.busy = true;
      updateFeedbackControls();
      feedbackStatus("Opening feedback...");
      try {
        const response = await fetch("/api/backlog", { cache: "no-store" });
        const data = await response.json();
        if (!response.ok) throw new Error(data.error ?? "Couldn't open feedback. Try again.");
        const entry = data.entries.find((value) => value.id === id);
        if (!entry) throw new Error("This feedback no longer exists. Refresh the planner.");
        feedbackState.entry = entry;
        feedbackState.imageRemoved = false;
        backlogTitle.value = entry.title;
        backlogDescription.value = entry.description ?? "";
        backlogImage.value = "";
        updateFeedbackScreenshot();
        feedbackStatus("");
      } catch (error) {
        feedbackStatus(error.message, true);
      } finally {
        feedbackState.busy = false;
        updateFeedbackControls();
      }
      if (feedbackState.entry) {
        document.getElementById("backlog-form-card").scrollIntoView({ block: "start" });
        backlogTitle.focus();
      }
    }

    backlogCancel.addEventListener("click", () => {
      if (feedbackState.busy) return;
      const id = feedbackState.entry?.id;
      resetFeedbackForm();
      feedbackStatus("Changes cancelled.");
      const row = [...board.querySelectorAll(".check-row")].find((value) => value.dataset.id === "backlog:" + id);
      (row?.querySelector(".feedback-edit") ?? backlogTitle).focus();
    });
    backlogImageRemove.addEventListener("click", () => {
      backlogImage.value = "";
      feedbackState.imageRemoved = !!feedbackState.entry?.imageUrl;
      updateFeedbackScreenshot();
      updateFeedbackControls();
    });
    backlogImage.addEventListener("change", () => {
      if (backlogImage.files?.length) feedbackState.imageRemoved = false;
      updateFeedbackScreenshot();
      updateFeedbackControls();
    });
    for (const input of [backlogTitle, backlogDescription]) input.addEventListener("input", updateFeedbackControls);

    async function submitFeedback() {
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
      let image = feedbackState.imageRemoved ? null : undefined;
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
      const entry = feedbackState.entry;
      const response = await fetch(entry ? "/api/backlog/" + encodeURIComponent(entry.id) : "/api/backlog", {
        method: entry ? "PATCH" : "POST", headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ title, description, image, revision: entry?.revision }),
      });
      const data = await response.json();
      if (!response.ok) throw new Error(data.error ?? "Couldn't save feedback. Try again.");
      resetFeedbackForm();
      feedbackStatus(data.warning ?? (entry ? "Saved “" + title + "”." : "Added “" + title + "” to the backlog."), !!data.warning);
      await load(true, true);
    }

    backlogForm.addEventListener("submit", async (event) => {
      event.preventDefault();
      if (feedbackState.busy) return;
      feedbackState.busy = true;
      updateFeedbackControls();
      feedbackStatus(feedbackState.entry ? "Saving..." : "Adding...");
      try {
        await submitFeedback();
      } catch (error) {
        feedbackStatus(error.message, true);
      } finally {
        feedbackState.busy = false;
        updateFeedbackControls();
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
        const signature = JSON.stringify({ features: planner.features, builds: planner.builds, accounting: planner.accounting, costView: planner.costView });
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

    const tabPlanningBtn = document.getElementById("tab-planning-btn");
    const tabCostBtn = document.getElementById("tab-cost-btn");
    const tabPlanning = document.getElementById("tab-planning");
    const tabCost = document.getElementById("tab-cost");
    function showTab(name) {
      const planning = name === "planning";
      tabPlanning.classList.toggle("hidden", !planning);
      tabCost.classList.toggle("hidden", planning);
      tabPlanningBtn.classList.toggle("active", planning);
      tabCostBtn.classList.toggle("active", !planning);
      tabPlanningBtn.setAttribute("aria-selected", String(planning));
      tabCostBtn.setAttribute("aria-selected", String(!planning));
    }
    tabPlanningBtn.addEventListener("click", () => showTab("planning"));
    tabCostBtn.addEventListener("click", () => showTab("cost"));

    load(false);
    setInterval(() => load(true), 10000);
  </script>
</body>
</html>`;
}
