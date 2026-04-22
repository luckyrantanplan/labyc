import { spawn } from "node:child_process";
import { existsSync } from "node:fs";
import { readFile } from "node:fs/promises";
import { createServer, type IncomingMessage, type ServerResponse } from "node:http";
import os from "node:os";
import path from "node:path";
import type { GalleryStageSnapshot, PipelineStage } from "./types.js";

interface GalleryState {
  title: string;
  projectDir: string;
  revision: number;
  updatedAt: string;
  stages: GalleryStageSnapshot[];
}

export interface PipelineGalleryHandle {
  readonly url: string;
  updateStage(index: number, update: Partial<GalleryStageSnapshot>): void;
  failStage(index: number, message: string): void;
  close(): Promise<void>;
}

function stageLabel(stage: PipelineStage, index: number): string {
  return stage.label ?? `${stage.kind} ${String(index + 1)}`;
}

function inferSvgPath(stage: GalleryStageSnapshot): string | undefined {
  if (stage.svgPath !== undefined && existsSync(stage.svgPath)) {
    return stage.svgPath;
  }

  if (stage.outputPath !== undefined && existsSync(stage.outputPath)) {
    return stage.outputPath;
  }

  return undefined;
}

function escapeHtml(value: string): string {
  return value
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#39;");
}

function buildArtifactUrl(svgPath: string, cacheToken: string | number): string {
  const search = new URLSearchParams({
    svg: svgPath,
    v: String(cacheToken)
  });
  return `/artifact?${search.toString()}`;
}

function buildPreviewUrl(svgPath: string, cacheToken: string | number): string {
  const search = new URLSearchParams({
    svg: svgPath,
    v: String(cacheToken)
  });
  return `/preview?${search.toString()}`;
}

function buildViewerUrl(svgPath: string): string {
  const search = new URLSearchParams({
    svg: svgPath
  });
  return `/viewer?${search.toString()}`;
}

function resolveQueriedSvgPath(requestUrl: URL): string | undefined {
  const svgPath = requestUrl.searchParams.get("svg");
  if (svgPath === null || svgPath.trim() === "") {
    return undefined;
  }

  if (!path.isAbsolute(svgPath)) {
    return undefined;
  }

  if (path.extname(svgPath).toLowerCase() !== ".svg") {
    return undefined;
  }

  return existsSync(svgPath) ? svgPath : undefined;
}

function findStageForSvgPath(stages: readonly GalleryStageSnapshot[], svgPath: string): GalleryStageSnapshot | undefined {
  return stages.find((stage) => inferSvgPath(stage) === svgPath);
}

function renderStageCard(stage: GalleryStageSnapshot): string {
  const svgPath = inferSvgPath(stage);
  const outputText = stage.outputPath ?? stage.svgPath ?? "pending output path";
  const outputName = outputText === "pending output path" ? outputText : path.basename(outputText);
  const previewMarkup = svgPath === undefined
    ? `<div class="placeholder ${stage.status}"><strong></strong><span>${escapeHtml(stage.message ?? "SVG will appear here as soon as the file is created.")}</span></div>`
    : `<iframe class="preview preview-frame" aria-label="${escapeHtml(stage.label)} preview" src="${escapeHtml(buildPreviewUrl(svgPath, Date.now()))}" loading="lazy"></iframe><a class="preview-link" href="${escapeHtml(buildViewerUrl(svgPath))}" target="_blank" rel="noopener noreferrer" aria-label="Open ${escapeHtml(stage.label)} in the zoom viewer"></a>`;

  return [
    '<article class="card">',
    '  <div class="card-head">',
    `    <div class="eyebrow">${escapeHtml(stage.stageKind)}</div>`,
    '    <div class="label-row">',
    `      <h2>${escapeHtml(stage.label)}</h2>`,
    `      <span class="badge ${stage.status}">${escapeHtml(stage.status)}</span>`,
    '    </div>',
    '  </div>',
    '  <div class="surface">',
    `    ${previewMarkup}`,
    '  </div>',
    '  <div class="card-foot">',
    `    <div class="path-name">${escapeHtml(outputName)}</div>`,
    `    <div class="path-value"><code>${escapeHtml(outputText)}</code></div>`,
    '  </div>',
    '</article>'
  ].join("\n");
}

function isTerminalStatus(status: GalleryStageSnapshot["status"]): boolean {
  return status === "completed" || status === "cached" || status === "failed";
}

function isStableState(stages: readonly GalleryStageSnapshot[]): boolean {
  return stages.some((stage) => stage.status === "failed")
    || stages.every((stage) => isTerminalStatus(stage.status));
}

function renderViewerHtml(stage: GalleryStageSnapshot, artifactUrl: string): string {
  const title = `${stage.label} Viewer`;

  return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>${escapeHtml(title)}</title>
  <style>
    :root {
      color-scheme: light;
      --paper: #f6f0e6;
      --panel: rgba(255, 251, 245, 0.92);
      --line: rgba(90, 71, 48, 0.18);
      --ink: #20170f;
      --muted: #6f5f4d;
      --accent: #1e6b78;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      min-height: 100vh;
      display: grid;
      grid-template-rows: auto 1fr;
      font-family: "Iosevka Aile", "IBM Plex Sans", sans-serif;
      color: var(--ink);
      background:
        radial-gradient(circle at top left, rgba(255, 255, 255, 0.9), transparent 24%),
        linear-gradient(135deg, #f7f1e7, #e4d3bf 52%, #d6c1aa);
    }

    header {
      display: flex;
      flex-wrap: wrap;
      justify-content: space-between;
      align-items: center;
      gap: 16px;
      padding: 18px 22px;
      background: var(--panel);
      border-bottom: 1px solid var(--line);
      backdrop-filter: blur(18px);
    }

    .title-block {
      display: grid;
      gap: 4px;
    }

    h1 {
      margin: 0;
      font-size: clamp(1.2rem, 2vw, 2rem);
      letter-spacing: -0.03em;
    }

    p {
      margin: 0;
      color: var(--muted);
    }

    .controls {
      display: flex;
      flex-wrap: wrap;
      gap: 10px;
      align-items: center;
    }

    button {
      border: 1px solid var(--line);
      background: white;
      color: var(--ink);
      border-radius: 999px;
      padding: 8px 14px;
      font: inherit;
      cursor: pointer;
    }

    button:hover {
      border-color: color-mix(in srgb, var(--accent) 42%, var(--line));
      color: var(--accent);
    }

    #zoomValue {
      min-width: 72px;
      text-align: center;
      font-variant-numeric: tabular-nums;
      color: var(--muted);
    }

    .canvas {
      padding: 22px;
    }

    .viewport {
      position: relative;
      overflow: hidden;
      min-height: calc(100vh - 120px);
      border: 1px solid var(--line);
      border-radius: 22px;
      background:
        linear-gradient(90deg, rgba(119, 91, 59, 0.06) 1px, transparent 1px),
        linear-gradient(rgba(119, 91, 59, 0.06) 1px, transparent 1px),
        linear-gradient(135deg, rgba(255, 255, 255, 0.78), rgba(243, 233, 220, 0.94));
      background-size: 24px 24px, 24px 24px, auto;
      cursor: grab;
      touch-action: none;
    }

    .viewport.dragging {
      cursor: grabbing;
    }

    .svg-host {
      position: absolute;
      inset: 0;
      padding: 22px;
    }

    .svg-host svg {
      display: block;
      width: 100%;
      height: 100%;
      background: rgba(255, 255, 255, 0.78);
      border-radius: 14px;
      box-shadow: 0 18px 26px rgba(42, 28, 15, 0.16);
    }

    .status {
      position: absolute;
      left: 18px;
      bottom: 18px;
      padding: 8px 12px;
      border-radius: 999px;
      background: rgba(255, 251, 245, 0.88);
      border: 1px solid var(--line);
      color: var(--muted);
      backdrop-filter: blur(12px);
    }
  </style>
</head>
<body>
  <header>
    <div class="title-block">
      <h1>${escapeHtml(title)}</h1>
      <p>Mouse wheel zooms, drag pans, double click resets.</p>
    </div>
    <div class="controls">
      <button id="zoomOut" type="button">-</button>
      <div id="zoomValue">100%</div>
      <button id="zoomIn" type="button">+</button>
      <button id="resetView" type="button">Reset view</button>
    </div>
  </header>
  <main class="canvas">
    <section id="viewport" class="viewport">
      <div id="svgHost" class="svg-host" aria-live="polite"></div>
      <div id="status" class="status">Loading SVG…</div>
    </section>
  </main>
  <script>
    const viewport = document.getElementById('viewport');
    const svgHost = document.getElementById('svgHost');
    const status = document.getElementById('status');
    const zoomValue = document.getElementById('zoomValue');
    const zoomIn = document.getElementById('zoomIn');
    const zoomOut = document.getElementById('zoomOut');
    const resetViewButton = document.getElementById('resetView');
    const artifactUrl = ${JSON.stringify(artifactUrl)};
    let currentSvg = null;
    let baseViewBox = null;
    let currentViewBox = null;
    let dragPointerId = null;
    let dragStartX = 0;
    let dragStartY = 0;
    let dragStartViewBox = null;

    function clamp(value, min, max) {
      return Math.min(Math.max(value, min), max);
    }

    function parseSvgNumber(value) {
      if (value === null) {
        return undefined;
      }

      const number = Number.parseFloat(value);
      return Number.isFinite(number) ? number : undefined;
    }

    function createViewBox(minX, minY, width, height) {
      return { minX, minY, width, height };
    }

    function cloneViewBox(viewBox) {
      return createViewBox(viewBox.minX, viewBox.minY, viewBox.width, viewBox.height);
    }

    function resolveViewBox(svg) {
      const rawViewBox = svg.getAttribute('viewBox');
      if (typeof rawViewBox === 'string') {
        const values = rawViewBox.trim().split(/[ ,]+/).map((value) => Number.parseFloat(value));
        if (values.length === 4 && values.every((value) => Number.isFinite(value)) && values[2] > 0 && values[3] > 0) {
          return createViewBox(values[0], values[1], values[2], values[3]);
        }
      }

      const width = parseSvgNumber(svg.getAttribute('width'));
      const height = parseSvgNumber(svg.getAttribute('height'));
      if (width !== undefined && height !== undefined && width > 0 && height > 0) {
        return createViewBox(0, 0, width, height);
      }

      try {
        const bounds = svg.getBBox();
        if (bounds.width > 0 && bounds.height > 0) {
          return createViewBox(bounds.x, bounds.y, bounds.width, bounds.height);
        }
      } catch {
        // Ignore getBBox failures and fall back to a neutral box.
      }

      return createViewBox(0, 0, 100, 100);
    }

    function getZoomLevel() {
      if (baseViewBox === null || currentViewBox === null) {
        return 1;
      }

      return baseViewBox.width / currentViewBox.width;
    }

    function applyViewBox() {
      if (currentSvg === null || currentViewBox === null) {
        return;
      }

      currentSvg.setAttribute(
        'viewBox',
        String(currentViewBox.minX) + ' ' + String(currentViewBox.minY) + ' ' + String(currentViewBox.width) + ' ' + String(currentViewBox.height)
      );
      zoomValue.textContent = String(Math.round(getZoomLevel() * 100)) + '%';
    }

    function getViewportAnchor(clientX, clientY) {
      const rect = viewport.getBoundingClientRect();
      return {
        x: clamp((clientX - rect.left) / rect.width, 0, 1),
        y: clamp((clientY - rect.top) / rect.height, 0, 1),
        rect
      };
    }

    function zoomBy(factor, clientX, clientY) {
      if (baseViewBox === null || currentViewBox === null) {
        return;
      }

      const currentZoom = getZoomLevel();
      const nextZoom = clamp(currentZoom * factor, 0.15, 20);
      const nextWidth = baseViewBox.width / nextZoom;
      const nextHeight = baseViewBox.height / nextZoom;
      const anchor = clientX === undefined || clientY === undefined
        ? { x: 0.5, y: 0.5 }
        : getViewportAnchor(clientX, clientY);
      const anchorWorldX = currentViewBox.minX + (anchor.x * currentViewBox.width);
      const anchorWorldY = currentViewBox.minY + (anchor.y * currentViewBox.height);

      currentViewBox = createViewBox(
        anchorWorldX - (anchor.x * nextWidth),
        anchorWorldY - (anchor.y * nextHeight),
        nextWidth,
        nextHeight
      );
      applyViewBox();
    }

    function resetView() {
      if (baseViewBox === null) {
        return;
      }

      currentViewBox = cloneViewBox(baseViewBox);
      applyViewBox();
    }

    async function loadSvg() {
      const response = await fetch(artifactUrl, { cache: 'no-store' });
      if (!response.ok) {
        throw new Error('Failed to load SVG: ' + String(response.status));
      }

      svgHost.innerHTML = await response.text();
      const svg = svgHost.querySelector('svg');
      if (!(svg instanceof SVGElement)) {
        throw new Error('The artifact is not a valid SVG.');
      }

      currentSvg = svg;
      baseViewBox = resolveViewBox(currentSvg);
      currentSvg.setAttribute('width', '100%');
      currentSvg.setAttribute('height', '100%');
      currentSvg.setAttribute('preserveAspectRatio', 'xMidYMid meet');
      currentSvg.style.width = '100%';
      currentSvg.style.height = '100%';
      currentSvg.style.maxWidth = 'none';
      currentSvg.style.maxHeight = 'none';
      status.textContent = 'Drag to pan. Wheel to zoom.';
      resetView();
    }

    viewport.addEventListener('wheel', (event) => {
      event.preventDefault();
      zoomBy(event.deltaY < 0 ? 1.12 : 1 / 1.12, event.clientX, event.clientY);
    }, { passive: false });

    viewport.addEventListener('pointerdown', (event) => {
      if (currentViewBox === null) {
        return;
      }

      dragPointerId = event.pointerId;
      dragStartX = event.clientX;
      dragStartY = event.clientY;
      dragStartViewBox = cloneViewBox(currentViewBox);
      viewport.classList.add('dragging');
      viewport.setPointerCapture(event.pointerId);
    });

    viewport.addEventListener('pointermove', (event) => {
      if (dragPointerId !== event.pointerId || dragStartViewBox === null) {
        return;
      }

      const { rect } = getViewportAnchor(event.clientX, event.clientY);
      const deltaX = event.clientX - dragStartX;
      const deltaY = event.clientY - dragStartY;
      currentViewBox = createViewBox(
        dragStartViewBox.minX - ((deltaX * dragStartViewBox.width) / rect.width),
        dragStartViewBox.minY - ((deltaY * dragStartViewBox.height) / rect.height),
        dragStartViewBox.width,
        dragStartViewBox.height
      );
      applyViewBox();
    });

    function endDrag(event) {
      if (dragPointerId !== event.pointerId) {
        return;
      }

      dragPointerId = null;
      dragStartViewBox = null;
      viewport.classList.remove('dragging');
      viewport.releasePointerCapture(event.pointerId);
    }

    viewport.addEventListener('pointerup', endDrag);
    viewport.addEventListener('pointercancel', endDrag);
    viewport.addEventListener('dblclick', () => {
      resetView();
    });
    zoomIn.addEventListener('click', () => {
      zoomBy(1.2);
    });
    zoomOut.addEventListener('click', () => {
      zoomBy(1 / 1.2);
    });
    resetViewButton.addEventListener('click', () => {
      resetView();
    });

    zoomValue.textContent = '100%';
    void loadSvg().catch((error) => {
      status.textContent = error instanceof Error ? error.message : String(error);
    });
  </script>
</body>
</html>`;
}

function renderPreviewHtml(svgPath: string, artifactUrl: string): string {
  const title = `${path.basename(svgPath, ".svg")} Preview`;

  return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>${escapeHtml(title)}</title>
  <style>
    * { box-sizing: border-box; }

    html, body {
      margin: 0;
      width: 100%;
      height: 100%;
      overflow: hidden;
      background: transparent;
    }

    body {
      padding: 12px;
      display: grid;
      place-items: stretch;
    }

    .frame {
      width: 100%;
      height: 100%;
      display: grid;
      place-items: center;
    }

    .frame svg {
      width: 100%;
      height: 100%;
      max-width: none;
      max-height: none;
      display: block;
      background: rgba(255, 255, 255, 0.22);
    }
  </style>
</head>
<body>
  <div id="frame" class="frame" aria-label="${escapeHtml(title)}"></div>
  <script>
    const frame = document.getElementById('frame');
    const artifactUrl = ${JSON.stringify(artifactUrl)};

    async function loadSvg() {
      const response = await fetch(artifactUrl, { cache: 'no-store' });
      if (!response.ok) {
        throw new Error('Failed to load SVG: ' + String(response.status));
      }

      frame.innerHTML = await response.text();
      const svg = frame.querySelector('svg');
      if (!(svg instanceof SVGElement)) {
        throw new Error('The artifact is not a valid SVG.');
      }

      svg.setAttribute('width', '100%');
      svg.setAttribute('height', '100%');
      svg.setAttribute('preserveAspectRatio', 'xMidYMid meet');
      svg.style.width = '100%';
      svg.style.height = '100%';

      try {
        const contentBounds = svg.getBBox();
        if (contentBounds.width > 0 && contentBounds.height > 0) {
          const padding = Math.max(Math.max(contentBounds.width, contentBounds.height) * 0.08, 4);
          const minX = contentBounds.x - padding;
          const minY = contentBounds.y - padding;
          const width = contentBounds.width + padding * 2;
          const height = contentBounds.height + padding * 2;
          svg.setAttribute('viewBox', [minX, minY, width, height].join(' '));
        }
      } catch {
        // Keep the original viewBox when bounds calculation is unavailable.
      }
    }

    void loadSvg().catch(() => {
      frame.textContent = 'Preview unavailable';
    });
  </script>
</body>
</html>`;
}

function renderGalleryHtml(state: GalleryState): string {
  const cards = state.stages.map((stage) => renderStageCard(stage)).join("\n");

  return `<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>${escapeHtml(state.title)}</title>
  <style>
    :root {
      color-scheme: light;
      --panel: #fffaf2;
      --line: #d7cbbb;
      --ink: #241b12;
      --muted: #705f4f;
      --waiting: #a67f2d;
      --running: #0f6c7a;
      --completed: #2d7d46;
      --cached: #5a53b2;
      --failed: #a12b2b;
    }

    * { box-sizing: border-box; }

    body {
      margin: 0;
      font-family: "Iosevka Aile", "IBM Plex Sans", sans-serif;
      background:
        radial-gradient(circle at top left, rgba(255, 255, 255, 0.9), transparent 24%),
        linear-gradient(135deg, #f7f1e7, #eee1cf 48%, #e3d2bd);
      color: var(--ink);
      min-height: 100vh;
    }

    main {
      max-width: 1400px;
      margin: 0 auto;
      padding: 32px 24px 48px;
    }

    header {
      display: grid;
      gap: 8px;
      margin-bottom: 28px;
    }

    h1 {
      margin: 0;
      font-size: clamp(1.8rem, 3vw, 3rem);
      letter-spacing: -0.04em;
    }

    p {
      margin: 0;
      color: var(--muted);
      max-width: 70ch;
    }

    .meta {
      display: flex;
      flex-wrap: wrap;
      gap: 12px;
      margin-top: 8px;
      font-size: 0.95rem;
      color: var(--muted);
      align-items: center;
    }

    .meta-actions {
      margin-left: auto;
      display: flex;
      gap: 10px;
      align-items: center;
    }

    .gallery {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
      gap: 18px;
    }

    .meta button {
      border: 1px solid color-mix(in srgb, var(--failed) 24%, var(--line));
      border-radius: 999px;
      padding: 8px 14px;
      background: color-mix(in srgb, white 82%, #f6dfdf);
      color: var(--failed);
      font: inherit;
      cursor: pointer;
    }

    .meta button:disabled {
      opacity: 0.7;
      cursor: wait;
    }

    #serverStatus {
      font-size: 0.85rem;
    }

    .card {
      background: color-mix(in srgb, var(--panel) 90%, white);
      border: 1px solid var(--line);
      border-radius: 18px;
      overflow: hidden;
      box-shadow: 0 12px 30px rgba(50, 33, 16, 0.08);
      display: grid;
      grid-template-rows: auto minmax(340px, 1fr) 104px;
      min-height: 520px;
    }

    .card-head {
      padding: 14px 16px 10px;
      display: grid;
      gap: 10px;
      border-bottom: 1px solid rgba(113, 92, 67, 0.12);
    }

    .eyebrow {
      font-size: 0.75rem;
      text-transform: uppercase;
      letter-spacing: 0.14em;
      color: var(--muted);
    }

    .label-row {
      display: flex;
      justify-content: space-between;
      gap: 12px;
      align-items: baseline;
    }

    .label-row h2 {
      margin: 0;
      font-size: 1.1rem;
    }

    .badge {
      border-radius: 999px;
      padding: 4px 10px;
      font-size: 0.78rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 0.08em;
      color: white;
      white-space: nowrap;
    }

    .badge.waiting { background: var(--waiting); }
    .badge.running { background: var(--running); }
    .badge.completed { background: var(--completed); }
    .badge.cached { background: var(--cached); }
    .badge.failed { background: var(--failed); }

    .surface {
      position: relative;
      min-height: 340px;
      background:
        linear-gradient(135deg, rgba(255, 255, 255, 0.55), rgba(245, 231, 210, 0.9)),
        repeating-linear-gradient(45deg, rgba(92, 70, 44, 0.05), rgba(92, 70, 44, 0.05) 12px, transparent 12px, transparent 24px);
    }

    .preview-link,
    .placeholder,
    .preview {
      position: absolute;
      inset: 0;
      width: 100%;
      height: 100%;
      border: 0;
    }

    .preview-link {
      display: block;
      overflow: hidden;
      cursor: zoom-in;
    }

    .preview-frame {
      pointer-events: none;
      background: rgba(255, 255, 255, 0.72);
    }

    .placeholder {
      display: grid;
      place-items: center;
      text-align: center;
      padding: 20px;
      color: var(--muted);
      gap: 8px;
    }

    .placeholder strong {
      font-size: 1rem;
      color: var(--ink);
    }

    .placeholder.waiting strong::before { content: "Waiting"; }
    .placeholder.running strong::before { content: "Running"; }
    .placeholder.failed strong::before { content: "Failed"; }

    .card-foot {
      padding: 10px 16px 14px;
      border-top: 1px solid rgba(113, 92, 67, 0.12);
      display: grid;
      gap: 4px;
      font-size: 0.86rem;
      color: var(--muted);
      align-content: start;
      min-height: 0;
      background: rgba(255, 250, 244, 0.72);
    }

    .path-name {
      font-size: 0.74rem;
      font-weight: 700;
      letter-spacing: 0.06em;
      text-transform: uppercase;
      color: color-mix(in srgb, var(--muted) 82%, var(--ink));
      white-space: nowrap;
      overflow: hidden;
      text-overflow: ellipsis;
    }

    .path-value {
      min-height: 0;
    }

    code {
      display: block;
      font-family: "Iosevka", "IBM Plex Mono", monospace;
      font-size: 0.82rem;
      line-height: 1.18;
      overflow-wrap: anywhere;
    }
  </style>
</head>
<body>
  <main>
    <header>
      <h1>${escapeHtml(state.title)}</h1>
      <p>The gallery predeclares each pipeline stage. Cards start in waiting mode until their SVG exists, then update in place without a full page refresh.</p>
      <div class="meta">
        <span>project: ${escapeHtml(state.projectDir)}</span>
        <span id="updatedAt">updated: ${escapeHtml(state.updatedAt)}</span>
        <div class="meta-actions">
          <span id="serverStatus">server running</span>
          <button id="stopServer" type="button">Stop server</button>
        </div>
      </div>
    </header>
    <section id="gallery" class="gallery">
${cards}
    </section>
  </main>
  <script>
    const gallery = document.getElementById('gallery');
    const updatedAt = document.getElementById('updatedAt');
    const serverStatus = document.getElementById('serverStatus');
    const stopServerButton = document.getElementById('stopServer');
    let latestRevision = ${String(state.revision)};
    let refreshTimer = undefined;
    let serverStopped = false;

    function esc(value) {
      return String(value)
        .replaceAll('&', '&amp;')
        .replaceAll('<', '&lt;')
        .replaceAll('>', '&gt;')
        .replaceAll('"', '&quot;')
        .replaceAll("'", '&#39;');
    }

    function stageMarkup(stage) {
      const outputText = stage.outputPath ?? stage.svgPath ?? 'pending output path';
      const outputName = outputText === 'pending output path' ? outputText : outputText.split(/[\\/]/).at(-1);
      const previewMarkup = typeof stage.svgPath === 'string'
        ? '<iframe class="preview preview-frame" aria-label="' + esc(stage.label) + ' preview" src="' + esc('/preview?svg=' + encodeURIComponent(stage.svgPath) + '&v=' + String(Date.now())) + '" loading="lazy"></iframe><a class="preview-link" href="' + esc('/viewer?svg=' + encodeURIComponent(stage.svgPath)) + '" target="_blank" rel="noopener noreferrer" aria-label="Open ' + esc(stage.label) + ' in the zoom viewer"></a>'
        : '<div class="placeholder ' + esc(stage.status) + '"><strong></strong><span>' + esc(stage.message ?? 'SVG will appear here as soon as the file is created.') + '</span></div>';

      return [
        '<article class="card">',
        '  <div class="card-head">',
        '    <div class="eyebrow">' + esc(stage.stageKind) + '</div>',
        '    <div class="label-row">',
        '      <h2>' + esc(stage.label) + '</h2>',
        '      <span class="badge ' + esc(stage.status) + '">' + esc(stage.status) + '</span>',
        '    </div>',
        '  </div>',
        '  <div class="surface">',
        '    ' + previewMarkup,
        '  </div>',
        '  <div class="card-foot">',
        '    <div class="path-name">' + esc(outputName ?? 'pending output path') + '</div>',
        '    <div class="path-value"><code>' + esc(outputText) + '</code></div>',
        '  </div>',
        '</article>'
      ].join('\\n');
    }

    function isTerminalStatus(status) {
      return status === 'completed' || status === 'cached' || status === 'failed';
    }

    function isStableState(stages) {
      return stages.some((stage) => stage.status === 'failed')
        || stages.every((stage) => isTerminalStatus(stage.status));
    }

    function renderState(state) {
      if (!Array.isArray(state.stages)) {
        throw new Error('Gallery status payload is missing stages.');
      }

      if (typeof state.revision === 'number' && state.revision <= latestRevision) {
        return isStableState(state.stages);
      }

      latestRevision = typeof state.revision === 'number' ? state.revision : latestRevision;
      gallery.innerHTML = state.stages.map(stageMarkup).join('\\n');
      updatedAt.textContent = 'updated: ' + String(state.updatedAt ?? new Date().toLocaleTimeString());
      return isStableState(state.stages);
    }

    async function refresh() {
      const response = await fetch('/api/status?ts=' + String(Date.now()), { cache: 'no-store' });
      if (!response.ok) {
        throw new Error('Gallery status request failed: ' + String(response.status));
      }

      const state = await response.json();
      return renderState(state);
    }

    async function poll() {
      let shouldContinue = true;
      try {
        shouldContinue = !(await refresh());
      } catch (error) {
        if (!serverStopped) {
          console.error('Gallery refresh failed', error);
          if (serverStatus instanceof HTMLElement) {
            serverStatus.textContent = 'refresh error';
          }
        }
      } finally {
        if (shouldContinue && !serverStopped) {
          refreshTimer = window.setTimeout(() => {
            void poll();
          }, 700);
        }
      }
    }

    async function stopServer() {
      if (!(stopServerButton instanceof HTMLButtonElement)) {
        return;
      }

      stopServerButton.disabled = true;
      serverStopped = true;
      if (typeof refreshTimer === 'number') {
        window.clearTimeout(refreshTimer);
      }
      if (serverStatus instanceof HTMLElement) {
        serverStatus.textContent = 'stopping server…';
      }

      try {
        const response = await fetch('/api/stop', {
          method: 'POST',
          cache: 'no-store'
        });
        if (!response.ok) {
          throw new Error('Stop request failed: ' + String(response.status));
        }

        if (serverStatus instanceof HTMLElement) {
          serverStatus.textContent = 'server stopped';
        }
      } catch (error) {
        serverStopped = false;
        stopServerButton.disabled = false;
        if (serverStatus instanceof HTMLElement) {
          serverStatus.textContent = error instanceof Error ? error.message : String(error);
        }
      }
    }

    if (stopServerButton instanceof HTMLButtonElement) {
      stopServerButton.addEventListener('click', () => {
        void stopServer();
      });
    }

    void poll();
  </script>
</body>
</html>`;
}

function markUpdated(state: GalleryState): void {
  state.revision += 1;
  state.updatedAt = new Date().toLocaleTimeString();
}

function collectCandidateHosts(): string[] {
  const configured = process.env["LABYNODEJS_GALLERY_HOST"];
  const candidates = configured === undefined || configured.trim() === ""
    ? []
    : [configured.trim()];

  const interfaces = os.networkInterfaces();
  for (const entries of Object.values(interfaces)) {
    for (const entry of entries ?? []) {
      if (entry.family === "IPv4" && !entry.internal) {
        candidates.push(entry.address);
      }
    }
  }

  candidates.push("127.0.0.1");
  return [...new Set(candidates)];
}

function selectBrowserHost(): string {
  return collectCandidateHosts()[0] ?? "127.0.0.1";
}

function sendHtml(response: ServerResponse, html: string): void {
  response.writeHead(200, {
    "content-type": "text/html; charset=utf-8",
    "cache-control": "no-store"
  });
  response.end(html);
}

function sendJson(response: ServerResponse, value: unknown): void {
  response.writeHead(200, {
    "content-type": "application/json; charset=utf-8",
    "cache-control": "no-store"
  });
  response.end(`${JSON.stringify(value)}\n`);
}

function sendNotFound(response: ServerResponse): void {
  response.writeHead(404, {
    "content-type": "text/plain; charset=utf-8",
    "cache-control": "no-store"
  });
  response.end("Not found\n");
}

async function sendArtifact(response: ServerResponse, filePath: string): Promise<void> {
  response.writeHead(200, {
    "content-type": "image/svg+xml; charset=utf-8",
    "cache-control": "no-store"
  });

  const data = await readFile(filePath, "utf8");
  response.end(data);
}

function openBrowser(url: string): void {
  const browser = process.env["BROWSER"];
  if (browser === undefined || browser.trim() === "") {
    return;
  }

  const child = spawn(browser, [url], {
    detached: true,
    stdio: "ignore"
  });
  child.unref();
}

export async function startPipelineGallery(
  projectDir: string,
  stages: readonly PipelineStage[],
  options: {
    title?: string;
    port?: number;
    openBrowser?: boolean;
    keepAlive?: boolean;
  } = {}
): Promise<PipelineGalleryHandle> {
  const state: GalleryState = {
    title: options.title ?? "LabyNodeJS Pipeline Gallery",
    projectDir,
    revision: 0,
    updatedAt: new Date().toLocaleTimeString(),
    stages: stages.map((stage, index) => {
      const base: GalleryStageSnapshot = {
        index,
        stageKind: stage.kind,
        label: stageLabel(stage, index),
        status: stage.kind === "source" ? "completed" : "waiting",
        message: stage.kind === "source" ? "Source SVG is available." : "Waiting for stage execution."
      };

      if (stage.kind === "source") {
        return {
          ...base,
          svgPath: stage.sourcePath,
          outputPath: stage.sourcePath
        };
      }

      return base;
    })
  };

  let closed = false;

  const closeServer = async (): Promise<void> => {
    if (closed) {
      return;
    }

    closed = true;
    await new Promise((resolve, reject) => {
      server.close((error) => {
        if (error !== undefined) {
          reject(error);
          return;
        }

        resolve(undefined);
      });
    });
  };

  const handleRequest = async (request: IncomingMessage, response: ServerResponse): Promise<void> => {
    try {
      const requestUrl = new URL(request.url ?? "/", "http://gallery.local");

      if (requestUrl.pathname === "/") {
        sendHtml(response, renderGalleryHtml(state));
        return;
      }

      if (requestUrl.pathname === "/api/status") {
        sendJson(response, {
          title: state.title,
          projectDir: state.projectDir,
          revision: state.revision,
          updatedAt: state.updatedAt,
          stable: isStableState(state.stages),
          stages: state.stages.map((stage) => ({
            ...stage,
            svgPath: inferSvgPath(stage)
          }))
        });
        return;
      }

      if (requestUrl.pathname === "/api/stop" && request.method === "POST") {
        response.once("finish", () => {
          void closeServer();
        });
        sendJson(response, {
          stopping: true
        });
        return;
      }

      if (requestUrl.pathname === "/artifact") {
        const filePath = resolveQueriedSvgPath(requestUrl);

        if (filePath === undefined) {
          sendNotFound(response);
          return;
        }

        await sendArtifact(response, filePath);
        return;
      }

      if (requestUrl.pathname === "/preview") {
        const filePath = resolveQueriedSvgPath(requestUrl);

        if (filePath === undefined) {
          sendNotFound(response);
          return;
        }

        sendHtml(response, renderPreviewHtml(filePath, buildArtifactUrl(filePath, state.revision)));
        return;
      }

      if (requestUrl.pathname === "/viewer") {
        const filePath = resolveQueriedSvgPath(requestUrl);
        const stage = filePath === undefined ? undefined : findStageForSvgPath(state.stages, filePath);

        if (filePath === undefined) {
          sendNotFound(response);
          return;
        }

        sendHtml(
          response,
          renderViewerHtml(
            stage ?? {
              index: -1,
              stageKind: "source",
              label: path.basename(filePath, ".svg"),
              status: "completed",
              message: "SVG is available.",
              svgPath: filePath,
              outputPath: filePath
            },
            buildArtifactUrl(filePath, state.revision)
          )
        );
        return;
      }

      sendNotFound(response);
    } catch {
      sendNotFound(response);
    }
  };

  const server = createServer((request, response) => {
    void handleRequest(request, response);
  });

  await new Promise((resolve, reject) => {
    server.once("error", reject);
    server.listen(options.port ?? 0, "0.0.0.0", () => {
      server.off("error", reject);
      resolve(undefined);
    });
  });

  const keepAlive = options.keepAlive ?? options.openBrowser ?? false;
  if (!keepAlive) {
    server.unref();
  }

  const address = server.address();
  if (address === null || typeof address === "string") {
    throw new Error("Failed to resolve gallery server address.");
  }

  const browserHost = selectBrowserHost();
  const port = address.port;
  const url = `http://${browserHost}:${String(port)}/`;

  if (options.openBrowser) {
    openBrowser(url);
  }

  return {
    url,
    updateStage(index, update) {
      const current = state.stages[index];
      if (current === undefined) {
        return;
      }

      state.stages[index] = {
        ...current,
        ...update
      };
      markUpdated(state);
    },
    failStage(index, message) {
      const current = state.stages[index];
      if (current === undefined) {
        return;
      }

      state.stages[index] = {
        ...current,
        status: "failed",
        message
      };
      markUpdated(state);
    },
    async close() {
      await closeServer();
    }
  };
}