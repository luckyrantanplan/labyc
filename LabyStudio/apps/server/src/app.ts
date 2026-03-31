import cors from "cors";
import express from "express";
import { ZodError } from "zod";
import type { AppDependencies } from "./lib/appDependencies.js";
import { resolveContext } from "./lib/context.js";
import { cleanupExpiredJobs, executeGraphJob, type JobStore } from "./lib/jobs.js";
import { registerContextRoutes } from "./routes/contextRoutes.js";
import { registerFileRoutes } from "./routes/fileRoutes.js";
import { registerGraphRoutes } from "./routes/graphRoutes.js";
import { registerJobRoutes } from "./routes/jobRoutes.js";

export function createApp(overrides: Partial<AppDependencies> = {}): express.Express {
  const jobs: JobStore = new Map();
  const dependencies: AppDependencies = {
    resolveContext,
    cleanupExpiredJobs,
    executeGraphJob,
    ...overrides
  };
  const app = express();

  app.use(cors());
  app.use(express.json({ limit: "4mb" }));

  registerContextRoutes(app, dependencies);
  registerFileRoutes(app, dependencies);
  registerGraphRoutes(app, dependencies);
  registerJobRoutes(app, jobs, dependencies);

  app.use((error: unknown, _request: express.Request, response: express.Response, next: express.NextFunction) => {
    void next;
    const message = error instanceof Error ? error.message : String(error);
    response.status(error instanceof ZodError ? 400 : 500).send(message);
  });

  return app;
}