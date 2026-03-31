import { useCallback, useEffect, useMemo, useState } from "react";
import { addEdge, type Connection, type Node, useNodesState } from "reactflow";
import {
    canConnectNodeKinds,
    type DirectoryEntry,
    type GridConfig,
    type NodeKind,
    type RenderConfig,
    type RouteConfig,
    type RuntimeContext,
    type WorkflowJob,
    type WorkflowNodeData
} from "@labystudio/shared";
import AppHeader from "./components/AppHeader";
import FileBrowserPanel from "./components/FileBrowserPanel";
import NodeInspector from "./components/NodeInspector";
import RunLogPanel from "./components/RunLogPanel";
import WorkflowCanvasPanel from "./components/WorkflowCanvasPanel";
import WorkspaceToolbar from "./components/WorkspaceToolbar";
import { useDirectoryListing } from "./hooks/useDirectoryListing";
import { useJobPolling } from "./hooks/useJobPolling";
import { useWorkspaceBootstrap } from "./hooks/useWorkspaceBootstrap";
import { api } from "./lib/api";
import { runSelectedWorkflow } from "./lib/workflowExecution";
import {
    formatJobStatus,
    moveLogicalNode,
    patchGridConfig,
    patchRenderConfig,
    patchRouteConfig,
    patchSelectedNodeData,
    updateDisplayPositionOverrides
} from "./lib/workflowNodeState";
import {
    basename,
    buildDisplayGraph,
    type DisplayNodeData,
    type DisplayPositionOverrides,
    createFlowNode,
    createStarterGraph,
    toDisplayNode,
    toGraphDocument
} from "./lib/workflowGraph";

function App() {
    const starterGraph = useMemo(() => createStarterGraph(), []);
    const [browserDir, setBrowserDir] = useState("");
    const [selectedBrowserPath, setSelectedBrowserPath] = useState("");
    const [projectDir, setProjectDir] = useState("");
    const [graphPath, setGraphPath] = useState("");
    const [nodes, setNodes] = useState(starterGraph.nodes);
    const [edges, setEdges] = useState(starterGraph.edges);
    const [selectedNodeId, setSelectedNodeId] = useState<string | null>("source");
    const [selectedDisplayNodeId, setSelectedDisplayNodeId] = useState<string | null>("source");
    const [displayPositionOverrides, setDisplayPositionOverrides] = useState<DisplayPositionOverrides>({});
    const [job, setJob] = useState<WorkflowJob | null>(null);
    const [activePlanNodeIds, setActivePlanNodeIds] = useState<string[]>([]);
    const [jobLog, setJobLog] = useState("");
    const [errorMessage, setErrorMessage] = useState("");

    const selectedNode = useMemo(
        () => nodes.find((node) => node.id === selectedNodeId) ?? null,
        [nodes, selectedNodeId]
    );

    const logicalDisplayNodes = useMemo(
        () => nodes.map((node) => toDisplayNode(node, nodes, edges)),
        [nodes, edges]
    );

    const displayGraph = useMemo(
        () => buildDisplayGraph(logicalDisplayNodes, edges, displayPositionOverrides),
        [logicalDisplayNodes, edges, displayPositionOverrides]
    );

    const [displayNodes, setDisplayNodes, onDisplayNodesChange] = useNodesState(displayGraph.nodes);

    const selectedCanvasNode = useMemo(
        () => displayNodes.find((node) => node.id === selectedDisplayNodeId) ?? null,
        [displayNodes, selectedDisplayNodeId]
    );

    const selectedDisplayNode = useMemo(
        () => logicalDisplayNodes.find((node) => node.id === selectedNodeId) ?? null,
        [logicalDisplayNodes, selectedNodeId]
    );

    const handleError = useCallback((error: unknown) => {
        if (error instanceof DOMException && error.name === "AbortError") {
            return;
        }

        setErrorMessage(error instanceof Error ? `${error.name}: ${error.message}` : String(error));
    }, []);

    const selectNode = useCallback((logicalNodeId: string | null, displayNodeId: string | null = logicalNodeId) => {
        setSelectedNodeId(logicalNodeId);
        setSelectedDisplayNodeId(displayNodeId);
    }, []);

    const addWorkflowNode = useCallback((kind: NodeKind, position: { x: number; y: number }) => {
        const id = crypto.randomUUID();
        setNodes((currentNodes) => [...currentNodes, createFlowNode(kind, id, position.x, position.y)]);
        selectNode(id);
    }, [selectNode]);

    const applyWorkspaceContext = useCallback((nextContext: RuntimeContext) => {
        setProjectDir(nextContext.defaultProjectDir);
        setBrowserDir(nextContext.defaultProjectDir);
        setGraphPath(`${nextContext.defaultProjectDir}/labystudio.graph.json`);
        selectNode("source");
    }, [selectNode]);

    const context = useWorkspaceBootstrap({
        onContext: applyWorkspaceContext,
        onError: handleError
    });

    const browserEntries = useDirectoryListing({
        browserDir,
        onError: handleError
    });

    useEffect(() => {
        setDisplayNodes(displayGraph.nodes);
    }, [displayGraph.nodes, setDisplayNodes]);

    useJobPolling({
        job,
        activePlanNodeIds,
        setJob,
        setJobLog,
        setActivePlanNodeIds,
        setNodes,
        onError: handleError
    });

    const handleSelectBrowserEntry = useCallback((entry: DirectoryEntry) => {
        setSelectedBrowserPath(entry.path);
        if (entry.kind === "directory") {
            setBrowserDir(entry.path);
        }
    }, []);

    const handleGoUpDirectory = useCallback(() => {
        setBrowserDir((currentBrowserDir) => currentBrowserDir.replace(/\/[^/]+$/, "") || "/");
    }, []);

    const handleUseSelectedPathAsProjectRoot = useCallback(() => {
        setProjectDir(selectedBrowserPath || browserDir);
    }, [browserDir, selectedBrowserPath]);

    function patchSelectedNode(update: (data: WorkflowNodeData) => WorkflowNodeData): void {
        setNodes((currentNodes) => patchSelectedNodeData(currentNodes, selectedNodeId, update));
    }

    function patchGrid(update: (config: GridConfig) => GridConfig): void {
        setNodes((currentNodes) => patchGridConfig(currentNodes, selectedNodeId, update));
    }

    function patchRoute(update: (config: RouteConfig) => RouteConfig): void {
        setNodes((currentNodes) => patchRouteConfig(currentNodes, selectedNodeId, update));
    }

    function patchRender(update: (config: RenderConfig) => RenderConfig): void {
        setNodes((currentNodes) => patchRenderConfig(currentNodes, selectedNodeId, update));
    }

    function handleConnect(connection: Connection): void {
        if (!connection.source || !connection.target) {
            return;
        }

        const sourceDisplayNode = displayGraph.nodes.find((node) => node.id === connection.source);
        const targetDisplayNode = displayGraph.nodes.find((node) => node.id === connection.target);
        if (!sourceDisplayNode || !targetDisplayNode) {
            return;
        }

        if (sourceDisplayNode.data.displayType !== "artifact" || sourceDisplayNode.data.artifactType !== "svg" || !sourceDisplayNode.data.connectable) {
            setErrorMessage("Only SVG artifact nodes can connect into a transformer.");
            return;
        }

        if (targetDisplayNode.data.displayType !== "transformer") {
            setErrorMessage("SVG artifacts must connect to transformer nodes.");
            return;
        }

        const sourceNode = nodes.find((node) => node.id === sourceDisplayNode.data.logicalNodeId);
        const targetNode = nodes.find((node) => node.id === targetDisplayNode.data.logicalNodeId);
        if (!sourceNode || !targetNode) {
            return;
        }

        if (!canConnectNodeKinds(sourceNode.data.kind, targetNode.data.kind)) {
            setErrorMessage(`Invalid edge: ${sourceNode.data.kind} cannot connect to ${targetNode.data.kind}.`);
            return;
        }

        if (edges.some((edge) => edge.target === targetNode.id)) {
            setErrorMessage("Each processing node accepts a single upstream edge in this first implementation.");
            return;
        }

        setEdges((currentEdges) => addEdge({ id: crypto.randomUUID(), source: sourceNode.id, target: targetNode.id, type: "smoothstep" }, currentEdges));
    }

    async function handleImportSvg(): Promise<void> {
        if (selectedNode?.data.kind !== "source" || !selectedBrowserPath.endsWith(".svg")) {
            return;
        }

        try {
            const response = await api.importSvg(selectedBrowserPath, projectDir);
            patchSelectedNode((data) => data.kind === "source"
                ? {
                    ...data,
                    label: basename(response.importedPath),
                    sourcePath: response.importedPath,
                    artifacts: {
                        ...(data.artifacts ?? {}),
                        outputPath: response.importedPath,
                        status: "completed",
                        lastRunAt: new Date().toISOString()
                    }
                }
                : data);
            setBrowserDir(projectDir);
        } catch (error) {
            handleError(error);
        }
    }

    async function handleSaveGraph(): Promise<void> {
        try {
            await api.saveGraph(graphPath, toGraphDocument(nodes, edges));
        } catch (error) {
            handleError(error);
        }
    }

    async function handleLoadGraph(): Promise<void> {
        try {
            const graph = await api.loadGraph(graphPath);
            setNodes(graph.nodes.map((node) => ({ ...node, type: "workflow" })));
            setEdges(graph.edges.map((edge) => ({ ...edge, type: "smoothstep" })));
            setDisplayPositionOverrides({});
            selectNode(graph.nodes[0]?.id ?? null);
        } catch (error) {
            handleError(error);
        }
    }

    async function handleRunSelected(): Promise<void> {
        if (!selectedNode) {
            return;
        }

        try {
            const { nextJob, nextNodes, activePlanNodeIds: nextPlanNodeIds } = await runSelectedWorkflow({
                nodes,
                edges,
                projectDir,
                targetNodeId: selectedNode.id
            });
            setActivePlanNodeIds(nextPlanNodeIds);
            setNodes(nextNodes);
            setJob(nextJob);
            setJobLog("");
        } catch (error) {
            handleError(error);
        }
    }

    const handleDisplayNodeDragStop = useCallback((node: Node<DisplayNodeData>) => {
        setDisplayPositionOverrides((current) => updateDisplayPositionOverrides(current, node));
        setNodes((currentNodes) => moveLogicalNode(currentNodes, node));
    }, []);

    const handleSelectDisplayNode = useCallback((node: Node<DisplayNodeData>) => {
        selectNode(node.data.logicalNodeId, node.id);
    }, [selectNode]);

    const jobStatus = formatJobStatus(job);

    return (
        <div className="app-shell">
            <AppHeader
                workspaceRoot={context?.workspaceRoot}
                binaryPath={context?.binaryPath}
                binaryExists={context?.binaryExists}
            />

            <WorkspaceToolbar
                projectDir={projectDir}
                graphPath={graphPath}
                onProjectDirChange={setProjectDir}
                onGraphPathChange={setGraphPath}
                onSaveGraph={() => { void handleSaveGraph(); }}
                onLoadGraph={() => { void handleLoadGraph(); }}
                onRunSelected={() => { void handleRunSelected(); }}
                canRunSelected={Boolean(selectedNode)}
                onAddNode={addWorkflowNode}
            />

            {errorMessage ? <div className="error-banner">{errorMessage}</div> : null}

            <main className="main-grid">
                <FileBrowserPanel
                    browserDir={browserDir}
                    browserEntries={browserEntries}
                    selectedBrowserPath={selectedBrowserPath}
                    onBrowserDirChange={setBrowserDir}
                    onSelectEntry={handleSelectBrowserEntry}
                    onGoUp={handleGoUpDirectory}
                    onUseSelectedPathAsProjectRoot={handleUseSelectedPathAsProjectRoot}
                    onImportSvg={() => { void handleImportSvg(); }}
                    canImportSvg={selectedNode?.data.kind === "source" && selectedBrowserPath.endsWith(".svg")}
                />

                <WorkflowCanvasPanel
                    displayNodes={displayNodes}
                    displayEdges={displayGraph.edges}
                    onDisplayNodesChange={onDisplayNodesChange}
                    onDisplayNodeDragStop={handleDisplayNodeDragStop}
                    onConnect={handleConnect}
                    onSelectNode={handleSelectDisplayNode}
                />

                <aside className="panel inspector-panel">
                    <div className="panel-header">
                        <h2>Inspector</h2>
                        <p>
                            {selectedCanvasNode
                                ? selectedCanvasNode.data.displayType === "artifact"
                                    ? `Editing ${selectedCanvasNode.data.title}`
                                    : `Editing ${selectedDisplayNode?.data.label ?? selectedCanvasNode.id}`
                                : "No node selected"}
                        </p>
                    </div>
                    <NodeInspector
                        selectedNode={selectedNode}
                        selectedCanvasNode={selectedCanvasNode as Node<DisplayNodeData> | null}
                        selectedDisplayNode={selectedDisplayNode}
                        onPatchSelectedNode={patchSelectedNode}
                        onPatchGrid={patchGrid}
                        onPatchRoute={patchRoute}
                        onPatchRender={patchRender}
                    />
                </aside>

                <RunLogPanel status={jobStatus} jobLog={jobLog} />
            </main>
        </div>
    );
}

export default App;