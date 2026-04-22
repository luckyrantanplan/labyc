# LabyNodeJS

LabyNodeJS is the TypeScript orchestration layer for the `labypath` CLI. It is a library-first package for small experiments: you compose source, grid, route, and render stages in TypeScript, then execute them against the existing C++ binary with deterministic memoization.

## What It Does

- keeps the C++ engine as the stable execution boundary
- builds protobuf-shaped JSON payloads for each stage
- hashes the effective payload, input SVG, and binary to avoid recomputing identical jobs
- stores cache, configs, logs, and SVG files inside a dedicated project data folder
- writes generated SVG outputs into that folder's `svg/` subdirectory

## Install

```bash
cd /workspace/LabyNodeJS
npm install
```

## Validate

```bash
npm run build
npm run lint
npm test
```

The test suite uses the built-in `node:test` runner. Coverage is enforced with `c8` and must stay above 80%.

## Example

```bash
cd /workspace/LabyNodeJS
npm run example
```

The example reads `LabyData/svg/square_circleorig.svg`, runs grid, route, and render stages, and prints the resulting artifact metadata.

For a full explicit reference configuration, see [examples/defaut.ts](./examples/defaut.ts).

When the example launches a pipeline, it also starts a local HTML gallery and opens it in the browser referenced by `$BROWSER`. Each stage card begins in `waiting` until its SVG becomes available, then updates in place as files are created or reused from cache.

## Programming Model

```ts
import { grid, pipeline, render, route, runPipeline, source } from "labynodejs";

const result = await runPipeline(
  pipeline(
    source("/workspace/LabyData/svg/square_circleorig.svg"),
    grid({
      simplificationOfOriginalSVG: 0.1,
      maxSep: 5,
      minSep: 0.1,
      seed: 9
    }),
    route({
      initialThickness: 1.8,
      decrementFactor: 1.5,
      minimalThickness: 0.5,
      smoothingTension: 1,
      smoothingIteration: 5,
      maxRoutingAttempt: 300,
      routing: {
        seed: 5,
        maxRandom: 300,
        distanceUnitCost: 1,
        viaUnitCost: 10
      },
      cell: {
        seed: 1,
        maxPin: 400,
        startNet: 30,
        resolution: 1
      },
      enableAlternateRouting: false,
      alternateRouting: {
        maxThickness: 1.8,
        minThickness: 0.5,
        pruning: 0,
        thicknessPercent: 1,
        simplifyDist: 0
      }
    }),
    render({
      smoothingTension: 0.5,
      smoothingIterations: 3,
      penConfig: {
        thickness: 0.35,
        antisymmetricAmplitude: 0.3,
        antisymmetricFreq: 10,
        antisymmetricSeed: 5,
        symmetricAmplitude: 0.1,
        symmetricFreq: 3,
        symmetricSeed: 8,
        resolution: 1
      }
    })
  ),
  {
    projectDir: "/workspace/LabyData"
  }
);
```

## Cache Layout

- cache manifest: `projectDir/cache/cache.json`
- stage configs: `projectDir/configs/*.json`
- stage logs: `projectDir/logs/*.log`
- stage SVG files: `projectDir/svg/*.svg`

Cache entries are keyed by stage kind, normalized payload hash, input SVG hash, and binary fingerprint. A cache hit reuses the stored output only if the output file still exists and its current hash matches the manifest.