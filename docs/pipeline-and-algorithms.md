# Pipeline And Algorithms

## Dispatch Model

The executable dispatch path is visible in [../LabyPath/src/MessageIO.cpp](../LabyPath/src/MessageIO.cpp). After reading the protobuf-shaped JSON payload, it conditionally runs:

1. `SkeletonGrid`
2. `Routing.Placement`
3. `Routing.AlternateRouting`
4. `GraphicRendering`

Those are independent checks against the parsed `AllConfig` message, so the routing portion is not one indivisible stage. The configuration can enable either routing pass or both.

## Stage 1: Skeleton/Grid Preparation

The first family of algorithms constructs the search substrate from the input geometry. The relevant source layout includes files such as:

- `SkeletonGrid.cpp`
- `SkeletonOffset.cpp`
- `SkeletonRadial.cpp`
- `GridIndex.cpp`

From those names and the surrounding code structure, the stage is better described as geometric preprocessing plus grid construction than as a single classical algorithm. It combines operations such as:

- reading and normalizing geometric input
- deriving offset or radial information from the shape
- indexing the generated structure for later routing queries

The practical outcome is a navigable representation that later routing stages can score and traverse.

## Stage 2: Placement Routing

Placement routing is the main path-search step. The old README description was too confident if it framed this simply as A*.

The implementation is an incremental pin-to-pin maze-growth pass over arrangement vertices rather than a single textbook shortest-path call. The main flow in `aniso::Placement` is:

- reload the grid-coded SVG and rebuild one arrangement per `GridIndex`
- derive a `Cell` from the grid boundary and limit ribbons
- seed nets from snapped boundary points, subdivided segments, and existing high-degree arrangement vertices
- route each net with `Routing::findRoute()` and commit the winning parent chain into `PolyConvex` tiles
- refine the committed path with simplification and Chaikin smoothing before reconnecting the maze and rendering the overlap-aware output

`Routing::findRoute()` itself is best described as weighted best-first graph exploration. It uses a pairing heap over arrangement vertices and relaxes candidates with a lexicographic cost that combines:

- congestion accumulated from existing routed geometry
- via count or direction-change penalties
- geometric distance and junction-degree penalties
- a random tie-breaker to avoid overly deterministic local choices

`SpatialIndex` then adds or blocks costs for candidate segments that intersect or crowd existing paths and pins. That is why the algorithm is better described as constrained, cost-driven search on a prepared geometric structure than as a narrow A* implementation.

## Stage 3: Alternate Routing

The protobuf schema places `AlternateRouting` next to `Placement` inside `Routing`, but it is a separate optional pass. In [../LabyPath/src/MessageIO.cpp](../LabyPath/src/MessageIO.cpp), the executable checks them independently.

The implementation is also fundamentally different from placement routing. `AlternateRoute` does not call `Routing::findRoute()` or run a queue-based search. Instead it:

- reloads the same grid-coded SVG input and reconstructs arrangements from `GridIndex`
- prunes the arrangement and removes antenna edges
- builds a direction-specific Voronoi or medial-skeleton arrangement for the circular and radial ribbons separately
- samples left and right offset pairs around eligible halfedges and clamps them between `minThickness` and `maxThickness`
- combines those offsets into origin and offset triplets
- converts consecutive triplets into trapezoid segments, inserts them into an arrangement, and turns that arrangement into `PolyConvex` strips
- reconnects neighboring strips at vertices, reuses `aniso::Routing::connectMaze()` for maze connectivity, then hands the result to the overlap renderer

So the alternate route stage is a geometry-derived strip-construction algorithm built from Voronoi structure, offsets, and trapezoids. It is only a follow-up pass in dispatch order. It does not consume placement's in-memory search state, and if both passes target the same output path the later one can overwrite the earlier file.

## Stage 4: Graphic Rendering

The rendering side is represented by types such as:

- `Ribbon`
- `OrientedRibbon`
- `Polyline`
- `PolyConvex`

There are two rendering flows that should be documented separately.

### Overlap-Aware Routing Output

Placement and alternate routing both finish by sending `PolyConvex` tiles into `PathRendering::pathRender()`. That path:

- detects intersecting or adjacent polygon pairs with box-intersection filtering
- groups them into families and patches
- builds a conflict graph of overlap nodes
- assigns node states with a greedy queue-based coloring pass
- overlays polygon arrangements in `NodeRendering` to recover oriented outer and hole boundaries
- adds the global union boundary and then writes the result with `GraphicRendering::printRibbonSvg()`

This is not just serialization. It is overlap flattening plus boundary extraction plus SVG emission.

### Standalone `gGraphicRendering`

The `gGraphicRendering` protobuf section is a separate file-based renderer. `GraphicRendering(proto::GraphicRendering)` reloads an input ribbon SVG, smooths each polyline, creates a `PenStroke`, rasterizes arrangement faces for the stroke body, and finally draws the outline. `PenStroke` internally builds two `HqNoise2D` fields from `penConfig`: a symmetric field that modulates width and an antisymmetric field that pushes the centerline sideways.

So in this codebase, “rendering” can mean either overlap-aware routed ribbon extraction or the later pen-stroke stylization pass used by the standalone render message.

## Algorithm Notes By Concern

### Geometry Handling

Classes like `GeomData`, `Polyline`, `PolyConvex`, and `Ribbon` show that geometry is represented and transformed explicitly throughout the pipeline rather than being treated as opaque blobs.

### Search Space Construction

`SkeletonGrid`, `SkeletonOffset`, `SkeletonRadial`, and `GridIndex` indicate a multi-step search-space construction process. The routing quality depends on this preparation, so describing routing alone is incomplete.

### Routing

Placement routing should be documented as constrained, cost-driven graph exploration over a prepared arrangement. Alternate routing should be documented as Voronoi and offset based strip generation over the same grid-coded input. Lumping both under one routing label hides an important implementation split.

### Rendering

Rendering is geometry-aware post-processing, not only serialization. It includes overlap flattening, oriented boundary recovery, SVG ribbon output, and a separate pen-stroke stylization pass driven by spectral noise.

## Field Generators And Noise Fields

Some generator code sits beside the canonical CLI stages but is not exposed as a first-class protobuf workflow.

### `StreamLine`

`generator::StreamLine` supports two field-generation modes:

- legacy regular-grid mode: if `Config.old_RegularGrid` is enabled, the constructor allocates a sampled vector field, `drawSpiral()` writes spiral vectors into that field, and `render()` traces radial and circular stream lines with `CGAL::Stream_lines_2`
- polygon-driven triangular-field mode: `getRadial()` and `getLongitudinal()` sample polygon edges into point and vector constraints, build a `CGAL::Triangular_field_2`, and then trace stream lines through that interpolated field

That makes `StreamLine` useful for experimental grid-like ribbon generation, but today it is an internal C++ utility and study path rather than a user-facing JSON stage.

### `HqNoise`

`HqNoise1D` and `HqNoise2D` generate continuous noise fields in Fourier space, then sample them back in the spatial domain. They are currently used by `PenStroke` during standalone render stylization, not by the production `SkeletonGrid` stage or by the routing stages.

If you want to create grid-like linework today, `StreamLine` is the relevant utility. If you want to perturb the final visual stroke, `HqNoise` is the relevant utility.

## Related Reading

- [repo-architecture.md](repo-architecture.md)
- [configuration-and-workflows.md](configuration-and-workflows.md)
- [labynodejs-config-and-cache.md](labynodejs-config-and-cache.md)
- [field-generators-and-noise.md](field-generators-and-noise.md)
- [../LabyPath/src/flatteningOverlap/README.md](../LabyPath/src/flatteningOverlap/README.md)
- [../LabyPath/src/flatteningOverlap/VISUAL_EXAMPLES.md](../LabyPath/src/flatteningOverlap/VISUAL_EXAMPLES.md)