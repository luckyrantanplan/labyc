import path from "node:path";
import { pathToFileURL } from "node:url";
import {
  grid,
  pipeline,
  render,
  route,
  runPipeline,
  source,
  type GridConfig,
  type RenderConfig,
  type RouteConfig
} from "../src/index.js";

export const defautGridConfig: GridConfig = {
  simplificationOfOriginalSVG: 0.1,
  maxSep: 5,
  minSep: 0.1,
  seed: 3
};

export const defautRouteConfig: RouteConfig = {
  initialThickness: 1.8,
  decrementFactor: 1.5,
  minimalThickness: 0.5,
  smoothingTension: 1,
  smoothingIteration: 3,
  maxRoutingAttempt: 300,
  routing: {
    seed: 5,
    maxRandom: 300,
    distanceUnitCost: 1,
    viaUnitCost: 10
  },
  cell: {
    seed: 1,
    maxPin: 400,
    startNet: 30,
    resolution: 1
  },
  enableAlternateRouting: false,
  alternateRouting: {
    maxThickness: 1.8,
    minThickness: 0.5,
    pruning: 0,
    thicknessPercent: 1,
    simplifyDist: 0
  }
};

export const defautRenderConfig: RenderConfig = {
  smoothingTension: 0.5,
  smoothingIterations: 3,
  penConfig: {
    thickness: 0.25,
    antisymmetricAmplitude: 0.3,
    antisymmetricFreq: 10,
    antisymmetricSeed: 5,
    symmetricAmplitude: 0.1,
    symmetricFreq: 3,
    symmetricSeed: 8,
    resolution: 1
  }
};

async function main(): Promise<void> {
  const workspaceRoot = path.resolve(process.cwd(), "..");
  const projectDir = path.join(workspaceRoot, "LabyData");
  const sourcePath = path.join(projectDir, "svg", "square_circleorig.svg");

  const result = await runPipeline(
    pipeline(
      source(sourcePath, { label: "Reference source" }),
      grid(defautGridConfig, { label: "Reference grid" }),
      route(defautRouteConfig, { label: "Reference route" }),
      render(defautRenderConfig, { label: "Reference render" })
    ),
    {
      projectDir,
      workspaceRoot,
      gallery: {
        enabled: true,
        keepAlive: true,
        openBrowser: true,
        title: "LabyNodeJS Reference Gallery"
      }
    }
  );

  if (result.galleryUrl !== undefined) {
    console.log(`Gallery: ${result.galleryUrl}`);
  }
  console.log(JSON.stringify(result, null, 2));
}

if (process.argv[1] !== undefined && import.meta.url === pathToFileURL(process.argv[1]).href) {
  void main();
}