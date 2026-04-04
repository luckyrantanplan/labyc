# Repository Architecture

## Top-Level Structure

The repository is a workspace that groups three executables around the same routing engine:

- `LabyPath`: the C++ command-line processor.
- `LabyPython`: the desktop GUI that manages projects and invokes `labypath`.
- `LabyStudio`: the browser-based editor whose backend also invokes `labypath`.

The root [CMakeLists.txt](../CMakeLists.txt) is intentionally small. It exists to make the workspace convenient in editors and CI, then delegates to `LabyPath` with `add_subdirectory(LabyPath)`.

## Runtime Responsibilities

### LabyPath

`LabyPath` owns the canonical processing pipeline and the canonical protobuf contract in [../LabyPath/API/AllConfig.proto](../LabyPath/API/AllConfig.proto). The executable reads protobuf-shaped JSON, dispatches the requested processing sections, and produces intermediate or rendered outputs.

### LabyPython

The Python application in [../LabyPython/src/LabyPython/App.py](../LabyPython/src/LabyPython/App.py) is a desktop orchestrator. It creates project files, prepares arguments, searches for the `labypath` binary in common build locations, launches jobs, and manages result files and logs.

### LabyStudio

LabyStudio is split into three layers:

- `apps/web`: the React frontend that edits workflow nodes and parameters.
- `apps/server`: the backend that serializes job inputs, invokes `labypath`, and streams job state.
- `packages/shared`: shared schemas and helpers used by both sides.

The important architectural point is that the web stack does not embed the C++ engine. It shells out to the CLI just like the desktop tool does.

## Build Layouts

Two build layouts coexist:

### Workspace Wrapper Build

- source directory: repository root
- build directory: `.cmake/build`
- used by: root VS Code settings and workspace tasks

This path treats the whole repository as the active workspace and keeps generated files out of `LabyPath/`.

### Direct LabyPath Build

- source directory: `LabyPath`
- build directory: `LabyPath/build`
- used by: devcontainer CMake Tools settings

This path is convenient when focusing only on the engine.

The documentation should describe both, because both are present in the checked-in configuration.

## Containers

### Devcontainer

The devcontainer is the full-featured development image. It installs compilers, Python, Qt-related dependencies, and Node.js 24 for LabyStudio development.

### Production Docker Image

The root [../Dockerfile](../Dockerfile) is currently aligned with CLI execution, not with the full developer workstation. It builds `labypath`, runs the C++ test suite during the build stage, prepares a lightweight Python environment, and copies the resulting runtime artifacts into the final image.

It does not install:

- Node.js
- the LabyStudio application
- the PyQt desktop runtime

That split is deliberate today, even if it may evolve later.

## Dependency Graph

```mermaid
graph TD
    Root[Workspace root] --> CMake[Root CMake wrapper]
    CMake --> Engine[LabyPath]

    StudioShared["@labystudio/shared"] --> StudioWeb[apps/web]
    StudioShared --> StudioServer[apps/server]
    StudioServer --> EngineCLI["labypath executable"]

    PythonGUI[LabyPython] --> EngineCLI
    Devcontainer[.devcontainer] --> Engine
    Devcontainer --> PythonGUI
    Devcontainer --> StudioWeb
    Devcontainer --> StudioServer
    ProdDocker[Dockerfile] --> EngineCLI
```

This graph is intentionally about module/runtime dependencies, not about every static library inside `LabyPath`.

## LabyPath Internal Subsystems

The higher-level graph above shows runtime boundaries. Inside `LabyPath`, the main subsystem relationships are tighter than that simplified picture suggests.

```mermaid
graph LR
    Dispatch["CLI / JSON-Proto Dispatch"]
    Input["SVG Input & Shape Import"]
    Geometry["Geometry & Arrangement Core"]
    Skeleton["Skeleton / Grid Construction"]
    Placement["Placement Routing"]
    Alternate["Alternate Routing"]
    Flatten["Overlap Flattening & Ribbon Assembly"]
    Render["Graphic Rendering & SVG Output"]

    Dispatch --> Skeleton
    Dispatch --> Placement
    Dispatch --> Alternate
    Dispatch --> Render

    Input --> Skeleton
    Input --> Placement
    Input --> Alternate
    Input --> Render

    Geometry --> Skeleton
    Geometry --> Placement
    Geometry --> Alternate
    Geometry --> Flatten
    Geometry --> Render

    Skeleton --> Placement
    Skeleton --> Alternate
    Skeleton --> Render
    Placement --> Flatten
    Alternate --> Flatten
    Alternate --> Placement
    Flatten --> Render
```

The `Alternate --> Placement` edge is a code-reuse edge rather than a pipeline edge: `AlternateRoute` reuses `aniso::Routing::connectMaze()` after it constructs its own trapezoid-derived strips.