import { createApp } from "./app.js";

const port = Number(process.env.LABYSTUDIO_PORT ?? 4310);

try {
  const app = createApp();
  const server = app.listen(port, "0.0.0.0", () => {
    console.log(`LabyStudio server listening on http://0.0.0.0:${port}`);
  });

  server.on("error", (error) => {
    console.error(`Failed to bind LabyStudio server on port ${port}:`, error);
    process.exit(1);
  });
} catch (error) {
  console.error("Failed to start LabyStudio server:", error);
  process.exit(1);
}