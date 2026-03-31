import type { NodeKind } from "@labystudio/shared";

type WorkspaceToolbarProps = {
    projectDir: string;
    graphPath: string;
    onProjectDirChange: (value: string) => void;
    onGraphPathChange: (value: string) => void;
    onSaveGraph: () => void;
    onLoadGraph: () => void;
    onRunSelected: () => void;
    canRunSelected: boolean;
    onAddNode: (kind: NodeKind, position: { x: number; y: number }) => void;
};

const toolbarNodes: Array<{ kind: NodeKind; label: string; position: { x: number; y: number } }> = [
    { kind: "source", label: "Add source", position: { x: 80, y: 340 } },
    { kind: "grid", label: "Add grid", position: { x: 340, y: 340 } },
    { kind: "route", label: "Add route", position: { x: 640, y: 340 } },
    { kind: "render", label: "Add render", position: { x: 940, y: 340 } }
];

export default function WorkspaceToolbar({
    projectDir,
    graphPath,
    onProjectDirChange,
    onGraphPathChange,
    onSaveGraph,
    onLoadGraph,
    onRunSelected,
    canRunSelected,
    onAddNode
}: WorkspaceToolbarProps) {
    return (
        <section className="toolbar-row">
            <div className="toolbar-group">
                {toolbarNodes.map((toolbarNode) => (
                    <button
                        key={toolbarNode.kind}
                        onClick={() => onAddNode(toolbarNode.kind, toolbarNode.position)}
                    >
                        {toolbarNode.label}
                    </button>
                ))}
            </div>
            <div className="toolbar-group toolbar-group--stretch">
                <label>
                    Project dir
                    <input value={projectDir} onChange={(event) => onProjectDirChange(event.target.value)} />
                </label>
                <label>
                    Graph file
                    <input value={graphPath} onChange={(event) => onGraphPathChange(event.target.value)} />
                </label>
                <button onClick={onSaveGraph}>Save graph</button>
                <button onClick={onLoadGraph}>Load graph</button>
                <button onClick={onRunSelected} disabled={!canRunSelected}>Run selected</button>
            </div>
        </section>
    );
}