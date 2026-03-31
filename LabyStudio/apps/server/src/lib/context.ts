import { existsSync } from "node:fs";
import path from "node:path";
import {
  defaultProjectDirCandidates,
  listBinaryCandidates,
  type RuntimeContext
} from "@labystudio/shared";

export function findWorkspaceRoot(startPath: string): string {
  let candidate = startPath;
  for (;;) {
    const hasLabyPath = existsSync(path.join(candidate, "LabyPath"));
    const hasLabyPython = existsSync(path.join(candidate, "LabyPython"));
    if (hasLabyPath && hasLabyPython) {
      return candidate;
    }

    const parent = path.dirname(candidate);
    if (parent === candidate) {
      return startPath;
    }

    candidate = parent;
  }
}

export function resolveContext(): RuntimeContext {
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

export function requireWorkspacePath(context: RuntimeContext, candidatePath: string, label: string): string {
  if (!path.isAbsolute(candidatePath)) {
    throw new Error(`${label} must be an absolute path.`);
  }

  const resolvedPath = path.resolve(candidatePath);
  if (resolvedPath !== context.workspaceRoot && !resolvedPath.startsWith(`${context.workspaceRoot}${path.sep}`)) {
    throw new Error(`${label} must stay within ${context.workspaceRoot}.`);
  }

  return resolvedPath;
}

export function requireSvgPath(context: RuntimeContext, candidatePath: string, label: string): string {
  const resolvedPath = requireWorkspacePath(context, candidatePath, label);
  if (path.extname(resolvedPath).toLowerCase() !== ".svg") {
    throw new Error(`${label} must point to an SVG file.`);
  }

  return resolvedPath;
}