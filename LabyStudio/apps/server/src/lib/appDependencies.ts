import type { GraphDocument, RuntimeContext } from "@labystudio/shared";
import type { JobStore } from "./jobs.js";

export type ResolveContext = () => RuntimeContext | Promise<RuntimeContext>;
export type CleanupExpiredJobs = (jobStore: JobStore) => void;
export type ExecuteGraphJob = (
  jobStore: JobStore,
  jobId: string,
  graph: GraphDocument,
  projectDir: string,
  targetNodeId: string
) => Promise<void>;

export type AppDependencies = {
  resolveContext: ResolveContext;
  cleanupExpiredJobs: CleanupExpiredJobs;
  executeGraphJob: ExecuteGraphJob;
};