# LabyNodeJS Config And Cache

This page documents how the LabyNodeJS stage model maps to the protobuf-backed JSON files passed to `labypath`, and how its project-local cache avoids repeated native execution.

The authoritative sources are:

- [../LabyPath/API/AllConfig.proto](../LabyPath/API/AllConfig.proto)
- [../LabyNodeJS/src/types.ts](../LabyNodeJS/src/types.ts)
- [../LabyNodeJS/src/payload.ts](../LabyNodeJS/src/payload.ts)
- [../LabyNodeJS/src/runner.ts](../LabyNodeJS/src/runner.ts)
- [../LabyNodeJS/src/cache.ts](../LabyNodeJS/src/cache.ts)

## Mapping Rules

- `AllConfig.proto` is the source of truth for the engine contract.
- LabyNodeJS emits protobuf JSON field names, so JSON keys use camelCase even when the `.proto` file uses snake_case.
- Each stage builder emits only the message fragment required for that stage.
- `maxRoutingAttempt` is serialized as a string to match protobuf JSON handling for `uint64`.
- `enableAlternateRouting` is a TypeScript-side convenience field that controls whether `routing.alternateRouting` is emitted.

## Stage Payload Mapping

### Grid Stage

| LabyNodeJS field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| stage input path | `skeletonGrid.inputfile` | `SkeletonGrid.inputfile` | Comes from the upstream artifact path. |
| stage output path | `skeletonGrid.outputfile` | `SkeletonGrid.outputfile` | Derived from the stage kind and cache key. |
| `simplificationOfOriginalSVG` | `skeletonGrid.simplificationOfOriginalSVG` | `SkeletonGrid.simplificationOfOriginalSVG` | Same name in TypeScript and protobuf JSON. |
| `maxSep` | `skeletonGrid.maxSep` | `SkeletonGrid.max_sep` | JSON uses protobuf camelCase. |
| `minSep` | `skeletonGrid.minSep` | `SkeletonGrid.min_sep` | JSON uses protobuf camelCase. |
| `seed` | `skeletonGrid.seed` | `SkeletonGrid.seed` | Integer value passed through unchanged. |

### Route Stage

#### Shared File Paths

| LabyNodeJS field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| stage input path | `routing.filepaths.inputfile` | `Routing.filepaths.inputfile` | Comes from the upstream artifact path. |
| stage output path | `routing.filepaths.outputfile` | `Routing.filepaths.outputfile` | Derived from the stage kind and cache key. |

#### Placement Fields

| LabyNodeJS field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| `initialThickness` | `routing.placement.initialThickness` | `Placement.initial_thickness` | JSON uses camelCase. |
| `decrementFactor` | `routing.placement.decrementFactor` | `Placement.decrement_factor` | JSON uses camelCase. |
| `minimalThickness` | `routing.placement.minimalThickness` | `Placement.minimal_thickness` | JSON uses camelCase. |
| `smoothingTension` | `routing.placement.smoothingTension` | `Placement.smoothing_tension` | JSON uses camelCase. |
| `smoothingIteration` | `routing.placement.smoothingIteration` | `Placement.smoothing_iteration` | JSON uses camelCase. |
| `maxRoutingAttempt` | `routing.placement.maxRoutingAttempt` | `Placement.max_routing_attempt` | Serialized as a string for protobuf JSON. |
| `routing.seed` | `routing.placement.routing.seed` | `RoutingCost.seed` | Passed through unchanged. |
| `routing.maxRandom` | `routing.placement.routing.maxRandom` | `RoutingCost.max_random` | JSON uses camelCase. |
| `routing.distanceUnitCost` | `routing.placement.routing.distanceUnitCost` | `RoutingCost.distance_unit_cost` | JSON uses camelCase. |
| `routing.viaUnitCost` | `routing.placement.routing.viaUnitCost` | `RoutingCost.via_unit_cost` | JSON uses camelCase. |
| `cell.seed` | `routing.placement.cell.seed` | `Cell.seed` | Passed through unchanged. |
| `cell.maxPin` | `routing.placement.cell.maxPin` | `Cell.maxPin` | Same name in TypeScript and protobuf JSON. |
| `cell.startNet` | `routing.placement.cell.startNet` | `Cell.startNet` | Same name in TypeScript and protobuf JSON. |
| `cell.resolution` | `routing.placement.cell.resolution` | `Cell.resolution` | Passed through unchanged. |

#### Alternate Routing Fields

| LabyNodeJS field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| `enableAlternateRouting` | not emitted directly | none | TypeScript-only gate that controls whether `alternateRouting` is included. |
| `alternateRouting.maxThickness` | `routing.alternateRouting.maxThickness` | `AlternateRouting.maxThickness` | Emitted only when enabled. |
| `alternateRouting.minThickness` | `routing.alternateRouting.minThickness` | `AlternateRouting.minThickness` | Emitted only when enabled. |
| `alternateRouting.pruning` | `routing.alternateRouting.pruning` | `AlternateRouting.pruning` | Passed through unchanged. |
| `alternateRouting.thicknessPercent` | `routing.alternateRouting.thicknessPercent` | `AlternateRouting.thicknessPercent` | Passed through unchanged. |
| `alternateRouting.simplifyDist` | `routing.alternateRouting.simplifyDist` | `AlternateRouting.simplifyDist` | Passed through unchanged. |

### Render Stage

| LabyNodeJS field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| stage input path | `gGraphicRendering.inputfile` | `GraphicRendering.inputfile` | Comes from the upstream artifact path. |
| stage output path | `gGraphicRendering.outputfile` | `GraphicRendering.outputfile` | Derived from the stage kind and cache key. |
| `smoothingTension` | `gGraphicRendering.smoothingTension` | `GraphicRendering.smoothing_tension` | JSON uses camelCase. |
| `smoothingIterations` | `gGraphicRendering.smoothingIterations` | `GraphicRendering.smoothing_iterations` | JSON uses camelCase. |
| `penConfig.thickness` | `gGraphicRendering.penConfig.thickness` | `PenStroke.thickness` | Passed through unchanged. |
| `penConfig.antisymmetricAmplitude` | `gGraphicRendering.penConfig.antisymmetricAmplitude` | `PenStroke.antisymmetric_amplitude` | JSON uses camelCase. |
| `penConfig.antisymmetricFreq` | `gGraphicRendering.penConfig.antisymmetricFreq` | `PenStroke.antisymmetric_freq` | JSON uses camelCase. |
| `penConfig.antisymmetricSeed` | `gGraphicRendering.penConfig.antisymmetricSeed` | `PenStroke.antisymmetric_seed` | JSON uses camelCase. |
| `penConfig.symmetricAmplitude` | `gGraphicRendering.penConfig.symmetricAmplitude` | `PenStroke.symmetric_amplitude` | JSON uses camelCase. |
| `penConfig.symmetricFreq` | `gGraphicRendering.penConfig.symmetricFreq` | `PenStroke.symmetric_freq` | JSON uses camelCase. |
| `penConfig.symmetricSeed` | `gGraphicRendering.penConfig.symmetricSeed` | `PenStroke.symmetric_seed` | JSON uses camelCase. |
| `penConfig.resolution` | `gGraphicRendering.penConfig.resolution` | `PenStroke.resolution` | Passed through unchanged. |

## Cache Manifest

The cache lives at `projectDir/cache/cache.json` and stores one entry per deterministic stage key.

Generated SVG artifacts live in `projectDir/svg/`, stage configuration payloads live in `projectDir/configs/`, and execution logs live in `projectDir/logs/`.

Each key includes:

- the stage kind
- the normalized stage payload hash
- the input SVG content hash
- the binary fingerprint of the selected `labypath` executable

Each cache entry stores:

- `inputPath` and `inputHash`
- `outputPath` and `outputHash`
- `configPath` and `logPath`
- the normalized payload and `payloadHash`
- `createdAt`, `updatedAt`, and `status`

## Cache Validity Rules

LabyNodeJS reuses a cached stage result only when all of these are true:

1. the cache key matches the current stage kind, payload, input SVG, and binary fingerprint
2. the output SVG still exists on disk
3. the current output SVG hash still matches the stored `outputHash`

If any of those checks fail, LabyNodeJS rewrites the stage config, reruns `labypath`, and updates the manifest.

## Related Reading

- [configuration-and-workflows.md](configuration-and-workflows.md)
- [pipeline-and-algorithms.md](pipeline-and-algorithms.md)
- [../LabyNodeJS/README.md](../LabyNodeJS/README.md)