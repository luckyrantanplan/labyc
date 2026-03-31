import { existsSync, promises as fs } from "node:fs";
import path from "node:path";
import {
  graphDocumentSchema,
  importedSourceName,
  type DirectoryEntry,
  type GraphDocument
} from "@labystudio/shared";

export async function listDirectory(dir: string): Promise<DirectoryEntry[]> {
  const entries = await fs.readdir(dir, { withFileTypes: true });
  return entries
    .filter((entry) => entry.name !== "." && entry.name !== "..")
    .map<DirectoryEntry>((entry) => ({
      name: entry.name,
      path: path.join(dir, entry.name),
      kind: entry.isDirectory() ? "directory" : "file",
      extension: entry.isDirectory() ? "" : path.extname(entry.name)
    }))
    .sort((left, right) => {
      if (left.kind !== right.kind) {
        return left.kind === "directory" ? -1 : 1;
      }

      return left.name.localeCompare(right.name);
    });
}

export async function readWorkspaceFile(filePath: string): Promise<string> {
  return fs.readFile(filePath, "utf8");
}

export async function writeWorkspaceFile(filePath: string, content: string): Promise<void> {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
  await fs.writeFile(filePath, content, "utf8");
}

export async function readSvgFile(filePath: string): Promise<string> {
  return fs.readFile(filePath, "utf8");
}

export async function copySvgIntoProject(sourcePath: string, projectDir: string): Promise<string> {
  await fs.mkdir(projectDir, { recursive: true });
  const destinationStem = path.basename(importedSourceName(sourcePath), ".svg");
  let index = 0;

  for (;;) {
    const suffix = index === 0 ? "" : `-${index}`;
    const destination = path.join(projectDir, `${destinationStem}${suffix}.svg`);
    const destinationExists = existsSync(destination);
    if (!destinationExists) {
      await fs.copyFile(sourcePath, destination);
      return destination;
    }

    index += 1;
  }
}

export async function loadGraphDocument(filePath: string): Promise<GraphDocument> {
  return graphDocumentSchema.parse(JSON.parse(await fs.readFile(filePath, "utf8")));
}

export async function saveGraphDocument(filePath: string, graph: GraphDocument): Promise<void> {
  await fs.mkdir(path.dirname(filePath), { recursive: true });
  await fs.writeFile(filePath, `${JSON.stringify(graph, null, 2)}\n`, "utf8");
}