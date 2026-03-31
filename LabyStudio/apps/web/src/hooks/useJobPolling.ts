import { useEffect } from "react";
import type { Dispatch, SetStateAction } from "react";
import type { Node } from "reactflow";
import type { WorkflowJob, WorkflowNodeData } from "@labystudio/shared";
import { api } from "../lib/api";

type UseJobPollingArgs = {
    job: WorkflowJob | null;
    activePlanNodeIds: string[];
    setJob: Dispatch<SetStateAction<WorkflowJob | null>>;
    setJobLog: Dispatch<SetStateAction<string>>;
    setActivePlanNodeIds: Dispatch<SetStateAction<string[]>>;
    setNodes: Dispatch<SetStateAction<Node<WorkflowNodeData>[]>>;
    onError: (error: unknown) => void;
};

function isPollingStatus(status: WorkflowJob["status"] | undefined): status is "queued" | "running" {
    return status === "queued" || status === "running";
}

function applyPlannedStatuses(
    currentNodes: Node<WorkflowNodeData>[],
    activePlanNodeIds: string[],
    nextJob: WorkflowJob
): Node<WorkflowNodeData>[] {
    const currentIndex = nextJob.currentNodeId ? activePlanNodeIds.indexOf(nextJob.currentNodeId) : -1;

    return currentNodes.map((node) => {
        const planIndex = activePlanNodeIds.indexOf(node.id);
        if (planIndex === -1) {
            return node;
        }

        let status = node.data.artifacts?.status ?? "idle";
        if (nextJob.status === "queued" || currentIndex === -1) {
            status = "queued";
        } else if (planIndex < currentIndex) {
            status = "completed";
        } else if (planIndex === currentIndex) {
            status = "running";
        } else {
            status = "queued";
        }

        return {
            ...node,
            data: {
                ...node.data,
                artifacts: {
                    ...(node.data.artifacts ?? {}),
                    status
                }
            }
        };
    });
}

function applyJobResults(currentNodes: Node<WorkflowNodeData>[], nextJob: WorkflowJob): Node<WorkflowNodeData>[] {
    if (!nextJob.result?.nodes) {
        return currentNodes;
    }

    return currentNodes.map((node) => {
        const result = nextJob.result?.nodes.find((candidate) => candidate.nodeId === node.id);
        if (!result) {
            return node;
        }

        return {
            ...node,
            data: {
                ...node.data,
                artifacts: {
                    ...result.artifacts,
                    status: nextJob.status === "failed" ? "failed" : result.artifacts.status ?? "completed"
                }
            }
        };
    });
}

function markFailedNode(currentNodes: Node<WorkflowNodeData>[], currentNodeId?: string): Node<WorkflowNodeData>[] {
    if (!currentNodeId) {
        return currentNodes;
    }

    return currentNodes.map((node) => {
        if (node.id !== currentNodeId) {
            return node;
        }

        return {
            ...node,
            data: {
                ...node.data,
                artifacts: {
                    ...(node.data.artifacts ?? {}),
                    status: "failed"
                }
            }
        };
    });
}

function applyJobState(
    currentNodes: Node<WorkflowNodeData>[],
    activePlanNodeIds: string[],
    nextJob: WorkflowJob
): Node<WorkflowNodeData>[] {
    let nextNodes = currentNodes;

    if (isPollingStatus(nextJob.status) && activePlanNodeIds.length > 0) {
        nextNodes = applyPlannedStatuses(nextNodes, activePlanNodeIds, nextJob);
    }

    nextNodes = applyJobResults(nextNodes, nextJob);

    if (nextJob.status === "failed") {
        nextNodes = markFailedNode(nextNodes, nextJob.currentNodeId);
    }

    return nextNodes;
}

export function useJobPolling({
    job,
    activePlanNodeIds,
    setJob,
    setJobLog,
    setActivePlanNodeIds,
    setNodes,
    onError
}: UseJobPollingArgs): void {
    const jobId = job?.id;
    const jobStatus = job?.status;

    useEffect(() => {
        if (!jobId || !isPollingStatus(jobStatus)) {
            return;
        }

        let cancelled = false;
        let timer: number | undefined;

        const poll = async (): Promise<void> => {
            try {
                const [nextJob, nextLog] = await Promise.all([
                    api.getJob(jobId),
                    api.getJobLog(jobId)
                ]);

                if (cancelled) {
                    return;
                }

                setJob(nextJob);
                setJobLog(nextLog.log);
                setNodes((currentNodes) => applyJobState(currentNodes, activePlanNodeIds, nextJob));

                if (nextJob.status === "failed") {
                    onError(new Error(nextJob.error ?? "Job failed"));
                }

                if (nextJob.status === "completed" || nextJob.status === "failed") {
                    setActivePlanNodeIds([]);
                    return;
                }
            } catch (error) {
                if (!cancelled) {
                    onError(error);
                }
            }

            if (!cancelled) {
                timer = window.setTimeout(() => {
                    void poll();
                }, 1000);
            }
        };

        timer = window.setTimeout(() => {
            void poll();
        }, 1000);

        return () => {
            cancelled = true;
            if (timer !== undefined) {
                window.clearTimeout(timer);
            }
        };
    }, [activePlanNodeIds, jobId, jobStatus, onError, setActivePlanNodeIds, setJob, setJobLog, setNodes]);
}