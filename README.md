# LabyPath

A path/maze generation and rendering system using [CGAL](https://www.cgal.org/) for computational geometry.

LabyPath generates labyrinth-style artwork from SVG shapes using Voronoi diagrams, medial axis computation, polygon offsetting, anisotropic routing, and pen-stroke rendering.

## Features

- **SVG parsing and output** – Import shapes from SVG files and export rendered results.
- **Skeleton grid generation** – Compute medial axes and Voronoi skeletons from polygonal regions.
- **Anisotropic routing** – Place and route paths with configurable cost functions.
- **Pen-stroke rendering** – Emulate hand-drawn line art with configurable pen dynamics.
- **Noise generation** – Poisson disk sampling and HQ noise for natural-looking distributions.

## Dependencies

| Library | Minimum version | Purpose |
|---------|----------------|---------|
| **CGAL** | 5.6+ | Computational geometry (arrangements, Voronoi, polygon ops) |
| **Boost** | 1.74+ | Multi-array, geometry utilities |
| **Protobuf** | 3.21+ | Configuration message serialization |
| **FFTW3** | 3.3+ | FFT for noise generation (optional) |
| **SVG++** | 1.3+ | SVG parsing library |
| **MS GSL** | 4.0+ | Microsoft Guidelines Support Library |
| **GTest** | 1.14+ | Unit testing framework |
| **CMake** | 3.20+ | Build system |

## Building

### Prerequisites (Ubuntu 24.04)

```bash
sudo apt-get install -y \
    g++-14 cmake ninja-build \
    libcgal-dev libgmp-dev libmpfr-dev \
    libboost-all-dev \
    libprotobuf-dev protobuf-compiler \
    libfftw3-dev \
    libsvgpp-dev libmsgsl-dev \
    libgtest-dev
```

### Build with CMake

```bash
cd LabyPath
cmake -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_CXX_COMPILER=g++-14
cmake --build build --parallel $(nproc)
```

### Run tests

```bash
cd LabyPath/build
ctest --output-on-failure
```

### Build options

| Option | Default | Description |
|--------|---------|-------------|
| `LABYPATH_BUILD_TESTS` | `ON` | Build unit tests |
| `LABYPATH_WERROR` | `OFF` | Treat compiler warnings as errors |
| `LABYPATH_ENABLE_PROFILER` | `OFF` | Enable easy_profiler integration |

## Docker

Build and run using the provided Dockerfile (Ubuntu 24.04 + GCC 14):

```bash
docker build -t labypath .
docker run --rm labypath <config.json>
```

## Usage

LabyPath reads a JSON configuration file (see `config.json` for an example):

```bash
./build/labypath config.json
```

The configuration is defined by Protocol Buffer messages in `API/AllConfig.proto`.

## Project structure

```
LabyPath/
├── CMakeLists.txt          # CMake build configuration
├── .clang-tidy             # Linter configuration
├── API/                    # Protobuf definitions
├── include/                # Legacy bundled headers (svgpp, rapidxml, gsl, CGAL)
│                           # Note: system packages are used instead at build time
├── src/
│   ├── Main.cpp            # Entry point
│   ├── MessageIO.*         # JSON config parsing via Protobuf
│   ├── GeomData.*          # CGAL type definitions
│   ├── SkeletonGrid.*      # Skeleton grid generation
│   ├── VoronoiMedialSkeleton.*  # Voronoi/medial axis
│   ├── Anisotrop/          # Anisotropic routing
│   ├── AlternaRoute/       # Alternate routing algorithms
│   ├── Rendering/          # Pen-stroke rendering
│   ├── SVGParser/          # SVG input parsing
│   ├── SVGWriter/          # SVG output generation
│   ├── basic/              # Utility classes
│   ├── generator/          # Noise and point generators
│   ├── flatteningOverlap/  # Path flattening
│   ├── agg/                # Anti-aliased graphics primitives
│   ├── fft/                # FFT utilities
│   └── protoc/             # Generated Protobuf code
├── tests/                  # Google Test unit tests
└── config.json             # Example configuration
```

## Code style

- **C++17** standard
- Strict compiler warnings (`-Wall -Wextra -Wpedantic` and more)
- Static analysis via [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) (see `.clang-tidy`)
- All project code lives in the `laby` namespace

## License

See individual source file headers for authorship information.
