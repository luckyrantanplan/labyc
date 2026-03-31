import type { Edge, Node } from "reactflow";
import {
    createDefaultNodeData,
    graphDocumentSchema,
    type ArtifactStatus,
    type GraphDocument,
    type NodeKind,
    type WorkflowNodeData
} from "@labystudio/shared";

export type WorkflowNodeDisplayData = WorkflowNodeData & {
    resolvedSourcePath?: string;
    resolvedOutputPath?: string;
};

export type TransformerDisplayData = {
    displayType: "transformer";
    logicalNodeId: string;
    kind: Exclude<NodeKind, "source">;
    label: string;
    status: ArtifactStatus;
};

export type ArtifactDisplayData = {
    displayType: "artifact";
    logicalNodeId: string;
    artifactType: "svg" | "json" | "status";
    title: string;
    value: string;
    path?: string;
    cacheKey?: string;
    status?: ArtifactStatus;
    acceptsInput: boolean;
    emitsOutput: boolean;
    connectable: boolean;
};

export type DisplayNodeData = TransformerDisplayData | ArtifactDisplayData;

export type DisplayPositionOverrides = Record<string, { x: number; y: number }>;

type CanvasPosition = { x: number; y: number };

// Keep display layout values together so stage and artifact positioning stays consistent.
const DISPLAY_LAYOUT = {
    stageSpacingX: 440,
    stageBaseY: 220,
    offsets: {
        config: { x: 0, y: -176 },
        status: { x: 258, y: -176 },
        linkArtifact: { x: 230, y: 22 },
        finalOutput: { x: 260, y: 28 }
    },
    linkArtifactStackY: 164
} as const;

export function basename(filePath?: string): string {
    if (!filePath) {
        return "";
    }

    const segments = filePath.replaceAll("\\", "/").split("/");
    return segments[segments.length - 1] ?? filePath;
}

export function outputSvgPathForNode(data: WorkflowNodeData): string {
    return data.artifacts?.outputPath ?? (data.kind === "source" ? data.sourcePath : "");
}

export function resolveUpstreamNodeId(nodeId: string, edges: Edge[]): string | undefined {
    return edges.find((edge) => edge.target === nodeId)?.source;
}

export function resolveSourceSvgPath(node: Node<WorkflowNodeData>, nodes: Node<WorkflowNodeData>[], edges: Edge[]): string {
    if (node.data.kind === "source") {
        return node.data.sourcePath;
    }

    const upstreamNodeId = resolveUpstreamNodeId(node.id, edges);
    if (!upstreamNodeId) {
        return node.data.artifacts?.inputPath ?? "";
    }

    const upstreamNode = nodes.find((candidate) => candidate.id === upstreamNodeId);
    if (!upstreamNode) {
        return node.data.artifacts?.inputPath ?? "";
    }

    const upstreamOutputPath = outputSvgPathForNode(upstreamNode.data);
    return upstreamOutputPath !== "" ? upstreamOutputPath : node.data.artifacts?.inputPath ?? "";
}

export function toDisplayNode(node: Node<WorkflowNodeData>, nodes: Node<WorkflowNodeData>[], edges: Edge[]): Node<WorkflowNodeDisplayData> {
    return {
        ...node,
        data: {
            ...node.data,
            resolvedSourcePath: resolveSourceSvgPath(node, nodes, edges),
            resolvedOutputPath: outputSvgPathForNode(node.data)
        }
    };
}

function createArtifactNode(
    id: string,
    logicalNodeId: string,
    position: CanvasPosition,
    data: Omit<ArtifactDisplayData, "displayType" | "logicalNodeId">
): Node<ArtifactDisplayData> {
    return {
        id,
        type: "artifact",
        position,
        draggable: true,
        selectable: true,
        data: {
            ...data,
            displayType: "artifact",
            logicalNodeId
        }
    };
}

function buildStageBasePositions(logicalNodes: Node<WorkflowNodeDisplayData>[]) {
    const orderedNodes = [...logicalNodes].sort((left, right) => left.position.x - right.position.x || left.position.y - right.position.y);
    const positions = new Map<string, CanvasPosition>();
    let nextX = 60;

    orderedNodes.forEach((node, index) => {
        nextX = index === 0 ? node.position.x : Math.max(node.position.x, nextX + DISPLAY_LAYOUT.stageSpacingX);
        positions.set(node.id, { x: nextX, y: Math.max(node.position.y, DISPLAY_LAYOUT.stageBaseY) });
    });

    return positions;
}

function offsetPosition(basePosition: CanvasPosition, offset: CanvasPosition): CanvasPosition {
    return {
        x: basePosition.x + offset.x,
        y: basePosition.y + offset.y
    };
}

function resolveDisplayPosition(
    overrides: DisplayPositionOverrides,
    id: string,
    basePosition: CanvasPosition,
    offset: CanvasPosition = { x: 0, y: 0 }
): CanvasPosition {
    return overrides[id] ?? offsetPosition(basePosition, offset);
}

export function buildDisplayGraph(
    logicalNodes: Node<WorkflowNodeDisplayData>[],
    logicalEdges: Edge[],
    positionOverrides: DisplayPositionOverrides = {}
): { nodes: Node<DisplayNodeData>[]; edges: Edge[] } {
    const nodes: Node<DisplayNodeData>[] = [];
    const edges: Edge[] = [];
    const nodeMap = new Map(logicalNodes.map((node) => [node.id, node]));
    const downstreamCount = new Map<string, number>();
    const outgoingOrdinals = new Map<string, number>();
    const stageBasePositions = buildStageBasePositions(logicalNodes);

    for (const edge of logicalEdges) {
        downstreamCount.set(edge.source, (downstreamCount.get(edge.source) ?? 0) + 1);
    }

    for (const node of logicalNodes) {
        const basePosition = stageBasePositions.get(node.id) ?? node.position;

        if (node.data.kind === "source") {
            nodes.push({
                id: node.id,
                type: "artifact",
                position: resolveDisplayPosition(positionOverrides, node.id, basePosition),
                draggable: true,
                selectable: true,
                data: {
                    displayType: "artifact",
                    logicalNodeId: node.id,
                    artifactType: "svg",
                    title: "Source SVG",
                    value: basename(node.data.resolvedOutputPath ?? node.data.sourcePath) || "Choose SVG",
                    path: node.data.resolvedOutputPath ?? node.data.sourcePath,
                    cacheKey: node.data.artifacts?.lastRunAt ?? node.data.sourcePath,
                    acceptsInput: false,
                    emitsOutput: true,
                    connectable: true
                }
            });
            continue;
        }

        nodes.push({
            id: node.id,
            type: "workflow",
            position: resolveDisplayPosition(positionOverrides, node.id, basePosition),
            draggable: true,
            selectable: true,
            data: {
                displayType: "transformer",
                logicalNodeId: node.id,
                kind: node.data.kind,
                label: node.data.label,
                status: node.data.artifacts?.status ?? "idle"
            }
        });

        nodes.push(createArtifactNode(
            `${node.id}::config`,
            node.id,
            resolveDisplayPosition(positionOverrides, `${node.id}::config`, basePosition, DISPLAY_LAYOUT.offsets.config),
            {
                artifactType: "json",
                title: "Config JSON",
                value: basename(node.data.artifacts?.configPath) || "Generated on run",
                path: node.data.artifacts?.configPath,
                cacheKey: node.data.artifacts?.lastRunAt,
                acceptsInput: false,
                emitsOutput: true,
                connectable: false
            }
        ));

        nodes.push(createArtifactNode(
            `${node.id}::status`,
            node.id,
            resolveDisplayPosition(positionOverrides, `${node.id}::status`, basePosition, DISPLAY_LAYOUT.offsets.status),
            {
                artifactType: "status",
                title: "Status",
                value: node.data.artifacts?.status ?? "idle",
                status: node.data.artifacts?.status ?? "idle",
                acceptsInput: true,
                emitsOutput: false,
                connectable: false
            }
        ));

        edges.push({
            id: `${node.id}::config-edge`,
            source: `${node.id}::config`,
            target: node.id,
            sourceHandle: "artifact-out",
            targetHandle: "config-in",
            type: "smoothstep",
            data: { logical: false }
        });

        edges.push({
            id: `${node.id}::status-edge`,
            source: node.id,
            target: `${node.id}::status`,
            sourceHandle: "status-out",
            targetHandle: "artifact-in",
            type: "smoothstep",
            data: { logical: false }
        });
    }

    for (const edge of logicalEdges) {
        const sourceNode = nodeMap.get(edge.source);
        const targetNode = nodeMap.get(edge.target);
        if (!sourceNode || !targetNode) {
            continue;
        }

        if (sourceNode.data.kind === "source") {
            edges.push({
                id: `${edge.id}::source-svg`,
                source: sourceNode.id,
                target: targetNode.id,
                sourceHandle: "artifact-out",
                targetHandle: "svg-in",
                type: "smoothstep",
                data: { logicalEdgeId: edge.id }
            });
            continue;
        }

        const artifactNodeId = `${edge.id}::svg`;
        const sourceBasePosition = stageBasePositions.get(sourceNode.id) ?? sourceNode.position;
        const outputOrdinal = outgoingOrdinals.get(sourceNode.id) ?? 0;
        outgoingOrdinals.set(sourceNode.id, outputOrdinal + 1);
        const defaultArtifactOffset = {
            x: DISPLAY_LAYOUT.offsets.linkArtifact.x,
            y: DISPLAY_LAYOUT.offsets.linkArtifact.y + outputOrdinal * DISPLAY_LAYOUT.linkArtifactStackY
        };

        nodes.push(createArtifactNode(
            artifactNodeId,
            sourceNode.id,
            resolveDisplayPosition(positionOverrides, artifactNodeId, sourceBasePosition, defaultArtifactOffset),
            {
                artifactType: "svg",
                title: "SVG Artifact",
                value: basename(sourceNode.data.resolvedOutputPath ?? sourceNode.data.artifacts?.outputPath) || "Pending SVG",
                path: sourceNode.data.resolvedOutputPath ?? sourceNode.data.artifacts?.outputPath,
                cacheKey: sourceNode.data.artifacts?.lastRunAt ?? sourceNode.data.resolvedOutputPath,
                acceptsInput: true,
                emitsOutput: true,
                connectable: true
            }
        ));

        edges.push({
            id: `${artifactNodeId}::from-transformer`,
            source: sourceNode.id,
            target: artifactNodeId,
            sourceHandle: "svg-out",
            targetHandle: "artifact-in",
            type: "smoothstep",
            data: { logicalEdgeId: edge.id }
        });

        edges.push({
            id: `${artifactNodeId}::to-transformer`,
            source: artifactNodeId,
            target: targetNode.id,
            sourceHandle: "artifact-out",
            targetHandle: "svg-in",
            type: "smoothstep",
            data: { logicalEdgeId: edge.id }
        });
    }

    for (const node of logicalNodes) {
        if (node.data.kind === "source") {
            continue;
        }

        if ((downstreamCount.get(node.id) ?? 0) > 0) {
            continue;
        }

        const artifactNodeId = `${node.id}::final-svg`;
        const basePosition = stageBasePositions.get(node.id) ?? node.position;
        nodes.push(createArtifactNode(
            artifactNodeId,
            node.id,
            resolveDisplayPosition(positionOverrides, artifactNodeId, basePosition, DISPLAY_LAYOUT.offsets.finalOutput),
            {
                artifactType: "svg",
                title: "Output SVG",
                value: basename(node.data.resolvedOutputPath ?? node.data.artifacts?.outputPath) || "Pending SVG",
                path: node.data.resolvedOutputPath ?? node.data.artifacts?.outputPath,
                cacheKey: node.data.artifacts?.lastRunAt ?? node.data.resolvedOutputPath,
                acceptsInput: true,
                emitsOutput: false,
                connectable: false
            }
        ));

        edges.push({
            id: `${artifactNodeId}::edge`,
            source: node.id,
            target: artifactNodeId,
            sourceHandle: "svg-out",
            targetHandle: "artifact-in",
            type: "smoothstep",
            data: { logical: false }
        });
    }

    return { nodes, edges };
}

export function createFlowNode(kind: NodeKind, id: string, x: number, y: number): Node<WorkflowNodeData> {
    return {
        id,
        type: "workflow",
        position: { x, y },
        data: createDefaultNodeData(kind)
    };
}

export function createStarterGraph(): { nodes: Node<WorkflowNodeData>[]; edges: Edge[] } {
    return {
        nodes: [
            createFlowNode("source", "source", 60, 200),
            createFlowNode("grid", "grid", 340, 200),
            createFlowNode("route", "route", 640, 200),
            createFlowNode("render", "render", 940, 200)
        ],
        edges: [
            { id: "edge-source-grid", source: "source", target: "grid", type: "smoothstep" },
            { id: "edge-grid-route", source: "grid", target: "route", type: "smoothstep" },
            { id: "edge-route-render", source: "route", target: "render", type: "smoothstep" }
        ]
    };
}

export function toGraphDocument(nodes: Node<WorkflowNodeData>[], edges: Edge[]): GraphDocument {
    return graphDocumentSchema.parse({
        version: 1,
        nodes: nodes.map((node) => ({
            id: node.id,
            type: node.type ?? "workflow",
            position: node.position,
            data: node.data
        })),
        edges: edges.map((edge) => ({
            id: edge.id,
            source: edge.source,
            target: edge.target
        }))
    });
}