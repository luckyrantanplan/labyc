import ReactFlow, {
    Background,
    Controls,
    MiniMap,
    type Connection,
    type Edge,
    type Node,
    type OnNodesChange
} from "reactflow";
import ArtifactNode from "./ArtifactNode";
import WorkflowNode from "./WorkflowNode";
import type { DisplayNodeData } from "../lib/workflowGraph";

const nodeTypes = {
    workflow: WorkflowNode,
    artifact: ArtifactNode
};

type WorkflowCanvasPanelProps = {
    displayNodes: Node<DisplayNodeData>[];
    displayEdges: Edge[];
    onDisplayNodesChange: OnNodesChange;
    onDisplayNodeDragStop: (node: Node<DisplayNodeData>) => void;
    onConnect: (connection: Connection) => void;
    onSelectNode: (node: Node<DisplayNodeData>) => void;
};

export default function WorkflowCanvasPanel({
    displayNodes,
    displayEdges,
    onDisplayNodesChange,
    onDisplayNodeDragStop,
    onConnect,
    onSelectNode
}: WorkflowCanvasPanelProps) {
    return (
        <section className="panel flow-panel">
            <div className="panel-header">
                <h2>Workflow graph</h2>
                <p>Build valid source to grid to route to render chains and branch them as needed.</p>
            </div>
            <div className="flow-canvas">
                <ReactFlow
                    nodes={displayNodes}
                    edges={displayEdges}
                    nodeTypes={nodeTypes}
                    onNodesChange={onDisplayNodesChange}
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