import { useRef } from "react";
import ReactFlow, {
    Background,
    Controls,
    MiniMap,
    type Connection,
    type Edge,
    type Node,
    type OnNodesChange
} from "reactflow";
import type { DisplayNodeData } from "../lib/workflowGraph";
import {
    workflowDeleteKeys,
    workflowEdgeTypes,
    workflowNodeTypes
} from "./flowRegistry";

type WorkflowCanvasPanelProps = {
    displayNodes: Node<DisplayNodeData>[];
    displayEdges: Edge[];
    onDisplayNodesChange: OnNodesChange;
    onDisplayNodeDragStop: (node: Node<DisplayNodeData>) => void;
    onDisplayNodesDelete: (nodes: Node<DisplayNodeData>[]) => void;
    onDisplayEdgesDelete: (edges: { id: string }[]) => void;
    onConnect: (connection: Connection) => void;
    onSelectNode: (node: Node<DisplayNodeData>) => void;
};

export default function WorkflowCanvasPanel({
    displayNodes,
    displayEdges,
    onDisplayNodesChange,
    onDisplayNodeDragStop,
    onDisplayNodesDelete,
    onDisplayEdgesDelete,
    onConnect,
    onSelectNode
}: WorkflowCanvasPanelProps) {
    const nodeTypes = useRef(workflowNodeTypes);
    const edgeTypes = useRef(workflowEdgeTypes);
    const deleteKeys = useRef(workflowDeleteKeys);

    return (
        <section className="panel flow-panel">
            <div className="panel-header">
                <h2>Workflow graph</h2>
                <p>Build SVG stage chains, then wire numeric parameter flows directly into transformer fields.</p>
            </div>
            <div className="flow-canvas">
                <ReactFlow
                    nodes={displayNodes}
                    edges={displayEdges}
                    nodeTypes={nodeTypes.current}
                    edgeTypes={edgeTypes.current}
                    deleteKeyCode={deleteKeys.current}
                    onNodesChange={onDisplayNodesChange}
                    onNodesDelete={onDisplayNodesDelete}
                    onEdgesDelete={onDisplayEdgesDelete}
                    onNodeDragStop={(_, node) => { onDisplayNodeDragStop(node); }}
                    onConnect={onConnect}
                    onNodeClick={(_, node) => { onSelectNode(node); }}
                    fitView
                >
                    <Background gap={18} color="rgba(48, 77, 85, 0.15)" />
                    <Controls />
                    <MiniMap pannable zoomable />
                </ReactFlow>
            </div>
        </section>
    );
}