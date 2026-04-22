import path from "node:path";
import { grid, pipeline, render, route, runPipeline, source } from "../src/index.js";
import { defautGridConfig, defautRenderConfig, defautRouteConfig } from "./defaut.js";

async function main(): Promise<void> {
  const workspaceRoot = path.resolve(process.cwd(), "..");
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "square_circleorig.svg");

  const result = await runPipeline(
    pipeline(
      source(sourcePath, { label: "Example source" }),
      grid(defautGridConfig),
      route(defautRouteConfig),
      render(defautRenderConfig)
    ),
    {
      projectDir,
      workspaceRoot,
      gallery: {
        enabled: true,
        keepAlive: true,
        openBrowser: true,
        title: "LabyNodeJS Example Gallery"
      }
    }
  );

  if (result.galleryUrl !== undefined) {
    console.log(`Gallery: ${result.galleryUrl}`);
  }
  console.log(JSON.stringify(result, null, 2));
}

void main();