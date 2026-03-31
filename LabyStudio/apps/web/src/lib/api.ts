import type {
  DirectoryListing,
  GraphDocument,
  RuntimeContext,
  WorkflowJob
} from "@labystudio/shared";

const API_ROOT = "http://127.0.0.1:4310/api";

async function requestJson<T>(pathname: string, init?: RequestInit): Promise<T> {
  const response = await fetch(`${API_ROOT}${pathname}`, {
    ...init,
    headers: {
      "Content-Type": "application/json",
      ...(init?.headers ?? {})
    }
  });

  if (!response.ok) {
    throw new Error(await response.text());
  }

  return response.json() as Promise<T>;
}

export const api = {
  getContext(): Promise<RuntimeContext> {
    return requestJson<RuntimeContext>("/context");
  },

  listDirectory(dir: string): Promise<DirectoryListing> {
    return requestJson<DirectoryListing>(`/fs/list?dir=${encodeURIComponent(dir)}`);
  },

  importSvg(sourcePath: string, projectDir: string): Promise<{ importedPath: string }> {
    return requestJson<{ importedPath: string }>("/fs/import", {
      method: "POST",
      body: JSON.stringify({ sourcePath, projectDir })
    });
  },

  loadGraph(path: string): Promise<GraphDocument> {
    return requestJson<GraphDocument>(`/graph/load?path=${encodeURIComponent(path)}`);
  },

  saveGraph(path: string, graph: GraphDocument): Promise<{ path: string }> {
    return requestJson<{ path: string }>("/graph/save", {
      method: "POST",
      body: JSON.stringify({ path, graph })
    });
  },

  runGraph(graph: GraphDocument, projectDir: string, targetNodeId: string): Promise<{ jobId: string }> {
    return requestJson<{ jobId: string }>("/jobs/run", {
      method: "POST",
      body: JSON.stringify({ graph, projectDir, targetNodeId })
    });
  },

  getJob(jobId: string): Promise<WorkflowJob> {
    return requestJson<WorkflowJob>(`/jobs/${encodeURIComponent(jobId)}`);
  },

  getJobLog(jobId: string): Promise<{ log: string }> {
    return requestJson<{ log: string }>(`/jobs/${encodeURIComponent(jobId)}/log`);
  },

  svgUrl(path: string): string {
    return `${API_ROOT}/fs/svg?path=${encodeURIComponent(path)}`;
  }
};