import type express from "express";
import { nanoid } from "nanoid";
import { jobRunRequestSchema } from "@labystudio/shared";
import type { AppDependencies } from "../lib/appDependencies.js";
import { requireWorkspacePath } from "../lib/context.js";
import { createQueuedJob, readJobLog, type JobStore } from "../lib/jobs.js";

type JobRouteDependencies = Pick<AppDependencies, "cleanupExpiredJobs" | "executeGraphJob" | "resolveContext">;

export function registerJobRoutes(
  app: express.Express,
  jobs: JobStore,
  dependencies: JobRouteDependencies
): void {
  app.post("/api/jobs/run", async (request, response, next) => {
    try {
      dependencies.cleanupExpiredJobs(jobs);
      const payload = jobRunRequestSchema.parse(request.body);
      const context = await dependencies.resolveContext();
      const projectDir = requireWorkspacePath(context, payload.projectDir, "Project directory");
      const jobId = nanoid();
      jobs.set(jobId, createQueuedJob(jobId, projectDir, payload.targetNodeId));
      void dependencies.executeGraphJob(jobs, jobId, payload.graph, projectDir, payload.targetNodeId);
      response.status(202).json({ jobId });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/jobs/:jobId", (request, response) => {
    dependencies.cleanupExpiredJobs(jobs);
    const job = jobs.get(request.params.jobId);
    if (!job) {
      response.status(404).json({ error: "Unknown job" });
      return;
    }

    response.json(job);
  });

  app.get("/api/jobs/:jobId/log", async (request, response) => {
    dependencies.cleanupExpiredJobs(jobs);
    const job = jobs.get(request.params.jobId);
    if (!job) {
      response.status(404).json({ error: "Unknown job" });
      return;
    }

    response.json({ log: await readJobLog(job) });
  });
}