import { memo } from "react";
import { SVG_OUTPUT_HANDLE } from "@labystudio/shared";
import { Handle, Position, type NodeProps } from "reactflow";
import SvgPreview from "./SvgPreview";
import type { ArtifactDisplayData } from "../lib/workflowGraph";

function ArtifactNode({ data, selected }: NodeProps<ArtifactDisplayData>) {
    return (
        <div className={`artifact-node artifact-node--${data.artifactType} ${selected ? "is-selected" : ""}`}>
            <div className="artifact-node__header">
                <span className="artifact-node__kind">{data.artifactType}</span>
                <strong>{data.title}</strong>
            </div>

            <div className="artifact-node__body">
                <div className="artifact-node__value" title={data.value}>{data.value}</div>
                <SvgPreview path={data.path} cacheKey={data.cacheKey} title={data.value} compact />
            </div>

            <Handle
                id={SVG_OUTPUT_HANDLE}
                type="source"
                position={Position.Right}
                className="artifact-node__handle"
                isConnectable={data.connectable}
                style={{ top: "50%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
            />
        </div>
    );
}

export default memo(ArtifactNode);