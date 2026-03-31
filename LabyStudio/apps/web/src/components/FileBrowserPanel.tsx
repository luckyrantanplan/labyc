import type { DirectoryEntry } from "@labystudio/shared";

type FileBrowserPanelProps = {
    browserDir: string;
    browserEntries: DirectoryEntry[];
    selectedBrowserPath: string;
    onBrowserDirChange: (value: string) => void;
    onSelectEntry: (entry: DirectoryEntry) => void;
    onGoUp: () => void;
    onUseSelectedPathAsProjectRoot: () => void;
    onImportSvg: () => void;
    canImportSvg: boolean;
};

export default function FileBrowserPanel({
    browserDir,
    browserEntries,
    selectedBrowserPath,
    onBrowserDirChange,
    onSelectEntry,
    onGoUp,
    onUseSelectedPathAsProjectRoot,
    onImportSvg,
    canImportSvg
}: FileBrowserPanelProps) {
    return (
        <aside className="panel column-panel file-panel">
            <div className="panel-header">
                <h2>Files</h2>
                <label>
                    Directory
                    <input value={browserDir} onChange={(event) => { onBrowserDirChange(event.target.value); }} />
                </label>
            </div>
            <div className="file-actions">
                <button onClick={onGoUp}>Up one level</button>
                <button onClick={onUseSelectedPathAsProjectRoot}>Use selected path as project root</button>
                <button onClick={onImportSvg} disabled={!canImportSvg}>Import SVG into source node</button>
            </div>
            <ul className="file-list">
                {browserEntries.map((entry) => (
                    <li key={entry.path}>
                        <button
                            className={selectedBrowserPath === entry.path ? "is-active" : ""}
                            onClick={() => { onSelectEntry(entry); }}
                        >
                            <span>{entry.kind === "directory" ? "dir" : "file"}</span>
                            <strong>{entry.name}</strong>
                        </button>
                    </li>
                ))}
            </ul>
        </aside>
    );
}