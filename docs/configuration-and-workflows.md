# Configuration And Workflows

## Canonical Schema

The canonical job schema is defined in [../LabyPath/API/AllConfig.proto](../LabyPath/API/AllConfig.proto). The top-level message contains three major sections:

- `routing`
- `gGraphicRendering`
- `skeletonGrid`

Inside `routing`, the notable nested sections are:

- `placement`
- `alternateRouting`

The root executable reads a protobuf-shaped JSON file and then dispatches whichever sections are present.

## LabyNodeJS Crosswalk

LabyNodeJS does not generate TypeScript from the protobuf schema. Instead, it keeps hand-written stage types and payload builders in [../LabyNodeJS/src/types.ts](../LabyNodeJS/src/types.ts), [../LabyNodeJS/src/stages.ts](../LabyNodeJS/src/stages.ts), and [../LabyNodeJS/src/payload.ts](../LabyNodeJS/src/payload.ts), then executes them through [../LabyNodeJS/src/runner.ts](../LabyNodeJS/src/runner.ts).

That arrangement stays close to the protobuf contract while still allowing small TypeScript files to compose experiments directly.

### Important Mapping Details

- `buildGridConfigPayload`, `buildRouteConfigPayload`, and `buildRenderConfigPayload` each produce only the stage-specific subset needed for the current job.
- LabyNodeJS emits protobuf JSON names, which means camelCase keys in JSON even when the `.proto` field is written in snake_case.
- `maxRoutingAttempt` is serialized as a string in the generated payload, matching protobuf JSON expectations for the `uint64` field.
- Route-stage configs now mirror the protobuf contract directly: `placement` and `alternateRouting` are sibling optional sections and are emitted independently.
- `gGraphicRendering.penConfig` later becomes the internal `HqNoise` configuration used by `PenStroke`.

These details are easy to miss if someone reads only the protobuf or only the TypeScript orchestration layer.

A field-by-field mapping table and cache description are available in [labynodejs-config-and-cache.md](labynodejs-config-and-cache.md).

## LabyNodeJS Execution Flow

The execution flow in [../LabyNodeJS/src/runner.ts](../LabyNodeJS/src/runner.ts) is roughly:

1. resolve the workspace root, project directory, and candidate `labypath` binary
2. materialize stage-specific config payloads from the TypeScript stage descriptors
3. hash the effective payload, input SVG, and binary to derive a deterministic cache key
4. check `cache/cache.json` inside the selected project data folder for a matching completed output whose hash still matches the file on disk
5. on a miss, write stage config and log files, invoke `labypath`, and persist the new output metadata

LabyNodeJS now always emits orchestration logs during execution. Every `runPipeline` invocation writes human-readable progress lines to the console and a run-scoped log file in the selected project `logs/` folder. Those run logs record stage ordering, cache-hit or cache-miss reasons, execution durations, and final success or failure summaries.

Per-stage native logs still exist separately from the orchestration log. The run log explains what the TypeScript runner decided to do. The stage log captures the raw `labypath` process output for the specific config file being executed.

Gallery configuration is also explicit now. The caller must provide the full gallery configuration object to `runPipeline` rather than relying on default gallery values.

This keeps the library surface small while still giving scripts precise control over parameters and intermediate outputs.

## LabyPython Workflow

The desktop workflow in [../LabyPython/src/LabyPython/App.py](../LabyPython/src/LabyPython/App.py) follows the same architectural principle:

1. manage project files and user selections
2. locate an appropriate `labypath` executable
3. launch the CLI with the prepared config
4. surface logs and generated outputs back to the user

The Python and TypeScript tools therefore share the same engine but provide different user experiences and integration surfaces.

## Documentation Guidance

When updating config-related docs, prefer these rules:

- treat `AllConfig.proto` as the source of truth for the engine contract
- document LabyNodeJS and LabyPython as orchestration layers around that contract
- call out TypeScript-side convenience fields when they do not map one-to-one to protobuf
- keep stage ordering aligned with the actual dispatch in `MessageIO.cpp`

## Related Reading

- [repo-architecture.md](repo-architecture.md)
- [pipeline-and-algorithms.md](pipeline-and-algorithms.md)
- [labynodejs-config-and-cache.md](labynodejs-config-and-cache.md)
- [field-generators-and-noise.md](field-generators-and-noise.md)
- [../LabyNodeJS/README.md](../LabyNodeJS/README.md)
