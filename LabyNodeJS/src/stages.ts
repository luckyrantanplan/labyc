import {
  type GridConfig,
  type GridStage,
  type NoiseConfig,
  type NoiseStage,
  type PipelineStage,
  type RenderConfig,
  type RenderStage,
  type RouteConfig,
  type RouteStage,
  type SourceStage,
  type StreamLineConfig,
  type StreamLineStage,
} from "./types.js";

export interface StageOptions {
  label?: string;
}

function withOptionalLabel<
  TStage extends
    | SourceStage
    | NoiseStage
    | GridStage
    | RouteStage
    | RenderStage
    | StreamLineStage,
>(stage: TStage, label: string | undefined): TStage {
  if (label === undefined) {
    return stage;
  }

  return {
    ...stage,
    label,
  };
}

export function source(
  sourcePath: string,
  options: StageOptions = {},
): SourceStage {
  return withOptionalLabel(
    {
      kind: "source",
      sourcePath,
    },
    options.label,
  );
}

export function noise(
  config: NoiseConfig,
  options: StageOptions = {},
): NoiseStage {
  return withOptionalLabel(
    {
      kind: "noise",
      config,
    },
    options.label,
  );
}

export function grid(
  config: GridConfig,
  options: StageOptions = {},
): GridStage {
  return withOptionalLabel(
    {
      kind: "grid",
      config,
    },
    options.label,
  );
}

export function route(
  config: RouteConfig,
  options: StageOptions = {},
): RouteStage {
  return withOptionalLabel(
    {
      kind: "route",
      config,
    },
    options.label,
  );
}

export function render(
  config: RenderConfig,
  options: StageOptions = {},
): RenderStage {
  return withOptionalLabel(
    {
      kind: "render",
      config,
    },
    options.label,
  );
}

export function streamline(
  config: StreamLineConfig,
  options: StageOptions = {},
): StreamLineStage {
  return withOptionalLabel(
    {
      kind: "streamline",
      config,
    },
    options.label,
  );
}

export function pipeline(
  ...stages: readonly PipelineStage[]
): readonly PipelineStage[] {
  return stages;
}
