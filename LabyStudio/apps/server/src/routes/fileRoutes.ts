import type express from "express";
import { z } from "zod";
import type { AppDependencies } from "../lib/appDependencies.js";
import { requireSvgPath, requireWorkspacePath } from "../lib/context.js";
import {
  copySvgIntoProject,
  listDirectory,
  readSvgFile,
  readWorkspaceFile,
  writeWorkspaceFile
} from "../lib/filesystem.js";

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

export function registerFileRoutes(
  app: express.Express,
  dependencies: Pick<AppDependencies, "resolveContext">
): void {
  app.get("/api/fs/list", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { dir } = directoryQuerySchema.parse(request.query);
      const resolvedDir = requireWorkspacePath(context, dir, "Directory");
      response.json({ dir: resolvedDir, entries: await listDirectory(resolvedDir) });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/fs/read", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { path: filePath } = pathQuerySchema.parse(request.query);
      const resolvedPath = requireWorkspacePath(context, filePath, "File path");
      response.json({ path: resolvedPath, content: await readWorkspaceFile(resolvedPath) });
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/fs/write", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { path: filePath, content } = fileWriteSchema.parse(request.body);
      const resolvedPath = requireWorkspacePath(context, filePath, "File path");
      await writeWorkspaceFile(resolvedPath, content);
      response.json({ path: resolvedPath });
    } catch (error) {
      next(error);
    }
  });

  app.get("/api/fs/svg", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { path: filePath } = pathQuerySchema.parse(request.query);
      const resolvedPath = requireSvgPath(context, filePath, "SVG path");
      response.type("image/svg+xml").send(await readSvgFile(resolvedPath));
    } catch (error) {
      next(error);
    }
  });

  app.post("/api/fs/import", async (request, response, next) => {
    try {
      const context = await dependencies.resolveContext();
      const { sourcePath, projectDir } = importSvgSchema.parse(request.body);
      const resolvedSourcePath = requireSvgPath(context, sourcePath, "Source SVG");
      const resolvedProjectDir = requireWorkspacePath(context, projectDir, "Project directory");
      response.json({ importedPath: await copySvgIntoProject(resolvedSourcePath, resolvedProjectDir) });
    } catch (error) {
      next(error);
    }
  });
}