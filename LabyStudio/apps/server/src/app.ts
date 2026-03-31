import cors from "cors";
import express from "express";
import { execa } from "execa";
import { existsSync, promises as fs } from "node:fs";
import path from "node:path";
import { nanoid } from "nanoid";
import { z } from "zod";
import {
  buildGraphExecutionPlan,
  buildGridConfigPayload,
  buildRenderConfigPayload,
  buildRouteConfigPayload,
  defaultProjectDirCandidates,
  graphDocumentSchema,
  importedSourceName,
  jobRunRequestSchema,
  listBinaryCandidates,
  stageStem,
  type ArtifactState,
  type DirectoryEntry,
  type GraphDocument,
  type GraphRunNodeResult,
  type RuntimeContext,
  type WorkflowJob
} from "@labystudio/shared";

type JobStore = Map<string, WorkflowJob>;

const JOB_TTL_MS = 24 * 60 * 60 * 1000;

const directoryQuerySchema = z.object({
  dir: z.string().min(1)
});

const pathQuerySchema = z.object({
  path: z.string().min(1)
});

const fileWriteSchema = z.object({
  path: z.string().min(1),
  content: z.string()
});

const importSvgSchema = z.object({
  sourcePath: z.string().min(1),
  projectDir: z.string().min(1)
});

const graphSaveSchema = z.object({
  path: z.string().min(1),
  graph: z.unknown()
});

function requireWorkspacePath(context: RuntimeContext, candidatePath: string, label: string): string {
  if (!path.isAbsolute(candidatePath)) {
    throw new Error(`${label} must be an absolute path.`);
  }

  const resolvedPath = path.resolve(candidatePath);
  if (resolvedPath !== context.workspaceRoot && !resolvedPath.startsWith(`${context.workspaceRoot}${path.sep}`)) {
    throw new Error(`${label} must stay within ${context.workspaceRoot}.`);
  }

  return resolvedPath;
}

function requireSvgPath(context: RuntimeContext, candidatePath: string, label: string): string {
  const resolvedPath = requireWorkspacePath(context, candidatePath, label);
  if (path.extname(resolvedPath).toLowerCase() !== ".svg") {
    throw new Error(`${label} must point to an SVG file.`);
  }

  return resolvedPath;
}

function cleanupExpiredJobs(jobStore: JobStore): void {
  const cutoff = Date.now() - JOB_TTL_MS;
  for (const [jobId, job] of jobStore.entries()) {
    if ((job.status === "completed" || job.status === "failed") && Date.parse(job.updatedAt) < cutoff) {
      jobStore.delete(jobId);
    }
  }
}

function findWorkspaceRoot(startPath: string): string {
  let candidate = startPath;
  while (true) {
    if (existsSync(path.join(candidate, "LabyPath")) && existsSync(path.join(candidate, "LabyPython"))) {
      return candidate;
    }

    const parent = path.dirname(candidate);
    if (parent === candidate) {
      return startPath;
    }

    candidate = parent;
  }
}

async function resolveContext(): Promise<RuntimeContext> {
  const workspaceRoot = findWorkspaceRoot(process.cwd());
  const binaryCandidates = listBinaryCandidates(workspaceRoot);
  const fallbackBinaryPath = path.join(workspaceRoot, ".cmake", "build", "LabyPath", "labypath");
  const binaryPath = binaryCandidates.find((candidate) => existsSync(candidate)) ?? fallbackBinaryPath;
  const defaultProjectDir = defaultProjectDirCandidates(workspaceRoot).find((candidate) => existsSync(candidate)) ?? workspaceRoot;

  return {
    workspaceRoot,
    binaryPath,
    binaryExists: existsSync(binaryPath),
    defaultProjectDir
  };
}

async function listDirectory(dir: string): Promise<DirectoryEntry[]> {
  const entries = await fs.readdir(dir, { withFileTypes: true });
  return entries
    .filter((entry) => entry.name !== "." && entry.name !== "..")
    .map<DirectoryEntry>((entry) => ({
      name: entry.name,
      path: path.join(dir, entry.name),
      kind: entry.isDirectory() ? "directory" : "file",
      extension: entry.isDirectory() ? "" : path.extname(entry.name)
    }))
    .sort((left, right) => {
      if (left.kind !== right.kind) {
        return left.kind === "directory" ? -1 : 1;
      }

      return left.name.localeCompare(right.name);
    });
}

async function copySvgIntoProject(sourcePath: string, projectDir: string): Promise<string> {
  await fs.mkdir(projectDir, { recursive: true });
  const destinationStem = path.basename(importedSourceName(sourcePath), ".svg");
  let index = 0;

  while (true) {
    const suffix = index === 0 ? "" : `-${index}`;
    const destination = path.join(projectDir, `${destinationStem}${suffix}.svg`);
    if (!existsSync(destination)) {
      await fs.copyFile(sourcePath, destination);
      return destination;
    }

    index += 1;
  }
}

async function appendLog(logPath: string, message: string): Promise<void> {
  await fs.mkdir(path.dirname(logPath), { recursive: true });
  await fs.appendFile(logPath, message, "utf8");
}

async function makeStagePaths(projectDir: string, inputPath: string, stage: "grid" | "route" | "render") {
  let index = 1;
  while (true) {
    const stem = stageStem(inputPath, stage, index);
    const configPath = path.join(projectDir, `${stem}.json`);
    const outputPath = path.join(projectDir, `${stem}.svg`);
    if (!existsSync(configPath) && !existsSync(outputPath)) {
      return { configPath, outputPath };
    }

    index += 1;
  }
}

async function executeStage(binaryPath: string, workspaceRoot: string, configPath: string, stageLogPath: string, jobLogPath: string): Promise<void> {
  const startedAt = new Date().toISOString();
  const commandLine = `${binaryPath} ${configPath}`;
  await appendLog(jobLogPath, `\ncommand: ${commandLine}\n`);

  try {
    const result = await execa(binaryPath, [configPath], {
      cwd: workspaceRoot,
      all: true
    });
    const output = result.all ?? "";
    await fs.writeFile(stageLogPath, `started at ${startedAt}\ncommand: ${commandLine}\n${output}`, "utf8");
    await appendLog(jobLogPath, `${output}\n`);
  } catch (error) {
    const output = typeof error === "object" && error && "all" in error ? String((error as { all?: string }).all ?? "") : String(error);
    await fs.writeFile(stageLogPath, `started at ${startedAt}\ncommand: ${commandLine}\n${output}`, "utf8");
    await appendLog(jobLogPath, `${output}\n`);
    throw error;
  }
}

async function executeGraphJob(jobStore: JobStore, jobId: string, graph: GraphDocument, projectDir: string, targetNodeId: string): Promise<void> {
  const context = await resolveContext();
  const plan = buildGraphExecutionPlan(graph, targetNodeId);
  const job = jobStore.get(jobId);
  if (!job) {
    return;
  }

  const artifactByNode = new Map<string, ArtifactState>();
  const jobLogPath = path.join(projectDir, "logs", `labystudio-job-${jobId}.log`);
  job.logPath = jobLogPath;
  job.status = "running";
  job.updatedAt = new Date().toISOString();
  await appendLog(jobLogPath, `started at ${job.updatedAt}\n`);

  try {
    if (!context.binaryExists) {
      throw new Error(`Cannot find labypath binary at ${context.binaryPath}`);
    }

    for (const step of plan) {
      job.currentNodeId = step.id;
      job.updatedAt = new Date().toISOString();

      if (step.data.kind === "source") {
        const sourcePath = requireSvgPath(context, step.data.sourcePath, `Source SVG for ${step.data.label}`);
        if (!existsSync(sourcePath)) {
          throw new Error(`Source SVG does not exist: ${sourcePath}`);
        }

        artifactByNode.set(step.id, {
          inputPath: sourcePath,
          outputPath: sourcePath,
          status: "completed",
          lastRunAt: new Date().toISOString()
        });
        continue;
      }

      if (!step.upstreamId) {
        throw new Error(`Node ${step.data.label} is missing its upstream dependency.`);
      }

      const upstreamArtifact = artifactByNode.get(step.upstreamId);
      if (!upstreamArtifact?.outputPath) {
        throw new Error(`Upstream artifact for ${step.data.label} is missing.`);
      }

      const stagePaths = await makeStagePaths(projectDir, upstreamArtifact.outputPath, step.data.kind);
      const stageLogPath = path.join(projectDir, "logs", `${path.basename(stagePaths.configPath, ".json")}.log`);

      let payload: Record<string, unknown>;
      if (step.data.kind === "grid") {
        payload = buildGridConfigPayload(upstreamArtifact.outputPath, stagePaths.outputPath, step.data.config);
      } else if (step.data.kind === "route") {
        payload = buildRouteConfigPayload(upstreamArtifact.outputPath, stagePaths.outputPath, step.data.config);
      } else {
        payload = buildRenderConfigPayload(upstreamArtifact.outputPath, stagePaths.outputPath, step.data.config);
      }

      await fs.writeFile(stagePaths.configPath, `${JSON.stringify(payload, null, 2)}\n`, "utf8");
      await executeStage(context.binaryPath, context.workspaceRoot, stagePaths.configPath, stageLogPath, jobLogPath);

      artifactByNode.set(step.id, {
        inputPath: upstreamArtifact.outputPath,
        outputPath: stagePaths.outputPath,
        configPath: stagePaths.configPath,
        logPath: stageLogPath,
        status: "completed",
        lastRunAt: new Date().toISOString()
      });
    }

    const resultNodes: GraphRunNodeResult[] = Array.from(artifactByNode.entries()).map(([nodeId, artifacts]) => ({
      nodeId,
      artifacts
    }));
    job.status = "completed";
    job.updatedAt = new Date().toISOString();
    job.result = { nodes: resultNodes };
    await appendLog(jobLogPath, `\ncompleted at ${job.updatedAt}\n`);
  } catch (error) {
    job.status = "failed";
    job.updatedAt = new Date().toISOString();
    job.error = error instanceof Error ? error.message : String(error);
    await appendLog(jobLogPath, `\nfailed at ${job.updatedAt}: ${job.error}\n`);
  }
}

export async function createApp(): Promise<express.Express> {
  const jobs: JobStore = new Map();
  const app = express();

  app.use(cors());
  app.use(express.json({ limit: "4mb" }));

  app.get("/api/context", async (_request, response, next) => {
    try {
      response.json(await resolveContext());
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/fs/list", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { dir } = directoryQuerySchema.parse(request.query);
      const resolvedDir = requireWorkspacePath(context, dir, "Directory");
      response.json({ dir: resolvedDir, entries: await listDirectory(resolvedDir) });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/fs/read", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { path: filePath } = pathQuerySchema.parse(request.query);
      const resolvedPath = requireWorkspacePath(context, filePath, "File path");
      response.json({ path: resolvedPath, content: await fs.readFile(resolvedPath, "utf8") });
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/fs/write", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { path: filePath, content } = fileWriteSchema.parse(request.body);
      const resolvedPath = requireWorkspacePath(context, filePath, "File path");
      await fs.mkdir(path.dirname(resolvedPath), { recursive: true });
      await fs.writeFile(resolvedPath, content, "utf8");
      response.json({ path: resolvedPath });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/fs/svg", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { path: filePath } = pathQuerySchema.parse(request.query);
      const resolvedPath = requireSvgPath(context, filePath, "SVG path");
      response.type("image/svg+xml").send(await fs.readFile(resolvedPath, "utf8"));
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/fs/import", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { sourcePath, projectDir } = importSvgSchema.parse(request.body);
      const resolvedSourcePath = requireSvgPath(context, sourcePath, "Source SVG");
      const resolvedProjectDir = requireWorkspacePath(context, projectDir, "Project directory");
      response.json({ importedPath: await copySvgIntoProject(resolvedSourcePath, resolvedProjectDir) });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/graph/load", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { path: filePath } = pathQuerySchema.parse(request.query);
      const resolvedPath = requireWorkspacePath(context, filePath, "Graph path");
      const graph = graphDocumentSchema.parse(JSON.parse(await fs.readFile(resolvedPath, "utf8")));
      response.json(graph);
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/graph/save", async (request, response, next) => {
    try {
      const context = await resolveContext();
      const { path: filePath, graph: rawGraph } = graphSaveSchema.parse(request.body);
      const resolvedPath = requireWorkspacePath(context, filePath, "Graph path");
      const graph = graphDocumentSchema.parse(rawGraph);
      await fs.mkdir(path.dirname(resolvedPath), { recursive: true });
      await fs.writeFile(resolvedPath, `${JSON.stringify(graph, null, 2)}\n`, "utf8");
      response.json({ path: resolvedPath });
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/jobs/run", async (request, response, next) => {
    try {
      cleanupExpiredJobs(jobs);
      const payload = jobRunRequestSchema.parse(request.body);
      const context = await resolveContext();
      const projectDir = requireWorkspacePath(context, payload.projectDir, "Project directory");
      buildGraphExecutionPlan(payload.graph, payload.targetNodeId);
      const jobId = nanoid();
      const job: WorkflowJob = {
        id: jobId,
        status: "queued",
        projectDir,
        targetNodeId: payload.targetNodeId,
        createdAt: new Date().toISOString(),
        updatedAt: new Date().toISOString()
      };
      jobs.set(jobId, job);
      void executeGraphJob(jobs, jobId, payload.graph, projectDir, payload.targetNodeId);
      response.status(202).json({ jobId });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/jobs/:jobId", (request, response) => {
    cleanupExpiredJobs(jobs);
    const job = jobs.get(request.params.jobId);
    if (!job) {
      response.status(404).json({ error: "Unknown job" });
      return;
    }

    response.json(job);
  });

  app.get("/api/jobs/:jobId/log", async (request, response) => {
    cleanupExpiredJobs(jobs);
    const job = jobs.get(request.params.jobId);
    if (!job) {
      response.status(404).json({ error: "Unknown job" });
      return;
    }

    if (!job.logPath || !existsSync(job.logPath)) {
      response.json({ log: "" });
      return;
    }

    response.json({ log: await fs.readFile(job.logPath, "utf8") });
  });

  app.use((error: unknown, _request: express.Request, response: express.Response, _next: express.NextFunction) => {
    const message = error instanceof Error ? error.message : String(error);
    response.status(500).send(message);
  });

  return app;
}