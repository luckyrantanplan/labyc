type AppHeaderProps = {
    workspaceRoot?: string;
    binaryPath?: string;
    binaryExists?: boolean;
};

export default function AppHeader({ workspaceRoot, binaryPath, binaryExists }: AppHeaderProps) {
    return (
        <header className="hero-strip">
            <div>
                <p className="eyebrow">LabyStudio</p>
                <h1>Browser workbench for LabyPath DAGs</h1>
                <p className="hero-copy">
                    React Flow handles the graph, the local Node service handles local files and labypath execution, and the preview stays attached to the generated SVG artifacts.
                </p>
            </div>
            <div className="context-card">
                <div>Workspace: {workspaceRoot ?? "loading"}</div>
                <div>Binary: {binaryPath ?? "loading"}</div>
                <div>Binary ready: {String(binaryExists ?? false)}</div>
            </div>
        </header>
    );
}