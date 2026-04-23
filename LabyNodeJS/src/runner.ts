import { spawn } from "node:child_process";
import { createWriteStream, existsSync } from "node:fs";
import { mkdir, readFile, writeFile } from "node:fs/promises";
import path from "node:path";
import {
  ensureCacheLayout,
  loadCacheManifest,
  saveCacheManifest,
  setCacheEntry,
} from "./cache.js";
import {
  requireSvgPath,
  requireWorkspacePath,
  resolveContext,
} from "./context.js";
import { startPipelineGallery } from "./gallery.js";
import { hashFile, hashString, stableStringify } from "./hash.js";
import { createRunLogger, formatDuration } from "./logging.js";
import { artifactStem, buildStagePayload } from "./payload.js";
import type {
  CacheEntry,
  PipelineRunResult,
  PipelineStage,
  RunPipelineOptions,
  SourceStage,
  StageArtifact,
  TransformerStage,
} from "./types.js";

function ensurePipelineShape(
  stages: readonly PipelineStage[],
): asserts stages is readonly [SourceStage, ...PipelineStage[]] {
  if (stages.length === 0) {
    throw new Error("Pipeline must contain at least one stage.");
  }

  if (stages[0]?.kind !== "source") {
    throw new Error("Pipeline must start with a source stage.");
  }
}

function resolveStagePaths(
  svgDir: string,
  inputPath: string,
  stageKind: TransformerStage["kind"],
  cacheKey: string,
  configDir: string,
  logDir: string,
) {
  const stem = artifactStem(inputPath, stageKind, cacheKey);
  return {
    configPath: path.join(configDir, `${stem}.json`),
    logPath: path.join(logDir, `${stem}.log`),
    outputPath: path.join(svgDir, `${stem}.svg`),
  };
}

async function readOutputHashIfPresent(
  outputPath: string,
): Promise<string | undefined> {
  if (!existsSync(outputPath)) {
    return undefined;
  }

  return hashFile(outputPath);
}

function hasDrawableSvgContent(svgText: string): boolean {
  if (!svgText.includes("<svg")) {
    return false;
  }

  const drawablePatterns = [
    /<path\b[^>]*\bd\s*=\s*(["'])\s*[^"'\s][^"']*\1/iu,
    /<(?:circle|ellipse|rect|line)\b/iu,
    /<(?:polyline|polygon)\b[^>]*\bpoints\s*=\s*(["'])\s*[^"'\s][^"']*\1/iu,
  ];

  return drawablePatterns.some((pattern) => pattern.test(svgText));
}

async function hasUsableSvgOutput(outputPath: string): Promise<boolean> {
  if (!existsSync(outputPath)) {
    return false;
  }

  return hasDrawableSvgContent(await readFile(outputPath, "utf8"));
}

async function executeStage(
  binaryPath: string,
  workspaceRoot: string,
  configPath: string,
  logPath: string,
): Promise<{ durationMs: number }> {
  const startedAt = new Date().toISOString();
  const startedAtMs = Date.now();
  const commandLine = `${binaryPath} ${configPath}`;

  await mkdir(path.dirname(logPath), { recursive: true });
  const logStream = createWriteStream(logPath, {
    encoding: "utf8",
    flags: "w",
  });
  logStream.write(`started at ${startedAt}\ncommand: ${commandLine}\n`);

  const child = spawn(binaryPath, [configPath], {
    cwd: workspaceRoot,
    stdio: ["ignore", "pipe", "pipe"],
  });

  let combinedOutput = "";

  const appendChunk = (chunk: Buffer | string): void => {
    const text = chunk.toString();
    combinedOutput += text;
    logStream.write(text);
  };

  child.stdout.on("data", appendChunk);
  child.stderr.on("data", appendChunk);

  const exitCode = await new Promise<number>((resolve, reject) => {
    child.once("error", reject);
    child.once("close", (code, signal) => {
      if (signal !== null) {
        reject(new Error(`Command terminated by signal ${signal}`));
        return;
      }

      resolve(code ?? 1);
    });
  });

  const durationMs = Date.now() - startedAtMs;
  logStream.write(
    `\nexit code: ${String(exitCode)}\nduration: ${formatDuration(durationMs)}\n`,
  );
  await new Promise<void>((resolve) => {
    logStream.end(() => {
      resolve();
    });
  });

  if (exitCode !== 0) {
    const output = combinedOutput.trim();
    throw new Error(
      output === ""
        ? `Command failed: ${commandLine} (exit code ${String(exitCode)})`
        : output,
    );
  }

  return { durationMs };
}

async function resolveCacheDecision(
  force: boolean | undefined,
  entry: CacheEntry | undefined,
  binaryFingerprint: string,
  outputHash: string | undefined,
): Promise<{ cached: boolean; reason: string }> {
  if (force) {
    return {
      cached: false,
      reason: "force=true bypassed cache",
    };
  }

  if (entry === undefined) {
    return {
      cached: false,
      reason: "no cache entry",
    };
  }

  if (entry.binaryFingerprint !== binaryFingerprint) {
    return {
      cached: false,
      reason: "binary fingerprint changed",
    };
  }

  if (outputHash === undefined) {
    return {
      cached: false,
      reason: "cached output file is missing",
    };
  }

  if (outputHash !== entry.outputHash) {
    return {
      cached: false,
      reason: "cached output hash changed",
    };
  }

  if (!(await hasUsableSvgOutput(entry.outputPath))) {
    return {
      cached: false,
      reason: "cached SVG is not drawable",
    };
  }

  return {
    cached: true,
    reason: "cache entry is valid",
  };
}

function createStageArtifact(
  base: Omit<StageArtifact, "label">,
  label: string | undefined,
): StageArtifact {
  if (label === undefined) {
    return base;
  }

  return {
    ...base,
    label,
  };
}

export async function runPipeline(
  stages: readonly PipelineStage[],
  options: RunPipelineOptions,
): Promise<PipelineRunResult> {
  ensurePipelineShape(stages);

  const discoveredContext = resolveContext(
    options.workspaceRoot ?? process.cwd(),
  );
  const workspaceRoot =
    options.workspaceRoot ?? discoveredContext.workspaceRoot;
  const binaryPath = options.binaryPath ?? discoveredContext.binaryPath;
  const projectDir = path.resolve(
    options.projectDir ?? discoveredContext.defaultProjectDir,
  );

  requireWorkspacePath(workspaceRoot, projectDir, "Project directory");

  const layout = await ensureCacheLayout(projectDir);
  const logger = await createRunLogger(layout.logDir);
  const results: StageArtifact[] = [];
  let gallery: Awaited<ReturnType<typeof startPipelineGallery>> | undefined;

  await logger.info("Pipeline started", {
    workspaceRoot,
    projectDir,
    binaryPath,
    galleryEnabled: options.gallery.enabled,
    force: options.force ?? false,
  });

  try {
    if (!existsSync(binaryPath)) {
      throw new Error(`Cannot find labypath binary at ${binaryPath}`);
    }

    const binaryFingerprint = await hashFile(binaryPath);
    const manifest = await loadCacheManifest(layout.cachePath);
    await mkdir(projectDir, { recursive: true });
    await logger.info("Execution context resolved", {
      binaryFingerprint,
      cachePath: layout.cachePath,
      runLogPath: logger.logPath,
    });

    gallery = options.gallery.enabled
      ? await startPipelineGallery(projectDir, stages, options.gallery)
      : undefined;

    if (gallery !== undefined) {
      await logger.info("Gallery started", { galleryUrl: gallery.url });
    } else {
      await logger.info("Gallery disabled");
    }

    const sourceStage = stages[0];
    let currentPath = requireSvgPath(
      workspaceRoot,
      sourceStage.sourcePath,
      sourceStage.label ?? "Source SVG",
    );

    gallery?.updateStage(0, {
      status: "completed",
      svgPath: currentPath,
      outputPath: currentPath,
      message: "Source SVG is ready.",
    });

    const sourceHash = await hashFile(currentPath);
    results.push(
      createStageArtifact(
        {
          stageKind: "source",
          inputPath: currentPath,
          outputPath: currentPath,
          cached: true,
          cacheReason: "source stage",
          durationMs: 0,
          outputHash: sourceHash,
        },
        sourceStage.label,
      ),
    );
    await logger.info("Source stage ready", {
      label: sourceStage.label ?? "Source SVG",
      outputPath: currentPath,
      outputHash: sourceHash,
    });

    const pipelineStartedAtMs = Date.now();

    for (const [offset, stage] of stages.slice(1).entries()) {
      const stageIndex = offset + 1;
      const stageStartedAtMs = Date.now();
      const inputPath = currentPath;
      const inputHash = await hashFile(inputPath);

      if (stage.kind === "source") {
        gallery?.failStage(
          stageIndex,
          "Only the first stage can be a source stage.",
        );
        throw new Error("Only the first stage can be a source stage.");
      }

      await logger.info("Stage started", {
        stageIndex,
        stageKind: stage.kind,
        label: stage.label ?? `${stage.kind} ${String(stageIndex + 1)}`,
        inputPath,
      });

      const previewPayload = buildStagePayload(
        stage,
        inputPath,
        "__OUTPUT_PATH__",
      );
      const payloadHash = hashString(stableStringify(previewPayload));
      const cacheKey = hashString(
        stableStringify({
          stageKind: stage.kind,
          binaryFingerprint,
          inputHash,
          payloadHash,
        }),
      );
      const stagePaths = resolveStagePaths(
        layout.svgDir,
        inputPath,
        stage.kind,
        cacheKey,
        layout.configDir,
        layout.logDir,
      );
      const payload = buildStagePayload(
        stage,
        inputPath,
        stagePaths.outputPath,
      );
      const existingEntry = manifest.entries[cacheKey];
      const existingOutputHash = await readOutputHashIfPresent(
        existingEntry?.outputPath ?? stagePaths.outputPath,
      );
      const cacheDecision = await resolveCacheDecision(
        options.force,
        existingEntry,
        binaryFingerprint,
        existingOutputHash,
      );

      await logger.info("Stage cache evaluated", {
        stageIndex,
        stageKind: stage.kind,
        cacheKey,
        payloadHash,
        decision: cacheDecision.cached ? "hit" : "miss",
        reason: cacheDecision.reason,
      });

      gallery?.updateStage(stageIndex, {
        outputPath: stagePaths.outputPath,
        message: "Waiting for the SVG file to appear.",
        status: "waiting",
      });

      if (cacheDecision.cached && existingEntry !== undefined) {
        const stageDurationMs = Date.now() - stageStartedAtMs;
        gallery?.updateStage(stageIndex, {
          status: "cached",
          svgPath: existingEntry.outputPath,
          outputPath: existingEntry.outputPath,
          message: "Reused cached SVG.",
        });
        await logger.info("Stage reused cached output", {
          stageIndex,
          stageKind: stage.kind,
          outputPath: existingEntry.outputPath,
          duration: formatDuration(stageDurationMs),
          logPath: existingEntry.logPath,
        });
        results.push(
          createStageArtifact(
            {
              stageKind: stage.kind,
              inputPath,
              outputPath: existingEntry.outputPath,
              configPath: existingEntry.configPath,
              logPath: existingEntry.logPath,
              cached: true,
              cacheReason: cacheDecision.reason,
              cacheKey,
              durationMs: stageDurationMs,
              inputHash,
              outputHash: existingEntry.outputHash,
            },
            stage.label,
          ),
        );
        currentPath = existingEntry.outputPath;
        continue;
      }

      gallery?.updateStage(stageIndex, {
        status: "running",
        outputPath: stagePaths.outputPath,
        message: "Stage is running.",
      });

      await logger.info("Stage executing", {
        stageIndex,
        stageKind: stage.kind,
        configPath: stagePaths.configPath,
        logPath: stagePaths.logPath,
        outputPath: stagePaths.outputPath,
        reason: cacheDecision.reason,
      });

      await writeFile(
        stagePaths.configPath,
        `${JSON.stringify(payload, null, 2)}\n`,
        "utf8",
      );
      const executionResult = await executeStage(
        binaryPath,
        workspaceRoot,
        stagePaths.configPath,
        stagePaths.logPath,
      );
      await logger.info("Stage process completed", {
        stageIndex,
        stageKind: stage.kind,
        executionDuration: formatDuration(executionResult.durationMs),
      });

      if (!existsSync(stagePaths.outputPath)) {
        gallery?.failStage(
          stageIndex,
          `Expected stage output was not created: ${stagePaths.outputPath}`,
        );
        await logger.error("Stage failed validation", {
          stageIndex,
          stageKind: stage.kind,
          reason: "output file missing",
          outputPath: stagePaths.outputPath,
        });
        throw new Error(
          `Expected stage output was not created: ${stagePaths.outputPath}`,
        );
      }

      if (!(await hasUsableSvgOutput(stagePaths.outputPath))) {
        gallery?.failStage(
          stageIndex,
          `Expected stage output to contain drawable SVG content: ${stagePaths.outputPath}`,
        );
        await logger.error("Stage failed validation", {
          stageIndex,
          stageKind: stage.kind,
          reason: "output SVG is not drawable",
          outputPath: stagePaths.outputPath,
        });
        throw new Error(
          `Expected stage output to contain drawable SVG content: ${stagePaths.outputPath}`,
        );
      }

      const outputHash = await hashFile(stagePaths.outputPath);
      const stageDurationMs = Date.now() - stageStartedAtMs;
      const now = new Date().toISOString();
      const entry: CacheEntry = {
        cacheKey,
        stageKind: stage.kind,
        binaryFingerprint,
        inputPath,
        inputHash,
        outputPath: stagePaths.outputPath,
        outputHash,
        configPath: stagePaths.configPath,
        logPath: stagePaths.logPath,
        payloadHash,
        payload,
        createdAt: existingEntry?.createdAt ?? now,
        updatedAt: now,
        status: "completed",
      };
      setCacheEntry(manifest, entry);

      gallery?.updateStage(stageIndex, {
        status: "completed",
        svgPath: stagePaths.outputPath,
        outputPath: stagePaths.outputPath,
        message: "SVG created.",
      });

      await logger.info("Stage completed", {
        stageIndex,
        stageKind: stage.kind,
        outputPath: stagePaths.outputPath,
        outputHash,
        duration: formatDuration(stageDurationMs),
      });

      results.push(
        createStageArtifact(
          {
            stageKind: stage.kind,
            inputPath,
            outputPath: stagePaths.outputPath,
            configPath: stagePaths.configPath,
            logPath: stagePaths.logPath,
            cached: false,
            cacheReason: cacheDecision.reason,
            cacheKey,
            durationMs: stageDurationMs,
            executionDurationMs: executionResult.durationMs,
            inputHash,
            outputHash,
          },
          stage.label,
        ),
      );
      currentPath = stagePaths.outputPath;
    }

    await saveCacheManifest(layout.cachePath, manifest);

    const durationMs = Date.now() - pipelineStartedAtMs;
    await logger.info("Pipeline completed", {
      duration: formatDuration(durationMs),
      stageCount: results.length,
      cachePath: layout.cachePath,
    });

    const result: PipelineRunResult = {
      projectDir,
      workspaceRoot,
      binaryPath,
      binaryFingerprint,
      cachePath: layout.cachePath,
      durationMs,
      runLogPath: logger.logPath,
      stages: results,
    };

    if (gallery !== undefined) {
      result.galleryUrl = gallery.url;
    }

    await logger.close();
    return result;
  } catch (error) {
    gallery?.failStage(
      results.length,
      error instanceof Error ? error.message : String(error),
    );
    await logger.error("Pipeline failed", {
      reason: error instanceof Error ? error.message : String(error),
    });
    await logger.close();
    throw error;
  }
}
