import { createApp } from "./app.js";

const port = Number(process.env.LABYSTUDIO_PORT ?? 4310);
const app = await createApp();

app.listen(port, "0.0.0.0", () => {
  console.log(`LabyStudio server listening on http://0.0.0.0:${port}`);
});