# flatteningOverlap – Visual Examples

This document illustrates the overlap resolution algorithm with geometry
diagrams, showing how overlapping convex polygons are detected, grouped,
colored, and merged into a clean boundary.

## 1. Input: Overlapping Convex Tiles

After routing, adjacent paths share convex polygon tiles that overlap:

```
  Polygon A (path 1)         Polygon B (path 2)
  ┌──────────────┐           ┌──────────────┐
  │              │           │              │
  │      A       │           │      B       │
  │              │           │              │
  └──────────────┘           └──────────────┘

         When paths cross, tiles overlap:

  ┌──────────────┐
  │    A    ┌────┼──────────┐
  │         │ A∩B│    B     │
  │         │    │          │
  └─────────┼────┘          │
            └───────────────┘

  The shaded region A∩B is the intersection zone.
```

## 2. Intersection Detection (Box Intersection)

CGAL's `box_self_intersection_d` efficiently finds overlapping bounding boxes.
Each pair `(i, j)` where tile `i` and tile `j` overlap is recorded:

```
  Bounding boxes:          Overlap pairs:
  ┌─────────┐
  │  bbox_A  │              Intersection(0, 1) → A ∩ B
  │    ┌─────┼───┐          Intersection(1, 2) → B ∩ C
  │    │     │   │          Intersection(0, 2) → A ∩ C
  └────┼─────┘   │
       │  bbox_B  │
       │    ┌─────┼───┐
       │    │     │   │
       └────┼─────┘   │
            │  bbox_C  │
            └──────────┘
```

## 3. Family Grouping (Union-Find)

Intersection pairs that share polygon indices are merged into families:

```
  Intersections:             Union-Find grouping:
  (A, B)  ─┐
            ├── Family 0 (all share polygon B)
  (B, C)  ─┘

  (D, E)  ──── Family 1 (isolated pair)
```

## 4. Patch Creation

Each family splits its polygon indices into connected components (patches):

```
  Family 0: polygons {A, B, C}

  Adjacency graph:       Patches (Union-Find):
    A ── B ── C            Patch 0: {A}  (left side)
                           Patch 1: {B, C}  (right side)

  A family with 2 patches → clean two-sided overlap.
  A family with 1 patch  → all polygons in same component.
```

## 5. Conflict Graph Construction

Each patch becomes a **Node** in the conflict graph:

```
  Node 0                   Node 1
  cover: {A}               cover: {B, C}
  state: -1 (unset)        state: -1 (unset)

  Relationship:
    Node 0 ←opposite→ Node 1
    (must have DIFFERENT rendering states)
```

When multiple families interact:

```
          Node 0            Node 2
          cover: {A}        cover: {D}
             │                 │
         opposite          opposite
             │                 │
          Node 1            Node 3
          cover: {B, C}     cover: {E}
             │
         adjacent ─────── Node 2
         (prefer different states)
```

## 6. Graph Coloring (State Assignment)

The greedy algorithm assigns states (colors) to avoid conflicts:

```
  Priority queue processes nodes with fewest opposites first:

  Step 1: Node 0 (1 opposite) → state = 0
  Step 2: Node 1 (opposite of 0) → state = 1  (0 is taken)
  Step 3: Node 2 (1 opposite, adjacent to 1) → state = 0  (alternating)
  Step 4: Node 3 (opposite of 2) → state = 1  (0 is taken)

  Result:
    Node 0: state=0  ■
    Node 1: state=1  □
    Node 2: state=0  ■
    Node 3: state=1  □
```

This 0/1 alternation is the simplest example, not a hard upper bound. More complex merged groups can require state 2 or higher.

## 7. Arrangement Overlay (Boundary Extraction)

CGAL overlays polygon arrangements of opposite nodes to find boundaries:

```
  Node 0 arrangement:       Node 1 arrangement:

  ┌──────────┐               ┌──────────────┐
  │ polygon  │               │   polygon    │
  │    A     │               │    B    C    │
  └──────────┘               └──────────────┘

  Overlay result:

  ┌──────┬────┬──────────────┐
  │  A   │A∩B │   B     C    │
  │ only │    │   only       │
  └──────┴────┴──────────────┘
           ↑
     Boundary edges extracted
     between different polygon IDs
```

### Edge boundary case

When each face has at most one polygon ID:

```
  Face 1        Face 2        Face 3
  (polygon A)   (polygon B)   (no polygon)
  ┌────────────┬─────────────┐
  │            │             │
  │     A      │      B      │
  │            │             │
  └────────────┴─────────────┘
               ↑
  This edge separates A from B → added to OrientedRibbon
  In the no-overlapping-face branch, shared edges are emitted directly as CCW boundaries
```

### Face boundary case

When a face has multiple polygon IDs (true overlap):

```
  ┌─────────────────────────┐
  │        Face f           │
  │  polygons: {A, B}       │
  │  winner: min_state(A,B) │
  │                         │
  │  ┌─────────────┐       │
  │  │   Hole h    │       │
  │  │  (other     │       │
  │  │   polygon)  │       │
  │  └─────────────┘       │
  └─────────────────────────┘

  Outer boundary of face f → CCW segments (outer_ccb)
  Hole boundaries → CW segments (inner_ccbs)
```

## 8. Global Union

The final step computes the union of ALL input polygons:

```
  Input polygons:              Union result:

  ┌────┐   ┌────┐              ┌─────────────┐
  │ A  │   │ B  │              │             │
  │  ┌─┼───┼─┐  │      →       │  Union(A,B) │
  │  │ │   │ │  │              │             │
  └──┼─┘   └─┼──┘              └─────────────┘
     └────────┘

  Outer boundary → CCW → OrientedRibbon.addCCW()
  Any holes      → CW  → OrientedRibbon.addCW()
```

## Complete Pipeline Example

Three overlapping paths creating a T-junction:

```
  Input tiles:

  Path 1 (horizontal):    [P0]──[P1]──[P2]──[P3]
  Path 2 (vertical):             [P4]
                                  │
                                 [P5]
                                  │
                                 [P6]

  Overlaps: P1∩P4, P2∩P4

  Step 1 - Intersections:
    Intersection(1, 4)
    Intersection(2, 4)

  Step 2 - Families:
    Family 0: {(1,4), (2,4)}  ← share polygon 4

  Step 3 - Patches:
    Patch A: {P1, P2}  (horizontal path tiles)
    Patch B: {P4}       (vertical path tile)

  Step 4 - Nodes:
    Node 0: cover={1,2}, state=0
    Node 1: cover={4},   state=1
    Node 0 ←opposite→ Node 1

  Step 5 - Graph coloring:
    Node 0 → state 0 (render as "main")
    Node 1 → state 1 (render as "secondary")

  Step 6 - Overlay:
    Extract boundary between Node 0 and Node 1 regions

  Step 7 - Union:
    Merge all tiles into single outer boundary

  Output: OrientedRibbon with:
    CCW segments → outer boundary of merged shape
    CW segments  → any internal holes
```

## Edge Cases

### Single-Patch Family

When all overlapping polygons form a single connected component:

```
  ┌────┐
  │ A  ├────┐
  │    │ B  │    All connected: {A, B, C} = 1 patch
  └────┤    ├────┐
       │    │ C  │
       └────┤    │
            └────┘

  Special handling: grouped during locateFamilies() with a second box-intersection
  pass, then consumed later by mergeFamilies().
```

### No Overlaps

When polygons don't overlap, the module returns early with just
the global union boundary and no conflict graph.
