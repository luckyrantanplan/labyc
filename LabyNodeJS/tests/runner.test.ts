import assert from "node:assert/strict";
import { chmod, mkdir, readdir, readFile, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { loadCacheManifest } from "../src/cache.js";
import {
  grid,
  noise,
  pipeline,
  render,
  runPipeline,
  source,
  streamline,
  type PipelineGalleryOptions,
} from "../src/index.js";
import {
  gridConfigFixture,
  noiseConfigFixture,
  renderConfigFixture,
  streamLineConfigFixture,
} from "./fixtures.js";

function createGalleryOptions(
  overrides: Partial<PipelineGalleryOptions> = {},
): PipelineGalleryOptions {
  return {
    enabled: false,
    openBrowser: false,
    keepAlive: false,
    port: 0,
    title: "Runner test gallery",
    ...overrides,
  };
}

async function makeTempWorkspace(): Promise<string> {
  const workspaceRoot = path.join(
    os.tmpdir(),
    `labynodejs-${String(Date.now())}-${Math.random().toString(16).slice(2)}`,
  );
  await mkdir(workspaceRoot, { recursive: true });
  await mkdir(path.join(workspaceRoot, "LabyPath", "input"), {
    recursive: true,
  });
  await mkdir(path.join(workspaceRoot, "LabyData", "svg"), { recursive: true });
  await mkdir(path.join(workspaceRoot, "LabyPython"), { recursive: true });
  return workspaceRoot;
}

async function writeFakeBinary(workspaceRoot: string): Promise<string> {
  const binaryPath = path.join(workspaceRoot, "fake-labypath.mjs");
  const script = `#!/usr/bin/env node
import { readFileSync, writeFileSync, existsSync } from "node:fs";
const configPath = process.argv[2];
const config = JSON.parse(readFileSync(configPath, "utf8"));
const noiseStage = config.hqNoise;
const streamLineStage = config.streamLine;
const stage = noiseStage?.filepaths ?? config.skeletonGrid ?? config.routing?.filepaths ?? streamLineStage?.filepaths ?? config.gGraphicRendering;
const inputPath = streamLineStage?.filepaths?.inputfile ?? stage.inputfile;
const outputPath = stage.outputfile;
const markerPath = outputPath + ".count";
const current = existsSync(markerPath) ? Number(readFileSync(markerPath, "utf8")) : 0;
writeFileSync(markerPath, String(current + 1));
if (noiseStage) {
  writeFileSync(outputPath, Buffer.from([1, 2, 3, 4]));
  writeFileSync(noiseStage.previewFile, '<svg xmlns="http://www.w3.org/2000/svg"><line x1="0" y1="0" x2="10" y2="10" stroke="red"/></svg>\\n');
  process.exit(0);
}

const input = readFileSync(inputPath, streamLineStage ? null : "utf8");
const svg = '<svg xmlns="http://www.w3.org/2000/svg"><path d="M0 0 L10 10" /></svg>\\n';
writeFileSync(outputPath, streamLineStage ? svg : String(input) + "\\n<!-- generated -->\\n");
`;
  await writeFile(binaryPath, script, "utf8");
  await chmod(binaryPath, 0o755);
  return binaryPath;
}

async function writeBrokenBinary(workspaceRoot: string): Promise<string> {
  const binaryPath = path.join(workspaceRoot, "broken-labypath.mjs");
  const script = `#!/usr/bin/env node
process.exit(0);
`;
  await writeFile(binaryPath, script, "utf8");
  await chmod(binaryPath, 0o755);
  return binaryPath;
}

async function writeFailingBinary(workspaceRoot: string): Promise<string> {
  const binaryPath = path.join(workspaceRoot, "failing-labypath.mjs");
  const script = `#!/usr/bin/env node
console.error("simulated failure");
process.exit(2);
`;
  await writeFile(binaryPath, script, "utf8");
  await chmod(binaryPath, 0o755);
  return binaryPath;
}

void test("runPipeline rejects an empty pipeline", async () => {
  await assert.rejects(
    async () => runPipeline([], { gallery: createGalleryOptions() }),
    /must contain at least one stage/,
  );
});

void test("runPipeline requires a source as the first stage", async () => {
  await assert.rejects(
    async () =>
      runPipeline([grid(gridConfigFixture)], {
        gallery: createGalleryOptions(),
      }),
    /must start with a source or noise stage/,
  );
});

void test("runPipeline rejects missing binaries before execution", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  await assert.rejects(
    async () =>
      runPipeline(pipeline(source(sourcePath), grid(gridConfigFixture)), {
        binaryPath: path.join(workspaceRoot, "missing-binary"),
        projectDir,
        workspaceRoot,
        gallery: createGalleryOptions(),
      }),
    /Cannot find labypath binary/,
  );
});

void test("runPipeline rejects source stages after the first stage", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFakeBinary(workspaceRoot);
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  await assert.rejects(
    async () =>
      runPipeline(pipeline(source(sourcePath), source(sourcePath)), {
        binaryPath,
        projectDir,
        workspaceRoot,
        gallery: createGalleryOptions(),
      }),
    /Only the first stage can be a source or noise stage/,
  );
});

void test("runPipeline accepts a noise stage as the first stage and feeds streamline", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const binaryPath = await writeFakeBinary(workspaceRoot);

  const firstRun = await runPipeline(
    pipeline(noise(noiseConfigFixture), streamline(streamLineConfigFixture)),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: createGalleryOptions({ enabled: true }),
    },
  );
  const secondRun = await runPipeline(
    pipeline(noise(noiseConfigFixture), streamline(streamLineConfigFixture)),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: createGalleryOptions(),
    },
  );

  const firstNoiseStage = firstRun.stages[0];
  const firstStreamLineStage = firstRun.stages[1];
  const secondNoiseStage = secondRun.stages[0];
  const secondStreamLineStage = secondRun.stages[1];
  assert.ok(firstNoiseStage);
  assert.ok(firstStreamLineStage);
  assert.ok(secondNoiseStage);
  assert.ok(secondStreamLineStage);

  assert.equal(firstNoiseStage.stageKind, "noise");
  assert.equal(firstNoiseStage.cached, false);
  assert.equal(firstStreamLineStage.stageKind, "streamline");
  assert.equal(firstStreamLineStage.cached, false);
  assert.equal(secondNoiseStage.cached, true);
  assert.equal(secondStreamLineStage.cached, true);
  assert.match(firstRun.galleryUrl ?? "", /^http:\/\/[^/]+:\d+\/$/);
});

void test("runPipeline fails when the executable does not create an output file", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeBrokenBinary(workspaceRoot);
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  await assert.rejects(
    async () =>
      runPipeline(pipeline(source(sourcePath), grid(gridConfigFixture)), {
        binaryPath,
        projectDir,
        workspaceRoot,
        gallery: createGalleryOptions(),
      }),
    /Expected stage output was not created/,
  );
});

void test("runPipeline surfaces stage execution failures and still exercises gallery failure updates", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFailingBinary(workspaceRoot);
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  await assert.rejects(
    async () =>
      runPipeline(pipeline(source(sourcePath), grid(gridConfigFixture)), {
        binaryPath,
        projectDir,
        workspaceRoot,
        gallery: createGalleryOptions({
          enabled: true,
          title: "Failing runner test gallery",
        }),
      }),
    /Command failed|simulated failure/,
  );

  const logFiles = await readdir(path.join(projectDir, "logs"));
  const runLogName = logFiles.find((entry) => entry.startsWith("pipeline-"));
  assert.ok(runLogName);
  const runLog = await readFile(
    path.join(projectDir, "logs", runLogName),
    "utf8",
  );
  assert.match(runLog, /Pipeline failed/);
  assert.match(runLog, /simulated failure/);
});

void test("runPipeline writes outputs, updates cache, and reuses cached entries", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFakeBinary(workspaceRoot);
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  const firstRun = await runPipeline(
    pipeline(
      source(sourcePath),
      grid(gridConfigFixture),
      render(renderConfigFixture),
    ),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: createGalleryOptions({ enabled: true }),
    },
  );
  const secondRun = await runPipeline(
    pipeline(
      source(sourcePath),
      grid(gridConfigFixture),
      render(renderConfigFixture),
    ),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: createGalleryOptions(),
    },
  );

  assert.equal(firstRun.stages[1]?.cached, false);
  assert.equal(firstRun.stages[2]?.cached, false);
  assert.equal(secondRun.stages[1]?.cached, true);
  assert.equal(secondRun.stages[2]?.cached, true);
  assert.match(firstRun.galleryUrl ?? "", /^http:\/\/[^/]+:\d+\/$/);

  const finalStage = secondRun.stages[2];
  assert.ok(finalStage);
  const finalOutputPath = finalStage.outputPath;

  const markerPath = `${finalOutputPath}.count`;
  const executions = await readFile(markerPath, "utf8");
  assert.equal(executions, "1");

  const manifest = await loadCacheManifest(firstRun.cachePath);
  assert.equal(Object.keys(manifest.entries).length, 2);
  assert.ok(firstRun.durationMs >= 0);
  assert.ok(firstRun.runLogPath.endsWith(".log"));
  const firstGridStage = firstRun.stages[1];
  const firstRenderStage = firstRun.stages[2];
  const secondGridStage = secondRun.stages[1];
  assert.ok(firstGridStage);
  assert.ok(firstRenderStage);
  assert.ok(secondGridStage);
  assert.ok(firstGridStage.durationMs !== undefined);
  assert.ok(firstRenderStage.executionDurationMs !== undefined);
  assert.equal(secondGridStage.cacheReason, "cache entry is valid");

  const firstRunLog = await readFile(firstRun.runLogPath, "utf8");
  const secondRunLog = await readFile(secondRun.runLogPath, "utf8");
  assert.match(firstRunLog, /Pipeline started/);
  assert.match(firstRunLog, /Stage completed/);
  assert.match(secondRunLog, /Stage reused cached output/);
});

void test("runPipeline invalidates cache when stage config changes", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFakeBinary(workspaceRoot);
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  const initialRun = await runPipeline(
    pipeline(source(sourcePath), grid(gridConfigFixture)),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: createGalleryOptions(),
    },
  );
  const changedRun = await runPipeline(
    pipeline(
      source(sourcePath),
      grid({
        ...gridConfigFixture,
        seed: 99,
      }),
    ),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: createGalleryOptions(),
    },
  );

  assert.notEqual(
    initialRun.stages[1]?.cacheKey,
    changedRun.stages[1]?.cacheKey,
  );
  assert.equal(changedRun.stages[1]?.cached, false);
});

void test("runPipeline fails when the executable creates an SVG with no drawable geometry", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = path.join(workspaceRoot, "empty-route-labypath.mjs");
  const script = `#!/usr/bin/env node
import { readFileSync, writeFileSync } from "node:fs";
const configPath = process.argv[2];
const config = JSON.parse(readFileSync(configPath, "utf8"));
const outputPath = config.gGraphicRendering.outputfile;
  writeFileSync(outputPath, '<svg xmlns="http://www.w3.org/2000/svg"><path d="" /></svg>\\n');
`;
  await writeFile(binaryPath, script, "utf8");
  await chmod(binaryPath, 0o755);
  await writeFile(
    sourcePath,
    '<svg xmlns="http://www.w3.org/2000/svg"><rect width="10" height="10" /></svg>\n',
    "utf8",
  );

  await assert.rejects(
    async () =>
      runPipeline(pipeline(source(sourcePath), render(renderConfigFixture)), {
        binaryPath,
        projectDir,
        workspaceRoot,
        gallery: createGalleryOptions(),
      }),
    /Expected stage output to contain drawable SVG content/,
  );
});
