import { useMemo } from "react";
import type { Dispatch, SetStateAction } from "react";
import type { WorkflowNodeData } from "@labystudio/shared";
import type { Node } from "reactflow";
import type { DisplayNodeData } from "../lib/workflowGraph";
import {
    patchBroadcastNode,
    patchNumericConstantValue,
    patchOperationNode
} from "../lib/workflowNodeState";

type UseInteractiveDisplayNodesArgs = {
    displayNodes: Node<DisplayNodeData>[];
    patchNodeById: (nodeId: string, update: (data: WorkflowNodeData) => WorkflowNodeData) => void;
    patchTransformerParameterById: (nodeId: string, fieldId: string, value: number) => void;
    toggleRouteAlternateRoutingById: (nodeId: string, enabled: boolean) => void;
    setNodes: Dispatch<SetStateAction<Node<WorkflowNodeData>[]>>;
};

export function useInteractiveDisplayNodes({
    displayNodes,
    patchNodeById,
    patchTransformerParameterById,
    toggleRouteAlternateRoutingById,
    setNodes
}: UseInteractiveDisplayNodesArgs): Node<DisplayNodeData>[] {
    return useMemo(
        () => displayNodes.map((node) => {
            if (node.data.displayType === "transformer") {
                return {
                    ...node,
                    data: {
                        ...node.data,
                        onUpdateLabel: (value: string) => {
                            patchNodeById(node.id, (data) => ({ ...data, label: value }));
                        },
                        onUpdateParameter: (fieldId: string, value: number) => {
                            patchTransformerParameterById(node.id, fieldId, value);
                        },
                        onToggleAlternateRouting: (enabled: boolean) => {
                            toggleRouteAlternateRoutingById(node.id, enabled);
                        }
                    }
                };
            }

            if (node.data.displayType === "numericConstant") {
                return {
                    ...node,
                    data: {
                        ...node.data,
                        onUpdateLabel: (value: string) => {
                            patchNodeById(node.id, (data) => ({ ...data, label: value }));
                        },
                        onUpdateValue: (value: number) => {
                            setNodes((currentNodes) => patchNumericConstantValue(currentNodes, node.id, value));
                        }
                    }
                };
            }

            if (node.data.displayType === "operation") {
                return {
                    ...node,
                    data: {
                        ...node.data,
                        onUpdateLabel: (value: string) => {
                            patchNodeById(node.id, (data) => ({ ...data, label: value }));
                        },
                        onUpdateOperation: (value: "add" | "multiply") => {
                            setNodes((currentNodes) => patchOperationNode(currentNodes, node.id, (data) => ({
                                ...data,
                                operation: value,
                                label: data.label || (value === "multiply" ? "Multiply" : "Add")
                            })));
                        },
                        onUpdateInput: (side: "left" | "right", value: number) => {
                            setNodes((currentNodes) => patchOperationNode(currentNodes, node.id, (data) => ({
                                ...data,
                                [side]: value
                            })));
                        }
                    }
                };
            }

            if (node.data.displayType === "broadcast") {
                return {
                    ...node,
                    data: {
                        ...node.data,
                        onUpdateLabel: (value: string) => {
                            patchNodeById(node.id, (data) => ({ ...data, label: value }));
                        },
                        onUpdateValue: (value: number) => {
                            setNodes((currentNodes) => patchBroadcastNode(currentNodes, node.id, (data) => ({ ...data, value })));
                        },
                        onUpdateOutputs: (value: number) => {
                            const nextOutputs = Math.max(2, Math.min(8, Math.round(value)));
                            setNodes((currentNodes) => patchBroadcastNode(currentNodes, node.id, (data) => ({ ...data, outputs: nextOutputs })));
                        }
                    }
                };
            }

            return node;
        }),
        [displayNodes, patchNodeById, patchTransformerParameterById, setNodes, toggleRouteAlternateRoutingById]
    );
}