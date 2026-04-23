import assert from "node:assert/strict";
import { mkdtemp, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { startPipelineGallery } from "../src/gallery.js";
import { source, grid } from "../src/stages.js";
import { gridConfigFixture } from "./fixtures.js";

void test("gallery page exposes waiting placeholders and serves SVG previews as they appear", async () => {
  const projectDir = await mkdtemp(
    path.join(os.tmpdir(), "labynodejs-gallery-"),
  );
  const sourcePath = path.join(projectDir, "source.svg");
  const outputPath = path.join(projectDir, "grid.svg");
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"></svg>\n',
    "utf8",
  );

  const gallery = await startPipelineGallery(
    projectDir,
    [
      source(sourcePath, { label: "Source" }),
      grid(gridConfigFixture, { label: "Grid" }),
    ],
    {
      enabled: true,
      openBrowser: false,
      keepAlive: false,
      port: 0,
      title: "Gallery Test",
    },
  );

  try {
    const initialResponse = await fetch(gallery.url);
    const initialHtml = await initialResponse.text();
    assert.match(initialHtml, /Gallery Test/);
    assert.match(initialHtml, /Waiting/);
    assert.match(initialHtml, /Source/);
    assert.match(initialHtml, /Grid/);

    const waitingResponse = await fetch(new URL("/api/status", gallery.url));
    const waitingState = (await waitingResponse.json()) as {
      stable: boolean;
      stages: { status: string; svgPath?: string }[];
    };
    const waitingGridStage = waitingState.stages[1];
    assert.ok(waitingGridStage);
    assert.equal(waitingState.stable, false);
    assert.equal(waitingGridStage.status, "waiting");
    assert.equal(waitingGridStage.svgPath, undefined);

    await writeFile(
      outputPath,
      '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
      "utf8",
    );
    gallery.updateStage(1, {
      status: "completed",
      svgPath: outputPath,
      outputPath,
      message: "SVG created.",
    });

    const readyResponse = await fetch(new URL("/api/status", gallery.url));
    const readyState = (await readyResponse.json()) as {
      stable: boolean;
      stages: { status: string; svgPath?: string }[];
    };
    const readyGridStage = readyState.stages[1];
    assert.ok(readyGridStage);
    assert.equal(readyState.stable, true);
    assert.equal(readyGridStage.status, "completed");
    assert.equal(readyGridStage.svgPath, outputPath);

    const readyHtmlResponse = await fetch(gallery.url);
    const readyHtml = await readyHtmlResponse.text();
    assert.match(readyHtml, /\/viewer\?svg=/);
    assert.match(readyHtml, /\/preview\?svg=/);
    assert.doesNotMatch(readyHtml, /\/viewer\//);

    const artifactResponse = await fetch(
      new URL(`/artifact?svg=${encodeURIComponent(outputPath)}`, gallery.url),
    );
    const artifactText = await artifactResponse.text();
    assert.match(artifactText, /<rect/);

    const previewResponse = await fetch(
      new URL(`/preview?svg=${encodeURIComponent(outputPath)}`, gallery.url),
    );
    const previewHtml = await previewResponse.text();
    assert.match(previewHtml, /Preview/);
    assert.match(previewHtml, /\/artifact\?svg=/);

    const viewerResponse = await fetch(
      new URL(`/viewer?svg=${encodeURIComponent(outputPath)}`, gallery.url),
    );
    const viewerHtml = await viewerResponse.text();
    assert.match(viewerHtml, /Grid Viewer/);
    assert.match(viewerHtml, /Drag to pan\. Wheel to zoom\./);
    assert.match(viewerHtml, /\/artifact\?svg=/);
  } finally {
    await gallery.close();
  }
});

void test("gallery tolerates unknown stage updates and allows repeated close", async () => {
  const projectDir = await mkdtemp(
    path.join(os.tmpdir(), "labynodejs-gallery-"),
  );
  const sourcePath = path.join(projectDir, "source.svg");
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"></svg>\n',
    "utf8",
  );

  const gallery = await startPipelineGallery(
    projectDir,
    [source(sourcePath, { label: "Source" })],
    {
      enabled: true,
      openBrowser: false,
      keepAlive: true,
      port: 0,
      title: "Gallery Test",
    },
  );

  gallery.updateStage(99, { status: "completed" });
  gallery.failStage(99, "ignored");

  const htmlResponse = await fetch(gallery.url);
  const html = await htmlResponse.text();
  assert.match(html, /Stop server/);

  const stopResponse = await fetch(new URL("/api/stop", gallery.url), {
    method: "POST",
  });
  const stopState = (await stopResponse.json()) as { stopping: boolean };
  assert.equal(stopState.stopping, true);

  await assert.rejects(async () => fetch(gallery.url), /fetch failed/i);
  await gallery.close();
});
