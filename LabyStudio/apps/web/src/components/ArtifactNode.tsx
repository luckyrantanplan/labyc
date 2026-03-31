import { memo } from "react";
import { Handle, Position, type NodeProps } from "reactflow";
import SvgPreview from "./SvgPreview";
import type { ArtifactDisplayData } from "../lib/workflowGraph";

function ArtifactNode({ data, selected }: NodeProps<ArtifactDisplayData>) {
    const isSvg = data.artifactType === "svg";

    return (
        <div className={`artifact-node artifact-node--${data.artifactType} ${selected ? "is-selected" : ""}`}>
            {data.acceptsInput ? (
                <Handle
                    id="artifact-in"
                    type="target"
                    position={Position.Left}
                    className="artifact-node__handle"
                    isConnectable={false}
                    style={{ top: "50%", left: 0, transform: "translate(-50%, -50%)" }}
                />
            ) : null}

            <div className="artifact-node__header">
                <span className="artifact-node__kind">{data.artifactType}</span>
                <strong>{data.title}</strong>
            </div>

            <div className="artifact-node__body">
                <div className="artifact-node__value" title={data.value}>{data.value}</div>
                {isSvg ? (
                    <SvgPreview path={data.path} cacheKey={data.cacheKey} title={data.value} compact />
                ) : null}
                {data.artifactType === "status" ? (
                    <div className={`artifact-node__status artifact-node__status--${data.status ?? "idle"}`}>{data.status ?? "idle"}</div>
                ) : null}
            </div>

            {data.emitsOutput ? (
                <Handle
                    id="artifact-out"
                    type="source"
                    position={Position.Right}
                    className="artifact-node__handle"
                    isConnectable={data.connectable}
                    style={{ top: "50%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
                />
            ) : null}
        </div>
    );
}

export default memo(ArtifactNode);