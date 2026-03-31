import type {
    GridConfig,
    RenderConfig,
    RouteConfig,
    WorkflowJob,
    WorkflowNodeData
} from "@labystudio/shared";
import type { Edge, Node } from "reactflow";
import type { DisplayNodeData, DisplayPositionOverrides } from "./workflowGraph";

type Point = { x: number; y: number };

export function patchSelectedNodeData(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    update: (data: WorkflowNodeData) => WorkflowNodeData
): Node<WorkflowNodeData>[] {
    if (!selectedNodeId) {
        return nodes;
    }

    return nodes.map((node) => {
        if (node.id !== selectedNodeId) {
            return node;
        }

        return {
            ...node,
            data: update(node.data)
        };
    });
}

export function patchGridConfig(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    update: (config: GridConfig) => GridConfig
): Node<WorkflowNodeData>[] {
    return patchSelectedNodeData(nodes, selectedNodeId, (data) => data.kind === "grid" ? { ...data, config: update(data.config) } : data);
}

export function patchRouteConfig(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    update: (config: RouteConfig) => RouteConfig
): Node<WorkflowNodeData>[] {
    return patchSelectedNodeData(nodes, selectedNodeId, (data) => data.kind === "route" ? { ...data, config: update(data.config) } : data);
}

export function patchRenderConfig(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    update: (config: RenderConfig) => RenderConfig
): Node<WorkflowNodeData>[] {
    return patchSelectedNodeData(nodes, selectedNodeId, (data) => data.kind === "render" ? { ...data, config: update(data.config) } : data);
}

export function patchNumericConstantValue(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    value: number
): Node<WorkflowNodeData>[] {
    return patchSelectedNodeData(nodes, selectedNodeId, (data) => data.kind === "numericConstant" ? { ...data, value } : data);
}

export function patchOperationNode(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    update: (data: Extract<WorkflowNodeData, { kind: "operation" }>) => Extract<WorkflowNodeData, { kind: "operation" }>
): Node<WorkflowNodeData>[] {
    return patchSelectedNodeData(nodes, selectedNodeId, (data) => data.kind === "operation" ? update(data) : data);
}

export function patchBroadcastNode(
    nodes: Node<WorkflowNodeData>[],
    selectedNodeId: string | null,
    update: (data: Extract<WorkflowNodeData, { kind: "broadcast" }>) => Extract<WorkflowNodeData, { kind: "broadcast" }>
): Node<WorkflowNodeData>[] {
    return patchSelectedNodeData(nodes, selectedNodeId, (data) => data.kind === "broadcast" ? update(data) : data);
}

export function queueNodesForExecution(
    nodes: Node<WorkflowNodeData>[],
    planNodeIds: string[]
): Node<WorkflowNodeData>[] {
    return nodes.map((node) => {
        if (!planNodeIds.includes(node.id)) {
            return node;
        }

        return {
            ...node,
            data: {
                ...node.data,
                artifacts: {
                    ...(node.data.artifacts ?? {}),
                    status: "queued"
                }
            }
        };
    });
}

export function updateDisplayPositionOverrides(
    overrides: DisplayPositionOverrides,
    node: Node<DisplayNodeData>
): DisplayPositionOverrides {
    return {
        ...overrides,
        [node.id]: node.position
    };
}

export function moveLogicalNode(
    nodes: Node<WorkflowNodeData>[],
    node: Node<DisplayNodeData>
): Node<WorkflowNodeData>[] {
    if (node.id !== node.data.logicalNodeId) {
        return nodes;
    }

    return nodes.map((candidate) => (
        candidate.id === node.id
            ? { ...candidate, position: node.position as Point }
            : candidate
    ));
}

export function removeLogicalNodes(
    nodes: Node<WorkflowNodeData>[],
    nodeIds: string[]
): Node<WorkflowNodeData>[] {
    if (nodeIds.length === 0) {
        return nodes;
    }

    const nodeIdSet = new Set(nodeIds);
    return nodes.filter((node) => !nodeIdSet.has(node.id));
}

export function removeLogicalEdges(
    edges: Edge[],
    edgeIds: string[]
): Edge[] {
    if (edgeIds.length === 0) {
        return edges;
    }

    const edgeIdSet = new Set(edgeIds);
    return edges.filter((edge) => !edgeIdSet.has(edge.id));
}

export function removeEdgesConnectedToNodes(
    edges: Edge[],
    nodeIds: string[]
): Edge[] {
    if (nodeIds.length === 0) {
        return edges;
    }

    const nodeIdSet = new Set(nodeIds);
    return edges.filter((edge) => !nodeIdSet.has(edge.source) && !nodeIdSet.has(edge.target));
}

export function removeDisplayPositionOverrides(
    overrides: DisplayPositionOverrides,
    nodeIds: string[]
): DisplayPositionOverrides {
    if (nodeIds.length === 0) {
        return overrides;
    }

    const nodeIdSet = new Set(nodeIds);
    return Object.fromEntries(
        Object.entries(overrides).filter(([nodeId]) => !nodeIdSet.has(nodeId))
    );
}

export function formatJobStatus(job: WorkflowJob | null): string {
    if (!job) {
        return "No active job";
    }

    return `${job.status}${job.currentNodeId ? ` on ${job.currentNodeId}` : ""}`;
}