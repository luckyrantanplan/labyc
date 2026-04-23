import type {
  GridConfig,
  NoiseConfig,
  RenderConfig,
  RouteConfig,
  StreamLineConfig,
} from "../src/types.js";

export const gridConfigFixture: GridConfig = {
  simplificationOfOriginalSVG: 0.1,
  maxSep: 5,
  minSep: 0.1,
  seed: 3,
};

export const routeConfigFixture: RouteConfig = {
  placement: {
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
      viaUnitCost: 10,
    },
    cell: {
      seed: 1,
      maxPin: 400,
      startNet: 30,
      resolution: 1,
    },
  },
  alternateRouting: {
    maxThickness: 1.8,
    minThickness: 0.5,
    pruning: 0,
    thicknessPercent: 1,
    simplifyDist: 0,
  },
};

export const renderConfigFixture: RenderConfig = {
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
    resolution: 1,
  },
};

export const noiseConfigFixture: NoiseConfig = {
  maxN: 16,
  accuracy: 8,
  amplitude: 1,
  seed: 7,
  gaussianFrequency: 2.5,
  powerlawFrequency: 1.2,
  powerlawPower: 2,
  complex: true,
  width: 32,
  height: 32,
  scale: 0.25,
  previewMode: "ARROWS",
  previewStride: 2,
};

export const streamLineConfigFixture: StreamLineConfig = {
  resolution: 0,
  simplifyDistance: 0.05,
  dRat: 1,
  epsilon: 0.01,
  size: 8,
  divisor: 0.45,
  strokeThickness: 0.1,
};
