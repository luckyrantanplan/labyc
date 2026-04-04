# LabyStudio

LabyStudio is the new web workbench for the LabyPath pipeline. It pairs a local Node backend with a React Flow frontend so you can browse local files, assemble a valid source to grid to route to render graph, edit stage parameters in the browser, and launch `labypath` against the local filesystem.

## Runtime

LabyStudio development uses Vite 7 and the devcontainer now installs the required Node.js 24 toolchain directly into the container:

```bash
node --version
npm --version
```

## Install

```bash
cd /workspace/LabyStudio
npm install
```

## Develop

Run the workspace scripts from `/workspace/LabyStudio` in separate terminals.

Start the backend:

```bash
cd /workspace/LabyStudio
npm run dev:server
```

Start the frontend:

```bash
cd /workspace/LabyStudio
npm run dev:web -- --host 0.0.0.0 --port 4173
```

Open http://127.0.0.1:4173/ in a browser.

## Build

```bash
cd /workspace/LabyStudio
npm run build
```

## Test

```bash
cd /workspace/LabyStudio
npm run test
```

## Current scope

- Local filesystem browsing and SVG import into the selected source node.
- React Flow graph editing for source, grid, route, and render nodes.
- Schema-driven parameter editing for the existing LabyPath stages.
- Graph save/load through the backend.
- `labypath` execution with combined job logs and generated SVG preview.

The current implementation is intentionally local-first and trusts the local machine. It does not yet include authentication, multi-user isolation, or a visual SVG editor.