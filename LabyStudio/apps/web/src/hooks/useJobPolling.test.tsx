// @vitest-environment jsdom

import { act, renderHook } from "@testing-library/react";
import { createDefaultNodeData, type WorkflowJob, type WorkflowNodeData } from "@labystudio/shared";
import type { Node } from "reactflow";
import { afterEach, beforeEach, describe, expect, it, vi } from "vitest";
import { api } from "../lib/api";
import { useJobPolling } from "./useJobPolling";

vi.mock("../lib/api", () => ({
    api: {
        getJob: vi.fn(),
        getJobLog: vi.fn()
    }
}));

function createNode(id: string, kind: WorkflowNodeData["kind"]): Node<WorkflowNodeData> {
    const data = createDefaultNodeData(kind);
    if (data.kind === "source") {
        data.sourcePath = `/tmp/${id}.svg`;
    }

    return {
        id,
        type: "workflow",
        position: { x: 0, y: 0 },
        data
    };
}

describe("useJobPolling", () => {
    beforeEach(() => {
        vi.useFakeTimers();
        vi.mocked(api.getJob).mockReset();
        vi.mocked(api.getJobLog).mockReset();
    });

    afterEach(() => {
        vi.useRealTimers();
    });

    it("applies completed job state and clears the active plan", async () => {
        const initialJob: WorkflowJob = {
            id: "job-1",
            status: "queued",
            projectDir: "/tmp/project",
            targetNodeId: "grid",
            createdAt: "2026-03-31T00:00:00.000Z",
            updatedAt: "2026-03-31T00:00:00.000Z"
        };
        const completedJob: WorkflowJob = {
            ...initialJob,
            status: "completed",
            currentNodeId: "grid",
            updatedAt: "2026-03-31T00:00:01.000Z",
            result: {
                nodes: [
                    {
                        nodeId: "source",
                        artifacts: {
                            outputPath: "/tmp/project/source.svg",
                            status: "completed"
                        }
                    },
                    {
                        nodeId: "grid",
                        artifacts: {
                            inputPath: "/tmp/project/source.svg",
                            outputPath: "/tmp/project/grid.svg",
                            status: "completed"
                        }
                    }
                ]
            }
        };

        vi.mocked(api.getJob).mockResolvedValue(completedJob);
        vi.mocked(api.getJobLog).mockResolvedValue({ log: "done" });

        const setJob = vi.fn();
        const setJobLog = vi.fn();
        const setActivePlanNodeIds = vi.fn();
        const setNodes = vi.fn();
        const onError = vi.fn();

        renderHook(() => {
            useJobPolling({
                job: initialJob,
                activePlanNodeIds: ["source", "grid"],
                setJob,
                setJobLog,
                setActivePlanNodeIds,
                setNodes,
                onError
            });
        });

        await act(async () => {
            await vi.advanceTimersByTimeAsync(1000);
        });

        expect(api.getJob).toHaveBeenCalledWith("job-1");
        expect(setJob).toHaveBeenCalledWith(completedJob);
        expect(setJobLog).toHaveBeenCalledWith("done");
        expect(setActivePlanNodeIds).toHaveBeenCalledWith([]);
        expect(onError).not.toHaveBeenCalled();

        const updateNodes = setNodes.mock.calls[0]?.[0] as ((nodes: Node<WorkflowNodeData>[]) => Node<WorkflowNodeData>[]) | undefined;
        expect(updateNodes).toBeTypeOf("function");

        const nextNodes = updateNodes?.([createNode("source", "source"), createNode("grid", "grid")]);
        expect(nextNodes?.find((node) => node.id === "grid")?.data.artifacts?.status).toBe("completed");
        expect(nextNodes?.find((node) => node.id === "grid")?.data.artifacts?.outputPath).toBe("/tmp/project/grid.svg");
    });

    it("marks the current node as failed and surfaces the error", async () => {
        const initialJob: WorkflowJob = {
            id: "job-2",
            status: "running",
            projectDir: "/tmp/project",
            targetNodeId: "route",
            currentNodeId: "route",
            createdAt: "2026-03-31T00:00:00.000Z",
            updatedAt: "2026-03-31T00:00:00.000Z"
        };
        const failedJob: WorkflowJob = {
            ...initialJob,
            status: "failed",
            error: "routing failed",
            updatedAt: "2026-03-31T00:00:02.000Z"
        };

        vi.mocked(api.getJob).mockResolvedValue(failedJob);
        vi.mocked(api.getJobLog).mockResolvedValue({ log: "boom" });

        const setJob = vi.fn();
        const setJobLog = vi.fn();
        const setActivePlanNodeIds = vi.fn();
        const setNodes = vi.fn();
        const onError = vi.fn();

        renderHook(() => {
            useJobPolling({
                job: initialJob,
                activePlanNodeIds: ["source", "grid", "route"],
                setJob,
                setJobLog,
                setActivePlanNodeIds,
                setNodes,
                onError
            });
        });

        await act(async () => {
            await vi.advanceTimersByTimeAsync(1000);
        });

        expect(onError).toHaveBeenCalledTimes(1);
        expect((onError.mock.calls[0]?.[0] as Error).message).toBe("routing failed");

        const updateNodes = setNodes.mock.calls[0]?.[0] as ((nodes: Node<WorkflowNodeData>[]) => Node<WorkflowNodeData>[]) | undefined;
        const nextNodes = updateNodes?.([
            createNode("source", "source"),
            createNode("grid", "grid"),
            createNode("route", "route")
        ]);
        expect(nextNodes?.find((node) => node.id === "route")?.data.artifacts?.status).toBe("failed");
    });

    it("applies in-progress artifact paths while a downstream stage is still running", async () => {
        const initialJob: WorkflowJob = {
            id: "job-3",
            status: "running",
            projectDir: "/tmp/project",
            targetNodeId: "render",
            currentNodeId: "route",
            createdAt: "2026-03-31T00:00:00.000Z",
            updatedAt: "2026-03-31T00:00:00.000Z"
        };
        const runningJob: WorkflowJob = {
            ...initialJob,
            updatedAt: "2026-03-31T00:00:03.000Z",
            result: {
                nodes: [
                    {
                        nodeId: "source",
                        artifacts: {
                            inputPath: "/tmp/project/source.svg",
                            outputPath: "/tmp/project/source.svg",
                            status: "completed"
                        }
                    },
                    {
                        nodeId: "grid",
                        artifacts: {
                            inputPath: "/tmp/project/source.svg",
                            outputPath: "/tmp/project/source-grid.svg",
                            configPath: "/tmp/project/source-grid.json",
                            status: "completed"
                        }
                    },
                    {
                        nodeId: "route",
                        artifacts: {
                            inputPath: "/tmp/project/source-grid.svg",
                            outputPath: "/tmp/project/source-route.svg",
                            configPath: "/tmp/project/source-route.json",
                            status: "running"
                        }
                    }
                ]
            }
        };

        vi.mocked(api.getJob).mockResolvedValue(runningJob);
        vi.mocked(api.getJobLog).mockResolvedValue({ log: "still running" });

        const setJob = vi.fn();
        const setJobLog = vi.fn();
        const setActivePlanNodeIds = vi.fn();
        const setNodes = vi.fn();
        const onError = vi.fn();

        renderHook(() => {
            useJobPolling({
                job: initialJob,
                activePlanNodeIds: ["source", "grid", "route", "render"],
                setJob,
                setJobLog,
                setActivePlanNodeIds,
                setNodes,
                onError
            });
        });

        await act(async () => {
            await vi.advanceTimersByTimeAsync(1000);
        });

        const updateNodes = setNodes.mock.calls[0]?.[0] as ((nodes: Node<WorkflowNodeData>[]) => Node<WorkflowNodeData>[]) | undefined;
        const nextNodes = updateNodes?.([
            createNode("source", "source"),
            createNode("grid", "grid"),
            createNode("route", "route"),
            createNode("render", "render")
        ]);

        expect(nextNodes?.find((node) => node.id === "grid")?.data.artifacts?.outputPath).toBe("/tmp/project/source-grid.svg");
        expect(nextNodes?.find((node) => node.id === "route")?.data.artifacts?.status).toBe("running");
        expect(nextNodes?.find((node) => node.id === "route")?.data.artifacts?.configPath).toBe("/tmp/project/source-route.json");
        expect(nextNodes?.find((node) => node.id === "render")?.data.artifacts?.status).toBe("queued");
    });
});