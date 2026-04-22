import assert from "node:assert/strict";
import { mkdtemp, writeFile } from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import test from "node:test";
import { ensureCacheLayout, loadCacheManifest, saveCacheManifest } from "../src/cache.js";

void test("loadCacheManifest returns an empty manifest when the file is missing", async () => {
  const projectDir = await mkdtemp(path.join(os.tmpdir(), "labynodejs-cache-"));
  const layout = await ensureCacheLayout(projectDir);
  const manifest = await loadCacheManifest(layout.cachePath);

  assert.deepEqual(manifest, {
    version: 1,
    entries: {}
  });
});

void test("saveCacheManifest persists entries atomically", async () => {
  const projectDir = await mkdtemp(path.join(os.tmpdir(), "labynodejs-cache-"));
  const layout = await ensureCacheLayout(projectDir);
  const manifest = {
    version: 1 as const,
    entries: {
      abc: {
        cacheKey: "abc",
        stageKind: "grid" as const,
        binaryFingerprint: "bin",
        inputPath: "/tmp/in.svg",
        inputHash: "in",
        outputPath: "/tmp/out.svg",
        outputHash: "out",
        configPath: "/tmp/config.json",
        logPath: "/tmp/log.txt",
        payloadHash: "payload",
        payload: {},
        createdAt: "now",
        updatedAt: "now",
        status: "completed" as const
      }
    }
  };

  await saveCacheManifest(layout.cachePath, manifest);
  const reloaded = await loadCacheManifest(layout.cachePath);

  assert.deepEqual(reloaded, manifest);
});

void test("loadCacheManifest falls back to an empty manifest for invalid JSON", async () => {
  const projectDir = await mkdtemp(path.join(os.tmpdir(), "labynodejs-cache-"));
  const layout = await ensureCacheLayout(projectDir);
  await writeFile(layout.cachePath, "{not-json}\n", "utf8");

  const manifest = await loadCacheManifest(layout.cachePath);

  assert.deepEqual(manifest, {
    version: 1,
    entries: {}
  });
});