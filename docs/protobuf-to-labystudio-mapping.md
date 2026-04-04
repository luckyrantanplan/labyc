# Protobuf To LabyStudio Mapping

This page documents how the LabyStudio editor model maps to the protobuf-backed JSON files written by the backend before it launches `labypath`.

The authoritative sources are:

- [../LabyPath/API/AllConfig.proto](../LabyPath/API/AllConfig.proto)
- [../LabyStudio/packages/shared/src/index.ts](../LabyStudio/packages/shared/src/index.ts)
- [../LabyStudio/apps/server/src/lib/jobs.ts](../LabyStudio/apps/server/src/lib/jobs.ts)

## Mapping Rules

- `AllConfig.proto` is the source of truth for the engine contract.
- LabyStudio emits protobuf JSON field names, which use camelCase rather than the snake_case field names shown in the `.proto` file.
- Each stage builder emits only the message fragment needed for that stage.
- Some editor fields are convenience fields only. They shape whether a payload section is emitted, but they are not themselves protobuf fields.

## Execution Flow

The backend resolves numeric helper nodes, applies the resolved values to the selected stage config, writes a stage-specific JSON file, and then launches `labypath` on that file.

The relevant functions are:

- `buildGridConfigPayload()`
- `buildRouteConfigPayload()`
- `buildRenderConfigPayload()`

## Grid Stage

| LabyStudio field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| stage input path | `skeletonGrid.inputfile` | `SkeletonGrid.inputfile` | Comes from the upstream artifact path. |
| stage output path | `skeletonGrid.outputfile` | `SkeletonGrid.outputfile` | Derived from the job stage stem. |
| `simplificationOfOriginalSVG` | `skeletonGrid.simplificationOfOriginalSVG` | `SkeletonGrid.simplificationOfOriginalSVG` | Same name in editor and protobuf JSON. |
| `maxSep` | `skeletonGrid.maxSep` | `SkeletonGrid.max_sep` | JSON uses protobuf camelCase. |
| `minSep` | `skeletonGrid.minSep` | `SkeletonGrid.min_sep` | JSON uses protobuf camelCase. |
| `seed` | `skeletonGrid.seed` | `SkeletonGrid.seed` | Integer value passed through unchanged. |

## Route Stage

### Shared File Paths

| LabyStudio field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| stage input path | `routing.filepaths.inputfile` | `Routing.filepaths.inputfile` | Comes from the upstream artifact path. |
| stage output path | `routing.filepaths.outputfile` | `Routing.filepaths.outputfile` | Derived from the job stage stem. |

### Placement Fields

| LabyStudio field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| `initialThickness` | `routing.placement.initialThickness` | `Placement.initial_thickness` | JSON uses camelCase. |
| `decrementFactor` | `routing.placement.decrementFactor` | `Placement.decrement_factor` | JSON uses camelCase. |
| `minimalThickness` | `routing.placement.minimalThickness` | `Placement.minimal_thickness` | JSON uses camelCase. |
| `smoothingTension` | `routing.placement.smoothingTension` | `Placement.smoothing_tension` | JSON uses camelCase. |
| `smoothingIteration` | `routing.placement.smoothingIteration` | `Placement.smoothing_iteration` | JSON uses camelCase. |
| `maxRoutingAttempt` | `routing.placement.maxRoutingAttempt` | `Placement.max_routing_attempt` | Serialized as a string to match protobuf JSON handling for `uint64`. |
| `routing.seed` | `routing.placement.routing.seed` | `RoutingCost.seed` | Passed through unchanged. |
| `routing.maxRandom` | `routing.placement.routing.maxRandom` | `RoutingCost.max_random` | JSON uses camelCase. |
| `routing.distanceUnitCost` | `routing.placement.routing.distanceUnitCost` | `RoutingCost.distance_unit_cost` | JSON uses camelCase. |
| `routing.viaUnitCost` | `routing.placement.routing.viaUnitCost` | `RoutingCost.via_unit_cost` | JSON uses camelCase. |
| `cell.seed` | `routing.placement.cell.seed` | `Cell.seed` | Passed through unchanged. |
| `cell.maxPin` | `routing.placement.cell.maxPin` | `Cell.maxPin` | Same name in editor and protobuf JSON. |
| `cell.startNet` | `routing.placement.cell.startNet` | `Cell.startNet` | Same name in editor and protobuf JSON. |
| `cell.resolution` | `routing.placement.cell.resolution` | `Cell.resolution` | Passed through unchanged. |

### Alternate Routing Fields

| LabyStudio field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| `enableAlternateRouting` | not emitted directly | none | Editor-only gate that controls whether `alternateRouting` is included. |
| `alternateRouting.maxThickness` | `routing.alternateRouting.maxThickness` | `AlternateRouting.maxThickness` | Emitted only when enabled. |
| `alternateRouting.minThickness` | `routing.alternateRouting.minThickness` | `AlternateRouting.minThickness` | Emitted only when enabled. |
| `alternateRouting.pruning` | `routing.alternateRouting.pruning` | `AlternateRouting.pruning` | Passed through unchanged. |
| `alternateRouting.thicknessPercent` | `routing.alternateRouting.thicknessPercent` | `AlternateRouting.thicknessPercent` | Passed through unchanged. |
| `alternateRouting.simplifyDist` | `routing.alternateRouting.simplifyDist` | `AlternateRouting.simplifyDist` | Passed through unchanged. |

## Render Stage

| LabyStudio field | Emitted JSON key | Protobuf field | Notes |
| --- | --- | --- | --- |
| stage input path | `gGraphicRendering.inputfile` | `GraphicRendering.inputfile` | Comes from the upstream artifact path. |
| stage output path | `gGraphicRendering.outputfile` | `GraphicRendering.outputfile` | Derived from the job stage stem. |
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

## Editor-Only Concepts

These concepts are part of the LabyStudio UX but are not protobuf fields:

- graph node IDs and labels
- numeric helper nodes such as constants, operations, and broadcasts
- `enableAlternateRouting`
- artifact bookkeeping like `configPath`, `logPath`, and execution status

They still affect the final job because the backend resolves numeric helper nodes before it calls the stage payload builders.

## Related Reading

- [configuration-and-workflows.md](configuration-and-workflows.md)
- [pipeline-and-algorithms.md](pipeline-and-algorithms.md)
- [../LabyStudio/README.md](../LabyStudio/README.md)