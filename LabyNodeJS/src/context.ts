import { existsSync } from "node:fs";
import path from "node:path";
import type { RuntimeContext } from "./types.js";

export function listBinaryCandidates(workspaceRoot: string): string[] {
  return [
    path.join(workspaceRoot, ".cmake", "build", "LabyPath", "labypath"),
    path.join(workspaceRoot, "LabyPath", "build", "labypath"),
    path.join(workspaceRoot, "LabyPath", "Debug", "LabyPath"),
  ];
}

export function defaultProjectDirCandidates(workspaceRoot: string): string[] {
  return [
    path.join(workspaceRoot, "LabyData"),
    path.join(workspaceRoot, "LabyPath", "input"),
    path.join(workspaceRoot, "LabyPath"),
    workspaceRoot,
  ];
}

export function findWorkspaceRoot(startPath: string): string {
  let candidate = path.resolve(startPath);
  for (;;) {
    const hasLabyPath = existsSync(path.join(candidate, "LabyPath"));
    const hasLabyPython = existsSync(path.join(candidate, "LabyPython"));
    if (hasLabyPath && hasLabyPython) {
      return candidate;
    }

    const parent = path.dirname(candidate);
    if (parent === candidate) {
      return path.resolve(startPath);
    }

    candidate = parent;
  }
}

export function resolveContext(startPath = process.cwd()): RuntimeContext {
  const workspaceRoot = findWorkspaceRoot(startPath);
  const binaryCandidates = listBinaryCandidates(workspaceRoot);
  const fallbackBinaryPath =
    binaryCandidates[0] ?? path.join(workspaceRoot, "labypath");
  const binaryPath =
    binaryCandidates.find((candidate) => existsSync(candidate)) ??
    fallbackBinaryPath;
  const defaultProjectDir =
    defaultProjectDirCandidates(workspaceRoot).find((candidate) =>
      existsSync(candidate),
    ) ?? workspaceRoot;

  return {
    workspaceRoot,
    binaryPath,
    binaryExists: existsSync(binaryPath),
    defaultProjectDir,
  };
}

export function requireWorkspacePath(
  workspaceRoot: string,
  candidatePath: string,
  label: string,
): string {
  if (!path.isAbsolute(candidatePath)) {
    throw new Error(`${label} must be an absolute path.`);
  }

  const resolvedPath = path.resolve(candidatePath);
  if (
    resolvedPath !== workspaceRoot &&
    !resolvedPath.startsWith(`${workspaceRoot}${path.sep}`)
  ) {
    throw new Error(`${label} must stay within ${workspaceRoot}.`);
  }

  return resolvedPath;
}

export function requireSvgPath(
  workspaceRoot: string,
  candidatePath: string,
  label: string,
): string {
  const resolvedPath = requireWorkspacePath(
    workspaceRoot,
    candidatePath,
    label,
  );
  if (path.extname(resolvedPath).toLowerCase() !== ".svg") {
    throw new Error(`${label} must point to an SVG file.`);
  }

  return resolvedPath;
}
