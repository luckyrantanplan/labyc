import { z } from "zod";

export type NodeKind = "source" | "grid" | "route" | "render";
export type ArtifactStatus = "idle" | "queued" | "running" | "completed" | "failed";

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

export const workflowNodeDataSchema = z.discriminatedUnion("kind", [
  sourceNodeDataSchema,
  gridNodeDataSchema,
  routeNodeDataSchema,
  renderNodeDataSchema
]);

export type WorkflowNodeData = z.infer<typeof workflowNodeDataSchema>;

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
  target: z.string()
});

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
  render: []
};

export function canConnectNodeKinds(sourceKind: NodeKind, targetKind: NodeKind): boolean {
  return NEXT_KIND[sourceKind].includes(targetKind);
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

export function stageStem(inputPath: string, kind: Exclude<NodeKind, "source">, index: number): string {
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

export function buildGraphExecutionPlan(
  graph: GraphDocument,
  targetNodeId: string
): (GraphDocument["nodes"][number] & { upstreamId?: string })[] {
  if (graph.nodes.length === 0) {
    throw new Error("Cannot build an execution plan for an empty graph.");
  }

  const nodeMap = new Map(graph.nodes.map((node) => [node.id, node]));
  const incoming = new Map<string, string[]>();
  for (const edge of graph.edges) {
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