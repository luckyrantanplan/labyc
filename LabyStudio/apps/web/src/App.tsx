import { useCallback, useEffect, useMemo, useState } from "react";
import { type Connection, type Node, useNodesState } from "reactflow";
import {
    getTransformerParameterField,
    numericSourceHandleToSlotId,
    numericTargetHandleToSlotId,
    resolveNumericGraph,
    routeToggleFieldGroup,
    SVG_INPUT_HANDLE,
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
import { useInteractiveDisplayNodes } from "./hooks/useInteractiveDisplayNodes";
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
    removeDisplayPositionOverrides,
    removeEdgesConnectedToNodes,
    removeLogicalEdges,
    removeLogicalNodes,
    updateDisplayPositionOverrides
} from "./lib/workflowNodeState";
import {
    basename,
    buildDisplayGraph,
    type DisplayNodeData,
    type DisplayPositionOverrides,
    createFlowNode,
    createStarterGraph,
    getFlowEdgeKind,
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

    const patchNodeById = useCallback((nodeId: string, update: (data: WorkflowNodeData) => WorkflowNodeData) => {
        setNodes((currentNodes) => patchSelectedNodeData(currentNodes, nodeId, update));
    }, []);

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

    const patchTransformerParameterById = useCallback((nodeId: string, fieldId: string, value: number) => {
        setNodes((currentNodes) => currentNodes.map((node) => {
            if (node.id !== nodeId) {
                return node;
            }

            if (node.data.kind === "grid") {
                const field = getTransformerParameterField("grid", fieldId);
                return field
                    ? { ...node, data: { ...node.data, config: field.setValue(node.data.config, value) } }
                    : node;
            }

            if (node.data.kind === "route") {
                const field = getTransformerParameterField("route", fieldId);
                return field
                    ? { ...node, data: { ...node.data, config: field.setValue(node.data.config, value) } }
                    : node;
            }

            if (node.data.kind === "render") {
                const field = getTransformerParameterField("render", fieldId);
                return field
                    ? { ...node, data: { ...node.data, config: field.setValue(node.data.config, value) } }
                    : node;
            }

            return node;
        }));
    }, []);

    const toggleRouteAlternateRoutingById = useCallback((nodeId: string, enabled: boolean) => {
        patchNodeById(nodeId, (data) => data.kind === "route"
            ? { ...data, config: routeToggleFieldGroup.setEnabled(data.config, enabled) }
            : data);
    }, [patchNodeById]);

    const interactiveDisplayNodes = useInteractiveDisplayNodes({
        displayNodes: displayGraph.nodes,
        patchNodeById,
        patchTransformerParameterById,
        toggleRouteAlternateRoutingById,
        setNodes
    });

    useEffect(() => {
        setDisplayNodes(interactiveDisplayNodes);
    }, [interactiveDisplayNodes, setDisplayNodes]);

    function handleConnect(connection: Connection): void {
        if (!connection.source || !connection.target) {
            return;
        }

        const sourceNode = nodes.find((node) => node.id === connection.source);
        const targetNode = nodes.find((node) => node.id === connection.target);
        if (!sourceNode || !targetNode) {
            return;
        }

        const sourceParameterSlot = numericSourceHandleToSlotId(sourceNode.data, connection.sourceHandle ?? undefined);
        const targetParameterSlot = numericTargetHandleToSlotId(targetNode.data, connection.targetHandle ?? undefined);
        const isArtifactConnection = connection.targetHandle === SVG_INPUT_HANDLE && sourceParameterSlot === undefined && targetParameterSlot === undefined;

        if (isArtifactConnection) {
            if (!canConnectNodeKinds(sourceNode.data.kind, targetNode.data.kind)) {
                setErrorMessage(`Invalid SVG edge: ${sourceNode.data.kind} cannot connect to ${targetNode.data.kind}.`);
                return;
            }

            if (edges.some((edge) => getFlowEdgeKind(edge) === "artifact" && edge.target === targetNode.id)) {
                setErrorMessage("Each transformer accepts exactly one upstream SVG edge.");
                return;
            }

            setEdges((currentEdges) => [
                ...currentEdges,
                {
                    id: crypto.randomUUID(),
                    source: sourceNode.id,
                    target: targetNode.id,
                    sourceHandle: connection.sourceHandle,
                    targetHandle: connection.targetHandle,
                    type: "smoothstep",
                    data: { kind: "artifact" }
                }
            ]);
            setErrorMessage("");
            return;
        }

        if (!sourceParameterSlot || !targetParameterSlot) {
            setErrorMessage("Numeric edges must connect from a numeric output handle into a numeric input handle.");
            return;
        }

        const nextEdge = {
            id: crypto.randomUUID(),
            source: sourceNode.id,
            target: targetNode.id,
            sourceHandle: connection.sourceHandle,
            targetHandle: connection.targetHandle,
            type: "smoothstep",
            data: { kind: "parameter" as const }
        };

        try {
            resolveNumericGraph(toGraphDocument(nodes, [...edges, nextEdge]));
            setEdges((currentEdges) => [...currentEdges, nextEdge]);
            setErrorMessage("");
        } catch (error) {
            handleError(error);
        }
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
            setEdges(graph.edges.map((edge) => ({
                ...edge,
                type: "smoothstep",
                sourceHandle: edge.sourceHandle,
                targetHandle: edge.targetHandle,
                data: { kind: edge.kind }
            })));
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

        if (selectedNode.data.kind === "numericConstant" || selectedNode.data.kind === "operation" || selectedNode.data.kind === "broadcast") {
            setErrorMessage("Numeric helper nodes do not execute directly. Run a source or transformer node instead.");
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

    const deleteLogicalNodes = useCallback((nodeIds: string[]) => {
        if (nodeIds.length === 0) {
            return;
        }

        setNodes((currentNodes) => removeLogicalNodes(currentNodes, nodeIds));
        setEdges((currentEdges) => removeEdgesConnectedToNodes(currentEdges, nodeIds));
        setDisplayPositionOverrides((currentOverrides) => removeDisplayPositionOverrides(currentOverrides, nodeIds));

        if (selectedNodeId !== null && nodeIds.includes(selectedNodeId)) {
            selectNode(null);
        }
    }, [selectNode, selectedNodeId]);

    const handleDeleteSelected = useCallback(() => {
        if (!selectedNodeId) {
            return;
        }

        deleteLogicalNodes([selectedNodeId]);
    }, [deleteLogicalNodes, selectedNodeId]);

    const handleDisplayNodesDelete = useCallback((deletedNodes: Node<DisplayNodeData>[]) => {
        deleteLogicalNodes(deletedNodes.map((node) => node.data.logicalNodeId));
    }, [deleteLogicalNodes]);

    const handleDisplayEdgesDelete = useCallback((deletedEdges: { id: string }[]) => {
        const edgeIds = deletedEdges.map((edge) => edge.id);
        setEdges((currentEdges) => removeLogicalEdges(currentEdges, edgeIds));
    }, []);

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
                onDeleteSelected={handleDeleteSelected}
                onRunSelected={() => { void handleRunSelected(); }}
                canRunSelected={Boolean(selectedNode && selectedNode.data.kind !== "numericConstant" && selectedNode.data.kind !== "operation" && selectedNode.data.kind !== "broadcast")}
                canDeleteSelected={selectedNodeId !== null}
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
                    onDisplayNodesDelete={handleDisplayNodesDelete}
                    onDisplayEdgesDelete={handleDisplayEdgesDelete}
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