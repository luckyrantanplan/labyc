# Field Generators And Noise

This page documents the lower-level generator utilities that sit beside the main `SkeletonGrid`, `Placement`, `AlternateRouting`, and `GraphicRendering` stages.

The important distinction is:

- `StreamLine` can generate grid-like ribbons or vector-field-driven linework.
- `HqNoise` generates continuous noise fields.
- In the current production pipeline, `StreamLine` is mostly an internal or study utility and `HqNoise` is mainly used by `PenStroke` during rendering.

## `StreamLine`

`generator::StreamLine` exposes two distinct construction patterns, and the meaning of its
configuration depends on which one you use.

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

Relevant `Config` parameters in this mode:

| Parameter | Meaning | Used by CGAL? | Notes |
| --- | --- | --- | --- |
| `resolution` | samples per world unit for the internally allocated `Regular_grid_2` | indirect | Controls the grid dimensions together with `size`. |
| `size` | world-space width and height of the square field | indirect | Controls the grid dimensions together with `resolution`. |
| `divisor` | target separating distance between streamlines in world units | yes | Converted to `dSep = resolution * divisor` in grid-sample coordinates. |
| `dRat` | saturation ratio for the farthest-point seeding strategy | yes | Passed as CGAL `saturation_ratio`. |
| `simplify_distance` | polyline decimation tolerance after placement | no | Post-process only. |
| `epsilon` | small fallback vector magnitude used by `drawSpiral()` outside the selected radius | no | Only affects spiral-field authoring, not CGAL placement itself. |
| `sample_scale` | world units per sample | no | Not normally set in this mode; `resolution` is the primary density control. |
| `old_RegularGrid` | enables the sampled-field workflow | no | Internal mode switch. |

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

Relevant `Config` parameters in this mode:

| Parameter | Meaning | Used by CGAL? | Notes |
| --- | --- | --- | --- |
| `resolution` | coordinate scaling applied while generating point/vector constraints | indirect | Passed into `VectorCompute`, then used to scale sample locations before building `Triangular_field_2`. |
| `divisor` | target separating distance | yes | Converted to `dSep = resolution * divisor`. |
| `dRat` | saturation ratio | yes | Passed as CGAL `saturation_ratio`. |
| `simplify_distance` | post-process decimation tolerance | no | Applied after CGAL returns streamlines. |
| `epsilon`, `size`, `sample_scale`, `old_RegularGrid` | not relevant | no | Not part of the triangular-field placement path. |

### 3. Field-Driven Pipeline Stage

The protobuf and LabyNodeJS pipeline now expose a third, field-driven workflow:

1. `HqNoise` writes a `ComplexField2DData` artifact.
2. `StreamLineCfg` loads that artifact.
3. `MessageIO` constructs a `Regular_grid_2` from the stored field values and traces streamlines.

This is the mode used by the `noise(...) -> streamline(...)` Node pipeline.

The field artifact already contains:

- `width`
- `height`
- `scale`
- `originX`
- `originY`

So several legacy `StreamLineCfg` fields are redundant in this mode and are now treated as derived values rather than primary inputs.

| Protobuf Field | Meaning in the field-driven stage | Status |
| --- | --- | --- |
| `filepaths.inputfile` | input `.field` protobuf artifact | required |
| `filepaths.outputfile` | output SVG path | required |
| `simplifyDistance` | post-process streamline simplification tolerance | user-controlled |
| `dRat` | CGAL `saturation_ratio` | user-controlled |
| `divisor` | streamline separating distance in world units | user-controlled |
| `strokeThickness` | final SVG stroke width | user-controlled |
| `resolution` | derived as `round(1.0 / field.scale)` when omitted or non-positive | derived |
| `size` | derived as `max(field.width, field.height) * field.scale` when omitted or non-positive | derived |
| `epsilon` | ignored by this stage | legacy/unused |

For the LabyNodeJS public API, the streamline stage therefore only exposes:

- `simplifyDistance`
- `dRat`
- `divisor`
- `strokeThickness`

That is the smallest parameter set that still maps cleanly to CGAL placement plus SVG output in the field-driven workflow.

## Using `StreamLine` Results

`StreamLine` returns `Ribbon` objects, not a `SkeletonGrid` output. Typical next steps in C++ are:

- add the ribbons to an arrangement with `addToArrangement()`
- feed those ribbons into your own geometry or rendering experiments
- serialize them with the same SVG output helpers used elsewhere in the engine

Today the field-driven workflow is exposed as a first-class protobuf and LabyNodeJS stage. The legacy spiral and polygon-derived constructors remain C++ utilities.

## `HqNoise`

`HqNoise1D` and `HqNoise2D` synthesize continuous noise fields in Fourier space and then expose sampled accessors in the spatial domain.

### Core `HqNoiseConfig` parameters

| Parameter | Type | Meaning |
| --- | --- | --- |
| `maxN` | `uint32` | Support radius of the noise field in world units. The sampling domain `(width-1)*scale` and `(height-1)*scale` must both be ≤ `maxN`. Also controls the internal FFT lattice size as `maxN * accuracy`. |
| `accuracy` | `uint32` | Oversampling factor for the internal FFT lattice (`lattice = maxN * accuracy + 2`). Higher values increase detail but cost more memory and CPU. |
| `amplitude` | `double` | Target peak-to-peak amplitude after normalization. The output is rescaled so the absolute range is `2 * amplitude`. |
| `seed` | `double` | Deterministic seed for the random number generator that fills the pre-FFT lattice. |
| `gaussian.frequency` | `double` | Standard deviation of the Gaussian low-pass envelope applied in the frequency domain. Larger values retain more high-frequency energy. |
| `powerlaw.frequency` | `double` | Roll-off knee of the power-law high-pass filter. Frequencies below this are attenuated less. |
| `powerlaw.power` | `double` | Exponent of the power-law spectral falloff. Higher values produce smoother, more correlated fields. |
| `complex` | `bool` | When `true`, both real and imaginary parts of the FFT lattice are filled and independently normalized, producing a complex-valued 2D field suitable for use as a vector field. |

### Sampling / output parameters (stage-level, not in `HqNoiseConfig`)

When consuming `HqNoise` through the protobuf pipeline stage or LabyNodeJS `noise(...)`, two additional parameters control the exported field grid:

| Parameter | Type | Meaning | Constraint |
| --- | --- | --- | --- |
| `width` | `uint32` | Number of sample columns in the exported `ComplexField2DData`. | `(width - 1) * scale <= maxN` |
| `height` | `uint32` | Number of sample rows in the exported `ComplexField2DData`. | `(height - 1) * scale <= maxN` |
| `scale` | `double` | World-unit spacing between adjacent samples. Together with `width`/`height` determines the sampled extent. | must be > 0 |
| `previewMode` | enum | `ARROWS` renders normalized direction arrows; `MAGNITUDE` renders a heatmap; `NONE` skips preview. | — |
| `previewStride` | `uint32` | Only draw every `n`-th sample in the preview SVG. Defaults to `max(width, height) / 32`. | — |

The relationship between all the parameters:

```
FFT lattice size = maxN * accuracy + 2
sampled extent X = (width - 1) * scale  ← must be ≤ maxN
sampled extent Y = (height - 1) * scale ← must be ≤ maxN
```

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

- If your goal is to generate field-driven linework from a stored complex field, use the `noise -> streamline` pipeline.
- If your goal is to generate experimental grid-like line fields directly in C++, start from `StreamLine`.
- If your goal is to make rendered strokes look less mechanical, start from `HqNoise` via `PenStroke`.
- If your goal is to make either of those workflows available from the CLI or LabyNodeJS, the current codebase needs new schema and dispatch wiring rather than just new docs.

## Related Reading

- [pipeline-and-algorithms.md](pipeline-and-algorithms.md)
- [configuration-and-workflows.md](configuration-and-workflows.md)
- [../LabyPath/src/studies/PathRenderingTest.cpp](../LabyPath/src/studies/PathRenderingTest.cpp)
- [../LabyPath/src/studies/grayPen.cpp](../LabyPath/src/studies/grayPen.cpp)