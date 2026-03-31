import { useEffect, useMemo, useState } from "react";
import type { Node } from "reactflow";
import {
    type NumberFieldConfig,
    type GridConfig,
    type RenderConfig,
    type RouteConfig,
    type WorkflowNodeData
} from "@labystudio/shared";
import SvgPreview from "./SvgPreview";
import {
    buildStagePayload,
    type StageConfigByKind,
    stageInspectorDefinitions,
    type StageKind
} from "./nodeInspectorConfig";
import type { DisplayNodeData, WorkflowNodeDisplayData } from "../lib/workflowGraph";

type StagePatchHandlers = {
    [K in StageKind]: (update: (config: StageConfigByKind[K]) => StageConfigByKind[K]) => void;
};

type NodeInspectorProps = {
    selectedNode: Node<WorkflowNodeData> | null;
    selectedCanvasNode: Node<DisplayNodeData> | null;
    selectedDisplayNode: Node<WorkflowNodeDisplayData> | null;
    onPatchSelectedNode: (update: (data: WorkflowNodeData) => WorkflowNodeData) => void;
    onPatchGrid: (update: (config: GridConfig) => GridConfig) => void;
    onPatchRoute: (update: (config: RouteConfig) => RouteConfig) => void;
    onPatchRender: (update: (config: RenderConfig) => RenderConfig) => void;
};

function NumberField({ label, value, step = "1", onChange }: { label: string; value: number; step?: string; onChange: (value: number) => void; }) {
    return (
        <label>
            {label}
            <input type="number" value={value} step={step} onChange={(event) => { onChange(Number(event.target.value)); }} />
        </label>
    );
}

function renderNumberFields<T>(
    fields: readonly NumberFieldConfig<T>[],
    config: T,
    onChange: (update: (config: T) => T) => void
) {
    return fields.map((field) => (
        <NumberField
            key={field.label}
            label={field.label}
            value={field.getValue(config)}
            step={field.step}
            onChange={(value) => { onChange((currentConfig) => field.setValue(currentConfig, value)); }}
        />
    ));
}

function patchStageConfig<K extends StageKind>(kind: K, nextConfig: StageConfigByKind[K], patchHandlers: StagePatchHandlers) {
    patchHandlers[kind](() => nextConfig);
}

export default function NodeInspector({
    selectedNode,
    selectedCanvasNode,
    selectedDisplayNode,
    onPatchSelectedNode,
    onPatchGrid,
    onPatchRoute,
    onPatchRender
}: NodeInspectorProps) {
    const [jsonDraft, setJsonDraft] = useState("");
    const [jsonError, setJsonError] = useState("");
    const nodeData = selectedNode?.data;
    const isTransformerNode = nodeData?.kind === "grid" || nodeData?.kind === "route" || nodeData?.kind === "render";
    const selectedArtifact = selectedCanvasNode?.data.displayType === "artifact" ? selectedCanvasNode.data : null;
    const sourceSvgPath = selectedDisplayNode?.data.resolvedSourcePath ?? (nodeData?.kind === "source" ? nodeData.sourcePath : "");
    const configPath = isTransformerNode ? (nodeData.artifacts?.configPath ?? "") : "";
    const outputSvgPath = selectedDisplayNode?.data.resolvedOutputPath ?? (nodeData?.kind === "source" ? nodeData.sourcePath : "");
    const showStructuredEditor = isTransformerNode && selectedArtifact === null;
    const transformerConfig = nodeData?.kind === "grid" || nodeData?.kind === "route" || nodeData?.kind === "render"
        ? nodeData.config
        : null;
    const stagePatchHandlers: StagePatchHandlers = {
        grid: onPatchGrid,
        route: onPatchRoute,
        render: onPatchRender
    };
    const generatedPayload = useMemo(() => {
        if (!nodeData || !isTransformerNode) {
            return "";
        }

        const safeInput = sourceSvgPath || "/input.svg";
        const safeOutput = outputSvgPath || "/output.svg";

        return buildStagePayload(nodeData.kind, safeInput, safeOutput, nodeData.config);
    }, [isTransformerNode, nodeData, outputSvgPath, sourceSvgPath]);

    useEffect(() => {
        if (!selectedNode || (selectedNode.data.kind !== "grid" && selectedNode.data.kind !== "route" && selectedNode.data.kind !== "render")) {
            setJsonDraft("");
            setJsonError("");
            return;
        }

        setJsonDraft(`${JSON.stringify(selectedNode.data.config, null, 2)}\n`);
        setJsonError("");
    }, [selectedNode]);

    if (!selectedNode) {
        return <div className="panel-empty">Select a node to edit its properties.</div>;
    }

    if (!nodeData) {
        return <div className="panel-empty">Select a node to edit its properties.</div>;
    }

    const data = nodeData;

    function applyJsonDraft() {
        if (!isTransformerNode) {
            return;
        }

        try {
            const parsed: unknown = JSON.parse(jsonDraft);

            if (data.kind === "grid") {
                patchStageConfig("grid", stageInspectorDefinitions.grid.parse(parsed), stagePatchHandlers);
            } else if (data.kind === "route") {
                patchStageConfig("route", stageInspectorDefinitions.route.parse(parsed), stagePatchHandlers);
            } else {
                patchStageConfig("render", stageInspectorDefinitions.render.parse(parsed), stagePatchHandlers);
            }

            setJsonError("");
        } catch (error) {
            setJsonError(error instanceof Error ? error.message : String(error));
        }
    }

    function renderStructuredConfigEditor() {
        if (!isTransformerNode) {
            return null;
        }

        if (data.kind === "grid") {
            const gridConfig = stageInspectorDefinitions.grid.parse(data.config);
            return renderNumberFields(stageInspectorDefinitions.grid.fields, gridConfig, (update) => { onPatchGrid(update); });
        }

        if (data.kind === "route") {
            const routeConfig = stageInspectorDefinitions.route.parse(data.config);
            const routeDefinition = stageInspectorDefinitions.route;
            const toggleFieldGroup = stageInspectorDefinitions.route.toggleFieldGroup;
            const baseRouteFields = routeDefinition.fields.filter((field) => !field.id.startsWith("alternateRouting."));

            return (
                <>
                    {renderNumberFields(baseRouteFields, routeConfig, (update) => { onPatchRoute(update); })}
                    <>
                        <label className="checkbox-row">
                            <input
                                type="checkbox"
                                checked={toggleFieldGroup.isEnabled(routeConfig)}
                                onChange={(event) => { onPatchRoute((currentConfig) => toggleFieldGroup.setEnabled(currentConfig, event.target.checked)); }}
                            />
                            {toggleFieldGroup.label}
                        </label>
                        {toggleFieldGroup.isEnabled(routeConfig)
                            ? renderNumberFields(toggleFieldGroup.fields, routeConfig, (update) => { onPatchRoute(update); })
                            : null}
                    </>
                </>
            );
        }

        if (data.kind !== "render") {
            return null;
        }

        const renderConfig = stageInspectorDefinitions.render.parse(data.config);
        return renderNumberFields(stageInspectorDefinitions.render.fields, renderConfig, (update) => { onPatchRender(update); });
    }

    return (
        <div className="inspector-form inspector-form--rich">
            <section className="inspector-card">
                <div className="inspector-card__eyebrow">Selection</div>
                <div className="inspector-title-row">
                    <strong>{selectedArtifact ? selectedArtifact.title : data.label}</strong>
                    <span className="inspector-chip">{selectedArtifact ? selectedArtifact.artifactType : data.kind}</span>
                </div>
                <div className="inspector-meta-grid">
                    <div>
                        <span>Owner</span>
                        <strong>{data.label}</strong>
                    </div>
                    <div>
                        <span>Logical node</span>
                        <strong>{selectedNode.id}</strong>
                    </div>
                </div>
            </section>

            {selectedArtifact?.artifactType === "svg" ? (
                <section className="inspector-card">
                    <div className="inspector-card__eyebrow">SVG Artifact</div>
                    <div className="inspector-meta-grid inspector-meta-grid--stacked">
                        <div>
                            <span>File</span>
                            <strong>{selectedArtifact.value}</strong>
                        </div>
                        <div>
                            <span>Path</span>
                            <strong>{selectedArtifact.path ?? "Not generated yet"}</strong>
                        </div>
                    </div>
                    <SvgPreview path={selectedArtifact.path} cacheKey={selectedArtifact.cacheKey} title={selectedArtifact.value} />
                </section>
            ) : null}

            <section className="inspector-card">
                <div className="inspector-card__eyebrow">Artifacts</div>
                <div className="inspector-meta-grid inspector-meta-grid--stacked">
                    <div>
                        <span>Source SVG</span>
                        <strong>{sourceSvgPath || "Not connected"}</strong>
                    </div>
                    {isTransformerNode ? (
                        <div>
                            <span>Config JSON</span>
                            <strong>{configPath || "Generated on run"}</strong>
                        </div>
                    ) : null}
                    <div>
                        <span>Output SVG</span>
                        <strong>{outputSvgPath || "Pending"}</strong>
                    </div>
                </div>
            </section>

            <section className="inspector-card">
                <div className="inspector-card__eyebrow">Node</div>
                <label>
                    Label
                    <input value={data.label} onChange={(event) => { onPatchSelectedNode((current) => ({ ...current, label: event.target.value })); }} />
                </label>

                {data.kind === "source" ? (
                    <label>
                        Source SVG Path
                        <input value={data.sourcePath} readOnly />
                    </label>
                ) : null}

                {data.kind === "numericConstant" ? (
                    <label>
                        Value
                        <input
                            type="number"
                            value={data.value}
                            onChange={(event) => { onPatchSelectedNode((current) => current.kind === "numericConstant" ? { ...current, value: Number(event.target.value) } : current); }}
                        />
                    </label>
                ) : null}

                {data.kind === "operation" ? (
                    <>
                        <label>
                            Operation
                            <select value={data.operation} onChange={(event) => { onPatchSelectedNode((current) => current.kind === "operation" ? { ...current, operation: event.target.value as "add" | "multiply" } : current); }}>
                                <option value="add">Add</option>
                                <option value="multiply">Multiply</option>
                            </select>
                        </label>
                        <label>
                            Left fallback
                            <input
                                type="number"
                                value={data.left}
                                onChange={(event) => { onPatchSelectedNode((current) => current.kind === "operation" ? { ...current, left: Number(event.target.value) } : current); }}
                            />
                        </label>
                        <label>
                            Right fallback
                            <input
                                type="number"
                                value={data.right}
                                onChange={(event) => { onPatchSelectedNode((current) => current.kind === "operation" ? { ...current, right: Number(event.target.value) } : current); }}
                            />
                        </label>
                    </>
                ) : null}

                {data.kind === "broadcast" ? (
                    <>
                        <label>
                            Input fallback
                            <input
                                type="number"
                                value={data.value}
                                onChange={(event) => { onPatchSelectedNode((current) => current.kind === "broadcast" ? { ...current, value: Number(event.target.value) } : current); }}
                            />
                        </label>
                        <label>
                            Outputs
                            <input
                                type="number"
                                min={2}
                                max={8}
                                step={1}
                                value={data.outputs}
                                onChange={(event) => { onPatchSelectedNode((current) => current.kind === "broadcast" ? { ...current, outputs: Math.max(2, Math.min(8, Math.round(Number(event.target.value)))) } : current); }}
                            />
                        </label>
                    </>
                ) : null}
            </section>

            {showStructuredEditor ? (
                <section className="inspector-card">
                    <div className="inspector-card__eyebrow">Structured Config</div>
                    <div className="inspector-field-grid">
                        {renderStructuredConfigEditor()}
                    </div>
                </section>
            ) : null}

            {isTransformerNode ? (
                <section className="inspector-card">
                    <div className="inspector-card__eyebrow">Config Object JSON</div>
                    <p className="inspector-card__copy">Edit the node configuration directly. This drives the generated JSON artifact for the selected stage.</p>
                    <textarea
                        className="inspector-json"
                        value={jsonDraft}
                        onChange={(event) => {
                            setJsonDraft(event.target.value);
                            if (jsonError) {
                                setJsonError("");
                            }
                        }}
                    />
                    {jsonError ? <div className="error-banner inspector-error">{jsonError}</div> : null}
                    <div className="inspector-actions">
                        <button type="button" onClick={applyJsonDraft}>Apply JSON</button>
                        <button type="button" onClick={() => { setJsonDraft(`${JSON.stringify(transformerConfig, null, 2)}\n`); }}>Reset Draft</button>
                    </div>
                </section>
            ) : null}

            {isTransformerNode ? (
                <section className="inspector-card">
                    <div className="inspector-card__eyebrow">Generated Stage Payload</div>
                    <p className="inspector-card__copy">This is the concrete payload written to disk when the stage runs.</p>
                    <textarea className="inspector-json inspector-json--readonly" value={generatedPayload} readOnly />
                </section>
            ) : null}
        </div>
    );
}