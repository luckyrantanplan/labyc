import { buildGraphExecutionPlan, type WorkflowJob, type WorkflowNodeData } from "@labystudio/shared";
import type { Edge, Node } from "reactflow";
import { api } from "./api";
import { queueNodesForExecution } from "./workflowNodeState";
import { toGraphDocument } from "./workflowGraph";

type RunSelectedWorkflowArgs = {
    nodes: Node<WorkflowNodeData>[];
    edges: Edge[];
    projectDir: string;
    targetNodeId: string;
};

type RunSelectedWorkflowResult = {
    nextJob: WorkflowJob;
    nextNodes: Node<WorkflowNodeData>[];
    activePlanNodeIds: string[];
};

export async function runSelectedWorkflow({
    nodes,
    edges,
    projectDir,
    targetNodeId
}: RunSelectedWorkflowArgs): Promise<RunSelectedWorkflowResult> {
    const graph = toGraphDocument(nodes, edges);
    const plan = buildGraphExecutionPlan(graph, targetNodeId);
    const activePlanNodeIds = plan.map((node) => node.id);
    const response = await api.runGraph(graph, projectDir, targetNodeId);
    const nextJob = await api.getJob(response.jobId);

    return {
        nextJob,
        nextNodes: queueNodesForExecution(nodes, activePlanNodeIds),
        activePlanNodeIds
    };
}