import path from "node:path";
import type {
  GridConfig,
  RenderConfig,
  RouteConfig,
  TransformerStage,
  TransformerStageKind,
} from "./types.js";

export function stemName(filePath: string): string {
  return path.basename(filePath, path.extname(filePath));
}

export function artifactStem(
  inputPath: string,
  kind: TransformerStageKind,
  cacheKey: string,
): string {
  return `${stemName(inputPath)}--${kind}--${cacheKey.slice(0, 12)}`;
}

export function buildGridConfigPayload(
  inputPath: string,
  outputPath: string,
  config: GridConfig,
): Record<string, unknown> {
  return {
    skeletonGrid: {
      outputfile: outputPath,
      inputfile: inputPath,
      simplificationOfOriginalSVG: config.simplificationOfOriginalSVG,
      maxSep: config.maxSep,
      minSep: config.minSep,
      seed: config.seed,
    },
  };
}

export function buildRouteConfigPayload(
  inputPath: string,
  outputPath: string,
  config: RouteConfig,
): Record<string, unknown> {
  const routing: {
    filepaths: {
      outputfile: string;
      inputfile: string;
    };
    placement?: Record<string, unknown>;
    alternateRouting?: Record<string, unknown>;
  } = {
    filepaths: {
      outputfile: outputPath,
      inputfile: inputPath,
    },
  };

  if (config.placement !== undefined) {
    routing.placement = {
      initialThickness: config.placement.initialThickness,
      decrementFactor: config.placement.decrementFactor,
      minimalThickness: config.placement.minimalThickness,
      smoothingTension: config.placement.smoothingTension,
      smoothingIteration: config.placement.smoothingIteration,
      maxRoutingAttempt: String(config.placement.maxRoutingAttempt),
      routing: {
        seed: config.placement.routing.seed,
        maxRandom: config.placement.routing.maxRandom,
        distanceUnitCost: config.placement.routing.distanceUnitCost,
        viaUnitCost: config.placement.routing.viaUnitCost,
      },
      cell: {
        seed: config.placement.cell.seed,
        maxPin: config.placement.cell.maxPin,
        startNet: config.placement.cell.startNet,
        resolution: config.placement.cell.resolution,
      },
    };
  }

  if (config.alternateRouting !== undefined) {
    routing.alternateRouting = {
      maxThickness: config.alternateRouting.maxThickness,
      minThickness: config.alternateRouting.minThickness,
      pruning: config.alternateRouting.pruning,
      thicknessPercent: config.alternateRouting.thicknessPercent,
      simplifyDist: config.alternateRouting.simplifyDist,
    };
  }

  return { routing };
}

export function buildRenderConfigPayload(
  inputPath: string,
  outputPath: string,
  config: RenderConfig,
): Record<string, unknown> {
  return {
    gGraphicRendering: {
      outputfile: outputPath,
      inputfile: inputPath,
      smoothingTension: config.smoothingTension,
      smoothingIterations: config.smoothingIterations,
      penConfig: {
        thickness: config.penConfig.thickness,
        antisymmetricAmplitude: config.penConfig.antisymmetricAmplitude,
        antisymmetricFreq: config.penConfig.antisymmetricFreq,
        antisymmetricSeed: config.penConfig.antisymmetricSeed,
        symmetricAmplitude: config.penConfig.symmetricAmplitude,
        symmetricFreq: config.penConfig.symmetricFreq,
        symmetricSeed: config.penConfig.symmetricSeed,
        resolution: config.penConfig.resolution,
      },
    },
  };
}

export function buildStagePayload(
  stage: TransformerStage,
  inputPath: string,
  outputPath: string,
): Record<string, unknown> {
  switch (stage.kind) {
    case "grid":
      return buildGridConfigPayload(inputPath, outputPath, stage.config);
    case "route":
      return buildRouteConfigPayload(inputPath, outputPath, stage.config);
    case "render":
      return buildRenderConfigPayload(inputPath, outputPath, stage.config);
  }
}
