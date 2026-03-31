import { useEffect, useMemo, useState } from "react";
import ReactFlow, {
    addEdge,
    Background,
    Controls,
    MiniMap,
    type Connection,
    type Edge,
    type Node,
    useEdgesState,
    useNodesState
} from "reactflow";
import {
    buildGraphExecutionPlan,
    canConnectNodeKinds,
    createDefaultNodeData,
    graphDocumentSchema,
    type DirectoryEntry,
    type GraphDocument,
    type GridConfig,
    type NodeKind,
    type RenderConfig,
    type RouteConfig,
    type RuntimeContext,
    type WorkflowJob,
    type WorkflowNodeData
} from "@labystudio/shared";
import WorkflowNode from "./components/WorkflowNode";
import { api } from "./lib/api";

const nodeTypes = {
    workflow: WorkflowNode
};

function basename(filePath?: string): string {
    if (!filePath) {
        return "";
    }

    const segments = filePath.replaceAll("\\", "/").split("/");
    return segments[segments.length - 1] ?? filePath;
}

function createFlowNode(kind: NodeKind, id: string, x: number, y: number): Node<WorkflowNodeData> {
    return {
        id,
        type: "workflow",
        position: { x, y },
        data: createDefaultNodeData(kind)
    };
}

function createStarterGraph(): { nodes: Node<WorkflowNodeData>[]; edges: Edge[] } {
    return {
        nodes: [
            createFlowNode("source", "source", 60, 140),
            createFlowNode("grid", "grid", 340, 140),
            createFlowNode("route", "route", 640, 140),
            createFlowNode("render", "render", 940, 140)
        ],
        edges: [
            { id: "edge-source-grid", source: "source", target: "grid", type: "smoothstep" },
            { id: "edge-grid-route", source: "grid", target: "route", type: "smoothstep" },
            { id: "edge-route-render", source: "route", target: "render", type: "smoothstep" }
        ]
    };
}

function toGraphDocument(nodes: Node<WorkflowNodeData>[], edges: Edge[]): GraphDocument {
    return graphDocumentSchema.parse({
        version: 1,
        nodes: nodes.map((node) => ({
            id: node.id,
            type: node.type ?? "workflow",
            position: node.position,
            data: node.data
        })),
        edges: edges.map((edge) => ({
            id: edge.id,
            source: edge.source,
            target: edge.target
        }))
    });
}

function App() {
    const starterGraph = useMemo(() => createStarterGraph(), []);
    const [context, setContext] = useState<RuntimeContext | null>(null);
    const [browserDir, setBrowserDir] = useState("");
    const [browserEntries, setBrowserEntries] = useState<DirectoryEntry[]>([]);
    const [selectedBrowserPath, setSelectedBrowserPath] = useState("");
    const [projectDir, setProjectDir] = useState("");
    const [graphPath, setGraphPath] = useState("");
    const [nodes, setNodes, onNodesChange] = useNodesState<WorkflowNodeData>(starterGraph.nodes);
    const [edges, setEdges, onEdgesChange] = useEdgesState(starterGraph.edges);
    const [selectedNodeId, setSelectedNodeId] = useState<string | null>("source");
    const [job, setJob] = useState<WorkflowJob | null>(null);
    const [activePlanNodeIds, setActivePlanNodeIds] = useState<string[]>([]);
    const [jobLog, setJobLog] = useState("");
    const [previewSvg, setPreviewSvg] = useState("");
    const [errorMessage, setErrorMessage] = useState("");

    const selectedNode = useMemo(
        () => nodes.find((node) => node.id === selectedNodeId) ?? null,
        [nodes, selectedNodeId]
    );

    useEffect(() => {
        api.getContext()
            .then((nextContext) => {
                setContext(nextContext);
                setProjectDir(nextContext.defaultProjectDir);
                setBrowserDir(nextContext.defaultProjectDir);
                setGraphPath(`${nextContext.defaultProjectDir}/labystudio.graph.json`);
            })
            .catch((error) => setErrorMessage(String(error)));
    }, []);

    useEffect(() => {
        if (!browserDir) {
            return;
        }

        api.listDirectory(browserDir)
            .then((listing) => setBrowserEntries(listing.entries))
            .catch((error) => setErrorMessage(String(error)));
    }, [browserDir]);

    useEffect(() => {
        const previewPath =
            selectedNode?.data.artifacts?.outputPath ??
            (selectedNode?.data.kind === "source" ? selectedNode.data.sourcePath : undefined) ??
            (selectedBrowserPath.endsWith(".svg") ? selectedBrowserPath : undefined);

        if (!previewPath) {
            setPreviewSvg("");
            return;
        }

        fetch(api.svgUrl(previewPath))
            .then(async (response) => {
                if (!response.ok) {
                    throw new Error(await response.text());
                }
                return response.text();
            })
            .then(setPreviewSvg)
            .catch((error) => setErrorMessage(String(error)));
    }, [selectedBrowserPath, selectedNode]);

    useEffect(() => {
        if (!job || (job.status !== "queued" && job.status !== "running")) {
            return;
        }

        const timer = window.setInterval(() => {
            api.getJob(job.id)
                .then(async (nextJob) => {
                    setJob(nextJob);
                    const nextLog = await api.getJobLog(nextJob.id);
                    setJobLog(nextLog.log);

                    if ((nextJob.status === "queued" || nextJob.status === "running") && activePlanNodeIds.length > 0) {
                        setNodes((currentNodes) => {
                            const currentIndex = nextJob.currentNodeId ? activePlanNodeIds.indexOf(nextJob.currentNodeId) : -1;
                            return currentNodes.map((node) => {
                                const planIndex = activePlanNodeIds.indexOf(node.id);
                                if (planIndex === -1) {
                                    return node;
                                }

                                let status = node.data.artifacts?.status ?? "idle";
                                if (nextJob.status === "queued") {
                                    status = "queued";
                                } else if (currentIndex === -1) {
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
                        });
                    }

                    if (nextJob.result?.nodes) {
                        setNodes((currentNodes) => currentNodes.map((node) => {
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
                        }));
                    }

                    if (nextJob.status === "failed") {
                        setNodes((currentNodes) => currentNodes.map((node) => node.id === nextJob.currentNodeId
                            ? {
                                ...node,
                                data: {
                                    ...node.data,
                                    artifacts: {
                                        ...(node.data.artifacts ?? {}),
                                        status: "failed"
                                    }
                                }
                            }
                            : node));
                        setErrorMessage(nextJob.error ?? "Job failed");
                    }

                    if (nextJob.status === "completed" || nextJob.status === "failed") {
                        setActivePlanNodeIds([]);
                    }
                })
                .catch((error) => setErrorMessage(String(error)));
        }, 1000);

        return () => window.clearInterval(timer);
    }, [job, setNodes]);

    function patchSelectedNode(update: (data: WorkflowNodeData) => WorkflowNodeData): void {
        if (!selectedNodeId) {
            return;
        }

        setNodes((currentNodes) => currentNodes.map((node) => {
            if (node.id !== selectedNodeId) {
                return node;
            }

            return {
                ...node,
                data: update(node.data)
            };
        }));
    }

    function patchGrid(update: (config: GridConfig) => GridConfig): void {
        patchSelectedNode((data) => data.kind === "grid" ? { ...data, config: update(data.config) } : data);
    }

    function patchRoute(update: (config: RouteConfig) => RouteConfig): void {
        patchSelectedNode((data) => data.kind === "route" ? { ...data, config: update(data.config) } : data);
    }

    function patchRender(update: (config: RenderConfig) => RenderConfig): void {
        patchSelectedNode((data) => data.kind === "render" ? { ...data, config: update(data.config) } : data);
    }

    function handleConnect(connection: Connection): void {
        if (!connection.source || !connection.target) {
            return;
        }

        const sourceNode = nodes.find((node) => node.id === connection.source);
        const targetNode = nodes.find((node) => node.id === connection.target);
        if (!sourceNode || !targetNode) {
            return;
        }

        if (!canConnectNodeKinds(sourceNode.data.kind, targetNode.data.kind)) {
            setErrorMessage(`Invalid edge: ${sourceNode.data.kind} cannot connect to ${targetNode.data.kind}.`);
            return;
        }

        if (edges.some((edge) => edge.target === connection.target)) {
            setErrorMessage("Each processing node accepts a single upstream edge in this first implementation.");
            return;
        }

        setEdges((currentEdges) => addEdge({ ...connection, id: crypto.randomUUID(), type: "smoothstep" }, currentEdges));
    }

    async function handleImportSvg(): Promise<void> {
        if (!selectedNode || selectedNode.data.kind !== "source" || !selectedBrowserPath.endsWith(".svg")) {
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
                        status: "completed"
                    }
                }
                : data);
            setBrowserDir(projectDir);
        } catch (error) {
            setErrorMessage(String(error));
        }
    }

    async function handleSaveGraph(): Promise<void> {
        try {
            await api.saveGraph(graphPath, toGraphDocument(nodes, edges));
        } catch (error) {
            setErrorMessage(String(error));
        }
    }

    async function handleLoadGraph(): Promise<void> {
        try {
            const graph = await api.loadGraph(graphPath);
            setNodes(graph.nodes.map((node) => ({ ...node, type: "workflow" })));
            setEdges(graph.edges.map((edge) => ({ ...edge, type: "smoothstep" })));
            setSelectedNodeId(graph.nodes[0]?.id ?? null);
        } catch (error) {
            setErrorMessage(String(error));
        }
    }

    async function handleRunSelected(): Promise<void> {
        if (!selectedNode) {
            return;
        }

        try {
            const graph = toGraphDocument(nodes, edges);
            const plan = buildGraphExecutionPlan(graph, selectedNode.id);
            setActivePlanNodeIds(plan.map((node) => node.id));
            setNodes((currentNodes) => currentNodes.map((node) => {
                const isInPlan = plan.some((planNode) => planNode.id === node.id);
                if (!isInPlan) {
                    return node;
                }

                return {
                    ...node,
                    data: {
                        ...node.data,
                        artifacts: {
                            ...(node.data.artifacts ?? {}),
                            status: "queued"
                        }
                    }
                };
            }));
            const response = await api.runGraph(graph, projectDir, selectedNode.id);
            const nextJob = await api.getJob(response.jobId);
            setJob(nextJob);
            setJobLog("");
        } catch (error) {
            setErrorMessage(String(error));
        }
    }

    function numberField(label: string, value: number, onChange: (nextValue: number) => void, step = "1") {
        return (
            <label>
                {label}
                <input type="number" value={value} step={step} onChange={(event) => onChange(Number(event.target.value))} />
            </label>
        );
    }

    function renderInspector() {
        if (!selectedNode) {
            return <div className="panel-empty">Select a node to edit its properties.</div>;
        }

        const data = selectedNode.data;

        return (
            <div className="inspector-form">
                <label>
                    Label
                    <input value={data.label} onChange={(event) => patchSelectedNode((current) => ({ ...current, label: event.target.value }))} />
                </label>

                {data.kind === "source" ? (
                    <label>
                        Source SVG
                        <input value={data.sourcePath} readOnly />
                    </label>
                ) : null}

                {data.kind === "grid" ? (
                    <>
                        {numberField("Simplification", data.config.simplificationOfOriginalSVG, (value) => patchGrid((config) => ({ ...config, simplificationOfOriginalSVG: value })), "0.1")}
                        {numberField("Max separation", data.config.maxSep, (value) => patchGrid((config) => ({ ...config, maxSep: value })), "0.1")}
                        {numberField("Min separation", data.config.minSep, (value) => patchGrid((config) => ({ ...config, minSep: value })), "0.1")}
                        {numberField("Seed", data.config.seed, (value) => patchGrid((config) => ({ ...config, seed: value })))}
                    </>
                ) : null}

                {data.kind === "route" ? (
                    <>
                        {numberField("Initial thickness", data.config.initialThickness, (value) => patchRoute((config) => ({ ...config, initialThickness: value })), "0.1")}
                        {numberField("Decrement factor", data.config.decrementFactor, (value) => patchRoute((config) => ({ ...config, decrementFactor: value })), "0.1")}
                        {numberField("Minimal thickness", data.config.minimalThickness, (value) => patchRoute((config) => ({ ...config, minimalThickness: value })), "0.1")}
                        {numberField("Smoothing tension", data.config.smoothingTension, (value) => patchRoute((config) => ({ ...config, smoothingTension: value })), "0.1")}
                        {numberField("Smoothing iteration", data.config.smoothingIteration, (value) => patchRoute((config) => ({ ...config, smoothingIteration: value })))}
                        {numberField("Max routing attempt", data.config.maxRoutingAttempt, (value) => patchRoute((config) => ({ ...config, maxRoutingAttempt: value })))}
                        {numberField("Routing seed", data.config.routing.seed, (value) => patchRoute((config) => ({ ...config, routing: { ...config.routing, seed: value } })))}
                        {numberField("Routing max random", data.config.routing.maxRandom, (value) => patchRoute((config) => ({ ...config, routing: { ...config.routing, maxRandom: value } })))}
                        {numberField("Distance unit cost", data.config.routing.distanceUnitCost, (value) => patchRoute((config) => ({ ...config, routing: { ...config.routing, distanceUnitCost: value } })))}
                        {numberField("Via unit cost", data.config.routing.viaUnitCost, (value) => patchRoute((config) => ({ ...config, routing: { ...config.routing, viaUnitCost: value } })))}
                        {numberField("Cell seed", data.config.cell.seed, (value) => patchRoute((config) => ({ ...config, cell: { ...config.cell, seed: value } })))}
                        {numberField("Cell max pin", data.config.cell.maxPin, (value) => patchRoute((config) => ({ ...config, cell: { ...config.cell, maxPin: value } })))}
                        {numberField("Cell start net", data.config.cell.startNet, (value) => patchRoute((config) => ({ ...config, cell: { ...config.cell, startNet: value } })))}
                        {numberField("Cell resolution", data.config.cell.resolution, (value) => patchRoute((config) => ({ ...config, cell: { ...config.cell, resolution: value } })), "0.1")}
                        <label className="checkbox-row">
                            <input type="checkbox" checked={data.config.enableAlternateRouting} onChange={(event) => patchRoute((config) => ({ ...config, enableAlternateRouting: event.target.checked }))} />
                            Enable alternate routing
                        </label>
                        {data.config.enableAlternateRouting ? (
                            <>
                                {numberField("Max thickness", data.config.alternateRouting.maxThickness, (value) => patchRoute((config) => ({ ...config, alternateRouting: { ...config.alternateRouting, maxThickness: value } })), "0.1")}
                                {numberField("Min thickness", data.config.alternateRouting.minThickness, (value) => patchRoute((config) => ({ ...config, alternateRouting: { ...config.alternateRouting, minThickness: value } })), "0.1")}
                                {numberField("Pruning", data.config.alternateRouting.pruning, (value) => patchRoute((config) => ({ ...config, alternateRouting: { ...config.alternateRouting, pruning: value } })))}
                                {numberField("Thickness percent", data.config.alternateRouting.thicknessPercent, (value) => patchRoute((config) => ({ ...config, alternateRouting: { ...config.alternateRouting, thicknessPercent: value } })), "0.1")}
                                {numberField("Simplify distance", data.config.alternateRouting.simplifyDist, (value) => patchRoute((config) => ({ ...config, alternateRouting: { ...config.alternateRouting, simplifyDist: value } })), "0.1")}
                            </>
                        ) : null}
                    </>
                ) : null}

                {data.kind === "render" ? (
                    <>
                        {numberField("Smoothing tension", data.config.smoothingTension, (value) => patchRender((config) => ({ ...config, smoothingTension: value })), "0.1")}
                        {numberField("Smoothing iterations", data.config.smoothingIterations, (value) => patchRender((config) => ({ ...config, smoothingIterations: value })), "0.1")}
                        {numberField("Pen thickness", data.config.penConfig.thickness, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, thickness: value } })), "0.05")}
                        {numberField("Antisymmetric amplitude", data.config.penConfig.antisymmetricAmplitude, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricAmplitude: value } })), "0.1")}
                        {numberField("Antisymmetric frequency", data.config.penConfig.antisymmetricFreq, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricFreq: value } })), "0.1")}
                        {numberField("Antisymmetric seed", data.config.penConfig.antisymmetricSeed, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, antisymmetricSeed: value } })), "0.1")}
                        {numberField("Symmetric amplitude", data.config.penConfig.symmetricAmplitude, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, symmetricAmplitude: value } })), "0.1")}
                        {numberField("Symmetric frequency", data.config.penConfig.symmetricFreq, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, symmetricFreq: value } })), "0.1")}
                        {numberField("Symmetric seed", data.config.penConfig.symmetricSeed, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, symmetricSeed: value } })), "0.1")}
                        {numberField("Resolution", data.config.penConfig.resolution, (value) => patchRender((config) => ({ ...config, penConfig: { ...config.penConfig, resolution: value } })), "0.1")}
                    </>
                ) : null}
            </div>
        );
    }

    return (
        <div className="app-shell">
            <header className="hero-strip">
                <div>
                    <p className="eyebrow">LabyStudio</p>
                    <h1>Browser workbench for LabyPath DAGs</h1>
                    <p className="hero-copy">
                        React Flow handles the graph, the local Node service handles local files and labypath execution, and the preview stays attached to the generated SVG artifacts.
                    </p>
                </div>
                <div className="context-card">
                    <div>Workspace: {context?.workspaceRoot ?? "loading"}</div>
                    <div>Binary: {context?.binaryPath ?? "loading"}</div>
                    <div>Binary ready: {String(context?.binaryExists ?? false)}</div>
                </div>
            </header>

            <section className="toolbar-row">
                <div className="toolbar-group">
                    <button onClick={() => setNodes((current) => [...current, createFlowNode("source", crypto.randomUUID(), 80, 340)])}>Add source</button>
                    <button onClick={() => setNodes((current) => [...current, createFlowNode("grid", crypto.randomUUID(), 340, 340)])}>Add grid</button>
                    <button onClick={() => setNodes((current) => [...current, createFlowNode("route", crypto.randomUUID(), 640, 340)])}>Add route</button>
                    <button onClick={() => setNodes((current) => [...current, createFlowNode("render", crypto.randomUUID(), 940, 340)])}>Add render</button>
                </div>
                <div className="toolbar-group toolbar-group--stretch">
                    <label>
                        Project dir
                        <input value={projectDir} onChange={(event) => setProjectDir(event.target.value)} />
                    </label>
                    <label>
                        Graph file
                        <input value={graphPath} onChange={(event) => setGraphPath(event.target.value)} />
                    </label>
                    <button onClick={handleSaveGraph}>Save graph</button>
                    <button onClick={handleLoadGraph}>Load graph</button>
                    <button onClick={handleRunSelected} disabled={!selectedNode}>Run selected</button>
                </div>
            </section>

            {errorMessage ? <div className="error-banner">{errorMessage}</div> : null}

            <main className="main-grid">
                <aside className="panel column-panel">
                    <div className="panel-header">
                        <h2>Files</h2>
                        <label>
                            Directory
                            <input value={browserDir} onChange={(event) => setBrowserDir(event.target.value)} />
                        </label>
                    </div>
                    <div className="file-actions">
                        <button onClick={() => setBrowserDir(browserDir.replace(/\/[^/]+$/, "") || "/")}>Up one level</button>
                        <button onClick={() => setProjectDir(selectedBrowserPath || browserDir)}>Use selected path as project root</button>
                        <button onClick={handleImportSvg} disabled={!selectedNode || selectedNode.data.kind !== "source" || !selectedBrowserPath.endsWith(".svg")}>Import SVG into source node</button>
                    </div>
                    <ul className="file-list">
                        {browserEntries.map((entry) => (
                            <li key={entry.path}>
                                <button
                                    className={selectedBrowserPath === entry.path ? "is-active" : ""}
                                    onClick={() => {
                                        setSelectedBrowserPath(entry.path);
                                        if (entry.kind === "directory") {
                                            setBrowserDir(entry.path);
                                        }
                                    }}
                                >
                                    <span>{entry.kind === "directory" ? "dir" : "file"}</span>
                                    <strong>{entry.name}</strong>
                                </button>
                            </li>
                        ))}
                    </ul>
                </aside>

                <section className="panel flow-panel">
                    <div className="panel-header">
                        <h2>Workflow graph</h2>
                        <p>Build valid source to grid to route to render chains and branch them as needed.</p>
                    </div>
                    <div className="flow-canvas">
                        <ReactFlow
                            nodes={nodes}
                            edges={edges}
                            nodeTypes={nodeTypes}
                            onNodesChange={onNodesChange}
                            onEdgesChange={onEdgesChange}
                            onConnect={handleConnect}
                            onNodeClick={(_, node) => setSelectedNodeId(node.id)}
                            fitView
                        >
                            <Background gap={18} color="rgba(48, 77, 85, 0.15)" />
                            <Controls />
                            <MiniMap pannable zoomable />
                        </ReactFlow>
                    </div>
                </section>

                <aside className="panel column-panel">
                    <div className="panel-header">
                        <h2>Inspector</h2>
                        <p>{selectedNode ? `Editing ${selectedNode.data.label}` : "No node selected"}</p>
                    </div>
                    {renderInspector()}
                </aside>

                <section className="panel preview-panel">
                    <div className="panel-header">
                        <h2>SVG preview</h2>
                        <p>{selectedNode ? basename(selectedNode.data.artifacts?.outputPath ?? (selectedNode.data.kind === "source" ? selectedNode.data.sourcePath : "")) : "Select a node or SVG file"}</p>
                    </div>
                    <div className="svg-preview" dangerouslySetInnerHTML={{ __html: previewSvg || "<p>No SVG preview available.</p>" }} />
                </section>

                <section className="panel log-panel">
                    <div className="panel-header">
                        <h2>Run log</h2>
                        <p>{job ? `${job.status}${job.currentNodeId ? ` on ${job.currentNodeId}` : ""}` : "No active job"}</p>
                    </div>
                    <pre>{jobLog || "Run a node to stream the combined job log here."}</pre>
                </section>
            </main>
        </div>
    );
}

export default App;