import { memo } from "react";
import { Handle, Position, type NodeProps } from "reactflow";
import type { TransformerDisplayData } from "../lib/workflowGraph";

function WorkflowNode({ data, selected }: NodeProps<TransformerDisplayData>) {
    return (
        <div className={`workflow-node workflow-node--transformer ${selected ? "is-selected" : ""}`}>
            <Handle
                id="config-in"
                type="target"
                position={Position.Left}
                className="workflow-node__handle workflow-node__handle--config"
                isConnectable={false}
                style={{ top: "28%", left: 0, transform: "translate(-50%, -50%)" }}
            />
            <Handle
                id="svg-in"
                type="target"
                position={Position.Left}
                className="workflow-node__handle workflow-node__handle--svg"
                style={{ top: "72%", left: 0, transform: "translate(-50%, -50%)" }}
            />
            <Handle
                id="status-out"
                type="source"
                position={Position.Right}
                className="workflow-node__handle workflow-node__handle--status"
                isConnectable={false}
                style={{ top: "28%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
            />
            <Handle
                id="svg-out"
                type="source"
                position={Position.Right}
                className="workflow-node__handle workflow-node__handle--svg"
                isConnectable={false}
                style={{ top: "72%", right: 0, left: "auto", transform: "translate(50%, -50%)" }}
            />

            <div className="workflow-node__header">
                <span className={`workflow-node__badge workflow-node__badge--${data.kind}`}>{data.kind}</span>
                <strong>{data.label}</strong>
            </div>

            <div className="workflow-node__body">
                <div className="workflow-node__transformer-body">
                    <div className="workflow-node__lane workflow-node__lane--left">
                        <span>Config JSON</span>
                        <span>Source SVG</span>
                    </div>
                    <div className="workflow-node__core">
                        <div className="workflow-node__core-title">Transformer</div>
                        <div className={`workflow-node__status workflow-node__status--${data.status}`}>{data.status}</div>
                    </div>
                    <div className="workflow-node__lane workflow-node__lane--right">
                        <span>Status</span>
                        <span>Output SVG</span>
                    </div>
                </div>
            </div>
        </div>
    );
}

export default memo(WorkflowNode);