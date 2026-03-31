import type { EdgeTypes, NodeTypes } from "reactflow";
import ArtifactNode from "./ArtifactNode";
import WorkflowNode from "./WorkflowNode";

export const workflowNodeTypes = {
    workflow: WorkflowNode,
    artifact: ArtifactNode
} satisfies NodeTypes;

export const workflowEdgeTypes = Object.freeze({}) satisfies EdgeTypes;

export const workflowDeleteKeys = ["Backspace", "Delete"];