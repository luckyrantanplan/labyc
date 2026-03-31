import type { Edge, Node } from "reactflow";
import {
    BROADCAST_INPUT_HANDLE,
    NUMERIC_CONSTANT_OUTPUT_HANDLE,
    OPERATION_LEFT_HANDLE,
    OPERATION_RESULT_HANDLE,
    OPERATION_RIGHT_HANDLE,
    SVG_INPUT_HANDLE,
    SVG_OUTPUT_HANDLE,
    createDefaultNodeData,
    getTransformerParameterDefinitions,
    graphDocumentSchema,
    numericSlotKey,
    resolveNumericGraph,
    routeToggleFieldGroup,
    type ArtifactStatus,
    type GraphDocument,
    type GraphEdge,
    type NodeKind,
    type OperationKind,
    type TransformerNodeKind,
    type WorkflowNodeData
} from "@labystudio/shared";

export type WorkflowNodeDisplayData = WorkflowNodeData & {
    resolvedSourcePath?: string;
    resolvedOutputPath?: string;
};

export type ParameterDisplayField = {
    id: string;
    label: string;
    step?: string;
    value: number;
    resolvedValue: number;
    isConnected: boolean;
};

export type TransformerDisplayData = {
    displayType: "transformer";
    logicalNodeId: string;
    kind: TransformerNodeKind;
    label: string;
    status: ArtifactStatus;
    sourceSvgPath: string;
    outputSvgPath: string;
    configPath?: string;
    alternateRoutingEnabled?: boolean;
    parameterFields: ParameterDisplayField[];
    onUpdateLabel?: (value: string) => void;
    onUpdateParameter?: (fieldId: string, value: number) => void;
    onToggleAlternateRouting?: (enabled: boolean) => void;
};

export type NumericConstantDisplayData = {
    displayType: "numericConstant";
    logicalNodeId: string;
    kind: "numericConstant";
    label: string;
    value: number;
    outputValue: number;
    onUpdateLabel?: (value: string) => void;
    onUpdateValue?: (value: number) => void;
};

export type OperationDisplayData = {
    displayType: "operation";
    logicalNodeId: string;
    kind: "operation";
    label: string;
    operation: OperationKind;
    left: { value: number; resolvedValue: number; isConnected: boolean };
    right: { value: number; resolvedValue: number; isConnected: boolean };
    result: number;
    onUpdateLabel?: (value: string) => void;
    onUpdateOperation?: (value: OperationKind) => void;
    onUpdateInput?: (side: "left" | "right", value: number) => void;
};

export type BroadcastDisplayData = {
    displayType: "broadcast";
    logicalNodeId: string;
    kind: "broadcast";
    label: string;
    value: number;
    inputValue: number;
    inputConnected: boolean;
    outputs: { handleId: string; value: number }[];
    onUpdateLabel?: (value: string) => void;
    onUpdateValue?: (value: number) => void;
    onUpdateOutputs?: (value: number) => void;
};

export type ArtifactDisplayData = {
    displayType: "artifact";
    logicalNodeId: string;
    artifactType: "svg";
    title: string;
    value: string;
    path?: string;
    cacheKey?: string;
    connectable: boolean;
};

export type DisplayNodeData =
    | TransformerDisplayData
    | NumericConstantDisplayData
    | OperationDisplayData
    | BroadcastDisplayData
    | ArtifactDisplayData;

export type DisplayPositionOverrides = Record<string, { x: number; y: number }>;

export function basename(filePath?: string): string {
    if (!filePath) {
        return "";
    }

    const segments = filePath.replaceAll("\\", "/").split("/");
    return segments[segments.length - 1] ?? filePath;
}

export function getFlowEdgeKind(edge: { data?: unknown; kind?: GraphEdge["kind"]; sourceHandle?: string | null; targetHandle?: string | null }): GraphEdge["kind"] {
    if (typeof edge.kind === "string") {
        return edge.kind;
    }

    const data = edge.data as { kind?: GraphEdge["kind"] } | undefined;
    return data?.kind ?? "artifact";
}

export function outputSvgPathForNode(data: WorkflowNodeData): string {
    return data.artifacts?.outputPath ?? (data.kind === "source" ? data.sourcePath : "");
}

export function resolveUpstreamNodeId(nodeId: string, edges: Edge[]): string | undefined {
    return edges.find((edge) => getFlowEdgeKind(edge) === "artifact" && edge.target === nodeId)?.source;
}

export function resolveSourceSvgPath(node: Node<WorkflowNodeData>, nodes: Node<WorkflowNodeData>[], edges: Edge[]): string {
    if (node.data.kind === "source") {
        return node.data.sourcePath;
    }

    if (node.data.kind === "numericConstant" || node.data.kind === "operation" || node.data.kind === "broadcast") {
        return "";
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

function toGraphDocumentFromFlow(nodes: Node<WorkflowNodeDisplayData>[], edges: Edge[]): GraphDocument {
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
            target: edge.target,
            kind: getFlowEdgeKind(edge),
            sourceHandle: edge.sourceHandle ?? undefined,
            targetHandle: edge.targetHandle ?? undefined
        }))
    });
}

export function buildDisplayGraph(
    logicalNodes: Node<WorkflowNodeDisplayData>[],
    logicalEdges: Edge[],
    positionOverrides: DisplayPositionOverrides = {}
): { nodes: Node<DisplayNodeData>[]; edges: Edge[] } {
    const numericGraph = resolveNumericGraph(toGraphDocumentFromFlow(logicalNodes, logicalEdges));
    const displayNodes = logicalNodes.map<Node<DisplayNodeData>>((node) => {
        const position = positionOverrides[node.id] ?? node.position;

        if (node.data.kind === "source") {
            return {
                id: node.id,
                type: "artifact",
                position,
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
                    connectable: true
                }
            };
        }

        if (node.data.kind === "grid" || node.data.kind === "route" || node.data.kind === "render") {
            const transformerData = node.data;
            const parameterFields = getTransformerParameterDefinitions(transformerData.kind).map((field) => {
                const value = field.getValue(transformerData.config);
                const slotKey = numericSlotKey(node.id, field.id);
                return {
                    id: field.id,
                    label: field.label,
                    step: field.step,
                    value,
                    resolvedValue: numericGraph.values[slotKey] ?? value,
                    isConnected: numericGraph.incomingEdges[slotKey] !== undefined
                };
            });

            return {
                id: node.id,
                type: "workflow",
                position,
                draggable: true,
                selectable: true,
                data: {
                    displayType: "transformer",
                    logicalNodeId: node.id,
                    kind: transformerData.kind,
                    label: transformerData.label,
                    status: transformerData.artifacts?.status ?? "idle",
                    sourceSvgPath: transformerData.resolvedSourcePath ?? "",
                    outputSvgPath: transformerData.resolvedOutputPath ?? "",
                    configPath: transformerData.artifacts?.configPath,
                    alternateRoutingEnabled: transformerData.kind === "route"
                        ? routeToggleFieldGroup.isEnabled(transformerData.config)
                        : undefined,
                    parameterFields
                }
            };
        }

        if (node.data.kind === "numericConstant") {
            return {
                id: node.id,
                type: "workflow",
                position,
                draggable: true,
                selectable: true,
                data: {
                    displayType: "numericConstant",
                    logicalNodeId: node.id,
                    kind: "numericConstant",
                    label: node.data.label,
                    value: node.data.value,
                    outputValue: numericGraph.values[numericSlotKey(node.id, NUMERIC_CONSTANT_OUTPUT_HANDLE)] ?? node.data.value
                }
            };
        }

        if (node.data.kind === "operation") {
            const leftKey = numericSlotKey(node.id, OPERATION_LEFT_HANDLE);
            const rightKey = numericSlotKey(node.id, OPERATION_RIGHT_HANDLE);
            const resultKey = numericSlotKey(node.id, OPERATION_RESULT_HANDLE);

            return {
                id: node.id,
                type: "workflow",
                position,
                draggable: true,
                selectable: true,
                data: {
                    displayType: "operation",
                    logicalNodeId: node.id,
                    kind: "operation",
                    label: node.data.label,
                    operation: node.data.operation,
                    left: {
                        value: node.data.left,
                        resolvedValue: numericGraph.values[leftKey] ?? node.data.left,
                        isConnected: numericGraph.incomingEdges[leftKey] !== undefined
                    },
                    right: {
                        value: node.data.right,
                        resolvedValue: numericGraph.values[rightKey] ?? node.data.right,
                        isConnected: numericGraph.incomingEdges[rightKey] !== undefined
                    },
                    result: numericGraph.values[resultKey] ?? node.data.left + node.data.right
                }
            };
        }

        const broadcastData = node.data;
        const inputKey = numericSlotKey(node.id, BROADCAST_INPUT_HANDLE);
        return {
            id: node.id,
            type: "workflow",
            position,
            draggable: true,
            selectable: true,
            data: {
                displayType: "broadcast",
                logicalNodeId: node.id,
                kind: "broadcast",
                label: broadcastData.label,
                value: broadcastData.value,
                inputValue: numericGraph.values[inputKey] ?? broadcastData.value,
                inputConnected: numericGraph.incomingEdges[inputKey] !== undefined,
                outputs: Array.from({ length: broadcastData.outputs }, (_, index) => {
                    const handleId = `output:${index + 1}`;
                    return {
                        handleId,
                        value: numericGraph.values[numericSlotKey(node.id, handleId)] ?? broadcastData.value
                    };
                })
            }
        };
    });

    const displayEdges = logicalEdges.map<Edge>((edge) => ({
        id: edge.id,
        source: edge.source,
        target: edge.target,
        sourceHandle: edge.sourceHandle ?? (getFlowEdgeKind(edge) === "artifact" ? SVG_OUTPUT_HANDLE : undefined),
        targetHandle: edge.targetHandle ?? (getFlowEdgeKind(edge) === "artifact" ? SVG_INPUT_HANDLE : undefined),
        type: "smoothstep",
        data: {
            logicalEdgeId: edge.id,
            kind: getFlowEdgeKind(edge)
        }
    }));

    return { nodes: displayNodes, edges: displayEdges };
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
            { id: "edge-source-grid", source: "source", target: "grid", type: "smoothstep", data: { kind: "artifact" } },
            { id: "edge-grid-route", source: "grid", target: "route", type: "smoothstep", data: { kind: "artifact" } },
            { id: "edge-route-render", source: "route", target: "render", type: "smoothstep", data: { kind: "artifact" } }
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
            target: edge.target,
            kind: getFlowEdgeKind(edge),
            sourceHandle: edge.sourceHandle ?? undefined,
            targetHandle: edge.targetHandle ?? undefined
        }))
    });
}
