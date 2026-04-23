export type TransformerStageKind = "grid" | "route" | "render" | "streamline";
export type GeneratedStageKind = "noise" | TransformerStageKind;
export type StageKind = "source" | GeneratedStageKind;

export type NoisePreviewMode = "ARROWS" | "MAGNITUDE" | "NONE";

export interface GridConfig {
  simplificationOfOriginalSVG: number;
  maxSep: number;
  minSep: number;
  seed: number;
}

export interface NoiseConfig {
  maxN: number;
  accuracy: number;
  amplitude: number;
  seed: number;
  gaussianFrequency: number;
  powerlawFrequency: number;
  powerlawPower: number;
  complex: boolean;
  width: number;
  height: number;
  scale: number;
  previewMode: NoisePreviewMode;
  previewStride: number;
}

export interface RoutePlacementConfig {
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
}

export interface AlternateRouteConfig {
  maxThickness: number;
  minThickness: number;
  pruning: number;
  thicknessPercent: number;
  simplifyDist: number;
}

export interface RouteConfig {
  placement?: RoutePlacementConfig;
  alternateRouting?: AlternateRouteConfig;
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

export interface StreamLineConfig {
  // Polyline decimation tolerance applied after CGAL generates the streamlines.
  simplifyDistance: number;
  // CGAL saturation ratio controlling when a cavity is still large enough to seed a new streamline.
  dRat: number;
  // Desired streamline spacing in world units.
  divisor: number;
  // Final SVG stroke width, applied after streamline generation.
  strokeThickness: number;
}

export interface TransformerConfigByKind {
  grid: GridConfig;
  route: RouteConfig;
  render: RenderConfig;
  streamline: StreamLineConfig;
}

export interface SourceStage {
  kind: "source";
  sourcePath: string;
  label?: string;
}

export interface NoiseStage {
  kind: "noise";
  config: NoiseConfig;
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

export interface StreamLineStage {
  kind: "streamline";
  config: StreamLineConfig;
  label?: string;
}

export type TransformerStage =
  | GridStage
  | RouteStage
  | RenderStage
  | StreamLineStage;
export type GeneratedStage = NoiseStage | TransformerStage;
export type PipelineStage = SourceStage | GeneratedStage;

export interface RuntimeContext {
  workspaceRoot: string;
  binaryPath: string;
  binaryExists: boolean;
  defaultProjectDir: string;
}

export interface CacheEntry {
  cacheKey: string;
  stageKind: GeneratedStageKind;
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
  cacheReason?: string;
  configPath?: string;
  logPath?: string;
  cacheKey?: string;
  durationMs?: number;
  executionDurationMs?: number;
  inputHash?: string;
  outputHash?: string;
}

export interface PipelineRunResult {
  projectDir: string;
  workspaceRoot: string;
  binaryPath: string;
  binaryFingerprint: string;
  cachePath: string;
  durationMs: number;
  runLogPath: string;
  stages: StageArtifact[];
  galleryUrl?: string;
}

export interface PipelineGalleryOptions {
  enabled: boolean;
  openBrowser: boolean;
  keepAlive: boolean;
  port: number;
  title: string;
}

export interface RunPipelineOptions {
  projectDir?: string;
  workspaceRoot?: string;
  binaryPath?: string;
  force?: boolean;
  gallery: PipelineGalleryOptions;
}

export type GalleryStageStatus =
  | "waiting"
  | "running"
  | "completed"
  | "cached"
  | "failed";

export interface GalleryStageSnapshot {
  index: number;
  stageKind: StageKind;
  label: string;
  status: GalleryStageStatus;
  svgPath?: string;
  outputPath?: string;
  message?: string;
}
