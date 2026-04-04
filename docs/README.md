# Technical Documentation

This folder holds the repository-level documentation that is too detailed for the root README.

- [repo-architecture.md](repo-architecture.md): how the C++ engine, Python GUI, web workbench, devcontainer, and Docker image fit together.
- [pipeline-and-algorithms.md](pipeline-and-algorithms.md): stage execution order, major data transformations, and algorithm notes.
- [configuration-and-workflows.md](configuration-and-workflows.md): how protobuf configuration maps to the Python and web tooling.
- [protobuf-to-labystudio-mapping.md](protobuf-to-labystudio-mapping.md): field-by-field mapping between the LabyStudio editor model and emitted protobuf JSON.
- [field-generators-and-noise.md](field-generators-and-noise.md): internal notes on `StreamLine`, `HqNoise`, and experimental field-driven line generation.

Related component-specific documents:

- [../LabyStudio/README.md](../LabyStudio/README.md)
- [../LabyPath/src/flatteningOverlap/README.md](../LabyPath/src/flatteningOverlap/README.md)
- [../LabyPath/src/flatteningOverlap/VISUAL_EXAMPLES.md](../LabyPath/src/flatteningOverlap/VISUAL_EXAMPLES.md)