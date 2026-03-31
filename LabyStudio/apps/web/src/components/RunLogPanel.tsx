type RunLogPanelProps = {
    status: string;
    jobLog: string;
};

export default function RunLogPanel({ status, jobLog }: RunLogPanelProps) {
    return (
        <section className="panel log-panel">
            <div className="panel-header">
                <h2>Run log</h2>
                <p>{status}</p>
            </div>
            <pre>{jobLog || "Run a node to stream the combined job log here."}</pre>
        </section>
    );
}