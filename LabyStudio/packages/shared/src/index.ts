import { z } from "zod";

export type TransformerNodeKind = "grid" | "route" | "render";
export type NumericNodeKind = "numericConstant" | "operation" | "broadcast";
export type ExecutionNodeKind = "source" | TransformerNodeKind;
export type NodeKind = ExecutionNodeKind | NumericNodeKind;
export type ArtifactStatus = "idle" | "queued" | "running" | "completed" | "failed";
export type OperationKind = "add" | "multiply";

function normalizePath(filePath: string): string {
  if (!filePath.trim()) {
    return "";
  }

  return filePath.replaceAll("\\", "/");
}

function joinPath(...segments: string[]): string {
  const joinedPath = segments.filter(Boolean).join("/");
  if (!joinedPath) {
    return "";
  }

  return normalizePath(joinedPath).replace(/\/+/g, "/");
}

function baseName(filePath: string): string {
  if (!filePath.trim()) {
    return "";
  }

  const normalized = normalizePath(filePath);
  const segments = normalized.split("/");
  return segments[segments.length - 1] ?? normalized;
}

function extensionName(filePath: string): string {
  const name = baseName(filePath);
  const dotIndex = name.lastIndexOf(".");
  return dotIndex >= 0 ? name.slice(dotIndex) : "";
}

function stemName(filePath: string): string {
  const name = baseName(filePath);
  const ext = extensionName(name);
  return ext ? name.slice(0, -ext.length) : name;
}

export const gridConfigSchema = z.object({
  simplificationOfOriginalSVG: z.number(),
  maxSep: z.number(),
  minSep: z.number(),
  seed: z.number().int()
});

export const routeConfigSchema = z.object({
  initialThickness: z.number(),
  decrementFactor: z.number(),
  minimalThickness: z.number(),
  smoothingTension: z.number(),
  smoothingIteration: z.number().int(),
  maxRoutingAttempt: z.number().int(),
  routing: z.object({
    seed: z.number().int(),
    maxRandom: z.number().int(),
    distanceUnitCost: z.number().int(),
    viaUnitCost: z.number().int()
  }),
  cell: z.object({
    seed: z.number().int(),
    maxPin: z.number().int(),
    startNet: z.number().int(),
    resolution: z.number()
  }),
  enableAlternateRouting: z.boolean(),
  alternateRouting: z.object({
    maxThickness: z.number(),
    minThickness: z.number(),
    pruning: z.number().int(),
    thicknessPercent: z.number(),
    simplifyDist: z.number()
  })
});

export const renderConfigSchema = z.object({
  smoothingTension: z.number(),
  smoothingIterations: z.number(),
  penConfig: z.object({
    thickness: z.number(),
    antisymmetricAmplitude: z.number(),
    antisymmetricFreq: z.number(),
    antisymmetricSeed: z.number(),
    symmetricAmplitude: z.number(),
    symmetricFreq: z.number(),
    symmetricSeed: z.number(),
    resolution: z.number()
  })
});

export type GridConfig = z.infer<typeof gridConfigSchema>;
export type RouteConfig = z.infer<typeof routeConfigSchema>;
export type RenderConfig = z.infer<typeof renderConfigSchema>;

export type NumberFieldConfig<TConfig> = {
  id: string;
  label: string;
  step?: string;
  getValue: (config: TConfig) => number;
  setValue: (config: TConfig, value: number) => TConfig;
};

export type ToggleFieldGroup<TConfig> = {
  label: string;
  isEnabled: (config: TConfig) => boolean;
  setEnabled: (config: TConfig, enabled: boolean) => TConfig;
  fields: readonly NumberFieldConfig<TConfig>[];
};

const gridParameterFields: readonly NumberFieldConfig<GridConfig>[] = [
  {
    id: "simplificationOfOriginalSVG",
    label: "Simplification",
    step: "0.1",
    getValue: (config) => config.simplificationOfOriginalSVG,
    setValue: (config, value) => ({ ...config, simplificationOfOriginalSVG: value })
  },
  {
    id: "maxSep",
    label: "Max separation",
    step: "0.1",
    getValue: (config) => config.maxSep,
    setValue: (config, value) => ({ ...config, maxSep: value })
  },
  {
    id: "minSep",
    label: "Min separation",
    step: "0.1",
    getValue: (config) => config.minSep,
    setValue: (config, value) => ({ ...config, minSep: value })
  },
  {
    id: "seed",
    label: "Seed",
    getValue: (config) => config.seed,
    setValue: (config, value) => ({ ...config, seed: value })
  }
];

const routeParameterFields: readonly NumberFieldConfig<RouteConfig>[] = [
  {
    id: "initialThickness",
    label: "Initial thickness",
    step: "0.1",
    getValue: (config) => config.initialThickness,
    setValue: (config, value) => ({ ...config, initialThickness: value })
  },
  {
    id: "decrementFactor",
    label: "Decrement factor",
    step: "0.1",
    getValue: (config) => config.decrementFactor,
    setValue: (config, value) => ({ ...config, decrementFactor: value })
  },
  {
    id: "minimalThickness",
    label: "Minimal thickness",
    step: "0.1",
    getValue: (config) => config.minimalThickness,
    setValue: (config, value) => ({ ...config, minimalThickness: value })
  },
  {
    id: "smoothingTension",
    label: "Smoothing tension",
    step: "0.1",
    getValue: (config) => config.smoothingTension,
    setValue: (config, value) => ({ ...config, smoothingTension: value })
  },
  {
    id: "smoothingIteration",
    label: "Smoothing iteration",
    getValue: (config) => config.smoothingIteration,
    setValue: (config, value) => ({ ...config, smoothingIteration: value })
  },
  {
    id: "maxRoutingAttempt",
    label: "Max routing attempt",
    getValue: (config) => config.maxRoutingAttempt,
    setValue: (config, value) => ({ ...config, maxRoutingAttempt: value })
  },
  {
    id: "routing.seed",
    label: "Routing seed",
    getValue: (config) => config.routing.seed,
    setValue: (config, value) => ({ ...config, routing: { ...config.routing, seed: value } })
  },
  {
    id: "routing.maxRandom",
    label: "Routing max random",
    getValue: (config) => config.routing.maxRandom,
    setValue: (config, value) => ({ ...config, routing: { ...config.routing, maxRandom: value } })
  },
  {
    id: "routing.distanceUnitCost",
    label: "Distance unit cost",
    getValue: (config) => config.routing.distanceUnitCost,
    setValue: (config, value) => ({ ...config, routing: { ...config.routing, distanceUnitCost: value } })
  },
  {
    id: "routing.viaUnitCost",
    label: "Via unit cost",
    getValue: (config) => config.routing.viaUnitCost,
    setValue: (config, value) => ({ ...config, routing: { ...config.routing, viaUnitCost: value } })
  },
  {
    id: "cell.seed",
    label: "Cell seed",
    getValue: (config) => config.cell.seed,
    setValue: (config, value) => ({ ...config, cell: { ...config.cell, seed: value } })
  },
  {
    id: "cell.maxPin",
    label: "Cell max pin",
    getValue: (config) => config.cell.maxPin,
    setValue: (config, value) => ({ ...config, cell: { ...config.cell, maxPin: value } })
  },
  {
    id: "cell.startNet",
    label: "Cell start net",
    getValue: (config) => config.cell.startNet,
    setValue: (config, value) => ({ ...config, cell: { ...config.cell, startNet: value } })
  },
  {
    id: "cell.resolution",
    label: "Cell resolution",
    step: "0.1",
    getValue: (config) => config.cell.resolution,
    setValue: (config, value) => ({ ...config, cell: { ...config.cell, resolution: value } })
  }
];

const alternateRouteParameterFields: readonly NumberFieldConfig<RouteConfig>[] = [
  {
    id: "alternateRouting.maxThickness",
    label: "Max thickness",
    step: "0.1",
    getValue: (config) => config.alternateRouting.maxThickness,
    setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, maxThickness: value } })
  },
  {
    id: "alternateRouting.minThickness",
    label: "Min thickness",
    step: "0.1",
    getValue: (config) => config.alternateRouting.minThickness,
    setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, minThickness: value } })
  },
  {
    id: "alternateRouting.pruning",
    label: "Pruning",
    getValue: (config) => config.alternateRouting.pruning,
    setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, pruning: value } })
  },
  {
    id: "alternateRouting.thicknessPercent",
    label: "Thickness percent",
    step: "0.1",
    getValue: (config) => config.alternateRouting.thicknessPercent,
    setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, thicknessPercent: value } })
  },
  {
    id: "alternateRouting.simplifyDist",
    label: "Simplify distance",
    step: "0.1",
    getValue: (config) => config.alternateRouting.simplifyDist,
    setValue: (config, value) => ({ ...config, alternateRouting: { ...config.alternateRouting, simplifyDist: value } })
  }
];

const renderParameterFields: readonly NumberFieldConfig<RenderConfig>[] = [
  {
    id: "smoothingTension",
    label: "Smoothing tension",
    step: "0.1",
    getValue: (config) => config.smoothingTension,
    setValue: (config, value) => ({ ...config, smoothingTension: value })
  },
  {
    id: "smoothingIterations",
    label: "Smoothing iterations",
    step: "0.1",
    getValue: (config) => config.smoothingIterations,
    setValue: (config, value) => ({ ...config, smoothingIterations: value })
  },
  {
    id: "penConfig.thickness",
    label: "Pen thickness",
    step: "0.05",
    getValue: (config) => config.penConfig.thickness,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, thickness: value } })
  },
  {
    id: "penConfig.antisymmetricAmplitude",
    label: "Antisymmetric amplitude",
    step: "0.1",
    getValue: (config) => config.penConfig.antisymmetricAmplitude,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricAmplitude: value } })
  },
  {
    id: "penConfig.antisymmetricFreq",
    label: "Antisymmetric frequency",
    step: "0.1",
    getValue: (config) => config.penConfig.antisymmetricFreq,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricFreq: value } })
  },
  {
    id: "penConfig.antisymmetricSeed",
    label: "Antisymmetric seed",
    step: "0.1",
    getValue: (config) => config.penConfig.antisymmetricSeed,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricSeed: value } })
  },
  {
    id: "penConfig.symmetricAmplitude",
    label: "Symmetric amplitude",
    step: "0.1",
    getValue: (config) => config.penConfig.symmetricAmplitude,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, symmetricAmplitude: value } })
  },
  {
    id: "penConfig.symmetricFreq",
    label: "Symmetric frequency",
    step: "0.1",
    getValue: (config) => config.penConfig.symmetricFreq,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, symmetricFreq: value } })
  },
  {
    id: "penConfig.symmetricSeed",
    label: "Symmetric seed",
    step: "0.1",
    getValue: (config) => config.penConfig.symmetricSeed,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, symmetricSeed: value } })
  },
  {
    id: "penConfig.resolution",
    label: "Resolution",
    step: "0.1",
    getValue: (config) => config.penConfig.resolution,
    setValue: (config, value) => ({ ...config, penConfig: { ...config.penConfig, resolution: value } })
  }
];

export const routeToggleFieldGroup: ToggleFieldGroup<RouteConfig> = {
  label: "Enable alternate routing",
  isEnabled: (config) => config.enableAlternateRouting,
  setEnabled: (config, enabled) => ({ ...config, enableAlternateRouting: enabled }),
  fields: alternateRouteParameterFields
};

export const transformerParameterDefinitions = {
  grid: gridParameterFields,
  route: [...routeParameterFields, ...alternateRouteParameterFields],
  render: renderParameterFields
} satisfies {
  grid: readonly NumberFieldConfig<GridConfig>[];
  route: readonly NumberFieldConfig<RouteConfig>[];
  render: readonly NumberFieldConfig<RenderConfig>[];
};

export type TransformerConfigByKind = {
  grid: GridConfig;
  route: RouteConfig;
  render: RenderConfig;
};

export function getTransformerParameterDefinitions<K extends TransformerNodeKind>(kind: K): readonly NumberFieldConfig<TransformerConfigByKind[K]>[] {
  return transformerParameterDefinitions[kind] as unknown as readonly NumberFieldConfig<TransformerConfigByKind[K]>[];
}

export function getTransformerParameterField<K extends TransformerNodeKind>(kind: K, fieldId: string): NumberFieldConfig<TransformerConfigByKind[K]> | undefined {
  return getTransformerParameterDefinitions(kind).find((field) => field.id === fieldId);
}

export const artifactStateSchema = z.object({
  inputPath: z.string().optional(),
  outputPath: z.string().optional(),
  configPath: z.string().optional(),
  logPath: z.string().optional(),
  status: z.enum(["idle", "queued", "running", "completed", "failed"]).optional(),
  lastRunAt: z.string().optional()
});

export type ArtifactState = z.infer<typeof artifactStateSchema>;

function createIdleArtifacts(): ArtifactState {
  return { status: "idle" };
}

const baseNodeDataSchema = z.object({
  label: z.string(),
  artifacts: artifactStateSchema.optional()
});

export const sourceNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("source"),
  sourcePath: z.string()
});

export const gridNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("grid"),
  config: gridConfigSchema
});

export const routeNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("route"),
  config: routeConfigSchema
});

export const renderNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("render"),
  config: renderConfigSchema
});

export const numericConstantNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("numericConstant"),
  value: z.number()
});

export const operationNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("operation"),
  operation: z.enum(["add", "multiply"]),
  left: z.number(),
  right: z.number()
});

export const broadcastNodeDataSchema = baseNodeDataSchema.extend({
  kind: z.literal("broadcast"),
  value: z.number(),
  outputs: z.number().int().min(2).max(8)
});

export const workflowNodeDataSchema = z.discriminatedUnion("kind", [
  sourceNodeDataSchema,
  gridNodeDataSchema,
  routeNodeDataSchema,
  renderNodeDataSchema,
  numericConstantNodeDataSchema,
  operationNodeDataSchema,
  broadcastNodeDataSchema
]);

export type WorkflowNodeData = z.infer<typeof workflowNodeDataSchema>;
export type WorkflowNode = z.infer<typeof graphNodeSchema>;

export const graphNodeSchema = z.object({
  id: z.string(),
  type: z.string().default("workflow"),
  position: z.object({
    x: z.number(),
    y: z.number()
  }),
  data: workflowNodeDataSchema
});

export const graphEdgeSchema = z.object({
  id: z.string(),
  source: z.string(),
  target: z.string(),
  kind: z.enum(["artifact", "parameter"]).default("artifact"),
  sourceHandle: z.string().optional(),
  targetHandle: z.string().optional()
});

export type GraphEdge = z.infer<typeof graphEdgeSchema>;

export const graphDocumentSchema = z.object({
  version: z.literal(1),
  nodes: z.array(graphNodeSchema),
  edges: z.array(graphEdgeSchema)
});

export type GraphDocument = z.infer<typeof graphDocumentSchema>;

export const directoryEntrySchema = z.object({
  name: z.string(),
  path: z.string(),
  kind: z.enum(["file", "directory"]),
  extension: z.string().optional()
});

export const directoryListingSchema = z.object({
  dir: z.string(),
  entries: z.array(directoryEntrySchema)
});

export type DirectoryEntry = z.infer<typeof directoryEntrySchema>;
export type DirectoryListing = z.infer<typeof directoryListingSchema>;

export const runtimeContextSchema = z.object({
  workspaceRoot: z.string(),
  binaryPath: z.string(),
  binaryExists: z.boolean(),
  defaultProjectDir: z.string()
});

export type RuntimeContext = z.infer<typeof runtimeContextSchema>;

export const graphRunNodeResultSchema = z.object({
  nodeId: z.string(),
  artifacts: artifactStateSchema
});

export const graphRunResultSchema = z.object({
  nodes: z.array(graphRunNodeResultSchema)
});

export type GraphRunNodeResult = z.infer<typeof graphRunNodeResultSchema>;
export type GraphRunResult = z.infer<typeof graphRunResultSchema>;

export const workflowJobSchema = z.object({
  id: z.string(),
  status: z.enum(["queued", "running", "completed", "failed"]),
  projectDir: z.string(),
  targetNodeId: z.string(),
  createdAt: z.string(),
  updatedAt: z.string(),
  currentNodeId: z.string().optional(),
  logPath: z.string().optional(),
  error: z.string().optional(),
  result: graphRunResultSchema.optional()
});

export type WorkflowJob = z.infer<typeof workflowJobSchema>;

export const jobRunRequestSchema = z.object({
  graph: graphDocumentSchema,
  projectDir: z.string(),
  targetNodeId: z.string()
});

const NEXT_KIND: Record<NodeKind, NodeKind[]> = {
  source: ["grid"],
  grid: ["route"],
  route: ["render"],
  render: [],
  numericConstant: [],
  operation: [],
  broadcast: []
};

export function canConnectNodeKinds(sourceKind: NodeKind, targetKind: NodeKind): boolean {
  return isExecutionNodeKind(sourceKind) && isTransformerNodeKind(targetKind)
    ? NEXT_KIND[sourceKind].includes(targetKind)
    : false;
}

export function isTransformerNodeKind(kind: NodeKind): kind is TransformerNodeKind {
  return kind === "grid" || kind === "route" || kind === "render";
}

export function isExecutionNodeKind(kind: NodeKind): kind is ExecutionNodeKind {
  return kind === "source" || isTransformerNodeKind(kind);
}

export function isNumericNodeKind(kind: NodeKind): kind is NumericNodeKind {
  return kind === "numericConstant" || kind === "operation" || kind === "broadcast";
}

export function isTransformerNodeData(data: WorkflowNodeData): data is z.infer<typeof gridNodeDataSchema> | z.infer<typeof routeNodeDataSchema> | z.infer<typeof renderNodeDataSchema> {
  return isTransformerNodeKind(data.kind);
}

export function isNumericNodeData(data: WorkflowNodeData): data is z.infer<typeof numericConstantNodeDataSchema> | z.infer<typeof operationNodeDataSchema> | z.infer<typeof broadcastNodeDataSchema> {
  return isNumericNodeKind(data.kind);
}

export function createDefaultNodeData(kind: NodeKind): WorkflowNodeData {
  if (kind === "source") {
    return {
      kind,
      label: "SVG source",
      sourcePath: "",
      artifacts: createIdleArtifacts()
    };
  }

  if (kind === "grid") {
    return {
      kind,
      label: "Grid",
      config: {
        simplificationOfOriginalSVG: 0.1,
        maxSep: 5,
        minSep: 0.1,
        seed: 3
      },
      artifacts: createIdleArtifacts()
    };
  }

  if (kind === "route") {
    return {
      kind,
      label: "Route",
      config: {
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
      },
      artifacts: createIdleArtifacts()
    };
  }

  if (kind === "numericConstant") {
    return {
      kind,
      label: "Constant",
      value: 1
    };
  }

  if (kind === "operation") {
    return {
      kind,
      label: "Add",
      operation: "add",
      left: 1,
      right: 1
    };
  }

  if (kind === "broadcast") {
    return {
      kind,
      label: "Broadcast",
      value: 1,
      outputs: 3
    };
  }

  return {
    kind,
    label: "Render",
    config: {
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
    },
    artifacts: createIdleArtifacts()
  };
}

export function listBinaryCandidates(workspaceRoot: string): string[] {
  return [
    joinPath(workspaceRoot, ".cmake", "build", "LabyPath", "labypath"),
    joinPath(workspaceRoot, "LabyPath", "build", "labypath"),
    joinPath(workspaceRoot, "LabyPath", "Debug", "LabyPath")
  ];
}

export function defaultProjectDirCandidates(workspaceRoot: string): string[] {
  return [
    joinPath(workspaceRoot, "LabyPath", "input"),
    joinPath(workspaceRoot, "LabyPath"),
    workspaceRoot
  ];
}

export function importedSourceName(sourcePath: string): string {
  const stem = stemName(sourcePath);
  if (!stem) {
    throw new Error("Cannot derive an imported SVG name from an empty source path.");
  }

  return `${stem}orig.svg`;
}

export function stageStem(inputPath: string, kind: TransformerNodeKind, index: number): string {
  const stem = stemName(inputPath);
  if (!stem) {
    throw new Error(`Cannot derive a stage name for ${kind} from an empty input path.`);
  }

  return `${stem}${index}${kind}`;
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
  const payload: Record<string, unknown> = {
    routing: {
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
      }
    }
  };

  if (config.enableAlternateRouting) {
    (payload.routing as Record<string, unknown>).alternateRouting = {
      maxThickness: config.alternateRouting.maxThickness,
      minThickness: config.alternateRouting.minThickness,
      pruning: config.alternateRouting.pruning,
      thicknessPercent: config.alternateRouting.thicknessPercent,
      simplifyDist: config.alternateRouting.simplifyDist
    };
  }

  return payload;
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

export const SVG_INPUT_HANDLE = "svg-in";
export const SVG_OUTPUT_HANDLE = "svg-out";
export const NUMERIC_CONSTANT_OUTPUT_HANDLE = "value";
export const OPERATION_LEFT_HANDLE = "left";
export const OPERATION_RIGHT_HANDLE = "right";
export const OPERATION_RESULT_HANDLE = "result";
export const BROADCAST_INPUT_HANDLE = "input";

export function broadcastOutputHandle(index: number): string {
  return `output:${index + 1}`;
}

export function toParameterInputHandle(fieldId: string): string {
  return `param-in:${fieldId}`;
}

export function toParameterOutputHandle(fieldId: string): string {
  return `param-out:${fieldId}`;
}

function removeHandlePrefix(handleId: string, prefix: string): string | undefined {
  if (!handleId.startsWith(prefix)) {
    return undefined;
  }

  const value = handleId.slice(prefix.length);
  return value.length > 0 ? value : undefined;
}

export function numericTargetHandleToSlotId(nodeData: WorkflowNodeData, handleId?: string): string | undefined {
  if (!handleId) {
    return undefined;
  }

  if (isTransformerNodeData(nodeData)) {
    const fieldId = removeHandlePrefix(handleId, "param-in:");
    return fieldId && getTransformerParameterField(nodeData.kind, fieldId) ? fieldId : undefined;
  }

  if (nodeData.kind === "operation") {
    return handleId === OPERATION_LEFT_HANDLE || handleId === OPERATION_RIGHT_HANDLE ? handleId : undefined;
  }

  if (nodeData.kind === "broadcast") {
    return handleId === BROADCAST_INPUT_HANDLE ? handleId : undefined;
  }

  return undefined;
}

export function numericSourceHandleToSlotId(nodeData: WorkflowNodeData, handleId?: string): string | undefined {
  if (!handleId) {
    return undefined;
  }

  if (isTransformerNodeData(nodeData)) {
    const fieldId = removeHandlePrefix(handleId, "param-out:");
    return fieldId && getTransformerParameterField(nodeData.kind, fieldId) ? fieldId : undefined;
  }

  if (nodeData.kind === "numericConstant") {
    return handleId === NUMERIC_CONSTANT_OUTPUT_HANDLE ? NUMERIC_CONSTANT_OUTPUT_HANDLE : undefined;
  }

  if (nodeData.kind === "operation") {
    return handleId === OPERATION_RESULT_HANDLE ? OPERATION_RESULT_HANDLE : undefined;
  }

  if (nodeData.kind === "broadcast") {
    return listNumericOutputHandles(nodeData).includes(handleId) ? handleId : undefined;
  }

  return undefined;
}

export function listNumericInputHandles(nodeData: WorkflowNodeData): string[] {
  if (isTransformerNodeData(nodeData)) {
    return getTransformerParameterDefinitions(nodeData.kind).map((field) => toParameterInputHandle(field.id));
  }

  if (nodeData.kind === "operation") {
    return [OPERATION_LEFT_HANDLE, OPERATION_RIGHT_HANDLE];
  }

  if (nodeData.kind === "broadcast") {
    return [BROADCAST_INPUT_HANDLE];
  }

  return [];
}

export function listNumericOutputHandles(nodeData: WorkflowNodeData): string[] {
  if (isTransformerNodeData(nodeData)) {
    return getTransformerParameterDefinitions(nodeData.kind).map((field) => toParameterOutputHandle(field.id));
  }

  if (nodeData.kind === "numericConstant") {
    return [NUMERIC_CONSTANT_OUTPUT_HANDLE];
  }

  if (nodeData.kind === "operation") {
    return [OPERATION_RESULT_HANDLE];
  }

  if (nodeData.kind === "broadcast") {
    return Array.from({ length: nodeData.outputs }, (_, index) => broadcastOutputHandle(index));
  }

  return [];
}

export function isArtifactEdge(edge: GraphEdge): boolean {
  return edge.kind === "artifact";
}

export function isParameterEdge(edge: GraphEdge): boolean {
  return edge.kind === "parameter";
}

export function numericSlotKey(nodeId: string, slotId: string): string {
  return `${nodeId}::${slotId}`;
}

export type ResolvedNumericGraph = {
  values: Record<string, number>;
  incomingEdges: Record<string, GraphEdge>;
};

export function resolveNumericGraph(graph: GraphDocument): ResolvedNumericGraph {
  const nodeMap = new Map(graph.nodes.map((node) => [node.id, node]));
  const incomingEdges = new Map<string, GraphEdge>();
  const resolvedValues = new Map<string, number>();
  const visiting = new Set<string>();

  for (const edge of graph.edges.filter(isParameterEdge)) {
    if (!edge.sourceHandle || !edge.targetHandle) {
      throw new Error(`Parameter edge ${edge.id} must include sourceHandle and targetHandle.`);
    }

    const sourceNode = nodeMap.get(edge.source);
    const targetNode = nodeMap.get(edge.target);
    if (!sourceNode || !targetNode) {
      throw new Error(`Parameter edge ${edge.id} references an unknown node.`);
    }

    const sourceSlotId = numericSourceHandleToSlotId(sourceNode.data, edge.sourceHandle);
    const targetSlotId = numericTargetHandleToSlotId(targetNode.data, edge.targetHandle);
    if (!sourceSlotId || !targetSlotId) {
      throw new Error(`Parameter edge ${edge.id} uses an incompatible handle.`);
    }

    const targetKey = numericSlotKey(edge.target, targetSlotId);
    if (incomingEdges.has(targetKey)) {
      throw new Error(`Numeric input ${edge.target}.${targetSlotId} already has an incoming connection.`);
    }

    incomingEdges.set(targetKey, edge);
  }

  function resolveSlot(nodeId: string, slotId: string): number {
    const slotKey = numericSlotKey(nodeId, slotId);
    const cached = resolvedValues.get(slotKey);
    if (cached !== undefined) {
      return cached;
    }

    if (visiting.has(slotKey)) {
      throw new Error(`Numeric parameter graph contains a cycle involving ${slotKey}.`);
    }

    const node = nodeMap.get(nodeId);
    if (!node) {
      throw new Error(`Unknown numeric node ${nodeId}.`);
    }

    visiting.add(slotKey);
    let value: number;

    const incomingEdge = incomingEdges.get(slotKey);
    if (incomingEdge) {
      const sourceNode = nodeMap.get(incomingEdge.source);
      if (!sourceNode) {
        throw new Error(`Unknown numeric source node ${incomingEdge.source}.`);
      }

      const sourceSlotId = numericSourceHandleToSlotId(sourceNode.data, incomingEdge.sourceHandle);
      if (!sourceSlotId) {
        throw new Error(`Parameter edge ${incomingEdge.id} uses an invalid source handle.`);
      }

      value = resolveSlot(incomingEdge.source, sourceSlotId);
    } else if (isTransformerNodeData(node.data)) {
      const field = getTransformerParameterField(node.data.kind, slotId);
      if (!field) {
        throw new Error(`Unknown parameter ${slotId} on ${node.data.kind}.`);
      }

      value = field.getValue(node.data.config as GridConfig & RouteConfig & RenderConfig);
    } else if (node.data.kind === "numericConstant") {
      if (slotId !== NUMERIC_CONSTANT_OUTPUT_HANDLE) {
        throw new Error(`Unknown constant slot ${slotId}.`);
      }

      value = node.data.value;
    } else if (node.data.kind === "operation") {
      if (slotId === OPERATION_LEFT_HANDLE) {
        value = node.data.left;
      } else if (slotId === OPERATION_RIGHT_HANDLE) {
        value = node.data.right;
      } else if (slotId === OPERATION_RESULT_HANDLE) {
        const left = resolveSlot(nodeId, OPERATION_LEFT_HANDLE);
        const right = resolveSlot(nodeId, OPERATION_RIGHT_HANDLE);
        value = node.data.operation === "multiply" ? left * right : left + right;
      } else {
        throw new Error(`Unknown operation slot ${slotId}.`);
      }
    } else if (node.data.kind === "broadcast") {
      if (slotId === BROADCAST_INPUT_HANDLE) {
        value = node.data.value;
      } else if (listNumericOutputHandles(node.data).includes(slotId)) {
        value = resolveSlot(nodeId, BROADCAST_INPUT_HANDLE);
      } else {
        throw new Error(`Unknown broadcast slot ${slotId}.`);
      }
    } else {
      throw new Error(`Node ${node.data.label} cannot participate in the numeric parameter graph.`);
    }

    visiting.delete(slotKey);
    resolvedValues.set(slotKey, value);
    return value;
  }

  for (const node of graph.nodes) {
    if (isTransformerNodeData(node.data)) {
      for (const field of getTransformerParameterDefinitions(node.data.kind)) {
        resolveSlot(node.id, field.id);
      }
      continue;
    }

    if (node.data.kind === "operation") {
      resolveSlot(node.id, OPERATION_RESULT_HANDLE);
      continue;
    }

    if (node.data.kind === "broadcast") {
      for (const outputHandle of listNumericOutputHandles(node.data)) {
        resolveSlot(node.id, outputHandle);
      }
    }
  }

  return {
    values: Object.fromEntries(resolvedValues),
    incomingEdges: Object.fromEntries(incomingEdges)
  };
}

export function applyResolvedTransformerConfig(
  node: GraphDocument["nodes"][number],
  resolvedNumericGraph: ResolvedNumericGraph
): GridConfig | RouteConfig | RenderConfig {
  if (!isTransformerNodeData(node.data)) {
    throw new Error(`Node ${node.id} is not a transformer.`);
  }

  let nextConfig = node.data.config;
  for (const field of getTransformerParameterDefinitions(node.data.kind)) {
    const resolvedValue = resolvedNumericGraph.values[numericSlotKey(node.id, field.id)];
    if (resolvedValue === undefined) {
      continue;
    }

    nextConfig = field.setValue(nextConfig, resolvedValue);
  }

  return nextConfig;
}

export function buildGraphExecutionPlan(
  graph: GraphDocument,
  targetNodeId: string
): (GraphDocument["nodes"][number] & { upstreamId?: string })[] {
  if (graph.nodes.length === 0) {
    throw new Error("Cannot build an execution plan for an empty graph.");
  }

  const nodeMap = new Map(graph.nodes.map((node) => [node.id, node]));
  const targetNode = nodeMap.get(targetNodeId);
  if (!targetNode) {
    throw new Error(`Unknown node ${targetNodeId}`);
  }

  if (!isExecutionNodeKind(targetNode.data.kind)) {
    throw new Error(`Cannot execute numeric helper node ${targetNode.data.label}.`);
  }

  const incoming = new Map<string, string[]>();
  for (const edge of graph.edges.filter(isArtifactEdge)) {
    const current = incoming.get(edge.target) ?? [];
    current.push(edge.source);
    incoming.set(edge.target, current);
  }

  const visiting = new Set<string>();
  const visited = new Set<string>();
  const ordered: (GraphDocument["nodes"][number] & { upstreamId?: string })[] = [];

  function visit(nodeId: string): void {
    if (visited.has(nodeId)) {
      return;
    }

    if (visiting.has(nodeId)) {
      throw new Error(`Graph contains a cycle involving node ${nodeId}.`);
    }

    const node = nodeMap.get(nodeId);
    if (!node) {
      throw new Error(`Unknown node ${nodeId}`);
    }

    if (!isExecutionNodeKind(node.data.kind)) {
      throw new Error(`Numeric helper node ${node.data.label} cannot be part of the execution plan.`);
    }

    visiting.add(nodeId);
    const upstreamIds = incoming.get(nodeId) ?? [];
    if (node.data.kind === "source" && upstreamIds.length > 0) {
      throw new Error(`Source node ${node.data.label} cannot have upstream inputs.`);
    }

    if (node.data.kind !== "source" && upstreamIds.length !== 1) {
      throw new Error(`Node ${node.data.label} must have exactly one upstream input.`);
    }

    for (const upstreamId of upstreamIds) {
      visit(upstreamId);
    }

    const upstreamId = upstreamIds[0];
    if (upstreamId) {
      const upstreamNode = nodeMap.get(upstreamId);
      if (!upstreamNode) {
        throw new Error(`Node ${node.data.label} references unknown upstream node ${upstreamId}.`);
      }

      if (!canConnectNodeKinds(upstreamNode.data.kind, node.data.kind)) {
        throw new Error(`Invalid edge into ${node.data.label}.`);
      }
    }

    ordered.push({ ...node, upstreamId });
    visiting.delete(nodeId);
    visited.add(nodeId);
  }

  visit(targetNodeId);
  return ordered;
}