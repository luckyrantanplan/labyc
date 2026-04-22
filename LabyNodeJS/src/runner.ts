import { execa } from "execa";
import { existsSync } from "node:fs";
import { mkdir, writeFile } from "node:fs/promises";
import path from "node:path";
import { ensureCacheLayout, loadCacheManifest, saveCacheManifest, setCacheEntry } from "./cache.js";
import { requireSvgPath, requireWorkspacePath, resolveContext } from "./context.js";
import { startPipelineGallery } from "./gallery.js";
import { hashFile, hashString, stableStringify } from "./hash.js";
import { artifactStem, buildStagePayload } from "./payload.js";
import type { CacheEntry, PipelineRunResult, PipelineStage, RunPipelineOptions, SourceStage, StageArtifact, TransformerStage } from "./types.js";

function ensurePipelineShape(stages: readonly PipelineStage[]): asserts stages is readonly [SourceStage, ...PipelineStage[]] {
  if (stages.length === 0) {
    throw new Error("Pipeline must contain at least one stage.");
  }

  if (stages[0]?.kind !== "source") {
    throw new Error("Pipeline must start with a source stage.");
  }
}

function resolveStagePaths(svgDir: string, inputPath: string, stageKind: TransformerStage["kind"], cacheKey: string, configDir: string, logDir: string) {
  const stem = artifactStem(inputPath, stageKind, cacheKey);
  return {
    configPath: path.join(configDir, `${stem}.json`),
    logPath: path.join(logDir, `${stem}.log`),
    outputPath: path.join(svgDir, `${stem}.svg`)
  };
}

function isCacheHit(entry: CacheEntry | undefined, outputHash: string | undefined, binaryFingerprint: string): entry is CacheEntry {
  if (entry === undefined) {
    return false;
  }

  return entry.binaryFingerprint === binaryFingerprint
    && typeof outputHash === "string"
    && outputHash === entry.outputHash;
}

async function readOutputHashIfPresent(outputPath: string): Promise<string | undefined> {
  if (!existsSync(outputPath)) {
    return undefined;
  }

  return hashFile(outputPath);
}

async function executeStage(binaryPath: string, workspaceRoot: string, configPath: string, logPath: string): Promise<void> {
  const startedAt = new Date().toISOString();
  const commandLine = `${binaryPath} ${configPath}`;

  try {
    const result = await execa(binaryPath, [configPath], {
      cwd: workspaceRoot,
      all: true
    });
    await writeFile(logPath, `started at ${startedAt}\ncommand: ${commandLine}\n${result.all}`, "utf8");
  } catch (error) {
    let output: string;

    if (typeof error === "object" && error !== null && "all" in error && typeof error.all === "string") {
      output = error.all;
    } else if (error instanceof Error) {
      output = error.message;
    } else {
      output = String(error);
    }

    await writeFile(logPath, `started at ${startedAt}\ncommand: ${commandLine}\n${output}`, "utf8");
    throw error;
  }
}

function createStageArtifact(base: Omit<StageArtifact, "label">, label: string | undefined): StageArtifact {
  if (label === undefined) {
    return base;
  }

  return {
    ...base,
    label
  };
}

export async function runPipeline(stages: readonly PipelineStage[], options: RunPipelineOptions = {}): Promise<PipelineRunResult> {
  ensurePipelineShape(stages);

  const discoveredContext = resolveContext(options.workspaceRoot ?? process.cwd());
  const workspaceRoot = options.workspaceRoot ?? discoveredContext.workspaceRoot;
  const binaryPath = options.binaryPath ?? discoveredContext.binaryPath;
  const projectDir = path.resolve(options.projectDir ?? discoveredContext.defaultProjectDir);

  requireWorkspacePath(workspaceRoot, projectDir, "Project directory");

  if (!existsSync(binaryPath)) {
    throw new Error(`Cannot find labypath binary at ${binaryPath}`);
  }

  const binaryFingerprint = await hashFile(binaryPath);
  const layout = await ensureCacheLayout(projectDir);
  const manifest = await loadCacheManifest(layout.cachePath);
  await mkdir(projectDir, { recursive: true });
  const galleryOptions = options.gallery;
  const gallery = galleryOptions?.enabled
    ? await startPipelineGallery(projectDir, stages, {
      ...(galleryOptions.title !== undefined ? { title: galleryOptions.title } : {}),
      ...(galleryOptions.port !== undefined ? { port: galleryOptions.port } : {}),
      ...(galleryOptions.keepAlive !== undefined ? { keepAlive: galleryOptions.keepAlive } : {}),
      ...(galleryOptions.openBrowser !== undefined ? { openBrowser: galleryOptions.openBrowser } : {})
    })
    : undefined;

  const results: StageArtifact[] = [];
  const sourceStage = stages[0];
  let currentPath = requireSvgPath(workspaceRoot, sourceStage.sourcePath, sourceStage.label ?? "Source SVG");

  gallery?.updateStage(0, {
    status: "completed",
    svgPath: currentPath,
    outputPath: currentPath,
    message: "Source SVG is ready."
  });

  results.push(createStageArtifact({
    stageKind: "source",
    inputPath: currentPath,
    outputPath: currentPath,
    cached: true,
    outputHash: await hashFile(currentPath)
  }, sourceStage.label));

  try {
    for (const [offset, stage] of stages.slice(1).entries()) {
      const stageIndex = offset + 1;
      const inputPath = currentPath;
      const inputHash = await hashFile(inputPath);

      if (stage.kind === "source") {
        gallery?.failStage(stageIndex, "Only the first stage can be a source stage.");
        throw new Error("Only the first stage can be a source stage.");
      }

      const previewPayload = buildStagePayload(stage, inputPath, "__OUTPUT_PATH__");
      const payloadHash = hashString(stableStringify(previewPayload));
      const cacheKey = hashString(stableStringify({
        stageKind: stage.kind,
        binaryFingerprint,
        inputHash,
        payloadHash
      }));
      const stagePaths = resolveStagePaths(layout.svgDir, inputPath, stage.kind, cacheKey, layout.configDir, layout.logDir);
      const payload = buildStagePayload(stage, inputPath, stagePaths.outputPath);
      const existingEntry = manifest.entries[cacheKey];
      const existingOutputHash = await readOutputHashIfPresent(stagePaths.outputPath);

      gallery?.updateStage(stageIndex, {
        outputPath: stagePaths.outputPath,
        message: "Waiting for the SVG file to appear.",
        status: "waiting"
      });

      if (!options.force && isCacheHit(existingEntry, existingOutputHash, binaryFingerprint)) {
        gallery?.updateStage(stageIndex, {
          status: "cached",
          svgPath: existingEntry.outputPath,
          outputPath: existingEntry.outputPath,
          message: "Reused cached SVG."
        });
        results.push(createStageArtifact({
          stageKind: stage.kind,
          inputPath,
          outputPath: existingEntry.outputPath,
          configPath: existingEntry.configPath,
          logPath: existingEntry.logPath,
          cached: true,
          cacheKey,
          inputHash,
          outputHash: existingEntry.outputHash
        }, stage.label));
        currentPath = existingEntry.outputPath;
        continue;
      }

      gallery?.updateStage(stageIndex, {
        status: "running",
        outputPath: stagePaths.outputPath,
        message: "Stage is running."
      });

      await writeFile(stagePaths.configPath, `${JSON.stringify(payload, null, 2)}\n`, "utf8");
      await executeStage(binaryPath, workspaceRoot, stagePaths.configPath, stagePaths.logPath);

      if (!existsSync(stagePaths.outputPath)) {
        gallery?.failStage(stageIndex, `Expected stage output was not created: ${stagePaths.outputPath}`);
        throw new Error(`Expected stage output was not created: ${stagePaths.outputPath}`);
      }

      const outputHash = await hashFile(stagePaths.outputPath);
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
        status: "completed"
      };
      setCacheEntry(manifest, entry);

      gallery?.updateStage(stageIndex, {
        status: "completed",
        svgPath: stagePaths.outputPath,
        outputPath: stagePaths.outputPath,
        message: "SVG created."
      });

      results.push(createStageArtifact({
        stageKind: stage.kind,
        inputPath,
        outputPath: stagePaths.outputPath,
        configPath: stagePaths.configPath,
        logPath: stagePaths.logPath,
        cached: false,
        cacheKey,
        inputHash,
        outputHash
      }, stage.label));
      currentPath = stagePaths.outputPath;
    }

    await saveCacheManifest(layout.cachePath, manifest);

    const result: PipelineRunResult = {
      projectDir,
      workspaceRoot,
      binaryPath,
      binaryFingerprint,
      cachePath: layout.cachePath,
      stages: results
    };

    if (gallery !== undefined) {
      result.galleryUrl = gallery.url;
    }

    return result;
  } catch (error) {
    const activeStageIndex = results.length;
    if (gallery !== undefined) {
      gallery.failStage(activeStageIndex, error instanceof Error ? error.message : String(error));
    }
    throw error;
  }
}