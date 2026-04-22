# Field Generators And Noise

This page documents the lower-level generator utilities that sit beside the main `SkeletonGrid`, `Placement`, `AlternateRouting`, and `GraphicRendering` stages.

The important distinction is:

- `StreamLine` can generate grid-like ribbons or vector-field-driven linework.
- `HqNoise` generates continuous noise fields.
- In the current production pipeline, `StreamLine` is mostly an internal or study utility and `HqNoise` is mainly used by `PenStroke` during rendering.

## `StreamLine`

`generator::StreamLine` exposes two usable construction patterns.

### 1. Regular-Grid Spiral Field

This is the legacy sampled-field mode. Enable `Config.old_RegularGrid`, allocate a square field, inject one or more spiral vector sources with `drawSpiral()`, then call `render()` to trace stream lines.

```cpp
generator::StreamLine::Config config;
config.old_RegularGrid = true;
config.size = 320;
config.resolution = 4;
config.divisor = 2;

generator::StreamLine streamLine(config);
streamLine.drawSpiral({{80.0, 80.0}, 100.0, M_PI / 40.0});
streamLine.drawSpiral({{40.0, 40.0}, 32.0, -M_PI / 4.0});
streamLine.render();

const Ribbon& radial = streamLine.radialList();
const Ribbon& circular = streamLine.circularList();
```

What happens internally:

- the constructor allocates `_field` as a complex-valued sampled vector field
- `drawSpiral()` writes direction vectors into that field, rotating them outside the selected radius
- `render()` converts the stored vectors into two `CGAL::Regular_grid_2` fields: radial and circular
- `CGAL::Stream_lines_2` traces streamlines through each field
- `postStreamCompute()` decimates and converts the result into `Ribbon` polylines

The closest direct example is [../LabyPath/src/studies/PathRenderingTest.cpp](../LabyPath/src/studies/PathRenderingTest.cpp).

### 2. Polygon-Derived Triangular Field

This is the more geometry-driven path. Instead of drawing spirals into a sampled field, you derive local direction constraints from polygon edges.

```cpp
generator::StreamLine::Config config;
config.resolution = 4;
config.divisor = 2;
config.simplify_distance = 0.1;
config.dRat = 1.0;

Ribbon radial = generator::StreamLine::getRadial(config, polygons);
Ribbon longitudinal = generator::StreamLine::getLongitudinal(config, polygons);
```

What happens internally:

- `getRadial()` samples polygon boundary segments and adds perpendicular vectors
- `getLongitudinal()` samples polygon boundary segments and adds tangential vectors
- `generateTriangularField()` constructs `CGAL::Triangular_field_2` from those point/vector constraints
- `streamPlacement()` traces stream lines through that interpolated vector field
- `postStreamCompute()` simplifies the resulting polylines and reconnects nearby endpoints

This mode is the clearest path if you want to create grid-like ribbons from polygon geometry rather than from hand-placed spirals.

## Using `StreamLine` Results

`StreamLine` returns `Ribbon` objects, not a ready-made `SkeletonGrid` protobuf stage output. Typical next steps in C++ are:

- add the ribbons to an arrangement with `addToArrangement()`
- feed those ribbons into your own geometry or rendering experiments
- serialize them with the same SVG output helpers used elsewhere in the engine

Today there is no top-level protobuf message or CLI stage that exposes `StreamLine` directly. If you want this to become a first-class workflow, the missing integration points are:

1. define a new protobuf message for the field-generator stage
2. dispatch it from `MessageIO.cpp`
3. decide whether the output should be a ribbon SVG, a grid-coded SVG, or another intermediate file format
4. expose the new stage in LabyNodeJS and, if needed, in LabyPython

## `HqNoise`

`HqNoise1D` and `HqNoise2D` synthesize continuous noise fields in Fourier space and then expose sampled accessors in the spatial domain.

The main configurable inputs are:

- `maxN`: canvas size or domain extent
- `accuracy`: sampling scale
- `amplitude`: output amplitude
- `seed`: deterministic random seed
- `powerlaw.frequency` and `powerlaw.power`: spectral falloff controls
- `gaussian.frequency`: high-frequency damping
- `complex`: whether to synthesize a complex-valued field

Example setup:

```cpp
generator::HqNoiseConfig config;
config.maxN = 800;
config.seed = 4;
config.amplitude = 40.0;
config.accuracy = 1;
config.gaussian.frequency = 800;
config.powerlaw.frequency = 50;
config.powerlaw.power = 2;
config.complex = false;

generator::HqNoise2D noise(config);
double sample = noise.get(120.0, 64.0);
```

Internally, `HqNoise2D`:

- fills the spatial array with random samples
- executes a forward FFT
- attenuates frequencies with the configured power-law and Gaussian terms
- executes the inverse FFT
- normalizes the result to the requested amplitude
- serves samples through bilinear interpolation in `get()`

The study-style usage example is [../LabyPath/src/studies/grayPen.cpp](../LabyPath/src/studies/grayPen.cpp).

## What `HqNoise` Is Used For Today

In the current production code, `HqNoise` is not used to build routing grids. It is wired into `PenStroke`:

- one noise field modulates stroke width symmetrically around the centerline
- another noise field adds antisymmetric lateral variation to mimic hand-drawn strokes

Those noise settings are exposed indirectly through `GraphicRendering.penConfig` in the protobuf schema and in LabyNodeJS's render-stage config.

## Practical Guidance

- If your goal is to generate experimental grid-like line fields, start from `StreamLine`.
- If your goal is to make rendered strokes look less mechanical, start from `HqNoise` via `PenStroke`.
- If your goal is to make either of those workflows available from the CLI or LabyNodeJS, the current codebase needs new schema and dispatch wiring rather than just new docs.

## Related Reading

- [pipeline-and-algorithms.md](pipeline-and-algorithms.md)
- [configuration-and-workflows.md](configuration-and-workflows.md)
- [../LabyPath/src/studies/PathRenderingTest.cpp](../LabyPath/src/studies/PathRenderingTest.cpp)
- [../LabyPath/src/studies/grayPen.cpp](../LabyPath/src/studies/grayPen.cpp)