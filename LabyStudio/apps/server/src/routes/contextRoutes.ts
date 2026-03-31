import type express from "express";
import type { AppDependencies } from "../lib/appDependencies.js";

export function registerContextRoutes(
  app: express.Express,
  dependencies: Pick<AppDependencies, "resolveContext">
): void {
  app.get("/api/context", async (_request, response, next) => {
    try {
      response.json(await dependencies.resolveContext());
    } catch (error) {
      next(error);
    }
  });
}