import assert from "node:assert/strict";
import { chmod, mkdir, readFile, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { loadCacheManifest } from "../src/cache.js";
import { grid, pipeline, render, runPipeline, source } from "../src/index.js";
import { gridConfigFixture, renderConfigFixture } from "./fixtures.js";

async function makeTempWorkspace(): Promise<string> {
  const workspaceRoot = path.join(os.tmpdir(), `labynodejs-${String(Date.now())}-${Math.random().toString(16).slice(2)}`);
  await mkdir(workspaceRoot, { recursive: true });
  await mkdir(path.join(workspaceRoot, "LabyPath", "input"), { recursive: true });
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
const stage = config.skeletonGrid ?? config.routing?.filepaths ?? config.gGraphicRendering;
const inputPath = stage.inputfile;
const outputPath = stage.outputfile;
const markerPath = outputPath + ".count";
const current = existsSync(markerPath) ? Number(readFileSync(markerPath, "utf8")) : 0;
writeFileSync(markerPath, String(current + 1));
const input = readFileSync(inputPath, "utf8");
writeFileSync(outputPath, input + "\\n<!-- generated -->\\n");
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
    async () => runPipeline([], {}),
    /must contain at least one stage/
  );
});

void test("runPipeline requires a source as the first stage", async () => {
  await assert.rejects(
    async () => runPipeline([grid(gridConfigFixture)], {}),
    /must start with a source stage/
  );
});

void test("runPipeline rejects missing binaries before execution", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  await writeFile(sourcePath, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n", "utf8");

  await assert.rejects(
    async () => runPipeline(
      pipeline(source(sourcePath), grid(gridConfigFixture)),
      {
        binaryPath: path.join(workspaceRoot, "missing-binary"),
        projectDir,
        workspaceRoot
      }
    ),
    /Cannot find labypath binary/
  );
});

void test("runPipeline rejects source stages after the first stage", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFakeBinary(workspaceRoot);
  await writeFile(sourcePath, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n", "utf8");

  await assert.rejects(
    async () => runPipeline(
      pipeline(source(sourcePath), source(sourcePath)),
      { binaryPath, projectDir, workspaceRoot }
    ),
    /Only the first stage can be a source stage/
  );
});

void test("runPipeline fails when the executable does not create an output file", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeBrokenBinary(workspaceRoot);
  await writeFile(sourcePath, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n", "utf8");

  await assert.rejects(
    async () => runPipeline(
      pipeline(source(sourcePath), grid(gridConfigFixture)),
      { binaryPath, projectDir, workspaceRoot }
    ),
    /Expected stage output was not created/
  );
});

void test("runPipeline surfaces stage execution failures and still exercises gallery failure updates", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFailingBinary(workspaceRoot);
  await writeFile(sourcePath, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n", "utf8");

  await assert.rejects(
    async () => runPipeline(
      pipeline(source(sourcePath), grid(gridConfigFixture)),
      {
        binaryPath,
        projectDir,
        workspaceRoot,
        gallery: {
          enabled: true,
          openBrowser: false,
          title: "Failing runner test gallery"
        }
      }
    ),
    /Command failed|simulated failure/
  );
});

void test("runPipeline writes outputs, updates cache, and reuses cached entries", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFakeBinary(workspaceRoot);
  await writeFile(sourcePath, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n", "utf8");

  const firstRun = await runPipeline(
    pipeline(source(sourcePath), grid(gridConfigFixture), render(renderConfigFixture)),
    {
      binaryPath,
      projectDir,
      workspaceRoot,
      gallery: {
        enabled: true,
        openBrowser: false,
        title: "Runner test gallery"
      }
    }
  );
  const secondRun = await runPipeline(
    pipeline(source(sourcePath), grid(gridConfigFixture), render(renderConfigFixture)),
    { binaryPath, projectDir, workspaceRoot }
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
});

void test("runPipeline invalidates cache when stage config changes", async () => {
  const workspaceRoot = await makeTempWorkspace();
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "source.svg");
  const binaryPath = await writeFakeBinary(workspaceRoot);
  await writeFile(sourcePath, "<svg xmlns=\"http://www.w3.org/2000/svg\"></svg>\n", "utf8");

  const initialRun = await runPipeline(
    pipeline(source(sourcePath), grid(gridConfigFixture)),
    { binaryPath, projectDir, workspaceRoot }
  );
  const changedRun = await runPipeline(
    pipeline(source(sourcePath), grid({
      ...gridConfigFixture,
      seed: 99
    })),
    { binaryPath, projectDir, workspaceRoot }
  );

  assert.notEqual(initialRun.stages[1]?.cacheKey, changedRun.stages[1]?.cacheKey);
  assert.equal(changedRun.stages[1]?.cached, false);
});