import path from "node:path";
import type {
  GridConfig,
  RenderConfig,
  RouteConfig,
  TransformerStage,
  TransformerStageKind
} from "./types.js";

export function stemName(filePath: string): string {
  return path.basename(filePath, path.extname(filePath));
}

export function artifactStem(inputPath: string, kind: TransformerStageKind, cacheKey: string): string {
  return `${stemName(inputPath)}--${kind}--${cacheKey.slice(0, 12)}`;
}

export function buildGridConfigPayload(inputPath: string, outputPath: string, config: GridConfig): Record<string, unknown> {
  return {
    skeletonGrid: {
      outputfile: outputPath,
      inputfile: inputPath,
      simplificationOfOriginalSVG: config.simplificationOfOriginalSVG,
      maxSep: config.maxSep,
      minSep: config.minSep,
      seed: config.seed
    }
  };
}

export function buildRouteConfigPayload(inputPath: string, outputPath: string, config: RouteConfig): Record<string, unknown> {
  const routing = {
    filepaths: {
      outputfile: outputPath,
      inputfile: inputPath
    },
    placement: {
      initialThickness: config.initialThickness,
      decrementFactor: config.decrementFactor,
      minimalThickness: config.minimalThickness,
      smoothingTension: config.smoothingTension,
      smoothingIteration: config.smoothingIteration,
      maxRoutingAttempt: String(config.maxRoutingAttempt),
      routing: {
        seed: config.routing.seed,
        maxRandom: config.routing.maxRandom,
        distanceUnitCost: config.routing.distanceUnitCost,
        viaUnitCost: config.routing.viaUnitCost
      },
      cell: {
        seed: config.cell.seed,
        maxPin: config.cell.maxPin,
        startNet: config.cell.startNet,
        resolution: config.cell.resolution
      }
    },
    alternateRouting: undefined as Record<string, unknown> | undefined
  };
  if (config.enableAlternateRouting) {
    routing.alternateRouting = {
      maxThickness: config.alternateRouting.maxThickness,
      minThickness: config.alternateRouting.minThickness,
      pruning: config.alternateRouting.pruning,
      thicknessPercent: config.alternateRouting.thicknessPercent,
      simplifyDist: config.alternateRouting.simplifyDist
    };
  }

  return { routing };
}

export function buildRenderConfigPayload(inputPath: string, outputPath: string, config: RenderConfig): Record<string, unknown> {
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
        resolution: config.penConfig.resolution
      }
    }
  };
}

export function buildStagePayload(stage: TransformerStage, inputPath: string, outputPath: string): Record<string, unknown> {
  switch (stage.kind) {
    case "grid":
      return buildGridConfigPayload(inputPath, outputPath, stage.config);
    case "route":
      return buildRouteConfigPayload(inputPath, outputPath, stage.config);
    case "render":
      return buildRenderConfigPayload(inputPath, outputPath, stage.config);
  }
}