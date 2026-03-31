import type express from "express";
import { z } from "zod";
import { graphDocumentSchema } from "@labystudio/shared";
import type { AppDependencies } from "../lib/appDependencies.js";
import { requireWorkspacePath } from "../lib/context.js";
import { loadGraphDocument, saveGraphDocument } from "../lib/filesystem.js";

const pathQuerySchema = z.object({
  path: z.string().min(1)
});

const graphSaveSchema = z.object({
  path: z.string().min(1),
  graph: z.unknown()
});

export function registerGraphRoutes(
  app: express.Express,
  dependencies: Pick<AppDependencies, "resolveContext">
): void {
  app.get("/api/graph/load", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { path: filePath } = pathQuerySchema.parse(request.query);
      const resolvedPath = requireWorkspacePath(context, filePath, "Graph path");
      response.json(await loadGraphDocument(resolvedPath));
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/graph/save", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { path: filePath, graph: rawGraph } = graphSaveSchema.parse(request.body);
      const resolvedPath = requireWorkspacePath(context, filePath, "Graph path");
      const graph = graphDocumentSchema.parse(rawGraph);
      await saveGraphDocument(resolvedPath, graph);
      response.json({ path: resolvedPath });
    } catch (error) {
      next(error);
    }
  });
}