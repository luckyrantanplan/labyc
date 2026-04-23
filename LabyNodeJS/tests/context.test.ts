import assert from "node:assert/strict";
import { mkdtemp, mkdir } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import {
  findWorkspaceRoot,
  requireSvgPath,
  requireWorkspacePath,
  resolveContext,
} from "../src/context.js";

void test("findWorkspaceRoot locates the repo boundary", async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "labynodejs-root-"));
  const nested = path.join(root, "a", "b", "c");
  await mkdir(path.join(root, "LabyPath"), { recursive: true });
  await mkdir(path.join(root, "LabyPython"), { recursive: true });
  await mkdir(nested, { recursive: true });

  assert.equal(findWorkspaceRoot(nested), root);
});

void test("requireWorkspacePath rejects paths outside the workspace", () => {
  assert.throws(
    () =>
      requireWorkspacePath("/workspace/root", "/tmp/outside.svg", "Candidate"),
    /must stay within/,
  );
});

void test("requireWorkspacePath rejects relative paths", () => {
  assert.throws(
    () =>
      requireWorkspacePath("/workspace/root", "relative/file.svg", "Candidate"),
    /must be an absolute path/,
  );
});

void test("requireSvgPath rejects non-SVG files", () => {
  assert.throws(
    () =>
      requireSvgPath(
        "/workspace/root",
        "/workspace/root/file.json",
        "Candidate",
      ),
    /must point to an SVG file/,
  );
});

void test("resolveContext falls back when no binary exists yet", async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "labynodejs-context-"));
  await mkdir(path.join(root, "LabyPath", "input"), { recursive: true });
  await mkdir(path.join(root, "LabyPython"), { recursive: true });

  const context = resolveContext(root);

  assert.equal(context.workspaceRoot, root);
  assert.equal(context.binaryExists, false);
  assert.equal(context.defaultProjectDir, path.join(root, "LabyPath", "input"));
});

void test("findWorkspaceRoot returns the input path when no repo markers exist", async () => {
  const root = await mkdtemp(path.join(os.tmpdir(), "labynodejs-no-root-"));

  assert.equal(findWorkspaceRoot(root), root);
});
