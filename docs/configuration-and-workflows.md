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

## LabyStudio Crosswalk

LabyStudio does not generate TypeScript from the protobuf schema. Instead, it maintains a parallel application model in [../LabyStudio/packages/shared/src/index.ts](../LabyStudio/packages/shared/src/index.ts) and the backend job runner in [../LabyStudio/apps/server/src/lib/jobs.ts](../LabyStudio/apps/server/src/lib/jobs.ts).

That arrangement is workable, but it creates a few mapping details that the docs should make explicit.

### Important Mapping Details

- `buildGridConfigPayload`, `buildRouteConfigPayload`, and `buildRenderConfigPayload` each produce only the stage-specific subset needed for the current job.
- LabyStudio emits protobuf JSON names, which means camelCase keys in JSON even when the `.proto` field is written in snake_case.
- `maxRoutingAttempt` is serialized as a string in the generated payload, matching protobuf JSON expectations for the `uint64` field.
- `enableAlternateRouting` exists in the editor-side model and controls whether the `alternateRouting` message is emitted at all.
- `gGraphicRendering.penConfig` later becomes the internal `HqNoise` configuration used by `PenStroke`.

These details are easy to miss if someone reads only the protobuf or only the web editor code.

A field-by-field mapping table is available in [protobuf-to-labystudio-mapping.md](protobuf-to-labystudio-mapping.md).

## LabyStudio Job Flow

The backend flow in [../LabyStudio/apps/server/src/lib/jobs.ts](../LabyStudio/apps/server/src/lib/jobs.ts) is roughly:

1. read the workflow state from the web application
2. materialize stage-specific config payloads
3. write config and log files in the job workspace
4. invoke `labypath`
5. collect status and outputs for the frontend

This keeps the browser application thin. The server owns filesystem access and process execution.

## LabyPython Workflow

The desktop workflow in [../LabyPython/src/LabyPython/App.py](../LabyPython/src/LabyPython/App.py) follows the same architectural principle:

1. manage project files and user selections
2. locate an appropriate `labypath` executable
3. launch the CLI with the prepared config
4. surface logs and generated outputs back to the user

The Python and web tools therefore share the same engine but provide different user experiences and integration surfaces.

## Documentation Guidance

When updating config-related docs, prefer these rules:

- treat `AllConfig.proto` as the source of truth for the engine contract
- document LabyStudio and LabyPython as orchestration layers around that contract
- call out editor-side convenience fields when they do not map one-to-one to protobuf
- keep stage ordering aligned with the actual dispatch in `MessageIO.cpp`

## Related Reading

- [repo-architecture.md](repo-architecture.md)
- [pipeline-and-algorithms.md](pipeline-and-algorithms.md)
- [protobuf-to-labystudio-mapping.md](protobuf-to-labystudio-mapping.md)
- [field-generators-and-noise.md](field-generators-and-noise.md)
- [../LabyStudio/README.md](../LabyStudio/README.md)