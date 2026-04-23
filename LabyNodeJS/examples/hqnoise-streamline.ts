import path from "node:path";
import { noise, pipeline, runPipeline, streamline } from "../src/index.js";

async function main(): Promise<void> {
  const workspaceRoot = path.resolve(process.cwd(), "..");
  const projectDir = path.join(workspaceRoot, "LabyData");

  const result = await runPipeline(
    pipeline(
      noise(
        {
          maxN: 16,
          accuracy: 8,
          amplitude: 1,
          seed: 7,
          gaussianFrequency: 2.5,
          powerlawFrequency: 1.2,
          powerlawPower: 2,
          complex: true,
          width: 64,
          height: 64,
          scale: 0.25,
          previewMode: "ARROWS",
          previewStride: 2,
        },
        { label: "HQ noise field" },
      ),
      streamline(
        {
          simplifyDistance: 0.05,
          dRat: 1,
          divisor: 0.45,
          strokeThickness: 0.1,
        },
        { label: "Streamline render" },
      ),
    ),
    {
      projectDir,
      workspaceRoot,
      gallery: {
        enabled: true,
        keepAlive: true,
        openBrowser: true,
        port: 0,
        title: "LabyNodeJS HQ Noise Streamline Gallery",
      },
    },
  );

  if (result.galleryUrl !== undefined) {
    console.log(`Gallery: ${result.galleryUrl}`);
  }
  console.log(JSON.stringify(result, null, 2));
}

void main();
