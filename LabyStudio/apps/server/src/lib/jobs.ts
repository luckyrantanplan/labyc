import { execa } from "execa";
import { existsSync, promises as fs } from "node:fs";
import path from "node:path";
import {
  buildGraphExecutionPlan,
  buildGridConfigPayload,
  buildRenderConfigPayload,
  buildRouteConfigPayload,
  stageStem,
  type ArtifactState,
  type GraphDocument,
  type GraphRunNodeResult,
  type WorkflowJob
} from "@labystudio/shared";
import { requireSvgPath, resolveContext } from "./context.js";

export type JobStore = Map<string, WorkflowJob>;

const JOB_TTL_MS = 24 * 60 * 60 * 1000;

export function cleanupExpiredJobs(jobStore: JobStore): void {
  const cutoff = Date.now() - JOB_TTL_MS;
  for (const [jobId, job] of jobStore.entries()) {
    if ((job.status === "completed" || job.status === "failed") && Date.parse(job.updatedAt) < cutoff) {
      jobStore.delete(jobId);
    }
  }
}

export function createQueuedJob(jobId: string, projectDir: string, targetNodeId: string): WorkflowJob {
  const createdAt = new Date().toISOString();

  return {
    id: jobId,
    status: "queued",
    projectDir,
    targetNodeId,
    createdAt,
    updatedAt: createdAt
  };
}

async function appendLog(logPath: string, message: string): Promise<void> {
  await fs.mkdir(path.dirname(logPath), { recursive: true });
  await fs.appendFile(logPath, message, "utf8");
}

function makeStagePaths(projectDir: string, inputPath: string, stage: "grid" | "route" | "render") {
  let index = 1;
  for (;;) {
    const stem = stageStem(inputPath, stage, index);
    const configPath = path.join(projectDir, `${stem}.json`);
    const outputPath = path.join(projectDir, `${stem}.svg`);
    const configExists = existsSync(configPath);
    const outputExists = existsSync(outputPath);
    if (!configExists && !outputExists) {
      return { configPath, outputPath };
    }

    index += 1;
  }
}

async function executeStage(
  binaryPath: string,
  workspaceRoot: string,
  configPath: string,
  stageLogPath: string,
  jobLogPath: string
): Promise<void> {
  const startedAt = new Date().toISOString();
  const commandLine = `${binaryPath} ${configPath}`;
  await appendLog(jobLogPath, `\ncommand: ${commandLine}\n`);

  try {
    const result = await execa(binaryPath, [configPath], {
      cwd: workspaceRoot,
      all: true
    });
    const output = result.all;
    await fs.writeFile(stageLogPath, `started at ${startedAt}\ncommand: ${commandLine}\n${output}`, "utf8");
    await appendLog(jobLogPath, `${output}\n`);
  } catch (error) {
    const rawOutput = typeof error === "object" && error && "all" in error
      ? (error as { all?: unknown }).all ?? error
      : error;
    const output = typeof rawOutput === "string" ? rawOutput : String(rawOutput);
    await fs.writeFile(stageLogPath, `started at ${startedAt}\ncommand: ${commandLine}\n${output}`, "utf8");
    await appendLog(jobLogPath, `${output}\n`);
    throw error;
  }
}

export async function executeGraphJob(
  jobStore: JobStore,
  jobId: string,
  graph: GraphDocument,
  projectDir: string,
  targetNodeId: string
): Promise<void> {
  const context = resolveContext();
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

      const stagePaths = makeStagePaths(projectDir, upstreamArtifact.outputPath, step.data.kind);
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

export async function readJobLog(job: WorkflowJob): Promise<string> {
  if (!job.logPath || !existsSync(job.logPath)) {
    return "";
  }

  return fs.readFile(job.logPath, "utf8");
}