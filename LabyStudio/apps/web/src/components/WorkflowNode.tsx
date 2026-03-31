import { memo } from "react";
import { Handle, Position, type NodeProps } from "reactflow";
import type { WorkflowNodeData } from "@labystudio/shared";

function basename(filePath?: string): string {
    if (!filePath) {
        return "unbound";
    }

    const segments = filePath.replaceAll("\\", "/").split("/");
    return segments[segments.length - 1] ?? filePath;
}

function WorkflowNode({ data, selected }: NodeProps<WorkflowNodeData>) {
    const sourceName = data.kind === "source" ? basename(data.sourcePath) : null;
    const artifactName = basename(data.artifacts?.outputPath ?? data.artifacts?.inputPath);

    return (
        <div className={`workflow-node ${selected ? "is-selected" : ""}`}>
            {data.kind !== "source" ? <Handle type="target" position={Position.Left} /> : null}
            <div className="workflow-node__header">
                <span className={`workflow-node__badge workflow-node__badge--${data.kind}`}>{data.kind}</span>
                <strong>{data.label}</strong>
            </div>
            <div className="workflow-node__body">
                {sourceName ? <div>Source: {sourceName}</div> : null}
                <div>Status: {data.artifacts?.status ?? "idle"}</div>
                <div>Artifact: {artifactName}</div>
            </div>
            <Handle type="source" position={Position.Right} />
        </div>
    );
}

export default memo(WorkflowNode);