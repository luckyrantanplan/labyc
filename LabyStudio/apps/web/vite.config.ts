import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

const host = process.env.LABYSTUDIO_WEB_HOST ?? "0.0.0.0";
const port = Number(process.env.LABYSTUDIO_WEB_PORT ?? 4173);

export default defineConfig({
  plugins: [react()],
  server: {
    port,
    host
  },
  preview: {
    port,
    host
  }
});