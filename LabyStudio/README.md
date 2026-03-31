# LabyStudio

LabyStudio is the new web workbench for the LabyPath pipeline. It pairs a local Node backend with a React Flow frontend so you can browse local files, assemble a valid source to grid to route to render graph, edit stage parameters in the browser, and launch `labypath` against the local filesystem.

## Runtime

Use the downloaded Node.js 24 toolchain from this workspace:

```bash
export PATH="/workspace/downloads/nodejs-v24/node-v24.14.1-linux-x64/bin:$PATH"
node --version
npm --version
```

## Install

```bash
cd /workspace/LabyStudio
npm install
```

## Develop

Start the backend:

```bash
cd /workspace/LabyStudio
export PATH="/workspace/downloads/nodejs-v24/node-v24.14.1-linux-x64/bin:$PATH"
npm run dev:server
```

Start the frontend:

```bash
cd /workspace/LabyStudio/apps/web
export PATH="/workspace/downloads/nodejs-v24/node-v24.14.1-linux-x64/bin:$PATH"
../../node_modules/.bin/vite --host 0.0.0.0 --port 4173
```

Open http://127.0.0.1:4173/ in a browser.

## Build

```bash
cd /workspace/LabyStudio
export PATH="/workspace/downloads/nodejs-v24/node-v24.14.1-linux-x64/bin:$PATH"
npm run build
```

## Current scope

- Local filesystem browsing and SVG import into the selected source node.
- React Flow graph editing for source, grid, route, and render nodes.
- Schema-driven parameter editing for the existing LabyPath stages.
- Graph save/load through the backend.
- `labypath` execution with combined job logs and generated SVG preview.

The current implementation is intentionally local-first and trusts the local machine. It does not yet include authentication, multi-user isolation, or a visual SVG editor.