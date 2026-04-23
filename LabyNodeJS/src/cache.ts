import path from "node:path";
import { mkdir, readFile, rename, writeFile } from "node:fs/promises";
import type { CacheEntry, CacheManifest } from "./types.js";

export interface CacheLayout {
  svgDir: string;
  cacheDir: string;
  cachePath: string;
  configDir: string;
  logDir: string;
}

export function getCacheLayout(projectDir: string): CacheLayout {
  const cacheDir = path.join(projectDir, "cache");
  return {
    svgDir: path.join(projectDir, "svg"),
    cacheDir,
    cachePath: path.join(cacheDir, "cache.json"),
    configDir: path.join(projectDir, "configs"),
    logDir: path.join(projectDir, "logs"),
  };
}

export async function ensureCacheLayout(
  projectDir: string,
): Promise<CacheLayout> {
  const layout = getCacheLayout(projectDir);
  await mkdir(layout.svgDir, { recursive: true });
  await mkdir(layout.configDir, { recursive: true });
  await mkdir(layout.logDir, { recursive: true });
  await mkdir(layout.cacheDir, { recursive: true });
  return layout;
}

export async function loadCacheManifest(
  cachePath: string,
): Promise<CacheManifest> {
  try {
    const raw = await readFile(cachePath, "utf8");
    const parsed = JSON.parse(raw) as Partial<CacheManifest>;
    return {
      version: 1,
      entries: parsed.entries ?? {},
    };
  } catch {
    return {
      version: 1,
      entries: {},
    };
  }
}

export async function saveCacheManifest(
  cachePath: string,
  manifest: CacheManifest,
): Promise<void> {
  const directory = path.dirname(cachePath);
  await mkdir(directory, { recursive: true });
  const tempPath = `${cachePath}.tmp`;
  await writeFile(tempPath, `${JSON.stringify(manifest, null, 2)}\n`, "utf8");
  await rename(tempPath, cachePath);
}

export function setCacheEntry(
  manifest: CacheManifest,
  entry: CacheEntry,
): void {
  manifest.entries[entry.cacheKey] = entry;
}
