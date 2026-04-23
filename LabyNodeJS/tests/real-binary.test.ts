import assert from "node:assert/strict";
import { access, mkdtemp, rm } from "node:fs/promises";
import path from "node:path";
import test from "node:test";
import { resolveContext } from "../src/context.js";
import { runPipeline } from "../src/runner.js";
import { grid, pipeline, source } from "../src/stages.js";
import { gridConfigFixture } from "./fixtures.js";

const packageRoot = path.resolve(
  path.dirname(new URL(import.meta.url).pathname),
  "..",
);
const workspaceRoot = path.resolve(packageRoot, "..");

void test(
  "runPipeline can invoke the real labypath binary when available",
  { timeout: 120000 },
  async (t) => {
    const context = resolveContext(workspaceRoot);
    const sourcePath = path.join(
      workspaceRoot,
      "LabyData",
      "svg",
      "square_circleorig.svg",
    );

    if (!context.binaryExists) {
      t.skip(`labypath binary not available at ${context.binaryPath}`);
      return;
    }

    await access(sourcePath);
    const projectDir = await mkdtemp(path.join(packageRoot, ".tmp-real-"));
    t.after(async () => rm(projectDir, { recursive: true, force: true }));

    const firstRun = await runPipeline(
      pipeline(source(sourcePath), grid(gridConfigFixture)),
      {
        binaryPath: context.binaryPath,
        projectDir,
        workspaceRoot,
        gallery: {
          enabled: false,
          openBrowser: false,
          keepAlive: false,
          port: 0,
          title: "Real binary test gallery",
        },
      },
    );
    const secondRun = await runPipeline(
      pipeline(source(sourcePath), grid(gridConfigFixture)),
      {
        binaryPath: context.binaryPath,
        projectDir,
        workspaceRoot,
        gallery: {
          enabled: false,
          openBrowser: false,
          keepAlive: false,
          port: 0,
          title: "Real binary test gallery",
        },
      },
    );

    const firstGridStage = firstRun.stages[1];
    const secondGridStage = secondRun.stages[1];
    assert.ok(firstGridStage);
    assert.ok(secondGridStage);

    assert.equal(firstGridStage.cached, false);
    assert.equal(secondGridStage.cached, true);
    await access(firstGridStage.outputPath);
  },
);
