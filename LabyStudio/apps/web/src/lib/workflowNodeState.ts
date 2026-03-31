import type {
    GridConfig,
    RenderConfig,
    RouteConfig,
    WorkflowJob,
    WorkflowNodeData
} from "@labystudio/shared";
import type { Node } from "reactflow";
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
    if (node.id === node.data.logicalNodeId) {
        const { [node.id]: removedOverride, ...remainingOverrides } = overrides;
        void removedOverride;
        return remainingOverrides;
    }

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

export function formatJobStatus(job: WorkflowJob | null): string {
    if (!job) {
        return "No active job";
    }

    return `${job.status}${job.currentNodeId ? ` on ${job.currentNodeId}` : ""}`;
}