import { mkdtemp, mkdir, readFile, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import type express from "express";
import request from "supertest";
import { createDefaultNodeData, type GraphDocument, type RuntimeContext } from "@labystudio/shared";
import { describe, expect, it, vi } from "vitest";
import { createApp } from "./app.js";
import type { JobStore } from "./lib/jobs.js";

async function createWorkspace(): Promise<{ workspaceRoot: string; projectDir: string; context: RuntimeContext }> {
  const workspaceRoot = await mkdtemp(path.join(os.tmpdir(), "labystudio-"));
  await mkdir(path.join(workspaceRoot, "LabyPath"), { recursive: true });
  await mkdir(path.join(workspaceRoot, "LabyPython"), { recursive: true });
  const projectDir = path.join(workspaceRoot, "project");
  await mkdir(projectDir, { recursive: true });

  return {
    workspaceRoot,
    projectDir,
    context: {
      workspaceRoot,
      binaryPath: path.join(workspaceRoot, "bin", "labypath"),
      binaryExists: true,
      defaultProjectDir: projectDir
    }
  };
}

function createGraph(workspaceRoot: string): GraphDocument {
  const sourceNode = createDefaultNodeData("source");
  if (sourceNode.kind !== "source") {
    throw new Error("Expected a source node");
  }

  sourceNode.sourcePath = path.join(workspaceRoot, "source.svg");

  return {
    version: 1,
    nodes: [
      {
        id: "source",
        type: "workflow",
        position: { x: 0, y: 0 },
        data: sourceNode
      }
    ],
    edges: []
  };
}

async function waitForJobStatus(app: express.Express, jobId: string, expectedStatus: string): Promise<{ status: string }> {
  for (let attempt = 0; attempt < 10; attempt += 1) {
    const response = await request(app)
      .get(`/api/jobs/${jobId}`)
      .expect(200);
    const jobBody = response.body as { status: string };
    if (jobBody.status === expectedStatus) {
      return jobBody;
    }

    await new Promise((resolve) => {
      setTimeout(resolve, 0);
    });
  }

  throw new Error(`Job ${jobId} did not reach status ${expectedStatus}`);
}

describe("createApp", () => {
  it("saves and loads graph documents within the workspace", async () => {
    const { workspaceRoot, projectDir, context } = await createWorkspace();
    const app = createApp({
      resolveContext: () => context,
      cleanupExpiredJobs: vi.fn(),
      executeGraphJob: vi.fn(() => Promise.resolve())
    });
    const graph = createGraph(workspaceRoot);
    const graphPath = path.join(projectDir, "labystudio.graph.json");

    await request(app)
      .post("/api/graph/save")
      .send({ path: graphPath, graph })
      .expect(200);

    const savedGraph = JSON.parse(await readFile(graphPath, "utf8")) as GraphDocument;
    expect(savedGraph).toEqual(graph);

    const response = await request(app)
      .get("/api/graph/load")
      .query({ path: graphPath })
      .expect(200);

    expect(response.body).toEqual(graph);
  });

  it("creates jobs and exposes status and log endpoints", async () => {
    const { workspaceRoot, projectDir, context } = await createWorkspace();
    const graph = createGraph(workspaceRoot);
    const cleanupExpiredJobs = vi.fn();
    const executeGraphJob = vi.fn(async (jobStore: JobStore, jobId: string) => {
      const job = jobStore.get(jobId);
      if (!job) {
        throw new Error("Expected queued job");
      }

      const logPath = path.join(projectDir, "logs", `${jobId}.log`);
      await mkdir(path.dirname(logPath), { recursive: true });
      await writeFile(logPath, "job log", "utf8");

      job.status = "completed";
      job.updatedAt = "2026-03-31T00:00:02.000Z";
      job.logPath = logPath;
      job.result = { nodes: [] };
    });

    const app = createApp({
      resolveContext: () => context,
      cleanupExpiredJobs,
      executeGraphJob
    });

    const runResponse = await request(app)
      .post("/api/jobs/run")
      .send({
        graph,
        projectDir,
        targetNodeId: "source"
      })
      .expect(202);

    await Promise.resolve();

    expect(executeGraphJob).toHaveBeenCalledTimes(1);
    expect(cleanupExpiredJobs).toHaveBeenCalled();

    const runBody = runResponse.body as { jobId: string };
    const jobId = runBody.jobId;

    const jobBody = await waitForJobStatus(app, jobId, "completed");
    expect(jobBody.status).toBe("completed");

    const logResponse = await request(app)
      .get(`/api/jobs/${jobId}/log`)
      .expect(200);
    expect(logResponse.body).toEqual({ log: "job log" });
  });
});