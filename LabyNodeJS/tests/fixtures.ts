import type { GridConfig, RenderConfig, RouteConfig } from "../src/types.js";

export const gridConfigFixture: GridConfig = {
  simplificationOfOriginalSVG: 0.1,
  maxSep: 5,
  minSep: 0.1,
  seed: 3
};

export const routeConfigFixture: RouteConfig = {
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
    resolution: 1
  }
};