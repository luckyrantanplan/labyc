import { appendFile, mkdir, writeFile } from "node:fs/promises";
import path from "node:path";

type LogDetails = Record<string, string | number | boolean | undefined>;

export interface RunLogger {
  readonly logPath: string;
  info(message: string, details?: LogDetails): Promise<void>;
  error(message: string, details?: LogDetails): Promise<void>;
  close(): Promise<void>;
}

function createLogStem(now: Date): string {
  return now.toISOString().replaceAll(":", "-").replaceAll(".", "-");
}

function formatDetails(details: LogDetails | undefined): string {
  if (details === undefined) {
    return "";
  }

  const parts = Object.entries(details)
    .filter(([, value]) => value !== undefined)
    .map(([key, value]) => `${key}=${String(value)}`);

  return parts.length === 0 ? "" : ` | ${parts.join(" ")}`;
}

function buildLine(
  level: "INFO" | "ERROR",
  message: string,
  details: LogDetails | undefined,
): string {
  return `[${new Date().toISOString()}] [${level}] ${message}${formatDetails(details)}\n`;
}

export function formatDuration(durationMs: number): string {
  if (durationMs < 1000) {
    return `${String(durationMs)}ms`;
  }

  return `${(durationMs / 1000).toFixed(3)}s`;
}

export async function createRunLogger(logDir: string): Promise<RunLogger> {
  await mkdir(logDir, { recursive: true });
  const logPath = path.join(
    logDir,
    `pipeline-${createLogStem(new Date())}.log`,
  );
  await writeFile(logPath, "", "utf8");

  let writeQueue = Promise.resolve();

  const writeLine = async (
    level: "INFO" | "ERROR",
    message: string,
    details?: LogDetails,
  ): Promise<void> => {
    const line = buildLine(level, message, details);
    const writer = level === "ERROR" ? process.stderr : process.stdout;
    writer.write(line);
    writeQueue = writeQueue.then(async () => appendFile(logPath, line, "utf8"));
    await writeQueue;
  };

  await writeLine("INFO", "Pipeline log created", { logPath });

  return {
    logPath,
    info(message, details) {
      return writeLine("INFO", message, details);
    },
    error(message, details) {
      return writeLine("ERROR", message, details);
    },
    async close() {
      await writeQueue;
    },
  };
}
