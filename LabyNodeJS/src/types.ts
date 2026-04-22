export type TransformerStageKind = "grid" | "route" | "render";
export type StageKind = "source" | TransformerStageKind;

export interface GridConfig {
  simplificationOfOriginalSVG: number;
  maxSep: number;
  minSep: number;
  seed: number;
}

export interface RouteConfig {
  initialThickness: number;
  decrementFactor: number;
  minimalThickness: number;
  smoothingTension: number;
  smoothingIteration: number;
  maxRoutingAttempt: number;
  routing: {
    seed: number;
    maxRandom: number;
    distanceUnitCost: number;
    viaUnitCost: number;
  };
  cell: {
    seed: number;
    maxPin: number;
    startNet: number;
    resolution: number;
  };
  enableAlternateRouting: boolean;
  alternateRouting: {
    maxThickness: number;
    minThickness: number;
    pruning: number;
    thicknessPercent: number;
    simplifyDist: number;
  };
}

export interface RenderConfig {
  smoothingTension: number;
  smoothingIterations: number;
  penConfig: {
    thickness: number;
    antisymmetricAmplitude: number;
    antisymmetricFreq: number;
    antisymmetricSeed: number;
    symmetricAmplitude: number;
    symmetricFreq: number;
    symmetricSeed: number;
    resolution: number;
  };
}

export interface TransformerConfigByKind {
  grid: GridConfig;
  route: RouteConfig;
  render: RenderConfig;
}

export interface SourceStage {
  kind: "source";
  sourcePath: string;
  label?: string;
}

export interface GridStage {
  kind: "grid";
  config: GridConfig;
  label?: string;
}

export interface RouteStage {
  kind: "route";
  config: RouteConfig;
  label?: string;
}

export interface RenderStage {
  kind: "render";
  config: RenderConfig;
  label?: string;
}

export type TransformerStage = GridStage | RouteStage | RenderStage;
export type PipelineStage = SourceStage | TransformerStage;

export interface RuntimeContext {
  workspaceRoot: string;
  binaryPath: string;
  binaryExists: boolean;
  defaultProjectDir: string;
}

export interface CacheEntry {
  cacheKey: string;
  stageKind: TransformerStageKind;
  binaryFingerprint: string;
  inputPath: string;
  inputHash: string;
  outputPath: string;
  outputHash: string;
  configPath: string;
  logPath: string;
  payloadHash: string;
  payload: Record<string, unknown>;
  createdAt: string;
  updatedAt: string;
  status: "completed";
}

export interface CacheManifest {
  version: 1;
  entries: Record<string, CacheEntry>;
}

export interface StageArtifact {
  stageKind: StageKind;
  label?: string;
  inputPath: string;
  outputPath: string;
  cached: boolean;
  configPath?: string;
  logPath?: string;
  cacheKey?: string;
  inputHash?: string;
  outputHash?: string;
}

export interface PipelineRunResult {
  projectDir: string;
  workspaceRoot: string;
  binaryPath: string;
  binaryFingerprint: string;
  cachePath: string;
  stages: StageArtifact[];
  galleryUrl?: string;
}

export interface RunPipelineOptions {
  projectDir?: string;
  workspaceRoot?: string;
  binaryPath?: string;
  force?: boolean;
  gallery?: {
    enabled?: boolean;
    openBrowser?: boolean;
    keepAlive?: boolean;
    port?: number;
    title?: string;
  };
}

export type GalleryStageStatus = "waiting" | "running" | "completed" | "cached" | "failed";

export interface GalleryStageSnapshot {
  index: number;
  stageKind: StageKind;
  label: string;
  status: GalleryStageStatus;
  svgPath?: string;
  outputPath?: string;
  message?: string;
}